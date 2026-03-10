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

#ifndef AURUM_SESSION_HPP
#define AURUM_SESSION_HPP

#include <memory>
#include <vector>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <aurum/protocol/session_type.hpp>

namespace aurum {
    /**
     * @brief Abstract base class representing a generic network connection session.
     * @details Defines unified methods required for identifying, managing, and transmitting.
     */
    class session {
    protected:
        /** @brief The protocol type identifying underlying transport implementation cleanly. */
        protocol::session_type type_;

    public:
        /**
         * @brief Constructs a new generic session interface mapping protocol layout accurately.
         * @param type The underlying transport protocol enumeration value clearly natively.
         */
        explicit session(protocol::session_type type) : type_(type) {}

        /**
         * @brief Destructs securely abstract generic implementation properly efficiently gracefully natively.
         */
        virtual ~session() = default;

        /**
         * @brief Initiates asynchronous operations cleanly mapping underlying protocol logic natively.
         */
        virtual void start() = 0;

        /**
         * @brief Thread-safely queues a binary message for transmission.
         * @param message A shared pointer to the payload bytes vector.
         */
        virtual void send(std::shared_ptr<const std::vector<std::uint8_t>> message) = 0;

        /**
         * @brief Gets the unique 16-byte identifier of the session.
         * @return The UUID assigned during initialization.
         */
        virtual boost::uuids::uuid get_id() const = 0;

        /**
         * @brief Gets the node identifier bound to this specific network link.
         * @return A valid UUID struct referencing the active peer node accurately.
         */
        virtual boost::uuids::uuid get_node_id() const = 0;

        /**
         * @brief Binds a remote node identifier to this currently active network session securely.
         * @param node_id The valid 16-byte node identification struct mapping.
         */
        virtual void set_node_id(boost::uuids::uuid node_id) = 0;

        /**
         * @brief Gets the node port bound to this specific network link.
         * @return A valid 16-bit integer representing the active peer node port accurately.
         */
        virtual std::uint16_t get_port() const = 0;

        /**
         * @brief Binds a remote node port to this currently active network session securely.
         * @param port The valid 16-bit integer representing the node port.
         */
        virtual void set_port(std::uint16_t port) = 0;

        /**
         * @brief Gets the node host bound to this specific network link.
         * @return A valid string representing the active peer node host accurately.
         */
        virtual std::string get_host() const = 0;

        /**
         * @brief Binds a remote node host to this currently active network session securely.
         * @param host The valid string representing the node host.
         */
        virtual void set_host(const std::string& host) = 0;

        /**
         * @brief Closes the underlying socket gracefully cleanly.
         */
        virtual void disconnect() = 0;

        /**
         * @brief Retrieves the underlying transport type defined explicitly by implementation layer natively.
         * @return The constant protocol::session_type enum value mapped correctly statically safely.
         */
        protocol::session_type get_type() const {
            return type_;
        }
    };
}

#endif // AURUM_SESSION_HPP
