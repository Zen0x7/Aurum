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
#include <aurum/protocol.hpp>
#include <aurum/handlers.hpp>

#include <cstring>

namespace aurum {
    /**
     * @brief Constructs a new state object and initializes default handlers.
     */
    state::state() {
        // Fill the entire handlers array with the default non-implemented fallback.
        handlers_.fill(handlers::get_non_implemented_handler());

        // Bind opcode ping to the ping operational handler.
        handlers_[ping] = handlers::get_ping_handler();
    }

    /**
     * @brief Retrieves the immutable array of operation handlers.
     * @return A constant reference to the 256-element array of handler functions.
     */
    const std::array<handler_type, 256>& state::get_handlers() const {
        // Return a const reference to the handler array.
        return handlers_;
    }

    /**
     * @brief Retrieves a mutable reference to the state configuration.
     * @return A reference to the configuration struct.
     */
    configuration & state::get_configuration() {
        // Return a reference to the mutable server configuration structure.
        return configuration_;
    }

    /**
     * @brief Retrieves a mutable reference to the active sessions container.
     * @return A reference to the sessions map.
     */
    sessions_container_t & state::get_sessions() {
        // Return a mutable reference to the underlying sessions unordered_map.
        return sessions_;
    }

    /**
     * @brief Registers a new TCP session into the state container.
     * @param session A shared pointer to the newly accepted session.
     * @return true if successfully added, false if a session with the same ID already exists.
     */
    bool state::add_session(std::shared_ptr<tcp_session> session) {
        // Acquire an exclusive lock on the sessions container to perform thread-safe insertion.
        std::unique_lock _lock(sessions_mutex_);
        // Attempt to insert the moved session shared_ptr mapped by its UUID.
        auto [_, _inserted] = sessions_.insert({session->get_id(), std::move(session)});
        // Return whether the insertion was successful.
        return _inserted;
    }

    /**
     * @brief Removes an active TCP session by its unique identifier.
     * @param id The UUID of the session to terminate.
     * @return true if a session was found and removed, false otherwise.
     */
    bool state::remove_session(const boost::uuids::uuid id) {
        // Acquire an exclusive lock to safely modify the sessions container.
        std::unique_lock _lock(sessions_mutex_);
        // Erase the session matching the given ID and check if one was actually removed.
        return sessions_.erase(id) == 1;
    }
}
