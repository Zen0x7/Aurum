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

#include <aurum/session_kernel.hpp>

#include <aurum/state.hpp>
#include <aurum/session.hpp>
#include <iostream>
#include <boost/crc.hpp>
#include <boost/endian/conversion.hpp>
#include <cstring>

namespace aurum {
    session_kernel::session_kernel(std::shared_ptr<state> state) : state_(std::move(state)) {

    }

    std::shared_ptr<const std::vector<std::uint8_t>> session_kernel::handle(
            const std::shared_ptr<std::vector<std::uint8_t>> &frame,
            std::shared_ptr<session> session) {

        // Verify frame boundaries are at least minimally valid
        if (!frame || frame->size() < 2) {
            // Drop invalid frames returning a null pointer
            return nullptr;
        }

        // Calculate a CRC16 object targeting the CCITT polynomial standard
        boost::crc_ccitt_type _crc;

        // Advance frame checking bounds towards the end of payload excluding checksum parameter
        _crc.process_bytes(frame->data(), frame->size() - 2);

        // Define variable container checking for expected valid crc parameter
        std::uint16_t _expected_crc;

        // Memory map pointer copy dumping last bits onto primitive
        std::memcpy(&_expected_crc, frame->data() + frame->size() - 2, sizeof(_expected_crc));

        // Ensure conversion back targeting native hardware format
        boost::endian::little_to_native_inplace(_expected_crc);

        // Cross-validate checksums avoiding payloads tampering scenarios
        if (_crc.checksum() != _expected_crc) {
            // Ignore frame on invalid CRC
            return nullptr;
        }

        // Map initial payload size cursor zero out of parsing bound
        std::size_t _offset = 0;

        // Avoid progressing missing limits bounds
        if (frame->size() - 2 < sizeof(std::uint16_t)) {
            // Skip invalid sizes limits completely
            return nullptr;
        }

        // Initialize parameter tracking the number of requests embedded in the payload
        std::uint16_t _number_of_requests = 0;

        // Copy the number of requests primitive value from the memory offset directly
        std::memcpy(&_number_of_requests, frame->data() + _offset, sizeof(_number_of_requests));

        // Convert the requests count parameter from network little-endian to native architecture
        boost::endian::little_to_native_inplace(_number_of_requests);

        // Advance the offset parsing cursor forward by the size of the counter token
        _offset += sizeof(std::uint16_t);

        // Prepare local vector storing lengths mapping each parsed payload sequentially
        std::vector<std::uint16_t> _requests_lengths;

        // Pre-allocate vector capacity preventing dynamic reallocation bottlenecks during processing
        _requests_lengths.reserve(_number_of_requests);

        // Ensure payload size physically covers the length array layout structurally
        if (frame->size() - 2 - _offset < _number_of_requests * sizeof(std::uint16_t)) {
            // Abort parsing frame processing boundaries rejecting corrupted packet
            return nullptr;
        }

        // Iterate extracting size structures dynamically for every single bundled payload
        for (std::uint16_t _index = 0; _index < _number_of_requests; _index++) {
            // Initialize length primitive memory target explicitly for current payload block
            std::uint16_t _n_request_length = 0;

            // Extract the length mapping directly from network bounds pointer safely
            std::memcpy(&_n_request_length, frame->data() + _offset, sizeof(_n_request_length));

            // Adapt the length value layout translating from little-endian protocol to native
            boost::endian::little_to_native_inplace(_n_request_length);

            // Push evaluated payload size tracking parameter into local tracking constraints array
            _requests_lengths.push_back(_n_request_length);

            // Move the bounds cursor forwarding parsing parameters to the next limit bounds object
            _offset += sizeof(std::uint16_t);
        }

        // Create an instance of the frame builder and get a response builder.
        aurum::protocol::frame_builder _frame_builder;
        // Output response target array properly matching requested context boundary objects.
        auto _response_builder = _frame_builder.as_response();

        // Reserve space covering required allocations to minimize vector memory reallocations correctly.
        _response_builder.reserve(_number_of_requests);

        // Iterate sequentially tracking each inner payload component processing its handler function
        for (const auto _request_length : _requests_lengths) {
            // Verify bounding limits securing memory accessing operations checking frame footprint correctly
            if (frame->size() - 2 - _offset < _request_length) {
                // Terminate operation preventing payload bounds invalid memory access securely
                return nullptr;
            }

            // Verify bounding sizes enforce minimum memory capacity bounds structurally for identifier parsing (opcode + type + uuid = 18 bytes)
            if (_request_length < sizeof(std::uint8_t) * 2 + 16) {
                // Return safely breaking mapping loop ensuring correct payload access sizes structure
                return nullptr;
            }

            // Extract opcode parameter extracting exact operational target mapping request command type
            std::uint8_t _opcode = frame->data()[_offset];

            // Extract message type mapping command direction
            auto _type = static_cast<message_type>(frame->data()[_offset + sizeof(std::uint8_t)]);

            // Setup identifier target instance representing transaction mapping unique operation
            transaction_id _transaction_id;

            // Copy identifier safely memory bounds evaluating parameters extracting exact transaction reference
            std::memcpy(_transaction_id.data, frame->data() + _offset + sizeof(std::uint8_t) * 2, 16);

            // Move offset pointer dynamically mapping current bounds safely towards inner parameters target
            std::size_t _payload_offset = _offset + sizeof(std::uint8_t) * 2 + 16;

            // Compute payload bounds safely tracking exact inner boundary evaluating length array accurately
            std::size_t _payload_length = _request_length - (sizeof(std::uint8_t) * 2 + 16);

            // Construct bounded payload wrapper object handling data parsing correctly safely accessing parameters
            payload_buffer _payload(frame->data() + _payload_offset, _payload_length);

            // Locate mapped handler array executing correct logic bounding operation opcode accurately
            const auto& _handler = state_->get_handlers()[_opcode];

            // Execute handler function passing parameters enabling correctly processing dynamic request targets accurately.
            _handler(_type, _response_builder, _transaction_id, _payload, session, state_);

            // Shift limits pointer moving towards next frame object correctly indexing bounds dynamically
            _offset += _request_length;
        }

        // Extract fully mapped serialized buffers representing operation correctly returning payloads structurally.
        auto _response_buffers = _response_builder.get_buffers();

        // Return dynamically bounded response pointer effectively handling array sequence payload return correctly.
        return std::make_shared<std::vector<std::uint8_t>>(std::move(_response_buffers));
    }
}
