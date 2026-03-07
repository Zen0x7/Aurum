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

#ifndef AURUM_STATE_HPP
#define AURUM_STATE_HPP

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <aurum/configuration.hpp>
#include <aurum/protocol/frame_builder.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/container_hash/hash.hpp>
#include <vector>
#include <span>
#include <functional>
#include <array>

namespace aurum {
    /**
     * Forward tcp_session
     */
    class tcp_session;

    /**
     * Forward state
     */
    class state;

    /** @brief Defines the binary return type for request callbacks. */
    using callback_return_type = std::vector<std::uint8_t>;

    /** @brief Represents a unique 16-byte transaction identifier. */
    using transaction_id = boost::uuids::uuid;

    /** @brief Non-owning view over a received payload buffer. */
    using payload_buffer = std::span<const std::uint8_t>;

    /** @brief Alias for a shared pointer to a TCP session. */
    using shared_tcp_session = std::shared_ptr<tcp_session>;

    /** @brief Alias for a shared pointer to the application state. */
    using shared_state = std::shared_ptr<state>;

    /** @brief Type definition for operation handlers based on opcode. */
    using handler_type = std::function<void(aurum::protocol::response_builder&, const transaction_id&, payload_buffer, shared_tcp_session, shared_state)>;

    /** @brief Container type mapping UUIDs to active TCP sessions. */
    using sessions_container_t = std::unordered_map<
        boost::uuids::uuid,
        std::shared_ptr<tcp_session>,
        boost::hash<boost::uuids::uuid>
    >;

    /**
     * @brief The core application state containing sessions, configurations, and handlers.
     * @details Shared among threads, it maintains thread-safe collections and operational logic routes.
     */
    class state : public std::enable_shared_from_this<state> {
        /** @brief Global configuration struct for server parameters. */
        configuration configuration_;

        /** @brief Container holding all currently active TCP sessions. */
        sessions_container_t sessions_;

        /** @brief Mutex to protect concurrent access to the sessions container. */
        std::shared_mutex sessions_mutex_;

        /** @brief Array of operation handlers indexed by 8-bit opcode. */
        std::array<handler_type, 256> handlers_;

    public:
        /**
         * @brief Constructs a new state object and initializes default handlers.
         */
        state();

        /**
         * @brief Retrieves the immutable array of operation handlers.
         * @return A constant reference to the 256-element array of handler functions.
         */
        const std::array<handler_type, 256>& get_handlers() const;

        /**
         * @brief Retrieves a mutable reference to the state configuration.
         * @return A reference to the configuration struct.
         */
        configuration &get_configuration();

        /**
         * @brief Retrieves a mutable reference to the active sessions container.
         * @return A reference to the sessions map.
         */
        sessions_container_t &get_sessions();

        /**
         * @brief Registers a new TCP session into the state container.
         * @param session A shared pointer to the newly accepted session.
         * @return true if successfully added, false if a session with the same ID already exists.
         */
        bool add_session(std::shared_ptr<tcp_session> session);

        /**
         * @brief Removes an active TCP session by its unique identifier.
         * @param id The UUID of the session to terminate.
         * @return true if a session was found and removed, false otherwise.
         */
        bool remove_session(boost::uuids::uuid id);
    };
}

#endif // AURUM_STATE_HPP
