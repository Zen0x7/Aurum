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
#include <aurum/tcp_client.hpp>

TEST_F(tcp_server_fixture, ConnectSendPayloadAndDisconnect) {
    const unsigned short _port =
            state_->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::tcp_client>();

    _client->connect("127.0.0.1", _port);

    wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    // Generate a random UUID to track the test ping transaction uniquely.
    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    // Construct request resolving serialized payload bounds format correctly.
    auto [_data, _frames_count] = _client->get_builder().add_ping(_transaction_id).get_data();

    // Send payload matching defined connection interface rules completely cleanly.
    _client->send(_data);

    // Process incoming responses targeting expected frames boundaries mappings naturally.
    const auto _response = _client->read(_frames_count);

    // The read buffer should contain at least the 4-byte header, 2-byte qty, and 2-byte crc.
    ASSERT_GE(_response.size(), 8);

    // Create pointer tracking initial sequence limit bounds representing payload header accurately.
    std::uint32_t _response_header_length;
    // Extract actual parsed frame dimension constraints parsing memory format strictly.
    std::memcpy(&_response_header_length, _response.data(), sizeof(_response_header_length));
    // Enforce correct processor alignment ensuring little-endian bounds parsing logic naturally.
    boost::endian::little_to_native_inplace(_response_header_length);

    // Declare quantity received placeholder tracker
    std::uint16_t _response_quantity;

    // Extract the number of elements bundled in the server response frame.
    std::memcpy(&_response_quantity, _response.data() + 4, sizeof(_response_quantity));

    // Convert the elements quantity from little-endian back to native architecture.
    boost::endian::little_to_native_inplace(_response_quantity);

    // Ensure the server returned exactly one response matching our single ping.
    ASSERT_EQ(_response_quantity, 1);

    // Declare length parameter to capture response specific element length limit
    std::uint16_t _response_length;

    // Copy the length of the first parsed response from the payload structure.
    std::memcpy(&_response_length, _response.data() + 6, sizeof(_response_length));

    // Correct memory formatting for native little endian processing unit architectures
    boost::endian::little_to_native_inplace(_response_length);

    // Ensure the ping response size matches the exact expected layout (16-byte UUID + 1-byte exit code).
    ASSERT_EQ(_response_length, 17);

    // Create destination parsing uuid object structure representing transaction bound matching
    boost::uuids::uuid _response_transaction_id;

    // Extract the 16-byte transaction UUID from the incoming server payload.
    std::memcpy(_response_transaction_id.data, _response.data() + 8, 16);

    // Cross-validate that returned ID exactly equals source request tracking token element
    ASSERT_EQ(_transaction_id, _response_transaction_id);

    // Validate the response status confirms the ping operation succeeded successfully.
    ASSERT_EQ(_response[24], aurum::exit_code::success);

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly
    _client->disconnect();

    wait_until([this] {
        return state_->get_sessions().size() == 0;
    });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}

TEST_F(tcp_server_fixture, ConnectSendMultiplePayloadsAndDisconnect) {
    const unsigned short _port =
            state_->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::tcp_client>();

    _client->connect("127.0.0.1", _port);

    wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    // Track multiple transaction identifiers
    boost::uuids::uuid _transaction_id_1 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_2 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_3 = boost::uuids::random_generator()();

    // Construct the bundled sequence request tracking frame dimension mappings securely.
    auto [_data, _frames_count] = _client->get_builder()
        .add_ping(_transaction_id_1)
        .add_ping(_transaction_id_2)
        .add_ping(_transaction_id_3)
        .get_data();

    // Send payload traversing generated constraints mappings cleanly.
    _client->send(_data);

    // Process all incoming multiplexed response mappings strictly cleanly.
    auto _response = _client->read(_frames_count);

    // Ensure adequate buffer dimension layout matching required minimal boundary sizes natively.
    ASSERT_GE(_response.size(), 8);

    const std::vector _expected_ids = { _transaction_id_1, _transaction_id_2, _transaction_id_3 };

    // Declare quantity received placeholder tracker
    std::uint16_t _response_quantity;

    // Parse the total count of sequential elements safely handled by the remote host.
    std::memcpy(&_response_quantity, _response.data() + 4, sizeof(_response_quantity));

    // Format structural boundary size tracking native integer sequence logic layout.
    boost::endian::little_to_native_inplace(_response_quantity);

    // Assert the frame effectively multiplexed multiple requests properly into responses.
    ASSERT_EQ(_response_quantity, 3);

    size_t _offset = 4 + sizeof(_response_quantity);

    std::vector<std::uint16_t> _response_lengths;

    // Traverse expected array block bounds parsing response lengths individually reliably.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Declare length parameter to capture response specific element length limit
        std::uint16_t _response_length;

        // Copy raw memory pointer length dynamically reflecting current parsing bound.
        std::memcpy(&_response_length, _response.data() + _offset, sizeof(_response_length));

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
        std::memcpy(_response_transaction_id.data, _response.data() + _offset, 16);

        // Cross-validate that returned ID exactly equals source request tracking token element
        ASSERT_EQ(_expected_ids[_i], _response_transaction_id);

        // Advance byte cursor matching dynamic loop iterations across valid target frames.
        _offset += 16;

        // Confirm ping target explicitly returned successful remote operation completion.
        ASSERT_EQ(_response[_offset], aurum::exit_code::success);

        // Adjust tracking index offset to process trailing checksum values securely.
        _offset += 1;
    }

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly
    _client->disconnect();

    wait_until([this] {
        return state_->get_sessions().size() == 0;
    });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}
