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
        // Return only the fully serialized sequence string buffering contents representation directly.
        return get_data().first;
    }

    /**
     * @brief Builds and returns a unified buffer containing all serialized frames along with the frame count.
     * @details Computes necessary sizes, splits payloads if max frame bounds are exceeded, and generates the final output.
     * @return A pair where the first element is the serialized buffer and the second element is the number of frames generated.
     */
    std::pair<std::vector<std::uint8_t>, std::size_t> base_builder::get_data() {
        // Acquire a shared read lock to prevent concurrent modifications during parsing.
        std::shared_lock _lock(shared_buffers_mutex_);

        // Ensure there is at least one payload to avoid serializing an empty frame.
        if (buffers_.empty()) {
            return {{}, 0};
        }

        // Initialize the output buffer where the serialized frames will be written.
        std::vector<std::uint8_t> _output_buffer;

        // Track the total number of frames successfully serialized locally.
        std::size_t _frames_count = 0;

        // Define maximum allowed requests in a single frame payload (fits in 16-bit uint).
        constexpr std::size_t _max_requests_per_frame = 65535; // std::numeric_limits<std::uint16_t>::max()
        // Define maximum frame payload size to avoid integer overflow issues during transmission.
        constexpr std::size_t _max_frame_payload_size = 4294967295 - 131078; // theoretical max limit avoiding overflows.

        // Track the current payload being processed.
        std::size_t _current_index = 0;
        // Store total number of payloads in the local buffer array.
        std::size_t _total_requests = buffers_.size();

        // Process all pending payloads creating frame chunks if required by limits.
        while (_current_index < _total_requests) {
            // Count how many individual payloads are included in this specific chunk.
            std::size_t _requests_in_frame = 0;
            // Accumulate total raw byte size for this chunk payload data.
            std::size_t _frame_payload_size = 0;
            // Record the starting payload index for the current chunk evaluation.
            std::size_t _start_index = _current_index;

            // Iterate over remaining payloads validating size limits per chunk.
            while (_current_index < _total_requests && _requests_in_frame < _max_requests_per_frame) {
                // Determine memory footprint required by the next payload.
                std::size_t _next_size = buffers_[_current_index].size();

                // Check if adding this payload will exceed the maximum frame bytes allowed.
                if (_frame_payload_size + _next_size > _max_frame_payload_size) {
                    // Ensure loop advances even if a single payload is overly large
                    if (_requests_in_frame == 0) {
                        _frame_payload_size += _next_size;
                        _requests_in_frame++;
                        _current_index++;
                    }
                    // Break loop returning back to chunk serialization logic for the current batch.
                    break;
                }

                // Add the evaluated payload size into the total frame weight accumulator.
                _frame_payload_size += _next_size;
                // Increment counter marking an extra payload mapped inside the active chunk.
                _requests_in_frame++;
                // Move index forward checking next pending payload item in line.
                _current_index++;
            }

            // Calculate exact total header bytes memory allocation footprint.
            std::uint32_t _header_size = sizeof(std::uint16_t) + (_requests_in_frame * sizeof(std::uint16_t)) + _frame_payload_size + sizeof(std::uint16_t);

            // Copy header length converting host value tracking endianness limits.
            std::uint32_t _header_size_le = _header_size;
            // Convert length into little endian binary format ensuring correct transmission.
            boost::endian::native_to_little_inplace(_header_size_le);
            // Create pointer tracking memory address for header byte conversion target.
            auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_le);
            // Push exact byte length sequence appending header data.
            _output_buffer.insert(_output_buffer.end(), _header_ptr, _header_ptr + sizeof(_header_size_le));

            // Mark starting offset tracking payload data evaluating CRC checksum.
            std::size_t _crc_start_offset = _output_buffer.size();

            // Set amount of requests parameter tracking limit lengths values.
            std::uint16_t _qty_le = static_cast<std::uint16_t>(_requests_in_frame);
            // Format requests quantity parameter to little endian primitive structure.
            boost::endian::native_to_little_inplace(_qty_le);
            // Construct pointer reading primitive integer byte sequences logically.
            auto* _qty_ptr = reinterpret_cast<const std::uint8_t*>(&_qty_le);
            // Append requests quantity identifier parameter into payload output sequence buffer.
            _output_buffer.insert(_output_buffer.end(), _qty_ptr, _qty_ptr + sizeof(_qty_le));

            // Traverse accepted chunk parameters copying corresponding lengths bytes.
            for (std::size_t _i = _start_index; _i < _current_index; ++_i) {
                // Fetch current buffer element sizing parameters memory payload.
                std::uint16_t _len_le = static_cast<std::uint16_t>(buffers_[_i].size());
                // Adapt parameter memory endianness architecture targeting little endian protocol format.
                boost::endian::native_to_little_inplace(_len_le);
                // Wrap parameter address pointer representation handling payload chunks lengths.
                auto* _len_ptr = reinterpret_cast<const std::uint8_t*>(&_len_le);
                // Feed corresponding length item block inside global output bytes boundary.
                _output_buffer.insert(_output_buffer.end(), _len_ptr, _len_ptr + sizeof(_len_le));
            }

            // Iterate over all valid payloads inside the active chunk bounds.
            for (std::size_t _i = _start_index; _i < _current_index; ++_i) {
                // Append payload content strictly tracking memory sequence parameters.
                _output_buffer.insert(_output_buffer.end(), buffers_[_i].begin(), buffers_[_i].end());
            }

            // Create target crc verification entity managing chunk validation parameter.
            boost::crc_ccitt_type _crc;
            // Evaluate bytes mapping content from start offset evaluating checksum hash.
            _crc.process_bytes(_output_buffer.data() + _crc_start_offset, _output_buffer.size() - _crc_start_offset);
            // Extract computed check sequence parameter variable.
            std::uint16_t _crc_val = _crc.checksum();
            // Translate primitive checksum byte data into correct network transmission representation format.
            boost::endian::native_to_little_inplace(_crc_val);
            // Reinterpret checksum block boundaries memory reference variable format.
            auto* _crc_ptr = reinterpret_cast<const std::uint8_t*>(&_crc_val);
            // Append complete checksum mapping to chunk payload limit trailing element.
            _output_buffer.insert(_output_buffer.end(), _crc_ptr, _crc_ptr + sizeof(_crc_val));

            // Increment the counter tracking the successful frame generated block cleanly.
            _frames_count++;
        }

        // Return the compiled serialized array representation of the complete transaction and its frame quantity natively.
        return std::make_pair(_output_buffer, _frames_count);
    }

    /**
     * @brief Resets the builder state clearing all internal payloads safely.
     */
    void base_builder::reset() {
        // Require exclusive lock preventing state reads while clearing buffers map securely.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Truncate dynamically allocated inner buffers vector avoiding memory dangling limits cleanly.
        buffers_.clear();
        // Return aggregate memory boundary tracking integer to absolute initial sequence mapping natively.
        total_payload_size_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Adds a ping request to the internal buffer.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_ping(boost::uuids::uuid id) {
        // Pre-allocate temporary storage for new ping payload elements.
        std::vector<std::uint8_t> _buffer;
        // Expand internal memory bounds matching standard ping layout constraints (17 bytes).
        _buffer.reserve(17);

        // Append initial opcode identifying incoming network primitive action target.
        _buffer.push_back(aurum::op_code::ping);
        // Feed generated or received uuid referencing ping operation target bounds.
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        // Protect access while inserting data updating global state tracking payload.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Append constructed ping target parameter object shifting memory content ownership.
        buffers_.push_back(std::move(_buffer));
        // Aggregate global payload tracker size memory parameters safely avoiding locking.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Return a mutable self reference enabling sequential payload accumulation chaining.
        return *this;
    }

    /**
     * @brief Adds a ping response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @param exit_code The success or error status code to reply with.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_ping(boost::uuids::uuid id, std::uint8_t exit_code) {
        // Initialize an empty vector handling the localized response payload contents structure.
        std::vector<std::uint8_t> _buffer;
        // Reserve pre-calculated memory size bounding payload size footprint parameters.
        _buffer.reserve(17);

        // Serialize tracking request target parameter variable matching response context context constraint.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Apply success operation result target indicating remote status condition tracking.
        _buffer.push_back(exit_code);

        // Require unique write token locking payload buffers tracking constraints logic variable mapping boundaries.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Commit newly evaluated payload response object moving array pointers ownership directly.
        buffers_.push_back(std::move(_buffer));
        // Keep tracking bytes bounds limits atomically incrementing current state parameter mapping value properly.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Grant ongoing object chaining logic matching fluent architectural reference model properly.
        return *this;
    }

    /**
     * @brief Adds a non-implemented error response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_non_implemented(boost::uuids::uuid id) {
        // Define temporary sequence container storing error state response payload.
        std::vector<std::uint8_t> _buffer;
        // Reserve memory exactly matching the footprint of an error response block (17 bytes).
        _buffer.reserve(17);

        // Embed UUID identifier connecting this payload with the original request parameters.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Emplace non-implemented exit code indicating the requested operation is unsupported.
        _buffer.push_back(aurum::exit_code::non_implemented);

        // Acquire lock ensuring thread-safe access to the shared payload array sequence.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Emplace the new payload array shifting ownership to the builder container state.
        buffers_.push_back(std::move(_buffer));
        // Sum total sizing securely updating memory lengths tracker across threads.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Return active object instance enabling sequential payload operation chaining.
        return *this;
    }

    /**
     * @brief Creates and returns a new request builder.
     * @return A request builder allocated on the stack.
     */
    request_builder frame_builder::as_request() const {
        // Build and return a fresh request builder allocated securely on the stack.
        return request_builder{};
    }

    /**
     * @brief Creates and returns a new response builder.
     * @return A response builder allocated on the stack.
     */
    response_builder frame_builder::as_response() const {
        // Build and return a fresh response builder allocated securely on the stack.
        return response_builder{};
    }

} // namespace aurum::protocol