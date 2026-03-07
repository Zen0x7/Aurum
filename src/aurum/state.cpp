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

#include <aurum/state.hpp>

#include <aurum/tcp_session.hpp>

#include <cstring>

namespace aurum {
    state::state() {
        handler_type _non_implemented = [](const transaction_id& _transaction_id, payload_buffer _payload, shared_tcp_session _session, shared_state _state) -> callback_return_type {
            callback_return_type _response(17);
            std::memcpy(_response.data(), _transaction_id.data, 16);
            _response[16] = 0; // Exit code 0 for non-implemented
            return _response;
        };

        handlers_.fill(_non_implemented);

        handlers_[1] = [](const transaction_id& _transaction_id, payload_buffer _payload, shared_tcp_session _session, shared_state _state) -> callback_return_type {
            callback_return_type _response(17);
            std::memcpy(_response.data(), _transaction_id.data, 16);
            _response[16] = 200; // Exit code 200 for ping (success)
            return _response;
        };
    }

    const std::array<handler_type, 256>& state::get_handlers() const {
        return handlers_;
    }

    configuration & state::get_configuration() { return configuration_; }

    sessions_container_t & state::get_sessions() {
        return sessions_;
    }

    bool state::add_session(std::shared_ptr<tcp_session> session) {
        std::unique_lock _lock(sessions_mutex_);
        auto [_, _inserted] = sessions_.insert({session->get_id(), std::move(session)});
        return _inserted;
    }

    bool state::remove_session(const boost::uuids::uuid id) {
        std::unique_lock _lock(sessions_mutex_);
        return sessions_.erase(id) == 1;
    }
}
