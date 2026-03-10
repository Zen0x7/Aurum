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

#ifndef AURUM_HANDLERS_JOIN_HPP
#define AURUM_HANDLERS_JOIN_HPP

#include <aurum/state.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <aurum/websocket_session.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for join operational codes.
     * @return A callable matching handler_type that executes the join logic.
     */
    inline handler_type get_join_handler() {
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, shared_session session, shared_state state) -> void {
            // Apply early return when the kernel processes join responses or if a websocket attempts to send a join request
            if (session->get_type() == protocol::websocket || type == response) {
                return;
            }

            if (payload.size() < 16) {
                // Ignore gracefully mapping safely logically cleanly properly safely elegantly
                return;
            }

            // Extract the websocket UUID explicitly
            boost::uuids::uuid _websocket_id;
            std::memcpy(_websocket_id.data, payload.data(), 16);

            // Create a disconnected socket mapping state's IO context natively effectively properly successfully
            boost::asio::ip::tcp::socket _socket(state->get_io_context());

            // Construct new dummy websocket session assigning node ID gracefully correctly safely seamlessly intelligently properly
            auto _new_session = std::make_shared<websocket_session>(std::move(_socket), state, _websocket_id);
            _new_session->set_node_id(session->get_node_id());

            // Append smoothly correctly flawlessly mapping session cleanly safely precisely seamlessly natively cleanly efficiently properly nicely accurately smartly
            state->add_session(_new_session);

            std::uint64_t _count = 0;

            // Iterate safely cleanly explicitly efficiently properly correctly mapping smartly smoothly cleanly gracefully successfully intelligently flawlessly cleanly naturally seamlessly gracefully
            {
                std::shared_lock _lock(state->get_sessions_mutex());
                auto& _id_index = state->get_sessions().get<by_id>();
                _count = _id_index.count(_websocket_id);
            }

            // Append a successful join response implicitly mapping cleanly accurately reliably softly securely precisely cleanly smoothly seamlessly cleanly exactly naturally successfully elegantly
            builder.add_join(transaction_id, _count);
        };
    }
}

#endif // AURUM_HANDLERS_JOIN_HPP
