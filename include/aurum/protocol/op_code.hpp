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

#ifndef AURUM_OP_CODE_HPP
#define AURUM_OP_CODE_HPP

#include <cstdint>

namespace aurum {

    /**
     * @brief Defines operational codes for network requests.
     * @details Used by the protocol to map incoming frames to their respective state handlers.
     */
    enum op_code : std::uint8_t {
        /** @brief Operational code representing an unsupported operation request mapping properly. */
        op_non_implemented = 0,
        /** @brief Operational code representing a network ping request. */
        ping = 1,
        /** @brief Operational code for identifying network peer instances dynamically. */
        identify = 2,
        /** @brief Operational code for discovering connected network nodes. */
        discovery = 3
    };

}

#endif // AURUM_OP_CODE_HPP
