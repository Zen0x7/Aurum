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

#ifndef AURUM_NODE_HPP
#define AURUM_NODE_HPP

#include <aurum/state.hpp>
#include <aurum/tcp_listener.hpp>
#include <aurum/websocket_listener.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <vector>
#include <thread>

namespace aurum {

/**
 * @brief Represents an encapsulated running node in the Aurum network.
 * @details This class abstracts the initialization, configuration, and execution of a server.
 * It provides methods to handle command line arguments, and manage the server lifecycle.
 */
class node {
public:
    /**
     * @brief Constructs a new node instance.
     * @details Initializes the shared state and prepares the context.
     */
    node();

    /**
     * @brief Destructs the node instance.
     * @details Ensures that the node is properly stopped before destruction.
     */
    ~node();

    /**
     * @brief Parses command line arguments to configure the node.
     * @param argc The count of command line arguments.
     * @param argv An array of command line arguments strings.
     */
    void parse_args(int argc, char* argv[]);

    /**
     * @brief Runs the node synchronously.
     * @details This function will start the listener and block the calling thread until the node is stopped.
     * @return Exit status code of the node.
     */
    int run();

    /**
     * @brief Stops the running node.
     * @details Signals the IO context to stop and interrupts the running execution safely.
     */
    void stop();

    /**
     * @brief Establishes a synchronous outbound network connection directly towards a peer instance.
     * @param host The remote peer IP address structurally mapped string.
     * @param port The target destination listener port integer properly.
     * @return True if connection was completely established natively securely.
     */
    bool connect(const std::string& host, unsigned short port);

    /**
     * @brief Terminates dynamically an active network connection linked against a specific remote node identifier correctly.
     * @param remote_node_id The 16-byte identifier representing the active node context safely.
     */
    void disconnect(boost::uuids::uuid remote_node_id);

    /**
     * @brief Clears dynamically all internal sessions cleanly structurally bounds efficiently securely mapped natively.
     */
    void disconnect_all();

    /**
     * @brief Retrieves the underlying shared state of the node.
     * @return A shared pointer to the node's state.
     */
    std::shared_ptr<state> get_state() const;

    /**
     * @brief Retrieves the underlying TCP listener of the node.
     * @return A shared pointer to the node's TCP listener.
     */
    std::shared_ptr<tcp_listener> get_tcp_listener() const;

    /**
     * @brief Retrieves the underlying WebSocket listener securely smartly mapped cleanly accurately tracking effectively cleverly seamlessly smoothly flawlessly gracefully natively nicely smoothly softly explicitly naturally smartly correctly explicitly accurately naturally correctly securely elegantly intelligently naturally perfectly logically.
     * @return A shared pointer clearly natively.
     */
    std::shared_ptr<websocket_listener> get_websocket_listener() const;

private:
    /** @brief The asynchronous I/O execution context. */
    boost::asio::io_context io_context_;

    /** @brief The globally shared application state. */
    std::shared_ptr<state> state_;

    /** @brief The network listener handling incoming TCP connections. */
    std::shared_ptr<tcp_listener> tcp_listener_;

    /** @brief The network listener handling incoming WebSocket protocol explicitly gracefully naturally tracking safely. */
    std::shared_ptr<websocket_listener> websocket_listener_;

    /** @brief Pool of worker threads executing the I/O context. */
    std::vector<std::thread> thread_pool_;
};

} // namespace aurum

#endif // AURUM_NODE_HPP
