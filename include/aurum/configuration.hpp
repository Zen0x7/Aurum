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

#ifndef AURUM_CONFIGURATION_HPP
#define AURUM_CONFIGURATION_HPP

#include <atomic>

namespace aurum {
    /**
     * @brief Holds global configuration parameters for the Aurum state.
     * @details This structure uses atomics to allow concurrent access across multiple threads.
     */
    struct configuration {
      /** @brief Number of active worker threads for IO operations. */
      std::atomic<std::size_t> threads_ {1};

      /** @brief TCP port where the listener is currently bound. */
      std::atomic<unsigned short> tcp_port_{0};

      /** @brief The number of connections permitted per node in the network. */
      std::atomic<std::uint16_t> connections_per_node_{1};

      /** @brief Flag indicating if the TCP listener is fully initialized and ready. */
      std::atomic<bool> tcp_ready_{false};

        /** @brief Port explicitly bound for incoming websocket protocol connections securely structurally mapped. */
        std::atomic<std::uint16_t> websocket_port_{0};

        /** @brief Flag signaling if the WebSocket server has completely bound to the designated port and is actively listening natively. */
        std::atomic<bool> websocket_ready_{false};
    };
}

#endif // AURUM_CONFIGURATION_HPP
