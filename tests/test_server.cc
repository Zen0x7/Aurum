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

    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    auto _data = _client->get_builder().add_ping(_transaction_id).get_data();

    _client->send(_data);

    const auto _response = _client->read();

    ASSERT_GE(_response.size(), 8);

    const auto _response_header_length = aurum::test_utils::read_uint32_le(_response, 0);

    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 4);

    ASSERT_EQ(_response_quantity, 1);

    const auto _response_length = aurum::test_utils::read_uint16_le(_response, 6);

    ASSERT_EQ(_response_length, 19);

    ASSERT_EQ(_response[8], aurum::op_code::ping);

    ASSERT_EQ(_response[9], aurum::message_type::response);

    const auto _response_transaction_id = aurum::test_utils::read_uuid(_response, 10);

    ASSERT_EQ(_transaction_id, _response_transaction_id);

    ASSERT_EQ(_response[26], aurum::exit_code::success);

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

    boost::uuids::uuid _transaction_id_1 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_2 = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_3 = boost::uuids::random_generator()();

    auto _data = _client->get_builder()
        .add_ping(_transaction_id_1)
        .add_ping(_transaction_id_2)
        .add_ping(_transaction_id_3)
        .get_data();

    _client->send(_data);

    auto _response = _client->read();

    ASSERT_GE(_response.size(), 8);

    const std::vector _expected_ids = { _transaction_id_1, _transaction_id_2, _transaction_id_3 };

    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 4);

    ASSERT_EQ(_response_quantity, 3);

    size_t _offset = 4 + sizeof(_response_quantity);

    std::vector<std::uint16_t> _response_lengths;

    for (size_t _i = 0; _i < 3; ++_i) {
        const auto _response_length = aurum::test_utils::read_uint16_le(_response, _offset);

        ASSERT_EQ(_response_length, 19);

        _response_lengths.push_back(_response_length);
        _offset += sizeof(_response_length);
    }

    for (size_t _i = 0; _i < 3; ++_i) {
        ASSERT_EQ(_response[_offset], aurum::op_code::ping);
        _offset += 1;

        ASSERT_EQ(_response[_offset], aurum::message_type::response);
        _offset += 1;

        const auto _response_transaction_id = aurum::test_utils::read_uuid(_response, _offset);

        ASSERT_EQ(_expected_ids[_i], _response_transaction_id);

        _offset += 16;

        ASSERT_EQ(_response[_offset], aurum::exit_code::success);

        _offset += 1;
    }

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

    const boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    auto _data = _client->get_builder().as_request().add_ping(_transaction_id).get_data(false);

    _client->send(_data);

    const auto _response = _client->read();

    ASSERT_GE(_response.size(), 4);

    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response, 0);

    ASSERT_EQ(_response_quantity, 1);

    const auto _response_length = aurum::test_utils::read_uint16_le(_response, 2);

    ASSERT_EQ(_response_length, 19);

    ASSERT_EQ(_response[4], aurum::op_code::ping);

    ASSERT_EQ(_response[5], aurum::message_type::response);

    const auto _response_uuid = aurum::test_utils::read_uuid(_response, 6);

    ASSERT_EQ(_response_uuid, _transaction_id);

    ASSERT_EQ(_response[22], aurum::exit_code::success);

    _client->disconnect();

    aurum::test_utils::wait_until([this] {
            return state_->get_sessions().size() == 0;
        });

    ASSERT_EQ(state_->get_sessions().size(), 0);
}
