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

#ifndef AURUM_TCP_LISTENER_HPP
#define AURUM_TCP_LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>

namespace aurum {
    class state;

    class tcp_listener {
        std::shared_ptr<state> state_;
    public:
        tcp_listener(boost::asio::io_context &io_context, std::shared_ptr<state> state);

        std::shared_ptr<state> & get_state();

        void start();
    private:
        void do_accept();

        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::ip::tcp::socket socket_;
    };
}

#endif // AURUM_TCP_LISTENER_HPP
