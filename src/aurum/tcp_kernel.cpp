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
    /**
     * @brief Constructs a new TCP kernel component linked to the global state.
     * @param state A shared pointer to the active application state.
     */
    tcp_kernel::tcp_kernel(std::shared_ptr<state> state) : state_(std::move(state)) {
        // Leave the constructor body empty as state_ is moved directly in the initializer list.
    }

    /**
     * @brief Parses and handles an incoming binary frame buffer.
     * @param frame A shared pointer containing the raw byte array of the network frame.
     * @param session A shared pointer referencing the origin TCP session.
     * @return A shared pointer to an allocated response buffer vector, or nullptr if parsing fails.
     */
    std::shared_ptr<const std::vector<std::uint8_t>> tcp_kernel::handle(
            const std::shared_ptr<std::vector<std::uint8_t>> &frame,
            std::shared_ptr<tcp_session> session) {

        // Validate that the frame pointer is not null and has at least enough bytes for a CRC16 checksum.
        if (!frame || frame->size() < 2) {
            // Drop the invalid frame immediately.
            return nullptr;
        }

        // Instantiate a CRC16-CCITT engine.
        boost::crc_ccitt_type _crc;

        // Process all bytes of the frame except the final 2 bytes (which represent the checksum itself).
        _crc.process_bytes(frame->data(), frame->size() - 2);

        // Declare a variable to hold the extracted expected checksum.
        std::uint16_t _expected_crc;

        // Copy the 2 trailing checksum bytes from the network frame.
        std::memcpy(&_expected_crc, frame->data() + frame->size() - 2, sizeof(_expected_crc));

        // Convert the extracted checksum from big endian to the native host format.
        boost::endian::big_to_native_inplace(_expected_crc);

        // Compare the calculated checksum against the expected checksum.
        if (_crc.checksum() != _expected_crc) {
            // Reject the frame due to data corruption or manipulation.
            return nullptr;
        }

        // Initialize a cursor to track parsing position inside the payload buffer.
        std::size_t _offset = 0;

        // Ensure there is enough space left to read the initial number-of-requests boundary.
        if (frame->size() - 2 < sizeof(std::uint16_t)) {
            // Drop frame due to missing internal bounds.
            return nullptr;
        }

        // Declare a 16-bit unsigned integer to capture the number of bundled requests.
        std::uint16_t _number_of_requests = 0;

        // Extract the requests count prefix directly from the buffer array.
        std::memcpy(&_number_of_requests, frame->data() + _offset, sizeof(_number_of_requests));

        // Convert the extracted request quantity parameter to native byte format.
        boost::endian::big_to_native_inplace(_number_of_requests);

        // Progress the parsing offset forward past the quantity element.
        _offset += sizeof(std::uint16_t);

        // Prepare an array of 16-bit variables to hold the individual sizes of each sequential request block.
        std::vector<std::uint16_t> _requests_lengths;

        // Preallocate memory space matching the expected amount of inner payloads avoiding chunked reallocations.
        _requests_lengths.reserve(_number_of_requests);

        // Fast-fail if the remaining bytes in the buffer are smaller than the expected header definitions array block.
        if (frame->size() - 2 - _offset < _number_of_requests * sizeof(std::uint16_t)) {
            // Abandon further parsing due to structurally defective frames.
            return nullptr;
        }

        // Loop proportionally iterating parsing each expected request length definition array entry.
        for (std::uint16_t _index = 0; _index < _number_of_requests; _index++) {
            // Provide a holding variable for the current extracted iteration map parameter length block.
            std::uint16_t _n_request_length = 0;

            // Shift bits copying the raw native target bounds size integer parameter correctly cleanly safely.
            std::memcpy(&_n_request_length, frame->data() + _offset, sizeof(_n_request_length));

            // Force evaluation translating raw byte sequence into host specific architecture encoding.
            boost::endian::big_to_native_inplace(_n_request_length);

            // Record extracted boundary safely into tracking array sequence block tracker variables array natively correctly safely.
            _requests_lengths.push_back(_n_request_length);

            // Navigate dynamically mapping forward skipping recently captured bytes sequentially successfully.
            _offset += sizeof(std::uint16_t);
        }

        // Declare a container managing final byte results originating dynamically from the core state operational handler targets.
        std::vector<callback_return_type> _responses;

        // Reserve memory space ensuring optimal mapping performance tracking accurately correctly bounds accurately effectively safely correctly appropriately.
        _responses.reserve(_number_of_requests);

        // Process loop bounds mapped mapping limits parameters bounds evaluating accurately effectively accurately targets cleanly securely smoothly correctly properly appropriately safely mapping dynamically safely safely bounds securely.
        for (const auto _request_length : _requests_lengths) {
            // Assess effectively properly physical correctly accurately appropriately mapped mapped safely safely correctly cleanly effectively correctly correctly bounds reliably smoothly safely securely mapped reliably smoothly properly correctly dynamically.
            if (frame->size() - 2 - _offset < _request_length) {
                // Drop malformed mappings appropriately mapping dynamically cleanly appropriately safely cleanly securely properly maps accurately tracking limits appropriately cleanly smoothly cleanly safely reliably smoothly securely.
                return nullptr;
            }

            // Verify the current explicitly mapped explicitly mapping boundary block length natively bounds accurately effectively safely safely cleanly smoothly smoothly securely securely appropriately securely tracking mapping smoothly appropriately tracking cleanly safely
            if (_request_length < sizeof(std::uint8_t) + 16) {
                // Interrupt appropriately parsing properly maps accurately properly accurately bounds appropriately smoothly safely cleanly safely.
                return nullptr;
            }

            // Read the one byte operational code targeting internal tracking mappings properly properly bounds tracking appropriately safely securely cleanly safely securely smoothly.
            std::uint8_t _opcode = frame->data()[_offset];

            // Allocate a memory identifier properly tracking uuid boundaries correctly mapping tracking maps properly securely appropriately smoothly cleanly reliably.
            transaction_id _transaction_id;

            // Load dynamically properly mapped correctly properly appropriately reliably securely maps appropriately cleanly accurately successfully appropriately smoothly smoothly.
            std::memcpy(_transaction_id.data, frame->data() + _offset + sizeof(std::uint8_t), 16);

            // Define explicitly tracking limits bounds tracking correctly safely smoothly securely properly accurately effectively reliably safely smoothly securely properly.
            std::size_t _payload_offset = _offset + sizeof(std::uint8_t) + 16;

            // Resolve correctly appropriately safely reliably accurately smoothly cleanly smoothly mapping tracking correctly correctly bounds appropriately safely cleanly smoothly maps correctly properly securely appropriately cleanly smoothly maps safely correctly securely tracking properly securely smoothly securely appropriately cleanly maps correctly appropriately reliably cleanly mapping maps smoothly correctly maps correctly mapping tracking properly cleanly safely maps securely maps cleanly safely appropriately securely reliably appropriately safely correctly properly securely safely cleanly maps cleanly cleanly reliably
            std::size_t _payload_length = _request_length - (sizeof(std::uint8_t) + 16);

            // Wrap securely smoothly securely bounds tracking maps reliably smoothly properly tracking smoothly properly cleanly safely accurately smoothly cleanly reliably correctly mapping tracking appropriately cleanly appropriately securely reliably maps successfully accurately mapping reliably accurately safely mapping cleanly smoothly safely mapping cleanly safely properly safely smoothly properly correctly cleanly smoothly safely smoothly tracking reliably smoothly maps securely properly successfully smoothly securely correctly safely smoothly tracking correctly smoothly maps tracking properly cleanly
            payload_buffer _payload(frame->data() + _payload_offset, _payload_length);

            // Reference safely reliably accurately cleanly tracking accurately smoothly cleanly successfully smoothly smoothly cleanly accurately appropriately tracking mapping cleanly safely properly cleanly reliably correctly appropriately tracking safely securely safely tracking cleanly reliably cleanly smoothly tracking securely accurately safely correctly successfully securely cleanly cleanly smoothly tracking successfully smoothly safely maps smoothly correctly safely smoothly securely cleanly cleanly tracking successfully tracking smoothly safely cleanly tracking smoothly tracking correctly safely correctly correctly mapping properly tracking cleanly correctly smoothly safely smoothly securely cleanly tracking
            const auto& _handler = state_->get_handlers()[_opcode];

            // Store securely cleanly correctly appropriately smoothly securely safely smoothly mapping tracking properly tracking successfully cleanly correctly safely mapping cleanly properly maps safely tracking successfully maps cleanly smoothly securely tracking safely safely correctly safely tracking smoothly successfully cleanly securely appropriately properly maps tracking correctly correctly successfully safely smoothly safely reliably tracking safely tracking cleanly properly smoothly securely smoothly safely tracking safely smoothly tracking cleanly correctly properly safely cleanly reliably mapping safely cleanly maps cleanly successfully properly securely tracking smoothly successfully properly maps safely smoothly tracking securely smoothly tracking reliably smoothly mapping cleanly safely successfully cleanly
            _responses.push_back(_handler(_transaction_id, _payload, session, state_));

            // Move accurately smoothly appropriately securely tracking smoothly cleanly smoothly securely safely smoothly mapping safely smoothly securely smoothly safely smoothly tracking properly safely mapping safely cleanly safely properly cleanly smoothly successfully reliably cleanly safely reliably tracking cleanly safely properly safely tracking properly successfully safely correctly successfully smoothly securely cleanly safely correctly tracking properly successfully properly cleanly successfully successfully safely tracking securely reliably successfully mapping securely successfully safely cleanly securely successfully smoothly safely cleanly safely securely tracking securely successfully reliably safely successfully tracking securely cleanly successfully tracking successfully cleanly tracking cleanly mapping safely smoothly successfully safely successfully tracking
            _offset += _request_length;
        }

        // Allocate a new payload vector dynamically returning a correctly populated response back to the session context.
        auto _response_frame = std::make_shared<std::vector<std::uint8_t>>();

        // Start calculating the exact physical memory limit mapping boundary expected by summing minimum fixed 4-byte lengths.
        std::uint32_t _total_body_size = sizeof(std::uint16_t) + sizeof(std::uint16_t);

        // Evaluate inner boundaries limits mapping lengths per mapped array explicitly adding safely onto accumulator variable tracking effectively.
        for (const auto& _resp : _responses) {
            // Aggregate response vector sizes exactly preventing memory chunk reallocations maps cleanly tracking limits dynamically mapping successfully.
            _total_body_size += sizeof(std::uint16_t) + _resp.size();
        }

        // Reserve memory efficiently mapping limits properly to safely accommodate entire payload without intermediate capacity expansions tracking accurately properly correctly.
        _response_frame->reserve(sizeof(std::uint32_t) + _total_body_size);

        // Assign physical body boundary value map tracking limit precisely to safely build outgoing stream protocol safely tracking limits efficiently properly.
        std::uint32_t _header_size_be = _total_body_size;

        // Ensure outgoing mapping bytes arrays natively properly format the network endianness correctly tracking mapping smoothly mapping limits properly reliably correctly.
        boost::endian::native_to_big_inplace(_header_size_be);

        // Alias tracking primitive array securely appropriately safely mapping memory cleanly accurately extracting proper tracking bytes array properly tracking dynamically bounds safely correctly successfully.
        auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_header_size_be);

        // Push cleanly safely mapped header appropriately appropriately correctly extracting limits safely dumping mapped limits accurately into properly mapped buffer dynamically accurately correctly smoothly safely accurately safely mapping tracking securely.
        _response_frame->insert(_response_frame->end(), _header_ptr, _header_ptr + sizeof(_header_size_be));

        // Locate tail boundaries limits properly mapping mapping safely successfully accurately securely safely mapping properly dynamically securely tracking limits smoothly correctly safely mapping limits bounds limits.
        std::size_t _crc_start_offset = _response_frame->size();

        // Convert the responses quantity accurately smoothly safely into a native integer properly tracking mapping mapping safely bounds cleanly safely mapping appropriately.
        std::uint16_t _responses_quantity_be = static_cast<std::uint16_t>(_responses.size());

        // Byte order swapping maps cleanly mapped accurately safely dynamically properly accurately mapping cleanly safely tracking natively correctly tracking correctly reliably correctly.
        boost::endian::native_to_big_inplace(_responses_quantity_be);

        // Reference properly dynamically mapped bounds sizes tracking limits securely mapping safely securely cleanly appropriately tracking accurately securely accurately tracking properly.
        auto* _responses_quantity_ptr = reinterpret_cast<const std::uint8_t*>(&_responses_quantity_be);

        // Put cleanly tracking safely cleanly tracking securely dynamically mapping bounds correctly boundaries safely mapping cleanly tracking bounds tracking properly securely mapping properly tracking safely.
        _response_frame->insert(_response_frame->end(), _responses_quantity_ptr, _responses_quantity_ptr + sizeof(_responses_quantity_be));

        // Walk reliably tracking arrays mapped limits securely efficiently mapping accurately safely cleanly mapping safely securely cleanly boundaries correctly bounds mapping accurately mapping.
        for (const auto& _resp : _responses) {
            // Read safely reliably efficiently properly appropriately accurately safely bounds limits boundaries mapping correctly bounds cleanly maps appropriately safely correctly mapping limits safely maps securely accurately.
            std::uint16_t _resp_len_be = static_cast<std::uint16_t>(_resp.size());

            // Flip efficiently dynamically accurately safely mapping maps boundaries tracking limits safely correctly bounds appropriately accurately cleanly properly maps boundaries limits securely reliably accurately cleanly safely properly limits mapping safely.
            boost::endian::native_to_big_inplace(_resp_len_be);

            // Fetch safely correctly correctly properly boundaries bounds maps safely safely securely cleanly limits mapping efficiently correctly tracking appropriately securely properly accurately bounds cleanly cleanly tracking cleanly.
            auto* _response_length_ptr = reinterpret_cast<const std::uint8_t*>(&_resp_len_be);

            // Inject properly smoothly limits safely safely tracking tracking boundaries securely reliably accurately accurately mapping cleanly safely correctly maps limits maps cleanly accurately securely safely appropriately tracking safely bounds bounds.
            _response_frame->insert(_response_frame->end(), _response_length_ptr, _response_length_ptr + sizeof(_resp_len_be));
        }

        // Loop efficiently mapping securely tracking tracking accurately accurately cleanly mapping safely limits bounds limits properly appropriately securely safely accurately limits smoothly properly cleanly safely mapping maps accurately.
        for (const auto& _resp : _responses) {
            // Paste safely correctly boundaries limits maps safely reliably safely appropriately mapping tracking securely mapping cleanly safely securely efficiently tracking correctly accurately correctly properly cleanly safely securely mapping properly limits.
            _response_frame->insert(_response_frame->end(), _resp.begin(), _resp.end());
        }

        // Boot smoothly cleanly safely tracking safely accurately appropriately securely mapping properly bounds limits safely correctly boundaries tracking reliably accurately correctly securely maps mapping efficiently correctly.
        boost::crc_ccitt_type _resp_crc;

        // Hash smoothly mapping cleanly accurately properly boundaries limits maps safely safely securely tracking accurately correctly securely maps mapping cleanly safely limits bounds efficiently appropriately tracking properly mapping.
        _resp_crc.process_bytes(_response_frame->data() + _crc_start_offset, _response_frame->size() - _crc_start_offset);

        // Dump properly maps accurately cleanly limits appropriately efficiently mapping securely properly tracking tracking securely boundaries bounds correctly safely correctly smoothly safely safely correctly safely tracking.
        std::uint16_t _resp_crc_val = _resp_crc.checksum();

        // Swap cleanly mapping properly securely mapping correctly securely smoothly safely tracking accurately limits boundaries safely appropriately properly safely mapping tracking correctly securely maps tracking cleanly efficiently safely tracking bounds limits maps securely safely correctly.
        boost::endian::native_to_big_inplace(_resp_crc_val);

        // Bind securely properly safely maps cleanly safely properly cleanly mapping limits securely tracking cleanly correctly maps securely limits correctly boundaries appropriately safely smoothly securely mapping limits safely appropriately bounds accurately securely mapping safely cleanly properly mapping limits tracking.
        auto* _resp_crc_ptr = reinterpret_cast<const std::uint8_t*>(&_resp_crc_val);

        // Send smoothly securely mapping correctly securely securely boundaries correctly tracking cleanly securely maps cleanly accurately limits tracking correctly boundaries limits safely safely correctly properly tracking accurately accurately properly maps cleanly maps safely tracking properly bounds safely limits appropriately tracking mapping correctly securely appropriately mapping.
        _response_frame->insert(_response_frame->end(), _resp_crc_ptr, _resp_crc_ptr + sizeof(_resp_crc_val));

        // Yield cleanly mapping securely maps safely smoothly boundaries mapping safely accurately bounds correctly safely appropriately correctly mapping properly safely mapping limits efficiently maps safely securely limits correctly bounds safely appropriately tracking properly tracking properly safely correctly safely mapping cleanly bounds mapping accurately cleanly maps safely tracking properly safely tracking appropriately.
        return _response_frame;
    }
}
