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

#ifndef AURUM_WEBSOCKET_LISTENER_HPP
#define AURUM_WEBSOCKET_LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <memory>

namespace aurum {
    /**
     * Forward state
     */
    class state;

    /**
     * @brief Asynchronous WebSocket server listener managing incoming client connections.
     * @details Opens a server socket, accepts incoming connections, and spawns new websocket_session instances cleanly.
     */
    class websocket_listener {
        /** @brief Shared pointer to the central application state. */
        std::shared_ptr<state> state_;

        /** @brief The Boost Asio acceptor used to listen for incoming connections. */
        boost::asio::ip::tcp::acceptor acceptor_;
    public:
        /**
         * @brief Constructs a new WebSocket listener bounded to the specified IO context.
         * @param io_context The Boost Asio execution context.
         * @param state A shared pointer to the application state containing configuration details.
         */
        websocket_listener(boost::asio::io_context &io_context, std::shared_ptr<state> state);

        /**
         * @brief Retrieves the application state reference held by the listener.
         * @return A mutable reference to the shared state pointer.
         */
        std::shared_ptr<state> & get_state();

        /**
         * @brief Starts the asynchronous connection acceptance loop.
         */
        void start();
    private:
        /**
         * @brief Internal method performing the asynchronous accept operation.
         */
        void do_accept();
    };
}

#endif // AURUM_WEBSOCKET_LISTENER_HPP
