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

int main(int argc, char *argv[]) {
    using namespace aurum;


    boost::asio::io_context _io_context;
    auto _state = std::make_shared<state>();

    std::size_t _threads;
    unsigned short _port;

    boost::program_options::options_description _option_descriptions("Program options");
    _option_descriptions.add_options()
            ("threads", boost::program_options::value<std::size_t>(&_threads)->default_value(1))
            ("port", boost::program_options::value<unsigned short>(&_port)->default_value(0));

    boost::program_options::variables_map _variables;
    store(parse_command_line(argc, argv, _option_descriptions), _variables);
    notify(_variables);

    _state->get_configuration().tcp_port_.store(_port, std::memory_order_release);
    _state->get_configuration().threads_.store(_threads, std::memory_order_release);

    boost::asio::signal_set _signals(_io_context, SIGINT, SIGTERM);
    _signals.async_wait([&](auto, auto){ _io_context.stop(); });

    auto _tcp_listener = std::make_shared<tcp_listener>(_io_context, _state);

    std::vector<std::thread> _thread_pool;
    _thread_pool.reserve(_state->get_configuration().threads_);

    for (std::size_t _index = 0; _index < _state->get_configuration().threads_.load(std::memory_order_acquire); ++_index) {
        _thread_pool.emplace_back([&]{ _io_context.run(); });
    }

    for (auto& _thread : _thread_pool) {
        _thread.join();
    }

    return 0;
}
