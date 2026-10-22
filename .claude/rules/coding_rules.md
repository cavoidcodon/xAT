---
paths:
  - "src/**/*.c"
  - "src/**/*.h"
  - "tests/**/*.c"
  - "tests/**/*.h"
---

# C coding rules

Applies to all C code in `src/` and `tests/`. Baseline: **AUTOSAR Classic R21-11
(AR 4.7.0)**, mainly `AUTOSAR_SRS_BSWGeneral`, `AUTOSAR_SWS_BSWGeneral`, `AUTOSAR_SWS_StandardTypes`,
`AUTOSAR_SWS_PlatformTypes`, `AUTOSAR_SWS_CompilerAbstraction`, and **MISRA C:2012**
(AUTOSAR requires MISRA C for BSW). Language: **C11** (`-std=c11`), no compiler extensions.

When a rule here conflicts with a name or signature that an AUTOSAR SWS defines, **the SWS wins**
(see 2.1). Any other deviation must be listed in section 10 before it is used.

---

## 1. General

1. Write all code, identifiers and comments in English, using ASCII only.
2. Identifiers contain only `[A-Za-z0-9_]` and start with a letter.
3. Do not use reserved identifiers: no leading underscore, no `__` anywhere, and no names
   reserved by the C standard library (`errno`, `str*`, `mem*`, `is*`, `to*`, `E[0-9A-Z]*`, `*_t` …).
4. Names must be descriptive and consistent with the rest of the module. Do not use
   abbreviations unless they are AUTOSAR terms (`Cfg`, `Id`, `Cbk`, `Det`, `Sid`, `Pdu` …).

## 2. Naming

### 2.1 Names defined by AUTOSAR (keep them exactly as written)

Any symbol that a SWS defines keeps its SWS spelling, even when that spelling breaks 1.3 or
2.2. This covers API functions, types, enum literals, macros, error codes, callbacks and
callouts. Examples: `Dio_ReadChannel`, `Dio_ChannelType`, `DIO_E_PARAM_INVALID_CHANNEL_ID`,
`Mcu_ConfigType`, `E_OK`, `STD_HIGH`, `NULL_PTR`, and the compiler symbol
`_GCC_C_CORTEXM33_` of `Compiler.h` (leading underscore, SWS_COMPILER_00010; marked with a
MISRA C:2012 Rule 21.1 deviation comment).

### 2.2 Names defined by the project

`<Ma>` = module abbreviation from the BSW module list (`Dio`, `Mcu`, `EcuM`, `IoHwAb`…).
`<MA>` = the same in upper case (`DIO`, `ECUM`, `IOHWAB`).

| Kind | Format | Example |
|---|---|---|
| Public API function | `<Ma>_<PascalCase>` | `Dio_ReadChannel` |
| Module-internal function (`static`) | `<Ma>_<PascalCase>` | `static void Adc_StartHwGroup(...)` |
| Type | `<Ma>_<PascalCase>Type` | `Pwm_ChannelConfigType` |
| Enum literal | `<MA>_<UPPER_CASE>` | `ADC_STATE_IDLE` |
| Macro / constant | `<MA>_<UPPER_CASE>` | `GPT_SID_START_TIMER` |
| Global variable (external linkage) | `g_<Ma>_<camelCase>` | `g_EcuM_shutdownTarget` |
| Static variable (file or function scope) | `s_<camelCase>` | `s_initState` |
| Local variable, function parameter | `snake_case` | `channel_id`, `level` |
| Configuration set instance | `<Ma>_Config[_<Name>]` | `Mcu_Config` |
| SWC runnable | `<SwcName>_<PascalCase>` | `SwcShiftCtrl_Run10ms` |
| Struct member | `snake_case` | `cfg->channel_count` |

Rules:

1. A project-defined variable name may be at most **20 characters** long, counting any
   prefix. This does not apply to SWS-defined names.
2. Do not use single-letter names, except for loop counters (`i`, `j`).
3. Prefer `static` (internal linkage) to `g_` variables. A `g_` variable is allowed only
   when another file of the **same module** needs it. Modules never share variables; they
   share data through APIs.
4. A boolean name reads as a question: `is_ready`, `s_isInitialized`.
5. Name service IDs `<MA>_SID_<API_NAME>`, using the IDs from the SWS ("Service ID [hex]").

## 3. Types

1. Use the AUTOSAR types from `Platform_Types.h` / `Std_Types.h`: `uint8`, `uint16`,
   `uint32`, `uint64`, `sint8` … `sint64`, `boolean`, `float32`, `float64`.
   Do not use the native `int`, `short`, `long`, `char` (except `char` for string
   literals) or `<stdint.h>` types in module code. The only exceptions are MCAL code that
   passes values to or from the NXP SDK, and `Platform_Types.h`, which maps the AUTOSAR
   types onto the `<stdint.h>` exact-width types (same widths on the ILP32 target and the
   LP64 host, same underlying types as the SDK; TASK-003).
2. Return `Std_ReturnType` (`E_OK` / `E_NOT_OK`) from functions that can fail, unless the
   SWS specifies another type.
3. Use `TRUE` / `FALSE`, `STD_HIGH` / `STD_LOW`, `STD_ON` / `STD_OFF` and `NULL_PTR`.
   Do not use `true`, `1`, or `NULL`.
4. Write unsigned constants with the `U` suffix (`0x10U`, `1000UL`).
5. Do not rely on implicit conversions between essential types (MISRA 10.x). Cast
   explicitly, and only after checking the range.
6. Do not use bit-fields or `union`s for register access or type punning.
7. Mark pointer parameters `const` wherever the function does not write through them.

## 4. Files and module structure

Each module lives in `src/<layer>/<Ma>/` and contains:

| File | Content |
|---|---|
| `include/<Ma>.h` | public API, public types (or includes `<Ma>_Types.h`), version info, DET error codes |
| `include/<Ma>_Types.h` | only when the SWS defines one |
| `include/<Ma>_Cbk.h` | callback/notification prototypes, only when the module has them |
| `src/<Ma>.c` | implementation |
| `src/<Ma>_Internal.h` | internal shared declarations (optional, never included outside the module) |
| `config/<Ma>_Cfg.h` | pre-compile parameters (`<MA>_DEV_ERROR_DETECT`, counts, symbolic names) |
| `config/<Ma>_Cfg.c` | link-time/post-build configuration data (`const`) |

Rules:

1. Use include guards of the form `#ifndef <MA>_H` / `#define <MA>_H`, with the file name
   in upper case. Do not use `#pragma once`.
2. Every header compiles on its own: it includes what it uses and nothing more.
3. `<Ma>.h` includes `Std_Types.h` and `<Ma>_Cfg.h`.
4. A `.c` file includes its own header first.
5. Do not define objects or functions in headers, except `static inline` helpers
   (`LOCAL_INLINE`).
6. Every file starts with the file header from the module template (file name, brief,
   AUTOSAR document, release).
7. Use this order inside a `.c` file: includes → version check → local macros → local
   types → local function prototypes → static variables → global variables → global
   functions → local functions.

## 5. Published information and version check

Every `<Ma>.h` defines:

```c
#define <MA>_VENDOR_ID                    (0xFFFFU)   /* xAT: unregistered vendor */
#define <MA>_MODULE_ID                    (<id>U)     /* AUTOSAR_TR_BSWModuleList */
#define <MA>_AR_RELEASE_MAJOR_VERSION     (4U)
#define <MA>_AR_RELEASE_MINOR_VERSION     (7U)
#define <MA>_AR_RELEASE_REVISION_VERSION  (0U)
#define <MA>_SW_MAJOR_VERSION             (1U)
#define <MA>_SW_MINOR_VERSION             (0U)
#define <MA>_SW_PATCH_VERSION             (0U)
```

1. `<Ma>.c` and `<Ma>_Cfg.c` check with `#if … #error` that they match the version of
   `<Ma>.h`, and also the AR release version of every BSW header they include.
2. Provide `<Ma>_GetVersionInfo()` when the SWS defines it, and switch it on or off with
   `<MA>_VERSION_INFO_API`.

## 6. Error handling (DET)

1. Development error detection is switched on or off with `<MA>_DEV_ERROR_DETECT`
   (`STD_ON`/`STD_OFF`) in `<Ma>_Cfg.h`. Code that is only for DET is enclosed in
   `#if (<MA>_DEV_ERROR_DETECT == STD_ON)`.
2. Report errors with `Det_ReportError(<MA>_MODULE_ID, instance_id, <MA>_SID_<API>, <MA>_E_<ERR>)`.
   Use the error codes and values from the SWS. Report runtime errors with
   `Det_ReportRuntimeError`.
3. Every API that the SWS says needs it checks, in this order: initialization
   (`<MA>_E_UNINIT`), then null pointers (`<MA>_E_PARAM_POINTER`), then parameter ranges.
   When a check fails, the API reports the error and returns without side effects.
4. Never ignore the return value of a non-`void` function. If it is really not needed,
   cast it to `(void)`.

## 7. Control flow and safety (MISRA C:2012 subset, enforced)

1. Do not allocate memory dynamically (`malloc`, `free`, VLAs), and do not use recursion,
   `goto`, `setjmp`/`longjmp`, or `stdio` in the target code.
2. Every function has **one exit point** (one `return` at the end).
3. Always use braces for `if`, `else`, `for`, `while` and `do`, even around a single
   statement.
4. Every `switch` has a `default`. Every non-empty `case` ends with `break`, with no
   fall-through.
5. Every `if … else if` chain ends with an `else`.
6. Initialize every variable before it is read. Initialize static variables explicitly.
7. Loops must have a fixed upper bound. Polling a HW flag must have a timeout counter.
8. Declare data shared between an ISR and a task, or memory-mapped registers, `volatile`.
   Protect read-modify-write accesses with `SchM_Enter_<Ma>_<Area>()` / `SchM_Exit_<Ma>_<Area>()`.
9. Do not compare floats with `==`. Floating point is not allowed in MCAL.
10. Use function-like macros only when a function cannot do the job. Parenthesize every
    macro parameter and the whole macro body.
11. Do not write magic numbers. Use named macros or configuration parameters.

## 8. Formatting

1. Indent with 4 spaces, never tabs. Keep lines at 120 characters or fewer.
2. Put braces on their own line (Allman style).
3. Write comments with `/* */`. Comment *why*, not *what*.
4. Document public APIs with Doxygen (`@brief`, `@param[in/out]`, `@return`) and the
   requirement IDs they implement (8.6).
5. Declare one variable per line, at the start of its block.
6. **Requirement tags.** Every piece of code that implements a requirement has the IDs of
   those requirements in a comment directly above it, so that coverage can be reviewed
   with `grep`. This is how `docs/autosar_compliance.md` (SWS) and
   `docs/specs/traceability.md` (SYS / SWR) trace requirements to code.
   - Format: each ID in square brackets; one comment may list several IDs, followed by a
     short text: `/* [SWS_Dio_00133] [REQ-007-03] channel ID out of range */`.
   - IDs: AUTOSAR `SWS_<Ma>_nnnnn` (and `SWS_BSW_nnnnn`); specs `SG-nn`,
     `SYS-<AREA>-nnn`, `SWR-<SWC>-nnn`, `SIM-<AREA>-nnn`; task requirements
     `REQ-<task>-nn` (section 5 of the task file, e.g. `REQ-003-01`).
   - Place the tag on the smallest unit that implements the requirement: a type, macro,
     constant, function, or a block inside a function (for example one DET check), not
     as one list at the top of the file.
   - The same applies to CMake, configuration (`config/`) and linker files
     (`# [REQ-003-10]`), and to tests (`testing_rules.md`).
   - A requirement that is not implemented (usage rule, deviation, not applicable) may be
     cited in an explaining comment; its status is in the compliance matrix.
   - List every tag:
     `grep -rnoE '\[(SWS_[A-Za-z]+_[0-9]{5}|SG-[0-9]{2}|(SYS|SWR|SIM)-[A-Z]+-[0-9]{3}|REQ-[0-9]{3}-[0-9]{2})\]' src tests cmake`.

## 9. Build and static checks

1. Compile with `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.
   SDK sources keep the vendor flags.
2. Target: run MISRA checks with cppcheck and its MISRA addon (to be added in a later
   task). Each justified violation gets a comment
   `/* MISRA C:2012 Rule x.y deviation: <reason> */`.

## 10. Project deviations from AUTOSAR

| ID | Deviation | Reason | Status |
|---|---|---|---|
| DEV-01 | No compiler abstraction pointer/function macros (`FUNC`, `FUNC_P2CONST`, `FUNC_P2VAR`, `P2VAR`, `P2CONST`, `CONSTP2VAR`, `CONSTP2CONST`, `P2FUNC`, `CONSTP2FUNC`, `VAR`, `CONST`); `Compiler_Cfg.h` has no memory / pointer classes. `Compiler.h` still provides `AUTOMATIC`, `TYPEDEF`, `INLINE`, `LOCAL_INLINE` and `NULL_PTR`. | Cortex-M33 has a flat address space; the macros make the code harder to read with no benefit. | Accepted |
| DEV-02 | No `<Ma>_MemMap.h` sections at first. | Single core, default linker sections are enough. | Revisit when memory partitioning or a safety split is needed |
| DEV-03 | Configuration is handwritten (no ECUC/ARXML generator). | Decided in CLAUDE.md. | Accepted |
