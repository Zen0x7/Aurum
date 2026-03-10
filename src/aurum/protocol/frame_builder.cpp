// Aurum
// Copyright (C) 2026 Ian Torres
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <aurum/protocol/frame_builder.hpp>
#include <aurum/protocol/op_code.hpp>
#include <aurum/protocol/exit_code.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/crc.hpp>
#include <cstring>
#include <limits>
#include <algorithm>
#include <mutex>

namespace aurum::protocol {

    /**
     * @brief Reserves capacity in the internal buffers container.
     * @param capacity The number of items to reserve space for.
     */
    void base_builder::reserve(std::size_t capacity) {
        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.reserve(capacity);
    }

    /**
     * @brief Builds and returns a unified buffer containing all serialized frames.
     * @param with_header Specifies if a 4-byte length header should be prepended natively.
     * @return A vector of bytes containing the fully serialized frame(s).
     */
    std::vector<std::uint8_t> base_builder::get_buffers(bool with_header) {
        return get_data(with_header);
    }

    /**
     * @brief Builds and returns a unified buffer containing a single serialized frame.
     * @details Computes necessary sizes and generates the final output. Enforces a single frame sequence explicitly natively.
     * @param with_header Specifies if a 4-byte length header should be prepended natively.
     * @return A vector of bytes containing the fully serialized frame.
     */
    std::vector<std::uint8_t> base_builder::get_data(bool with_header) {
        std::shared_lock _lock(shared_buffers_mutex_);

        if (buffers_.empty()) {
            return {};
        }

        std::vector<std::uint8_t> _output_buffer;

        std::size_t _total_requests = buffers_.size();

        constexpr std::size_t _max_requests_per_frame = 65535;
        if (_total_requests > _max_requests_per_frame) {
            throw std::overflow_error("Exceeded maximum number of requests for a single frame.");
        }

        constexpr std::size_t _max_frame_payload_size = 4294967295 - 131078;

        std::size_t _frame_payload_size = 0;

        for (std::size_t _i = 0; _i < _total_requests; ++_i) {
            _frame_payload_size += buffers_[_i].size();
        }

        if (_frame_payload_size > _max_frame_payload_size) {
            throw std::overflow_error("Exceeded maximum payload byte size for a single frame.");
        }

        const std::uint32_t _header_size = sizeof(std::uint16_t) + (_total_requests * sizeof(std::uint16_t)) + _frame_payload_size + sizeof(std::uint16_t);

        if (with_header) {
            std::uint32_t _header_size_le = _header_size;
            boost::endian::native_to_little_inplace(_header_size_le);

            auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_le);
            _output_buffer.insert(_output_buffer.end(), _header_ptr, _header_ptr + sizeof(_header_size_le));
        }

        std::size_t _crc_start_offset = _output_buffer.size();

        std::uint16_t _qty_le = static_cast<std::uint16_t>(_total_requests);
        boost::endian::native_to_little_inplace(_qty_le);
        auto* _qty_ptr = reinterpret_cast<const std::uint8_t*>(&_qty_le);
        _output_buffer.insert(_output_buffer.end(), _qty_ptr, _qty_ptr + sizeof(_qty_le));

        for (std::size_t _i = 0; _i < _total_requests; ++_i) {
            std::uint16_t _len_le = static_cast<std::uint16_t>(buffers_[_i].size());
            boost::endian::native_to_little_inplace(_len_le);
            auto* _len_ptr = reinterpret_cast<const std::uint8_t*>(&_len_le);
            _output_buffer.insert(_output_buffer.end(), _len_ptr, _len_ptr + sizeof(_len_le));
        }

        for (std::size_t _i = 0; _i < _total_requests; ++_i) {
            _output_buffer.insert(_output_buffer.end(), buffers_[_i].begin(), buffers_[_i].end());
        }

        boost::crc_ccitt_type _crc;
        _crc.process_bytes(_output_buffer.data() + _crc_start_offset, _output_buffer.size() - _crc_start_offset);
        std::uint16_t _crc_val = _crc.checksum();
        boost::endian::native_to_little_inplace(_crc_val);
        auto* _crc_ptr = reinterpret_cast<const std::uint8_t*>(&_crc_val);
        _output_buffer.insert(_output_buffer.end(), _crc_ptr, _crc_ptr + sizeof(_crc_val));

        return _output_buffer;
    }

    /**
     * @brief Resets the builder state clearing all internal payloads safely.
     */
    void base_builder::flush() {
        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.clear();
        total_payload_size_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Adds a ping request to the internal buffer.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_ping(boost::uuids::uuid id) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(18);

        _buffer.push_back(ping);
        _buffer.push_back(message_type::request);
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(18, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds an identify request containing the local node identifier.
     * @param node_id The 16-byte identifier representing the active node context.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @param port The optional target port mapping correctly accurately.
     * @param host The optional target host string mapping cleanly correctly.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_identify(boost::uuids::uuid node_id, boost::uuids::uuid id, std::uint16_t port, const std::string& host) {
        std::vector<std::uint8_t> _buffer;
        std::size_t _expected_size = 34;

        if (port != 0 || !host.empty()) {
            _expected_size += 2 + host.size();
        }

        _buffer.reserve(_expected_size);

        _buffer.push_back(identify);
        _buffer.push_back(message_type::request);
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        _buffer.insert(_buffer.end(), node_id.begin(), node_id.end());

        if (port != 0 || !host.empty()) {
            std::uint16_t _port_le = port;
            boost::endian::native_to_little_inplace(_port_le);
            auto* _port_ptr = reinterpret_cast<const std::uint8_t*>(&_port_le);
            _buffer.insert(_buffer.end(), _port_ptr, _port_ptr + sizeof(_port_le));

            if (!host.empty()) {
                _buffer.insert(_buffer.end(), host.begin(), host.end());
            }
        }

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(_expected_size, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a discovery request.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_discovery(boost::uuids::uuid id) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(18);

        _buffer.push_back(discovery);
        _buffer.push_back(message_type::request);
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(18, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a join request.
     * @param websocket_id The UUID of the websocket session joining.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_join(boost::uuids::uuid websocket_id, boost::uuids::uuid id) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(34);

        _buffer.push_back(join);
        _buffer.push_back(message_type::request);
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        _buffer.insert(_buffer.end(), websocket_id.begin(), websocket_id.end());

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(34, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a leave request.
     * @param websocket_id The UUID of the websocket session leaving.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_leave(boost::uuids::uuid websocket_id, boost::uuids::uuid id) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(34);

        _buffer.push_back(leave);
        _buffer.push_back(message_type::request);
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        _buffer.insert(_buffer.end(), websocket_id.begin(), websocket_id.end());

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(34, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a ping response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @param exit_code The success or error status code to reply with.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_ping(boost::uuids::uuid id, std::uint8_t exit_code) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(19);

        _buffer.push_back(ping);
        _buffer.push_back(message_type::response);
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        _buffer.push_back(exit_code);

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(19, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds an identify response containing the node peer tracking context.
     * @param id The transaction ID to respond to mapping request properly.
     * @param node_id The target local node identifier structurally mapped.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_identify(boost::uuids::uuid id, boost::uuids::uuid node_id) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(34);

        _buffer.push_back(identify);
        _buffer.push_back(message_type::response);
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        _buffer.insert(_buffer.end(), node_id.begin(), node_id.end());

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(34, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a non-implemented error response to the internal buffer.
     * @param op The operational code that triggered the error.
     * @param id The transaction ID to respond to.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_non_implemented(std::uint8_t op, boost::uuids::uuid id) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(19);

        _buffer.push_back(op);
        _buffer.push_back(message_type::response);
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        _buffer.push_back(aurum::exit_code::non_implemented);

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(19, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a discovery response containing the active tracked nodes.
     * @param id The transaction ID to respond to.
     * @param nodes A vector of host and port pairs representing connected peers.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_discovery(boost::uuids::uuid id, const std::vector<std::pair<std::string, std::uint16_t>>& nodes) {
        std::vector<std::uint8_t> _buffer;
        std::size_t _expected_size = 18 + 4;

        for (const auto& _node : nodes) {
            _expected_size += 2 + 2 + _node.first.size();
        }

        _buffer.reserve(_expected_size);

        _buffer.push_back(discovery);
        _buffer.push_back(message_type::response);
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        std::uint32_t _nodes_size_le = static_cast<std::uint32_t>(nodes.size());
        boost::endian::native_to_little_inplace(_nodes_size_le);
        auto* _nodes_size_ptr = reinterpret_cast<const std::uint8_t*>(&_nodes_size_le);
        _buffer.insert(_buffer.end(), _nodes_size_ptr, _nodes_size_ptr + sizeof(_nodes_size_le));

        for (const auto& _node : nodes) {
            std::uint16_t _port_le = _node.second;
            boost::endian::native_to_little_inplace(_port_le);
            auto* _port_ptr = reinterpret_cast<const std::uint8_t*>(&_port_le);
            _buffer.insert(_buffer.end(), _port_ptr, _port_ptr + sizeof(_port_le));

            std::uint16_t _host_size_le = static_cast<std::uint16_t>(_node.first.size());
            boost::endian::native_to_little_inplace(_host_size_le);
            auto* _host_size_ptr = reinterpret_cast<const std::uint8_t*>(&_host_size_le);
            _buffer.insert(_buffer.end(), _host_size_ptr, _host_size_ptr + sizeof(_host_size_le));
        }

        for (const auto& _node : nodes) {
            _buffer.insert(_buffer.end(), _node.first.begin(), _node.first.end());
        }

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(_expected_size, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a join response containing the count of registered websocket sessions gracefully safely cleanly naturally.
     * @param id The transaction ID to respond to mapping properly safely.
     * @param count The number of current tracked sessions mapping safely smoothly cleanly.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_join(boost::uuids::uuid id, std::uint64_t count) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(26);

        _buffer.push_back(join);
        _buffer.push_back(message_type::response);
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        std::uint64_t _count_le = count;
        boost::endian::native_to_little_inplace(_count_le);
        auto* _count_ptr = reinterpret_cast<const std::uint8_t*>(&_count_le);
        _buffer.insert(_buffer.end(), _count_ptr, _count_ptr + sizeof(_count_le));

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(26, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Adds a leave response containing the count of removed websocket sessions gracefully safely cleanly naturally.
     * @param id The transaction ID to respond to mapping properly safely.
     * @param count The number of removed tracked sessions mapping safely smoothly cleanly.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_leave(boost::uuids::uuid id, std::uint64_t count) {
        std::vector<std::uint8_t> _buffer;
        _buffer.reserve(26);

        _buffer.push_back(leave);
        _buffer.push_back(message_type::response);
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        std::uint64_t _count_le = count;
        boost::endian::native_to_little_inplace(_count_le);
        auto* _count_ptr = reinterpret_cast<const std::uint8_t*>(&_count_le);
        _buffer.insert(_buffer.end(), _count_ptr, _count_ptr + sizeof(_count_le));

        std::unique_lock _lock(shared_buffers_mutex_);
        buffers_.push_back(std::move(_buffer));
        total_payload_size_.fetch_add(26, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief Creates and returns a new request builder.
     * @return A request builder allocated on the stack.
     */
    request_builder frame_builder::as_request() const {
        return request_builder{};
    }

    /**
     * @brief Creates and returns a new response builder.
     * @return A response builder allocated on the stack.
     */
    response_builder frame_builder::as_response() const {
        return response_builder{};
    }

} // namespace aurum::protocol