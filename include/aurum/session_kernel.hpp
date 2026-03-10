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

#ifndef AURUM_SESSION_KERNEL_HPP
#define AURUM_SESSION_KERNEL_HPP

#include <memory>
#include <vector>
#include <cstdint>

namespace aurum {
    /**
     * Forward state
     */
    class state;

    /**
     * Forward session
     */
    class session;

    /**
     * @brief Protocol parsing kernel responsible for extracting requests from raw network frames.
     * @details Parses binary buffers, validates CRC checksums, and routes opcodes to the state handlers.
     */
    class session_kernel {
        /** @brief Shared reference to the central application state. */
        std::shared_ptr<state> state_;
    public:
        /**
         * @brief Constructs a new session kernel component linked to the global state.
         * @param state A shared pointer to the active application state.
         */
        explicit session_kernel(std::shared_ptr<state> state);

        /**
         * @brief Parses and handles an incoming binary frame buffer.
         * @param frame A shared pointer containing the raw byte array of the network frame.
         * @param session A shared pointer referencing the origin generic session.
         * @return A shared pointer to an allocated response buffer vector, or nullptr if parsing fails.
         */
        std::shared_ptr<const std::vector<std::uint8_t>> handle(
            const std::shared_ptr<std::vector<std::uint8_t>> & frame,
            std::shared_ptr<session> session);
    };
}

#endif // AURUM_SESSION_KERNEL_HPP
