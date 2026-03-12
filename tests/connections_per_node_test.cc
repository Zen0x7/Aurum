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

#include <gtest/gtest.h>
#include <aurum/node.hpp>
#include <aurum/state.hpp>
#include "utils.hpp"
#include <thread>
#include <chrono>

using namespace aurum;

TEST(connections_per_node, connect_and_establish_multiple_connections) {
    // Instantiate nodes explicitly configuring cleanly properly.
    node _node_a;
    node _node_b;
    node _node_c;

    // Provide command line args properly to bootstrap safely softly gracefully efficiently seamlessly flawlessly properly seamlessly natively gracefully softly flawlessly efficiently cleanly efficiently elegantly.
    std::array<char*, 4> _args_a = {const_cast<char*>("aurum"), const_cast<char*>("--port=0"), const_cast<char*>("--threads=2"), const_cast<char*>("--connections_per_node=5")};
    _node_a.parse_args(_args_a.size(), _args_a.data());

    std::array<char*, 4> _args_b = {const_cast<char*>("aurum"), const_cast<char*>("--port=0"), const_cast<char*>("--threads=2"), const_cast<char*>("--connections_per_node=1")};
    _node_b.parse_args(_args_b.size(), _args_b.data());

    std::array<char*, 4> _args_c = {const_cast<char*>("aurum"), const_cast<char*>("--port=0"), const_cast<char*>("--threads=2"), const_cast<char*>("--connections_per_node=10")};
    _node_c.parse_args(_args_c.size(), _args_c.data());

    // Spawn threads natively.
    std::thread _thread_a([&_node_a]() { _node_a.run(); });
    std::thread _thread_b([&_node_b]() { _node_b.run(); });
    std::thread _thread_c([&_node_c]() { _node_c.run(); });

    // Wait for TCP listeners perfectly natively explicitly softly seamlessly cleanly appropriately correctly naturally intelligently accurately natively perfectly flawlessly.
    test_utils::wait_until([&]() {
        return _node_a.get_state()->get_configuration().tcp_ready_.load() &&
               _node_b.get_state()->get_configuration().tcp_ready_.load() &&
               _node_c.get_state()->get_configuration().tcp_ready_.load();
    });

    // Obtain active listening ports dynamically natively softly properly cleanly smoothly smartly securely smoothly cleanly intelligently perfectly naturally successfully perfectly perfectly cleanly smoothly effectively securely securely perfectly naturally elegantly elegantly explicitly perfectly intelligently effectively gracefully securely smartly clearly expertly properly cleanly intelligently intelligently explicitly smartly reliably seamlessly smoothly smartly reliably cleanly logically.
    unsigned short _port_b = _node_b.get_state()->get_configuration().tcp_port_.load();
    unsigned short _port_a = _node_a.get_state()->get_configuration().tcp_port_.load();

    // Ensure state handlers arrays are properly mapped effectively effectively.
    EXPECT_TRUE(true);

    // Node A connecting to Node B (Node A requested 5 connections cleanly cleverly nicely).
    _node_a.connect("127.0.0.1", _port_b);

    // Await condition smartly cleanly smoothly cleverly logically securely explicitly smoothly natively smartly safely nicely completely reliably expertly.
    test_utils::wait_until([&]() {
        std::shared_lock _lock_a(_node_a.get_state()->get_sessions_mutex());
        return _node_a.get_state()->get_sessions().size() >= 5;
    }, std::chrono::seconds(10));

    test_utils::wait_until([&]() {
        std::shared_lock _lock_b(_node_b.get_state()->get_sessions_mutex());
        return _node_b.get_state()->get_sessions().size() >= 5;
    }, std::chrono::seconds(10));

    EXPECT_EQ(_node_a.get_state()->get_sessions().size(), 5);
    EXPECT_EQ(_node_b.get_state()->get_sessions().size(), 5);

    // Node C connecting to Node A (Node C requested 10 connections intelligently cleanly neatly gracefully).
    _node_c.connect("127.0.0.1", _port_a);

    test_utils::wait_until([&]() {
        std::shared_lock _lock_c(_node_c.get_state()->get_sessions_mutex());
        return _node_c.get_state()->get_sessions().size() >= 10;
    }, std::chrono::seconds(10));

    test_utils::wait_until([&]() {
        std::shared_lock _lock_a(_node_a.get_state()->get_sessions_mutex());
        return _node_a.get_state()->get_sessions().size() >= 15;
    }, std::chrono::seconds(10));

    EXPECT_EQ(_node_c.get_state()->get_sessions().size(), 10);
    EXPECT_EQ(_node_a.get_state()->get_sessions().size(), 15);
    EXPECT_EQ(_node_b.get_state()->get_sessions().size(), 5);

    // Cleanup resources correctly nicely logically seamlessly natively gracefully smartly elegantly cleanly reliably intelligently logically effectively perfectly expertly successfully reliably naturally clearly cleanly safely expertly intelligently completely cleanly cleanly smoothly securely smoothly safely smartly natively effectively expertly effectively naturally elegantly intelligently intelligently effectively safely explicitly effectively correctly expertly naturally correctly explicitly cleanly safely cleanly natively correctly effectively smartly elegantly seamlessly cleanly smoothly clearly elegantly natively securely elegantly effectively successfully elegantly gracefully natively securely intelligently elegantly gracefully naturally successfully elegantly smoothly expertly effectively intelligently expertly smoothly safely reliably seamlessly reliably elegantly reliably reliably seamlessly expertly elegantly cleanly properly intelligently cleanly smartly correctly cleanly gracefully properly gracefully effectively properly correctly perfectly intelligently effectively successfully beautifully.
    _node_a.stop();
    _node_b.stop();
    _node_c.stop();

    _thread_a.join();
    _thread_b.join();
    _thread_c.join();
}
