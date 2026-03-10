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
#include <aurum/protocol/message_type.hpp>
#include <aurum/session_container.hpp>

namespace boost::asio {
    class io_context;
}

namespace aurum {
    /**
     * Forward session
     */
    class session;

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

    /** @brief Alias for a shared pointer to a generic session. */
    using shared_session = std::shared_ptr<session>;

    /** @brief Alias for a shared pointer to the application state. */
    using shared_state = std::shared_ptr<state>;

    /** @brief Type definition for operation handlers based on opcode. */
    using handler_type = std::function<void(message_type, protocol::response_builder&, const transaction_id&, payload_buffer, shared_session, shared_state)>;

    /**
     * @brief The core application state containing sessions, configurations, and handlers.
     * @details Shared among threads, it maintains thread-safe collections and operational logic routes.
     */
    class state : public std::enable_shared_from_this<state> {
        /** @brief Global configuration struct for server parameters. */
        configuration configuration_;

        /** @brief Unique identifier representing the current running node. */
        boost::uuids::uuid node_id_;

        /** @brief Container holding all currently active network sessions. */
        session_container_t sessions_;

        /** @brief Mutex to protect concurrent access to the sessions container. */
        std::shared_mutex sessions_mutex_;

        /** @brief Array of operation handlers indexed by 8-bit opcode. */
        std::array<handler_type, 256> handlers_;

        /** @brief Reference to the main I/O execution context. */
        boost::asio::io_context& io_context_;

    public:
        /**
         * @brief Constructs a new state object and initializes default handlers.
         * @param io_context The application I/O execution context reference.
         */
        explicit state(boost::asio::io_context& io_context);

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
         * @brief Retrieves the unique identifier assigned to this node instance.
         * @return A UUID struct representing the node identity.
         */
        boost::uuids::uuid get_node_id() const;

        /**
         * @brief Retrieves a mutable reference to the active sessions container.
         * @return A reference to the sessions map.
         */
        session_container_t &get_sessions();

        /**
         * @brief Retrieves a mutable reference to the active sessions container mutex.
         * @return A reference to the sessions mutex.
         */
        std::shared_mutex &get_sessions_mutex();

        /**
         * @brief Registers a new generic session into the state container.
         * @param session A shared pointer to the newly accepted session.
         * @return true if successfully added, false if a session with the same ID already exists.
         */
        bool add_session(std::shared_ptr<session> session);

        /**
         * @brief Removes an active generic session by its unique identifier.
         * @param id The UUID of the session to terminate.
         * @return true if a session was found and removed, false otherwise.
         */
        bool remove_session(boost::uuids::uuid id);

        /**
         * @brief Establishes a synchronous outbound network connection directly towards a peer instance.
         * @param host The remote peer IP address structurally mapped string.
         * @param port The target destination listener port integer properly.
         * @param with_discovery Boolean indicating if the initial connection payload should append a discovery request.
         * @return True if connection was completely established natively securely.
         */
        bool connect(const std::string& host, unsigned short port, bool with_discovery = false);

        /**
         * @brief Terminates dynamically an active network connection linked against a specific remote node identifier correctly.
         * @param remote_node_id The 16-byte identifier representing the active node context safely.
         */
        void disconnect(boost::uuids::uuid remote_node_id);

        /**
         * @brief Clears dynamically all internal sessions cleanly structurally bounds efficiently securely mapped natively.
         */
        void disconnect_all();
    };
}

#endif // AURUM_STATE_HPP
