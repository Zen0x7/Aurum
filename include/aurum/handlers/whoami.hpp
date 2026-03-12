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

#ifndef AURUM_HANDLERS_WHOAMI_HPP
#define AURUM_HANDLERS_WHOAMI_HPP

#include <aurum/state.hpp>
#include <aurum/session.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <boost/core/ignore_unused.hpp>

namespace aurum::handlers {
    /**
     * @brief Generates the handler callback for whoami operational codes.
     * @return A callable matching handler_type that executes the whoami logic natively correctly safely.
     */
    inline handler_type get_whoami_handler() {
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, const shared_session& session, const shared_state& state) -> void {
            boost::ignore_unused(payload);

            if (type == response) {
                return;
            }

            builder.add_whoami(transaction_id, session->get_id(), state->get_node_id(), session->get_type());
        };
    }
}

#endif // AURUM_HANDLERS_WHOAMI_HPP