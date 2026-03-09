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
#include <aurum/session.hpp>
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
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, shared_session session, shared_state state) -> void {
            // Ignore the incoming payload since a discovery request carries no data payload.
            boost::ignore_unused(payload);

            // Only process incoming request messages.
            if (type == request) {
                // Initialize a vector to collect the connected node properties (host and port).
                std::vector<std::pair<std::string, std::uint16_t>> _nodes;

                // Acquire a shared lock to read the sessions container safely without blocking other readers.
                std::shared_lock _lock(state->get_sessions_mutex());

                // Iterate over all active network connections mapped in the server state container safely.
                for (const auto& _target_session : state->get_sessions()) {
                    // Filter out the node originating the request, and the local processing node itself.
                    if (_target_session->get_node_id() != session->get_node_id() && _target_session->get_node_id() != state->get_node_id()) {
                        // Gather the target host and port string representation.
                        _nodes.emplace_back(_target_session->get_host(), _target_session->get_port());
                    }
                }

                // Call the response builder with the collected node list to encode the payload.
                builder.add_discovery(transaction_id, _nodes);
            } else if (type == response) {
                // Ensure the payload has the minimum size to hold the 4-byte count structure.
                if (payload.size() < 4) return;

                // Create a variable to extract the number of discovered peer elements.
                std::uint32_t _nodes_count;
                // Parse the element count directly from the incoming payload block.
                std::memcpy(&_nodes_count, payload.data(), sizeof(_nodes_count));
                // Convert the encoded numeric structure safely to the architecture format.
                boost::endian::little_to_native_inplace(_nodes_count);

                // Initialize a tracking offset starting right after the initial size block.
                std::size_t _offset = 4;

                // Process each explicitly parsed network coordinate mapped in the stream securely.
                for (std::uint32_t _i = 0; _i < _nodes_count; ++_i) {
                    // Prevent parsing over boundaries avoiding out-of-bounds pointer reads securely.
                    if (_offset + 4 > payload.size()) break;

                    // Isolate the remote network connection port natively.
                    std::uint16_t _port;
                    // Copy exactly two bytes indicating the targeted listening port properly.
                    std::memcpy(&_port, payload.data() + _offset, sizeof(_port));
                    // Standardize the byte format converting the networking boundaries accurately.
                    boost::endian::little_to_native_inplace(_port);
                    // Update the pointer indexing securely.
                    _offset += 2;

                    // Isolate explicitly the payload characters representing the hostname boundaries string.
                    std::uint16_t _host_size;
                    // Copy explicitly matching array pointers securely.
                    std::memcpy(&_host_size, payload.data() + _offset, sizeof(_host_size));
                    // Match native layout format correctly.
                    boost::endian::little_to_native_inplace(_host_size);
                    // Standardize advancing reading pointer cleanly.
                    _offset += 2;

                    // Prevent buffer overlap exceptions gracefully stopping execution successfully.
                    if (_offset + _host_size > payload.size()) break;

                    // Parse securely evaluating string components exactly representing targeted peers correctly.
                    std::string _host(reinterpret_cast<const char*>(payload.data() + _offset), _host_size);
                    // Shift the buffer offset forward safely past the parsed characters block.
                    _offset += _host_size;

                    // Track whether the local node is already actively peered with the target.
                    bool _already_connected = false;

                    // Acquire a scoped read lock to inspect the current state's connection map.
                    {
                        std::shared_lock _lock(state->get_sessions_mutex());
                        // Iterate through the actively registered network connection sessions natively correctly.
                        for (const auto& _target_session : state->get_sessions()) {
                            // If a peer identically matches the discovered host and port, mark it to prevent redundant connections.
                            if (_target_session->get_host() == _host && _target_session->get_port() == _port) {
                                _already_connected = true;
                                break;
                            }
                        }
                    }

                    // Proceed to establish a new connection if the peer is genuinely unknown.
                    if (!_already_connected) {
                        // Request the state to connect to the peer dynamically, avoiding chained discovery loops.
                        state->connect(_host, _port, false);
                    }
                }
            }
        };
    }
}

#endif // AURUM_HANDLERS_DISCOVERY_HPP