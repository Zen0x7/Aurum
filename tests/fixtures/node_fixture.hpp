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

#ifndef AURUM_NODE_FIXTURE_HPP
#define AURUM_NODE_FIXTURE_HPP

#include <gtest/gtest.h>

#include <aurum/node.hpp>
#include <aurum/state.hpp>
#include <aurum/configuration.hpp>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>

#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include "../utils.hpp"

class node_fixture : public ::testing::Test {
protected:
    std::unique_ptr<aurum::node> node_a_;
    std::unique_ptr<aurum::node> node_b_;
    std::unique_ptr<aurum::node> node_c_;

    std::thread runner_thread_a_;
    std::thread runner_thread_b_;
    std::thread runner_thread_c_;

    void SetUp() override {
        node_a_ = std::make_unique<aurum::node>();
        node_b_ = std::make_unique<aurum::node>();
        node_c_ = std::make_unique<aurum::node>();

        node_a_->get_state()->get_configuration().tcp_port_.store(0);
        node_a_->get_state()->get_configuration().threads_.store(1);

        node_b_->get_state()->get_configuration().tcp_port_.store(0);
        node_b_->get_state()->get_configuration().threads_.store(1);

        node_c_->get_state()->get_configuration().tcp_port_.store(0);
        node_c_->get_state()->get_configuration().threads_.store(1);

        runner_thread_a_ = std::thread([this] { node_a_->run(); });
        runner_thread_b_ = std::thread([this] { node_b_->run(); });
        runner_thread_c_ = std::thread([this] { node_c_->run(); });

        aurum::test_utils::wait_until([this] {
            return node_a_->get_state()->get_configuration().tcp_ready_.load() &&
                   node_b_->get_state()->get_configuration().tcp_ready_.load() &&
                   node_c_->get_state()->get_configuration().tcp_ready_.load();
        });
    }

    void TearDown() override {
        node_a_->stop();
        node_b_->stop();
        node_c_->stop();

        if (runner_thread_a_.joinable())
            runner_thread_a_.join();
        if (runner_thread_b_.joinable())
            runner_thread_b_.join();
        if (runner_thread_c_.joinable())
            runner_thread_c_.join();
    }
};

#endif // AURUM_NODE_FIXTURE_HPP