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

## 2026-10-17: TASK-026 system design (PlantUML)

### Summary

Phase B of TASK-026. The plan was approved by the user before any file was written (AC0). It
runs on the spec baseline of commit `79929ee` (F-01 .. F-18 resolved in Phase A). Documentation
only: no code, build or unit test.

- `docs/architecture/system/system_design.md` (new). Sections:
  - purpose and inputs with versions, design rules ("system level only", diagram conventions);
  - system elements E-01 .. E-15 and interfaces I-01 .. I-17c;
  - operating modes and the running software part over time;
  - allocation of all 61 `Approved` SYS and 18 SIM requirements;
  - scenario catalogue;
  - coverage: SYS, SG safe states and SIM -> diagrams;
  - open questions: F-01 .. F-18 resolved, new findings N-01 .. N-11, assumption A-01.
- `docs/architecture/system/diagrams/`, 43 PlantUML files:
  - component diagrams `cmp_system_context`, `cmp_system_interfaces`, `cmp_tcu_software_parts`;
  - the state diagram `stm_tcu_modes` (index of the scenarios);
  - 39 sequence diagrams `seq_*`: start-up 4, selector 5, shifting 7, faults 8, CAN 4,
    diagnostics 4, reprogramming 7.
- The catalogue and coverage tables of `system_design.md` are generated from the `Covers:` header
  notes of the diagrams, so the two cannot diverge.
- `docs/architecture/overview.md`: one link to the system design.
- `backlog.md`: TASK-026 -> DONE.

### Decisions

- SC-BOOT-01 (19 steps) is one file on three pages (`newpage`): application and hand-over, PBL,
  SBL and restart. The plan's fallback split into two files was not needed, and the catalogue
  keeps one file per scenario.
- The TCU is split into its software parts in the boot scenarios and also in SC-STA-01 .. SC-STA-03
  and SC-DIA-02, because there the running part is the subject.
- The diagrams draw the updated spec text of Phase A (e.g. `28 01 01` / `68 01`, `SIM_Analog`
  timeout -> limp-home, mode "Bootloader", entry with an invalid application). No
  `ASSUMPTION (F-nn)` marker is used.

### Deviations and findings

- SC-COM-04 shows more than the plan's two branches. During a bus-off the 30 ms reception timeouts
  set the communication faults first, which leads to finding N-06.
- SC-BOOT-05 separates three interruption windows: before the erase (previous application
  starts), during erase / download (PBL default session), and after a complete download (the PBL
  check decides). This leads to finding N-10.
- New spec findings N-01 .. N-11 are recorded in `system_design.md` section 10.2, each with a
  proposal. The specs are not changed (AC5); a spec update needs a separate user decision.
  The two findings of medium severity:
  - N-06: the SYS-FLT-003 reaction cannot be met for faults set during a bus-off.
  - N-10: the wording of SYS-BOOT-007; an image after a complete download starts without `0202` /
    `FF01`.
- One `ASSUMPTION:` marker is used (A-01 / N-07, SC-SFT-07): the cold-oil start-up check uses
  the first received oil temperature, not the substitute 80 degC.

### Verification

| AC | Command / check | Result |
|---|---|---|
| AC0 | plan approval | approved by the user before any file was written |
| AC1 | `plantuml -checkonly docs/architecture/system/diagrams/*.puml` | rc 0 |
| AC1 | `plantuml -tpng -o <scratchpad> docs/architecture/system/diagrams/*.puml`; every PNG opened and reviewed | rc 0; 45 PNG (SC-BOOT-01 = 3 pages), no error image, largest 2919 x 1784 px (below the 4096 px limit); layout fixes applied to `cmp_system_interfaces`, `cmp_tcu_software_parts`, `stm_tcu_modes` |
| AC2 | coverage script (scratchpad, not committed) | `AC2 PASSED`, output below |
| AC3 | `grep -nwE 'CanIf\|PduR\|CanTp\|Dcm\|Dem\|Rte\|SchM\|EcuM\|Os\|Swc[A-Za-z]*\|runnable\|Rte_[A-Za-z_]*' docs/architecture/system/diagrams/*.puml` | no output (rc 1). A wider case-insensitive sweep (task, API, SWC, port, function, ...) with requirement IDs masked was also clean. |
| AC4 | spot check: every distinct UDS byte string, NRC and value (ms, rpm, bar, degC, km/h, %, KiB, mV) listed from the diagrams and compared with can_matrix, reprogramming, system_requirements and sim_requirements | all values from the specs; NRCs not named by a spec (`31` for an unknown DID, `35`) labelled `(ISO 14229-1)` |
| AC5 | `git diff --stat 79929ee -- docs/specs/` | empty |
| AC6 | `overview.md` link, this entry, backlog DONE | done |

AC2 script output:

```
specs: 61 Approved SYS, 2 Deleted SYS ['SYS-BOOT-003', 'SYS-BOOT-012'], 18 SIM, 6 SG
diagrams: 43 files, 39 seq_*
check 1: 61 coverage rows; Approved SYS without any diagram: ['SYS-TIM-002'] (reason given: True)
check 2: 84 distinct IDs cited in diagrams, invalid or Deleted: none
check 3: 43 catalogue rows (39 scenarios), all files exist and all files listed: True
check 4: SG-01 -> ['SC-FLT-02', 'SC-SEL-04']; diagrams naming SG-01: ['SC-FLT-02', 'SC-SEL-04']
check 4: SG-02 -> ['SC-SEL-05', 'SC-SFT-02', 'SC-SFT-05']; diagrams naming SG-02: ['SC-SEL-05', 'SC-SFT-02', 'SC-SFT-05']
check 4: SG-03 -> ['SC-SEL-02', 'SC-SEL-03', 'SC-SEL-05']; diagrams naming SG-03: ['SC-SEL-02', 'SC-SEL-03', 'SC-SEL-05']
check 4: SG-04 -> ['SC-COM-01', 'SC-COM-02', 'SC-COM-04', 'SC-FLT-01', 'SC-FLT-02', 'SC-FLT-03', 'SC-FLT-04', 'SC-FLT-05', 'SC-FLT-06', 'SC-SFT-06', 'SC-STA-03']; diagrams naming SG-04: ['SC-COM-01', 'SC-COM-02', 'SC-COM-04', 'SC-FLT-01', 'SC-FLT-02', 'SC-FLT-03', 'SC-FLT-04', 'SC-FLT-05', 'SC-FLT-06', 'SC-SFT-06', 'SC-STA-03']
check 4: SG-05 -> ['SC-DIA-04']; diagrams naming SG-05: ['SC-DIA-04']
check 4: SG-06 -> ['SC-BOOT-03', 'SC-BOOT-04', 'SC-BOOT-05', 'SC-STA-02']; diagrams naming SG-06: ['SC-BOOT-03', 'SC-BOOT-04', 'SC-BOOT-05', 'SC-STA-02']
check 5: SIM rows 18/18; open questions referenced in diagrams ['N-04', 'N-05', 'N-06', 'N-07', 'N-08', 'N-09', 'N-10', 'N-11']; ASSUMPTION in ['seq_sft_cold_oil.puml'], rows ['A-01']

AC2 PASSED
```

The checks of the script:

1. every `Approved` SYS has a coverage row, and a diagram or a reason;
2. every SYS / SIM / SG ID in the diagrams exists and is not `Deleted`;
3. catalogue rows and files match one to one, titles carry the scenario ID, `@startuml <file name>`,
   and the `Covers:` header equals the catalogue *Covers* column;
4. every SG has a safe-state row whose scenarios name the SG;
5. SIM coverage is complete, every open question referenced in a diagram is defined, and every
   `ASSUMPTION` is listed.

## 2026-10-17: Spec findings N-01 .. N-11 from the system design (spec update)

### Summary

Findings recorded in `docs/architecture/system/system_design.md` section 10.2 while drawing
the TASK-026 diagrams, resolved with the user:

- `system_requirements.md` (v0.4):
  - SYS-HMI-001 (N-01): FaultLamp / red LED ON while the selector fault is set.
  - SYS-SEN-006 (N-04): pressure plausibility suspended while the line pressure sensor fault
    or the `SIM_Analog` communication fault is set.
  - SYS-COM-003 (N-05): the alive counter is compared with the last received message of the
    same ID, ignored or not.
  - SYS-FLT-003 (N-06): the bus-off exception covers every fault set while in bus-off,
    including the communication faults; an outage > 30 ms enters limp-home through them.
  - SYS-SFT-008 (N-07): "at startup" = first oil temperature received after reset.
  - SYS-DIA-003 (N-08): every return to the default session (`10 01` or S3) undoes `28` / `85`.
  - SYS-BOOT-007 (N-10): the next reset starts the unchanged previous application or ends in
    the PBL default session, or starts the new application once it has been accepted. SYS-BOOT-015 restated: the SBL programs the application header
    sector only after `0202` and `FF01` passed (header written last) instead of erasing it
    after a failed check.
  - SYS-SFT-006, SYS-SEL-005 (N-11): over the 6000 rpm limit, the lowest higher gear within the
    limit.
  - Open question 10 added and closed.
- `can_matrix.md` (v1.3): `SelectorState` = 7 while the selector fault is set (N-02);
  `OilTemp` = 0xFF while the substitute is used (N-03).
- `reprogramming.md` (v0.5): step 18 programs the header sector before the positive response;
  section 11 header-last rule (N-10); section 3 security lockout not kept across a reset,
  accepted for v1 (N-09). Open question 6 added and closed.

### Decisions (user)

- N-01 lamp ON; N-06 extend the exception; N-09 accept the reset behaviour; N-10 header
  written last (detailed design in TASK-024). The other findings as proposed.

### Verification

Documentation only. All requirement IDs referenced in `docs/specs/` are defined (checked
with a grep script).

## 2026-10-17: TASK-026 system design updated for N-01 .. N-11

### Summary

The system design follows the spec update of N-01 .. N-11 (commit `0fcaba6`):

- `system_design.md` 0.2: input versions (system_requirements 0.4, can_matrix 1.3,
  reprogramming 0.5); section 10.2 lists N-01 .. N-11 as resolved with the decision and the
  spec location; assumption A-01 removed (section 10.3: none).
- Diagrams updated (15):
  - SC-SEL-02: lamp / red LED ON while the selector fault is set, `SelectorState` = 7.
  - SC-FLT-03, SC-COM-01: `OilTemp` = 0xFF while the substitute is used.
  - SC-FLT-04: pressure plausibility suspended with the line pressure sensor fault.
  - SC-COM-02: counter reference = last received message.
  - SC-COM-04: SYS-FLT-003 exception for faults set in bus-off.
  - SC-SFT-07: start-up instant from SYS-SFT-008 (no assumption left).
  - SC-DIA-01: `10 01` undoes `28` / `85`.
  - SC-BOOT-02: lockout not kept across a reset.
  - SC-BOOT-01, SC-BOOT-03, SC-BOOT-05, `cmp_tcu_software_parts`: application header sector
    programmed last (SYS-BOOT-015); SC-BOOT-05 now has the branches before the erase,
    steps 13 .. 18 and step 19.
  - SC-SFT-02, SC-SEL-05: lowest higher gear within the 6000 rpm limit.

### Deviations

- While updating SC-BOOT-05, the new SYS-BOOT-007 wording was found to miss the interruption in
  step 19 (application already accepted). Fixed in the spec commit before it was used
  (amended into `0fcaba6`; development log entry of the N-01 .. N-11 spec update adjusted).

### Verification

| Check | Result |
|---|---|
| `plantuml -checkonly docs/architecture/system/diagrams/*.puml` | rc = 0 |
| Render of the 15 changed diagrams to the scratchpad | rc = 0; SC-BOOT-05, SC-SEL-02 and SC-COM-02 (largest changes) viewed, layout correct |
| AC2 script (coverage, IDs, catalogue, SG, SIM) | `AC2 PASSED`; no open question or `ASSUMPTION` referenced in the diagrams |
| AC3 grep for software-design names | no match (rc = 1) |
| `git diff --stat 0fcaba6 -- docs/specs src` | empty |


## 2026-10-18: PBL -> SBL hand-over wording and `0301` response timing (spec update)

### Summary

- `reprogramming.md` (v0.6):
  - Section 8 reworded: no state is kept in RAM across a reset; the PBL -> SBL hand-over at
    `31 01 03 01` happens without a reset and is not affected. The old text ("no state is
    passed between the application, PBL and SBL in RAM") contradicted section 1.
  - Section 11: response to activateSecondaryBootloader. Refusals (`24` / `22`) come from the
    PBL within P2server; otherwise the PBL sends `7F 31 78`, waits until it is transmitted
    and starts the SBL, which sends `71 01 03 01 00` within P2*server (5000 ms). Step 11 of
    section 2 updated.
  - Open question 7 added and closed.
- Diagrams: `seq_boot_programming.puml` (SC-BOOT-01 step 11) and `stm_tcu_modes.puml`
  (transition PBL -> SBL programming session) show `7F 31 78` from the PBL.

### Decisions (user)

- Response pending from the PBL before the SBL starts, instead of an SBL response within
  P2server: the SBL start-up time is not known yet, and the tester already handles
  `7F 31 78` (step 13, RB-05).
- Not covered: an SBL that hangs at start-up stays until a power cycle (open question 7).

### Verification

| Check | Result |
|---|---|
| `plantuml -checkonly docs/architecture/system/diagrams/*.puml` | rc = 0 |
| Render of `stm_tcu_modes` and `seq_boot_programming` to the scratchpad | rc = 0; transition label and step 11 viewed, layout correct |
| Requirement IDs referenced in `docs/specs/` defined (grep script) | no undefined ID |

## 2026-10-19: System design approved

### Summary

- `docs/architecture/system/system_design.md` (v0.2): status `Draft (for review)` -> `Approved`
  after the user review. It is now the approved input for the SWC composition (TASK-027).

## 2026-10-21: TASK-027 SWC composition

### Summary

- New `docs/architecture/swc_composition.md` (v0.1, design draft) with these sections:
  - five SWCs and the boundary between fault detection and fault decision;
  - allocation of all 61 `Approved` SYS requirements: 33 application, 28 not application;
  - port table: 28 ports, 48 data elements / operations;
  - RTE mapping to `Com`, `IoHwAb` and BSW services, with the target in the hardware variant
    (DC-01);
  - `IoHwAb` signal list for TASK-028: `FaultLampLed`;
  - task schedule, end-to-end latency, initialisation, data consistency, execution-time
    budget;
  - operating modes;
  - decisions, findings, assumptions, design inputs to the BSW, RTE and OS tasks.
- New diagrams:
  - `docs/architecture/swc_composition.puml`, two pages: data flow between the SWCs, and the
    ports mapped by the RTE to `Com`, `IoHwAb` and the BSW services;
  - `docs/architecture/swc_task_cycle.puml`: one `OsTask_10ms` activation in order, the
    100 ms activation, the 1 ms preemption.
- `docs/architecture/overview.md`:
  - layer picture with the five SWCs, `BswM`, `Dem`, `CanSM`, `IoHwAb` and the debug UART
    CDD;
  - new module rows;
  - link from the SWCs row to the composition;
  - *Signal flow* table (every RTE port) and *Scheduling* table filled;
  - new `IoHwAb` statement.
- `CLAUDE.md`, `.claude/rules/layering.md`: `IoHwAb` serves the board I/O now; the hardware
  variant adds the sensor and actuator signals.
- `.claude/rules/layering.md` (G-06): a BSW service module with AUTOSAR service ports
  (`BswM`, `Dcm`, `Dem`) may include its own `Rte_<Ma>.h`.
- Backlog:
  - TASK-018 renamed to "sensor input"; TASK-017 title names the mapping targets;
  - TASK-021 depends on TASK-030;
  - new tasks: TASK-029 `CanSM`, TASK-030 `BswM`, TASK-031 debug UART CDD `DbgOut`.
- TASK-027.md: count of `Approved` SYS corrected from 62 to 61 (G-01).

### Decisions

User decisions (2026-10-21):

- `SwcVehicleSpeed` takes `SIM_Speed` and `SIM_Analog` and is renamed `SwcSensorInput`.
- Minimal `CanSM` and `BswM` are new tasks:
  - `CanSM` does the bus-off recovery;
  - `BswM` starts the TCU_Tx I-PDU group only when control is active AND `28` does not
    disable Tx.
- `SYS-HMI-002` gets its port and runnable now, plus a debug UART CDD task.

Decisions from the approved plan (composition section 6.1):

- **D4.** The detecting SWC reports the fault condition; `SwcFaultMgr` only latches,
  aggregates and decides.
- **D5.** One SWC per received message evaluates the `Rte_Read` status.
- **D6.** Every `TCU_*` signal is written directly by its producer.
- **D7.** All S/R access is explicit.
- **D8.** Mode switch ports, with a simplification for the handwritten RTE.
- **D9.** Init ends on a limp-home fault, or when the selector is known and the vehicle
  speed is valid.
- **D10.** No watchdog port; the watchdog is served last in `OsTask_10ms`.
- **D11.** `SwcFaultMgr` reports the Dem events.

The order of the main functions and runnables in `OsTask_10ms` is a design input. Each BSW
module design confirms it or reports a conflict.

### Deviations and findings

- Deviation: `swc_composition.puml` page 1 draws the data flow between the SWCs (component to
  component, with the ports on the arrows), not lollipop interfaces. The interface layout was
  3400 px wide and could not be read. Page 2 keeps the interfaces.
  `portin` / `portout` are not supported by PlantUML 1.2020.02.
- Findings G-02 .. G-05, G-07 and G-08 wait for the user. The specs and the system design are
  unchanged.
  - G-02: invalid throttle.
  - G-03: GearActual / GearTarget semantics.
  - G-04: no `SYS-FLT-003` exception for `28 01 01`.
  - G-05: vehicle speed above 250 km/h.
  - G-07: TASK reference in `system_design.md` section 1.
  - G-08: PCS characteristic for `SYS-SEN-006`.

### Verification

Documentation only (no build, no unit tests, task section 0).

| Check | Result |
|---|---|
| AC1 `plantuml -checkonly docs/architecture/*.puml` | rc = 0 |
| AC1 render to the scratchpad | rc = 0; both pages of `swc_composition` and `swc_task_cycle` viewed, layout readable |
| AC2 script: Approved SYS vs allocation tables | 61 Approved, 61 rows (33 + 28), no duplicate, missing or extra ID |
| AC3 script: port table cells, BSW-mapped ports in the mapping table, runnables in tasks | 48 rows complete, every BSW / Com peer mapped, 11 runnables each in exactly one task |
| AC4 script: Com-mapped elements vs can_matrix | 21 elements: signal name in its message, ID, matrix scaling present |
| AC6 script: overview ports vs composition | same 28 ports; every periodic runnable in the overview schedule |
| AC7 `git diff --stat ad00113 -- docs/specs/ docs/architecture/system/` | empty |

## 2026-10-22: TASK-003 platform types, firmware entry, CMake helpers, Unity + CMock

### Summary

- Platform headers in `src/platform/include/` (header-only, only `<stdint.h>` included):
  - `Platform_Types.h`: CPU symbols (`CPU_TYPE_32`, `LSB_FIRST`, `LOW_BYTE_FIRST`),
    `boolean`, `TRUE` / `FALSE`, `uint8..uint64`, `sint8..sint64`, `*_least`, `float32`,
    `float64`, `VoidPtr`, `ConstVoidPtr`;
  - `Std_Types.h`: `Std_ReturnType`, `E_OK` / `E_NOT_OK` (`STATUSTYPEDEFINED`), `STD_*`,
    `Std_VersionInfoType`, transformer error and message types,
    `Std_ExtractProtocolHeaderFieldsType`;
  - `Compiler.h`: compiler symbols `_GCC_C_CORTEXM33_` (target) / `_GCC_C_AARCH64_` (host),
    `AUTOMATIC`, `TYPEDEF`, `NULL_PTR`, `INLINE`, `LOCAL_INLINE`, GCC >= 13 check;
  - `Compiler_Cfg.h`: empty (DEV-01);
  - published information in the three main headers: vendor `0xFFFF`, module IDs
    Platform 199 / Std 197 / Compiler 198, AR 4.7.0, SW 1.0.0.
- Firmware entry `src/ecu/` (`CMakeLists.txt`, `prj.conf`, `main.c`): SDK hello_world board
  port, banner `xAT ecu: boot OK`, idle loop, place of `EcuM_Init()` marked for TASK-010,
  `_Static_assert` block that checks the type widths, `boolean`, `CPU_TYPE` and the byte
  order for the target compiler at every build.
- `cmake/xat_module.cmake`: `xat_add_module(NAME DIR)` (header-only modules supported),
  `xat_add_project_sources(SOURCES)`, `XAT_PROJECT_C_FLAGS` (shared with the host tests).
- `cmake/xat_unit_test.cmake`: `xat_add_unit_test(NAME SOURCES MOCKS INCLUDES DEFINES
  [PASS_REGEX])` (CMock mocks and Unity runner generated per test) and
  `xat_add_compile_check(NAME HEADERS SOURCES INCLUDES)` (each header alone, include guard).
- `third_party/unity` (Unity v2.7.0) and `third_party/cmock` (CMock v2.7.0): complete tag
  archives, SHA-256 in `third_party/README.md`. The three Makefiles that Unity's own
  `.gitignore` hides are added with `git add -f`.
- Tests in `tests/unit/`: `platform/` (`test_Platform_Types.c` 20 tests, `test_Std_Types.c`
  10, `test_Compiler.c` 6, compile checks for the four headers and two predefined-symbol
  cases), `framework/` (`test_CMock_Selftest.c` 2 tests, guard `test_CMock_Guard.c` with
  two tests that must fail).
- Every definition and test carries its requirement tags (`[SWS_...]`, `[REQ-003-xx]`), as
  the user asked; `docs/unit_testing.md` gives the grep command that lists them.
- Docs: `docs/modules/Platform/design.md` + `component.puml`, `docs/unit_testing.md` (new);
  `docs/setup_host.md` (section 5 Ruby), `docs/operations.md` (firmware build notes),
  `docs/autosar_compliance.md` (sections Platform, Std, Compiler: 97 rows),
  `docs/architecture/overview.md` (design link), `third_party/README.md`.
- Rules and skills: `coding_rules.md` 2.1 (SWS spelling also wins over 1.3), 3.1
  (`Platform_Types.h` may use `<stdint.h>`), DEV-01 text completed; `verify` skill step 2
  final; `new-module` skill creates `tests/unit/<layer>/<Ma>/CMakeLists.txt` from the new
  template `CMakeLists_test.txt` and registers the module in both builds.
- Host: Ruby 3.2.3 installed (`sudo apt-get install -y ruby`).

### Decisions

From the approved plan (2026-10-22):

- **D1.** The AUTOSAR integer types map onto the `<stdint.h>` exact-width types. Reason:
  newlib defines `uint32_t` as `unsigned long` on the target and glibc as `unsigned int` on
  the host. A hand-written `unsigned int` would have the right width but give
  incompatible-pointer errors (GCC 15) whenever MCAL passes a `uint32 *` to the SDK. This
  replaces the "must not use `long`" note of the task context; the widths are checked on
  both compilers. The `*_least` types are fixed to 32 bit on both.
- **D2.** The non-draft transformer and message types of `Std_Types.h` are implemented;
  the draft ones (SWS_Std_00027..00029) are not.
- **D3.** The compiler symbol keeps the SWS spelling with the leading underscore (MISRA
  C:2012 Rule 21.1 deviation comment). One `Compiler.h` serves the two compiler / platform
  pairs and stops with `#error` on any other.
- **D4.** Task open questions: Unity / CMock vendored as release snapshots (a); Ruby
  installed; SDK hello_world board port reused until `Mcu` / `Port`; `PRINTF` for the
  banner; framework self-test kept permanently, plus the automatic guard test.
- **D6.** `new-module` template for the test `CMakeLists.txt`.
- **D5.** Requirement tags are a permanent rule (user, 2026-10-22): `coding_rules.md` 8.6
  (format, ID kinds incl. task `REQ-<task>-nn`, placement on the smallest unit, also in
  CMake / config files, grep command), `testing_rules.md` (every test tagged), `verify`
  skill step 6 (tag check), `new-module` templates (`Module.c`, `test_Module.c`);
  `docs/specs/README.md` workflow step 3 refers to 8.6 (wording only, no requirement
  changed).

### Deviations and findings

- The map file is `output.map` (SDK default name), not `ecu_cm33_core0.map` as AC1 says.
- Beyond the task file: three test files instead of one (one per SWS), compile checks,
  the guard test, the `new-module` template and the `coding_rules.md` changes (all in the
  approved plan).
- The CMock-generated mocks and the Unity runners compile without any warning under the
  strict flags; no `-Wno-*` exception was needed (plan risk did not occur).
- The first `ops run` after connecting the board failed the pyOCD read-back verify at one
  byte (`0x470`: memory `0x84`, file `0x80`). The next `ops flash --flasher pyocd` and two
  `ops run` passed verify; not reproduced. Watch for it in later flash runs.
- Module IDs and abbreviations checked against `AUTOSAR_TR_BSWModuleList` R21-11 (document
  150, added to `docs/references/autosar/` by the user): `Std` 197, `Compiler` 198,
  `Platform` 199.
- TASK-009 (OS) must define `StatusType` and `E_OK` inside `#ifndef STATUSTYPEDEFINED`
  (`docs/modules/Platform/design.md` decision 4).

### Verification (2026-10-22)

`ops` = `python3 scripts/operations.py`.

| AC | Command | Result |
|---|---|---|
| AC0 | — | plan approved before any change (2026-10-22) |
| AC1 | `ops -v build --clean` | exit 0, 0 warnings in the whole build log; `ecu_cm33_core0.elf` / `.bin` and `output.map`; text 16592, data 20, bss 3444 |
| AC2 | `ops --config release build --clean` | exit 0; text 9564, data 20, bss 3444 |
| AC3 | `ops run --expect "xAT ecu: boot OK" --timeout 15`, then `ops monitor --expect ...` | exit 0 / exit 0, banner received (pyOCD); first attempt: verify mismatch, see findings |
| AC4 | `cmake -G Ninja -S tests/unit -B build/unit && cmake --build build/unit && ctest --test-dir build/unit --output-on-failure` | clean build: 0 warnings; 5/5 CTest tests passed (38 test cases + guard) |
| AC5 | manual, then reverted (files compared with backups: identical) | broken assertion (`8U` -> `9U`): `test_Platform_Types` FAIL "Expected 9 Was 8", ctest exit 8; unmet expectation (extra `DummyDep_Write_Expect`): FAIL "Called fewer times than expected.", ctest exit 8; `#ifndef TRUE` removed: build fails "TRUE redefined"; `STATUSTYPEDEFINED` guard removed: build fails "E_OK redefined". Automatic: `test_CMock_Guard` passes only on both failures + summary |
| AC6 | `verify` step 3 greps; `grep '#include' src/platform` | both greps empty; platform headers include only `<stdint.h>` and each other |
| AC7 | SHA-256 of all SDK files before (= TASK-001 baseline) and after all builds | 123,843 files, identical |
| AC8 | `diff -r` of `third_party/unity`, `third_party/cmock` against the extracted tag archives (after the builds) | identical; only the two empty CMock `vendor/` directories are missing |
| AC9 | review | docs listed in the summary updated |
| — | script: compliance matrix vs tags in `src/` and `tests/` | 65 Implemented / Partial IDs, all tagged in code; tests or compile checks cover all but SWS_Platform_00002 and SWS_Std_00004 (verified by review) |
| — | `plantuml -checkonly docs/modules/Platform/component.puml`, render | rc = 0, diagram viewed |
