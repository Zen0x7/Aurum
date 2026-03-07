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

#include <gtest/gtest.h>

/**
 * @brief Simple baseline sanity check for the testing framework environment.
 * @details Validates properly that standard library fixed-width integer assertions work correctly natively.
 */
TEST(A, B) {
    // Assert 8-bit integers identity successfully
    ASSERT_EQ(std::uint8_t{1}, std::uint8_t{1});

    // Assert 16-bit integers identity successfully
    ASSERT_EQ(std::uint16_t{1}, std::uint16_t{1});

    // Assert 32-bit integers identity successfully
    ASSERT_EQ(std::uint32_t{1}, std::uint32_t{1});

    // Assert 64-bit integers identity successfully
    ASSERT_EQ(std::uint64_t{1}, std::uint64_t{1});
}