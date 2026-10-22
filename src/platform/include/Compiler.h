/**
 * @file    Compiler.h
 * @brief   AUTOSAR compiler abstraction for GNU GCC (target Cortex-M33 and host unit tests).
 * @details AUTOSAR Classic R21-11, AUTOSAR_SWS_CompilerAbstraction (document 51). Layer: platform.
 *
 *          Deviation DEV-01 (coding_rules.md section 10): the function, pointer, constant and
 *          variable macros (FUNC, FUNC_P2CONST, FUNC_P2VAR, P2VAR, P2CONST, CONSTP2VAR,
 *          CONSTP2CONST, P2FUNC, CONSTP2FUNC, CONST, VAR) are not provided. The Cortex-M33 has
 *          a flat address space, so they would all expand to plain C.
 *
 *          Requirement tags: [SWS_COMPILER_xxxxx] = AUTOSAR_SWS_CompilerAbstraction,
 *          [SWS_Std_xxxxx] = AUTOSAR_SWS_StandardTypes, [REQ-003-xx] = .claude/tasks/TASK-003.md.
 */
/* [SWS_COMPILER_00004] the file name is Compiler.h */
/* [SWS_COMPILER_00047] protected against multiple inclusion; only comments outside the guard */
#ifndef COMPILER_H
#define COMPILER_H

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
/* [SWS_COMPILER_00055] Compiler.h includes Compiler_Cfg.h (SWS figure 1, include structure) */
#include "Compiler_Cfg.h"

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
/* [REQ-003-06] published information (SWS 10.3 says "not applicable"; kept for coding_rules.md 5) */
#define COMPILER_VENDOR_ID                      (0xFFFFU)   /* xAT: unregistered vendor */
#define COMPILER_MODULE_ID                      (198U)      /* AUTOSAR_TR_BSWModuleList: Compiler */
#define COMPILER_AR_RELEASE_MAJOR_VERSION       (4U)
#define COMPILER_AR_RELEASE_MINOR_VERSION       (7U)
#define COMPILER_AR_RELEASE_REVISION_VERSION    (0U)
#define COMPILER_SW_MAJOR_VERSION               (1U)
#define COMPILER_SW_MINOR_VERSION               (0U)
#define COMPILER_SW_PATCH_VERSION               (0U)

/*==================================================================================================
 *                                  COMPILER IDENTIFICATION
 *================================================================================================*/
/* [SWS_COMPILER_00030] Supported compiler vendor and versions: GNU GCC.
 *   - target: Arm GNU Toolchain 15.2.Rel1 (arm-none-eabi-gcc 15.2.1), Cortex-M33
 *   - host unit tests: Ubuntu gcc 13.3.0, aarch64
 * Older or other compilers are rejected. */
#if !defined(__GNUC__) || (__GNUC__ < 13)
#error "Compiler.h: GNU GCC 13 or newer is required"
#endif

/* [SWS_COMPILER_00003] [SWS_COMPILER_00010] One compiler abstraction per compiler and platform:
 * the symbol _<COMPILERNAME>_C_<PLATFORMNAME>_ identifies the pair, without a value.
 * MISRA C:2012 Rule 21.1 deviation: the leading underscore is the SWS spelling of the symbol
 * (coding_rules.md 2.1, SWS names win). */
#if defined(__ARM_ARCH_8M_MAIN__)
#define _GCC_C_CORTEXM33_                       /* target: arm-none-eabi-gcc, Cortex-M33 (Armv8-M) */
#elif defined(__aarch64__)
#define _GCC_C_AARCH64_                         /* host: gcc, aarch64 (Unity + CMock unit tests) */
#else
#error "Compiler.h: unsupported platform for GNU GCC"
#endif

/*==================================================================================================
 *                                   GENERAL DEFINITIONS
 *================================================================================================*/
/* [SWS_COMPILER_00046] [SWS_COMPILER_00005] memory class for local (auto) variables: empty */
#define AUTOMATIC

/* [SWS_COMPILER_00059] [SWS_COMPILER_00005] memory class for type definitions: empty */
#define TYPEDEF

/* [SWS_Std_00031] [REQ-003-07] void pointer to zero, provided by the compiler abstraction */
#define NULL_PTR                                ((void *)0)

/* [SWS_COMPILER_00057] [REQ-003-07] abstraction of the keyword inline (C11 'inline').
 * Note: with C11 inline semantics a plain 'inline' function defined in a header also needs one
 * external definition; use LOCAL_INLINE for helpers in headers. */
#define INLINE                                  inline

/* [SWS_COMPILER_00060] [REQ-003-07] inline function with static scope (C11 'static inline') */
#define LOCAL_INLINE                            static inline

#endif /* COMPILER_H */
