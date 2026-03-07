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
#include <boost/asio/read.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/crc.hpp>

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

// Benchmark measuring TCP ping throughput.
static void BM_TCP_Ping_Throughput(benchmark::State& state) {
    if (!g_server) {
        g_server = std::make_unique<server_fixture>();
    }

    size_t _requests_quantity = state.range(0);
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

    // Allocate the header plus payload buffer.
    std::vector<uint8_t> _payload;

    // requests_quantity (2 bytes)
    std::uint16_t req_qty_be = static_cast<std::uint16_t>(_requests_quantity);
    boost::endian::native_to_big_inplace(req_qty_be);
    auto* qty_ptr = reinterpret_cast<const uint8_t*>(&req_qty_be);
    _payload.insert(_payload.end(), qty_ptr, qty_ptr + sizeof(req_qty_be));

    // each request length (1 + 16 = 17 bytes)
    for (size_t i = 0; i < _requests_quantity; ++i) {
        std::uint16_t req_len = 17;
        boost::endian::native_to_big_inplace(req_len);
        auto* len_ptr = reinterpret_cast<const uint8_t*>(&req_len);
        _payload.insert(_payload.end(), len_ptr, len_ptr + sizeof(req_len));
    }

    // each request payload
    for (size_t i = 0; i < _requests_quantity; ++i) {
        // opcode (1)
        _payload.push_back(1);

        // transaction_id (16 bytes)
        boost::uuids::uuid tx_id = boost::uuids::random_generator()();
        _payload.insert(_payload.end(), tx_id.begin(), tx_id.end());
    }

    // crc16
    boost::crc_ccitt_type crc;
    crc.process_bytes(_payload.data(), _payload.size());
    std::uint16_t crc_val = crc.checksum();
    boost::endian::native_to_big_inplace(crc_val);
    auto* crc_ptr = reinterpret_cast<const uint8_t*>(&crc_val);
    _payload.insert(_payload.end(), crc_ptr, crc_ptr + sizeof(crc_val));

    // buffer
    std::vector<uint8_t> _buffer;
    uint32_t header = static_cast<uint32_t>(_payload.size());
    boost::endian::native_to_big_inplace(header);
    auto* h_ptr = reinterpret_cast<const uint8_t*>(&header);
    _buffer.insert(_buffer.end(), h_ptr, h_ptr + sizeof(header));
    _buffer.insert(_buffer.end(), _payload.begin(), _payload.end());

    size_t total_bytes_written = 0;
    size_t total_bytes_read = 0;

    for (auto _ : state) {
        // Send buffer (header + payload body).
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _ec);
        if (_ec) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        total_bytes_written += _buffer.size();

        uint32_t resp_header = 0;
        boost::asio::read(_socket, boost::asio::buffer(&resp_header, sizeof(resp_header)), _ec);
        if (_ec) {
            state.SkipWithError("Failed to read response header.");
            break;
        }

        boost::endian::big_to_native_inplace(resp_header);

        std::vector<uint8_t> resp_body(resp_header);
        boost::asio::read(_socket, boost::asio::buffer(resp_body), _ec);
        if (_ec) {
            state.SkipWithError("Failed to read response body.");
            break;
        }

        total_bytes_read += sizeof(resp_header) + resp_body.size();

        benchmark::DoNotOptimize(_buffer);
        benchmark::DoNotOptimize(resp_body);
    }

    state.SetBytesProcessed(total_bytes_written + total_bytes_read);
    state.SetItemsProcessed(state.iterations() * _requests_quantity);
}
BENCHMARK(BM_TCP_Ping_Throughput)->Arg(1)->Arg(64)->Arg(1024)->Arg(10240);

// Run benchmark engine.
int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
