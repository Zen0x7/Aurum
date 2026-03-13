#include <gtest/gtest.h>
#include <aurum/node.hpp>
#include <thread>
#include <chrono>
#include "utils.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include "fixtures/node_fixture.hpp"

class connections_per_node : public ::testing::Test {
protected:
    std::unique_ptr<aurum::node> node_a_;
    std::unique_ptr<aurum::node> node_b_;
    std::thread runner_thread_a_;
    std::thread runner_thread_b_;

    void SetUp() override {
        node_a_ = std::make_unique<aurum::node>();
        node_b_ = std::make_unique<aurum::node>();

        node_a_->get_state()->get_configuration().tcp_port_.store(0);
        node_a_->get_state()->get_configuration().threads_.store(1);
        node_a_->get_state()->get_configuration().connections_per_node_.store(5);

        node_b_->get_state()->get_configuration().tcp_port_.store(0);
        node_b_->get_state()->get_configuration().threads_.store(1);
        node_b_->get_state()->get_configuration().connections_per_node_.store(1);

        runner_thread_a_ = std::thread([this] { node_a_->run(); });
        runner_thread_b_ = std::thread([this] { node_b_->run(); });

        aurum::test_utils::wait_until([this] {
            return node_a_->get_state()->get_configuration().tcp_ready_.load() &&
                   node_b_->get_state()->get_configuration().tcp_ready_.load();
        });
    }

    void TearDown() override {
        node_a_->disconnect_all();
        node_b_->disconnect_all();

        aurum::test_utils::wait_until([this] {
            return node_a_->get_state()->get_sessions().size() == 0 &&
                   node_b_->get_state()->get_sessions().size() == 0;
        });

        node_a_->stop();
        node_b_->stop();

        if (runner_thread_a_.joinable())
            runner_thread_a_.join();
        if (runner_thread_b_.joinable())
            runner_thread_b_.join();
    }
};

TEST_F(connections_per_node, node_a_initiates_multiple_connections) {
    auto _port_b = node_b_->get_state()->get_configuration().tcp_port_.load();

    // A connects to B
    ASSERT_TRUE(node_a_->connect("127.0.0.1", _port_b));

    auto _node_a_id = node_a_->get_state()->get_node_id();
    auto _node_b_id = node_b_->get_state()->get_node_id();

    // Give some time for identify and connect to round-trip
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check node A has 5 sessions mapped to B's node ID
    bool _a_has_5 = false;
    for (int i = 0; i < 50; ++i) { // 50 * 100ms = 5s timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::shared_lock _lock(node_a_->get_state()->get_sessions_mutex());
        auto& _idx = node_a_->get_state()->get_sessions().get<aurum::by_node_id>();
        if (_idx.count(_node_b_id) == 5) {
            _a_has_5 = true;
            break;
        }
    }

    // Check node B has 5 sessions mapped to A's node ID
    bool _b_has_5 = false;
    for (int i = 0; i < 50; ++i) { // 50 * 100ms = 5s timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::shared_lock _lock(node_b_->get_state()->get_sessions_mutex());
        auto& _idx = node_b_->get_state()->get_sessions().get<aurum::by_node_id>();
        if (_idx.count(_node_a_id) == 5) {
            _b_has_5 = true;
            break;
        }
    }

    ASSERT_TRUE(_a_has_5) << "Node A did not establish 5 connections to Node B";
    ASSERT_TRUE(_b_has_5) << "Node B did not establish 5 connections back to Node A";
}
