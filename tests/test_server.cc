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

#include "tcp_server_fixture.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/crc.hpp>
#include <aurum/protocol.hpp>

TEST_F(tcp_server_fixture, ConnectSendPayloadAndDisconnect) {
    boost::asio::io_context client_io;
    boost::asio::ip::tcp::socket socket(client_io);

    unsigned short port =
            state->get_configuration().tcp_port_.load(std::memory_order_acquire);

    socket.connect({
        boost::asio::ip::address_v4::loopback(),
        port
    });

    wait_until([this] {
            return state->get_sessions().size() == 1;
        });

    ASSERT_EQ(state->get_sessions().size(), 1);

    // Track identifier mappings lengths limitations properly loops mapping sizes sizes pointers boundaries properly limits limits.
    boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    // Adjust mapped frame sizes loops variables boundaries limitations arrays maps constraints sizes.
    aurum::protocol::frame_builder _frame_builder;
    // Push limits mapped loops limits variables sizes lengths maps parameters mapped parameters lengths sizes mappings properly boundaries mappings.
    auto _request_builder = _frame_builder.as_request();
    // Execute variables arrays mappings parameters loops mapping limits mapping mapping.
    _request_builder.add_ping(_transaction_id);

    // Adjust sizes mappings limits maps bounds properly mapping boundaries limitations.
    auto _payload = _request_builder.get_buffers();

    // Dump actual serialized payload through TCP sequence pipeline
    boost::asio::write(socket, boost::asio::buffer(_payload));

    // Declare placeholder buffer integer for received server bound wrapper length
    uint32_t _response_header_length = 0;

    // Trigger socket bound sync read routine parsing frame limits explicitly
    boost::asio::read(socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)));

    // Reformat input stream bounds representation to little endian parsing
    boost::endian::little_to_native_inplace(_response_header_length);

    // Prepare receiving target dynamic array based off received length boundary
    std::vector<uint8_t> _response_body(_response_header_length);

    // Flush rest of the network stream frame sequence down towards local buffer space
    boost::asio::read(socket, boost::asio::buffer(_response_body));

    // Assure frame constraints limits hold valid minimum bounds limits rules
    ASSERT_GE(_response_body.size(), 4); // at least qty + crc

    // Declare quantity received placeholder tracker
    std::uint16_t _response_quantity;

    // Deep copy header structure bytes back mapping primitive properties
    std::memcpy(&_response_quantity, _response_body.data(), sizeof(_response_quantity));

    // Adapt back input array sequence mapping towards correct host primitive
    boost::endian::little_to_native_inplace(_response_quantity);

    // Compare structural output enforcing ping single return size bounds
    ASSERT_EQ(_response_quantity, 1);

    // Declare length parameter to capture response specific element length limit
    std::uint16_t _response_length;

    // Transfer mapping copying memory segment representation directly
    std::memcpy(&_response_length, _response_body.data() + 2, sizeof(_response_length));

    // Correct memory formatting for native little endian processing unit architectures
    boost::endian::little_to_native_inplace(_response_length);

    // Ping length bounds expected explicitly sized to ID (16) and Exit (1) = 17
    ASSERT_EQ(_response_length, 17);

    // Create destination parsing uuid object structure representing transaction bound matching
    boost::uuids::uuid _response_transaction_id;

    // Restore raw memory copying bytes sequentially mapping representation boundaries
    std::memcpy(_response_transaction_id.data, _response_body.data() + 4, 16);

    // Cross-validate that returned ID exactly equals source request tracking token element
    ASSERT_EQ(_transaction_id, _response_transaction_id);

    // Assure that expected successful command operations properly log status mapping bound variable flag
    ASSERT_EQ(_response_body[20], aurum::exit_code::success);

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly
    socket.close();

    wait_until([this] {
        return state->get_sessions().size() == 0;
    });

    ASSERT_EQ(state->get_sessions().size(), 0);
}

TEST_F(tcp_server_fixture, ConnectSendMultiplePayloadsAndDisconnect) {
    boost::asio::io_context client_io;
    boost::asio::ip::tcp::socket socket(client_io);

    unsigned short port =
            state->get_configuration().tcp_port_.load(std::memory_order_acquire);

    socket.connect({
        boost::asio::ip::address_v4::loopback(),
        port
    });

    wait_until([this] {
            return state->get_sessions().size() == 1;
        });

    ASSERT_EQ(state->get_sessions().size(), 1);

    // Track multiple transaction identifiers
    boost::uuids::uuid _transaction_id_1 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_2 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_3 = boost::uuids::random_generator()();

    // Setup multiple pings into a single payload frame boundary mapping limits
    aurum::protocol::frame_builder _frame_builder;
    auto _request_builder = _frame_builder.as_request();

    // Push the pings into the same frame mapping requests bounds
    _request_builder.add_ping(_transaction_id_1);
    _request_builder.add_ping(_transaction_id_2);
    _request_builder.add_ping(_transaction_id_3);

    // Get the composed payload with size mapping
    auto _payload = _request_builder.get_buffers();

    // Dump actual serialized payload through TCP sequence pipeline
    boost::asio::write(socket, boost::asio::buffer(_payload));

    // Verify response mapping loops boundaries correctly mapped.
    // Since the server processes the entire frame and returns a single frame with multiple responses,
    // we should read the response once and parse the responses within the frame.
    std::vector<boost::uuids::uuid> _expected_ids = { _transaction_id_1, _transaction_id_2, _transaction_id_3 };

    // Declare placeholder buffer integer for received server bound wrapper length
    uint32_t _response_header_length = 0;

    // Trigger socket bound sync read routine parsing frame limits explicitly
    boost::asio::read(socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)));

    // Reformat input stream bounds representation to little endian parsing
    boost::endian::little_to_native_inplace(_response_header_length);

    // Prepare receiving target dynamic array based off received length boundary
    std::vector<uint8_t> _response_body(_response_header_length);

    // Flush rest of the network stream frame sequence down towards local buffer space
    boost::asio::read(socket, boost::asio::buffer(_response_body));

    // Assure frame constraints limits hold valid minimum bounds limits rules
    ASSERT_GE(_response_body.size(), 4); // at least qty + crc

    // Declare quantity received placeholder tracker
    std::uint16_t _response_quantity;

    // Deep copy header structure bytes back mapping primitive properties
    std::memcpy(&_response_quantity, _response_body.data(), sizeof(_response_quantity));

    // Adapt back input array sequence mapping towards correct host primitive
    boost::endian::little_to_native_inplace(_response_quantity);

    // Compare structural output enforcing multiple ping return size bounds mapping limits bounds.
    ASSERT_EQ(_response_quantity, 3);

    size_t _offset = sizeof(_response_quantity);

    std::vector<std::uint16_t> _response_lengths;

    // Parse length mapping parameter boundaries variables properly constraints mapping.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Declare length parameter to capture response specific element length limit
        std::uint16_t _response_length;

        // Transfer mapping copying memory segment representation directly
        std::memcpy(&_response_length, _response_body.data() + _offset, sizeof(_response_length));

        // Correct memory formatting for native little endian processing unit architectures
        boost::endian::little_to_native_inplace(_response_length);

        // Ping length bounds expected explicitly sized to ID (16) and Exit (1) = 17
        ASSERT_EQ(_response_length, 17);

        _response_lengths.push_back(_response_length);
        _offset += sizeof(_response_length);
    }

    // Process response payload boundaries values parameters boundaries.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Create destination parsing uuid object structure representing transaction bound matching
        boost::uuids::uuid _response_transaction_id;

        // Restore raw memory copying bytes sequentially mapping representation boundaries
        std::memcpy(_response_transaction_id.data, _response_body.data() + _offset, 16);

        // Cross-validate that returned ID exactly equals source request tracking token element
        ASSERT_EQ(_expected_ids[_i], _response_transaction_id);

        // Advance bounds mapping loops pointers.
        _offset += 16;

        // Assure that expected successful command operations properly log status mapping bound variable flag
        ASSERT_EQ(_response_body[_offset], aurum::exit_code::success);

        // Adjust pointer limit bounds mapped sizes values.
        _offset += 1;
    }

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly
    socket.close();

    wait_until([this] {
        return state->get_sessions().size() == 0;
    });

    ASSERT_EQ(state->get_sessions().size(), 0);
}
