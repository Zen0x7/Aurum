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

/**
 * @brief Integration test verifying end-to-end ping connection lifecycle and exact binary protocol framing correctly.
 */
TEST_F(tcp_server_fixture, ConnectSendPayloadAndDisconnect) {
    // Spin up an isolated client IO context for the connection.
    boost::asio::io_context client_io;

    // Create a local TCP client socket.
    boost::asio::ip::tcp::socket socket(client_io);

    // Retrieve the dynamically allocated listener port from the server state.
    unsigned short port =
            state->get_configuration().tcp_port_.load(std::memory_order_acquire);

    // Connect the client synchronously to the server address loopback.
    socket.connect({
        // Resolve the loopback IP explicitly.
        boost::asio::ip::address_v4::loopback(),
        // Pass the targeted ephemeral port.
        port
    });

    // Wait until the server actively captures and tracks the connection session.
    wait_until([this] {
            // Check the active server sessions registry array mapping appropriately.
            return state->get_sessions().size() == 1;
        });

    // Ensure that exactly one session was created by the server socket acceptor.
    ASSERT_EQ(state->get_sessions().size(), 1);

    // Declare the payload buffer array.
    std::vector<uint8_t> _payload;

    // Set the amount of requests inside the frame, in this case 1.
    std::uint16_t _requests_quantity = 1;

    // Convert the quantity value to big endian for network transmission.
    boost::endian::native_to_big_inplace(_requests_quantity);

    // Cast a byte pointer pointing to the native variable address to copy it.
    auto* _requests_quantity_ptr = reinterpret_cast<const uint8_t*>(&_requests_quantity);

    // Insert the big endian payload size element into the buffer.
    _payload.insert(_payload.end(), _requests_quantity_ptr, _requests_quantity_ptr + sizeof(_requests_quantity));

    // Define the request length byte size limit constraint (1 byte opcode + 16 byte uuid).
    std::uint16_t _request_length = 17;

    // Enforce big endian representation to format the length chunk.
    boost::endian::native_to_big_inplace(_request_length);

    // Convert integer into bytes pointer.
    auto* _request_length_ptr = reinterpret_cast<const uint8_t*>(&_request_length);

    // Append the request size component into the body stream.
    _payload.insert(_payload.end(), _request_length_ptr, _request_length_ptr + sizeof(_request_length));

    // Push explicitly the ping operational code (1) at the first byte.
    _payload.push_back(1);

    // Create a new transactional uniform identifier string.
    boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

    // Include the generated identifier segment into the payload buffer.
    _payload.insert(_payload.end(), _transaction_id.begin(), _transaction_id.end());

    // Allocate CCITT type CRC16 engine checker to protect integrity.
    boost::crc_ccitt_type _crc;

    // Update inner bytes context to generate matching CRC sequence.
    _crc.process_bytes(_payload.data(), _payload.size());

    // Pull resulting computed value representing checksum target.
    std::uint16_t _crc_value = _crc.checksum();

    // Overwrite the actual local variable format into big endian structure.
    boost::endian::native_to_big_inplace(_crc_value);

    // Build memory mapped alias over crc value.
    auto* _crc_ptr = reinterpret_cast<const uint8_t*>(&_crc_value);

    // Safely dump tail bytes acting as integrity token wrapper.
    _payload.insert(_payload.end(), _crc_ptr, _crc_ptr + sizeof(_crc_value));

    // Define length limit for the full internal body wrapper payload.
    uint32_t _header_length = _payload.size();

    // Endianness swap towards big to properly encode frame limit bound.
    boost::endian::native_to_big_inplace(_header_length);

    // Write outgoing size limit through TCP endpoint layer.
    boost::asio::write(socket, boost::asio::buffer(&_header_length, sizeof(_header_length)));

    // Dump actual serialized payload through TCP sequence pipeline.
    boost::asio::write(socket, boost::asio::buffer(_payload));

    // Declare placeholder buffer integer for received server bound wrapper length.
    uint32_t _response_header_length = 0;

    // Trigger socket bound sync read routine parsing frame limits explicitly.
    boost::asio::read(socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)));

    // Reformat input stream bounds representation to native endianness for parsing.
    boost::endian::big_to_native_inplace(_response_header_length);

    // Prepare receiving target dynamic array based off received length boundary.
    std::vector<uint8_t> _response_body(_response_header_length);

    // Flush rest of the network stream frame sequence down towards local buffer space.
    boost::asio::read(socket, boost::asio::buffer(_response_body));

    // Assure frame constraints limits hold valid minimum bounds limits rules.
    ASSERT_GE(_response_body.size(), 4); // at least qty + crc

    // Declare quantity received placeholder tracker.
    std::uint16_t _response_quantity;

    // Deep copy header structure bytes back mapping primitive properties.
    std::memcpy(&_response_quantity, _response_body.data(), sizeof(_response_quantity));

    // Adapt back input array sequence mapping towards correct host primitive.
    boost::endian::big_to_native_inplace(_response_quantity);

    // Compare structural output enforcing ping single return size bounds.
    ASSERT_EQ(_response_quantity, 1);

    // Declare length parameter to capture response specific element length limit.
    std::uint16_t _response_length;

    // Transfer mapping copying memory segment representation directly.
    std::memcpy(&_response_length, _response_body.data() + 2, sizeof(_response_length));

    // Correct memory formatting for native little endian processing unit architectures.
    boost::endian::big_to_native_inplace(_response_length);

    // Ping length bounds expected explicitly sized to ID (16) and Exit (1) = 17.
    ASSERT_EQ(_response_length, 17);

    // Create destination parsing uuid object structure representing transaction bound matching.
    boost::uuids::uuid _response_transaction_id;

    // Restore raw memory copying bytes sequentially mapping representation boundaries.
    std::memcpy(_response_transaction_id.data, _response_body.data() + 4, 16);

    // Cross-validate that returned ID exactly equals source request tracking token element.
    ASSERT_EQ(_transaction_id, _response_transaction_id);

    // Assure that expected successful command operations properly log status mapping bound variable flag.
    ASSERT_EQ(_response_body[20], 200);

    // Cleanup close network connection triggering server side disconnection hooks lifecycle handlers correctly.
    socket.close();

    // Block test execution asynchronously yielding until the server session goes to zero.
    wait_until([this] {
        // Evaluate the server sessions map length safely checking container destruction.
        return state->get_sessions().size() == 0;
    });

    // Assert that the container correctly dropped the closed socket.
    ASSERT_EQ(state->get_sessions().size(), 0);
}
