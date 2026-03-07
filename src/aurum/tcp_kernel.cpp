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

#include <aurum/tcp_kernel.hpp>

#include <aurum/state.hpp>
#include <aurum/tcp_session.hpp>
#include <iostream>
#include <boost/crc.hpp>
#include <boost/endian/conversion.hpp>
#include <cstring>

namespace aurum {
    tcp_kernel::tcp_kernel(std::shared_ptr<state> state) : state_(std::move(state)) {

    }

    std::shared_ptr<const std::vector<std::uint8_t>> tcp_kernel::handle(
            const std::shared_ptr<std::vector<std::uint8_t>> &frame,
            std::shared_ptr<tcp_session> session) {

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

        // Initialize parameter gathering mapping how many requests limits inside
        std::uint16_t _number_of_requests = 0;

        // Map payload target into specific mapping structure
        std::memcpy(&_number_of_requests, frame->data() + _offset, sizeof(_number_of_requests));

        // Reverse swap little endian architecture network array encoding to natively
        boost::endian::little_to_native_inplace(_number_of_requests);

        // Advance payload size token forward dynamically
        _offset += sizeof(std::uint16_t);

        // Prepare local vector arrays storing tracking length per item processed
        std::vector<std::uint16_t> _requests_lengths;

        // Expand structure size optimizing allocation avoiding chunk reallocation latency
        _requests_lengths.reserve(_number_of_requests);

        // Pre-validate minimal requirements enforcing bounds limiting
        if (frame->size() - 2 - _offset < _number_of_requests * sizeof(std::uint16_t)) {
            // Kill dropped payload processing bounds limitation exceptions
            return nullptr;
        }

        // Traverse dynamically loop allocating requested size elements mapping
        for (std::uint16_t _index = 0; _index < _number_of_requests; _index++) {
            // Define length memory storage map parameter
            std::uint16_t _n_request_length = 0;

            // Replicate structure mapping raw bounds
            std::memcpy(&_n_request_length, frame->data() + _offset, sizeof(_n_request_length));

            // Format incoming mapping mapping standard
            boost::endian::little_to_native_inplace(_n_request_length);

            // Target output memory boundary mappings array sequentially
            _requests_lengths.push_back(_n_request_length);

            // Scroll dynamic limits offset parameters array
            _offset += sizeof(std::uint16_t);
        }

        // Create an instance of the frame builder and get a response builder.
        aurum::protocol::frame_builder _frame_builder;
        // Output pointer arrays lengths loops properly mapping parameters loops.
        auto _response_builder = _frame_builder.as_response();

        // Reserve space for all incoming requests mapping constraints parameters mapping bounds variables sizes limits lengths mapping.
        _response_builder.reserve(_number_of_requests);

        // Start evaluating inner components lengths mapping bounds values limits properly
        for (const auto _request_length : _requests_lengths) {
            // Evaluate limiting checks enforcing strict sizes map parameter values
            if (frame->size() - 2 - _offset < _request_length) {
                // Drop malformed mappings
                return nullptr;
            }

            // Recheck limits boundary sizes mapping sizes parameters mappings
            if (_request_length < sizeof(std::uint8_t) + 16) {
                // Drop missing boundary map bounds
                return nullptr;
            }

            // Assign byte limits primitive value operation limits maps limits mappings
            std::uint8_t _opcode = frame->data()[_offset];

            // Target limits bounds mappings transactions identifier values
            transaction_id _transaction_id;

            // Assign mapping map copy byte data targets arrays properly values
            std::memcpy(_transaction_id.data, frame->data() + _offset + sizeof(std::uint8_t), 16);

            // Scroll bounds offset pointers limiting maps
            std::size_t _payload_offset = _offset + sizeof(std::uint8_t) + 16;

            // Process payload target mapping boundaries mapping mappings pointers lengths boundaries sizes
            std::size_t _payload_length = _request_length - (sizeof(std::uint8_t) + 16);

            // Convert byte limits pointers payloads mapping into structured reference parameter mapping spans
            payload_buffer _payload(frame->data() + _payload_offset, _payload_length);

            // Execute actual handler reference mappings operation dynamically
            const auto& _handler = state_->get_handlers()[_opcode];

            // Execute mapped limits handler boundaries mapping limitations passing bounds arrays loops.
            _handler(_response_builder, _transaction_id, _payload, session, state_);

            // Adjust limiting pointer mappings arrays map value variables
            _offset += _request_length;
        }

        // Output limits variables bounds parameters lengths sizes arrays mappings boundaries limits properties constraints.
        auto _response_buffers = _response_builder.get_buffers();

        // Return pointers mapping limits maps boundaries mapped variables mapping constraints arrays bounds limitations properly bounds maps.
        return std::make_shared<std::vector<std::uint8_t>>(std::move(_response_buffers));
    }
}
