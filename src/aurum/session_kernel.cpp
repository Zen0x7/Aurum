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

        if (!frame || frame->size() < 2) {
            return nullptr;
        }

        boost::crc_ccitt_type _crc;

        _crc.process_bytes(frame->data(), frame->size() - 2);

        std::uint16_t _expected_crc;

        std::memcpy(&_expected_crc, frame->data() + frame->size() - 2, sizeof(_expected_crc));

        boost::endian::little_to_native_inplace(_expected_crc);

        if (_crc.checksum() != _expected_crc) {
            return nullptr;
        }

        std::size_t _offset = 0;

        if (frame->size() - 2 < sizeof(std::uint16_t)) {
            return nullptr;
        }

        std::uint16_t _number_of_requests = 0;

        std::memcpy(&_number_of_requests, frame->data() + _offset, sizeof(_number_of_requests));

        boost::endian::little_to_native_inplace(_number_of_requests);

        _offset += sizeof(std::uint16_t);

        std::vector<std::uint16_t> _requests_lengths;

        _requests_lengths.reserve(_number_of_requests);

        if (frame->size() - 2 - _offset < _number_of_requests * sizeof(std::uint16_t)) {
            return nullptr;
        }

        for (std::uint16_t _index = 0; _index < _number_of_requests; _index++) {
            std::uint16_t _n_request_length = 0;

            std::memcpy(&_n_request_length, frame->data() + _offset, sizeof(_n_request_length));

            boost::endian::little_to_native_inplace(_n_request_length);

            _requests_lengths.push_back(_n_request_length);

            _offset += sizeof(std::uint16_t);
        }

        aurum::protocol::frame_builder _frame_builder;
        auto _response_builder = _frame_builder.as_response();

        _response_builder.reserve(_number_of_requests);

        for (const auto _request_length : _requests_lengths) {
            if (frame->size() - 2 - _offset < _request_length) {
                return nullptr;
            }

            if (_request_length < sizeof(std::uint8_t) * 2 + 16) {
                return nullptr;
            }

            std::uint8_t _opcode = frame->data()[_offset];

            auto _type = static_cast<message_type>(frame->data()[_offset + sizeof(std::uint8_t)]);

            transaction_id _transaction_id;

            std::memcpy(_transaction_id.data, frame->data() + _offset + sizeof(std::uint8_t) * 2, 16);

            std::size_t _payload_offset = _offset + sizeof(std::uint8_t) * 2 + 16;

            std::size_t _payload_length = _request_length - (sizeof(std::uint8_t) * 2 + 16);

            payload_buffer _payload(frame->data() + _payload_offset, _payload_length);

            const auto& _handler = state_->get_handlers()[_opcode];

            _handler(_type, _response_builder, _transaction_id, _payload, session, state_);

            _offset += _request_length;
        }

        bool _with_header = (session->get_type() == protocol::tcp);

        auto _response_buffers = _response_builder.get_buffers(_with_header);

        return std::make_shared<std::vector<std::uint8_t>>(std::move(_response_buffers));
    }
}
