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
option(BUILD_BENCHMARK "Enable building of benchmark" ON)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_PROFILE "Enable Profiling (gprof / debug symbols)" OFF)

if(ENABLE_PROFILE)
    message(STATUS "Profiling enabled: adding -g and -pg flags")
    add_compile_options(-g -pg)
    add_link_options(-pg)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build: enabling O3 + native + LTO")

    add_compile_options(
            -O3
            -march=native
            -flto
    )
    add_link_options(
            -flto
    )
endif()

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASAN and TSAN cannot be enabled together")
endif()

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

    if(ENABLE_ASAN)
        message(STATUS "AddressSanitizer enabled")
        target_compile_options(tests PRIVATE
                -O1
                -g
                -fno-omit-frame-pointer
                -fsanitize=address,undefined,leak
        )
        target_link_options(tests PRIVATE
                -fsanitize=address,undefined,leak
        )
    endif()

    if(ENABLE_TSAN)
        message(STATUS "ThreadSanitizer enabled")
        target_compile_options(tests PRIVATE
                -O1
                -g
                -fno-omit-frame-pointer
                -fsanitize=thread
        )
        target_link_options(tests PRIVATE
                -fsanitize=thread)
    endif()
endif ()

if(BUILD_BENCHMARK)
    FetchContent_Declare(
            googlebenchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.9.1
    )
    # Disable building tests for Google Benchmark itself to save time
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)

    file(GLOB_RECURSE BENCHMARK_OBJECTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/*.cc")

    add_executable(benchmarks ${BENCHMARK_OBJECTS} ${AURUM_SOURCES})
    target_link_libraries(benchmarks PRIVATE ${Boost_LIBRARIES} benchmark::benchmark benchmark::benchmark_main)
    target_include_directories(benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

add_executable(aurum main.cpp ${AURUM_SOURCES})
target_link_libraries(aurum ${Boost_LIBRARIES})
