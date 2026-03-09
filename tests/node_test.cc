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

#include "node_fixture.hpp"
#include <boost/uuid/random_generator.hpp>
#include <aurum/protocol.hpp>
#include <aurum/tcp_client.hpp>
#include <aurum/tcp_session.hpp>
#include <iostream>

TEST_F(node_fixture, ConnectIdentifyAndDiscovery) {
    const unsigned short _port_b = node_b_->get_state()->get_configuration().tcp_port_.load(std::memory_order_acquire);
    const unsigned short _port_a = node_a_->get_state()->get_configuration().tcp_port_.load(std::memory_order_acquire);

    const auto _client_a = std::make_shared<aurum::tcp_client>();

    _client_a->connect("127.0.0.1", _port_b);

    wait_until([this] {
        return node_b_->get_state()->get_sessions().size() == 1;
    });

    ASSERT_EQ(node_b_->get_state()->get_sessions().size(), 1);

    boost::uuids::uuid _transaction_id_identify = boost::uuids::random_generator()();
    boost::uuids::uuid _transaction_id_discovery = boost::uuids::random_generator()();

    // Node A sends identify with its own port and host
    auto [_data_identify, _frames_count_identify] = _client_a->get_builder()
        .add_identify(node_a_->get_state()->get_node_id(), _transaction_id_identify, _port_a, "127.0.0.1")
        .get_data();

    _client_a->send(_data_identify);

    const auto _response_identify = _client_a->read(_frames_count_identify);
    ASSERT_GE(_response_identify.size(), 8);

    // Wait until node b processed identify and set properties
    bool _properties_set = false;
    wait_until([this, &_properties_set] {
        std::shared_lock _lock(node_b_->get_state()->get_sessions_mutex());
        for(const auto& [_, _session] : node_b_->get_state()->get_sessions()) {
            if (_session->get_node_id() == node_a_->get_state()->get_node_id() && _session->get_port() > 0 && !_session->get_host().empty()) {
                _properties_set = true;
                return true;
            }
        }
        return false;
    });

    ASSERT_TRUE(_properties_set);

    _client_a->get_builder().flush();

    // Node A sends discovery to Node B
    auto [_data_discovery, _frames_count_discovery] = _client_a->get_builder()
        .add_discovery(_transaction_id_discovery)
        .get_data();

    _client_a->send(_data_discovery);

    const auto _response_discovery = _client_a->read(_frames_count_discovery);

    ASSERT_GE(_response_discovery.size(), 8);

    std::uint16_t _response_quantity;
    std::memcpy(&_response_quantity, _response_discovery.data() + 4, sizeof(_response_quantity));
    boost::endian::little_to_native_inplace(_response_quantity);
    ASSERT_EQ(_response_quantity, 1);

    std::uint16_t _response_length;
    std::memcpy(&_response_length, _response_discovery.data() + 6, sizeof(_response_length));
    boost::endian::little_to_native_inplace(_response_length);

    // Minimum size for discovery response: opcode (1) + type (1) + tx_id (16) + nodes_size (4) = 22
    ASSERT_GE(_response_length, 22);

    ASSERT_EQ(_response_discovery[8], aurum::op_code::discovery);
    ASSERT_EQ(_response_discovery[9], aurum::message_type::response);

    boost::uuids::uuid _response_transaction_id;
    std::memcpy(_response_transaction_id.data, _response_discovery.data() + 10, 16);
    ASSERT_EQ(_transaction_id_discovery, _response_transaction_id);

    std::uint32_t _nodes_size;
    std::memcpy(&_nodes_size, _response_discovery.data() + 26, sizeof(_nodes_size));
    boost::endian::little_to_native_inplace(_nodes_size);

    // Since node C is not connected yet, it should be 0.
    ASSERT_EQ(_nodes_size, 0);

    // Cleanup
    _client_a->disconnect();

    wait_until([this] {
        return node_b_->get_state()->get_sessions().size() == 0;
    });
}

TEST_F(node_fixture, ConnectDiscoverNodesAndConnectToThem) {
    const unsigned short _port_b = node_b_->get_state()->get_configuration().tcp_port_.load(std::memory_order_acquire);

    // Autonomously invoke node C's connection to node B.
    ASSERT_TRUE(node_c_->connect("127.0.0.1", _port_b));

    // Wait until node B registers node C's connection effectively.
    wait_until([this] { return node_b_->get_state()->get_sessions().size() == 1; });

    // Ensure node C's session correctly mapped the host and port dynamically via identify parsing.
    bool _c_properties_set = false;
    wait_until([this, &_c_properties_set] {
        std::shared_lock _lock(node_b_->get_state()->get_sessions_mutex());
        for (const auto& [_, _session] : node_b_->get_state()->get_sessions()) {
            if (_session->get_node_id() == node_c_->get_state()->get_node_id() && _session->get_port() > 0 && !_session->get_host().empty()) {
                _c_properties_set = true;
                return true;
            }
        }
        return false;
    });
    ASSERT_TRUE(_c_properties_set);

    // Node A autonomously connects to node B requesting discovery implicitly.
    ASSERT_TRUE(node_a_->connect("127.0.0.1", _port_b));

    // Wait until node B correctly acknowledges the second active session.
    wait_until([this] { return node_b_->get_state()->get_sessions().size() == 2; });

    // Because node A autonomously requested discovery within its initial frame layout,
    // the target node B automatically replied with a discovery response containing C's address.
    // Node A then automatically processes this response directly traversing its discovery handler natively.
    // Once A processes C's existence, it establishes a new link securely.
    wait_until([this] { return node_c_->get_state()->get_sessions().size() == 1; }, std::chrono::seconds(5));

    // Validate that node C correctly registered the connection actively sent autonomously by node A.
    ASSERT_EQ(node_c_->get_state()->get_sessions().size(), 1);

    // Validate node A locally recorded both connections (one to B, one to C).
    wait_until([this] { return node_a_->get_state()->get_sessions().size() == 2; }, std::chrono::seconds(5));
    ASSERT_EQ(node_a_->get_state()->get_sessions().size(), 2);

    // Cleanup resources to ensure thread sanitization handles closures predictably properly.
    node_a_->disconnect_all();
    node_c_->disconnect_all();

    // Verify closures effectively eliminated all remaining contexts.
    wait_until([this] { return node_b_->get_state()->get_sessions().size() == 0; });
    wait_until([this] { return node_c_->get_state()->get_sessions().size() == 0; });
}
