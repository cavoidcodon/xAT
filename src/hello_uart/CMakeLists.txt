# xAT hello_uart: bring-up smoke test built out-of-tree against the MCUXpresso SDK.
#
# Configure with (scripts/operations.py does this for you):
#   cmake -G Ninja -S src/hello_uart -B build/hello_uart/debug \
#     -DCMAKE_TOOLCHAIN_FILE=<sdk>/cmake/toolchain/armgcc.cmake \
#     -DSdkRootDirPath=<sdk> -Dboard=frdmmcxn947 -Dcore_id=cm33_core0 -DCMAKE_BUILD_TYPE=debug
#
# PROJECT_BOARD_PORT_PATH (relative to SdkRootDirPath) reuses the board port of the SDK
# hello_world example, so hardware_init.c (BOARD_InitHardware), pin_mux.c and app.h come
# from the SDK unchanged and nothing is copied into this directory.
cmake_minimum_required(VERSION 3.22.0)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(hello_uart LANGUAGES C CXX ASM PROJECT_BOARD_PORT_PATH ${board_root}/${board}/demo_apps/hello_world)

include(${SdkRootDirPath}/CMakeLists.txt)

mcux_add_source(
    BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES main.c
)

# SDK debug config uses "-O0 -g": add full debug info (macros) for GDB. Appended after the
# SDK flags, so it takes precedence.
mcux_add_configuration(TARGETS debug CC "-g3" TOOLCHAINS armgcc)

# Project code is C11; SDK sources keep the vendor default (-std=gnu99).
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/main.c PROPERTIES COMPILE_OPTIONS "-std=c11")

include(${SdkRootDirPath}/${board_root}/${board}/demo_apps/hello_world/reconfig.cmake OPTIONAL)

mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)
