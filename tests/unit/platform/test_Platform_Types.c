/**
 * @file    test_Platform_Types.c
 * @brief   Unit tests for Platform_Types.h (Unity, host build).
 * @details AUTOSAR Classic R21-11, AUTOSAR_SWS_PlatformTypes. The host is LP64 (aarch64); the
 *          same widths are checked for the ILP32 target by the static assertions in
 *          src/ecu/main.c at every firmware build.
 *
 *          Requirement tags: [SWS_Platform_xxxxx] = AUTOSAR_SWS_PlatformTypes,
 *          [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
 */
#include <float.h>
#include <limits.h>
#include <string.h>

#include "unity.h"
#include "Platform_Types.h"

/* Number of bits of an object of the given type. */
#define BITS_OF(type) ((uint32)(sizeof(type) * (uint32)CHAR_BIT))

void setUp(void)
{
}

void tearDown(void)
{
}

/*==================================================================================================
 *                                    UNSIGNED INTEGER TYPES
 *================================================================================================*/
/* [SWS_Platform_00013] [REQ-003-01] */
void test_Uint8_WidthAndRange_Is8BitUnsigned(void)
{
    TEST_ASSERT_EQUAL_UINT32(8U, BITS_OF(uint8));
    TEST_ASSERT_EQUAL_HEX8(0xFFU, (uint8)~(uint8)0U);
    TEST_ASSERT_TRUE((uint8)-1 > (uint8)0U);
}

/* [SWS_Platform_00014] [REQ-003-01] */
void test_Uint16_WidthAndRange_Is16BitUnsigned(void)
{
    TEST_ASSERT_EQUAL_UINT32(16U, BITS_OF(uint16));
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, (uint16)~(uint16)0U);
    TEST_ASSERT_TRUE((uint16)-1 > (uint16)0U);
}

/* [SWS_Platform_00015] [REQ-003-01] */
void test_Uint32_WidthAndRange_Is32BitUnsigned(void)
{
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(uint32));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFUL, (uint32)~(uint32)0U);
    TEST_ASSERT_TRUE((uint32)-1 > (uint32)0U);
}

/* [SWS_Platform_00066] [REQ-003-01] */
void test_Uint64_WidthAndRange_Is64BitUnsigned(void)
{
    TEST_ASSERT_EQUAL_UINT32(64U, BITS_OF(uint64));
    TEST_ASSERT_EQUAL_HEX64(0xFFFFFFFFFFFFFFFFULL, (uint64)~(uint64)0U);
    TEST_ASSERT_TRUE((uint64)-1 > (uint64)0U);
}

/*==================================================================================================
 *                                     SIGNED INTEGER TYPES
 *================================================================================================*/
/* [SWS_Platform_00016] [REQ-003-01] */
void test_Sint8_WidthAndRange_Is8BitSigned(void)
{
    sint8 min_value = (sint8)(-128);
    sint8 max_value = (sint8)127;

    TEST_ASSERT_EQUAL_UINT32(8U, BITS_OF(sint8));
    TEST_ASSERT_EQUAL_INT8(-128, min_value);
    TEST_ASSERT_EQUAL_INT8(127, max_value);
    TEST_ASSERT_TRUE((sint8)-1 < (sint8)0);
}

/* [SWS_Platform_00017] [REQ-003-01] */
void test_Sint16_WidthAndRange_Is16BitSigned(void)
{
    sint16 min_value = (sint16)(-32768);
    sint16 max_value = (sint16)32767;

    TEST_ASSERT_EQUAL_UINT32(16U, BITS_OF(sint16));
    TEST_ASSERT_EQUAL_INT16(-32768, min_value);
    TEST_ASSERT_EQUAL_INT16(32767, max_value);
    TEST_ASSERT_TRUE((sint16)-1 < (sint16)0);
}

/* [SWS_Platform_00018] [REQ-003-01] */
void test_Sint32_WidthAndRange_Is32BitSigned(void)
{
    sint32 min_value = (sint32)(-2147483647L - 1L);
    sint32 max_value = (sint32)2147483647L;

    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(sint32));
    TEST_ASSERT_EQUAL_INT32(-2147483647L - 1L, min_value);
    TEST_ASSERT_EQUAL_INT32(2147483647L, max_value);
    TEST_ASSERT_TRUE((sint32)-1 < (sint32)0);
}

/* [SWS_Platform_00067] [REQ-003-01] */
void test_Sint64_WidthAndRange_Is64BitSigned(void)
{
    sint64 min_value = (sint64)(-9223372036854775807LL - 1LL);
    sint64 max_value = (sint64)9223372036854775807LL;

    TEST_ASSERT_EQUAL_UINT32(64U, BITS_OF(sint64));
    TEST_ASSERT_EQUAL_INT64(-9223372036854775807LL - 1LL, min_value);
    TEST_ASSERT_EQUAL_INT64(9223372036854775807LL, max_value);
    TEST_ASSERT_TRUE((sint64)-1 < (sint64)0);
}

/* [SWS_Platform_00061] the bit pattern of -1 is all ones only in two's complement */
void test_SignedTypes_Representation_IsTwosComplement(void)
{
    sint8 minus_one_8 = (sint8)-1;
    sint32 minus_one_32 = (sint32)-1;
    uint8 bits_8 = 0U;
    uint32 bits_32 = 0U;

    (void)memcpy(&bits_8, &minus_one_8, sizeof(bits_8));
    (void)memcpy(&bits_32, &minus_one_32, sizeof(bits_32));

    TEST_ASSERT_EQUAL_HEX8(0xFFU, bits_8);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFUL, bits_32);
}

/*==================================================================================================
 *                                  OPTIMIZED INTEGER TYPES
 *================================================================================================*/
/* [SWS_Platform_00020] [SWS_Platform_00021] [SWS_Platform_00022] at least 8 / 16 / 32 bit
 * unsigned; xAT uses the 32-bit register width for all of them */
void test_UnsignedLeastTypes_Width_AtLeastRequiredAnd32Bit(void)
{
    TEST_ASSERT_TRUE(BITS_OF(uint8_least) >= 8U);
    TEST_ASSERT_TRUE(BITS_OF(uint16_least) >= 16U);
    TEST_ASSERT_TRUE(BITS_OF(uint32_least) >= 32U);
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(uint8_least));
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(uint16_least));
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(uint32_least));
    TEST_ASSERT_TRUE((uint8_least)-1 > (uint8_least)0U);
    TEST_ASSERT_TRUE((uint16_least)-1 > (uint16_least)0U);
    TEST_ASSERT_TRUE((uint32_least)-1 > (uint32_least)0U);
}

/* [SWS_Platform_00023] [SWS_Platform_00024] [SWS_Platform_00025] at least 8 / 16 / 32 bit signed */
void test_SignedLeastTypes_Width_AtLeastRequiredAnd32Bit(void)
{
    TEST_ASSERT_TRUE(BITS_OF(sint8_least) >= 8U);
    TEST_ASSERT_TRUE(BITS_OF(sint16_least) >= 16U);
    TEST_ASSERT_TRUE(BITS_OF(sint32_least) >= 32U);
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(sint8_least));
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(sint16_least));
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(sint32_least));
    TEST_ASSERT_TRUE((sint8_least)-1 < (sint8_least)0);
    TEST_ASSERT_TRUE((sint16_least)-1 < (sint16_least)0);
    TEST_ASSERT_TRUE((sint32_least)-1 < (sint32_least)0);
}

/*==================================================================================================
 *                                          BOOLEAN
 *================================================================================================*/
/* [SWS_Platform_00026] [SWS_Platform_00027] [SWS_Platform_00056] [REQ-003-02] */
void test_Boolean_SizeAndValues_8BitUnsignedTrueOneFalseZero(void)
{
    TEST_ASSERT_EQUAL_UINT32(8U, BITS_OF(boolean));
    TEST_ASSERT_TRUE((boolean)-1 > (boolean)0);
    TEST_ASSERT_EQUAL_INT(1, TRUE);
    TEST_ASSERT_EQUAL_INT(0, FALSE);
}

/* [SWS_Platform_00060] [SWS_Platform_00034] boolean can be passed by pointer (no bit type) and is
 * used with TRUE / FALSE only */
void test_Boolean_PassedByPointer_KeepsTrueAndFalse(void)
{
    boolean flag = FALSE;
    boolean *flag_ptr = &flag;

    *flag_ptr = TRUE;
    TEST_ASSERT_TRUE(flag == TRUE);

    *flag_ptr = FALSE;
    TEST_ASSERT_TRUE(flag == FALSE);
}

/*==================================================================================================
 *                                     CPU SYMBOLS
 *================================================================================================*/
/* [SWS_Platform_00038] [SWS_Platform_00039] [SWS_Platform_00064] values from SWS annex A.1 */
void test_CpuSymbolValues_Definitions_MatchSwsAnnex(void)
{
    TEST_ASSERT_EQUAL_INT(8, CPU_TYPE_8);
    TEST_ASSERT_EQUAL_INT(16, CPU_TYPE_16);
    TEST_ASSERT_EQUAL_INT(32, CPU_TYPE_32);
    TEST_ASSERT_EQUAL_INT(64, CPU_TYPE_64);
    TEST_ASSERT_EQUAL_INT(0, MSB_FIRST);
    TEST_ASSERT_EQUAL_INT(1, LSB_FIRST);
    TEST_ASSERT_EQUAL_INT(0, HIGH_BYTE_FIRST);
    TEST_ASSERT_EQUAL_INT(1, LOW_BYTE_FIRST);
}

/* [SWS_Platform_00043] [SWS_Platform_00044] [SWS_Platform_00045] [SWS_Platform_00046]
 * [SWS_Platform_00049] [SWS_Platform_00051] [REQ-003-03] MCXN947: 32 bit, little endian */
void test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst(void)
{
    TEST_ASSERT_EQUAL_INT(CPU_TYPE_32, CPU_TYPE);
    TEST_ASSERT_EQUAL_INT(LSB_FIRST, CPU_BIT_ORDER);
    TEST_ASSERT_EQUAL_INT(LOW_BYTE_FIRST, CPU_BYTE_ORDER);
}

/* [SWS_Platform_00046] [SWS_Platform_00051] [REQ-003-03] the host has the byte order of the target,
 * so host tests of byte packing are representative */
void test_CpuByteOrder_HostMemory_LowByteFirst(void)
{
    uint16 value = 0x0102U;
    uint8 bytes[2] = {0U, 0U};

    (void)memcpy(bytes, &value, sizeof(bytes));

    TEST_ASSERT_EQUAL_HEX8(0x02U, bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, bytes[1]);
}

/*==================================================================================================
 *                                FLOATING POINT AND POINTER TYPES
 *================================================================================================*/
/* [SWS_Platform_00041] float32: IEEE 754 binary32 (24-bit significand, max exponent 128) */
void test_Float32_Format_IsIeee754Binary32(void)
{
    TEST_ASSERT_EQUAL_UINT32(32U, BITS_OF(float32));
    TEST_ASSERT_EQUAL_INT(1, _Generic((float32)0.0F, float: 1, default: 0));
    TEST_ASSERT_EQUAL_INT(2, FLT_RADIX);
    TEST_ASSERT_EQUAL_INT(24, FLT_MANT_DIG);
    TEST_ASSERT_EQUAL_INT(128, FLT_MAX_EXP);
}

/* [SWS_Platform_00042] float64: IEEE 754 binary64 (53-bit significand, max exponent 1024) */
void test_Float64_Format_IsIeee754Binary64(void)
{
    TEST_ASSERT_EQUAL_UINT32(64U, BITS_OF(float64));
    TEST_ASSERT_EQUAL_INT(1, _Generic((float64)0.0, double: 1, default: 0));
    TEST_ASSERT_EQUAL_INT(53, DBL_MANT_DIG);
    TEST_ASSERT_EQUAL_INT(1024, DBL_MAX_EXP);
}

/* [SWS_Platform_91001] [SWS_Platform_91002] VoidPtr = void *, ConstVoidPtr = const void * */
void test_VoidPtrTypes_Definitions_PointToVoidAndConstVoid(void)
{
    uint8 buffer[2] = {0U, 0U};
    VoidPtr data_out = buffer;
    ConstVoidPtr data_in = buffer;

    TEST_ASSERT_EQUAL_INT(1, _Generic(data_out, void *: 1, default: 0));
    TEST_ASSERT_EQUAL_INT(1, _Generic(data_in, const void *: 1, default: 0));
    TEST_ASSERT_EQUAL_PTR(buffer, data_out);
    TEST_ASSERT_EQUAL_PTR(buffer, data_in);
}

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
/* [REQ-003-06] AR release 4.7.0, SW version 1.0.0, module ID 199, unregistered vendor */
void test_PublishedInfo_PlatformTypes_Ar470Sw100(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, PLATFORM_VENDOR_ID);
    TEST_ASSERT_EQUAL_UINT16(199U, PLATFORM_MODULE_ID);
    TEST_ASSERT_EQUAL_UINT8(4U, PLATFORM_AR_RELEASE_MAJOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(7U, PLATFORM_AR_RELEASE_MINOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, PLATFORM_AR_RELEASE_REVISION_VERSION);
    TEST_ASSERT_EQUAL_UINT8(1U, PLATFORM_SW_MAJOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, PLATFORM_SW_MINOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, PLATFORM_SW_PATCH_VERSION);
}
