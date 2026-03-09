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

#ifndef AURUM_HANDLERS_DISCOVERY_HPP
#define AURUM_HANDLERS_DISCOVERY_HPP

#include <aurum/state.hpp>
#include <aurum/tcp_session.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <boost/core/ignore_unused.hpp>
#include <vector>
#include <string>
#include <utility>

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for discovery operational codes.
     * @return A callable matching handler_type that executes the discovery logic.
     */
    inline handler_type get_discovery_handler() {
        // Return a lambda capturing nothing, taking the required handler_type arguments.
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, shared_tcp_session session, shared_state state) -> void {
            // Ignore the incoming payload since a discovery request carries no data payload.
            boost::ignore_unused(payload);

            // Only process incoming request messages.
            if (type == request) {
                // Initialize a vector to collect the connected node properties (host and port).
                std::vector<std::pair<std::string, std::uint16_t>> _nodes;

                // Acquire a shared lock to read the sessions container safely without blocking other readers.
                std::shared_lock _lock(state->get_sessions_mutex());

                // Iterate over all active TCP connections mapped in the server state.
                for (const auto& [_id, _target_session] : state->get_sessions()) {
                    // Filter out the node originating the request, and the local processing node itself.
                    if (_target_session->get_node_id() != session->get_node_id() && _target_session->get_node_id() != state->get_node_id()) {
                        // Gather the target host and port string representation.
                        _nodes.emplace_back(_target_session->get_host(), _target_session->get_port());
                    }
                }

                // Call the response builder with the collected node list to encode the payload.
                builder.add_discovery(transaction_id, _nodes);
            }
        };
    }
}

#endif // AURUM_HANDLERS_DISCOVERY_HPP