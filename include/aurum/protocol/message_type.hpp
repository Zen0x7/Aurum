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

#ifndef AURUM_MESSAGE_TYPE_HPP
#define AURUM_MESSAGE_TYPE_HPP

#include <cstdint>

namespace aurum {

    /**
     * @brief Defines message types for network payloads.
     * @details Used by the protocol to distinguish between a request and a response.
     */
    enum message_type : std::uint8_t {
        /** @brief Message type representing a network request. */
        request = 0,
        /** @brief Message type representing a network response. */
        response = 1
    };

}

#endif // AURUM_MESSAGE_TYPE_HPP
