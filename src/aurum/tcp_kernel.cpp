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

        if (!frame || frame->size() < 2) {
            return nullptr;
        }

        // The last 2 bytes contain a CRC16 calculated from frame.begin() to frame.end() - 2
        boost::crc_ccitt_type _crc;
        _crc.process_bytes(frame->data(), frame->size() - 2);

        std::uint16_t _expected_crc;
        std::memcpy(&_expected_crc, frame->data() + frame->size() - 2, sizeof(_expected_crc));
        boost::endian::big_to_native_inplace(_expected_crc);

        if (_crc.checksum() != _expected_crc) {
            return nullptr; // Ignore frame on invalid CRC
        }

        std::size_t _offset = 0;

        if (frame->size() - 2 < sizeof(std::uint16_t)) {
            return nullptr;
        }

        // The first 2 bytes correspond to the quantity of requests in big endian.
        std::uint16_t _number_of_requests = 0;
        std::memcpy(&_number_of_requests, frame->data() + _offset, sizeof(_number_of_requests));
        boost::endian::big_to_native_inplace(_number_of_requests);
        _offset += sizeof(std::uint16_t);

        std::vector<std::uint16_t> _requests_lengths;
        _requests_lengths.reserve(_number_of_requests);

        if (frame->size() - 2 - _offset < _number_of_requests * sizeof(std::uint16_t)) {
            return nullptr;
        }

        // Extract request lengths
        for (std::uint16_t i = 0; i < _number_of_requests; i++) {
            std::uint16_t _n_request_length = 0;
            std::memcpy(&_n_request_length, frame->data() + _offset, sizeof(_n_request_length));
            boost::endian::big_to_native_inplace(_n_request_length);
            _requests_lengths.push_back(_n_request_length);
            _offset += sizeof(std::uint16_t);
        }

        std::vector<callback_return_type> _responses;
        _responses.reserve(_number_of_requests);

        // Process each request
        for (const auto _request_length : _requests_lengths) {
            if (frame->size() - 2 - _offset < _request_length) {
                return nullptr; // Malformed frame
            }

            if (_request_length < sizeof(std::uint8_t) + 16) {
                return nullptr; // Minimum length is opcode + transaction_id
            }

            // Opcode is the first byte
            std::uint8_t _opcode = frame->data()[_offset];

            // Transaction ID is the next 16 bytes
            transaction_id _transaction_id;
            std::memcpy(_transaction_id.data, frame->data() + _offset + sizeof(std::uint8_t), 16);

            // Payload is the rest of the request
            std::size_t _payload_offset = _offset + sizeof(std::uint8_t) + 16;
            std::size_t _payload_length = _request_length - (sizeof(std::uint8_t) + 16);

            payload_buffer _payload(frame->data() + _payload_offset, _payload_length);

            // Invoke handler
            const auto& _handler = state_->get_handlers()[_opcode];
            _responses.push_back(_handler(_transaction_id, _payload, session, state_));

            _offset += _request_length;
        }

        // Build the response frame
        auto _response_frame = std::make_shared<std::vector<std::uint8_t>>();

        // Compute total body size: responses_quantity(2) + N * (response_length(2) + payload) + CRC16(2)
        std::uint32_t _total_body_size = sizeof(std::uint16_t) + sizeof(std::uint16_t); // responses_quantity + CRC16
        for (const auto& _resp : _responses) {
            _total_body_size += sizeof(std::uint16_t) + _resp.size();
        }

        // Reserve space: header(4) + body
        _response_frame->reserve(sizeof(std::uint32_t) + _total_body_size);

        // Header (4 bytes, total size of response body in big endian)
        std::uint32_t _header_size_be = _total_body_size;
        boost::endian::native_to_big_inplace(_header_size_be);
        auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_be);
        _response_frame->insert(_response_frame->end(), _header_ptr, _header_ptr + sizeof(_header_size_be));

        // Note: CRC16 covers from responses_quantity to the end of the last payload
        std::size_t _crc_start_offset = _response_frame->size();

        // Responses quantity (2 bytes in big endian)
        std::uint16_t _responses_quantity_be = static_cast<std::uint16_t>(_responses.size());
        boost::endian::native_to_big_inplace(_responses_quantity_be);
        auto* _qty_ptr = reinterpret_cast<const std::uint8_t*>(&_responses_quantity_be);
        _response_frame->insert(_response_frame->end(), _qty_ptr, _qty_ptr + sizeof(_responses_quantity_be));

        // Each response length
        for (const auto& _resp : _responses) {
            std::uint16_t _resp_len_be = static_cast<std::uint16_t>(_resp.size());
            boost::endian::native_to_big_inplace(_resp_len_be);
            auto* _len_ptr = reinterpret_cast<const std::uint8_t*>(&_resp_len_be);
            _response_frame->insert(_response_frame->end(), _len_ptr, _len_ptr + sizeof(_resp_len_be));
        }

        // Each response payload
        for (const auto& _resp : _responses) {
            _response_frame->insert(_response_frame->end(), _resp.begin(), _resp.end());
        }

        // Calculate CRC16 for the response body
        boost::crc_ccitt_type _resp_crc;
        _resp_crc.process_bytes(_response_frame->data() + _crc_start_offset, _response_frame->size() - _crc_start_offset);

        std::uint16_t _resp_crc_val = _resp_crc.checksum();
        boost::endian::native_to_big_inplace(_resp_crc_val);
        auto* _crc_ptr = reinterpret_cast<const std::uint8_t*>(&_resp_crc_val);
        _response_frame->insert(_response_frame->end(), _crc_ptr, _crc_ptr + sizeof(_resp_crc_val));

        return _response_frame;
    }
}
