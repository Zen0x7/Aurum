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
#include <aurum/websocket_session.hpp>

#include <boost/core/ignore_unused.hpp>
#include <aurum/session_kernel.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <iostream>

namespace aurum {

    websocket_session::websocket_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<state> state)
        : session(protocol::websocket),
          id_(boost::uuids::random_generator()()),
          state_(std::move(state)),
          node_id_(boost::uuids::nil_uuid()),
          port_(0),
          host_(""),
          kernel_(std::make_shared<session_kernel>(state_)),
          strand_(boost::asio::make_strand(socket.get_executor())),
          ws_(std::move(socket)) {
    }

    void websocket_session::start() {
        boost::asio::post(strand_, [this, _self = shared_from_this()]() {
            ws_.async_accept(boost::beast::bind_front_handler(&websocket_session::on_accept, shared_from_this()));
        });
    }

    void websocket_session::on_accept(boost::system::error_code error_code) {
        if (!error_code) {
            ws_.binary(true);
            read_body();
        } else {
            state_->remove_session(get_id());
        }
    }

    void websocket_session::send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        boost::asio::post(strand_, [this, _self = shared_from_this(), _message = std::move(message)]() {
            on_send(std::move(_message));
        });
    }

    void websocket_session::on_send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        queue_.push_back(std::move(message));

        if (queue_.size() > 1) {
            return;
        }

        ws_.async_write(boost::asio::buffer(*queue_.front()), boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
    }

    void websocket_session::on_write(boost::system::error_code error_code, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (error_code) {
            state_->remove_session(get_id());
            return;
        }

        queue_.erase(queue_.begin());

        if (!queue_.empty()) {
            ws_.async_write(boost::asio::buffer(*queue_.front()), boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
        }
    }

    void websocket_session::read_body() {
        ws_.async_read(read_buffer_, boost::beast::bind_front_handler(&websocket_session::on_read, shared_from_this()));
    }

    void websocket_session::on_read(boost::system::error_code error_code, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (!error_code) {
            auto _self = shared_from_this();

            auto _payload = std::make_shared<std::vector<std::uint8_t>>();

            _payload->resize(read_buffer_.size());

            boost::asio::buffer_copy(boost::asio::buffer(*_payload), read_buffer_.data());

            read_buffer_.consume(read_buffer_.size());

            if (const auto _response = kernel_->handle(_payload, _self); _response) {
                send(_response);
            }

            read_body();
        } else {
            state_->remove_session(get_id());
        }
    }

    boost::uuids::uuid websocket_session::get_id() const {
        return id_;
    }

    boost::uuids::uuid websocket_session::get_node_id() const {
        return node_id_;
    }

    void websocket_session::set_node_id(boost::uuids::uuid node_id) {
        node_id_ = node_id;
    }

    std::uint16_t websocket_session::get_port() const {
        return port_;
    }

    void websocket_session::set_port(std::uint16_t port) {
        port_ = port;
    }

    std::string websocket_session::get_host() const {
        return host_;
    }

    void websocket_session::set_host(const std::string& host) {
        host_ = host;
    }

    void websocket_session::disconnect() {
        if (ws_.is_open()) {
            boost::system::error_code _ec;
            ws_.close(boost::beast::websocket::close_code::normal, _ec);
        }
    }
}
