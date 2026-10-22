# xAT firmware module helpers (TASK-003).
#
# Included by src/ecu/CMakeLists.txt after the SDK CMakeLists.txt, so that the SDK functions
# mcux_add_source() / mcux_add_include() exist and the application target is defined.
#
#   xat_add_module(NAME <Ma> DIR <path>)
#       Adds <path>/src/*.c and <path>/config/*.c as sources and <path>/include and
#       <path>/config (those that exist) as include directories. A module without .c files
#       (e.g. Platform) is header-only: only its include directories are added.
#
#   xat_add_project_sources(SOURCES <files> [BASE_PATH <dir>])
#       Adds project sources that are not in a module directory (e.g. src/ecu/main.c).
#
# Both apply XAT_PROJECT_C_FLAGS to the project sources only; SDK sources keep the vendor
# flags (-std=gnu99 ...).
#
# Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.

include_guard(GLOBAL)

# [REQ-003-08] [REQ-003-10] project compile flags (coding_rules.md 9.1). As source file options
# they come after the SDK target flags, so -std=c11 overrides the SDK's -std=gnu99.
set(XAT_PROJECT_C_FLAGS -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)

# [REQ-003-10] add project sources with the project compile flags
function(xat_add_project_sources)
    cmake_parse_arguments(ARG "" "BASE_PATH" "SOURCES" ${ARGN})
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "xat_add_project_sources: SOURCES is empty")
    endif()
    if(NOT ARG_BASE_PATH)
        set(ARG_BASE_PATH ${CMAKE_CURRENT_LIST_DIR})
    endif()

    mcux_add_source(BASE_PATH ${ARG_BASE_PATH} SOURCES ${ARG_SOURCES})

    foreach(src IN LISTS ARG_SOURCES)
        get_filename_component(src_abs ${ARG_BASE_PATH}/${src} ABSOLUTE)
        set_source_files_properties(${src_abs} PROPERTIES COMPILE_OPTIONS "${XAT_PROJECT_C_FLAGS}")
    endforeach()
endfunction()

# [REQ-003-10] adding a module to the firmware takes one call
function(xat_add_module)
    cmake_parse_arguments(ARG "" "NAME;DIR" "" ${ARGN})
    if(NOT ARG_NAME OR NOT ARG_DIR)
        message(FATAL_ERROR "xat_add_module: NAME and DIR are required")
    endif()
    get_filename_component(module_dir ${ARG_DIR} ABSOLUTE)
    if(NOT IS_DIRECTORY ${module_dir})
        message(FATAL_ERROR "xat_add_module(${ARG_NAME}): ${module_dir} is not a directory")
    endif()

    set(include_dirs "")
    foreach(sub IN ITEMS include config)
        if(IS_DIRECTORY ${module_dir}/${sub})
            list(APPEND include_dirs ${sub})
        endif()
    endforeach()
    if(include_dirs)
        mcux_add_include(BASE_PATH ${module_dir} INCLUDES ${include_dirs})
    endif()

    # CONFIGURE_DEPENDS: a new .c file in the module is picked up without a manual re-configure
    file(GLOB module_sources RELATIVE ${module_dir} CONFIGURE_DEPENDS
         ${module_dir}/src/*.c ${module_dir}/config/*.c)
    list(SORT module_sources)
    if(module_sources)
        xat_add_project_sources(BASE_PATH ${module_dir} SOURCES ${module_sources})
    endif()

    list(LENGTH module_sources source_count)
    message(STATUS "xAT module ${ARG_NAME}: ${source_count} source(s), include: ${include_dirs}")
endfunction()
