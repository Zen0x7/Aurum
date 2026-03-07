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

#include "tcp_server_fixture.hpp"

TEST_F(tcp_server_fixture, ConnectSendPayloadAndDisconnect) {
    boost::asio::io_context client_io;
    boost::asio::ip::tcp::socket socket(client_io);

    unsigned short port =
            state->get_configuration().tcp_port_.load(std::memory_order_acquire);

    socket.connect({
        boost::asio::ip::address_v4::loopback(),
        port
    });

    wait_until([this] {
            return state->get_sessions().size() == 1;
        });

    ASSERT_EQ(state->get_sessions().size(), 1);

    std::vector<uint8_t> payload{1, 2, 3, 4};

    uint32_t header = payload.size();
    boost::endian::native_to_big_inplace(header);

    boost::asio::write(socket, boost::asio::buffer(&header, sizeof(header)));
    boost::asio::write(socket, boost::asio::buffer(payload));

    socket.close();

    wait_until([this] {
        return state->get_sessions().size() == 0;
    });

    ASSERT_EQ(state->get_sessions().size(), 0);
}
