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

#include "fixtures/node_fixture.hpp"
#include "utils.hpp"
#include <boost/uuid/random_generator.hpp>
#include <aurum/protocol.hpp>
#include <aurum/tcp_client.hpp>
#include <aurum/tcp_session.hpp>
#include <iostream>

TEST_F(node_fixture, connect_to_node_and_send_identify_and_discovery) {
    const unsigned short _port_b = node_b_->get_state()->get_configuration().tcp_port_.load(std::memory_order_acquire);
    const unsigned short _port_a = node_a_->get_state()->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client_a = std::make_shared<aurum::tcp_client>();

    _client_a->connect("127.0.0.1", _port_b);

    aurum::test_utils::wait_until([this] {
        return node_b_->get_state()->get_sessions().size() == 1;
    });

    ASSERT_EQ(node_b_->get_state()->get_sessions().size(), 1);

    boost::uuids::uuid _transaction_id_identify = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_discovery = boost::uuids::random_generator()();

    auto _data_identify = _client_a->get_builder()
        .add_identify(node_a_->get_state()->get_node_id(), _transaction_id_identify, _port_a, "127.0.0.1")
        .get_data();

    _client_a->send(_data_identify);

    const auto _response_identify = _client_a->read();
    ASSERT_GE(_response_identify.size(), 8);

    bool _properties_set = false;
    aurum::test_utils::wait_until([this, &_properties_set] {
        std::shared_lock _lock(node_b_->get_state()->get_sessions_mutex());
        for(const auto& _session : node_b_->get_state()->get_sessions()) {
            if (_session->get_node_id() == node_a_->get_state()->get_node_id() && _session->get_port() > 0 && !_session->get_host().empty()) {
                _properties_set = true;
                return true;
            }
        }
        return false;
    });

    ASSERT_TRUE(_properties_set);

    _client_a->get_builder().flush();

    auto _data_discovery = _client_a->get_builder()
        .add_discovery(_transaction_id_discovery)
        .get_data();

    _client_a->send(_data_discovery);

    const auto _response_discovery = _client_a->read();

    ASSERT_GE(_response_discovery.size(), 8);

    const auto _response_quantity = aurum::test_utils::read_uint16_le(_response_discovery, 4);
    ASSERT_EQ(_response_quantity, 1);

    const auto _response_length = aurum::test_utils::read_uint16_le(_response_discovery, 6);

    ASSERT_GE(_response_length, 22);

    ASSERT_EQ(_response_discovery[8], aurum::op_code::discovery);
    ASSERT_EQ(_response_discovery[9], aurum::message_type::response);

    const auto _response_transaction_id = aurum::test_utils::read_uuid(_response_discovery, 10);
    ASSERT_EQ(_transaction_id_discovery, _response_transaction_id);

    const auto _nodes_size = aurum::test_utils::read_uint32_le(_response_discovery, 26);

    ASSERT_EQ(_nodes_size, 0);

    _client_a->disconnect();

    aurum::test_utils::wait_until([this] {
        return node_b_->get_state()->get_sessions().size() == 0;
    });
}

TEST_F(node_fixture, connect_discover_nodes_and_connect_to_them) {
    const unsigned short _port_b = node_b_->get_state()->get_configuration().tcp_port_.load(std::memory_order_acquire);

    ASSERT_TRUE(node_c_->connect("127.0.0.1", _port_b));

    aurum::test_utils::wait_until([this] { return node_b_->get_state()->get_sessions().size() == 1; });

    bool _c_properties_set = false;
    aurum::test_utils::wait_until([this, &_c_properties_set] {
        std::shared_lock _lock(node_b_->get_state()->get_sessions_mutex());
        for (const auto& _session : node_b_->get_state()->get_sessions()) {
            if (_session->get_node_id() == node_c_->get_state()->get_node_id() && _session->get_port() > 0 && !_session->get_host().empty()) {
                _c_properties_set = true;
                return true;
            }
        }
        return false;
    });
    ASSERT_TRUE(_c_properties_set);

    ASSERT_TRUE(node_a_->connect("127.0.0.1", _port_b));

    aurum::test_utils::wait_until([this] { return node_b_->get_state()->get_sessions().size() == 2; });

    aurum::test_utils::wait_until([this] { return node_c_->get_state()->get_sessions().size() == 2; }, std::chrono::seconds(5));

    ASSERT_EQ(node_c_->get_state()->get_sessions().size(), 2);

    aurum::test_utils::wait_until([this] { return node_a_->get_state()->get_sessions().size() == 2; }, std::chrono::seconds(5));
    ASSERT_EQ(node_a_->get_state()->get_sessions().size(), 2);

    ASSERT_EQ(node_b_->get_state()->get_sessions().size(), 2);

    node_a_->disconnect_all();
    node_b_->disconnect_all();
    node_c_->disconnect_all();

    aurum::test_utils::wait_until([this] { return node_b_->get_state()->get_sessions().size() == 0; });
    aurum::test_utils::wait_until([this] { return node_c_->get_state()->get_sessions().size() == 0; });
}
