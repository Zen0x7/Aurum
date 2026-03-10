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

#pragma once

#include <aurum/node.hpp>
#include <aurum/state.hpp>

#include <thread>
#include <mutex>
#include <memory>

// Global context required to run the server in the background for benchmarks.
struct server_fixture {
    std::unique_ptr<aurum::node> node_;
    std::shared_ptr<aurum::state> state_;
    std::thread runner_thread_;
    int thread_count_;

    server_fixture(const int thread_count = 1) : node_(std::make_unique<aurum::node>()), thread_count_(thread_count) {
        state_ = node_->get_state();

        state_->get_configuration().tcp_port_.store(0, std::memory_order_release);
        state_->get_configuration().threads_.store(thread_count, std::memory_order_release);

        runner_thread_ = std::thread([this]() {
            node_->run();
        });

        while (!state_->get_configuration().tcp_ready_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    ~server_fixture() {
        node_->stop();
        if (runner_thread_.joinable()) {
            runner_thread_.join();
        }
    }

    unsigned short get_port() const {
        return state_->get_configuration().tcp_port_.load(std::memory_order_acquire);
    }
};

// Start a single server instance for all benchmarks.
inline std::unique_ptr<server_fixture> g_server;
inline int g_server_threads = 0;
inline std::mutex g_server_mutex;

inline void setup_server(int threads) {
    std::lock_guard _lock(g_server_mutex);
    if (!g_server || g_server_threads != threads) {
        g_server = std::make_unique<server_fixture>(threads);
        g_server_threads = threads;
    }
}
