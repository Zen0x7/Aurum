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

#ifndef AURUM_TCP_SESSION_HPP
#define AURUM_TCP_SESSION_HPP

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/uuid/uuid.hpp>

namespace aurum {
    /**
     * Forward state
     */
    class state;

    /**
     * Forward tcp_kernel
     */
    class tcp_kernel;

    /**
     * @brief Represents a single client TCP connection session.
     * @details Manages the lifecycle, asynchronous reading, and sequential writing of network frames.
     */
    class tcp_session : public std::enable_shared_from_this<tcp_session> {
        /** @brief Unique logical identifier for this session instance. */
        boost::uuids::uuid id_;

        /** @brief Shared reference to the central application state. */
        std::shared_ptr<state> state_;

        /** @brief Protocol parser associated exclusively with this session. */
        std::shared_ptr<tcp_kernel> kernel_;
    public:
        /**
         * @brief Constructs a new TCP session using an accepted socket.
         * @param socket The connected network endpoint socket.
         * @param state The central application state managing sessions.
         */
        explicit tcp_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<state> state);

        /**
         * @brief Initiates the asynchronous reading cycle for the session.
         */
        void start();

        /**
         * @brief Thread-safely queues a binary message for transmission.
         * @param message A shared pointer to the payload bytes vector.
         */
        void send(std::shared_ptr<const std::vector<std::uint8_t>> message);

        /**
         * @brief Gets the unique 16-byte identifier of the session.
         * @return The UUID assigned during initialization.
         */
        boost::uuids::uuid get_id() const;
    private:
        /**
         * @brief Initiates an asynchronous read targeting the 4-byte frame header limit.
         */
        void read_header();

        /**
         * @brief Initiates an asynchronous read allocating the full payload size determined by the header.
         */
        void read_body();

        /**
         * @brief Internal dispatch method posting write operations serialized inside the strand/executor loop.
         * @param message The payload to add to the internal buffer sequence.
         */
        void on_send(std::shared_ptr<const std::vector<std::uint8_t>> message);

        /**
         * @brief Callback invoked when an async write cycle is completed.
         * @param error_code The Boost system error status of the operation.
         * @param bytes_transferred The number of physical bytes flushed into the socket interface.
         */
        void on_write(boost::system::error_code error_code, std::size_t bytes_transferred);

        /** @brief The physical network socket object tied to this session. */
        boost::asio::ip::tcp::socket socket_;

        /** @brief Cached header value tracking the upcoming payload limits boundary. */
        std::uint32_t header_length_ { 0 };

        /** @brief FIFO output buffer staging vector sequence managing async serialization overlap. */
        std::vector<std::shared_ptr<const std::vector<std::uint8_t>>> queue_;
    };
}

#endif // AURUM_TCP_SESSION_HPP
