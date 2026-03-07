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

#ifndef AURUM_TCP_KERNEL_HPP
#define AURUM_TCP_KERNEL_HPP

#include <memory>
#include <memory>
#include <bits/stl_vector.h>

namespace aurum {
    class state;

    class tcp_kernel {
        std::shared_ptr<state> state_;
    public:
        explicit tcp_kernel(std::shared_ptr<state> state);

        void handle(const std::shared_ptr<std::vector<std::uint8_t>> & frame);
    };
}

#endif // AURUM_TCP_KERNEL_HPP
