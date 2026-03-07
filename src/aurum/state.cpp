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
    /**
     * @brief Constructs a new state object and initializes default handlers.
     */
    state::state() {
        // Initialize an unhandled operation callback placeholder for undefined opcodes.
        handler_type _non_implemented = [](const transaction_id& _transaction_id, payload_buffer _payload, shared_tcp_session _session, shared_state _state) -> callback_return_type {
            // Allocate 17 bytes for the response (16 for ID, 1 for status).
            callback_return_type _response(17);
            // Copy the original transaction ID into the response buffer.
            std::memcpy(_response.data(), _transaction_id.data, 16);
            // Set the exit status to 0, representing a non-implemented operation error.
            _response[16] = 0; // Exit code 0 for non-implemented
            // Return the constructed response vector.
            return _response;
        };

        // Fill the entire handlers array with the default non-implemented fallback.
        handlers_.fill(_non_implemented);

        // Bind opcode 1 to the ping operational handler.
        handlers_[1] = [](const transaction_id& _transaction_id, payload_buffer _payload, shared_tcp_session _session, shared_state _state) -> callback_return_type {
            // Allocate 17 bytes for the ping success response.
            callback_return_type _response(17);
            // Echo the original transaction ID back in the response.
            std::memcpy(_response.data(), _transaction_id.data, 16);
            // Set the exit status to 200 to represent success.
            _response[16] = 200; // Exit code 200 for ping (success)
            // Return the ping response vector.
            return _response;
        };
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
