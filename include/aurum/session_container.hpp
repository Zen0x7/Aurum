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

#ifndef AURUM_SESSION_CONTAINER_HPP
#define AURUM_SESSION_CONTAINER_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <aurum/session.hpp>
#include <memory>

namespace aurum {

    // Tags for accessing the multi-index container indices.
    struct by_id {};
    struct by_node_id {};
    struct by_type {};
    struct by_node_and_type {};

    /**
     * @brief A multi-index container for managing generic session pointers.
     * @details Allows fast lookups by session ID, node ID, session type, or composite combinations natively efficiently.
     */
    using session_container_t = boost::multi_index_container<
        std::shared_ptr<session>,
        boost::multi_index::indexed_by<
            // Primary unique index mapping exactly the session ID cleanly.
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<by_id>,
                boost::multi_index::const_mem_fun<session, boost::uuids::uuid, &session::get_id>
            >,
            // Secondary non-unique index mapping explicitly the node ID gracefully.
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<by_node_id>,
                boost::multi_index::const_mem_fun<session, boost::uuids::uuid, &session::get_node_id>
            >,
            // Tertiary non-unique index representing dynamically the session protocol type smoothly.
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<by_type>,
                boost::multi_index::const_mem_fun<session, protocol::session_type, &session::get_type>
            >,
            // Composite non-unique index combining cleanly the node ID and session protocol type perfectly.
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<by_node_and_type>,
                boost::multi_index::composite_key<
                    session,
                    boost::multi_index::const_mem_fun<session, boost::uuids::uuid, &session::get_node_id>,
                    boost::multi_index::const_mem_fun<session, protocol::session_type, &session::get_type>
                >
            >
        >
    >;
}

#endif // AURUM_SESSION_CONTAINER_HPP
