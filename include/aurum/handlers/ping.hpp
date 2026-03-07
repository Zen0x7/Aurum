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

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for ping operational codes.
     * @return A callable matching handler_type that executes the ping logic.
     */
    inline handler_type get_ping_handler() {
        // Return a lambda capturing nothing, taking the required handler_type arguments.
        return [](aurum::protocol::response_builder& _builder, const transaction_id& _transaction_id, payload_buffer _payload, shared_tcp_session _session, shared_state _state) -> void {
            // Append a successful ping response frame to the builder for the given transaction.
            _builder.add_ping(_transaction_id, aurum::exit_code::success);
        };
    }
}

#endif // AURUM_HANDLERS_PING_HPP