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
        return [](message_type type, protocol::response_builder& builder, const transaction_id& transaction_id, payload_buffer payload, shared_session session, shared_state state) -> void {
            if (payload.size() < 16) {
                return;
            }

            boost::uuids::uuid _remote_node_id;
            std::memcpy(_remote_node_id.data, payload.data(), 16);

            session->set_node_id(_remote_node_id);

            if (payload.size() >= 18) {
                std::uint16_t _remote_port;
                std::memcpy(&_remote_port, payload.data() + 16, sizeof(_remote_port));
                boost::endian::little_to_native_inplace(_remote_port);
                session->set_port(_remote_port);

                if (payload.size() > 18) {
                    std::string _remote_host(reinterpret_cast<const char*>(payload.data() + 18), payload.size() - 18);
                    session->set_host(_remote_host);
                }
            }

            if (type == request) {
                builder.add_identify(transaction_id, state->get_node_id());
            }
        };
    }
}

#endif // AURUM_HANDLERS_IDENTIFY_HPP
