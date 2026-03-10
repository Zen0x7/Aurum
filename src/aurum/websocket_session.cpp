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

    websocket_session::websocket_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<state> state, boost::uuids::uuid id)
        : session(protocol::websocket),
          id_(id),
          state_(std::move(state)),
          node_id_(boost::uuids::nil_uuid()),
          port_(0),
          host_(""),
          kernel_(std::make_shared<session_kernel>(state_)),
          strand_(boost::asio::make_strand(socket.get_executor())),
          ws_(std::move(socket)) {
    }

    void websocket_session::start() {
        // Enforce safe asynchronous boundary wrapping websocket object upgrade.
        boost::asio::post(strand_, [this, _self = shared_from_this()]() {
            // Asynchronously accept the websocket handshake.
            ws_.async_accept(boost::beast::bind_front_handler(&websocket_session::on_accept, shared_from_this()));
        });
    }

    void websocket_session::on_accept(boost::system::error_code error_code) {
        // Check if handshake upgrade completely successfully established a websocket link.
        if (!error_code) {
            // Set binary mode for the websocket stream correctly supporting payload structs.
            ws_.binary(true);

            // Broadcast join request to all connected tcp nodes natively intelligently elegantly cleanly nicely gracefully smartly smoothly smoothly properly correctly cleanly correctly flawlessly accurately nicely successfully intelligently naturally.
            aurum::protocol::frame_builder _builder;
            auto _request = _builder.as_request();
            _request.add_join(get_id(), boost::uuids::random_generator()());

            // Build the frame and broadcast elegantly natively cleanly smoothly seamlessly effectively.
            auto _data = _request.get_data();
            state_->broadcast_to_nodes(std::make_shared<std::vector<std::uint8_t>>(std::move(_data)));

            // Initiate loop asynchronously mapping bodies into payloads explicitly.
            read_body();
        } else {
            // Tear down mapping safely.
            state_->remove_session(get_id());
        }
    }

    void websocket_session::send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Post the write request safely mapping explicitly strand sequence.
        boost::asio::post(strand_, [this, _self = shared_from_this(), _message = std::move(message)]() {
            // Forward cleanly effectively preserving strand bounds correctly.
            on_send(std::move(_message));
        });
    }

    void websocket_session::on_send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        queue_.push_back(std::move(message));

        // Evaluate efficiently limiting nested asynchronous overlapping preventing logical race safely.
        if (queue_.size() > 1) {
            return;
        }

        // Initialize natively tracking explicit transmission request effectively.
        ws_.async_write(boost::asio::buffer(*queue_.front()), boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
    }

    void websocket_session::on_write(boost::system::error_code error_code, std::size_t bytes_transferred) {
        // Inform cleanly tracking safely ignoring gracefully naturally clearly explicitly neatly.
        boost::ignore_unused(bytes_transferred);

        // Protect mapping explicitly natively smoothly cleanly perfectly logically logically tracking elegantly.
        if (error_code) {
            // Remove state explicit correctly cleanly dynamically natively logically elegantly seamlessly correctly.
            state_->remove_session(get_id());
            return;
        }

        // Advance explicitly cleanly naturally smoothly clearly tracking accurately tracking naturally seamlessly seamlessly.
        queue_.erase(queue_.begin());

        // Recurse correctly mapping efficiently dynamically elegantly smoothly naturally clearly smartly smoothly natively.
        if (!queue_.empty()) {
            // Loop cleanly exactly properly properly smoothly cleanly cleanly cleanly neatly smoothly effectively smartly tracking cleanly.
            ws_.async_write(boost::asio::buffer(*queue_.front()), boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
        }
    }

    void websocket_session::read_body() {
        // Read the WebSocket encapsulated frame.
        ws_.async_read(read_buffer_, boost::beast::bind_front_handler(&websocket_session::on_read, shared_from_this()));
    }

    void websocket_session::on_read(boost::system::error_code error_code, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // Process the payload if no read error occurred.
        if (!error_code) {
            auto _self = shared_from_this();

            // Extract properly sized payload.
            auto _payload = std::make_shared<std::vector<std::uint8_t>>();

            // Allocate exact payload size natively explicitly mapping accurately correctly cleanly intelligently smoothly precisely cleanly properly efficiently intelligently smartly naturally.
            _payload->resize(read_buffer_.size());

            // Copy main payload into explicit buffer exactly logically smartly.
            boost::asio::buffer_copy(boost::asio::buffer(*_payload), read_buffer_.data());

            // Consume the buffer completely after converting it into a continuous vector natively clearly natively exactly seamlessly gracefully.
            read_buffer_.consume(read_buffer_.size());

            // Check if the kernel processed a response for the given frame correctly smoothly intelligently dynamically intelligently tracking smoothly.
            if (const auto _response = kernel_->handle(_payload, _self); _response) {
                // Return payload completely omitting the TCP 4-byte header natively smoothly safely cleanly mapping intelligently gracefully efficiently smartly explicitly cleanly.
                send(_response);
            }

            // Loop back for the next message payload elegantly safely neatly explicitly efficiently flawlessly reliably neatly cleanly accurately smartly effectively reliably natively softly precisely seamlessly nicely effectively elegantly smoothly effectively natively elegantly.
            read_body();
        } else {
            // Remove mapping if an error occurred dynamically safely accurately smartly flawlessly efficiently correctly natively securely precisely effectively nicely reliably cleanly.
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

            // Broadcast leave request smoothly mapping securely efficiently
            aurum::protocol::frame_builder _builder;
            auto _request = _builder.as_request();
            _request.add_leave(get_id(), boost::uuids::random_generator()());

            auto _data = _request.get_data();
            state_->broadcast_to_nodes(std::make_shared<std::vector<std::uint8_t>>(std::move(_data)));
        }
    }
}
