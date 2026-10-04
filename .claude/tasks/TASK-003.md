# TASK-003: Platform types, firmware entry `src/ecu`, CMake module helper, Unity + CMock host test build

| Field        | Value                                                                                   |
|--------------|-----------------------------------------------------------------------------------------|
| Priority     | P0 (blocks every module task)                                                           |
| Depends on   | TASK-001 (tooling)                                                                      |
| Module(s)    | Platform (`src/platform`), ECU integration (`src/ecu`), build/test infrastructure       |
| AUTOSAR doc  | `AUTOSAR_SWS_PlatformTypes`, `AUTOSAR_SWS_StandardTypes`, `AUTOSAR_SWS_CompilerAbstraction` (R21-11) |

---

## 0. Plan contents (task-specific)

Run with the `module-task` skill. Nothing is installed before the plan is approved. The plan
must also contain:
- the exact Unity and CMock versions (release tags) and how they are vendored;
- every host command that needs `sudo` or installs software, listed explicitly;
- the public interface of the CMake helpers (`xat_add_module`, `xat_add_unit_test`);
- the list of SWS requirement IDs covered for the three platform headers (taken from the
  R21-11 PDFs in `docs/references/autosar/`; if a PDF is missing, say so).

## 1. Goal

Create the foundation that every later module task builds on:

1. The AUTOSAR **platform headers** (`Platform_Types.h`, `Std_Types.h`, `Compiler.h`,
   `Compiler_Cfg.h`) for the MCXN947 (Cortex-M33, 32-bit, little endian).
2. The **firmware entry project** `src/ecu`. It builds out of tree against the
   MCUXpresso SDK with `scripts/operations.py`, boots on the board and prints a banner.
   EcuM takes over the start-up sequence later (TASK-010).
3. A **CMake helper** (`cmake/xat_module.cmake`), so that adding a module to the firmware
   takes one line.
4. A **host unit test build** with Unity + CMock (`tests/unit/`, `cmake/xat_unit_test.cmake`).
   Adding a module test with mocked dependencies must take one function call.

When this task is done, TASK-004 (`Det`) can start with the `new-module` skill and be built,
tested and verified with the `verify` skill, with no further infrastructure work.

## 2. Context (verified on 2026-10-03)

- **SDK.** MCUXpresso SDK 26.09.00 at `workspace/mcuxsdk` (read-only). CMake functions are
  in `cmake/extension/function.cmake`: `mcux_add_source` (line 4), `mcux_add_include`
  (line 418) and `mcux_add_configuration` (line 900).
- **Board port.** `examples/_boards/frdmmcxn947/demo_apps/hello_world/cm33_core0/`
  contains `app.h`, `hardware_init.c` (`BOARD_InitHardware`), `prj.conf` and
  `reconfig.cmake`. The board-level `pin_mux.c`, `board.c` and `clock_config.c` are in
  `examples/_boards/frdmmcxn947/`. TASK-001 used this port successfully with
  `PROJECT_BOARD_PORT_PATH` (see the "Firmware build notes" in `docs/operations.md`).
- **Image name.** The SDK names the image `<project>_<core_id>`. With `--app src/ecu`,
  `scripts/operations.py` expects `build/ecu/<config>/ecu_cm33_core0.elf`. The default
  `--app` is already `src/ecu`.
- **Host compilers.** `gcc` 13.3.0 (Ubuntu, **aarch64, LP64**) for the unit tests, and
  `arm-none-eabi-gcc` 15.2.rel1 for the target. CMake 3.31.10 and Ninja 1.13.2 are in `.venv`.
- **Unity / CMock.**
  - The SDK contains `components/unity/` (`unity.c/.h`), but it has no version macros
    and no CMock, so it is **not** used.
  - CMock generates mocks with Ruby, and **Ruby is not installed** (apt candidate
    `ruby 1:3.2~ubuntu1`).
  - GitHub is reachable from the host.
- **Platform type width.** On the target, `long` is 32-bit (ILP32). On the host it is
  64-bit (LP64). `Platform_Types.h` must give the same widths on both, so it must not use
  `long` for `uint32`/`sint32`.

## 3. Scope

### In scope
1. `src/platform/include/`:
   - `Platform_Types.h`: CPU type, bit order and byte order symbols; `boolean`; the
     `uint8`..`uint64` and `sint8`..`sint64` types; the `*_least` types; `float32` and
     `float64`; `TRUE`/`FALSE`; published version info.
   - `Std_Types.h`: `Std_ReturnType`, `E_OK`/`E_NOT_OK`, `STD_HIGH`/`STD_LOW`,
     `STD_ACTIVE`/`STD_IDLE`, `STD_ON`/`STD_OFF`, `Std_VersionInfoType`; published
     version info.
   - `Compiler.h` / `Compiler_Cfg.h`: `AUTOMATIC`, `TYPEDEF`, `NULL_PTR`, `INLINE`,
     `LOCAL_INLINE`, the compiler identification symbol for GCC; published version info.
     No `FUNC`/`P2VAR`/… macros (deviation DEV-01).
2. `src/ecu/`:
   - `CMakeLists.txt`: out-of-tree SDK build, the same approach as the TASK-001 bring-up.
   - `prj.conf`.
   - `main.c`: calls `BOARD_InitHardware()`, prints `xAT ecu: boot OK` on the debug
     console, then idles in an endless loop. A comment marks where `EcuM_Init()` goes in
     TASK-010.
3. `cmake/xat_module.cmake`:
   - `xat_add_module(NAME <Ma> DIR <path>)` adds `<path>/src/*.c` and `<path>/config/*.c`
     as sources, and `<path>/include` and `<path>/config` as include directories.
   - It applies the project compile flags (`-std=c11 -Wall -Wextra -Wpedantic -Wconversion
     -Wshadow -Werror`) to project sources only. SDK sources keep the vendor flags.
   - It also supports header-only modules such as `platform`.
4. `third_party/unity/` and `third_party/cmock/`: vendored at pinned release tags,
   unmodified, with versions recorded in `third_party/README.md`.
5. `cmake/xat_unit_test.cmake` and `tests/unit/CMakeLists.txt`:
   - `xat_add_unit_test(NAME <test> SOURCES <module sources + test file> MOCKS <headers> INCLUDES <dirs> DEFINES <defs>)`
     generates the CMock mocks into the build directory, builds one executable per test
     and registers it with CTest.
   - Shared CMock settings go in `tests/unit/cmock.yml`.
   - Host tests use the same strict warning flags as the target.
6. Unit tests:
   - `tests/unit/platform/test_Platform_Types.c`: type widths and signedness, boolean
     values, and the `Std_Types` constant values required by the SWS.
   - `tests/unit/framework/`: a minimal self-test that mocks a dummy header with CMock.
     It proves that mock generation, expectations and failure detection work.
7. Docs:
   - `docs/modules/Platform.md` (from the `module_doc.md` template);
   - `docs/unit_testing.md` (how to build and run the tests, and how to add a module
     test);
   - `docs/setup_host.md` (Ruby);
   - `docs/operations.md` (`src/ecu` replaces the notes on the removed bring-up);
   - `docs/autosar_compliance.md` (sections for the three platform documents);
   - `.claude/skills/verify/SKILL.md` (the final test commands);
   - `.claude/tasks/backlog.md`;
   - `docs/development_log.md`.
8. Host setup: install Ruby (`sudo apt install ruby`), only after approval.

### Out of scope
- Any AUTOSAR module other than the platform headers: `Det`, `EcuM`, MCAL and the rest.
  `main.c` must not call AUTOSAR APIs yet.
- `ComStack_Types.h`, `<Ma>_MemMap.h` (DEV-02).
- MISRA static analysis (cppcheck), code coverage, CI.
- On-target tests (`tests/target/`).
- A `test` sub-command in `scripts/operations.py`. This can be a later task.
- Replacing the SDK board port (`BOARD_InitHardware`, pin mux, clocks) with `Mcu`/`Port`.
  That belongs to TASK-005/006.

## 4. Files (expected)

| File | Action |
|---|---|
| `src/platform/include/Platform_Types.h` | add |
| `src/platform/include/Std_Types.h` | add |
| `src/platform/include/Compiler.h` | add |
| `src/platform/include/Compiler_Cfg.h` | add |
| `src/ecu/CMakeLists.txt` | add |
| `src/ecu/prj.conf` | add |
| `src/ecu/main.c` | add |
| `cmake/xat_module.cmake` | add |
| `cmake/xat_unit_test.cmake` | add |
| `third_party/unity/**`, `third_party/cmock/**` | add (vendored) |
| `third_party/README.md` | change (versions) |
| `tests/unit/CMakeLists.txt`, `tests/unit/cmock.yml` | add |
| `tests/unit/platform/test_Platform_Types.c` (+ `CMakeLists.txt` if per-directory) | add |
| `tests/unit/framework/test_CMock_Selftest.c`, `tests/unit/framework/dummy_dep.h` | add |
| `docs/modules/Platform.md`, `docs/unit_testing.md` | add |
| `docs/setup_host.md`, `docs/operations.md`, `docs/autosar_compliance.md`, `docs/development_log.md` | change |
| `.claude/skills/verify/SKILL.md`, `.claude/tasks/backlog.md` | change |
| `.gitkeep` in directories that get content | remove |

## 5. Requirements

SWS IDs are taken from the R21-11 PDFs during planning and added to the compliance matrix.

| ID | Requirement |
|---|---|
| REQ-003-01 | `uint8/16/32/64` and `sint8/16/32/64` are exactly 8/16/32/64 bits wide, with the right signedness, on **both** the target (ILP32) and the host (LP64). |
| REQ-003-02 | `boolean` is an 8-bit unsigned type. `TRUE` is 1 and `FALSE` is 0. |
| REQ-003-03 | `CPU_TYPE` = `CPU_TYPE_32`, `CPU_BIT_ORDER` = `LSB_FIRST`, `CPU_BYTE_ORDER` = `LOW_BYTE_FIRST` for the MCXN947. |
| REQ-003-04 | `Std_ReturnType` is `uint8`. `E_OK` is 0 and `E_NOT_OK` is 1. `STD_HIGH`/`STD_LOW`, `STD_ACTIVE`/`STD_IDLE` and `STD_ON`/`STD_OFF` have the SWS values. |
| REQ-003-05 | `Std_VersionInfoType` has the SWS members (`vendorID`, `moduleID`, `sw_major_version`, `sw_minor_version`, `sw_patch_version`). |
| REQ-003-06 | Each platform header publishes AR release version 4.7.0 and a SW version. |
| REQ-003-07 | `NULL_PTR` is `((void *)0)`. `INLINE` and `LOCAL_INLINE` work with C11 GCC (`inline`, `static inline`). |
| REQ-003-08 | Every header compiles alone, is protected by an include guard, and builds without warnings with the strict flag set. |
| REQ-003-09 | The firmware builds with `ops build`, and the SDK is not modified. |
| REQ-003-10 | Adding a module to the firmware takes one `xat_add_module(...)` call. Adding a module test with mocked dependencies takes one `xat_add_unit_test(...)` call. |
| REQ-003-11 | A failing assertion or an unmet CMock expectation makes `ctest` fail. This is checked once by hand and recorded in the development log. |

## 6. Acceptance criteria

| AC | Criterion | How verified |
|---|---|---|
| AC0 | The plan was approved before any change or install | — |
| AC1 | The debug firmware builds with no warnings from project sources and produces `ecu_cm33_core0.elf/.bin/.map` | `python3 scripts/operations.py build --clean` |
| AC2 | The release firmware builds | `python3 scripts/operations.py --config release build --clean` |
| AC3 | On the board, the firmware boots and prints the banner | `python3 scripts/operations.py run --expect "xAT ecu: boot OK"` |
| AC4 | The host unit tests (platform + CMock self-test) pass | `cmake -G Ninja -S tests/unit -B build/unit && cmake --build build/unit && ctest --test-dir build/unit --output-on-failure` |
| AC5 | A deliberately broken assertion and a deliberately unmet expectation each make `ctest` exit non-zero. Both are reverted afterwards. | manual, recorded in the log |
| AC6 | No layering violation. The platform headers include only C standard headers. | `verify` skill, step 3 |
| AC7 | The SDK is unchanged | SHA-256 of the SDK tree before and after, as in TASK-001 |
| AC8 | The third-party code is pristine and the versions are recorded | compare against the release archive or tag; `third_party/README.md` |
| AC9 | Docs updated (section 3.7) | review |

## 7. Open questions

1. **How to vendor Unity/CMock.** Option (a): copy a release snapshot into `third_party/`.
   It is simple, needs no network at build time, and the tag is recorded in the README.
   Option (b): git submodules. *Proposal: (a).*
2. **Ruby.** May `sudo apt install ruby` (Ubuntu 3.2) be run on the host?
3. **Board port.** Reuse the SDK `hello_world` board port (`BOARD_InitHardware`: clocks,
   pins, debug console) until `Mcu`/`Port` take over? *Proposal: yes. Record it as a
   temporary integration shortcut in `docs/modules/Platform.md` or the development log.*
4. **Boot banner.** Is it OK for `src/ecu/main.c` (integration layer) to use the SDK debug
   console (`PRINTF`) for the banner? *Proposal: yes. The layering rules allow
   `src/ecu` to use the SDK.*
5. **CMock self-test.** Keep `tests/unit/framework/` permanently as a guard for the test
   infrastructure, or delete it once the first real module test uses mocks?
   *Proposal: keep it.*
