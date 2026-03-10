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

#ifndef AURUM_TCP_CLIENT_HPP
#define AURUM_TCP_CLIENT_HPP

#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <aurum/protocol/frame_builder.hpp>

namespace aurum {

    /**
     * @brief TCP client class handling raw socket communication.
     */
    class tcp_client {
        /** @brief IO context managing execution of asynchronous operations. */
        boost::asio::io_context io_context_;

        /** @brief TCP socket instance managing the active network connection. */
        boost::asio::ip::tcp::socket socket_;

        /** @brief Internal request builder for constructing payloads. */
        protocol::request_builder request_builder_;

    public:
        /**
         * @brief Default constructor for tcp_client.
         */
        tcp_client();

        /**
         * @brief Destructor for tcp_client.
         */
        ~tcp_client();

        /**
         * @brief Connects to the specified host and port.
         * @param host The remote server address to connect to.
         * @param port The target port number on the remote server.
         */
        void connect(const std::string& host, unsigned short port);

        /**
         * @brief Retrieves the underlying builder mapping incoming network structures.
         * @return A reference to the active request builder context.
         */
        protocol::request_builder& get_builder();

        /**
         * @brief Sends the binary payload across the connected socket interface.
         * @param data The payload byte stream structured mapping memory buffer blocks.
         */
        void send(const std::vector<std::uint8_t>& data);

        /**
         * @brief Reads a single frame returning raw payload sequences.
         * @return A consolidated vector containing total requested read bytes sequence mapping.
         */
        std::vector<std::uint8_t> read();

        /**
         * @brief Disconnects the underlying socket closing sequence transmission cleanly.
         */
        void disconnect();
    };

} // namespace aurum

#endif // AURUM_TCP_CLIENT_HPP
