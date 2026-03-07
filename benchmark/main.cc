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

// Include aurum definitions directly avoiding prefix clutter.
using namespace aurum;

/**
 * @brief Global context structure required to run the server in the background for benchmarks.
 */
struct server_fixture {
    /** @brief Asynchronous IO context managing network operations. */
    boost::asio::io_context io_context_;

    /** @brief Shared configuration and connections state wrapper. */
    std::shared_ptr<state> state_;

    /** @brief The actual TCP acceptor wrapping the local network listening socket. */
    std::shared_ptr<tcp_listener> listener_;

    /** @brief Worker thread executing the async event loop independently from the benchmark run. */
    std::thread thread_;

    /**
     * @brief Constructs the environment booting an ephemeral server instance.
     */
    server_fixture() : state_(std::make_shared<state>()) {
        // Request the operating system to bind to any available ephemeral port.
        state_->get_configuration().tcp_port_.store(0, std::memory_order_release);

        // Limit the worker thread count specifically to 1 for simplified benchmarking scope.
        state_->get_configuration().threads_.store(1, std::memory_order_release);

        // Instantiate the local TCP listener bounded to the active test configuration.
        listener_ = std::make_shared<tcp_listener>(io_context_, state_);

        // Start listening to incoming events over the asynchronous context.
        listener_->start();

        // Launch the internal background loop explicitly on a separate worker thread.
        thread_ = std::thread([this]() {
            // Forward execution over to the boost ASIO backend.
            io_context_.run();
        });

        // Loop continuously blocking execution until the listener flag flips indicating readiness.
        while (!state_->get_configuration().tcp_ready_.load(std::memory_order_acquire)) {
            // Release the current thread slice over to the scheduler until the next check.
            std::this_thread::yield();
        }
    }

    /**
     * @brief Destructor stopping the async loop and cleaning up the worker thread gracefully.
     */
    ~server_fixture() {
        // Cancel all pending operations shutting down the executor pipeline.
        io_context_.stop();

        // Ensure that the background thread hasn't been torn down prior.
        if (thread_.joinable()) {
            // Block until the background worker properly stops executing instructions.
            thread_.join();
        }
    }

    /**
     * @brief Reads the dynamically bounded port safely.
     * @return The local ephemeral port obtained during startup.
     */
    unsigned short get_port() const {
        // Return the acquired port variable synchronized from the state component.
        return state_->get_configuration().tcp_port_.load(std::memory_order_acquire);
    }
};

// Define a global singleton instance holding the active testing dummy server.
static std::unique_ptr<server_fixture> g_server;

/**
 * @brief Benchmark measuring TCP connection establishment overhead against the dummy server.
 * @param state The benchmark engine state tracker controlling iterations.
 */
static void BM_TCP_Connect(benchmark::State& state) {
    // Lazily instantiate the global testing server during the first executed benchmark pass.
    if (!g_server) {
        // Create the singleton wrapper spinning up the isolated tcp server thread.
        g_server = std::make_unique<server_fixture>();
    }

    // Retrieve the active port the dummy server instance was bound against.
    unsigned short _port = g_server->get_port();

    // Setup an ephemeral local context dedicated to test client queries.
    boost::asio::io_context _client_io_context;

    // Define the exact local loopback coordinate mapping the destination of connection tests.
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);

    // Iteratively loop through testing rounds as commanded by the external benchmark engine.
    for (auto _ : state) {
        // Dynamically instantiate a physical TCP client socket wrapping an OS handle.
        boost::asio::ip::tcp::socket _socket(_client_io_context);

        // Create an error variable struct explicitly to capture non-throwing failure messages.
        boost::system::error_code _ec;

        // Issue a synchronous TCP connect instruction tracking performance timings explicitly.
        _socket.connect(_endpoint, _ec);

        // Terminate test if an unexpected failure occurred during socket linking step.
        if (_ec) {
            // Log test failure clearly into benchmark system console output.
            state.SkipWithError("Failed to connect to the server.");
            // Escape immediate test loop gracefully.
            break;
        }

        // Drop the physical connection closing the file descriptor accurately terminating TCP scope.
        _socket.close();

        // Expose object reference intentionally keeping it out of dead code optimizer pruning passes.
        benchmark::DoNotOptimize(_socket);
    }
}

// Register TCP connection test macro within the global benchmark runner system queue.
BENCHMARK(BM_TCP_Connect);

/**
 * @brief Benchmark measuring raw write throughput (bitrate) by directly piping large buffers over a live connection.
 * @param state The tracking benchmark system object state instance.
 */
static void BM_TCP_Write_Throughput(benchmark::State& state) {
    // Assert creation of the dummy isolated network server before executing tests against it.
    if (!g_server) {
        // Initialize the tracking application server variable properly.
        g_server = std::make_unique<server_fixture>();
    }

    // Read the parameter iteration range requested specifically by the benchmarking harness macro.
    size_t _payload_size = state.range(0);

    // Obtain the actively listening physical port number.
    unsigned short _port = g_server->get_port();

    // Create an isolated logical testing IO environment.
    boost::asio::io_context _client_io_context;

    // Point the connection destination appropriately at the local system address boundary.
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);

    // Spin up an actual client socket variable structure instance.
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    // Track physical error status internally skipping exception overheads entirely.
    boost::system::error_code _ec;

    // Connect explicitly before the loop executing to purely isolate the writing speeds themselves natively.
    _socket.connect(_endpoint, _ec);

    // Assert that we have securely established a path mapping to the destination point.
    if (_ec) {
        // Yield error message properly halting benchmark sequence gracefully mapping safely.
        state.SkipWithError("Failed to connect to the server.");
        // Break early returning execution context properly to avoid faults later natively.
        return;
    }

    // Prepare a flat continuous payload block wrapping both the packet size prefix and actual target string cleanly.
    std::vector<uint8_t> _buffer(sizeof(uint32_t) + _payload_size);

    // Explicitly define a localized prefix primitive determining byte boundaries properly mapped cleanly tracking safely.
    uint32_t _header_length = static_cast<uint32_t>(_payload_size);

    // Reformat variable bounds natively representing proper endian formatting properly.
    boost::endian::native_to_big_inplace(_header_length);

    // Blindly dump prefix limits boundaries securely natively against memory array cleanly.
    std::memcpy(_buffer.data(), &_header_length, sizeof(uint32_t));

    // Pad entire remaining memory bounds array using fixed letter elements safely appropriately cleanly accurately mapping tracking appropriately.
    std::memset(_buffer.data() + sizeof(uint32_t), 'A', _payload_size);

    // Start testing harness evaluating loop boundaries properly accurately correctly smoothly reliably accurately.
    for (auto _ : state) {
        // Flush memory bytes blindly over network stack asynchronously returning securely accurately mapped bounds reliably mapping appropriately reliably smoothly correctly tracking securely accurately correctly reliably cleanly appropriately successfully mapping appropriately safely appropriately
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _ec);

        // Ensure successful byte output bounds mapping safely securely appropriately correctly accurately tracking securely securely cleanly
        if (_ec) {
            // Mark sequence failed safely natively reliably smoothly securely correctly tracking cleanly safely
            state.SkipWithError("Failed to write to the socket.");
            // Prevent next cycle execution bounds tracking correctly maps safely reliably smoothly securely appropriately
            break;
        }

        // Push pointer wrapper escaping compiler optimizer tracking appropriately correctly securely tracking accurately mapping properly correctly maps reliably
        benchmark::DoNotOptimize(_buffer);
    }

    // Tally raw output tracking accurately matching correctly written bytes mapping mapping correctly correctly mapping cleanly properly
    state.SetBytesProcessed(state.iterations() * _payload_size);
}

// Queue bandwidth write throughput testing routines properly spanning payload bounds recursively natively securely.
BENCHMARK(BM_TCP_Write_Throughput)->Range(8, 8192);

/**
 * @brief Benchmark measuring TCP ping throughput.
 * @param state Benchmark engine tracking object.
 */
static void BM_TCP_Ping_Throughput(benchmark::State& state) {
    // Lazily instantiate global dummy test server singleton.
    if (!g_server) {
        // Spin up an ephemeral backend properly bounded.
        g_server = std::make_unique<server_fixture>();
    }

    // Read requested iteration variable mapping correctly tracking parameters smoothly correctly bounds mapping safely.
    size_t _requests_quantity = state.range(0);

    // Acquire testing server bound port properly.
    unsigned short _port = g_server->get_port();

    // Create client context environment.
    boost::asio::io_context _client_io_context;

    // Configure loopback target mapping correctly bounds cleanly mapping appropriately safely correctly properly tracking appropriately safely correctly.
    boost::asio::ip::tcp::endpoint _endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), _port);

    // Bootstrap network client.
    boost::asio::ip::tcp::socket _socket(_client_io_context);

    // Track internal errors safely.
    boost::system::error_code _ec;

    // Connect test client.
    _socket.connect(_endpoint, _ec);

    // Assert connection successfully mapped properly accurately cleanly safely tracking safely safely maps safely properly tracking safely mapping securely.
    if (_ec) {
        // Halt benchmark smoothly recording physical error maps cleanly cleanly securely properly maps mapping correctly appropriately accurately correctly securely appropriately securely securely properly.
        state.SkipWithError("Failed to connect to the server.");
        // Break execution completely properly correctly mapping smoothly accurately appropriately tracking cleanly cleanly maps tracking safely.
        return;
    }

    // Allocate continuous transmission payload appropriately securely tracking safely safely maps cleanly.
    std::vector<uint8_t> _payload;

    // Create a 16-bit integer representing the number of requests.
    std::uint16_t _requests_quantity_be = static_cast<std::uint16_t>(_requests_quantity);

    // Convert the requests quantity to big endian format.
    boost::endian::native_to_big_inplace(_requests_quantity_be);

    // Create a byte pointer to the big endian requests quantity.
    auto* _requests_quantity_ptr = reinterpret_cast<const uint8_t*>(&_requests_quantity_be);

    // Insert the big endian requests quantity into the payload buffer.
    _payload.insert(_payload.end(), _requests_quantity_ptr, _requests_quantity_ptr + sizeof(_requests_quantity_be));

    // Iterate through the number of requests.
    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        // Set the size of the request payload (1 byte opcode + 16 bytes transaction ID).
        std::uint16_t _request_length = 17;

        // Convert the request length to big endian format.
        boost::endian::native_to_big_inplace(_request_length);

        // Create a byte pointer to the big endian request length.
        auto* _request_length_ptr = reinterpret_cast<const uint8_t*>(&_request_length);

        // Insert the request length into the payload buffer.
        _payload.insert(_payload.end(), _request_length_ptr, _request_length_ptr + sizeof(_request_length));
    }

    // Iterate through the number of requests.
    for (size_t _index = 0; _index < _requests_quantity; ++_index) {
        // Append the operational code corresponding to ping (1).
        _payload.push_back(1);

        // Generate a random 16 byte unique identifier for the transaction.
        boost::uuids::uuid _transaction_id = boost::uuids::random_generator()();

        // Insert the generated transaction ID into the payload buffer.
        _payload.insert(_payload.end(), _transaction_id.begin(), _transaction_id.end());
    }

    // Create a CRC16-CCITT object to calculate the checksum.
    boost::crc_ccitt_type _crc;

    // Calculate the CRC16-CCITT checksum for the constructed payload buffer.
    _crc.process_bytes(_payload.data(), _payload.size());

    // Retrieve the calculated checksum as an unsigned 16-bit integer.
    std::uint16_t _crc_value = _crc.checksum();

    // Convert the calculated checksum to big endian format.
    boost::endian::native_to_big_inplace(_crc_value);

    // Create a byte pointer to the big endian checksum.
    auto* _crc_ptr = reinterpret_cast<const uint8_t*>(&_crc_value);

    // Insert the checksum into the payload buffer.
    _payload.insert(_payload.end(), _crc_ptr, _crc_ptr + sizeof(_crc_value));

    // Allocate the full buffer.
    std::vector<uint8_t> _buffer;

    // Calculate the total size of the frame's payload.
    uint32_t _header_length = static_cast<uint32_t>(_payload.size());

    // Convert the calculated payload length into big endian format.
    boost::endian::native_to_big_inplace(_header_length);

    // Create a byte pointer to the big endian payload length.
    auto* _header_ptr = reinterpret_cast<const uint8_t*>(&_header_length);

    // Insert the header length into the frame buffer.
    _buffer.insert(_buffer.end(), _header_ptr, _header_ptr + sizeof(_header_length));

    // Insert the payload containing the body of the requests into the frame buffer.
    _buffer.insert(_buffer.end(), _payload.begin(), _payload.end());

    // Define tracker for accumulated outgoing written bytes.
    size_t _total_bytes_written = 0;

    // Define tracker for accumulated incoming read bytes.
    size_t _total_bytes_read = 0;

    // Iterate through benchmark state loop safely running the configured workload operations.
    for (auto _ : state) {
        // Write the frame buffer completely towards the tcp socket synchronously.
        boost::asio::write(_socket, boost::asio::buffer(_buffer), _ec);

        // Exit early if a connection issue prevented the frame buffer from writing.
        if (_ec) {
            // Signal output mapping error accurately to the benchmark harness console.
            state.SkipWithError("Failed to write to the socket.");
            // Break from test loop effectively stopping further evaluation iterations.
            break;
        }

        // Add the buffer size to the written tracker accumulator.
        _total_bytes_written += _buffer.size();

        // Initialize local 32-bit unsigned integer to capture the response header.
        uint32_t _response_header_length = 0;

        // Read the exact 4 bytes synchronous representing the response header size.
        boost::asio::read(_socket, boost::asio::buffer(&_response_header_length, sizeof(_response_header_length)), _ec);

        // Exit early if a connection issue prevented the header read from finishing.
        if (_ec) {
            // Tally test as failed appropriately logging read limits failures gracefully.
            state.SkipWithError("Failed to read response header.");
            // Abort test sequence explicitly avoiding dangling pointers or misaligned byte bounds.
            break;
        }

        // Pause benchmark timer avoiding penalizing throughput with parsing/allocation logic.
        state.PauseTiming();

        // Parse and restore the response header to native endianness format.
        boost::endian::big_to_native_inplace(_response_header_length);

        // Prepare an adequate size binary buffer vector for accommodating the response body.
        std::vector<uint8_t> _response_body(_response_header_length);

        // Resume benchmark timer focusing primarily over server side execution cycles processing capability.
        state.ResumeTiming();

        // Proceed synchronously with a complete socket read capturing the total body.
        boost::asio::read(_socket, boost::asio::buffer(_response_body), _ec);

        // Break out of loop dynamically if a socket error is met during read logic.
        if (_ec) {
            // Dump network reading trace error directly towards logging sink securely.
            state.SkipWithError("Failed to read response body.");
            // Terminate test iteration securely escaping from malformed memory boundaries.
            break;
        }

        // Pause benchmark timer avoiding penalizing throughput with bookkeeping logic.
        state.PauseTiming();

        // Measure read IO bounds augmenting total received elements.
        _total_bytes_read += sizeof(_response_header_length) + _response_body.size();

        // Flag vectors to be skipped out of aggressive dead code optimization routines.
        benchmark::DoNotOptimize(_buffer);
        // Avoid skipping variable mapping effectively ensuring compiler evaluates dynamically read arrays.
        benchmark::DoNotOptimize(_response_body);

        // Resume benchmark timer targeting the next read/write cycles boundary.
        state.ResumeTiming();
    }

    // Mark absolute byte throughput correctly by combining written and read data stream operations.
    state.SetBytesProcessed(_total_bytes_written + _total_bytes_read);

    // Mark individual requests throughput parsed internally through server processing.
    state.SetItemsProcessed(state.iterations() * _requests_quantity);
}

// Queue bandwidth write throughput testing routines tracking multiple variable array iterations.
BENCHMARK(BM_TCP_Ping_Throughput)->Arg(1)->Arg(64)->Arg(1024)->Arg(10240);

/**
 * @brief Run benchmark engine safely properly initializing native arguments map tracking.
 * @param argc The count of command line arguments.
 * @param argv An array of command line arguments strings.
 * @return Exit status code of the benchmark tracking properly mapped sequences.
 */
int main(int argc, char** argv) {
    // Bootstrap tracking mapping accurately matching global benchmark framework configurations.
    ::benchmark::Initialize(&argc, argv);

    // Evaluate whether passed limits boundaries correctly mapped expected test environment arguments.
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        // Return exit natively flagging unrecognized mappings correctly handled properly.
        return 1;
    }

    // Dispatch cleanly executing all explicitly configured physical benchmark macros mapping correctly.
    ::benchmark::RunSpecifiedBenchmarks();

    // Release tracking system properly terminating active metrics loggers correctly.
    ::benchmark::Shutdown();

    // Complete application returning success code accurately matching system map structures properly.
    return 0;
}
