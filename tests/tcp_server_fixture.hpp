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

#ifndef AURUM_TEST_FIXTURE_HPP
#define AURUM_TEST_FIXTURE_HPP

#include <gtest/gtest.h>

#include <aurum/node.hpp>
#include <aurum/state.hpp>
#include <aurum/configuration.hpp>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>

#include <thread>
#include <atomic>
#include <memory>

class tcp_server_fixture : public ::testing::Test {
protected:
    std::unique_ptr<aurum::node> test_node;
    std::shared_ptr<aurum::state> state;
    std::thread runner_thread;

    void SetUp() override {
        test_node = std::make_unique<aurum::node>();
        state = test_node->get_state();

        state->get_configuration().tcp_port_.store(0);
        state->get_configuration().threads_.store(1);

        runner_thread = std::thread([this] { test_node->run(); });

        wait_until([this] {
            return state->get_configuration().tcp_ready_.load();
        });
    }

    void TearDown() override {
        test_node->stop();
        if (runner_thread.joinable())
            runner_thread.join();
    }

    template<class Predicate>
    static void wait_until(Predicate condition,
                           std::chrono::milliseconds timeout = std::chrono::seconds(2)) {
        const auto _start = std::chrono::steady_clock::now();
        while (!condition()) {
            if (std::chrono::steady_clock::now() - _start > timeout) {
                FAIL() << "wait_until timeout";
            }
            std::this_thread::yield();
        }
    }
};

#endif // AURUM_TEST_FIXTURE_HPP
