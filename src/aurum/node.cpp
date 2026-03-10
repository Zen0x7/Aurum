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
}

/**
 * @brief Destructs the node instance.
 * @details Ensures that the node is properly stopped before destruction.
 */
node::~node() {
    stop();
}

/**
 * @brief Parses command line arguments to configure the node.
 * @param argc The count of command line arguments.
 * @param argv An array of command line arguments strings.
 */
void node::parse_args(int argc, char* argv[]) {
    std::size_t _threads;

    unsigned short _port;

    unsigned short _websocket_port;

    boost::program_options::options_description _option_descriptions("Program options");

    _option_descriptions.add_options()
            ("threads", boost::program_options::value<std::size_t>(&_threads)->default_value(1))
            ("port", boost::program_options::value<unsigned short>(&_port)->default_value(0))
            ("websocket_port", boost::program_options::value<unsigned short>(&_websocket_port)->default_value(0));

    boost::program_options::variables_map _variables;

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, _option_descriptions), _variables);

    boost::program_options::notify(_variables);

    state_->get_configuration().tcp_port_.store(_port, std::memory_order_release);

    state_->get_configuration().threads_.store(_threads, std::memory_order_release);

    state_->get_configuration().websocket_port_.store(_websocket_port, std::memory_order_release);
}

/**
 * @brief Runs the node synchronously.
 * @details This function will start the listener and block the calling thread until the node is stopped.
 * @return Exit status code of the node.
 */
int node::run() {
    boost::asio::signal_set _signals(io_context_, SIGINT, SIGTERM);

    _signals.async_wait([&](auto, auto) { io_context_.stop(); });

    tcp_listener_ = std::make_shared<tcp_listener>(io_context_, state_);

    tcp_listener_->start();

    websocket_listener_ = std::make_shared<websocket_listener>(io_context_, state_);

    websocket_listener_->start();

    thread_pool_.reserve(state_->get_configuration().threads_.load(std::memory_order_acquire));

    for (std::size_t _index = 0; _index < state_->get_configuration().threads_.load(std::memory_order_acquire); ++_index) {
        thread_pool_.emplace_back([this] { io_context_.run(); });
    }

    for (auto& _thread : thread_pool_) {
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    return 0;
}

/**
 * @brief Stops the running node.
 * @details Signals the IO context to stop and interrupts the running execution safely.
 */
void node::stop() {
    io_context_.stop();
}

/**
 * @brief Establishes a synchronous outbound network connection directly towards a peer instance.
 * @param host The remote peer IP address structurally mapped string.
 * @param port The target destination listener port integer properly.
 * @return True if connection was completely established natively securely.
 */
bool node::connect(const std::string& host, unsigned short port) {
    return state_->connect(host, port, true);
}

/**
 * @brief Terminates dynamically an active network connection linked against a specific remote node identifier correctly.
 * @param remote_node_id The 16-byte identifier representing the active node context safely.
 */
void node::disconnect(boost::uuids::uuid remote_node_id) {
    state_->disconnect(remote_node_id);
}

/**
 * @brief Clears dynamically all internal sessions cleanly structurally bounds efficiently securely mapped natively.
 */
void node::disconnect_all() {
    state_->disconnect_all();
}

/**
 * @brief Retrieves the underlying shared state of the node.
 * @return A shared pointer to the node's state.
 */
std::shared_ptr<state> node::get_state() const {
    return state_;
}

/**
 * @brief Retrieves the underlying TCP listener of the node.
 * @return A shared pointer to the node's TCP listener.
 */
std::shared_ptr<tcp_listener> node::get_tcp_listener() const {
    return tcp_listener_;
}

/**
 * @brief Retrieves the underlying WebSocket listener cleanly natively mapping cleanly seamlessly accurately securely cleanly explicitly effectively expertly tracking cleanly smartly correctly reliably.
 * @return A shared pointer to the active websocket protocol handler gracefully smoothly cleanly intelligently flawlessly.
 */
std::shared_ptr<websocket_listener> node::get_websocket_listener() const {
    return websocket_listener_;
}

} // namespace aurum
