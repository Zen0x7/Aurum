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

/**
 * @brief Base test fixture initializing a live dummy TCP server for integration tests.
 * @details Spins up an asynchronous io_context thread dynamically assigning an ephemeral port binding.
 */
class tcp_server_fixture : public ::testing::Test {
protected:
    /** @brief Main execution context for managing asynchronous IO operations. */
    boost::asio::io_context io;

    /** @brief Shared application state required for server initialization. */
    std::shared_ptr<aurum::state> state;

    /** @brief The server listener handling incoming TCP connections. */
    std::unique_ptr<aurum::tcp_listener> listener;

    /** @brief Background thread used to run the IO context loop. */
    std::thread io_thread;

    /**
     * @brief Prepares the test environment by starting an ephemeral dummy server.
     */
    void SetUp() override {
        // Initialize the shared state to track configurations and sessions.
        state = std::make_shared<aurum::state>();

        // Assign port 0 to force the OS to pick an ephemeral, available port dynamically.
        state->get_configuration().tcp_port_.store(0);

        // Limit the threads configuration to 1 for basic testing purposes.
        state->get_configuration().threads_.store(1);

        // Instantiate the TCP listener bound to the testing IO context and state.
        listener = std::make_unique<aurum::tcp_listener>(io, state);

        // Start the listener to begin accepting connections.
        listener->start();

        // Spawn a background worker thread that executes the async IO event loop.
        io_thread = std::thread([this] { io.run(); });

        // Block the main test execution until the server successfully bounds the port.
        wait_until([this] {
                  // Check if the listener flipped the readiness flag to true.
                  return state->get_configuration().tcp_ready_.load();
              });
    }

    /**
     * @brief Cleans up resources, stopping the server loop and joining threads.
     */
    void TearDown() override {
        // Signal the IO context to stop processing any further events.
        io.stop();
        // Check if the background IO loop thread is still running.
        if (io_thread.joinable())
            // Wait for the background thread to safely exit.
            io_thread.join();
    }

    /**
     * @brief Blocks execution synchronously yielding until a condition evaluates to true or a timeout occurs.
     * @tparam Predicate Functor evaluating to boolean.
     * @param condition The lambda function expressing the condition to wait for.
     * @param timeout The maximum allowed wait time before forcing a test failure.
     */
    template<class Predicate>
    static void wait_until(Predicate condition,
                           std::chrono::milliseconds timeout = std::chrono::seconds(2)) {
        // Capture the start time using a monotonic clock for accurate duration tracking.
        const auto _start = std::chrono::steady_clock::now();

        // Loop continuously as long as the condition returns false.
        while (!condition()) {
            // Compare the elapsed time against the timeout threshold.
            if (std::chrono::steady_clock::now() - _start > timeout) {
                // If the timeout is exceeded, fail the GoogleTest immediately.
                FAIL() << "wait_until timeout";
            }
            // Yield the current thread to the OS scheduler to avoid busy-waiting unnecessarily.
            std::this_thread::yield();
        }
    }
};

#endif // AURUM_TEST_FIXTURE_HPP
