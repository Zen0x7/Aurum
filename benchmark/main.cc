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
#include <aurum/state.hpp>
#include <aurum/tcp_listener.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>

#include <thread>

using namespace aurum;

// Global context required to run the server in the background for benchmarks.
struct server_fixture {
    boost::asio::io_context io_context_;
    std::shared_ptr<state> state_;
    std::shared_ptr<tcp_listener> listener_;
    std::thread thread_;

    server_fixture() : state_(std::make_shared<state>()) {
        // Start on an ephemeral port.
        state_->get_configuration().tcp_port_.store(0, std::memory_order_release);
        state_->get_configuration().threads_.store(1, std::memory_order_release);

        listener_ = std::make_shared<tcp_listener>(io_context_, state_);
        listener_->start();

        // Run the io_context on a background thread.
        thread_ = std::thread([this]() {
            io_context_.run();
        });

        // Wait until the server binds and the port is set.
        while (!state_->get_configuration().tcp_ready_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    ~server_fixture() {
        io_context_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    unsigned short get_port() const {
        return state_->get_configuration().tcp_port_.load(std::memory_order_acquire);
    }
};

// Start a single server instance for all benchmarks.
static std::unique_ptr<server_fixture> g_server;

// Benchmark measuring TCP connection establishment overhead to the dummy server.
static void BM_TCP_Connect(benchmark::State& state) {
    if (!g_server) {
        g_server = std::make_unique<server_fixture>();
    }

    unsigned short _port = g_server->get_port();
    boost::asio::io_context _client_io_context;
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);

    for (auto _ : state) {
        boost::asio::ip::tcp::socket _socket(_client_io_context);

        // Measure time taken to successfully connect.
        boost::system::error_code _ec;
        _socket.connect(_endpoint, _ec);

        if (_ec) {
            state.SkipWithError("Failed to connect to the server.");
            break;
        }

        // Close immediately after connecting to measure only connection time.
        _socket.close();
        benchmark::DoNotOptimize(_socket);
    }
}
BENCHMARK(BM_TCP_Connect);

// Benchmark measuring write throughput (bitrate) by sending raw payloads
// and exploiting the simplistic processing flow.
static void BM_TCP_Write_Throughput(benchmark::State& state) {
    if (!g_server) {
        g_server = std::make_unique<server_fixture>();
    }

    size_t _payload_size = state.range(0);
    unsigned short _port = g_server->get_port();

    boost::asio::io_context _client_io_context;
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    // Create a connection specifically for this benchmark sequence to avoid connection timing overhead in the loop.
    boost::system::error_code _ec;
    _socket.connect(_endpoint, _ec);

    if (_ec) {
        state.SkipWithError("Failed to connect to the server.");
        return;
    }

    // Allocate the header plus payload buffer entirely.
    std::vector<uint8_t> _buffer(sizeof(uint32_t) + _payload_size);

    // Set header length directly to vector (Big Endian 4-bytes).
    uint32_t _header_length = static_cast<uint32_t>(_payload_size);
    boost::endian::native_to_big_inplace(_header_length);
    std::memcpy(_buffer.data(), &_header_length, sizeof(uint32_t));

    // Fill body with dummy repeating data.
    std::memset(_buffer.data() + sizeof(uint32_t), 'A', _payload_size);

    for (auto _ : state) {
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

// Run benchmark engine.
int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
