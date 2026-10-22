/**
 * @file    test_Compiler.c
 * @brief   Unit tests for Compiler.h / Compiler_Cfg.h (Unity, host build).
 * @details AUTOSAR Classic R21-11, AUTOSAR_SWS_CompilerAbstraction. The host build is the GCC /
 *          aarch64 pair of the compiler abstraction; the GCC / Cortex-M33 pair is compiled by
 *          every firmware build (src/ecu/main.c includes Std_Types.h).
 *
 *          Requirement tags: [SWS_COMPILER_xxxxx] = AUTOSAR_SWS_CompilerAbstraction,
 *          [SWS_Std_xxxxx] = AUTOSAR_SWS_StandardTypes, [REQ-003-xx] = .claude/tasks/TASK-003.md.
 */
#include "unity.h"
#include "Compiler.h"
#include "Platform_Types.h"

/* Expansion of a macro as a string literal (a function cannot do this). */
#define TEST_STRINGIFY(x)        #x
#define TEST_EXPANSION_OF(macro) TEST_STRINGIFY(macro)

/* Inline helper with static scope, as a module would define it in a header. */
LOCAL_INLINE uint8 Test_IncrementLocal(uint8 value)
{
    return (uint8)(value + 1U);
}

/* Inline function with external linkage: the extern declaration below makes this translation
 * unit hold the one external definition that C11 requires. */
INLINE uint8 Test_IncrementExtern(uint8 value)
{
    return (uint8)(value + 2U);
}
extern uint8 Test_IncrementExtern(uint8 value);

void setUp(void)
{
}

void tearDown(void)
{
}

/*==================================================================================================
 *                                   GENERAL DEFINITIONS
 *================================================================================================*/
/* [SWS_Std_00031] [REQ-003-07] NULL_PTR is a void pointer to zero */
void test_NullPtr_Definition_IsVoidPointerToZero(void)
{
    TEST_ASSERT_NULL(NULL_PTR);
    TEST_ASSERT_EQUAL_INT(1, _Generic(NULL_PTR, void *: 1, default: 0));
    TEST_ASSERT_EQUAL_STRING("((void *)0)", TEST_EXPANSION_OF(NULL_PTR));
}

/* [SWS_COMPILER_00060] [REQ-003-07] LOCAL_INLINE = static inline, usable in C11 */
void test_LocalInline_HelperFunction_ExpandsToStaticInlineAndRuns(void)
{
    TEST_ASSERT_EQUAL_STRING("static inline", TEST_EXPANSION_OF(LOCAL_INLINE));
    TEST_ASSERT_EQUAL_UINT8(6U, Test_IncrementLocal(5U));
}

/* [SWS_COMPILER_00057] [REQ-003-07] INLINE = inline, usable in C11 */
void test_Inline_ExternFunction_ExpandsToInlineAndRuns(void)
{
    TEST_ASSERT_EQUAL_STRING("inline", TEST_EXPANSION_OF(INLINE));
    TEST_ASSERT_EQUAL_UINT8(7U, Test_IncrementExtern(5U));
}

/* [SWS_COMPILER_00046] [SWS_COMPILER_00059] [SWS_COMPILER_00005] memory classes without effect */
void test_MemoryClasses_AutomaticAndTypedef_ExpandToEmpty(void)
{
    uint8 AUTOMATIC local_value = 3U;

    TEST_ASSERT_EQUAL_STRING("", TEST_EXPANSION_OF(AUTOMATIC));
    TEST_ASSERT_EQUAL_STRING("", TEST_EXPANSION_OF(TYPEDEF));
    TEST_ASSERT_EQUAL_UINT8(3U, local_value);
}

/*==================================================================================================
 *                                  COMPILER IDENTIFICATION
 *================================================================================================*/
/* [SWS_COMPILER_00003] [SWS_COMPILER_00010] the host build selects the GCC / aarch64 pair only.
 * MISRA C:2012 Rule 21.1 deviation: SWS spelling of the compiler symbols (see Compiler.h). */
void test_CompilerSymbol_HostBuild_GccAarch64Only(void)
{
#if defined(_GCC_C_AARCH64_) && !defined(_GCC_C_CORTEXM33_)
    TEST_ASSERT_EQUAL_STRING("", TEST_EXPANSION_OF(_GCC_C_AARCH64_));
#else
    TEST_FAIL_MESSAGE("host build must define _GCC_C_AARCH64_ and not _GCC_C_CORTEXM33_");
#endif
}

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
/* [REQ-003-06] AR release 4.7.0, SW version 1.0.0, module ID 198, unregistered vendor */
void test_PublishedInfo_Compiler_Ar470Sw100(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, COMPILER_VENDOR_ID);
    TEST_ASSERT_EQUAL_UINT16(198U, COMPILER_MODULE_ID);
    TEST_ASSERT_EQUAL_UINT8(4U, COMPILER_AR_RELEASE_MAJOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(7U, COMPILER_AR_RELEASE_MINOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, COMPILER_AR_RELEASE_REVISION_VERSION);
    TEST_ASSERT_EQUAL_UINT8(1U, COMPILER_SW_MAJOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, COMPILER_SW_MINOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, COMPILER_SW_PATCH_VERSION);
}
