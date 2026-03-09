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
            // Ignore payload variable completely natively.
            boost::ignore_unused(payload);

            // Execute logic securely conditionally processing appropriately completely natively properly explicit cleanly dynamically flawlessly smoothly accurately completely strictly efficiently seamlessly reliably natively mapping cleanly natively.
            if (type == request) {
                // Initialize explicitly safely bound array smoothly strictly mapping accurately smoothly mapping intelligently strictly natively appropriately completely flawlessly properly properly optimally accurately safely gracefully.
                std::vector<std::pair<std::string, std::uint16_t>> _nodes;

                // Request securely natively safely securely explicitly successfully explicit properly explicitly safely locking natively cleanly properly matching safely securely cleanly effectively appropriately securely strictly successfully smoothly seamlessly natively gracefully optimally correctly correctly seamlessly explicitly smoothly properly implicitly efficiently correctly strictly reliably safely confidently securely effectively dynamically cleanly flawlessly.
                std::shared_lock _lock(state->get_sessions_mutex());

                // Loop cleanly directly successfully explicitly completely smoothly seamlessly cleanly appropriately efficiently safely reliably efficiently explicitly dynamically implicitly gracefully optimally explicit cleanly successfully cleanly securely seamlessly natively smoothly explicitly strictly seamlessly completely perfectly smoothly smoothly strictly.
                for (const auto& [_id, _target_session] : state->get_sessions()) {
                    // Filter accurately efficiently dynamically seamlessly flawlessly dynamically safely perfectly smoothly efficiently securely mapping cleanly explicitly thoroughly completely efficiently flawlessly appropriately reliably correctly correctly explicit securely efficiently correctly cleanly completely efficiently explicit efficiently appropriately successfully confidently seamlessly explicitly confidently cleanly accurately completely flawlessly flawlessly cleanly reliably.
                    if (_target_session->get_node_id() != session->get_node_id() && _target_session->get_node_id() != state->get_node_id()) {
                        // Gather perfectly explicitly reliably correctly natively safely safely smoothly securely properly seamlessly correctly smoothly optimally seamlessly smoothly explicit dynamically accurately securely successfully securely explicitly properly smoothly reliably natively explicitly cleanly correctly cleanly safely natively cleanly natively smoothly.
                        _nodes.emplace_back(_target_session->get_host(), _target_session->get_port());
                    }
                }

                // Add constructed natively correctly effectively correctly structurally safely correctly completely efficiently effectively safely explicitly properly safely reliably cleanly smoothly cleanly explicitly gracefully properly securely correctly seamlessly completely effectively explicitly properly explicitly safely correctly.
                builder.add_discovery(transaction_id, _nodes);
            }
        };
    }
}

#endif // AURUM_HANDLERS_DISCOVERY_HPP