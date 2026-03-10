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

using namespace aurum;

// Benchmark measuring TCP connection establishment overhead to the dummy server.
static void BM_TCP_Connect(benchmark::State& state) {
    setup_server(1);

    const unsigned short _port = g_server->get_port();
    boost::asio::io_context _client_io_context;
    const boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);

    for (auto _ : state) {
        boost::asio::ip::tcp::socket _socket(_client_io_context);

        boost::system::error_code _error_code;
        _socket.connect(_endpoint, _error_code);

        if (_error_code) {
            state.SkipWithError("Failed to connect to the server.");
            break;
        }

        _socket.close();
        benchmark::DoNotOptimize(_socket);
    }
}
BENCHMARK(BM_TCP_Connect);
