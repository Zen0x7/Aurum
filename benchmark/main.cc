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
#include <aurum/node.hpp>
#include <aurum/state.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/crc.hpp>

#include <thread>
#include <mutex>
#include <memory>
#include <aurum/protocol/op_code.hpp>

using namespace aurum;

// Global context required to run the server in the background for benchmarks.
struct server_fixture {
    std::unique_ptr<node> node_;
    std::shared_ptr<state> state_;
    std::thread runner_thread_;
    int thread_count_;

    server_fixture(const int thread_count = 1) : node_(std::make_unique<node>()), thread_count_(thread_count) {
        state_ = node_->get_state();

        // Start on an ephemeral port.
        state_->get_configuration().tcp_port_.store(0, std::memory_order_release);
        state_->get_configuration().threads_.store(thread_count, std::memory_order_release);

        runner_thread_ = std::thread([this]() {
            node_->run();
        });

        // Wait until the server binds and the port is set.
        while (!state_->get_configuration().tcp_ready_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    ~server_fixture() {
        node_->stop();
        if (runner_thread_.joinable()) {
            runner_thread_.join();
        }
    }

    unsigned short get_port() const {
        return state_->get_configuration().tcp_port_.load(std::memory_order_acquire);
    }
};

// Start a single server instance for all benchmarks.
static std::unique_ptr<server_fixture> g_server;
static int g_server_threads = 0;
static std::mutex g_server_mutex;

static void setup_server(int threads) {
    std::lock_guard _lock(g_server_mutex);
    if (!g_server || g_server_threads != threads) {
        g_server = std::make_unique<server_fixture>(threads);
        g_server_threads = threads;
    }
}

// Benchmark measuring TCP connection establishment overhead to the dummy server.
static void BM_TCP_Connect(benchmark::State& state) {
    setup_server(1);

    const unsigned short _port = g_server->get_port();
    boost::asio::io_context _client_io_context;
    const boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);

    for (auto _ : state) {
        boost::asio::ip::tcp::socket _socket(_client_io_context);

        // Measure time taken to successfully connect.
        boost::system::error_code _error_code;
        _socket.connect(_endpoint, _error_code);

        if (_error_code) {
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

// Benchmark measuring TCP ping throughput.
static void BM_TCP_Ping_Throughput(benchmark::State& state) {
    setup_server(1);

    const size_t _requests_quantity = state.range(0);
    const unsigned short _port = g_server->get_port();

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

    // Create an instance of the frame builder to construct the requests payload.
    protocol::frame_builder _frame_builder;
    // Instantiate a specific request builder using the factory pattern logic.
    auto _request_builder = _frame_builder.as_request();
    // Pre-allocate memory exactly matching the quantity of generated benchmark requests.
    _request_builder.reserve(_requests_quantity);

    // Iterate through the number of requests to generate the required payload bounds.
    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        // Sequentially append ping payloads matching the benchmark iteration requirements.
        _request_builder.add_ping();
    }

    // Allocate the full buffer
    std::vector<uint8_t> _buffer = _request_builder.get_buffers();

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
        boost::endian::little_to_native_inplace(_response_header_length);

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

        // Increment read metrics tracking bytes matching IO payload responses bounds.
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

// Benchmark measuring TCP ping throughput in multithreaded environment.
static void BM_TCP_Ping_Throughput_MT(benchmark::State& state) {
    // Setup server with thread count equal to the second range parameter
    setup_server(state.range(1));

    const size_t _requests_quantity = state.range(0);
    const unsigned short _port = g_server->get_port();

    boost::asio::io_context _client_io_context;
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    // Create a connection specifically for this benchmark sequence to avoid connection timing overhead in the loop.
    boost::system::error_code _error_code;
    _socket.connect(_endpoint, _error_code);

    if (_error_code) {
        state.SkipWithError("Failed to connect to the server.");
        return;
    }

    // Create an instance of the frame builder to construct the requests payload.
    constexpr protocol::frame_builder _frame_builder;
    // Instantiate a specific request builder using the factory pattern logic.
    auto _request_builder = _frame_builder.as_request();
    // Pre-allocate memory exactly matching the quantity of generated benchmark requests.
    _request_builder.reserve(_requests_quantity);

    // Iterate through the number of requests to generate the required payload bounds.
    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        // Sequentially append ping payloads matching the benchmark iteration requirements.
        _request_builder.add_ping();
    }

    // Allocate the full buffer
    std::vector<uint8_t> _buffer = _request_builder.get_buffers();

    // Define tracker for accumulated outgoing written bytes
    size_t _total_bytes_written = 0;

    // Define tracker for accumulated incoming read bytes
    size_t _total_bytes_read = 0;

    for (auto _ : state) {
        // Write the frame buffer completely towards the tcp socket synchronously
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _error_code);

        // Exit early if a connection issue prevented the frame buffer from writing
        if (_error_code) {
            state.SkipWithError("Failed to write to the socket.");
            break;
        }

        // Add the buffer size to the written tracker accumulator
        _total_bytes_written += _buffer.size();

        // Initialize local 32-bit unsigned integer to capture the response header
        uint32_t _response_header_length = 0;

        // Read the exact 4 bytes synchronous representing the response header size
        boost::asio::read(_socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)), _error_code);

        // Exit early if a connection issue prevented the header read from finishing
        if (_error_code) {
            state.SkipWithError("Failed to read response header.");
            break;
        }

        // Pause benchmark timer avoiding penalizing throughput with parsing/allocation logic
        state.PauseTiming();

        // Parse and restore the response header to native endianness format
        boost::endian::little_to_native_inplace(_response_header_length);

        // Prepare an adequate size binary buffer vector for accommodating the response body
        std::vector<uint8_t> _response_body(_response_header_length);

        // Resume benchmark timer focusing primarily over server side execution cycles processing capability
        state.ResumeTiming();

        // Proceed synchronously with a complete socket read capturing the total body
        boost::asio::read(_socket, boost::asio::buffer(_response_body), _error_code);

        // Break out of loop dynamically if a socket error is met during read logic
        if (_error_code) {
            state.SkipWithError("Failed to read response body.");
            break;
        }

        // Pause benchmark timer avoiding penalizing throughput with bookkeeping logic
        state.PauseTiming();

        // Increment read metrics tracking bytes matching IO payload responses bounds.
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

// Arguments are (payload_size, thread_count)
BENCHMARK(BM_TCP_Ping_Throughput_MT)
    ->Args({1, 2})->Args({64, 2})->Args({1024, 2})->Args({10240, 2})
    ->Args({1, 4})->Args({64, 4})->Args({1024, 4})->Args({10240, 4})
    ->Args({1, 8})->Args({64, 8})->Args({1024, 8})->Args({10240, 8})
    ->ThreadRange(1, 8); // Setup client threads range

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

// Run benchmark engine.
int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
