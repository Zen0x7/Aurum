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

#include <aurum/websocket_listener.hpp>
#include <aurum/state.hpp>
#include <aurum/websocket_session.hpp>
#include <iostream>

namespace aurum {
    websocket_listener::websocket_listener(boost::asio::io_context & io_context, std::shared_ptr<state> state) : state_(std::move(state)),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint{
                      boost::asio::ip::tcp::v4(), state_->get_configuration().websocket_port_.load(std::memory_order_acquire)
                  }) {

        state_->get_configuration().websocket_port_.store(acceptor_.local_endpoint().port(), std::memory_order_release);

        state_->get_configuration().websocket_ready_.store(true, std::memory_order_release);

        std::cout << "WebSocket server is running on " << state_->get_configuration().websocket_port_.load(std::memory_order_acquire) << std::endl;
    }

    std::shared_ptr<state> & websocket_listener::get_state() {
        return state_;
    }

    void websocket_listener::start() {
        do_accept();
    }

    void websocket_listener::do_accept() {
        acceptor_.async_accept(boost::asio::make_strand(acceptor_.get_executor()), [this] (const boost::system::error_code &error_code, boost::asio::ip::tcp::socket socket) {
            if (!error_code) {
                const auto _session = std::make_shared<websocket_session>(std::move(socket), state_);
                state_->add_session(_session);
                _session->start();
            }
            do_accept();
        });
    }
}
