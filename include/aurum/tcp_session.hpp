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
    class state;

    class tcp_kernel;

    class tcp_session : public std::enable_shared_from_this<tcp_session> {
        boost::uuids::uuid id_;
        std::shared_ptr<state> state_;
        std::shared_ptr<tcp_kernel> kernel_;
    public:
        explicit tcp_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<state> state);
        void start();

        void send(std::shared_ptr<const std::vector<std::uint8_t>> message);

        boost::uuids::uuid get_id() const;
    private:
        void read_header();
        void read_body();
        void on_send(std::shared_ptr<const std::vector<std::uint8_t>> message);
        void on_write(boost::system::error_code error_code, std::size_t bytes_transferred);

        boost::asio::ip::tcp::socket socket_;
        std::uint32_t header_length_ { 0 };
        std::vector<std::shared_ptr<const std::vector<std::uint8_t>>> queue_;
    };
}

#endif // AURUM_TCP_SESSION_HPP
