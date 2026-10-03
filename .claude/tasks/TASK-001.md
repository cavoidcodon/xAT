# TASK-001: Build / Flash / Debug tooling for FRDM-MCXN947 (single Python script, OOP + Strategy)

| Field        | Value                                                         |
|--------------|---------------------------------------------------------------|
| Status       | TODO                                                          |
| Priority     | P0 (blocks every later task)                                  |
| Depends on   | none                                                          |
| Board / MCU  | FRDM-MCXN947 / MCXN947, core `cm33_core0`                     |
| Host         | Ubuntu on Raspberry Pi (**aarch64**)                          |
| SDK          | MCUXpresso SDK 26.09.00 at `workspace/mcuxsdk` (read-only)    |

---

## 0. Planning gate (mandatory, do this first)

Do **not** create, modify or delete any file, and do **not** install anything, until the user has approved a written plan.

1. Read this task, `CLAUDE.md` and `claude/rules/coding_rules.md`.
2. Investigate with **read-only** actions only (reading files, `ls`, `lsusb`, `which`, `--version` checks, etc.) to resolve the open questions in §8 as far as possible.
3. Present a plan to the user that contains:
   - the final file list (added / changed), confirming or adjusting §4;
   - the class layout inside `scripts/operations.py` (classes, responsibilities, how strategies are selected);
   - every host setup command that needs `sudo` or installs software, listed explicitly;
   - how the out-of-tree SDK CMake build will work (result of the §8 investigation);
   - the verification steps mapped to the acceptance criteria in §6;
   - known risks and the questions that need a decision from the user.
4. Stop and wait for approval. If the user changes the plan, present the updated plan before starting.
5. After approval, follow the approved plan. If something forces a deviation (e.g. a tool does not work on aarch64), stop, explain it, and get approval for the change before continuing.

## 1. Goal

Create the first piece of the xAT project: **one** Python script, `scripts/operations.py`, that **builds**, **flashes** and **debugs** firmware on the connected FRDM-MCXN947 board, using the NXP SDK in `workspace/mcuxsdk` as input.

Use the script to bring up a minimal smoke-test firmware that prints a message over the debug UART. That proves the whole toolchain → probe → board → terminal path works before any AUTOSAR module is written.

## 2. Context (verified on 2026-10-03)

- **SDK layout.** `workspace/mcuxsdk` is a west workspace (`.west/config` → `manifests/west.yml`). It uses the CMake + Kconfig build flow:
  - `mcuxsdk/cmake/extension/mcux.cmake`
  - `mcuxsdk/cmake/toolchain/armgcc.cmake`
- **Reference example.** `mcuxsdk/examples/demo_apps/hello_world`, with its board port in `mcuxsdk/examples/_boards/frdmmcxn947/demo_apps/hello_world/` (`pin_mux.c`, `prj.conf`, `reconfig.cmake`). Its target is `frdmmcxn947@cm33_core0`.
- **Linker script and startup:** `mcuxsdk/devices/MCX/MCXN/MCXN947/gcc/MCXN947_cm33_core0_flash.ld` and `startup_MCXN947_cm33_core0.S`.
- **Debug runners in the SDK:** `mcuxsdk/cmake/extension/runner/{linkserver,jlink,pyocd,openocd}.board.cmake`. The board file `board_runner.cmake` configures LinkServer (`MCXN947:FRDM-MCXN947`) and J-Link.
- **Probe.** The on-board probe enumerates as `1fc9:0143 NXP MCU-LINK FRDM-MCXN947 CMSIS-DAP V3.128`. The VCOM UART is expected at `/dev/ttyACM*`, 115200 8N1.
- **Missing tools.** None of these are installed yet: `arm-none-eabi-gcc`, `arm-none-eabi-gdb` / `gdb-multiarch`, `cmake`, `ninja`, `west`, `pyocd`, `LinkServer`, `pip`.
- **aarch64 limits.**
  - NXP LinkServer and SEGGER J-Link may not be available or supported on aarch64 Linux. Verify this before relying on them.
  - pyOCD (CMSIS-DAP) is the expected default flash/debug backend on this host.
  - Use the Arm GNU Toolchain build for an **AArch64 Linux host** (`arm-gnu-toolchain-*-aarch64-arm-none-eabi`).

## 3. Scope

### In scope
1. Set up the host prerequisites, and document every step in `xAT/docs/setup_host.md`.
2. Write the tool as a **single Python file**, `scripts/operations.py` (OOP, Strategy pattern), with the `build`, `flash`, `debug`, `monitor`, `run` and `doctor` sub-commands.
3. Write the smoke-test firmware `hello_uart`. It prints over the debug UART (LPFLEXCOMM4 / VCOM, same as the SDK `hello_world`).
4. Write unit tests for the script.
5. On the real board, verify that build, flash, UART output and a GDB session all work.

### Out of scope
- Any AUTOSAR module (MCAL/BSW/RTE/application).
- Multicore (`cm33_core1`), TrustZone secure/non-secure split, and XIP/QSPI variants.
- CI integration.
- Splitting the tool into a Python package or multiple modules.
- Any modification of files under `workspace/mcuxsdk/` (vendor SDK, read-only).

## 4. Deliverables (proposed layout; confirm in the plan)

```
xAT/
├── scripts/
│   └── operations.py              # the whole tool: CLI + config + strategies (single file)
├── tests/scripts/
│   └── test_operations.py         # pytest unit tests for operations.py (no hardware)
├── src/hello_uart/                # smoke-test firmware
│   ├── CMakeLists.txt             # out-of-tree project using SdkRootDirPath
│   ├── prj.conf
│   ├── main.c
│   └── gdb/smoke_test.gdb         # scripted GDB session used for verification
└── docs/
    ├── setup_host.md              # host setup + Python dependencies (pyocd, pyserial, pytest)
    ├── operations.md              # usage + class diagram
    └── development_log.md
```

No other Python files are allowed for the tool. Project defaults (board, core, toolchain paths, backends, pyOCD settings) live in a clearly marked defaults section (a frozen dataclass) at the top of `operations.py`, and can be overridden by environment variables and CLI options.

## 5. Design requirements

### 5.1 OOP + Strategy pattern (all inside `scripts/operations.py`)
Group the file into clearly commented sections, in this order: defaults/config, errors, `CommandRunner`, value objects, strategy interfaces, strategy implementations, registry, `Target`, `SerialMonitor`, `GdbClient`, CLI, `main()`.

- **Context.** `Target` holds the resolved configuration (SDK root, board, core, build dir, ELF path). It also holds one strategy per concern:
  - `build_strategy: BuildStrategy`
  - `flash_strategy: FlashStrategy`
  - `debug_strategy: DebugServerStrategy`

  `Target` delegates and never contains tool-specific logic.
- **Strategy interfaces** (abstract base classes, using `abc.ABC`):
  - `BuildStrategy`: `configure()`, `build()`, `clean()`, `artifacts() -> BuildArtifacts` (elf, bin, hex, map).
    - Implementations: `CMakeNinjaBuild` (default), `WestBuild` (optional, only if west is installed).
  - `FlashStrategy`: `is_available() -> bool`, `probes() -> list[ProbeInfo]`, `flash(image, verify=True)`, `erase()`, `reset()`.
    - Implementations: `PyOcdFlash` (default on aarch64), `LinkServerFlash`, `JLinkFlash`.
  - `DebugServerStrategy`: `is_available()`, `start() -> GdbEndpoint`, `stop()`. Implement it as a context manager, so the server is always killed.
    - Implementations: `PyOcdGdbServer` (default, port 3333), `LinkServerGdbServer`, `JLinkGdbServer`.
  - `GdbClient` (not a strategy) takes `GdbEndpoint` + ELF and runs GDB interactively or in batch mode (`-batch -x script.gdb`).
  - `SerialMonitor` (pyserial), with expect/timeout.
- **Selection.**
  - Strategies are registered by name in a registry (dicts mapping name → class) in the same file.
  - The user can choose one with `--builder`, `--flasher` and `--debugger`, or with the defaults section.
  - `auto` picks the first backend whose `is_available()` returns True, in a fixed order: pyocd → linkserver → jlink.
- **Adding a backend** must mean adding one class plus one registry entry, with no edits to `Target` or the CLI code (Open/Closed principle). Prove this in `docs/operations.md` with the class diagram.
- **External commands.** All of them go through `CommandRunner`. It logs the exact command line and raises `OperationsError` with stdout/stderr on a non-zero exit. Tests can inject a fake runner.
- **Code quality:**
  - Python ≥ 3.10, type hints everywhere, `dataclasses` for config/value objects.
  - `logging` instead of `print` (except CLI output).
  - No hard-coded absolute paths. Paths are derived from the script location, environment (`ARMGCC_DIR`, `XAT_SDK_ROOT`) or the CLI.
  - Only the standard library, plus `pyserial` and `pyocd` (pyocd used through its CLI or its API, inside its strategy only). Import optional dependencies lazily, so `doctor` and `--help` work even when they are missing.
  - Runnable as `python3 scripts/operations.py <command>` from the `xAT/` root.

### 5.2 CLI (minimum)
```
python3 scripts/operations.py doctor                           # check toolchain, cmake, ninja, gdb, probe, serial port; print a report
python3 scripts/operations.py build   [--config debug|release] [--clean] [--app src/hello_uart]
python3 scripts/operations.py flash   [--flasher auto|pyocd|linkserver|jlink] [--probe <uid>] [--no-verify]
python3 scripts/operations.py debug   [--debugger auto|pyocd|...] [--batch <script.gdb>] [--port 3333]
python3 scripts/operations.py monitor [--port /dev/ttyACM0] [--baud 115200] [--expect "<text>"] [--timeout 10]
python3 scripts/operations.py run     # build + flash + monitor --expect (one-shot smoke test)
```
- **Exit codes:** `0` means success, and a non-zero code means failure. `monitor --expect` returns non-zero on timeout, so the tool can be used in scripts and tests.

### 5.3 Build specifics
- Build out-of-tree. `-DSdkRootDirPath=<workspace>/mcuxsdk`, `-Dboard=frdmmcxn947`, `-Dcore_id=cm33_core0`.
- Toolchain file: `${SdkRootDirPath}/cmake/toolchain/armgcc.cmake`, with `ARMGCC_DIR` exported.
- Generator: Ninja.
- Build dir: `xAT/build/<app>/<config>/`. It must not be inside the SDK.
- Follow how `mcuxsdk/examples/demo_apps/hello_world/CMakeLists.txt` integrates with `mcux.cmake`. Reuse the board files (`board.c`, `clock_config.c`, `pin_mux.c`) through SDK CMake/Kconfig, and copy files into `src/hello_uart/` only where unavoidable, with the reason documented.
- Produce `.elf` and `.bin`. A debug build uses `-O0 -g3`.

### 5.4 Smoke-test firmware `src/hello_uart`
- Initialise the board (`BOARD_InitHardware()` or equivalent from the SDK board layer) and the debug console.
- Print `xAT hello_uart: boot OK` once, then `xAT tick <n>` about once per second.
- Keep a global counter `g_tickCount` (named per `claude/rules/coding_rules.md`), so GDB can inspect it.
- C11, follows `claude/rules/coding_rules.md`.
- **Exception to the CLAUDE.md layering rule.** This is a **bring-up program, not AUTOSAR application code**, so it may include NXP SDK headers directly. Document this in its header comment and in the development log.

## 6. Acceptance criteria

All of these must pass on the real board connected to this Raspberry Pi. `ops` below stands for `python3 scripts/operations.py`.

| # | Criterion | How to verify |
|---|-----------|---------------|
| AC0 | A plan was presented and approved by the user before any file was changed or any software was installed | approval noted in `docs/development_log.md` |
| AC1 | `ops doctor` reports every required tool as found, and detects the MCU-Link probe and VCOM port | command output, exit code 0 |
| AC2 | `ops build --clean` builds `hello_uart` from scratch, producing `.elf` + `.bin` | exit code 0; `arm-none-eabi-size` output in the log |
| AC3 | `ops flash` programs and verifies the image, then resets the MCU | exit code 0; flasher log shows the verify passed |
| AC4 | After reset, the UART shows `xAT hello_uart: boot OK` followed by increasing `xAT tick <n>` lines | `ops monitor --expect "boot OK" --timeout 10` exits 0 |
| AC5 | `ops run` performs AC2 to AC4 in one command | exit code 0 |
| AC6 | GDB connects through the selected debug server, and in batch mode: loads symbols, sets a breakpoint at `main`, resets/halts, continues to `main`, reads `g_tickCount`, steps, resumes, then detaches cleanly. The debug server process is terminated afterwards. | `ops debug --batch src/hello_uart/gdb/smoke_test.gdb` exits 0; transcript saved in `docs/development_log.md` |
| AC7 | Interactive `ops debug` gives a usable `(gdb)` prompt attached to the target | manual check, noted in log |
| AC8 | Unit tests pass, cover every strategy class and `Target` with a fake `CommandRunner`, and need no hardware | `python -m pytest tests/scripts` |
| AC9 | The tool is exactly one Python file, `scripts/operations.py` | `find . -name '*.py' -not -path './tests/*' -not -path './.venv/*'` lists only it |
| AC10 | No file under `workspace/mcuxsdk/` was modified | `git -C mcuxsdk status` / checksum comparison |
| AC11 | Docs exist: `setup_host.md`, `operations.md` (usage + class diagram), and a `development_log.md` entry | review |

## 7. Implementation steps (only after the plan in §0 is approved)

1. **Host setup** (list every command in the plan; get approval before installing anything system-wide):
   - Arm GNU Toolchain (AArch64 host → arm-none-eabi). Record the version.
   - `cmake` ≥ 3.22 and `ninja`, through apt.
   - `python3-venv` and `python3-pip`. Create a venv at `xAT/.venv`, then install `pyocd`, `pyserial` and `pytest` (versions recorded in `docs/setup_host.md`).
   - pyOCD target support for MCXN947: check `pyocd list --targets | grep -i mcxn947`. If it is missing, install the `NXP::MCXN947_DFP` pack (`pyocd pack install mcxn947`).
   - Add udev rules for the MCU-Link probe (`1fc9:0143`), and add the user to `dialout` for `/dev/ttyACM*`.
   - Optional: check whether LinkServer has an aarch64 Linux build. If it does not, document that and leave the strategy as "unavailable on this host".
2. Build `hello_world` from the SDK manually once with cmake + ninja (build dir outside the SDK), to confirm the toolchain and SDK work before writing any Python.
3. Write the `src/hello_uart` CMake project outside the SDK and build it manually.
4. Implement the core of `operations.py`: defaults, `CommandRunner`, errors, `Target`, registry, CLI skeleton, and `doctor`.
5. Implement `CMakeNinjaBuild`, plus its unit tests.
6. Implement `PyOcdFlash` and `SerialMonitor`, plus their unit tests, then verify AC3 and AC4 on hardware.
7. Implement `PyOcdGdbServer` and `GdbClient`, plus their unit tests and `smoke_test.gdb`, then verify AC6 and AC7.
8. Implement the LinkServer and J-Link strategies (with `is_available()` and unit tests). Verify them on hardware only if those tools work on this host.
9. Write `run`, write the docs, and run the full acceptance pass.

## 8. Risks and open questions (investigate read-only during §0, answer in the plan)

- **LinkServer / J-Link on aarch64.** They may be unavailable, which makes pyOCD the only hardware-verified backend. That is acceptable for this task, but must be documented.
- **pyOCD and MCXN947.** Pack support and the reset type are uncertain (MCXN947 may need a `reset_type` / `connect_mode` tweak). Record whatever settings worked in the defaults section of `operations.py`.
- **SDK CMake flow for an out-of-tree app.** `mcux.cmake` expects the `examples/_boards/<board>/...` layout (`PROJECT_BOARD_PORT_PATH`), so it may need extra variables to accept an app outside `mcuxsdk/examples`. Investigate before the plan is written, so `CMakeNinjaBuild` can be designed with it.
- **GDB binary.** Arm GNU Toolchain 14.x+ builds of `arm-none-eabi-gdb` need `libpython`/`libncursesw` of matching versions. `gdb-multiarch` from apt is the fallback, and should be configurable (defaults section / CLI option).
- **Single-file size.** Keeping every strategy in one file makes it long. Keep it readable with the section order from §5.1, and keep each class small.
- **The workspace is not a git repository yet.** CLAUDE.md workflow step 8 ("Review git diff") needs `git init` in `xAT/`. Ask the user as part of the plan.

## 9. Definition of Done
- The plan was approved before implementation (AC0).
- All acceptance criteria AC0–AC11 are met, and the evidence (command output) is recorded in `docs/development_log.md`.
- The final report lists the changed and added files, the verification results, and the remaining risks (CLAUDE.md workflow step 10).
