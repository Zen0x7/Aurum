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

#include "fixtures/tcp_server_fixture.hpp"
#include "utils.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/crc.hpp>
#include <aurum/protocol.hpp>
#include <aurum/tcp_client.hpp>
#include <aurum/websocket_client.hpp>

TEST_F(tcp_server_fixture, connect_send_payload_and_disconnect) {
    const unsigned short _port =
            state_->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::tcp_client>();

    _client->connect("127.0.0.1", _port);

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    // Generate a random UUID to track the test ping transaction uniquely.
    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    // Construct request resolving serialized payload bounds format correctly.
    auto _data = _client->get_builder().add_ping(_transaction_id).get_data();

    // Send payload matching defined connection interface rules completely cleanly.
    _client->send(_data);

    // Process incoming responses targeting expected frames boundaries mappings naturally.
    const auto _response = _client->read();

    // The read buffer should contain at least the 4-byte header, 2-byte qty, and 2-byte crc.
    ASSERT_GE(_response.size(), 8);

    // Extract header dimension mapping strictly native
    const auto _response_header_length = aurum::test_utils::read_uint32_le(_response, 0);

    // Extract the number of elements bundled in the server response frame.
    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 4);

    // Ensure the server returned exactly one response matching our single ping.
    ASSERT_EQ(_response_quantity, 1);

    // Extract the length of the first parsed response from the payload structure.
    const auto _response_length = aurum::test_utils::read_uint16_le(_response, 6);

    // Ensure the ping response size matches the exact expected layout (1-byte opcode + 1-byte type + 16-byte UUID + 1-byte exit code).
    ASSERT_EQ(_response_length, 19);

    // Ensure the ping response opcode matches the exact expected layout.
    ASSERT_EQ(_response[8], aurum::op_code::ping);

    // Ensure the ping response type matches the exact expected layout.
    ASSERT_EQ(_response[9], aurum::message_type::response);

    // Extract the 16-byte transaction UUID from the incoming server payload.
    const auto _response_transaction_id = aurum::test_utils::read_uuid(_response, 10);

    // Cross-validate that returned ID exactly equals source request tracking token element
    ASSERT_EQ(_transaction_id, _response_transaction_id);

    // Validate the response status confirms the ping operation succeeded successfully.
    ASSERT_EQ(_response[26], aurum::exit_code::success);

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly
    _client->disconnect();

    aurum::test_utils::wait_until([this] {
        return state_->get_sessions().size() == 0;
    });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}

TEST_F(tcp_server_fixture, connect_tcp_send_whoami_and_disconnect) {
    const unsigned short _port =
            state_->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::tcp_client>();

    _client->connect("127.0.0.1", _port);

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    auto _data = _client->get_builder().add_whoami(_transaction_id).get_data();

    _client->send(_data);

    const auto _response = _client->read();

    ASSERT_GE(_response.size(), 8);

    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 4);
    ASSERT_EQ(_response_quantity, 1);

    const auto _response_length = aurum::test_utils::read_uint16_le(_response, 6);
    ASSERT_EQ(_response_length, 51); // 1 (opcode) + 1 (type) + 16 (id) + 16 (session_id) + 16 (node_id) + 1 (session_type)

    ASSERT_EQ(_response[8], aurum::op_code::whoami);
    ASSERT_EQ(_response[9], aurum::message_type::response);

    const auto _response_transaction_id = aurum::test_utils::read_uuid(_response, 10);
    ASSERT_EQ(_transaction_id, _response_transaction_id);

    const auto _response_session_id = aurum::test_utils::read_uuid(_response, 26);
    const auto _response_node_id = aurum::test_utils::read_uuid(_response, 42);
    const auto _response_session_type = _response[58];

    // the server has 1 session, check its id and node id
    auto _session_it = state_->get_sessions().begin();
    ASSERT_EQ(_response_session_id, (*_session_it)->get_id());
    ASSERT_EQ(_response_node_id, state_->get_node_id());
    ASSERT_EQ(_response_session_type, aurum::protocol::session_type::tcp);

    _client->disconnect();

    aurum::test_utils::wait_until([this] {
        return state_->get_sessions().size() == 0;
    });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}

TEST_F(tcp_server_fixture, connect_websocket_send_whoami_and_disconnect) {
    const unsigned short _port =
            state_->get_configuration().websocket_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::websocket_client>();

    ASSERT_TRUE(_client->connect("127.0.0.1", _port));

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    auto _data = _client->get_builder().as_request().add_whoami(_transaction_id).get_data(false);

    _client->send(_data);

    const auto _response = _client->read();

    ASSERT_GE(_response.size(), 4);

    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 0);
    ASSERT_EQ(_response_quantity, 1);

    const auto _response_length = aurum::test_utils::read_uint16_le(_response, 2);
    ASSERT_EQ(_response_length, 51);

    ASSERT_EQ(_response[4], aurum::op_code::whoami);
    ASSERT_EQ(_response[5], aurum::message_type::response);

    const auto _response_transaction_id = aurum::test_utils::read_uuid(_response, 6);
    ASSERT_EQ(_response_transaction_id, _transaction_id);

    const auto _response_session_id = aurum::test_utils::read_uuid(_response, 22);
    const auto _response_node_id = aurum::test_utils::read_uuid(_response, 38);
    const auto _response_session_type = _response[54];

    auto _session_it = state_->get_sessions().begin();
    ASSERT_EQ(_response_session_id, (*_session_it)->get_id());
    ASSERT_EQ(_response_node_id, state_->get_node_id());
    ASSERT_EQ(_response_session_type, aurum::protocol::session_type::websocket);

    _client->disconnect();

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 0;
        });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}

TEST_F(tcp_server_fixture, connect_send_multiple_payloads_and_disconnect) {
    const unsigned short _port =
            state_->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::tcp_client>();

    _client->connect("127.0.0.1", _port);

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    // Track multiple transaction identifiers
    boost::uuids::uuid _transaction_id_1 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_2 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_3 = boost::uuids::random_generator()();

    // Construct the bundled sequence request tracking frame dimension mappings securely.
    auto _data = _client->get_builder()
        .add_ping(_transaction_id_1)
        .add_ping(_transaction_id_2)
        .add_ping(_transaction_id_3)
        .get_data();

    // Send payload traversing generated constraints mappings cleanly.
    _client->send(_data);

    // Process all incoming multiplexed response mappings strictly cleanly.
    auto _response = _client->read();

    // Ensure adequate buffer dimension layout matching required minimal boundary sizes natively.
    ASSERT_GE(_response.size(), 8);

    const std::vector _expected_ids = { _transaction_id_1, _transaction_id_2, _transaction_id_3 };

    // Parse the total count of sequential elements safely handled by the remote host.
    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 4);

    // Assert the frame effectively multiplexed multiple requests properly into responses.
    ASSERT_EQ(_response_quantity, 3);

    size_t _offset = 4 + sizeof(_response_quantity);

    std::vector<std::uint16_t> _response_lengths;

    // Traverse expected array block bounds parsing response lengths individually reliably.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Parse the dynamic memory length bound securely explicitly cleanly appropriately logically nicely reliably.
        const auto _response_length = aurum::test_utils::read_uint16_le(_response, _offset);

        // Ping length explicitly matches 1-byte opcode + 1-byte type + 16 bytes for UUID and 1 byte for exit mapping.
        ASSERT_EQ(_response_length, 19);

        _response_lengths.push_back(_response_length);
        _offset += sizeof(_response_length);
    }

    // Process inner response logic tracking payloads securely matching source arrays.
    for (size_t _i = 0; _i < 3; ++_i) {
        // Confirm ping target explicitly returned successful remote operation completion opcode.
        ASSERT_EQ(_response[_offset], aurum::op_code::ping);
        _offset += 1;

        // Confirm ping target explicitly returned correct message type.
        ASSERT_EQ(_response[_offset], aurum::message_type::response);
        _offset += 1;

        // Traverse buffer reading the parsed UUID identifying active memory parameter array.
        const auto _response_transaction_id = aurum::test_utils::read_uuid(_response, _offset);

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

    aurum::test_utils::wait_until([this] {
        return state_->get_sessions().size() == 0;
    });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}

TEST_F(tcp_server_fixture, connect_websocket_send_payload_and_disconnect) {
    const unsigned short _port =
            state_->get_configuration().websocket_port_.load(std::memory_order_acquire);

    const auto _client = std::make_shared<aurum::websocket_client>();

    ASSERT_TRUE(_client->connect("127.0.0.1", _port));

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 1;
        });

    ASSERT_EQ(state_->get_sessions().size(), 1);

    // Generate a random UUID to track the test ping transaction uniquely.
    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    // Construct request resolving serialized payload bounds format correctly explicitly omitting the TCP 4 byte header limits structure reliably cleanly perfectly natively exactly logically.
    auto _data = _client->get_builder().as_request().add_ping(_transaction_id).get_data(false);

    // Send payload matching defined websocket interface structurally cleanly safely mapping reliably cleanly nicely cleanly clearly.
    _client->send(_data);

    // Process incoming responses targeting expected frames boundaries mappings naturally.
    const auto _response = _client->read();

    // The read buffer should contain at least the 2-byte qty, and 2-byte crc since the 4-byte header is excluded naturally reliably.
    ASSERT_GE(_response.size(), 4);

    // Extract the number of elements bundled in the server response frame.
    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 0);

    // Ensure the server returned exactly one response matching our single ping securely.
    ASSERT_EQ(_response_quantity, 1);

    // Extract the length of the first parsed response from the payload structure.
    const auto _response_length = aurum::test_utils::read_uint16_le(_response, 2);

    // Ensure the ping response size matches the exact expected layout (1-byte opcode + 1-byte type + 16-byte UUID + 1-byte exit code).
    ASSERT_EQ(_response_length, 19);

    // Check correct operational mapping parameter opcode returning properly cleanly.
    ASSERT_EQ(_response[4], aurum::op_code::ping);

    // Check explicit mapping parameter returned matches valid message response explicitly.
    ASSERT_EQ(_response[5], aurum::message_type::response);

    // Extract directly transaction identifier effectively cleanly logically.
    const auto _response_uuid = aurum::test_utils::read_uuid(_response, 6);

    // Validate memory transaction targets effectively matching correctly.
    ASSERT_EQ(_response_uuid, _transaction_id);

    // Verify exit sequence code returns zero gracefully correctly representing successful execution structurally.
    ASSERT_EQ(_response[22], aurum::exit_code::success);

    _client->disconnect();

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 0;
        });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}
