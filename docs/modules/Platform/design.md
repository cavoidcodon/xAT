# Platform - design

| Field | Value |
|---|---|
| Layer | platform (`src/platform`) |
| AUTOSAR document | `AUTOSAR_SWS_PlatformTypes` (48), `AUTOSAR_SWS_StandardTypes` (49), `AUTOSAR_SWS_CompilerAbstraction` (51), R21-11 |
| Module ID | Platform 199, Std 197, Compiler 198 (`AUTOSAR_TR_BSWModuleList`) |
| Status | Implemented (TASK-003) |

## Purpose in xAT

The four headers give every module the AUTOSAR base types and symbols: fixed-width integer
types, `boolean`, `Std_ReturnType`, `E_OK` / `E_NOT_OK`, `STD_ON` / `STD_OFF`, `NULL_PTR` and
the inline keywords. They are header-only and include only `<stdint.h>`, so every layer
(app, rte, bsw, mcal, os, ecu) and the host unit tests can use them (`layering.md`).

## Derived requirements

- Same type widths on the target (arm-none-eabi-gcc 15.2, ILP32) and on the host unit test
  build (gcc 13.3 aarch64, LP64): REQ-003-01.
- MCAL code passes `uint32` / `uint16` / `uint8` values and pointers to the NXP SDK
  (`uint32_t` ...) without casts.

## Scope

### Supported API

The headers have no functions. Supported items:

| File | Items | SWS IDs |
|---|---|---|
| `Platform_Types.h` | `CPU_TYPE`, `CPU_BIT_ORDER`, `CPU_BYTE_ORDER` and their values; `boolean`, `TRUE`, `FALSE`; `uint8..uint64`, `sint8..sint64`; `*_least`; `float32`, `float64`; `VoidPtr`, `ConstVoidPtr` | SWS_Platform_00002, 00013-00018, 00020-00027, 00038, 00039, 00041-00046, 00049, 00051, 00054, 00056, 00060, 00061, 00064, 00066, 00067, 91001, 91002 |
| `Std_Types.h` | `Std_ReturnType`, `E_OK`, `E_NOT_OK` (`STATUSTYPEDEFINED`), `STD_HIGH/LOW`, `STD_ACTIVE/IDLE`, `STD_ON/OFF`, `Std_VersionInfoType`, transformer error types, message types, `Std_ExtractProtocolHeaderFieldsType` | SWS_Std_00004-00007, 00010, 00013-00015, 00017, 00019, 00021-00026, 91001-91003 |
| `Compiler.h` | compiler symbol, `AUTOMATIC`, `TYPEDEF`, `NULL_PTR`, `INLINE`, `LOCAL_INLINE` | SWS_COMPILER_00003-00005, 00010, 00030, 00046, 00047, 00057, 00059, 00060; SWS_Std_00031 |
| `Compiler_Cfg.h` | empty (no memory / pointer classes) | SWS_COMPILER_00055 (partial) |

Each of the three main headers publishes vendor ID `0xFFFF`, its module ID, AR release
4.7.0 and SW version 1.0.0 (REQ-003-06).

### Not supported

| API / feature / container | Reason |
|---|---|
| `FUNC`, `FUNC_P2CONST`, `FUNC_P2VAR`, `P2VAR`, `P2CONST`, `CONSTP2VAR`, `CONSTP2CONST`, `P2FUNC`, `CONSTP2FUNC`, `CONST`, `VAR` and the memclass / ptrclass parameters | Deviation DEV-01: flat address space on the Cortex-M33 |
| `Std_TransformerForward`, `Std_TransformerForwardCode`, `E_SAFETY_INVALID_*` (SWS_Std_00027-00029) | status "draft" in R21-11 |
| `ComStack_Types.h` (SWS_Std_00030) | belongs to the communication stack (CanIf task) |
| `<Ma>_MemMap.h` | Deviation DEV-02 |

## Interfaces

Diagram: [component.puml](component.puml)

| Direction | Module | Interface | Notes |
|---|---|---|---|
| provided to | every module, the RTE, SWCs, `src/ecu`, host tests | types and symbols | modules include `Std_Types.h`, which includes the other headers |
| required from | C standard library | `<stdint.h>` | only include of the platform headers |

## Design decisions

1. **Integer types from `<stdint.h>`** (TASK-003 decision D1). The C11 exact-width types
   have the right width on every compiler. A hand-written `unsigned int` for `uint32` would
   also be 32-bit on both, but newlib defines `uint32_t` as `unsigned long` on the target.
   With GCC 15, `uint32 *` -> `uint32_t *` would then be an incompatible-pointer error in
   MCAL code. Only `Platform_Types.h` uses `<stdint.h>` types (`coding_rules.md` 3.1).
2. **`*_least` types are 32-bit** on both compilers, as in the SWS annex for 32-bit CPUs.
   `uint_fastN_t` is not used, because glibc (host) and newlib (target) define it differently.
3. **One `Compiler.h` for two compiler / platform pairs** (SWS_COMPILER_00003): it defines
   `_GCC_C_CORTEXM33_` for the target and `_GCC_C_AARCH64_` for the host test build, and
   stops with `#error` on any other compiler or platform. MCAL code that needs
   target-only code (e.g. inline assembly) can test `_GCC_C_CORTEXM33_`, and the host build
   does not see it. The leading underscore is the SWS spelling (MISRA C:2012 Rule 21.1
   deviation, `coding_rules.md` 2.1).
4. **`E_OK` and `StatusType`** are defined inside `#ifndef STATUSTYPEDEFINED`, as
   SWS_Std_00006 writes it. The OS (TASK-009) must define `StatusType` and `E_OK` inside the
   same guard; `tests/unit/platform/compile_checks/check_StatusTypePredefined.c` checks the
   case where the OS header comes first.
5. **`INLINE` vs `LOCAL_INLINE`.** With C11 semantics, a plain `inline` function defined in a
   header also needs one external definition, otherwise a `-O0` build can fail to link.
   Helpers in headers use `LOCAL_INLINE` (`coding_rules.md` 4.5).
6. **Version check.** The headers have no `.c` file; a module that includes them checks
   `STD_AR_RELEASE_MAJOR_VERSION` / `MINOR` like any other BSW header (`coding_rules.md` 5.1).

## Behaviour

None (types and symbols only).

### Initialisation

None.

## State

None.

## Configuration

None. `Compiler_Cfg.h` would hold module memory / pointer classes, which DEV-01 removes.

## Hardware mapping (FRDM-MCXN947)

None. The MCXN947 Cortex-M33 core is 32-bit and little endian (`CPU_TYPE_32`, `LSB_FIRST`,
`LOW_BYTE_FIRST`); `src/ecu/main.c` checks this against the target compiler with
`_Static_assert` at every firmware build.

## Error handling

None. The three SWS define no development, runtime or production errors.

## Reentrancy and exclusive areas

None.

## Deviations from the SWS

| SWS requirement | Deviation | Reason |
|---|---|---|
| SWS_COMPILER_00001, 00006, 00013, 00023, 00026, 00031, 00032, 00035, 00036, 00039-00041, 00048, 00053, 00058, 00061-00065 | macros not provided (DEV-01) | flat address space |
| SWS_COMPILER_00055 | `Compiler_Cfg.h` has no classes | follows from DEV-01 |

## Open questions and assumptions

- Module IDs and abbreviations checked against `AUTOSAR_TR_BSWModuleList` R21-11 (document
  150): Standard Types `Std` 197, Compiler Abstraction `Compiler` 198, Platform Types
  `Platform` 199.
- The include structure `Std_Types.h` -> `Platform_Types.h`, `Compiler.h` follows figure 1
  of the CompilerAbstraction SWS; the corresponding figure of the StandardTypes SWS was lost
  in the PDF-to-Markdown conversion.

SWS requirement -> code -> test: see `docs/autosar_compliance.md`.
