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

#include <aurum/node.hpp>
#include <memory>

/**
 * @brief Application entry point.
 * @param argc The count of command line arguments.
 * @param argv An array of command line arguments strings.
 * @return Exit status code of the application.
 */
int main(int argc, char *argv[]) {
    using namespace aurum;

    auto _node = std::make_unique<node>();

    _node->parse_args(argc, argv);

    return _node->run();
}
