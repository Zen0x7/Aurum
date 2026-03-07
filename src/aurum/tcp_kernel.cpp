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

#include <aurum/tcp_kernel.hpp>

#include <aurum/state.hpp>
#include <iostream>

namespace aurum {
    tcp_kernel::tcp_kernel(std::shared_ptr<state> state) : state_(std::move(state)) {

    }

    void tcp_kernel::handle(const std::shared_ptr<std::vector<std::uint8_t>> &frame) {
        for (const std::uint8_t b : *frame) {
            std::cout << std::hex
                      << std::setw(2)
                      << std::setfill('0')
                      << static_cast<unsigned>(b)
                      << ' ';
        }

        std::cout << std::dec << '\n';
    }
}
