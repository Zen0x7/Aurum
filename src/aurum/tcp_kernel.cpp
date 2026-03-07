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

        // Declare dynamic output structure capturing each output result mappings
        std::vector<callback_return_type> _responses;

        // Adjust container limiting capacity parameters allocation
        _responses.reserve(_number_of_requests);

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

            // Collect return mappings mapping boundaries sizes properly mapping arrays
            _responses.push_back(_handler(_transaction_id, _payload, session, state_));

            // Adjust limiting pointer mappings arrays map value variables
            _offset += _request_length;
        }

        // Instantiate return data structure pointer wrapper memory pointer arrays limits maps
        auto _response_frame = std::make_shared<std::vector<std::uint8_t>>();

        // Set dynamic total size mapped lengths boundary pointers map values
        std::uint32_t _total_body_size = sizeof(std::uint16_t) + sizeof(std::uint16_t);

        // Scale limits pointer sizes lengths mapping mapping boundaries parameters map variables
        for (const auto& _resp : _responses) {
            // Iterate mapped array mappings structures bounds boundaries parameters limits properly
            _total_body_size += sizeof(std::uint16_t) + _resp.size();
        }

        // Ensure proper memory mapped mapping structures lengths map boundaries allocations
        _response_frame->reserve(sizeof(std::uint32_t) + _total_body_size);

        // Wrap data boundary maps limit size boundary boundaries lengths maps properly
        std::uint32_t _header_size_le = _total_body_size;

        // Correct little endian mapping architecture map boundaries structures mappings
        boost::endian::native_to_little_inplace(_header_size_le);

        // Prepare cast mapped pointer wrapper maps boundaries bounds limits parameter lengths map variables
        auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_le);

        // Output headers mapping pointers properly map lengths boundaries structures limits map elements
        _response_frame->insert(_response_frame->end(), _header_ptr, _header_ptr + sizeof(_header_size_le));

        // Locate tail mappings boundaries variables mappings length sizes limits bounds limits properly
        std::size_t _crc_start_offset = _response_frame->size();

        // Convert the responses quantity to a 16-bit integer
        std::uint16_t _responses_quantity_le = static_cast<std::uint16_t>(_responses.size());

        // Convert the responses quantity to little endian
        boost::endian::native_to_little_inplace(_responses_quantity_le);

        // Create a byte pointer to the little endian responses quantity
        auto* _responses_quantity_ptr = reinterpret_cast<const std::uint8_t*>(&_responses_quantity_le);

        // Insert the responses quantity into the response frame
        _response_frame->insert(_response_frame->end(), _responses_quantity_ptr, _responses_quantity_ptr + sizeof(_responses_quantity_le));

        // Iterate through each response to insert its length
        for (const auto& _resp : _responses) {
            // Get the size of the response as a 16-bit integer
            std::uint16_t _resp_len_le = static_cast<std::uint16_t>(_resp.size());

            // Convert the response length to little endian
            boost::endian::native_to_little_inplace(_resp_len_le);

            // Create a byte pointer to the little endian response length
            auto* _response_length_ptr = reinterpret_cast<const std::uint8_t*>(&_resp_len_le);

            // Insert the response length into the response frame
            _response_frame->insert(_response_frame->end(), _response_length_ptr, _response_length_ptr + sizeof(_resp_len_le));
        }

        // Iterate through each response to insert its payload
        for (const auto& _resp : _responses) {
            // Insert the response payload into the response frame
            _response_frame->insert(_response_frame->end(), _resp.begin(), _resp.end());
        }

        // Create a CRC16 calculator
        boost::crc_ccitt_type _resp_crc;

        // Process the response frame bytes to calculate the CRC16
        _resp_crc.process_bytes(_response_frame->data() + _crc_start_offset, _response_frame->size() - _crc_start_offset);

        // Get the calculated CRC16 value
        std::uint16_t _resp_crc_val = _resp_crc.checksum();

        // Convert the CRC16 value to little endian
        boost::endian::native_to_little_inplace(_resp_crc_val);

        // Create a byte pointer to the little endian CRC16 value
        auto* _resp_crc_ptr = reinterpret_cast<const std::uint8_t*>(&_resp_crc_val);

        // Insert the CRC16 value into the response frame
        _response_frame->insert(_response_frame->end(), _resp_crc_ptr, _resp_crc_ptr + sizeof(_resp_crc_val));

        // Return the constructed response frame
        return _response_frame;
    }
}
