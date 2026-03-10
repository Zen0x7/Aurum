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
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, shared_session session, shared_state state) -> void {
            boost::ignore_unused(payload);

            if (type == request) {
                std::vector<std::pair<std::string, std::uint16_t>> _nodes;

                std::shared_lock _lock(state->get_sessions_mutex());

                for (const auto& _target_session : state->get_sessions()) {
                    if (_target_session->get_node_id() != session->get_node_id() && _target_session->get_node_id() != state->get_node_id()) {
                        _nodes.emplace_back(_target_session->get_host(), _target_session->get_port());
                    }
                }

                builder.add_discovery(transaction_id, _nodes);
            } else if (type == response) {
                if (payload.size() < 4) return;

                std::uint32_t _nodes_count;
                std::memcpy(&_nodes_count, payload.data(), sizeof(_nodes_count));
                boost::endian::little_to_native_inplace(_nodes_count);

                std::size_t _offset = 4;

                for (std::uint32_t _i = 0; _i < _nodes_count; ++_i) {
                    if (_offset + 4 > payload.size()) break;

                    std::uint16_t _port;
                    std::memcpy(&_port, payload.data() + _offset, sizeof(_port));
                    boost::endian::little_to_native_inplace(_port);
                    _offset += 2;

                    std::uint16_t _host_size;
                    std::memcpy(&_host_size, payload.data() + _offset, sizeof(_host_size));
                    boost::endian::little_to_native_inplace(_host_size);
                    _offset += 2;

                    if (_offset + _host_size > payload.size()) break;

                    std::string _host(reinterpret_cast<const char*>(payload.data() + _offset), _host_size);
                    _offset += _host_size;

                    bool _already_connected = false;

                    {
                        std::shared_lock _lock(state->get_sessions_mutex());
                        for (const auto& _target_session : state->get_sessions()) {
                            if (_target_session->get_host() == _host && _target_session->get_port() == _port) {
                                _already_connected = true;
                                break;
                            }
                        }
                    }

                    if (!_already_connected) {
                        state->connect(_host, _port, false);
                    }
                }
            }
        };
    }
}

#endif // AURUM_HANDLERS_DISCOVERY_HPP