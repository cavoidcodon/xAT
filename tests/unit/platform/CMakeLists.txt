# Platform headers: Platform_Types.h, Std_Types.h, Compiler.h, Compiler_Cfg.h (header-only).
#
# Requirement tags: [SWS_...] = AUTOSAR SWS, [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.

set(PLATFORM_INCLUDE ${XAT_ROOT}/src/platform/include)

xat_add_unit_test(NAME test_Platform_Types SOURCES test_Platform_Types.c)
xat_add_unit_test(NAME test_Std_Types SOURCES test_Std_Types.c)
xat_add_unit_test(NAME test_Compiler SOURCES test_Compiler.c)

# [REQ-003-08] [SWS_Std_00014] [SWS_COMPILER_00047] each header alone, include guards
# [SWS_COMPILER_00004] [SWS_COMPILER_00055] Compiler.h and Compiler_Cfg.h exist under these names
# [SWS_COMPILER_00030] compiling Compiler.h passes its GCC version check (#error otherwise)
# [SWS_Platform_00054] [SWS_Std_00006] symbols that a compiler, a library or the OS defined first
xat_add_compile_check(NAME compile_check_platform
    HEADERS
        ${PLATFORM_INCLUDE}/Platform_Types.h
        ${PLATFORM_INCLUDE}/Std_Types.h
        ${PLATFORM_INCLUDE}/Compiler.h
        ${PLATFORM_INCLUDE}/Compiler_Cfg.h
    SOURCES
        compile_checks/check_TrueFalsePredefined.c
        compile_checks/check_StatusTypePredefined.c)
