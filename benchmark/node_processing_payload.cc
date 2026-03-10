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

#include <benchmark/benchmark.h>
#include "server_fixture.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>

#include <vector>

using namespace aurum;

// Benchmark measuring write throughput (bitrate) by sending raw payloads
// and exploiting the simplistic processing flow.
static void BM_TCP_Write_Throughput(benchmark::State& state) {
    setup_server(1);

    const size_t _payload_size = state.range(0);
    const unsigned short _port = g_server->get_port();

    boost::asio::io_context _client_io_context;
    const boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    // Create a connection specifically for this benchmark sequence to avoid connection timing overhead in the loop.
    boost::system::error_code _ec;
    _socket.connect(_endpoint, _ec);

    if (_ec) {
        state.SkipWithError("Failed to connect to the server.");
        return;
    }

    for (auto _ : state) {
        // Pause benchmark timer avoiding penalizing throughput with allocation logic
        state.PauseTiming();

        // Allocate the header plus payload buffer entirely.
        std::vector<uint8_t> _buffer(sizeof(uint32_t) + _payload_size);

        // Set header length directly to vector (Little Endian 4-bytes).
        uint32_t _header_length = static_cast<uint32_t>(_payload_size);
        boost::endian::native_to_little_inplace(_header_length);
        std::memcpy(_buffer.data(), &_header_length, sizeof(uint32_t));

        // Fill body with dummy repeating data.
        if (_payload_size > 0) {
            std::memset(_buffer.data() + sizeof(uint32_t), 'A', _payload_size);
        }

        // Resume benchmark timer focusing primarily over server side execution cycles processing capability
        state.ResumeTiming();

        // Send buffer (header + payload body).
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _ec);
        if (_ec) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        benchmark::DoNotOptimize(_buffer);
    }

    // Measure raw total bytes correctly.
    state.SetBytesProcessed(state.iterations() * _payload_size);
}
BENCHMARK(BM_TCP_Write_Throughput)->Range(8, 8192);

// Benchmark measuring TCP write throughput in multithreaded environment.
static void BM_TCP_Write_Throughput_MT(benchmark::State& state) {
    // Setup server with thread count equal to the second range parameter
    setup_server(state.range(1));

    const size_t _payload_size = state.range(0);
    const unsigned short _port = g_server->get_port();

    boost::asio::io_context _client_io_context;
    const boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    // Create a connection specifically for this benchmark sequence to avoid connection timing overhead in the loop.
    boost::system::error_code _error_code;
    _socket.connect(_endpoint, _error_code);

    if (_error_code) {
        state.SkipWithError("Failed to connect to the server.");
        return;
    }

    for (auto _ : state) {
        // Pause benchmark timer avoiding penalizing throughput with allocation logic
        state.PauseTiming();

        // Allocate the header plus payload buffer entirely.
        std::vector<uint8_t> _buffer(sizeof(uint32_t) + _payload_size);

        // Set header length directly to vector (Little Endian 4-bytes).
        uint32_t _header_length = static_cast<uint32_t>(_payload_size);
        boost::endian::native_to_little_inplace(_header_length);
        std::memcpy(_buffer.data(), &_header_length, sizeof(uint32_t));

        // Fill body with dummy repeating data.
        if (_payload_size > 0) {
            std::memset(_buffer.data() + sizeof(uint32_t), 'A', _payload_size);
        }

        // Resume benchmark timer focusing primarily over server side execution cycles processing capability
        state.ResumeTiming();

        // Send buffer (header + payload body).
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _error_code);
        if (_error_code) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        benchmark::DoNotOptimize(_buffer);
    }

    // Measure raw total bytes correctly.
    state.SetBytesProcessed(state.iterations() * _payload_size);
}

// Arguments are (payload_size, thread_count)
BENCHMARK(BM_TCP_Write_Throughput_MT)
    ->Args({8, 2})->Args({8192, 2})
    ->Args({8, 4})->Args({8192, 4})
    ->Args({8, 8})->Args({8192, 8})
    ->ThreadRange(1, 8); // Setup client threads range
