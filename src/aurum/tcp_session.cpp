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

#include <aurum/state.hpp>
#include <aurum/tcp_session.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/asio/read.hpp>

#include <boost/endian/conversion.hpp>
#include <aurum/session_kernel.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace aurum {
    /**
     * @brief Constructs a new TCP session using an accepted socket.
     * @param socket The connected network endpoint socket.
     * @param state The central application state managing sessions.
     */
    tcp_session::tcp_session(boost::asio::ip::tcp::socket socket,
                             std::shared_ptr<state> state) : session(protocol::tcp),
                                                             id_(boost::uuids::random_generator()()),
                                                             state_(std::move(state)),
                                                             node_id_(boost::uuids::nil_uuid()),
                                                             port_(0),
                                                             host_(""),
                                                             kernel_(std::make_shared<session_kernel>(state_)),
                                                             socket_(std::move(socket)),
                                                             strand_(make_strand(socket_.get_executor())) {
    }

    /**
     * @brief Initiates the asynchronous reading cycle for the session.
     */
    void tcp_session::start() {
        post(strand_, [this, _self = shared_from_this()]() {
            read_header();
        });
    }

    /**
     * @brief Thread-safely queues a binary message for transmission.
     * @param message A shared pointer to the payload bytes vector.
     */
    void tcp_session::send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        post(
            strand_,
            [this, _self = shared_from_this(), _message = message]() {
                on_send(std::move(_message));
            }
        );
    }

    /**
     * @brief Internal dispatch method posting write operations serialized inside the strand/executor loop.
     * @param message The payload to add to the internal buffer sequence.
     */
    void tcp_session::on_send(std::shared_ptr<const std::vector<std::uint8_t>> message) {

        queue_.push_back(std::move(message));

        if (queue_.size() > 1) {
            return;
        }

        async_write(
            socket_,
            boost::asio::buffer(*queue_.front()),
            bind_executor(strand_, [this, _self = shared_from_this()](const boost::system::error_code& error_code, std::size_t bytes_transferred) {
                on_write(error_code, bytes_transferred);
            })
        );
    }

    /**
     * @brief Callback invoked when an async write cycle is completed.
     * @param error_code The Boost system error status of the operation.
     * @param bytes_transferred The number of physical bytes flushed into the socket interface.
     */
    void tcp_session::on_write(boost::system::error_code error_code, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (error_code) {
            state_->remove_session(get_id());
            return;
        }

        queue_.erase(queue_.begin());

        if (!queue_.empty()) {
            async_write(
                socket_,
                boost::asio::buffer(*queue_.front()),
                bind_executor(strand_, [this, _self = shared_from_this()](const boost::system::error_code& scoped_error_code, const std::size_t scoped_bytes_transferred) {
                    on_write(scoped_error_code, scoped_bytes_transferred);
                })
            );
        }
    }

    /**
     * @brief Gets the unique 16-byte identifier of the session.
     * @return The UUID assigned during initialization.
     */
    boost::uuids::uuid tcp_session::get_id() const {
        return id_;
    }

    /**
     * @brief Gets the node identifier bound to this specific network link.
     * @return A valid UUID struct referencing the active peer node accurately.
     */
    boost::uuids::uuid tcp_session::get_node_id() const {
        return node_id_;
    }

    /**
     * @brief Binds a remote node identifier to this currently active network session securely.
     * @param node_id The valid 16-byte node identification struct mapping.
     */
    void tcp_session::set_node_id(boost::uuids::uuid node_id) {
        node_id_ = node_id;
    }

    /**
     * @brief Gets the node port bound to this specific network link.
     * @return A valid 16-bit integer representing the active peer node port accurately.
     */
    std::uint16_t tcp_session::get_port() const {
        return port_;
    }

    /**
     * @brief Binds a remote node port to this currently active network session securely.
     * @param port The valid 16-bit integer representing the node port.
     */
    void tcp_session::set_port(std::uint16_t port) {
        port_ = port;
    }

    /**
     * @brief Gets the node host bound to this specific network link.
     * @return A valid string representing the active peer node host accurately.
     */
    std::string tcp_session::get_host() const {
        return host_;
    }

    /**
     * @brief Binds a remote node host to this currently active network session securely.
     * @param host The valid string representing the node host.
     */
    void tcp_session::set_host(const std::string& host) {
        host_ = host;
    }

    /**
     * @brief Closes the underlying socket gracefully cleanly.
     */
    void tcp_session::disconnect() {
        if (socket_.is_open()) {
            boost::system::error_code _ec;
            socket_.close(_ec);
        }
    }

    /**
     * @brief Initiates an asynchronous read targeting the 4-byte frame header limit.
     */
    void tcp_session::read_header() {
        auto _self = shared_from_this();
        async_read(socket_, boost::asio::buffer(&header_length_, sizeof(header_length_)),
                   bind_executor(strand_, [this, _self](const boost::system::error_code &error_code, std::size_t bytes_transferred) {
                       boost::ignore_unused(bytes_transferred);
                       boost::endian::little_to_native_inplace(header_length_);

                       if (!error_code) {
                           read_body();
                       } else {
                           state_->remove_session(get_id());
                       }
                   }));
    }

    /**
     * @brief Initiates an asynchronous read allocating the full payload size determined by the header.
     */
    void tcp_session::read_body() {
        auto _self = shared_from_this();
        auto _body = std::make_shared<std::vector<std::uint8_t> >(header_length_);
        async_read(socket_, boost::asio::buffer(*_body),
                   bind_executor(strand_, [this, _self, _body](const boost::system::error_code &error_code, std::size_t bytes_transferred) {
                       boost::ignore_unused(bytes_transferred);
                       if (!error_code) {

                           if (const auto _response = kernel_->handle(_body, _self); _response) {
                               send(_response);
                           }

                           read_header();
                       } else {
                           state_->remove_session(get_id());
                       }
                   }));
    }
}
