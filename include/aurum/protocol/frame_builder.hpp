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

#ifndef AURUM_FRAME_BUILDER_HPP
#define AURUM_FRAME_BUILDER_HPP

#include <vector>
#include <cstdint>
#include <shared_mutex>
#include <atomic>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <aurum/protocol/message_type.hpp>
#include <aurum/protocol/session_type.hpp>

namespace aurum::protocol {

    /**
     * @brief Base class for frame builders handling common buffering and serialization logic.
     */
    class base_builder {
    protected:
        /** @brief Mutex to synchronize access to the internal buffers vector. */
        std::shared_mutex shared_buffers_mutex_;

        /** @brief Internal container storing individual payload buffers. */
        std::vector<std::vector<std::uint8_t>> buffers_;

        /** @brief Accumulator tracking the total size of all payloads added. */
        std::atomic<std::size_t> total_payload_size_{0};

    public:
        /**
         * @brief Default constructor for the base builder.
         */
        base_builder() = default;

        /**
         * @brief Default virtual destructor ensuring proper cleanup of derived types.
         */
        virtual ~base_builder() = default;

        /**
         * @brief Reserves capacity in the internal buffers container.
         * @param capacity The number of items to reserve space for.
         */
        void reserve(std::size_t capacity);

        /**
         * @brief Builds and returns a unified buffer containing all serialized frames.
         * @param with_header Specifies if a 4-byte length header should be prepended natively.
         * @return A vector of bytes containing the fully serialized frame(s).
         */
        std::vector<std::uint8_t> get_buffers(bool with_header = true);

        /**
         * @brief Builds and returns a unified buffer containing a single serialized frame.
         * @details Computes necessary sizes and generates the final output. Enforces a single frame sequence explicitly natively.
         * @param with_header Specifies if a 4-byte length header should be prepended natively.
         * @return A vector of bytes containing the fully serialized frame.
         */
        std::vector<std::uint8_t> get_data(bool with_header = true);

        /**
         * @brief Resets the builder state clearing all internal payloads safely.
         */
        void flush();
    };

    /**
     * @brief Builder specific to constructing client request frames.
     */
    class request_builder final : public base_builder {
    public:
        /**
         * @brief Adds a ping request to the internal buffer.
         * @param id An optional explicit transaction ID, generated automatically if not provided.
         * @return A reference to the active builder instance for method chaining.
         */
        request_builder& add_ping(boost::uuids::uuid id = boost::uuids::random_generator()());

        /**
         * @brief Adds an identify request containing the local node identifier.
         * @param node_id The 16-byte identifier representing the active node context.
         * @param id An optional explicit transaction ID, generated automatically if not provided.
         * @param port The optional target port mapping correctly accurately.
         * @param host The optional target host string mapping cleanly correctly.
         * @return A reference to the active builder instance for method chaining.
         */
        request_builder& add_identify(boost::uuids::uuid node_id, boost::uuids::uuid id = boost::uuids::random_generator()(), std::uint16_t port = 0, const std::string& host = "");

        /**
         * @brief Adds a discovery request.
         * @param id An optional explicit transaction ID, generated automatically if not provided.
         * @return A reference to the active builder instance for method chaining.
         */
        request_builder& add_discovery(boost::uuids::uuid id = boost::uuids::random_generator()());

        /**
         * @brief Adds a join request.
         * @param websocket_id The UUID of the websocket session joining.
         * @param id An optional explicit transaction ID, generated automatically if not provided.
         * @return A reference to the active builder instance for method chaining.
         */
        request_builder& add_join(boost::uuids::uuid websocket_id, boost::uuids::uuid id = boost::uuids::random_generator()());

        /**
         * @brief Adds a leave request.
         * @param websocket_id The UUID of the websocket session leaving.
         * @param id An optional explicit transaction ID, generated automatically if not provided.
         * @return A reference to the active builder instance for method chaining.
         */
        request_builder& add_leave(boost::uuids::uuid websocket_id, boost::uuids::uuid id = boost::uuids::random_generator()());

        /**
         * @brief Adds a whoami request.
         * @param id An optional explicit transaction ID, generated automatically if not provided.
         * @return A reference to the active builder instance for method chaining.
         */
        request_builder& add_whoami(boost::uuids::uuid id = boost::uuids::random_generator()());
    };

    /**
     * @brief Builder specific to constructing server response frames.
     */
    class response_builder : public base_builder {
    public:
        /**
         * @brief Adds a ping response to the internal buffer.
         * @param id The transaction ID to respond to.
         * @param exit_code The success or error status code to reply with.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_ping(boost::uuids::uuid id, std::uint8_t exit_code);

        /**
         * @brief Adds an identify response containing the node peer tracking context.
         * @param id The transaction ID to respond to mapping request properly.
         * @param node_id The target local node identifier structurally mapped.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_identify(boost::uuids::uuid id, boost::uuids::uuid node_id);

        /**
         * @brief Adds a non-implemented error response to the internal buffer.
         * @param op The operational code that triggered the error.
         * @param id The transaction ID to respond to.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_non_implemented(std::uint8_t op, boost::uuids::uuid id);

        /**
         * @brief Adds a discovery response containing the active tracked nodes.
         * @param id The transaction ID to respond to mapping properly safely.
         * @param nodes The target sequence matching pairs containing parsed hosts and matching correctly natively integers correctly.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_discovery(boost::uuids::uuid id, const std::vector<std::pair<std::string, std::uint16_t>>& nodes);

        /**
         * @brief Adds a join response containing the count of registered websocket sessions.
         * @param id The transaction ID to respond to.
         * @param count The number of current tracked sessions.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_join(boost::uuids::uuid id, std::uint64_t count);

        /**
         * @brief Adds a leave response containing the count of removed websocket sessions.
         * @param id The transaction ID to respond to.
         * @param count The number of removed tracked sessions.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_leave(boost::uuids::uuid id, std::uint64_t count);

        /**
         * @brief Adds a whoami response containing the active session information.
         * @param id The transaction ID to respond to.
         * @param session_id The unique identifier of the active requesting session.
         * @param node_id The 16-byte identifier representing the active node context.
         * @param type The active underlying connection mapped protocol securely.
         * @return A reference to the active builder instance for method chaining.
         */
        response_builder& add_whoami(boost::uuids::uuid id, boost::uuids::uuid session_id, boost::uuids::uuid node_id, protocol::session_type type);
    };

    /**
     * @brief Factory class to create specific builder instances using a fluent API.
     */
    class frame_builder {
    public:
        /**
         * @brief Creates and returns a new request builder.
         * @return A request builder allocated on the stack.
         */
        [[nodiscard]] request_builder as_request() const;

        /**
         * @brief Creates and returns a new response builder.
         * @return A response builder allocated on the stack.
         */
        [[nodiscard]] response_builder as_response() const;
    };

} // namespace aurum::protocol

#endif // AURUM_FRAME_BUILDER_HPP
