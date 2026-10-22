# xAT host unit tests (Unity + CMock), built with the native gcc.
#
#   cmake -G Ninja -S tests/unit -B build/unit
#   cmake --build build/unit
#   ctest --test-dir build/unit --output-on-failure
#
# One sub-directory per module, mirroring src/ (see docs/unit_testing.md).
cmake_minimum_required(VERSION 3.22.0)

project(xat_unit_tests LANGUAGES C)

enable_testing()

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/xat_unit_test.cmake)

add_subdirectory(framework)
add_subdirectory(platform)
