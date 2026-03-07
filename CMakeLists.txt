# Aurum
# Copyright (C) 2026 Ian Torres
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.28)
project(aurum)

set(CMAKE_CXX_STANDARD 20)
option(BUILD_TESTS "Enable tests" ON)

find_package(Boost 1.90.0 REQUIRED COMPONENTS json program_options charconv)

include_directories(include)
include(FetchContent)
file(GLOB_RECURSE AURUM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(BUILD_TESTS)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
    enable_testing()
    file(GLOB_RECURSE AURUM_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc")

    add_executable(tests ${AURUM_TESTS} ${AURUM_SOURCES})
    target_link_libraries(tests PRIVATE GTest::gtest_main ${Boost_LIBRARIES})
    target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    gtest_discover_tests(tests)
endif ()

add_executable(aurum main.cpp ${AURUM_SOURCES})
target_link_libraries(aurum ${Boost_LIBRARIES})
