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

#include <aurum/node.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/connect.hpp>
#include <aurum/tcp_session.hpp>

namespace aurum {

/**
 * @brief Constructs a new node instance.
 * @details Initializes the shared state and prepares the context.
 */
node::node() : state_(std::make_shared<state>(io_context_)) {
    // Constructor initializes the shared state tracking configuration and sessions.
}

/**
 * @brief Destructs the node instance.
 * @details Ensures that the node is properly stopped before destruction.
 */
node::~node() {
    // Invoke the stop procedure to ensure threads and io_context are terminated.
    stop();
}

/**
 * @brief Parses command line arguments to configure the node.
 * @param argc The count of command line arguments.
 * @param argv An array of command line arguments strings.
 */
void node::parse_args(int argc, char* argv[]) {
    // Define variable storing active thread count parameter.
    std::size_t _threads;

    // Define variable storing application listener port.
    unsigned short _port;

    // Initialize command line options parser instance.
    boost::program_options::options_description _option_descriptions("Program options");

    // Define program options with defaults for threads and port.
    _option_descriptions.add_options()
            ("threads", boost::program_options::value<std::size_t>(&_threads)->default_value(1))
            ("port", boost::program_options::value<unsigned short>(&_port)->default_value(0));

    // Construct local variable map container.
    boost::program_options::variables_map _variables;

    // Parse the command line options.
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, _option_descriptions), _variables);

    // Notify the variables map to update bound variables.
    boost::program_options::notify(_variables);

    // Persist parsed application port parameter into configuration.
    state_->get_configuration().tcp_port_.store(_port, std::memory_order_release);

    // Persist parsed thread count into configuration.
    state_->get_configuration().threads_.store(_threads, std::memory_order_release);
}

/**
 * @brief Runs the node synchronously.
 * @details This function will start the listener and block the calling thread until the node is stopped.
 * @return Exit status code of the node.
 */
int node::run() {
    // Register async application shutdown handlers for SIGINT and SIGTERM.
    boost::asio::signal_set _signals(io_context_, SIGINT, SIGTERM);

    // Bind async wait to stop the io_context on signal.
    _signals.async_wait([&](auto, auto) { io_context_.stop(); });

    // Spawn networking connection listener targeting incoming traffic.
    tcp_listener_ = std::make_shared<tcp_listener>(io_context_, state_);

    // Start accepting TCP connections.
    tcp_listener_->start();

    // Preallocate vector capacity for worker threads.
    thread_pool_.reserve(state_->get_configuration().threads_.load(std::memory_order_acquire));

    // Spawn the requested number of worker threads.
    for (std::size_t _index = 0; _index < state_->get_configuration().threads_.load(std::memory_order_acquire); ++_index) {
        // Run the io_context in each thread.
        thread_pool_.emplace_back([this] { io_context_.run(); });
    }

    // Wait for all threads to complete.
    for (auto& _thread : thread_pool_) {
        // Check if thread is joinable before joining to prevent errors.
        if (_thread.joinable()) {
            // Block until the thread finishes execution.
            _thread.join();
        }
    }

    // Return success code.
    return 0;
}

/**
 * @brief Stops the running node.
 * @details Signals the IO context to stop and interrupts the running execution safely.
 */
void node::stop() {
    // Stop the IO context preventing further async operations.
    io_context_.stop();
}

/**
 * @brief Establishes a synchronous outbound network connection directly towards a peer instance.
 * @param host The remote peer IP address structurally mapped string.
 * @param port The target destination listener port integer properly.
 * @return True if connection was completely established natively securely.
 */
bool node::connect(const std::string& host, unsigned short port) {
    // Delegate the connection logic to the application state handler properly initializing the networking request securely.
    // By default, explicit node instances directly initiated connections request peer discovery seamlessly.
    return state_->connect(host, port, true);
}

/**
 * @brief Terminates dynamically an active network connection linked against a specific remote node identifier correctly.
 * @param remote_node_id The 16-byte identifier representing the active node context safely.
 */
void node::disconnect(boost::uuids::uuid remote_node_id) {
    // Pass the command to terminate the connection to the application state manager.
    state_->disconnect(remote_node_id);
}

/**
 * @brief Clears dynamically all internal sessions cleanly structurally bounds efficiently securely mapped natively.
 */
void node::disconnect_all() {
    // Instruct the central state object to close all registered active network sessions.
    state_->disconnect_all();
}

/**
 * @brief Retrieves the underlying shared state of the node.
 * @return A shared pointer to the node's state.
 */
std::shared_ptr<state> node::get_state() const {
    // Return the internal state tracker object pointer.
    return state_;
}

/**
 * @brief Retrieves the underlying TCP listener of the node.
 * @return A shared pointer to the node's TCP listener.
 */
std::shared_ptr<tcp_listener> node::get_tcp_listener() const {
    // Return the internal listener instance pointer.
    return tcp_listener_;
}

} // namespace aurum
