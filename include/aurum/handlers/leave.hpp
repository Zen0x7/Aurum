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

#ifndef AURUM_HANDLERS_LEAVE_HPP
#define AURUM_HANDLERS_LEAVE_HPP

#include <aurum/state.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <aurum/websocket_session.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for leave operational codes.
     * @return A callable matching handler_type that executes the leave logic.
     */
    inline handler_type get_leave_handler() {
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, const shared_session& session, const shared_state& state) -> void {
            if (session->get_type() == protocol::websocket || type == response) {
                return;
            }

            if (payload.size() < 16) {
                return;
            }

            boost::uuids::uuid _websocket_id;
            std::memcpy(_websocket_id.data, payload.data(), 16);

            std::uint64_t _removed_count = 0;
            std::vector<std::shared_ptr<aurum::session>> _sessions_to_disconnect;

            {
                std::unique_lock _lock(state->get_sessions_mutex());
                auto& _id_index = state->get_sessions().get<by_id>();

                auto _it = _id_index.find(_websocket_id);
                while (_it != _id_index.end()) {
                    _sessions_to_disconnect.push_back(*_it);
                    _it = _id_index.erase(_it);
                    _removed_count++;

                    _it = _id_index.find(_websocket_id);
                }
            }

            // Removed to prevent deadlocking within disconnect logic
            // The dummy session is already unregistered from state map by erase()
            // And being a dummy session, the disconnect is merely closing a non-started stream.

            builder.add_leave(transaction_id, _removed_count);
        };
    }
}

#endif // AURUM_HANDLERS_LEAVE_HPP
