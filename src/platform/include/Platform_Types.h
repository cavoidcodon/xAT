/**
 * @file    Platform_Types.h
 * @brief   AUTOSAR platform types for the MCXN947 (Cortex-M33, 32-bit, little endian).
 * @details AUTOSAR Classic R21-11, AUTOSAR_SWS_PlatformTypes (document 48). Layer: platform.
 *
 *          The integer types map onto the C11 <stdint.h> exact-width types. This gives the
 *          same widths on the target (arm-none-eabi-gcc, ILP32) and on the host unit test
 *          build (gcc aarch64, LP64), and the same underlying types as the NXP SDK
 *          (uint32_t is 'unsigned long' with newlib), so MCAL code passes pointers to the SDK
 *          without casts (TASK-003 decision D1, coding_rules.md 3.1).
 *
 *          Requirement tags: [SWS_Platform_xxxxx] = AUTOSAR_SWS_PlatformTypes,
 *          [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
 */
/* [SWS_Platform_00002] all platform types and symbols are defined in this file only */
#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
/* [REQ-003-01] exact-width types from the C standard library (the only include of this file) */
#include <stdint.h>

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
/* [REQ-003-06] published information (SWS_PlatformTypes 10.1 -> SWS_BSWGeneral 10.3) */
#define PLATFORM_VENDOR_ID                      (0xFFFFU)   /* xAT: unregistered vendor */
#define PLATFORM_MODULE_ID                      (199U)      /* AUTOSAR_TR_BSWModuleList: Platform */
#define PLATFORM_AR_RELEASE_MAJOR_VERSION       (4U)
#define PLATFORM_AR_RELEASE_MINOR_VERSION       (7U)
#define PLATFORM_AR_RELEASE_REVISION_VERSION    (0U)
#define PLATFORM_SW_MAJOR_VERSION               (1U)
#define PLATFORM_SW_MINOR_VERSION               (0U)
#define PLATFORM_SW_PATCH_VERSION               (0U)

/*==================================================================================================
 *                                    SYMBOL DEFINITIONS
 *================================================================================================*/
/* [SWS_Platform_00064] possible values of CPU_TYPE (values from SWS annex A.1) */
#define CPU_TYPE_8                              (8)
#define CPU_TYPE_16                             (16)
#define CPU_TYPE_32                             (32)
#define CPU_TYPE_64                             (64)

/* [SWS_Platform_00038] possible values of CPU_BIT_ORDER (values from SWS annex A.1) */
#define MSB_FIRST                               (0)
#define LSB_FIRST                               (1)

/* [SWS_Platform_00039] possible values of CPU_BYTE_ORDER (values from SWS annex A.1) */
#define HIGH_BYTE_FIRST                         (0)
#define LOW_BYTE_FIRST                          (1)

/* [SWS_Platform_00044] [SWS_Platform_00045] [REQ-003-03] Cortex-M33 has 32-bit registers */
#define CPU_TYPE                                (CPU_TYPE_32)

/* [SWS_Platform_00043] [SWS_Platform_00049] [REQ-003-03] little endian bit order on register level */
#define CPU_BIT_ORDER                           (LSB_FIRST)

/* [SWS_Platform_00046] [SWS_Platform_00051] [REQ-003-03] little endian byte order in memory */
#define CPU_BYTE_ORDER                          (LOW_BYTE_FIRST)

/* [SWS_Platform_00056] [SWS_Platform_00054] [REQ-003-02] TRUE / FALSE exactly as the SWS defines
 * them; the #ifndef guards avoid a redefinition when a compiler or a library already has them.
 * [SWS_Platform_00055] they are used only with boolean (coding_rules.md 3.3). */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*==================================================================================================
 *                                     TYPE DEFINITIONS
 *================================================================================================*/
/* [SWS_Platform_00061] signed types are two's complement: guaranteed by C11 for intN_t (7.20.1.1) */

/* [SWS_Platform_00026] [SWS_Platform_00027] [SWS_Platform_00060] [REQ-003-02] boolean: 8-bit
 * unsigned, the shortest native type, addressable (no bit type), so it can be passed by pointer.
 * [SWS_Platform_00034] only used with TRUE / FALSE (coding_rules.md 3.3). */
typedef uint8_t boolean;

/* [SWS_Platform_00013] [REQ-003-01] uint8: 8-bit unsigned, 0..255 */
typedef uint8_t uint8;

/* [SWS_Platform_00014] [REQ-003-01] uint16: 16-bit unsigned, 0..65535 */
typedef uint16_t uint16;

/* [SWS_Platform_00015] [REQ-003-01] uint32: 32-bit unsigned, 0..4294967295 */
typedef uint32_t uint32;

/* [SWS_Platform_00066] [REQ-003-01] uint64: 64-bit unsigned, 0..18446744073709551615 */
typedef uint64_t uint64;

/* [SWS_Platform_00016] [REQ-003-01] sint8: 8-bit signed, -128..+127 */
typedef int8_t sint8;

/* [SWS_Platform_00017] [REQ-003-01] sint16: 16-bit signed, -32768..+32767 */
typedef int16_t sint16;

/* [SWS_Platform_00018] [REQ-003-01] sint32: 32-bit signed, -2147483648..+2147483647 */
typedef int32_t sint32;

/* [SWS_Platform_00067] [REQ-003-01] sint64: 64-bit signed */
typedef int64_t sint64;

/* [SWS_Platform_00020] [SWS_Platform_00021] [SWS_Platform_00022] optimized unsigned types: the
 * native register width of the Cortex-M33 (32 bit), as in the SWS annex for 32-bit CPUs. Fixed
 * widths instead of uint_fastN_t, because those differ between the target and the host. */
typedef uint32_t uint8_least;
typedef uint32_t uint16_least;
typedef uint32_t uint32_least;

/* [SWS_Platform_00023] [SWS_Platform_00024] [SWS_Platform_00025] optimized signed types (32 bit) */
typedef int32_t sint8_least;
typedef int32_t sint16_least;
typedef int32_t sint32_least;

/* [SWS_Platform_00041] float32: IEEE 754-2008 binary32 (GCC 'float' on both compilers) */
typedef float float32;

/* [SWS_Platform_00042] float64: IEEE 754-2008 binary64 (GCC 'double' on both compilers; software
 * emulated on the target, whose FPU is single precision) */
typedef double float64;

/* [SWS_Platform_91001] VoidPtr: pointer to buffers that return data to the caller */
typedef void *VoidPtr;

/* [SWS_Platform_91002] ConstVoidPtr: pointer to buffers that are passed to the callee */
typedef const void *ConstVoidPtr;

#endif /* PLATFORM_TYPES_H */
