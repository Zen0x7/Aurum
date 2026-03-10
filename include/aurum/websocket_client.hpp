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

#ifndef AURUM_WEBSOCKET_CLIENT_HPP
#define AURUM_WEBSOCKET_CLIENT_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <vector>
#include <cstdint>
#include <string>
#include <aurum/protocol/frame_builder.hpp>

namespace aurum {
    /**
     * @brief A synchronous WebSocket client utility designed exclusively for integration testing completely.
     */
    class websocket_client {
        /** @brief Execution context. */
        boost::asio::io_context io_context_;

        /** @brief The actual synchronous websocket stream. */
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;

        /** @brief Frame builder instance. */
        protocol::frame_builder builder_;
    public:
        /**
         * @brief Constructs a new synchronous websocket client.
         */
        websocket_client();

        /**
         * @brief Establishes a synchronous websocket connection to the specified endpoint.
         * @param host Target host.
         * @param port Target port.
         * @return True if connected and successfully upgraded.
         */
        bool connect(const std::string& host, unsigned short port);

        /**
         * @brief Sends a binary buffer payload synchronously via websocket frame.
         * @param message Buffer containing the serialized frame.
         */
        void send(const std::vector<std::uint8_t>& message);

        /**
         * @brief Reads synchronously the response payload from a websocket frame.
         * @return Byte array containing the response frame.
         */
        std::vector<std::uint8_t> read();

        /**
         * @brief Access the internal frame builder.
         * @return Frame builder reference.
         */
        protocol::frame_builder& get_builder();

        /**
         * @brief Terminates the websocket connection.
         */
        void disconnect();
    };
}

#endif // AURUM_WEBSOCKET_CLIENT_HPP
