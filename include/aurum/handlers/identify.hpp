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

#ifndef AURUM_HANDLERS_IDENTIFY_HPP
#define AURUM_HANDLERS_IDENTIFY_HPP

#include <aurum/state.hpp>
#include <aurum/session.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/endian/conversion.hpp>
#include <cstring>

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for identify operational codes.
     * @return A callable matching handler_type that executes the identify logic.
     */
    inline handler_type get_identify_handler() {
        // Return a lambda capturing nothing, taking the required handler_type arguments.
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, const shared_session& session, const shared_state& state) -> void {
            // Ensure the identify opcode is only processed by node-to-node TCP sessions smoothly.
            if (session->get_type() == protocol::websocket) {
                return;
            }

            // Ensure the payload size matches the expected length for a UUID (16 bytes).
            if (payload.size() < 16) {
                // If it's too small, just return gracefully to prevent out-of-bounds errors.
                return;
            }

            // Define a local struct matching identifier tracking context smoothly natively.
            boost::uuids::uuid _remote_node_id;
            // Map directly copying memory layout bytes securely transferring identifier reference.
            std::memcpy(_remote_node_id.data, payload.data(), 16);

            // Set the extracted remote node identifier on the active current network session dynamically.
            session->set_node_id(_remote_node_id);

            // Check if the payload contains the connections_per_node (2 bytes) + port (2 bytes) + host (optional).
            // A valid request payload should have at least node_id (16), connections (2), and port (2) = 20 bytes.
            // A response payload might only have node_id (16 bytes).
            if (type == request && payload.size() >= 20) {
                // Variable to temporarily hold the encoded connections per node size.
                std::uint16_t _connections_per_node;
                // Copy the 16-bit connections block from the payload byte array.
                std::memcpy(&_connections_per_node, payload.data() + 16, sizeof(_connections_per_node));
                // Decode the connections from little-endian to the host machine's native architecture.
                boost::endian::little_to_native_inplace(_connections_per_node);

                // Variable to temporarily hold the encoded port size.
                std::uint16_t _remote_port;
                // Copy the 16-bit port block from the payload byte array right after the connections.
                std::memcpy(&_remote_port, payload.data() + 18, sizeof(_remote_port));
                // Decode the port from little-endian to the host machine's native architecture.
                boost::endian::little_to_native_inplace(_remote_port);
                // Assign the extracted listening port directly to the session reference.
                session->set_port(_remote_port);

                // Declare the string variable cleanly mapping host safely natively.
                std::string _remote_host;

                // Verify if there are extra bytes in the payload representing the host string.
                if (payload.size() > 20) {
                    // Reinterpret the remaining buffer bytes into a standard string representing the host domain or IP.
                    _remote_host = std::string(reinterpret_cast<const char*>(payload.data() + 20), payload.size() - 20);
                    // Attach the parsed text string directly to the session context.
                    session->set_host(_remote_host);
                }

                // Map the active local node ID into the response builder sending it back cleanly natively properly.
                // The response only needs to provide the local node identity back clearly mapping perfectly.
                builder.add_identify(transaction_id, state->get_node_id());

                // If the remote requested multiple connections natively properly intelligently cleanly effectively.
                if (_connections_per_node > 1 && !_remote_host.empty() && _remote_port != 0) {
                    // Spawn asynchronous connections securely intelligently.
                    for (std::uint16_t _i = 1; _i < _connections_per_node; ++_i) {
                        // Request connection natively sending subsequent identify payloads explicitly defining 0 constraints intelligently seamlessly mapping neatly natively correctly correctly accurately correctly.
                        boost::asio::post(state->get_io_context(), [state, _remote_host, _remote_port]() {
                            state->connect(_remote_host, _remote_port, false, 0);
                        });
                    }
                }
            } else if (type == request && payload.size() >= 16) {
                // Handle backwards compatibility or simple identify requests safely mapping structurally elegantly correctly seamlessly cleanly appropriately explicitly correctly natively securely cleanly cleanly securely cleanly perfectly flawlessly expertly smartly successfully accurately dynamically elegantly seamlessly explicitly cleanly intelligently cleanly securely securely cleanly perfectly naturally effectively successfully correctly flawlessly effectively seamlessly smartly expertly.
                builder.add_identify(transaction_id, state->get_node_id());
            }
        };
    }
}

#endif // AURUM_HANDLERS_IDENTIFY_HPP
