# Development log

## 2026-10-03: TASK-001 build / flash / debug tooling

### Summary

- Added `scripts/operations.py`: a single-file tool (OOP + Strategy pattern) with the
  `doctor`, `build`, `flash`, `debug`, `monitor` and `run` sub-commands. Backends are
  CMake+Ninja / west, pyOCD, NXP LinkServer and SEGGER J-Link.
- Added the bring-up firmware `src/hello_uart`, built out of tree against
  MCUXpresso SDK 26.09.00. It prints `xAT hello_uart: boot OK`, then `xAT tick <n>` about
  once per second.
- Added `src/hello_uart/gdb/smoke_test.gdb`, 47 unit tests (`tests/scripts/test_operations.py`)
  and the docs `setup_host.md` and `operations.md`.
- Host: Arm GNU Toolchain 15.2.rel1 (aarch64 host), a venv with cmake 3.31.10, ninja
  1.13.2, pyOCD 0.45.1 + `NXP.MCXN947_DFP` 26.09.00, pyserial and pytest, and NXP
  LinkServer 26.9.130 (ARM64). Details are in [setup_host.md](setup_host.md).
- Ran `git init` in `xAT/` and added a `.gitignore` (`build/`, `.venv/`, Python caches).
  Nothing has been committed yet.

### Plan approval (AC0)

The plan was presented before any file was changed or any software was installed. The
user approved it on 2026-10-03 with these decisions:

- "Approve as written".
- CMake/Ninja: "pip in .venv" (the apt versions 3.28.3 / 1.11.1 are below the SDK
  minimums of 3.30 / 1.12).
- git: "Yes, git init + .gitignore".
- LinkServer: "I'll download it". The user downloaded
  `LinkServer_26.9.130.aarch64.deb.bin`, asked for it to be installed and integrated, and
  asked for the sudo steps to be run for them.

Deviations from the approved plan, all explained to the user while they happened:

1. **udev / dialout.** The custom udev rule and `usermod -aG dialout` were dropped. The
   LinkServer installer's rules already give `0666` to the MCU-Link USB, HID and
   `ttyACM` devices. The manual rules are still documented for a pyOCD-only host.
2. **gdb-multiarch** was not installed, because the toolchain's `arm-none-eabi-gdb`
   (16.3.90) works. It is still supported as a fallback.
3. **pyOCD target.** `mcxn947vdf` comes from the CMSIS pack. pyOCD 0.45.1 has no built-in
   MCXN947 target.
4. **Extra venv packages** `jsonschema` and `PyYAML`. The SDK's build scripts use them to
   validate `tool.yml`; without them the build only prints a warning.

### Exception to the layering rule

`src/hello_uart/main.c` is a **bring-up program, not AUTOSAR application code**. As
TASK-001 §5.4 allows, it includes NXP SDK headers (`fsl_common.h`,
`fsl_debug_console.h`, `board.h`, `app.h`) directly. Its header comment documents this.

### Findings during bring-up

| Topic | Finding | Resolution |
|---|---|---|
| Out-of-tree SDK build | `PROJECT_BOARD_PORT_PATH` is resolved relative to `SdkRootDirPath` | point it at the SDK hello_world board port; no SDK files copied |
| Image name | SDK names the image `<project>_<core_id>` (`hello_uart_cm33_core0.elf`) | `CMakeNinjaBuild.artifacts()` looks for both names |
| Debug flags | SDK debug config uses `-O0 -g -std=gnu99` | app adds `-g3` (`mcux_add_configuration`) and `-std=c11` for `main.c` only |
| SDK tree pollution | SDK Kconfig scripts wrote `scripts/kconfig/__pycache__/*.pyc` into the SDK | build sets `PYTHONDONTWRITEBYTECODE=1`; the two generated `.pyc` files were removed |
| pyOCD verify | the `.bin` has zero padding `0x2b0..0x3ff` that is never programmed, so comparing it failed at `0x2b0` | verify compares each ELF `PT_LOAD` segment instead |
| pyOCD reset (GDB) | default/sw reset: each `continue` after `monitor reset halt` only single-steps | GDB server uses `reset_type=hw` |
| pyOCD reset (load) | `reset_type=hw` makes `ResetCatchClear` on cm33_core1 fail (SWD FAULT ACK), reproducibly | load/commander/reset keep the pack default |
| LinkServer GDB | PC stays in the flash driver in RAM; `monitor reset` fails without `load` | LinkServer session runs `load` + `monitor reset` (as the LinkServer Readme requires) |
| Cleanup | a SIGTERM to the tool used to leave the GDB server running | SIGTERM → `SystemExit`, so the context manager always stops the server |
| Async GDB in batch | `continue &` + `interrupt` is unreliable in `-batch` | the script resumes to a breakpoint on `SDK_DelayAtLeastUs` instead |

### Verification (on the real board, 2026-10-03)

`ops` = `python3 scripts/operations.py`. Full outputs are in `build/evidence/` (not committed).

| AC | Command | Result |
|---|---|---|
| AC0 | — | plan approved before any change (see above) |
| AC1 | `ops doctor` | exit 0, all required checks OK |
| AC2 | `ops build --clean` | exit 0, `.elf` + `.bin` + `.map`; release build also OK |
| AC3 | `ops flash --flasher pyocd` / `--flasher linkserver` | exit 0 / exit 0, verify passed, MCU reset |
| AC4 | `ops monitor --expect "boot OK" --timeout 10` | exit 0 (pyOCD and LinkServer reset); a timeout returns exit 3 |
| AC5 | `ops run --flasher pyocd` / `--flasher linkserver` | exit 0 / exit 0 (also `run --clean`) |
| AC6 | `ops debug --batch src/hello_uart/gdb/smoke_test.gdb` (pyOCD and LinkServer) | exit 0, `SMOKE_TEST_DONE`, no server process left |
| AC7 | `ops debug` (interactive) | `(gdb)` prompt, break at `main`, `print g_tickCount`, `detach`, `quit` checked through a pseudo-terminal (`script`) with LinkServer; **manual check by the user pending** |
| AC8 | `.venv/bin/python -m pytest tests/scripts` | 47 passed in 0.84 s |
| AC9 | `find . -name '*.py' -not -path './tests/*' -not -path './.venv/*'` | only `./scripts/operations.py` |
| AC10 | SHA-256 of all 123,843 SDK files, before and after | identical |
| AC11 | docs | `setup_host.md`, `operations.md` (usage + class diagram), this entry |

AC1, `ops doctor`:

```
[OK  ] SDK                      /home/cavoidpi5/workspace/mcuxsdk
[OK  ] ARMGCC_DIR               /home/cavoidpi5/workspace/tools/arm-gnu-toolchain-15.2.rel1-aarch64-arm-none-eabi
[OK  ] arm-none-eabi-gcc        arm-none-eabi-gcc (Arm GNU Toolchain 15.2.Rel1 (Build arm-15.86)) 15.2.1 20251203
[OK  ] cmake                    cmake version 3.31.10
[OK  ] ninja                    1.13.2.git.kitware.jobserver-pipe-1
[OK  ] gdb                      /home/cavoidpi5/workspace/tools/arm-gnu-toolchain-15.2.rel1-aarch64-arm-none-eabi/bin/arm-none-eabi-gdb
[OK  ] pyocd                    0.45.1
[OK  ] pyocd target mcxn947vdf  supported
[OK  ] probe                    Bus 004 Device 002: ID 1fc9:0143 NXP Semiconductors MCU-LINK FRDM-MCXN947 (r0E7) CMSIS-DAP V3.128
[OK  ] serial port              /dev/ttyACM0
[OK  ] flasher:pyocd            available
[OK  ] flasher:linkserver       available
[--  ] flasher:jlink            not installed
[OK  ] debugger:pyocd           available
[OK  ] debugger:linkserver      available
[--  ] debugger:jlink           not installed
[--  ] builder:west             not installed

all required checks passed
```

AC2, `arm-none-eabi-size` (debug, `-O0 -g3`):

```
   text	   data	    bss	    dec	    hex	filename
  17036	     20	   3452	  20508	   501c	build/hello_uart/debug/hello_uart_cm33_core0.elf
```

Release: `text 9672, data 20, bss 3452`.

AC3, flasher logs:

```
pyocd:      Erased 24576 bytes (3 sectors), programmed 17408 bytes (34 pages)
            INFO pyocd: verify passed (3 segments, 17056 bytes)
linkserver: Pb: (100) File matches flash.  Nt: ... verify succeeded
            INFO linkserver: verify passed
            INFO: Wire reset issued on probe DFALTHITCX1OJ (device MCXN947:FRDM-MCXN947)
```

AC4, UART after reset:

```
xAT hello_uart: boot OK
xAT tick 1
xAT tick 2
xAT tick 3
expected text found: 'boot OK'
```

AC6, transcript (LinkServer; the pyOCD transcript is the same apart from the reset lines
`Resetting target with halt` / `Successfully halted device on reset`):

```
0x20018040 in ?? ()
Loading section .interrupts, size 0x2b0 lma 0x0
Loading section .text, size 0x3fd4 lma 0x400
Loading section .ARM, size 0x8 lma 0x43d4
Loading section .init_array, size 0x4 lma 0x43dc
Loading section .fini_array, size 0x4 lma 0x43e0
Loading section .data, size 0xc lma 0x43e4
Start address 0x0000047c, load size 17056
Transfer rate: 18 KB/sec, 2436 bytes/write.
Processor connection reset

== continue to main ==
Breakpoint 1 at 0x3618: file /home/cavoidpi5/workspace/xAT/src/hello_uart/main.c, line 27.
Note: automatically using hardware breakpoints for read-only addresses.

Breakpoint 1, main () at /home/cavoidpi5/workspace/xAT/src/hello_uart/main.c:27
27	    BOARD_InitHardware();
pc             0x3618              0x3618 <main+4>

== read g_tickCount at main ==
$1 = 0

== step over board init and boot message ==
29	    (void)PRINTF("xAT hello_uart: boot OK\r\n");
33	        SDK_DelayAtLeastUs(HELLO_TICK_PERIOD_US, SystemCoreClock);
Line 33 of "/home/cavoidpi5/workspace/xAT/src/hello_uart/main.c" starts at address 0x3622 <main+14> and ends at 0x362e <main+26>.

== resume until one tick has been printed ==
Breakpoint 2 at 0x1c84: file /home/cavoidpi5/workspace/mcuxsdk/drivers/common/fsl_common_arm.c, line 221.

Breakpoint 2, SDK_DelayAtLeastUs (delayTime_us=1000000, coreClock_Hz=150000000) at /home/cavoidpi5/workspace/mcuxsdk/drivers/common/fsl_common_arm.c:221
221	    if (delayTime_us > 0U)

Breakpoint 2, SDK_DelayAtLeastUs (delayTime_us=1000000, coreClock_Hz=150000000) at /home/cavoidpi5/workspace/mcuxsdk/drivers/common/fsl_common_arm.c:221
221	    if (delayTime_us > 0U)
$2 = 1

== detach (target keeps running) ==
[Inferior 1 (Remote target) detached]

SMOKE_TEST_DONE
INFO stopping linkserver GDB server
```

After both runs, `ps` shows no `pyocd gdbserver` or `LinkServer gdbserver` process.

### Remaining risks / open points

- **AC7** still needs the manual interactive check by the user.
- **J-Link and west** are implemented and unit-tested, but not verified on hardware (not
  installed; J-Link needs a J-Link probe).
- The **MCU-Link firmware** is V3.128; LinkServer recommends 3.174. It was not updated.
- The pyOCD reset settings depend on pyOCD 0.45.1 and pack 26.09.00. Re-check them after
  upgrading.
- `monitor` resets the MCU after opening the port, so a few lines from the previous run can
  appear before `boot OK`. The first byte after reset is sometimes a glitch character on
  the UART.
- LinkServer leaves its shared `redlinkserv` daemon running after flash/reset commands.
  That is by design and not a GDB server.

## 2026-10-03: TASK-002 project skeleton and Claude Code structure

### Summary

- Renamed `claude/` to `.claude/`, so Claude Code loads rules, skills and agents
  automatically.
- Created the AUTOSAR folder skeleton: `src/{platform,mcal,bsw,os,rte,app,ecu}`, with
  `include/ src/ config/` for each planned module, plus `tests/{unit,target}`,
  `third_party/` and `cmake/`. Empty directories hold a `.gitkeep`.
- Rewrote `.claude/rules/coding_rules.md` to follow AUTOSAR R21-11 and MISRA C:2012. It
  covers naming (SWS names take precedence), types, file structure, published version
  info, DET, control flow and formatting, and lists the project deviations DEV-01..03.
- Added the path-scoped rules `layering.md`, `mcal_rules.md`, `app_rules.md` and
  `testing_rules.md`.
- Added the skills `new-module` (with code, test and doc templates), `module-task` and
  `verify`.
- Added `.claude/tasks/TASK_TEMPLATE.md` and `.claude/tasks/backlog.md`.
- Added `docs/architecture/overview.md`, `docs/autosar_compliance.md` and
  `docs/references/autosar/README.md` (the PDFs are git-ignored), plus
  `third_party/README.md`.
- Updated `CLAUDE.md` with the AUTOSAR release, the test framework, the repository
  layout, and the new rules/skills paths.

### Decisions (user, 2026-10-03)

- AUTOSAR release: **R21-11** (AR 4.7.0). Unit tests: **Unity + CMock**.
- For SWS-defined names, the AUTOSAR naming takes precedence over the project naming
  rules.
- Removed the bring-up firmware `src/hello_uart` (and its build output). The project
  starts directly with AUTOSAR code. The firmware entry is now `src/ecu`, and the default
  `--app` in `scripts/operations.py` and the VS Code CMake source directory were changed
  to match. The out-of-tree SDK build notes were kept in `docs/operations.md`.
- `tests/scripts/test_operations.py` was deleted by the user and will not be restored.
  The Tests section of `docs/operations.md` was removed.

### Verification

None was requested for this skeleton task. Note that `ops build` fails until TASK-003
adds `src/ecu/CMakeLists.txt`.
