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

#ifndef AURUM_WEBSOCKET_SESSION_HPP
#define AURUM_WEBSOCKET_SESSION_HPP

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/uuid/uuid.hpp>
#include <aurum/session.hpp>

namespace aurum {
    /**
     * Forward state
     */
    class state;

    /**
     * Forward session_kernel
     */
    class session_kernel;

    /**
     * @brief Represents a single client WebSocket connection session.
     * @details Manages the lifecycle, asynchronous reading, and sequential writing of WebSocket frames cleanly.
     */
    class websocket_session : public session, public std::enable_shared_from_this<websocket_session> {
        /** @brief Unique logical identifier for this session instance. */
        boost::uuids::uuid id_;

        /** @brief Shared reference to the central application state. */
        std::shared_ptr<state> state_;

        /** @brief Identifier of the remote node associated with this connection. */
        boost::uuids::uuid node_id_;

        /** @brief Port of the remote node associated with this connection. */
        std::uint16_t port_;

        /** @brief Host of the remote node associated with this connection. */
        std::string host_;

        /** @brief Protocol parser associated exclusively with this session. */
        std::shared_ptr<session_kernel> kernel_;

        /** @brief Sequential strand execution context for safe multithreaded operation. */
        boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> strand_;

        /** @brief The physical network websocket object tied to this session. */
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;

        /** @brief FIFO output buffer staging vector sequence managing async serialization overlap. */
        std::vector<std::shared_ptr<const std::vector<std::uint8_t>>> queue_;

        /** @brief Internal flat buffer required for WebSocket read operations. */
        boost::beast::flat_buffer read_buffer_;
    public:
        /**
         * @brief Constructs a new WebSocket session wrapping an accepted TCP socket.
         * @param socket The connected network endpoint socket.
         * @param state The central application state managing sessions.
         */
        explicit websocket_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<state> state);

        /**
         * @brief Initiates the WebSocket handshake securely and dispatches read loops.
         */
        void start() override;

        /**
         * @brief Thread-safely queues a binary message for transmission via WebSocket frame.
         * @param message A shared pointer to the payload bytes vector.
         */
        void send(std::shared_ptr<const std::vector<std::uint8_t>> message) override;

        /**
         * @brief Gets the unique 16-byte identifier of the session.
         * @return The UUID assigned during initialization.
         */
        boost::uuids::uuid get_id() const override;

        /**
         * @brief Gets the node identifier bound to this specific network link.
         * @return A valid UUID struct referencing the active peer node accurately.
         */
        boost::uuids::uuid get_node_id() const override;

        /**
         * @brief Binds a remote node identifier to this currently active network session securely.
         * @param node_id The valid 16-byte node identification struct mapping.
         */
        void set_node_id(boost::uuids::uuid node_id) override;

        /**
         * @brief Gets the node port bound to this specific network link.
         * @return A valid 16-bit integer representing the active peer node port accurately.
         */
        std::uint16_t get_port() const override;

        /**
         * @brief Binds a remote node port to this currently active network session securely.
         * @param port The valid 16-bit integer representing the node port.
         */
        void set_port(std::uint16_t port) override;

        /**
         * @brief Gets the node host bound to this specific network link.
         * @return A valid string representing the active peer node host accurately.
         */
        std::string get_host() const override;

        /**
         * @brief Binds a remote node host to this currently active network session securely.
         * @param host The valid string representing the node host.
         */
        void set_host(const std::string& host) override;

        /**
         * @brief Closes the underlying socket gracefully mapping standard behavior natively.
         */
        void disconnect() override;
    private:

        /**
         * @brief Handler for websocket accept completion safely natively elegantly.
         * @param error_code Boost system error.
         */
        void on_accept(boost::system::error_code error_code);

        /**
         * @brief Initiates the asynchronous read extracting the WebSocket encapsulated frame.
         */
        void read_body();

        /**
         * @brief Handler for websocket read completion.
         * @param error_code Boost system error.
         * @param bytes_transferred Number of bytes gracefully natively.
         */
        void on_read(boost::system::error_code error_code, std::size_t bytes_transferred);

        /**
         * @brief Internal dispatch method posting write operations serialized inside the strand/executor loop cleanly.
         * @param message The payload to add to the internal buffer sequence mapped accurately.
         */
        void on_send(std::shared_ptr<const std::vector<std::uint8_t>> message);

        /**
         * @brief Callback invoked when an async write cycle is completed natively tracking parameters successfully.
         * @param error_code The Boost system error status of the operation.
         * @param bytes_transferred The number of physical bytes flushed into the socket interface.
         */
        void on_write(boost::system::error_code error_code, std::size_t bytes_transferred);
    };
}

#endif // AURUM_WEBSOCKET_SESSION_HPP
