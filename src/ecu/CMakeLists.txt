# xAT firmware entry (FRDM-MCXN947, cm33_core0), built out of tree against the MCUXpresso SDK.
#
# Configure with (scripts/operations.py does this for you):
#   cmake -G Ninja -S src/ecu -B build/ecu/debug \
#     -DCMAKE_TOOLCHAIN_FILE=<sdk>/cmake/toolchain/armgcc.cmake \
#     -DSdkRootDirPath=<sdk> -Dboard=frdmmcxn947 -Dcore_id=cm33_core0 -DCMAKE_BUILD_TYPE=debug
#
# PROJECT_BOARD_PORT_PATH (relative to SdkRootDirPath) reuses the board port of the SDK
# hello_world example (BOARD_InitHardware: clocks, pins, debug console), so hardware_init.c,
# pin_mux.c, board.c, clock_config.c and app.h come from the SDK unchanged and nothing is
# copied. This is a temporary integration shortcut until Mcu / Port take over (TASK-005/006).
#
# Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
cmake_minimum_required(VERSION 3.22.0)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

# [REQ-003-09] the image is named <project>_<core_id>: ecu_cm33_core0.elf
project(ecu LANGUAGES C CXX ASM PROJECT_BOARD_PORT_PATH ${board_root}/${board}/demo_apps/hello_world)

include(${SdkRootDirPath}/CMakeLists.txt)

get_filename_component(XAT_ROOT ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
include(${XAT_ROOT}/cmake/xat_module.cmake)

# [REQ-003-10] one line per module
xat_add_module(NAME Platform DIR ${XAT_ROOT}/src/platform)

xat_add_project_sources(SOURCES main.c)

# SDK debug config uses "-O0 -g": add full debug info (macros) for GDB. Appended after the
# SDK flags, so it takes precedence.
mcux_add_configuration(TARGETS debug CC "-g3" TOOLCHAINS armgcc)

include(${SdkRootDirPath}/${board_root}/${board}/demo_apps/hello_world/reconfig.cmake OPTIONAL)

mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)
