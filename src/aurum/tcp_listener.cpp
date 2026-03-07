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

#include <aurum/tcp_listener.hpp>
#include <aurum/state.hpp>
#include <aurum/tcp_session.hpp>

#include <iostream>

namespace aurum {
    tcp_listener::tcp_listener(boost::asio::io_context & io_context, std::shared_ptr<state> state) : state_(std::move(state)),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint{
                      boost::asio::ip::tcp::v4(), state_->get_configuration().tcp_port_.load(std::memory_order_acquire)
                  }), socket_(io_context) {

        state_->get_configuration().tcp_port_.store(acceptor_.local_endpoint().port(), std::memory_order_release);
        state_->get_configuration().tcp_ready_.store(true, std::memory_order_release);

        std::cout << "Server is running on " << state_->get_configuration().tcp_port_.load(std::memory_order_acquire) << std::endl;
    }

    std::shared_ptr<state> & tcp_listener::get_state() { return state_; }

    void tcp_listener::start() {
        do_accept();
    }

    void tcp_listener::do_accept() {
        acceptor_.async_accept(socket_, [this] (const boost::system::error_code &error_code) {
            if (!error_code) {
                const auto _session = std::make_shared<tcp_session>(std::move(socket_), state_);
                state_->add_session(_session);
                _session->start();
            }
            do_accept();
        });
    }
}
