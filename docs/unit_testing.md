# Host unit tests (Unity + CMock)

The unit tests run on the Raspberry Pi host, not on the board. They are built with the
native `gcc` (13.3.0, aarch64) and the same strict warning flags as the firmware. The rules
for writing tests are in `.claude/rules/testing_rules.md`.

| Part | Version / location |
|---|---|
| Unity (test framework) | v2.7.0, `third_party/unity/` |
| CMock (mock generator) | v2.7.0, `third_party/cmock/` |
| Ruby (runs CMock and the Unity runner generator) | 3.2.3, `docs/setup_host.md` section 5 |
| CMake helpers | `cmake/xat_unit_test.cmake` |
| Shared CMock settings | `tests/unit/cmock.yml` |

## Build and run

From the `xAT/` root, with the CMake / Ninja of the venv:

```sh
export PATH=$PWD/.venv/bin:$PATH
cmake -G Ninja -S tests/unit -B build/unit
cmake --build build/unit
ctest --test-dir build/unit --output-on-failure
```

`ctest` exits non-zero when any test fails. A test executable can also be run directly, for
example `build/unit/platform/test_Std_Types`; it prints one line per failure and the Unity
summary (`N Tests M Failures 0 Ignored`), and its exit code is the number of failures.

## Layout

```
tests/unit/
├── CMakeLists.txt            project, includes cmake/xat_unit_test.cmake, add_subdirectory()
├── cmock.yml                 shared CMock settings (also read by the runner generator)
├── framework/                self-test of Unity + CMock (kept permanently)
├── platform/                 Platform_Types.h, Std_Types.h, Compiler.h
└── <layer>/<Ma>/             one directory per module, mirroring src/
    ├── CMakeLists.txt
    └── test_<Ma>.c
```

Generated files (mocks, runners, header checks) are written into `build/unit/`, never into
the source tree.

## Adding a module test

1. Create `tests/unit/<layer>/<Ma>/test_<Ma>.c` (the `new-module` skill does this) and
   `tests/unit/<layer>/<Ma>/CMakeLists.txt` with one call:

   ```cmake
   set(MA_DIR ${XAT_ROOT}/src/mcal/Dio)

   xat_add_unit_test(NAME test_Dio
       SOURCES ${MA_DIR}/src/Dio.c ${MA_DIR}/config/Dio_Cfg.c test_Dio.c
       MOCKS ${XAT_ROOT}/src/bsw/services/Det/include/Det.h
             ${XAT_ROOT}/tests/unit/stubs/sdk/fsl_gpio.h
       INCLUDES ${MA_DIR}/include ${MA_DIR}/config)

   xat_add_compile_check(NAME compile_check_Dio
       HEADERS ${MA_DIR}/include/Dio.h ${MA_DIR}/config/Dio_Cfg.h
       INCLUDES ${MA_DIR}/include ${MA_DIR}/config)
   ```

2. Add `add_subdirectory(<layer>/<Ma>)` to `tests/unit/CMakeLists.txt`.

### `xat_add_unit_test`

```cmake
xat_add_unit_test(NAME <test> SOURCES <module .c files + one test_*.c>
                  [MOCKS <headers>] [INCLUDES <dirs>] [DEFINES <defs>] [PASS_REGEX <regex>])
```

- Each header in `MOCKS` becomes `mock_<header>.c/.h`; the test includes
  `mock_<header>.h` instead of the real header. Every test has its own mock directory, so
  two tests can mock the same header.
- The Unity runner (`main()`, `setUp` / `tearDown`, CMock init / verify / destroy around
  each test) is generated from the `test_*.c` file.
- `src/platform/include` is always on the include path.
- `PASS_REGEX` is only for guard tests that must fail (see `framework/`).

The mock functions follow the CMock plugins in `cmock.yml`. For
`Std_ReturnType DummyDep_Read(uint8 channel, uint8 *value)` they are, for example:

| Call | Meaning |
|---|---|
| `DummyDep_Read_ExpectAndReturn(1U, ptr, E_OK)` | the next call must be `DummyDep_Read(1U, ...)`, with `*value` equal to `*ptr`; it returns `E_OK` |
| `DummyDep_Read_IgnoreArg_value()` | do not check `value` in the last expectation |
| `DummyDep_Read_ExpectAnyArgsAndReturn(E_OK)` | expect one call with any arguments |
| `DummyDep_Read_ReturnThruPtr_value(&v)` | write `v` through `value` |
| `DummyDep_Read_StubWithCallback(fn)` | call `fn` instead |

Calls must come in the expected order (`enforce_strict_ordering`), an unexpected call fails
the test (`fail_on_unexpected_calls`), and an expectation that is never met fails the test
at the end of it.

### `xat_add_compile_check`

```cmake
xat_add_compile_check(NAME <check> [HEADERS <headers>] [SOURCES <files>] [INCLUDES <dirs>])
```

Compile-only checks that are part of the normal build, so a failure stops
`cmake --build`:

- each header in `HEADERS` is compiled on its own, included twice, and its include guard
  `<FILE_NAME>_H` must be defined (`coding_rules.md` 4.1, 4.2);
- each file in `SOURCES` is compiled as it is, e.g. `platform/compile_checks/` checks that
  `TRUE` / `FALSE` or `E_OK` defined before the platform headers do not clash.

## Compiler flags

Test files, module sources, generated mocks and runners are compiled with
`-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`, the same list as the
firmware (`XAT_PROJECT_C_FLAGS` in `cmake/xat_module.cmake`). Only Unity and CMock
themselves keep their defaults.

The host is LP64 and the target ILP32. `Platform_Types.h` maps the AUTOSAR integer types to
the `<stdint.h>` exact-width types, so `uint32` and the other types have the same width on
both. A test must not depend on the width of `int`, `long` or pointers.

## Failure detection (guard tests)

`framework/test_CMock_Guard.c` holds two tests that must fail: a failing assertion and an
expectation that is never called. CTest runs it with `PASS_REGEX`, so it passes only when
both failures and the Unity summary `2 Tests 2 Failures 0 Ignored` appear in the output. If
Unity or CMock ever stopped detecting failures, this test would fail.

## Requirement tags

Every test has the requirement IDs it covers in a comment above it, and the code has them
above the definitions that implement them (rule: `coding_rules.md` 8.6, `testing_rules.md`):

```c
/* [SWS_Platform_00015] [REQ-003-01] */
void test_Uint32_WidthAndRange_Is32BitUnsigned(void)
```

List every tag in the code and the tests:

```sh
grep -rnoE '\[(SWS_[A-Za-z]+_[0-9]{5}|SG-[0-9]{2}|(SYS|SWR|SIM)-[A-Z]+-[0-9]{3}|REQ-[0-9]{3}-[0-9]{2})\]' src tests cmake | sort
```

The SWS requirement -> code -> test matrix is `docs/autosar_compliance.md`.
