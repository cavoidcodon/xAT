# Project overview

xAT is a simplified transmission control unit (TCU) for a 4-speed automatic transmission,
running on the NXP FRDM-MCXN947 and following the AUTOSAR Classic layered architecture.
Each AUTOSAR module is implemented from its SWS.

There is no real transmission. All sensor inputs and actuator outputs are exchanged over
CAN with a plant model on the host. The TCU is reprogrammed over CAN with UDS (primary
bootloader PBL in flash, secondary bootloader SBL downloaded to RAM).

The project is **spec-driven**: required behaviour is specified in `docs/specs/`
(`docs/specs/README.md`), and code implements only `Approved` requirements. For BSW and
MCAL modules, the requirements are the AUTOSAR SWS (`docs/references/autosar/`).

# Target and environment

- TCU: FRDM-MCXN947 (MCXN947, Cortex-M33 `cm33_core0`), single core
- Host: Raspberry Pi 5, Ubuntu aarch64. Builds, flashes and debugs the boards, and runs
  the plant model, console and UDS tester (Python, SocketCAN `can0`)
- CAN gateway: FRDM-MCXN236, transparent SLCAN <-> CAN, plain NXP SDK application (not AUTOSAR)
- CAN bus: 500 kbit/s, onboard TJA1057 transceivers
- Language: C11
- AUTOSAR release: Classic R21-11 (AR 4.7.0); configuration is handwritten C (no generator)
- SDK: MCUXpresso SDK 26.09.00 at `../mcuxsdk` (read-only)
- Unit tests: Unity + CMock, host build (`third_party/`)

# Architecture

Dependency direction: `app -> rte -> bsw -> mcal -> NXP SDK`. The full include rules are in
`.claude/rules/layering.md`. Do not modify vendor SDK files unless explicitly approved.

SWCs reach sensor and actuator values only through RTE ports. Today the RTE maps those
ports to `Com` signals; a later hardware variant maps them to `IoHwAb` without changing
the SWCs (design constraints DC-01..05 in `docs/specs/system/system_context.md`).

# Repository layout

```
src/platform/        Std_Types.h, Platform_Types.h, Compiler.h
src/mcal/<Ma>/       MCAL drivers (Mcu, Port, Dio, Gpt, Wdg, Can)
src/bsw/services/    Det, EcuM, SchM, Dcm, ...
src/bsw/ecual/       ECU abstraction (IoHwAb only for a later hardware I/O variant)
src/bsw/com/         communication stack (CanIf, PduR, Com, CanTp)
src/os/              scheduler / OS
src/rte/             handwritten RTE
src/app/<Swc>/       transmission control SWCs
src/boot/pbl/        primary bootloader (separate image, in flash)
src/boot/sbl/        secondary bootloader (separate image, downloaded to RAM; only image with flash erase / write)
src/ecu/             firmware entry: main.c, EcuM callouts, board port, top-level CMakeLists.txt
tests/unit/          host unit tests (Unity + CMock), mirrors src/
tests/target/        on-board functional tests
docs/specs/          system, safety and simulation requirements, traceability
docs/                architecture, module design notes, AUTOSAR compliance matrix, development log
tools/sim/           host test tools: plant model, console, UDS tester (Python, not AUTOSAR)
tools/gateway/       FRDM-MCXN236 SLCAN <-> CAN gateway firmware (NXP SDK, not AUTOSAR)
scripts/operations.py  build / flash / debug / monitor tool (docs/operations.md)
.claude/             rules, skills, tasks for Claude Code
```

The AUTOSAR coding rules apply to `src/` and `tests/`; `tools/` follows its own conventions.

Every module directory has `include/` (public API), `src/` (implementation) and
`config/` (handwritten configuration). New modules are created with the `new-module` skill.

# Language

Code, comments and documents are written in English. Reply to the user in Vietnamese.

# Workflow

Every task runs through the `module-task` skill: no file is changed before the user has
approved a written plan. Use the `verify` skill after implementation.
Tasks live in `.claude/tasks/`; `backlog.md` is the only place that holds task status.

# Coding rules

The rules are in `.claude/rules/` and load automatically for the paths they cover:
`coding_rules.md` (C files in `src/`, `tests/`), `layering.md`, `mcal_rules.md`, `app_rules.md`,
`testing_rules.md`.
