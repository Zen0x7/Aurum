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

    // Create a 16-bit integer representing the number of requests
    std::uint16_t _requests_quantity_be = static_cast<std::uint16_t>(_requests_quantity);

    // Convert the requests quantity to big endian format
    boost::endian::native_to_big_inplace(_requests_quantity_be);

    // Create a byte pointer to the big endian requests quantity
    auto* _requests_quantity_ptr = reinterpret_cast<const uint8_t*>(&_requests_quantity_be);

    // Insert the big endian requests quantity into the payload buffer
    _payload.insert(_payload.end(), _requests_quantity_ptr, _requests_quantity_ptr + sizeof(_requests_quantity_be));

    // Iterate through the number of requests
    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        // Set the size of the request payload (1 byte opcode + 16 bytes transaction ID)
        std::uint16_t _request_length = 17;

        // Convert the request length to big endian format
        boost::endian::native_to_big_inplace(_request_length);

        // Create a byte pointer to the big endian request length
        auto* _request_length_ptr = reinterpret_cast<const uint8_t*>(&_request_length);

        // Insert the request length into the payload buffer
        _payload.insert(_payload.end(), _request_length_ptr, _request_length_ptr + sizeof(_request_length));
    }

    // Iterate through the number of requests
    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        // Append the operational code corresponding to ping (1)
        _payload.push_back(1);

        // Generate a random 16 byte unique identifier for the transaction
        boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

        // Insert the generated transaction ID into the payload buffer
        _payload.insert(_payload.end(), _transaction_id.begin(), _transaction_id.end());
    }

    // Create a CRC16-CCITT object to calculate the checksum
    boost::crc_ccitt_type _crc;

    // Calculate the CRC16-CCITT checksum for the constructed payload buffer
    _crc.process_bytes(_payload.data(), _payload.size());

    // Retrieve the calculated checksum as an unsigned 16-bit integer
    std::uint16_t _crc_value = _crc.checksum();

    // Convert the calculated checksum to big endian format
    boost::endian::native_to_big_inplace(_crc_value);

    // Create a byte pointer to the big endian checksum
    auto* _crc_ptr = reinterpret_cast<const uint8_t*>(&_crc_value);

    // Insert the checksum into the payload buffer
    _payload.insert(_payload.end(), _crc_ptr, _crc_ptr + sizeof(_crc_value));

    // Allocate the full buffer
    std::vector<uint8_t> _buffer;

    // Calculate the total size of the frame's payload
    uint32_t _header_length = static_cast<uint32_t>(_payload.size());

    // Convert the calculated payload length into big endian format
    boost::endian::native_to_big_inplace(_header_length);

    // Create a byte pointer to the big endian payload length
    auto* _header_ptr = reinterpret_cast<const uint8_t*>(&_header_length);

    // Insert the header length into the frame buffer
    _buffer.insert(_buffer.end(), _header_ptr, _header_ptr + sizeof(_header_length));

    // Insert the payload containing the body of the requests into the frame buffer
    _buffer.insert(_buffer.end(), _payload.begin(), _payload.end());

    // Define tracker for accumulated outgoing written bytes
    size_t _total_bytes_written = 0;

    // Define tracker for accumulated incoming read bytes
    size_t _total_bytes_read = 0;

    for (auto _ : state) {
        // Write the frame buffer completely towards the tcp socket synchronously
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _ec);

        // Exit early if a connection issue prevented the frame buffer from writing
        if (_ec) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        // Add the buffer size to the written tracker accumulator
        _total_bytes_written += _buffer.size();

        // Initialize local 32-bit unsigned integer to capture the response header
        uint32_t _response_header_length = 0;

        // Read the exact 4 bytes synchronous representing the response header size
        boost::asio::read(_socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)), _ec);

        // Exit early if a connection issue prevented the header read from finishing
        if (_ec) {
            state.SkipWithError("Failed to read response header.");
            break;
        }

        // Pause benchmark timer avoiding penalizing throughput with parsing/allocation logic
        state.PauseTiming();

        // Parse and restore the response header to native endianness format
        boost::endian::big_to_native_inplace(_response_header_length);

        // Prepare an adequate size binary buffer vector for accommodating the response body
        std::vector<uint8_t> _response_body(_response_header_length);

        // Resume benchmark timer focusing primarily over server side execution cycles processing capability
        state.ResumeTiming();

        // Proceed synchronously with a complete socket read capturing the total body
        boost::asio::read(_socket, boost::asio::buffer(_response_body), _ec);

        // Break out of loop dynamically if a socket error is met during read logic
        if (_ec) {
            state.SkipWithError("Failed to read response body.");
            break;
        }

        // Pause benchmark timer avoiding penalizing throughput with bookkeeping logic
        state.PauseTiming();

        // Measure read IO bounds augmenting total received elements
        _total_bytes_read += sizeof(_response_header_length) + _response_body.size();

        // Flag vectors to be skipped out of aggressive dead code optimization routines
        benchmark::DoNotOptimize(_buffer);
        benchmark::DoNotOptimize(_response_body);

        // Resume benchmark timer targeting the next read/write cycles boundary
        state.ResumeTiming();
    }

    // Mark absolute byte throughput correctly by combining written and read data stream operations
    state.SetBytesProcessed(_total_bytes_written + _total_bytes_read);

    // Mark individual requests throughput parsed internally through server processing
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
