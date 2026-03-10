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

#ifndef AURUM_TEST_UTILS_HPP
#define AURUM_TEST_UTILS_HPP

#include <vector>
#include <cstdint>
#include <cstring>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>

namespace aurum::test_utils {

    /**
     * Extracts a 16-bit unsigned integer from a byte buffer at the specified offset,
     * converting it from little-endian to native endianness.
     *
     * @param buffer The input byte buffer.
     * @param offset The starting index to read from.
     * @return The parsed 16-bit integer.
     */
    inline std::uint16_t read_uint16_le(const std::vector<std::uint8_t>& buffer, std::size_t offset) {
        std::uint16_t _value;
        std::memcpy(&_value, buffer.data() + offset, sizeof(_value));
        boost::endian::little_to_native_inplace(_value);
        return _value;
    }

    /**
     * Extracts a 32-bit unsigned integer from a byte buffer at the specified offset,
     * converting it from little-endian to native endianness.
     *
     * @param buffer The input byte buffer.
     * @param offset The starting index to read from.
     * @return The parsed 32-bit integer.
     */
    inline std::uint32_t read_uint32_le(const std::vector<std::uint8_t>& buffer, std::size_t offset) {
        std::uint32_t _value;
        std::memcpy(&_value, buffer.data() + offset, sizeof(_value));
        boost::endian::little_to_native_inplace(_value);
        return _value;
    }

    /**
     * Extracts a 16-byte UUID from a byte buffer at the specified offset.
     *
     * @param buffer The input byte buffer.
     * @param offset The starting index to read from.
     * @return The parsed UUID.
     */
    inline boost::uuids::uuid read_uuid(const std::vector<std::uint8_t>& buffer, std::size_t offset) {
        boost::uuids::uuid _uuid;
        std::memcpy(_uuid.data, buffer.data() + offset, 16);
        return _uuid;
    }

} // namespace aurum::test_utils

#endif // AURUM_TEST_UTILS_HPP