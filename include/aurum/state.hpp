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
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/container_hash/hash.hpp>
#include <vector>
#include <span>
#include <functional>
#include <array>

namespace aurum {
    class tcp_session;
    class state;

    using callback_return_type = std::vector<std::uint8_t>;
    using transaction_id = boost::uuids::uuid;
    using payload_buffer = std::span<const std::uint8_t>;
    using shared_tcp_session = std::shared_ptr<tcp_session>;
    using shared_state = std::shared_ptr<state>;
    using handler_type = std::function<callback_return_type(const transaction_id&, payload_buffer, shared_tcp_session, shared_state)>;

    using sessions_container_t = std::unordered_map<
        boost::uuids::uuid,
        std::shared_ptr<tcp_session>,
        boost::hash<boost::uuids::uuid>
    >;

    class state : public std::enable_shared_from_this<state> {
        configuration configuration_;

        sessions_container_t sessions_;
        std::shared_mutex sessions_mutex_;

        std::array<handler_type, 256> handlers_;

    public:
        state();

        const std::array<handler_type, 256>& get_handlers() const;

        configuration &get_configuration();

        sessions_container_t &get_sessions();

        bool add_session(std::shared_ptr<tcp_session> session);

        bool remove_session(boost::uuids::uuid id);
    };
}

#endif // AURUM_STATE_HPP
