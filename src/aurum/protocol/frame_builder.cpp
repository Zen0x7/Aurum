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

        // Define maximum frame payload size to avoid integer overflow issues during transmission.
        constexpr std::size_t _max_frame_payload_size = 4294967295 - 131078; // theoretical max limit avoiding overflows.

        // Initialize an index to track the current position in the payload list.
        std::size_t _current_index = 0;
        // Retrieve the total number of queued payloads from the internal buffer.
        std::size_t _total_requests = buffers_.size();

        // Iterate through all pending payloads to group them into frame chunks.
        while (_current_index < _total_requests) {
            // Define maximum allowed requests in a single frame payload (fits in 16-bit uint).
            constexpr std::size_t _max_requests_per_frame = 65535;
            // Initialize a counter to track the number of payloads in the current chunk.
            std::size_t _requests_in_frame = 0;
            // Initialize an accumulator for the total byte size of the current chunk's payload.
            std::size_t _frame_payload_size = 0;
            // Record the starting payload index to reference the subset for the current chunk.
            std::size_t _start_index = _current_index;

            // Group payloads until the chunk hits the maximum allowed items or the end of the list.
            while (_current_index < _total_requests && _requests_in_frame < _max_requests_per_frame) {
                // Get the byte size of the payload currently being evaluated.
                const std::size_t _next_size = buffers_[_current_index].size();

                // Stop grouping if adding the current payload exceeds the maximum frame byte size.
                if (_frame_payload_size + _next_size > _max_frame_payload_size) {
                    // Forcefully include the payload if it is the only item in the chunk to prevent an infinite loop.
                    if (_requests_in_frame == 0) {
                        _frame_payload_size += _next_size;
                        _requests_in_frame++;
                        _current_index++;
                    }
                    // Exit the grouping loop to begin serializing the current chunk.
                    break;
                }

                // Add the byte size of the current payload to the chunk's total payload size accumulator.
                _frame_payload_size += _next_size;
                // Increment the counter tracking the total number of payloads in the current chunk.
                _requests_in_frame++;
                // Advance the index to evaluate the next queued payload in the buffer.
                _current_index++;
            }

            // Calculate the total number of bytes required for the frame header.
            const std::uint32_t _header_size = sizeof(std::uint16_t) + (_requests_in_frame * sizeof(std::uint16_t)) + _frame_payload_size + sizeof(std::uint16_t);

            // Copy the calculated header size into a local variable to be encoded.
            std::uint32_t _header_size_le = _header_size;
            // Convert the header size from the native endianness to little endian format.
            boost::endian::native_to_little_inplace(_header_size_le);
            // Reinterpret the converted 32-bit integer as an array of bytes.
            auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_le);
            // Append the little-endian encoded header size bytes to the final output buffer.
            _output_buffer.insert(_output_buffer.end(), _header_ptr, _header_ptr + sizeof(_header_size_le));

            // Record the current output buffer size to use as the starting point for CRC calculation.
            std::size_t _crc_start_offset = _output_buffer.size();

            // Cast the number of requests in the frame into a 16-bit unsigned integer.
            std::uint16_t _qty_le = static_cast<std::uint16_t>(_requests_in_frame);
            // Convert the requests quantity to little-endian format.
            boost::endian::native_to_little_inplace(_qty_le);
            // Reinterpret the 16-bit requests quantity integer as an array of bytes.
            auto* _qty_ptr = reinterpret_cast<const std::uint8_t*>(&_qty_le);
            // Append the encoded quantity value to the output buffer.
            _output_buffer.insert(_output_buffer.end(), _qty_ptr, _qty_ptr + sizeof(_qty_le));

            // Iterate over all payload items included in this frame chunk.
            for (std::size_t _i = _start_index; _i < _current_index; ++_i) {
                // Determine the size in bytes of the current payload and store it as a 16-bit integer.
                std::uint16_t _len_le = static_cast<std::uint16_t>(buffers_[_i].size());
                // Convert the size length value into little-endian architecture format.
                boost::endian::native_to_little_inplace(_len_le);
                // Cast the 16-bit length value into an addressable byte pointer.
                auto* _len_ptr = reinterpret_cast<const std::uint8_t*>(&_len_le);
                // Push the encoded little-endian payload length into the frame's output buffer.
                _output_buffer.insert(_output_buffer.end(), _len_ptr, _len_ptr + sizeof(_len_le));
            }

            // Iterate sequentially over all the buffered payloads assigned to the current frame.
            for (std::size_t _i = _start_index; _i < _current_index; ++_i) {
                // Copy the actual raw payload bytes directly into the combined output buffer.
                _output_buffer.insert(_output_buffer.end(), buffers_[_i].begin(), buffers_[_i].end());
            }

            // Initialize a CCITT standard CRC calculator instance.
            boost::crc_ccitt_type _crc;
            // Feed the constructed frame data (from quantity to end of payload) into the CRC calculator.
            _crc.process_bytes(_output_buffer.data() + _crc_start_offset, _output_buffer.size() - _crc_start_offset);
            // Extract the generated 16-bit checksum value from the CRC instance.
            std::uint16_t _crc_val = _crc.checksum();
            // Convert the calculated checksum to little-endian network representation.
            boost::endian::native_to_little_inplace(_crc_val);
            // Cast the checksum value to a byte pointer to allow appending.
            auto* _crc_ptr = reinterpret_cast<const std::uint8_t*>(&_crc_val);
            // Append the checksum bytes at the very end of the constructed frame chunk.
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
    void base_builder::flush() {
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
        // Allocate an empty vector for storing the newly requested ping payload.
        std::vector<std::uint8_t> _buffer;
        // Pre-allocate exactly 17 bytes to prevent memory reallocation during construction.
        _buffer.reserve(17);

        // Add the explicit ping opcode byte to the beginning of the payload.
        _buffer.push_back(ping);
        // Append the 16-byte UUID referencing the current operation to the payload buffer.
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        // Acquire an exclusive lock over the shared buffers sequence to ensure thread safety.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Emplace the newly constructed ping payload into the internal storage using move semantics.
        buffers_.push_back(std::move(_buffer));
        // Atomically increase the global payload counter by the exact 17 bytes appended.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Return a reference to the builder instance to support method chaining.
        return *this;
    }

    /**
     * @brief Adds a ping response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @param exit_code The success or error status code to reply with.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_ping(boost::uuids::uuid id, std::uint8_t exit_code) {
        // Allocate an empty vector to store the contents of the ping response payload.
        std::vector<std::uint8_t> _buffer;
        // Pre-allocate the memory needed for a standard 17-byte ping response.
        _buffer.reserve(17);

        // Copy the target 16-byte transaction UUID into the response payload.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Append the provided 1-byte exit status code to signify operation outcome.
        _buffer.push_back(exit_code);

        // Acquire an exclusive lock on the shared payload buffers to protect concurrent writes.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Transfer ownership of the built response payload into the builder's local storage array.
        buffers_.push_back(std::move(_buffer));
        // Atomically update the total tracked memory footprint by the newly inserted 17 bytes.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Return the current builder reference to facilitate chained function calls.
        return *this;
    }

    /**
     * @brief Adds a non-implemented error response to the internal buffer.
     * @param id The transaction ID to respond to.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_non_implemented(boost::uuids::uuid id) {
        // Allocate a temporary vector for holding the error response payload bytes.
        std::vector<std::uint8_t> _buffer;
        // Pre-allocate 17 bytes to match the exact size of a non-implemented response.
        _buffer.reserve(17);

        // Insert the 16-byte transaction UUID into the response payload buffer.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Append the specific exit code denoting that the requested opcode is not supported.
        _buffer.push_back(aurum::exit_code::non_implemented);

        // Acquire an exclusive lock on the shared payload array to prevent data races.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Transfer the temporary payload buffer into the builder's state using move semantics.
        buffers_.push_back(std::move(_buffer));
        // Atomically increment the total payload size counter by the 17 bytes added.
        total_payload_size_.fetch_add(17, std::memory_order_relaxed);

        // Return a reference to this builder instance to allow method chaining.
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