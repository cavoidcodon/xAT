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

## 2026-10-04: System specification (spec-driven development), first draft

### Summary

- Added `docs/specs/` for spec-driven development: `README.md` (process, ID scheme,
  status, verification methods), `templates/requirements_template.md`,
  `system/system_context.md`, `system/safety_goals.md` (SG-01..06),
  `system/system_requirements.md` (SEL, SFT, ACT, SEN, FLT, COM, TIM, STA, HMI, DIA, BOOT),
  `system/can_matrix.md`, `system/reprogramming.md`, `simulation/sim_requirements.md`
  (PLT, GW, CON, TST) and `traceability.md`. Everything is `Draft`; open values are
  marked `[NC: ...]` or `[NEEDS CLARIFICATION: ...]`.
- `TASK_TEMPLATE.md`: requirements table has a "Parent (SYS / SWR)" column.
- `new-module/templates/module_doc.md`: new section "Derived requirements".
- `docs/architecture/overview.md`: link to `docs/specs/`.

### Decisions (user, 2026-10-03 .. 2026-10-04)

- Spec-kit itself is not installed; its method (specify, clarify, analyze, approval gate)
  is applied with the existing task and skill structure.
- No real sensors / actuators: all TCU I/O is exchanged over CAN (500 kbit/s) with a
  plant model on the Pi5, through an FRDM-MCXN236 acting as a transparent SLCAN gateway.
  Both boards have an onboard TJA1057 transceiver.
- 4-speed transmission, selector P/R/N/D, limp-home in 3rd gear; no torque converter
  lockup and no manual mode in v1.
- Adc / Pwm / Icu are not needed. Design constraint DC-01 keeps the SWCs independent of
  the signal source so that hardware I/O can be added later.
- Firmware update over CAN with UDS: PBL (flash, immutable) + SBL (downloaded to RAM,
  only part with flash erase / write code); application uses `CanTp` + `Dcm`.
  v1 security: simple seed / key and CRC32.

### Verification

Documentation only; no build or test run. The user will review the content.

### Remaining risks / open points

- Open questions are listed at the end of `system_requirements.md`, `reprogramming.md`
  and `sim_requirements.md` (notably the no-init RAM / SBL RAM addresses and whether the
  MCXN947 boot ROM or SRAM ECC init clears RAM on a software reset).
- `backlog.md`, `CLAUDE.md` (repository layout for `tools/sim/` and `src/boot/`) and
  `scripts/operations.py` (second board) are not updated yet; planned as a separate step
  after the spec review.

## 2026-10-04: Reprogramming robustness (spec update)

### Summary

- The MCXN947 reference manual is not available, so RAM content after reset, boot ROM RAM
  usage and erased-flash read behaviour are unknown. Findings from the SDK: RAM ECC per
  bank (`SYSCON->ECC_ENABLE_CTRL`, disabled by `SystemInit()`), reset source in
  `CMC->SRS` / `SSRS`, flash ECC with bus error on multi-bit errors
  (`SYSCON->NVM_CTRL`), flash driver provided as ROM API.
- Decision (user): no board experiment; use a design that does not depend on these
  unknowns.
- `system_requirements.md`: SYS-BOOT-003 (validated RAM boot record, only after a
  software reset), SYS-BOOT-004 (only the SBL calls the ROM flash API), new SYS-BOOT-011
  (boot window), SYS-BOOT-012 (reset loop detection), SYS-BOOT-013 (fault-free application
  check).
- `reprogramming.md` (v0.2): sections 8 (RAM boot record), 9 (PBL start-up decision),
  10 (robustness rules RB-01..06); later sections renumbered.
- `traceability.md` and SIM-TST-002 updated.

### Verification

Documentation only. All requirement IDs referenced in `docs/specs/` are defined (checked
with a grep script).

## 2026-10-04: Simplify the Claude Code structure

### Summary

- `CLAUDE.md`: the architecture section points to `layering.md` instead of repeating the
  rules; the workflow section points to the `module-task` skill instead of repeating its
  steps.
- Task status is held only in `.claude/tasks/backlog.md`: the `Status` row was removed
  from `TASK_TEMPLATE.md`, `TASK-001.md` and `TASK-003.md`.
- `TASK_TEMPLATE.md` and `TASK-003.md`: section 0 lists only the task-specific plan
  contents; the generic planning gate lives in `module-task`.
- `backlog.md`: TASK-002 row removed (no task file); TASK-003 now depends on TASK-001.
- `coding_rules.md`: `paths:` frontmatter added, so it loads only for C files in `src/`
  and `tests/`.
- One home per trace link: SWS -> code -> test only in `docs/autosar_compliance.md`;
  SG -> SYS -> SWR / module -> test only in `docs/specs/traceability.md`. The module doc
  template loses its "Test mapping" section and the "Parent" column of "Derived
  requirements". `docs/specs/README.md` and the `module-task` log step updated.
- `CLAUDE.md` brought in line with the 2026-10-04 spec decisions: TCU I/O over CAN with a
  plant model, Pi5 host, FRDM-MCXN236 gateway, PBL / SBL reprogramming, spec-driven
  development, DC-01 (RTE maps ports to `Com`), MCAL list without Adc / Pwm / Icu, `Can`
  and the com stack added.
- Repository locations decided (user): `src/boot/{pbl,sbl}`, `tools/sim/`,
  `tools/gateway/`; `sim_requirements.md` open question 1 answered. `CLAUDE.md` has a
  Language section (English artefacts, Vietnamese replies).
- `layering.md`: the RTE maps ports to `Com` (today) or `IoHwAb` (hardware variant).
- `docs/architecture/overview.md`: layer diagram and module list without Adc / Pwm / Icu /
  IoHwAb, with Can, CanIf, PduR, Com, CanTp, Dcm, PBL / SBL; status column removed (the
  backlog holds status).
- `backlog.md` rewritten from TASK-011: gateway + second board, Can stack, sim tools, Rte,
  SWCs, Wdg, CanTp, Dcm, UDS tester, PBL, SBL, Dem (later).
- Removed the empty skeleton directories `src/mcal/{Adc,Pwm,Icu}` and
  `src/bsw/ecual/IoHwAb`.
- `TASK-001.md`: fixed the path `claude/rules` -> `.claude/rules`.
- `module-task` step 9: a decision still in force is also written to the file that owns
  it (`CLAUDE.md`, rules, specs), because the development log is not loaded automatically.

### Verification

Documentation and Claude configuration only; no build or test needed.

## 2026-10-17: Spec findings before the system design (spec update)

### Summary

Findings from preparing TASK-026 (system design), resolved with the user:

- `system_requirements.md` (v0.2):
  - SYS-FLT-007: one DTC per fault bit, status bits testFailed / confirmedDTC, `19 01` /
    `19 02` / `14 FF FF FF`, volatile DTC memory (the application cannot write flash,
    DC-04), `14` does not leave limp-home; priority `Could` -> `Should`.
  - SYS-DIA-002: `14` and `19` (01, 02) added, default and extended session, no security.
  - SYS-BOOT-009: the bootloader always resets on S3 timeout in the programming session;
    the PBL start-up decision then applies.
  - Open questions 6 .. 8 added and closed.
- `can_matrix.md` (v1.1): DTC column in the `FaultBits` table (section 3.6).
- `sim_requirements.md` (v0.2): SIM-TST-003 reads and clears DTCs.
- `traceability.md` (v0.2): section 3 no longer cites the deleted SYS-BOOT-003 / -012;
  SIM-TST-003 supports SYS-FLT-007.
- `backlog.md`: new TASK-026 (system design, PlantUML); TASK-018 depends on it; TASK-025
  also covers the `Dcm` services `14` / `19` and depends on TASK-021.

### Verification

Documentation only. All requirement IDs referenced in `docs/specs/` are defined (checked
with a grep script).

## 2026-10-17: Spec findings from the system design (F-01 .. F-18, spec update)

### Summary

Findings from planning TASK-026, resolved with the user before any diagram is written
(decision: fix the specs first). Proposals of the approved TASK-026 plan applied:

- `system_requirements.md` (v0.3):
  - SYS-COM-005 (F-01): stop with `28 01 01`, restart with `28 00 01`; reception continues.
  - SYS-SEL-005, SYS-SFT-007, SYS-SEN-005, term "shift in progress" (F-02, F-03): shifts and
    shift / ratio monitoring only in D; in P / R / N the prepared pattern follows directly
    (only `SYS-SFT-005` applies); a shift is complete at OutputShaftSpeed <= 200 rpm.
  - SYS-BOOT-001 (F-04): the start check also covers `hw_id` and `min_pbl_version`; new
    SYS-BOOT-015: the SBL erases the application header sector after a failed `0202` /
    `FF01`.
  - SYS-BOOT-006 (F-05): eraseMemory named. SYS-BOOT-014 (F-08): the application is silent
    between its `50 02` and the reset.
  - SYS-SEL-001, SYS-COM-002, SYS-STA-001 (F-09): selector N until the first accepted
    position; timeouts monitored from the application start with substitute values until the
    first reception; control active within 500 ms also without plant messages.
  - SYS-FLT-003, SYS-COM-006 (F-10): bus-off: red LED ON within 100 ms, limp-home outputs when
    the bus is back, recovery continues.
  - New SYS-FLT-008 (F-11): limp-home faults stay set until the power cycle (selector fault:
    own rule); oil temperature sensor fault and overtemperature recover. SYS-SEN-002: set on
    the first invalid value.
  - SYS-SEL-004 (F-13): RLS stays ON only while R remains selected and the speed is valid.
  - SYS-FLT-002 (F-14): communication fault of `SIM_Analog` enters limp-home.
  - SYS-STA-002 note (F-15): no watchdog in the PBL / SBL in v1. SYS-COM-004 note (F-18): no
    `TCU_Status` before control active. Open question 9 added and closed.
- `can_matrix.md` (v1.2): section 5 functional addressing only for `3E 80` (F-16); section 6
  fault bits per timeout, `SIM_Analog` timeout -> limp-home, substitute values until the first
  reception (F-12, F-14).
- `reprogramming.md` (v0.4): step 4 `28 01 01` / `68 01`; entry with an invalid application
  (F-07); section 3 services per part and session, `10 01` in a bootloader programming
  session resets, lockout rule for level 0x01 (F-06, F-17); section 9 entry note; section 11
  version comparison and header invalidation, new 11.1 request sequence and address rules
  (F-05). Open question 5 added and closed.
- `system_context.md` (v0.2): mode "Programming" renamed "Bootloader" (PBL start-up, default
  and programming sessions, SBL); Init sends no `TCU_Status` (F-18).
- `traceability.md` (v0.3): SYS-FLT-008 under SG-04, SYS-BOOT-015 under SG-06 and SIM-TST-002.
- `TASK-026.md` section 2: pointer to this update.

### Decisions

- F-14: the stricter option was taken (a `SIM_Analog` timeout enters limp-home like a line
  pressure sensor fault); consequently the `SIM_Analog` communication fault is latched by
  SYS-FLT-008 and is not in its list of recovering faults.
- F-06: `10 01` in a PBL / SBL programming session answers `50 01` and resets (the plan named
  `10 01` as supported without stating its effect).

### Verification

Documentation only. All requirement IDs referenced in `docs/specs/` are defined (checked
with a grep script).
