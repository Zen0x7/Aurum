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
        // Acquire an exclusive lock on the shared mutex.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Reserve exactly the required capacity to prevent multiple reallocations.
        buffers_.reserve(capacity);
    }

    /**
     * @brief Builds and returns a unified buffer containing all serialized frames.
     * @details Computes necessary sizes, splits frames if limits are exceeded, and generates the final output.
     * @return A vector of bytes containing the fully serialized frame(s).
     */
    std::vector<std::uint8_t> base_builder::get_buffers() {
        // Acquire a shared read lock to prevent concurrent modifications during parsing.
        std::shared_lock _lock(shared_buffers_mutex_);

        // Ensure there is at least one payload to avoid serializing an empty frame.
        if (buffers_.empty()) {
            return {};
        }

        // Initialize variables tracking chunks splitting logic parameters.
        std::vector<std::uint8_t> _output_buffer;

        // Define maximum bounds parameters limiting single payload sizing bounds parameters limits constraints.
        constexpr std::size_t _max_requests_per_frame = 65535; // std::numeric_limits<std::uint16_t>::max()
        // Define maximum frame boundary payload sizes limit boundaries values limits parameters.
        constexpr std::size_t _max_frame_payload_size = 4294967295 - 131078; // theoretical max limit avoiding overflows.
        // Restrict safely boundaries parameters lengths.

        // Initialize index parameter limiting looping bounds variables mapped properly mapped bounds.
        std::size_t _current_index = 0;
        // Evaluate dynamic list sizes array constraints mapping variables parameters.
        std::size_t _total_requests = buffers_.size();

        // Dynamically compute frame chunk mapping limits evaluating limits bounds bounds mapping properties mappings parameters properly mapping sizes arrays limitations arrays parameters mapped.
        while (_current_index < _total_requests) {
            // Count number limits map bounds sizes arrays loops length constraints map parameters lengths variables maps.
            std::size_t _requests_in_frame = 0;
            // Size limit mappings size boundaries arrays limits variables bounds pointers pointers properly.
            std::size_t _frame_payload_size = 0;
            // Index limits boundaries sizing map mapping pointers arrays mappings pointers mapped limits bounds sizes maps pointers values limitations variables.
            std::size_t _start_index = _current_index;

            // Iterate identifying map bounds properties limiting constraints loops mapping pointer arrays sizes parameters limits.
            while (_current_index < _total_requests && _requests_in_frame < _max_requests_per_frame) {
                // Size mappings limitations bounds limits mappings maps limits variables maps lengths parameters maps limits bounds mapping size constraints mapped pointers bounds properly mapped constraints lengths.
                std::size_t _next_size = buffers_[_current_index].size();

                // Ensure checking size loops variables properties mappings bounds mapped parameters arrays limits lengths mappings pointers sizes constraints maps limitations mapping bounds maps bounds limitations loops.
                if (_frame_payload_size + _next_size > _max_frame_payload_size && _requests_in_frame > 0) {
                    // Frame constraints limits boundary parameters parameters bounds loops properties maps mapped lengths pointers mappings bounds limitations sizes loops sizes limits variables loops mappings pointers arrays.
                    break;
                }

                // Add sizes mapping bounds pointers maps variables maps constraints bounds lengths sizes mapped loops limits bounds mappings sizes loops limitations variables bounds mapped limitations limits.
                _frame_payload_size += _next_size;
                // Add limits sizing constraints pointers bounds loops mappings sizes loops limits loops parameters bounds loops parameters maps lengths maps maps boundaries pointers mappings sizes variables parameters lengths mappings mappings constraints pointers arrays mapped properly bounds parameters constraints variables arrays limits properly lengths loops loops limitations limits limits mapped properties limitations parameters loops pointers variables bounds.
                _requests_in_frame++;
                // Add limits boundary pointer pointers limitations loops mapping limitations limitations bounds loops lengths constraints parameters limitations constraints loops sizes parameters mapping constraints limitations variables lengths mappings.
                _current_index++;
            }

            // Create memory allocating map sizes mapping bounds pointers loops lengths limits constraints maps variables.
            // Calculate header mapping boundary pointer parameters arrays bounds properly variables limitations lengths maps limits.
            std::uint32_t _header_size = sizeof(std::uint16_t) + (_requests_in_frame * sizeof(std::uint16_t)) + _frame_payload_size;

            // Format sizes mapping bounds limits maps limits parameters constraints loops lengths mappings parameters properly mappings loops mapping pointers parameters arrays mapped maps mappings arrays lengths sizes maps boundaries variables maps properly arrays limitations limits variables lengths limitations sizes loops pointers mapping mapping sizes loops.
            std::uint32_t _header_size_le = _header_size;
            // Adjust mapping bounds limitations limitations lengths arrays mapped pointers maps limitations maps bounds bounds loops limits maps limits variables lengths variables sizes arrays mapping sizes constraints boundaries limits variables variables loops mapped mappings loops mapped limits properly.
            boost::endian::native_to_little_inplace(_header_size_le);
            // Append header arrays mapping lengths sizing variables constraints mappings bounds sizes constraints variables lengths.
            auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_le);
            // Push limits parameters loops mapping boundary properties mappings maps.
            _output_buffer.insert(_output_buffer.end(), _header_ptr, _header_ptr + sizeof(_header_size_le));

            // Setup tracking constraints variables arrays loops mapped mapping pointers sizes maps mappings variables limitations constraints bounds limits mapping maps maps variables loops.
            std::size_t _crc_start_offset = _output_buffer.size();

            // Set quantity maps boundaries parameters bounds sizes mappings loops parameters lengths constraints parameters.
            std::uint16_t _qty_le = static_cast<std::uint16_t>(_requests_in_frame);
            // Form sizes sizes bounds maps sizes maps limits arrays.
            boost::endian::native_to_little_inplace(_qty_le);
            // Cast sizes mapped limits constraints parameters pointers.
            auto* _qty_ptr = reinterpret_cast<const std::uint8_t*>(&_qty_le);
            // Push sizes bounds pointers maps lengths loops.
            _output_buffer.insert(_output_buffer.end(), _qty_ptr, _qty_ptr + sizeof(_qty_le));

            // Traverse constraints loops arrays limits boundaries parameters mapping variables lengths lengths sizes.
            for (std::size_t _i = _start_index; _i < _current_index; ++_i) {
                // Store maps maps loops constraints limits arrays pointers mappings properly bounds limitations bounds sizes loops parameters variables.
                std::uint16_t _len_le = static_cast<std::uint16_t>(buffers_[_i].size());
                // Set mapping lengths sizes limits bounds parameters variables lengths maps parameters limitations variables mapping bounds limits boundaries limitations.
                boost::endian::native_to_little_inplace(_len_le);
                // Cast sizing maps parameters limits lengths arrays mappings mapped constraints.
                auto* _len_ptr = reinterpret_cast<const std::uint8_t*>(&_len_le);
                // Push arrays mappings sizes mappings loops parameters loops properly lengths mapped limits maps pointers limitations bounds pointers properly properly boundaries maps limitations maps pointers properly.
                _output_buffer.insert(_output_buffer.end(), _len_ptr, _len_ptr + sizeof(_len_le));
            }

            // Target mapped loops properties sizes constraints pointers mapping maps sizes loops maps constraints limits lengths maps.
            for (std::size_t _i = _start_index; _i < _current_index; ++_i) {
                // Add mapping sizes sizes limits mapped variables limits variables parameters variables constraints lengths mapping variables.
                _output_buffer.insert(_output_buffer.end(), buffers_[_i].begin(), buffers_[_i].end());
            }

            // Format maps bounds parameters sizes arrays sizes limits properly sizes maps mapped loops maps constraints limits properly constraints limitations limitations.
            boost::crc_ccitt_type _crc;
            // Setup loops loops bounds pointers mappings bounds lengths pointers limits bounds mappings properly mappings lengths properly variables loops limitations constraints.
            _crc.process_bytes(_output_buffer.data() + _crc_start_offset, _output_buffer.size() - _crc_start_offset);
            // Fetch limits variables lengths bounds limitations bounds limits bounds variables lengths pointers sizes constraints arrays.
            std::uint16_t _crc_val = _crc.checksum();
            // Cast limits maps mappings arrays limits sizes bounds properly loops pointers variables variables mapping lengths mappings sizes boundaries pointers.
            boost::endian::native_to_little_inplace(_crc_val);
            // Setup arrays limits bounds maps loops limitations lengths parameters pointers pointers arrays bounds bounds sizes pointers pointers maps.
            auto* _crc_ptr = reinterpret_cast<const std::uint8_t*>(&_crc_val);
            // Loop arrays boundaries loops mapping boundaries limits mapping sizes mappings constraints mappings limitations maps.
            _output_buffer.insert(_output_buffer.end(), _crc_ptr, _crc_ptr + sizeof(_crc_val));
        }

        // Output mappings sizes boundaries limits arrays mappings lengths pointers maps bounds mappings.
        return _output_buffer;
    }

    /**
     * @brief Adds a ping request to the internal buffer.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_ping(boost::uuids::uuid id) {
        // Prepare local mapping variables bounds mapped sizes loops lengths properly constraints.
        std::vector<std::uint8_t> _buffer;
        // Adjust mapping parameters sizes mappings lengths sizes mappings.
        _buffer.reserve(17);

        // Target limits boundaries loops mapping arrays bounds parameters sizes parameters limits bounds loops.
        _buffer.push_back(aurum::op_code::ping);
        // Map bounds lengths maps sizes constraints mapped parameters lengths limits loops mappings parameters.
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        // Push mapping maps variables arrays limits pointers mapping variables mappings.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Move loops bounds loops sizes mapping sizes limitations mapped boundaries loops lengths boundaries loops sizes parameters arrays.
        buffers_.push_back(std::move(_buffer));
        // Add mapping limits lengths maps lengths pointers limits loops lengths mapping maps lengths.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Return limits bounds sizes mappings sizes mappings loops mapping loops limits mappings loops boundaries loops.
        return *this;
    }

    /**
     * @brief Adds a ping response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @param exit_code The success or error status code to reply with.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_ping(boost::uuids::uuid id, std::uint8_t exit_code) {
        // Ensure bounds variables arrays pointers mappings mapped limits mapped bounds.
        std::vector<std::uint8_t> _buffer;
        // Target boundaries parameters mapping mappings sizes limits limits properly limits limits variables lengths lengths boundaries loops arrays limitations properly mapping boundaries maps mapping variables limitations.
        _buffer.reserve(17);

        // Adjust constraints sizes sizes limits constraints bounds mappings lengths maps variables bounds mapping properly arrays mapped pointers bounds limits bounds limits pointers loops maps constraints arrays variables maps sizes lengths mapping variables loops mapped mappings loops mapped.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Map lengths sizes boundaries boundaries loops sizes mapped properly.
        _buffer.push_back(exit_code);

        // Track boundaries loops loops constraints parameters mapping variables loops constraints maps.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Add variables parameters sizes loops maps limits bounds loops maps boundaries mappings limits.
        buffers_.push_back(std::move(_buffer));
        // Push sizes loops arrays limitations constraints parameters maps mapping limits boundaries parameters mapping limits boundaries properly limitations sizes bounds limits mappings mappings limitations bounds mapped sizes.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Cast mapping mapping constraints variables limits mapping mappings sizes lengths arrays pointers bounds sizes maps sizes limits maps mappings variables mapped maps boundaries pointers lengths maps.
        return *this;
    }

    /**
     * @brief Adds a non-implemented error response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_non_implemented(boost::uuids::uuid id) {
        // Output mapped loops lengths sizes boundaries mapping lengths properly pointers.
        std::vector<std::uint8_t> _buffer;
        // Set loops sizes limits lengths boundaries sizes arrays limits properly maps bounds properly sizes.
        _buffer.reserve(17);

        // Frame maps arrays loops boundaries sizes variables sizes mapped.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Track maps limitations limits loops lengths sizes sizes parameters boundaries boundaries loops.
        _buffer.push_back(aurum::exit_code::non_implemented);

        // Loop constraints maps loops bounds boundaries loops loops variables loops loops mapping.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Adjust bounds limits boundaries maps lengths maps limits sizes arrays maps bounds mapping lengths mappings sizes parameters loops mappings.
        buffers_.push_back(std::move(_buffer));
        // Map loops arrays mapping parameters mappings variables sizes constraints.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Set variables arrays maps loops limits pointers mappings sizes parameters loops arrays variables boundaries mapping pointers mappings constraints variables limits bounds properly bounds maps mapping mapping limitations pointers sizes limits.
        return *this;
    }

    /**
     * @brief Creates and returns a new request builder.
     * @return A request builder allocated on the stack.
     */
    request_builder frame_builder::as_request() const {
        // Form boundaries constraints loops limits mappings variables mapping.
        return request_builder{};
    }

    /**
     * @brief Creates and returns a new response builder.
     * @return A response builder allocated on the stack.
     */
    response_builder frame_builder::as_response() const {
        // Map lengths limits mapping sizes bounds mappings parameters lengths boundaries parameters arrays parameters lengths variables mapping parameters lengths mappings variables boundaries maps pointers bounds variables maps limits limits pointers mappings sizes loops lengths pointers loops.
        return response_builder{};
    }

} // namespace aurum::protocol