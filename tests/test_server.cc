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

    // Generate a random UUID to track the test ping transaction uniquely.
    boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    // Instantiate a frame builder to construct the ping request payload.
    aurum::protocol::frame_builder _frame_builder;
    // Extract a request-specific builder to format outgoing client messages.
    auto _request_builder = _frame_builder.as_request();
    // Insert a new ping command tagged with the generated tracking identifier.
    _request_builder.add_ping(_transaction_id);

    // Extract the complete binary payload ready for network transmission.
    auto _payload = _request_builder.get_buffers();

    // Dump actual serialized payload through TCP sequence pipeline
    boost::asio::write(socket, boost::asio::buffer(_payload));

    // Declare placeholder buffer integer for received server bound wrapper length
    uint32_t _response_header_length = 0;

    // Read the exact 4 bytes of the header to discover the body size.
    boost::asio::read(socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)));

    // Reformat incoming header length from network little-endian to native architecture.
    boost::endian::little_to_native_inplace(_response_header_length);

    // Prepare receiving target dynamic array based off received length boundary
    std::vector<uint8_t> _response_body(_response_header_length);

    // Flush rest of the network stream frame sequence down towards local buffer space
    boost::asio::read(socket, boost::asio::buffer(_response_body));

    // Verify the response body is large enough to contain at least the quantity and CRC.
    ASSERT_GE(_response_body.size(), 4); // at least qty + crc

    // Declare quantity received placeholder tracker
    std::uint16_t _response_quantity;

    // Extract the number of elements bundled in the server response frame.
    std::memcpy(&_response_quantity, _response_body.data(), sizeof(_response_quantity));

    // Convert the elements quantity from little-endian back to native architecture.
    boost::endian::little_to_native_inplace(_response_quantity);

    // Ensure the server returned exactly one response matching our single ping.
    ASSERT_EQ(_response_quantity, 1);

    // Declare length parameter to capture response specific element length limit
    std::uint16_t _response_length;

    // Copy the length of the first parsed response from the payload structure.
    std::memcpy(&_response_length, _response_body.data() + 2, sizeof(_response_length));

    // Correct memory formatting for native little endian processing unit architectures
    boost::endian::little_to_native_inplace(_response_length);

    // Ensure the ping response size matches the exact expected layout (16-byte UUID + 1-byte exit code).
    ASSERT_EQ(_response_length, 17);

    // Create destination parsing uuid object structure representing transaction bound matching
    boost::uuids::uuid _response_transaction_id;

    // Extract the 16-byte transaction UUID from the incoming server payload.
    std::memcpy(_response_transaction_id.data, _response_body.data() + 4, 16);

    // Cross-validate that returned ID exactly equals source request tracking token element
    ASSERT_EQ(_transaction_id, _response_transaction_id);

    // Validate the response status confirms the ping operation succeeded successfully.
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

    // Instantiate a frame builder to construct a bundled sequence of requests.
    aurum::protocol::frame_builder _frame_builder;
    auto _request_builder = _frame_builder.as_request();

    // Enqueue three unique ping requests dynamically into the shared builder frame.
    _request_builder.add_ping(_transaction_id_1);
    _request_builder.add_ping(_transaction_id_2);
    _request_builder.add_ping(_transaction_id_3);

    // Resolve the constructed payload dynamically returning exact binary layout sequence.
    auto _payload = _request_builder.get_buffers();

    // Dump actual serialized payload through TCP sequence pipeline
    boost::asio::write(socket, boost::asio::buffer(_payload));

    // Verify response frame accurately reflects the multiplexed structure logic.
    // Since the server processes the entire frame and returns a single frame with multiple responses,
    // we should read the response once and parse the responses within the frame.
    std::vector<boost::uuids::uuid> _expected_ids = { _transaction_id_1, _transaction_id_2, _transaction_id_3 };

    // Declare placeholder buffer integer for received server bound wrapper length
    uint32_t _response_header_length = 0;

    // Trigger synchronous stream fetch extracting just the length marker explicitly.
    boost::asio::read(socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)));

    // Reformat incoming network format into native system integer logic structure.
    boost::endian::little_to_native_inplace(_response_header_length);

    // Prepare receiving target dynamic array based off received length boundary
    std::vector<uint8_t> _response_body(_response_header_length);

    // Flush rest of the network stream frame sequence down towards local buffer space
    boost::asio::read(socket, boost::asio::buffer(_response_body));

    // Enforce minimal expected memory layout preventing out of bounds parsing routines.
    ASSERT_GE(_response_body.size(), 4); // at least qty + crc

    // Declare quantity received placeholder tracker
    std::uint16_t _response_quantity;

    // Parse the total count of sequential elements safely handled by the remote host.
    std::memcpy(&_response_quantity, _response_body.data(), sizeof(_response_quantity));

    // Format structural boundary size tracking native integer sequence logic layout.
    boost::endian::little_to_native_inplace(_response_quantity);

    // Assert the frame effectively multiplexed multiple requests properly into responses.
    ASSERT_EQ(_response_quantity, 3);

    size_t _offset = sizeof(_response_quantity);

    std::vector<std::uint16_t> _response_lengths;

    // Traverse expected array block bounds parsing response lengths individually reliably.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Declare length parameter to capture response specific element length limit
        std::uint16_t _response_length;

        // Copy raw memory pointer length dynamically reflecting current parsing bound.
        std::memcpy(&_response_length, _response_body.data() + _offset, sizeof(_response_length));

        // Correct memory formatting for native little endian processing unit architectures
        boost::endian::little_to_native_inplace(_response_length);

        // Ping length explicitly matches 16 bytes for UUID and 1 byte for exit mapping.
        ASSERT_EQ(_response_length, 17);

        _response_lengths.push_back(_response_length);
        _offset += sizeof(_response_length);
    }

    // Process inner response logic tracking payloads securely matching source arrays.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Create destination parsing uuid object structure representing transaction bound matching
        boost::uuids::uuid _response_transaction_id;

        // Traverse buffer reading the parsed UUID identifying active memory parameter array.
        std::memcpy(_response_transaction_id.data, _response_body.data() + _offset, 16);

        // Cross-validate that returned ID exactly equals source request tracking token element
        ASSERT_EQ(_expected_ids[_i], _response_transaction_id);

        // Advance byte cursor matching dynamic loop iterations across valid target frames.
        _offset += 16;

        // Confirm ping target explicitly returned successful remote operation completion.
        ASSERT_EQ(_response_body[_offset], aurum::exit_code::success);

        // Adjust tracking index offset to process trailing checksum values securely.
        _offset += 1;
    }

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly
    socket.close();

    wait_until([this] {
        return state->get_sessions().size() == 0;
    });

    ASSERT_EQ(state->get_sessions().size(), 0);
}
