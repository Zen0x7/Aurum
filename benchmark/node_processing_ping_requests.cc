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
#include <boost/asio/read.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/crc.hpp>
#include <aurum/protocol/op_code.hpp>

#include <vector>

using namespace aurum;

// Benchmark measuring TCP ping throughput.
static void BM_TCP_Ping_Throughput(benchmark::State& state) {
    setup_server(1);

    const size_t _requests_quantity = state.range(0);
    const unsigned short _port = g_server->get_port();

    boost::asio::io_context _client_io_context;
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    boost::system::error_code _ec;
    _socket.connect(_endpoint, _ec);

    if (_ec) {
        state.SkipWithError("Failed to connect to the server.");
        return;
    }

    protocol::frame_builder _frame_builder;
    auto _request_builder = _frame_builder.as_request();
    _request_builder.reserve(_requests_quantity);

    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        _request_builder.add_ping();
    }

    std::vector<uint8_t> _buffer = _request_builder.get_buffers();

    size_t _total_bytes_written = 0;

    size_t _total_bytes_read = 0;

    for (auto _ : state) {
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _ec);

        if (_ec) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        _total_bytes_written += _buffer.size();

        uint32_t _response_header_length = 0;

        boost::asio::read(_socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)), _ec);

        if (_ec) {
            state.SkipWithError("Failed to read response header.");
            break;
        }

        state.PauseTiming();

        boost::endian::little_to_native_inplace(_response_header_length);

        std::vector<uint8_t> _response_body(_response_header_length);

        state.ResumeTiming();

        boost::asio::read(_socket, boost::asio::buffer(_response_body), _ec);

        if (_ec) {
            state.SkipWithError("Failed to read response body.");
            break;
        }

        state.PauseTiming();

        _total_bytes_read += sizeof(_response_header_length) + _response_body.size();

        benchmark::DoNotOptimize(_buffer);
        benchmark::DoNotOptimize(_response_body);

        state.ResumeTiming();
    }

    state.SetBytesProcessed(_total_bytes_written + _total_bytes_read);

    state.SetItemsProcessed(state.iterations() * _requests_quantity);
}
BENCHMARK(BM_TCP_Ping_Throughput)->Arg(1)->Arg(64)->Arg(1024)->Arg(10240);

// Benchmark measuring TCP ping throughput in multithreaded environment.
static void BM_TCP_Ping_Throughput_MT(benchmark::State& state) {
    setup_server(state.range(1));

    const size_t _requests_quantity = state.range(0);
    const unsigned short _port = g_server->get_port();

    boost::asio::io_context _client_io_context;
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    boost::system::error_code _error_code;
    _socket.connect(_endpoint, _error_code);

    if (_error_code) {
        state.SkipWithError("Failed to connect to the server.");
        return;
    }

    constexpr protocol::frame_builder _frame_builder;
    auto _request_builder = _frame_builder.as_request();
    _request_builder.reserve(_requests_quantity);

    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        _request_builder.add_ping();
    }

    std::vector<uint8_t> _buffer = _request_builder.get_buffers();

    size_t _total_bytes_written = 0;

    size_t _total_bytes_read = 0;

    for (auto _ : state) {
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _error_code);

        if (_error_code) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        _total_bytes_written += _buffer.size();

        uint32_t _response_header_length = 0;

        boost::asio::read(_socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)), _error_code);

        if (_error_code) {
            state.SkipWithError("Failed to read response header.");
            break;
        }

        state.PauseTiming();

        boost::endian::little_to_native_inplace(_response_header_length);

        std::vector<uint8_t> _response_body(_response_header_length);

        state.ResumeTiming();

        boost::asio::read(_socket, boost::asio::buffer(_response_body), _error_code);

        if (_error_code) {
            state.SkipWithError("Failed to read response body.");
            break;
        }

        state.PauseTiming();

        _total_bytes_read += sizeof(_response_header_length) + _response_body.size();

        benchmark::DoNotOptimize(_buffer);
        benchmark::DoNotOptimize(_response_body);

        state.ResumeTiming();
    }

    state.SetBytesProcessed(_total_bytes_written + _total_bytes_read);

    state.SetItemsProcessed(state.iterations() * _requests_quantity);
}

// Arguments are (payload_size, thread_count)
BENCHMARK(BM_TCP_Ping_Throughput_MT)
    ->Args({1, 2})->Args({64, 2})->Args({1024, 2})->Args({10240, 2})
    ->Args({1, 4})->Args({64, 4})->Args({1024, 4})->Args({10240, 4})
    ->Args({1, 8})->Args({64, 8})->Args({1024, 8})->Args({10240, 8})
    ->ThreadRange(1, 8); // Setup client threads range
