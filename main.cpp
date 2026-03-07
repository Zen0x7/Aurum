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

#include <aurum/state.hpp>
#include <aurum/tcp_listener.hpp>
#include <aurum/tcp_session.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>

/**
 * @brief Application entry point.
 * @param argc The count of command line arguments.
 * @param argv An array of command line arguments strings.
 * @return Exit status code of the application.
 */
int main(int argc, char *argv[]) {
    // Import framework namespace.
    using namespace aurum;

    // Allocate single IO context for processing asynchronous requests.
    boost::asio::io_context _io_context;

    // Instantiate global shared state tracking network sessions and configuration.
    auto _state = std::make_shared<state>();

    // Define variable storing active thread count parameter.
    std::size_t _threads;

    // Define variable storing application listener port.
    unsigned short _port;

    // Initialize command line options parser instance.
    boost::program_options::options_description _option_descriptions("Program options");

    // Define program options with defaults for threads and port.
    _option_descriptions.add_options()
            ("threads", boost::program_options::value<std::size_t>(&_threads)->default_value(1))
            ("port", boost::program_options::value<unsigned short>(&_port)->default_value(0));

    // Construct local variable map container.
    boost::program_options::variables_map _variables;

    // Parse the command line options.
    store(parse_command_line(argc, argv, _option_descriptions), _variables);

    // Notify the variables map to update bound variables.
    notify(_variables);

    // Persist parsed application port parameter into configuration.
    _state->get_configuration().tcp_port_.store(_port, std::memory_order_release);

    // Persist parsed thread count into configuration.
    _state->get_configuration().threads_.store(_threads, std::memory_order_release);

    // Register async application shutdown handlers for SIGINT and SIGTERM.
    boost::asio::signal_set _signals(_io_context, SIGINT, SIGTERM);

    // Bind async wait to stop the io_context on signal.
    _signals.async_wait([&](auto, auto){ _io_context.stop(); });

    // Spawn networking connection listener targeting incoming traffic.
    auto _tcp_listener = std::make_shared<tcp_listener>(_io_context, _state);

    // Start accepting TCP connections.
    _tcp_listener->start();

    // Create a vector to store thread handles.
    std::vector<std::thread> _thread_pool;

    // Preallocate vector capacity.
    _thread_pool.reserve(_state->get_configuration().threads_);

    // Spawn the requested number of worker threads.
    for (std::size_t _index = 0; _index < _state->get_configuration().threads_.load(std::memory_order_acquire); ++_index) {
        // Run the io_context in each thread.
        _thread_pool.emplace_back([&]{ _io_context.run(); });
    }

    // Wait for all threads to complete.
    for (auto& _thread : _thread_pool) {
        // Block until the thread finishes execution.
        _thread.join();
    }

    // Exit application with success code.
    return 0;
}
