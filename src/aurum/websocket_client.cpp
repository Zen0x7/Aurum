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

#include <aurum/websocket_client.hpp>
#include <boost/asio/connect.hpp>
#include <stdexcept>
#include <iostream>

namespace aurum {
    websocket_client::websocket_client() : ws_(io_context_) {
    }

    bool websocket_client::connect(const std::string& host, unsigned short port) {
        boost::asio::ip::tcp::resolver _resolver(io_context_);
        boost::system::error_code _resolve_ec;
        auto _endpoints = _resolver.resolve(host, std::to_string(port), _resolve_ec);

        if (_resolve_ec) {
            std::cerr << "WebSocket resolve failed: " << _resolve_ec.message() << std::endl;
            return false;
        }

        boost::system::error_code _connect_ec;
        boost::asio::connect(ws_.next_layer(), _endpoints, _connect_ec);

        if (_connect_ec) {
            std::cerr << "WebSocket connect failed: " << _connect_ec.message() << std::endl;
            return false;
        }

        ws_.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type& req) {
                req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING " aurum-websocket-client");
            }));

        try {
            ws_.handshake(host + ":" + std::to_string(port), "/");
        } catch (const boost::system::system_error& se) {
            std::cerr << "WebSocket Handshake failed via exception: " << se.what() << std::endl;
            return false;
        }

        ws_.binary(true);

        return true;
    }

    void websocket_client::send(const std::vector<std::uint8_t>& message) {
        boost::system::error_code _ec;

        // Send directly the buffer.
        ws_.write(boost::asio::buffer(message), _ec);
        if (_ec) {
            throw std::runtime_error("Failed to send WebSocket payload: " + _ec.message());
        }
    }

    std::vector<std::uint8_t> websocket_client::read() {
        boost::beast::flat_buffer _buffer;
        boost::system::error_code _ec;

        ws_.read(_buffer, _ec);

        if (_ec) {
            throw std::runtime_error("Failed to read WebSocket payload: " + _ec.message());
        }

        // Extract native bytes correctly mapped smoothly perfectly smoothly flawlessly neatly clearly directly cleanly cleanly.
        std::vector<std::uint8_t> _payload;
        _payload.resize(_buffer.size());
        boost::asio::buffer_copy(boost::asio::buffer(_payload), _buffer.data());

        return _payload;
    }

    protocol::frame_builder& websocket_client::get_builder() {
        return builder_;
    }

    void websocket_client::disconnect() {
        if (ws_.is_open()) {
            boost::system::error_code _ec;
            ws_.close(boost::beast::websocket::close_code::normal, _ec);
        }
    }
}
