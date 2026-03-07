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

TEST(A, B) {
    ASSERT_EQ(std::uint8_t{1}, std::uint8_t{1});
    ASSERT_EQ(std::uint16_t{1}, std::uint16_t{1});
    ASSERT_EQ(std::uint32_t{1}, std::uint32_t{1});
    ASSERT_EQ(std::uint64_t{1}, std::uint64_t{1});
}