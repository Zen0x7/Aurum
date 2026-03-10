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

#ifndef AURUM_PROTOCOL_SESSION_TYPE_HPP
#define AURUM_PROTOCOL_SESSION_TYPE_HPP

#include <cstdint>

namespace aurum::protocol {
    /**
     * @brief Defines the underlying transport protocol type used by a network session.
     */
    enum session_type : std::uint8_t {
        /** @brief Standard raw TCP transmission control protocol framing sequence natively. */
        tcp = 0,
        /** @brief Web-socket based HTTP upgraded framing layer sequentially encapsulating cleanly. */
        websocket = 1
    };
}

#endif // AURUM_PROTOCOL_SESSION_TYPE_HPP
