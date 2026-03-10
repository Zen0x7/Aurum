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

#include <aurum/tcp_client.hpp>
#include <boost/endian/conversion.hpp>
#include <iostream>

namespace aurum {

    /**
     * @brief Default constructor for tcp_client.
     */
    tcp_client::tcp_client()
        : io_context_(),
          socket_(io_context_),
          request_builder_() {
    }

    /**
     * @brief Destructor for tcp_client.
     */
    tcp_client::~tcp_client() {
        disconnect();
    }

    /**
     * @brief Connects to the specified host and port.
     * @param host The remote server address to connect to.
     * @param port The target port number on the remote server.
     */
    void tcp_client::connect(const std::string& host, unsigned short port) {
        boost::asio::ip::tcp::resolver _resolver(io_context_);
        auto _endpoints = _resolver.resolve(host, std::to_string(port));
        boost::asio::connect(socket_, _endpoints);
    }

    /**
     * @brief Retrieves the underlying builder mapping incoming network structures.
     * @return A reference to the active request builder context.
     */
    aurum::protocol::request_builder& tcp_client::get_builder() {
        return request_builder_;
    }

    /**
     * @brief Sends the binary payload across the connected socket interface.
     * @param data The payload byte stream structured mapping memory buffer blocks.
     */
    void tcp_client::send(const std::vector<std::uint8_t>& data) {
        boost::asio::write(socket_, boost::asio::buffer(data));
    }

    /**
     * @brief Reads a single frame returning raw payload sequences.
     * @return A consolidated vector containing total requested read bytes sequence mapping.
     */
    std::vector<std::uint8_t> tcp_client::read() {
        std::vector<std::uint8_t> _output_buffer;

        std::uint32_t _response_header_length = 0;

        boost::asio::read(socket_, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)));

        boost::endian::little_to_native_inplace(_response_header_length);

        std::vector<std::uint8_t> _response_body(_response_header_length);

        boost::asio::read(socket_, boost::asio::buffer(_response_body));

        std::uint32_t _response_header_length_le = _response_header_length;
        boost::endian::native_to_little_inplace(_response_header_length_le);
        auto* _header_ptr = reinterpret_cast<const std::uint8_t*>(&_response_header_length_le);

        _output_buffer.insert(_output_buffer.end(), _header_ptr, _header_ptr + sizeof(_response_header_length_le));
        _output_buffer.insert(_output_buffer.end(), _response_body.begin(), _response_body.end());

        return _output_buffer;
    }

    /**
     * @brief Disconnects the underlying socket closing sequence transmission cleanly.
     */
    void tcp_client::disconnect() {
        if (socket_.is_open()) {
            boost::system::error_code _ec;
            socket_.close(_ec);
        }
    }

} // namespace aurum