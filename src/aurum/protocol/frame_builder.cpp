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
        // Pre-allocate exactly 18 bytes to prevent memory reallocation during construction.
        _buffer.reserve(18);

        // Add the explicit ping opcode byte to the beginning of the payload.
        _buffer.push_back(ping);
        // Add the request message type byte.
        _buffer.push_back(message_type::request);
        // Append the 16-byte UUID referencing the current operation to the payload buffer.
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        // Acquire an exclusive lock over the shared buffers sequence to ensure thread safety.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Emplace the newly constructed ping payload into the internal storage using move semantics.
        buffers_.push_back(std::move(_buffer));
        // Atomically increase the global payload counter by the exact 18 bytes appended.
        total_payload_size_.fetch_add(18, std::memory_order_relaxed);

        // Return a reference to the builder instance to support method chaining.
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
        // Allocate a dedicated array matching identify payload memory footprint structures securely.
        std::vector<std::uint8_t> _buffer;
        // Start counting the required payload capacity initialized to 34 bytes for the basic payload structure.
        std::size_t _expected_size = 34;

        // Evaluate if either the port or host variables were supplied.
        if (port != 0 || !host.empty()) {
            // Include space to allocate a 16-bit numeric port constraint and the string length limit bounds accurately.
            _expected_size += 2 + host.size();
        }

        // Reserve vector space pre-calculating boundary limit requirements to avoid automatic reallocation.
        _buffer.reserve(_expected_size);

        // Define the opcode context identifier as identify structurally mapped accurately.
        _buffer.push_back(identify);
        // Declare this message specifically as an outgoing request natively.
        _buffer.push_back(message_type::request);
        // Forward mapped UUID explicitly tracking transmission operations limits logically securely.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Track unique structural identifier matching local state context explicitly seamlessly correctly safely.
        _buffer.insert(_buffer.end(), node_id.begin(), node_id.end());

        // Process optional arguments appending corresponding dynamic payload natively matching formats.
        if (port != 0 || !host.empty()) {
            // Structure listening port variable natively properly correctly completely matching logic appropriately natively.
            std::uint16_t _port_le = port;
        // Decode the integer to match little-endian network architecture.
            boost::endian::native_to_little_inplace(_port_le);
        // Interpret the little-endian integer variable as a byte pointer.
            auto* _port_ptr = reinterpret_cast<const std::uint8_t*>(&_port_le);
        // Append the 2 bytes of the port into the payload buffer.
            _buffer.insert(_buffer.end(), _port_ptr, _port_ptr + sizeof(_port_le));

        // Evaluate if the host string parameter has content.
            if (!host.empty()) {
            // Append the raw character bytes of the host string into the end of the buffer array.
                _buffer.insert(_buffer.end(), host.begin(), host.end());
            }
        }

    // Acquire an exclusive lock on the shared buffers to protect concurrent writes.
        std::unique_lock _lock(shared_buffers_mutex_);
    // Emplace the fully configured identify payload into the internal storage safely.
        buffers_.push_back(std::move(_buffer));
    // Increment the global payload size counter by the exact computed size.
        total_payload_size_.fetch_add(_expected_size, std::memory_order_relaxed);

    // Return the builder instance reference for method chaining.
        return *this;
    }

    /**
     * @brief Adds a discovery request.
     * @param id An optional explicit transaction ID, generated automatically if not provided.
     * @return A reference to the active builder instance for method chaining.
     */
    request_builder& request_builder::add_discovery(boost::uuids::uuid id) {
    // Initialize an empty vector for holding the discovery request payload bytes.
        std::vector<std::uint8_t> _buffer;
    // Pre-allocate the memory needed for a standard 18-byte discovery request.
        _buffer.reserve(18); // opcode (1) + type (1) + tx_id (16) = 18

    // Append the discovery target opcode safely to the payload array.
        _buffer.push_back(discovery);
    // Add the explicitly bound message type indicating an outbound request.
        _buffer.push_back(message_type::request);
    // Emplace the transaction ID UUID elements into the array.
        _buffer.insert(_buffer.end(), id.begin(), id.end());

    // Secure an exclusive access lock on the thread-safe payload buffering container map.
        std::unique_lock _lock(shared_buffers_mutex_);
    // Store the prepared request buffer instance safely inside the global arrays.
        buffers_.push_back(std::move(_buffer));
    // Instruct the overall buffer counter explicitly updating its sequence.
        total_payload_size_.fetch_add(18, std::memory_order_relaxed);

    // Provide the method chaining context return binding safely.
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
        // Pre-allocate the memory needed for a standard 19-byte ping response.
        _buffer.reserve(19);

        // Add the ping opcode byte to the beginning of the payload.
        _buffer.push_back(ping);
        // Add the response message type byte.
        _buffer.push_back(message_type::response);
        // Copy the target 16-byte transaction UUID into the response payload.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Append the provided 1-byte exit status code to signify operation outcome.
        _buffer.push_back(exit_code);

        // Acquire an exclusive lock on the shared payload buffers to protect concurrent writes.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Transfer ownership of the built response payload into the builder's local storage array.
        buffers_.push_back(std::move(_buffer));
        // Atomically update the total tracked memory footprint by the newly inserted 19 bytes.
        total_payload_size_.fetch_add(19, std::memory_order_relaxed);

        // Return the current builder reference to facilitate chained function calls.
        return *this;
    }

    /**
     * @brief Adds an identify response containing the node peer tracking context.
     * @param id The transaction ID to respond to mapping request properly.
     * @param node_id The target local node identifier structurally mapped.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_identify(boost::uuids::uuid id, boost::uuids::uuid node_id) {
        // Allocate an array mapping response identity correctly checking structural limits.
        std::vector<std::uint8_t> _buffer;
        // Reserve exact mapped space mapping boundaries natively completely safely.
        _buffer.reserve(34); // opcode (1) + type (1) + tx_id (16) + node_id (16) = 34

        // Append explicit identify target mapped opcode correctly.
        _buffer.push_back(identify);
        // Attach valid transmission response mapped token efficiently.
        _buffer.push_back(message_type::response);
        // Copy transaction context evaluating parameters accurately natively.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Attach node structural identifiers safely tracking payload content directly.
        _buffer.insert(_buffer.end(), node_id.begin(), node_id.end());

        // Protect shared mapping references safely binding execution completely securely.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Attach constructed sequence block dynamically matching buffer target layout accurately.
        buffers_.push_back(std::move(_buffer));
        // Expand tracking payload variables mapping specific footprint allocations correctly natively.
        total_payload_size_.fetch_add(34, std::memory_order_relaxed);

        // Route self memory pointer completely executing chains reliably.
        return *this;
    }

    /**
     * @brief Adds a non-implemented error response to the internal buffer.
     * @param op The operational code that triggered the error.
     * @param id The transaction ID to respond to.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_non_implemented(std::uint8_t op, boost::uuids::uuid id) {
        // Allocate a temporary vector for holding the error response payload bytes.
        std::vector<std::uint8_t> _buffer;
        // Pre-allocate 19 bytes to match the exact size of a non-implemented response.
        _buffer.reserve(19);

        // Add the original opcode byte to the beginning of the payload.
        _buffer.push_back(op);
        // Add the response message type byte.
        _buffer.push_back(message_type::response);
        // Insert the 16-byte transaction UUID into the response payload buffer.
        _buffer.insert(_buffer.end(), id.begin(), id.end());
        // Append the specific exit code denoting that the requested opcode is not supported.
        _buffer.push_back(aurum::exit_code::non_implemented);

        // Acquire an exclusive lock on the shared payload array to prevent data races.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Transfer the temporary payload buffer into the builder's state using move semantics.
        buffers_.push_back(std::move(_buffer));
        // Atomically increment the total payload size counter by the 19 bytes added.
        total_payload_size_.fetch_add(19, std::memory_order_relaxed);

        // Return a reference to this builder instance to allow method chaining.
        return *this;
    }

    /**
     * @brief Adds a discovery response containing the active tracked nodes.
     * @param id The transaction ID to respond to.
     * @param nodes A vector of host and port pairs representing connected peers.
     * @return A reference to the active builder instance for method chaining.
     */
    response_builder& response_builder::add_discovery(boost::uuids::uuid id, const std::vector<std::pair<std::string, std::uint16_t>>& nodes) {
        // Initialize an empty vector for holding the discovery response payload elements.
        std::vector<std::uint8_t> _buffer;
        // Declare the base payload size boundary mapping the header, message type, ID, and list length.
        std::size_t _expected_size = 18 + 4; // opcode (1) + type (1) + tx_id (16) + nodes_size (4)

        // Loop through all nodes to compute their required capacity.
        for (const auto& _node : nodes) {
            // Expand the payload size adding the 2-byte port, 2-byte host string length, and the dynamic string length.
            _expected_size += 2 + 2 + _node.first.size(); // port (2) + host_size (2) + host_length
        }

        // Allocate vector capacity matching exactly the evaluated payload length to prevent reallocations.
        _buffer.reserve(_expected_size);

        // Add the explicitly bound discovery opcode.
        _buffer.push_back(discovery);
        // Define the current payload stream as a network response payload type.
        _buffer.push_back(message_type::response);
        // Insert the transaction identifier required for the response matching context.
        _buffer.insert(_buffer.end(), id.begin(), id.end());

        // Construct a 32-bit limit indicator tracking the sequence length of the connected elements array.
        std::uint32_t _nodes_size_le = static_cast<std::uint32_t>(nodes.size());
        // Convert the structural length value format from native architectural little-endian rules.
        boost::endian::native_to_little_inplace(_nodes_size_le);
        // Retrieve the raw memory map referencing the little-endian sequence boundaries constraint structure safely.
        auto* _nodes_size_ptr = reinterpret_cast<const std::uint8_t*>(&_nodes_size_le);
        // Add the integer limits boundaries mapping elements.
        _buffer.insert(_buffer.end(), _nodes_size_ptr, _nodes_size_ptr + sizeof(_nodes_size_le));

        // Iterate sequentially appending the metadata bindings constraints to represent individual tracked limits.
        for (const auto& _node : nodes) {
            // Extract the 16-bit matching numeric representation explicitly.
            std::uint16_t _port_le = _node.second;
            // Decode numeric architecture limit formats structurally logically logically.
            boost::endian::native_to_little_inplace(_port_le);
            // Access the integer memory byte pointer sequence structure directly.
            auto* _port_ptr = reinterpret_cast<const std::uint8_t*>(&_port_le);
            // Emplace the mapped numeric port limits values.
            _buffer.insert(_buffer.end(), _port_ptr, _port_ptr + sizeof(_port_le));

            // Convert string bounds mapping natively matching little-endian length.
            std::uint16_t _host_size_le = static_cast<std::uint16_t>(_node.first.size());
            // Format architectural limitations.
            boost::endian::native_to_little_inplace(_host_size_le);
            // Assign pointer to extract internal character arrays structures logic length limits.
            auto* _host_size_ptr = reinterpret_cast<const std::uint8_t*>(&_host_size_le);
            // Append length mapping to active memory constraints bounds explicitly correctly.
            _buffer.insert(_buffer.end(), _host_size_ptr, _host_size_ptr + sizeof(_host_size_le));
        }

        // Iterate over the nodes again to sequentially append their host strings right after the metadata bounds list.
        for (const auto& _node : nodes) {
            // Copy all characters composing the specific host string exactly into the payload array.
            _buffer.insert(_buffer.end(), _node.first.begin(), _node.first.end());
        }

        // Guarantee mutually exclusive write access to the central memory container to preserve sequence ordering.
        std::unique_lock _lock(shared_buffers_mutex_);
        // Provide the generated buffer chunk strictly into the output collection container.
        buffers_.push_back(std::move(_buffer));
        // Push atomic memory tracking properties safely.
        total_payload_size_.fetch_add(_expected_size, std::memory_order_relaxed);

        // Chain builder invocation securely.
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