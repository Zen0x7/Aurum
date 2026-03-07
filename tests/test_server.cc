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
#include <boost/uuid/random_generator.hpp>
#include <boost/crc.hpp>

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

    // Build Ping Frame
    std::vector<uint8_t> payload;

    // requests_quantity (1)
    std::uint16_t requests_qty = 1;
    boost::endian::native_to_big_inplace(requests_qty);
    auto* qty_ptr = reinterpret_cast<const uint8_t*>(&requests_qty);
    payload.insert(payload.end(), qty_ptr, qty_ptr + sizeof(requests_qty));

    // request_length (1 + 16 = 17 bytes)
    std::uint16_t req_len = 17;
    boost::endian::native_to_big_inplace(req_len);
    auto* len_ptr = reinterpret_cast<const uint8_t*>(&req_len);
    payload.insert(payload.end(), len_ptr, len_ptr + sizeof(req_len));

    // opcode (1)
    payload.push_back(1);

    // transaction_id (16 bytes)
    boost::uuids::uuid tx_id = boost::uuids::random_generator()();
    payload.insert(payload.end(), tx_id.begin(), tx_id.end());

    // crc16
    boost::crc_ccitt_type crc;
    crc.process_bytes(payload.data(), payload.size());
    std::uint16_t crc_val = crc.checksum();
    boost::endian::native_to_big_inplace(crc_val);
    auto* crc_ptr = reinterpret_cast<const uint8_t*>(&crc_val);
    payload.insert(payload.end(), crc_ptr, crc_ptr + sizeof(crc_val));

    // Send header + payload
    uint32_t header = payload.size();
    boost::endian::native_to_big_inplace(header);

    boost::asio::write(socket, boost::asio::buffer(&header, sizeof(header)));
    boost::asio::write(socket, boost::asio::buffer(payload));

    // Read response header
    uint32_t resp_header = 0;
    boost::asio::read(socket, boost::asio::buffer(&resp_header, sizeof(resp_header)));
    boost::endian::big_to_native_inplace(resp_header);

    // Read response body
    std::vector<uint8_t> resp_body(resp_header);
    boost::asio::read(socket, boost::asio::buffer(resp_body));

    // Verify response
    ASSERT_GE(resp_body.size(), 4); // at least qty + crc

    std::uint16_t resp_qty;
    std::memcpy(&resp_qty, resp_body.data(), sizeof(resp_qty));
    boost::endian::big_to_native_inplace(resp_qty);
    ASSERT_EQ(resp_qty, 1);

    std::uint16_t resp_len;
    std::memcpy(&resp_len, resp_body.data() + 2, sizeof(resp_len));
    boost::endian::big_to_native_inplace(resp_len);
    ASSERT_EQ(resp_len, 17);

    // Check transaction id
    boost::uuids::uuid resp_tx_id;
    std::memcpy(resp_tx_id.data, resp_body.data() + 4, 16);
    ASSERT_EQ(tx_id, resp_tx_id);

    // Check exit code
    ASSERT_EQ(resp_body[20], 200);

    socket.close();

    wait_until([this] {
        return state->get_sessions().size() == 0;
    });

    ASSERT_EQ(state->get_sessions().size(), 0);
}
