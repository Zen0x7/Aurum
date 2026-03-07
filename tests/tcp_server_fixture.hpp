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

#include <aurum/state.hpp>
#include <aurum/configuration.hpp>
#include <aurum/tcp_listener.hpp>
#include <aurum/tcp_session.hpp>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>

#include <thread>
#include <atomic>

class tcp_server_fixture : public ::testing::Test {
protected:
    boost::asio::io_context io;
    std::shared_ptr<aurum::state> state;
    std::unique_ptr<aurum::tcp_listener> listener;
    std::thread io_thread;

    void SetUp() override {
        state = std::make_shared<aurum::state>();

        state->get_configuration().tcp_port_.store(0);
        state->get_configuration().threads_.store(1);

        listener = std::make_unique<aurum::tcp_listener>(io, state);
        listener->start();

        io_thread = std::thread([this] { io.run(); });

        wait_until([this] {
                  return state->get_configuration().tcp_ready_.load();
              });
    }

    void TearDown() override {
        io.stop();
        if (io_thread.joinable())
            io_thread.join();
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
