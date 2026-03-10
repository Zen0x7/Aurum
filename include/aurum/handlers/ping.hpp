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

#ifndef AURUM_HANDLERS_PING_HPP
#define AURUM_HANDLERS_PING_HPP

#include <aurum/state.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <aurum/protocol/exit_code.hpp>
#include <boost/core/ignore_unused.hpp>

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for ping operational codes.
     * @return A callable matching handler_type that executes the ping logic.
     */
    inline handler_type get_ping_handler() {
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, shared_session session, shared_state state) -> void {
            boost::ignore_unused(payload, session, state);

            if (type == response) {
                return;
            }

            builder.add_ping(transaction_id, success);
        };
    }
}

#endif // AURUM_HANDLERS_PING_HPP