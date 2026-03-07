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

#ifndef AURUM_EXIT_CODE_HPP
#define AURUM_EXIT_CODE_HPP

#include <cstdint>

namespace aurum {

    /**
     * @brief Defines exit codes for network responses.
     * @details Used by the protocol to signal the success or failure of an operation to the client.
     */
    enum exit_code : std::uint8_t {
        /** @brief Fallback exit code representing a non-implemented operation. */
        non_implemented = 0,

        /** @brief Exit code representing a successful operation. */
        success = 200
    };

}

#endif // AURUM_EXIT_CODE_HPP
