# xAT host unit test helpers: Unity + CMock (TASK-003).
#
# Included by tests/unit/CMakeLists.txt. Needs Ruby (CMock and the Unity runner generator).
#
#   xat_add_unit_test(NAME <test>
#                     SOURCES <module sources + one test_*.c file>
#                     [MOCKS <headers>] [INCLUDES <dirs>] [DEFINES <defs>]
#                     [PASS_REGEX <regex>])
#       Generates a CMock mock (mock_<header>.c/.h) for each header in MOCKS and a Unity
#       runner for the test_*.c file, builds one executable and registers it with CTest.
#       PASS_REGEX is only for guard tests that must fail: CTest then passes the test when its
#       output matches the regular expression, whatever the exit code.
#
#   xat_add_compile_check(NAME <check> [HEADERS <headers>] [SOURCES <files>] [INCLUDES <dirs>])
#       Compile-only checks, part of the normal build (a failure stops the build):
#       - each header in HEADERS is compiled alone, included twice, and its include guard
#         <FILE_NAME>_H must be defined (coding_rules.md 4.1, 4.2);
#       - each file in SOURCES is compiled as written (e.g. a predefined-symbol case).
#
# All test code, module sources, generated mocks and runners use XAT_PROJECT_C_FLAGS, the same
# strict flags as the firmware. Unity and CMock themselves keep their vendor defaults.
#
# Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.

include_guard(GLOBAL)

get_filename_component(XAT_ROOT ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)

# XAT_PROJECT_C_FLAGS: one definition for the firmware and the host tests.
include(${XAT_ROOT}/cmake/xat_module.cmake)

set(XAT_UNITY_DIR ${XAT_ROOT}/third_party/unity)
set(XAT_CMOCK_DIR ${XAT_ROOT}/third_party/cmock)
set(XAT_CMOCK_CONFIG ${XAT_ROOT}/tests/unit/cmock.yml)
set(XAT_PLATFORM_INCLUDE ${XAT_ROOT}/src/platform/include)

find_program(XAT_RUBY ruby REQUIRED)

# Unity + CMock runtime, shared by every test executable.
if(NOT TARGET xat_unity_cmock)
    add_library(xat_unity_cmock STATIC
        ${XAT_UNITY_DIR}/src/unity.c
        ${XAT_CMOCK_DIR}/src/cmock.c)
    target_include_directories(xat_unity_cmock PUBLIC ${XAT_UNITY_DIR}/src ${XAT_CMOCK_DIR}/src)
    # 64-bit assertions for uint64 / sint64
    target_compile_definitions(xat_unity_cmock PUBLIC UNITY_SUPPORT_64)
    target_compile_options(xat_unity_cmock PRIVATE -std=c11)
endif()

# [REQ-003-10] adding a module test with mocked dependencies takes one call
function(xat_add_unit_test)
    cmake_parse_arguments(ARG "" "NAME;PASS_REGEX" "SOURCES;MOCKS;INCLUDES;DEFINES" ${ARGN})
    if(NOT ARG_NAME OR NOT ARG_SOURCES)
        message(FATAL_ERROR "xat_add_unit_test: NAME and SOURCES are required")
    endif()

    set(sources "")
    set(test_file "")
    foreach(src IN LISTS ARG_SOURCES)
        get_filename_component(src_abs ${src} ABSOLUTE)
        list(APPEND sources ${src_abs})
        get_filename_component(src_name ${src_abs} NAME)
        if(src_name MATCHES "^test_.*\\.c$")
            if(test_file)
                message(FATAL_ERROR "xat_add_unit_test(${ARG_NAME}): more than one test_*.c file")
            endif()
            set(test_file ${src_abs})
        endif()
    endforeach()
    if(NOT test_file)
        message(FATAL_ERROR "xat_add_unit_test(${ARG_NAME}): no test_*.c file in SOURCES")
    endif()

    # One mock directory per test, so that two tests can mock the same header.
    set(mock_dir ${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}_mocks)
    set(mock_sources "")
    set(mocked_header_dirs "")
    foreach(header IN LISTS ARG_MOCKS)
        get_filename_component(header_abs ${header} ABSOLUTE)
        get_filename_component(header_name ${header_abs} NAME_WE)
        get_filename_component(header_dir ${header_abs} DIRECTORY)
        set(mock_c ${mock_dir}/mock_${header_name}.c)
        add_custom_command(
            OUTPUT ${mock_c} ${mock_dir}/mock_${header_name}.h
            COMMAND ${CMAKE_COMMAND} -E make_directory ${mock_dir}
            COMMAND ${XAT_RUBY} ${XAT_CMOCK_DIR}/lib/cmock.rb -o${XAT_CMOCK_CONFIG}
                    --mock_path=${mock_dir} ${header_abs}
            DEPENDS ${header_abs} ${XAT_CMOCK_CONFIG}
            COMMENT "CMock: mock_${header_name} (${ARG_NAME})"
            VERBATIM)
        list(APPEND mock_sources ${mock_c})
        list(APPEND mocked_header_dirs ${header_dir})
    endforeach()
    list(REMOVE_DUPLICATES mocked_header_dirs)

    set(runner ${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}_runner.c)
    add_custom_command(
        OUTPUT ${runner}
        COMMAND ${XAT_RUBY} ${XAT_UNITY_DIR}/auto/generate_test_runner.rb ${XAT_CMOCK_CONFIG}
                ${test_file} ${runner}
        DEPENDS ${test_file} ${XAT_CMOCK_CONFIG}
        COMMENT "Unity runner: ${ARG_NAME}"
        VERBATIM)

    add_executable(${ARG_NAME} ${sources} ${mock_sources} ${runner})
    target_include_directories(${ARG_NAME} PRIVATE
        ${mock_dir} ${ARG_INCLUDES} ${mocked_header_dirs} ${XAT_PLATFORM_INCLUDE})
    target_compile_definitions(${ARG_NAME} PRIVATE ${ARG_DEFINES})
    # [REQ-003-08] host tests use the same strict flags as the target
    target_compile_options(${ARG_NAME} PRIVATE ${XAT_PROJECT_C_FLAGS})
    target_link_libraries(${ARG_NAME} PRIVATE xat_unity_cmock)

    # [REQ-003-11] the Unity runner returns the number of failures: CTest fails on any failure
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    if(ARG_PASS_REGEX)
        set_tests_properties(${ARG_NAME} PROPERTIES PASS_REGULAR_EXPRESSION "${ARG_PASS_REGEX}")
    endif()
endfunction()

# [REQ-003-08] every header compiles alone and is protected by its include guard
function(xat_add_compile_check)
    cmake_parse_arguments(ARG "" "NAME" "HEADERS;SOURCES;INCLUDES" ${ARGN})
    if(NOT ARG_NAME OR (NOT ARG_HEADERS AND NOT ARG_SOURCES))
        message(FATAL_ERROR "xat_add_compile_check: NAME and HEADERS or SOURCES are required")
    endif()

    set(check_dir ${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME})
    set(sources "")
    set(header_dirs "")
    foreach(header IN LISTS ARG_HEADERS)
        get_filename_component(header_abs ${header} ABSOLUTE)
        get_filename_component(header_file ${header_abs} NAME)
        get_filename_component(header_name ${header_abs} NAME_WE)
        get_filename_component(header_dir ${header_abs} DIRECTORY)
        string(TOUPPER "${header_file}" guard)
        string(REPLACE "." "_" guard "${guard}")
        set(check_c ${check_dir}/check_${header_name}.c)
        # The typedef keeps the translation unit non-empty for headers with macros only
        # (-Wpedantic: "ISO C forbids an empty translation unit").
        file(WRITE ${check_c}.tmp
            "/* Generated by xat_add_compile_check: ${header_file} compiles on its own. */\n"
            "#include \"${header_file}\"\n"
            "#include \"${header_file}\" /* second inclusion must be a no-op */\n"
            "#ifndef ${guard}\n"
            "#error \"${header_file}: include guard ${guard} is missing\"\n"
            "#endif\n"
            "typedef unsigned char CompileCheck_${header_name}Type;\n")
        configure_file(${check_c}.tmp ${check_c} COPYONLY)
        list(APPEND sources ${check_c})
        list(APPEND header_dirs ${header_dir})
    endforeach()
    foreach(src IN LISTS ARG_SOURCES)
        get_filename_component(src_abs ${src} ABSOLUTE)
        list(APPEND sources ${src_abs})
    endforeach()
    list(REMOVE_DUPLICATES header_dirs)

    add_library(${ARG_NAME} OBJECT ${sources})
    target_include_directories(${ARG_NAME} PRIVATE ${ARG_INCLUDES} ${header_dirs} ${XAT_PLATFORM_INCLUDE})
    target_compile_options(${ARG_NAME} PRIVATE ${XAT_PROJECT_C_FLAGS})
endfunction()
