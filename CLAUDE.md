# Project overview
This repository implements a simplified automotive transsmission control system on the NXP FRDM-MCXN947 board.

This project follows the AUTOSAR Classic layered architecture, implement necessary AUTOSAR modules base on it's AUTOSAR specification and application
with AUTOSAR compliance.

# Target

- Board: FRDM-MCXN947
- MCU: MCXN947
- Language: C11
- Base platform: Ubuntu using SDK from NXP for FRDM-MCXN947
- Initial execution model: single core
- Configuration for AUTOSAR modules: handwritten C headers and source files
- AUTOSAR release: Classic R21-11 (AR 4.7.0)
- Unit test framework: Unity + CMock (host build, `third_party/`)
- SDK: MCUXpresso SDK 26.09.00 at `../mcuxsdk` (read-only)

# Architecture

Allowed dependency direction:

application -> rte -> bsw -> mcal -> NXP SDK

Rules:
- Application code must not include NXP SDK header.
- RTE must not contain transmission-control algorithm.
- MCAL wrapper must remain thin and deterministics.
- Configuration values belong in each module config/
- Do not modify vendor SDK files unless explicity approved.
- Public module APIs belong in include directories.
- Every module must have coresponding tests (functional test base on functionality).

# Repository layout

```
src/platform/        Std_Types.h, Platform_Types.h, Compiler.h
src/mcal/<Ma>/       MCAL drivers (Mcu, Port, Dio, Gpt, Adc, Pwm, Icu, Wdg)
src/bsw/services/    Det, EcuM, SchM, ...
src/bsw/ecual/       IoHwAb, ...
src/bsw/com/         communication stack (later)
src/os/              scheduler / OS
src/rte/             handwritten RTE
src/app/<Swc>/       transmission control SWCs
src/ecu/             firmware entry: main.c, EcuM callouts, board port, top-level CMakeLists.txt
tests/unit/          host unit tests (Unity + CMock), mirrors src/
tests/target/        on-board functional tests
docs/                architecture, per-module design notes, compliance matrix, development log
scripts/operations.py  build / flash / debug / monitor tool (docs/operations.md)
.claude/             rules, skills, agents, tasks for Claude Code
```

Every module directory has `include/` (public API), `src/` (implementation) and
`config/` (handwritten configuration). New modules are created with the `new-module` skill.

# Workflow

For every task:
1. Read the task file and relevant design documents.
2. Inspect existing code before proposing changes.
3. Produce a plan.
4. Do not modify files until the plan is approved.
5. Make only changes required by the current task.
6. Build after implementaion.
7. Run relevant tests.
8. Review git diff.
9. Update docs/development_log.md.
10. Report changed files, verification results, and remaining risks.

Use the `module-task` skill to run a task and the `verify` skill for steps 6-8.
Tasks live in `.claude/tasks/` (`backlog.md`, `TASK_TEMPLATE.md`).

# Coding rules
The coding rules are in `.claude/rules/coding_rules.md`. Layer-specific rules (`layering.md`,
`mcal_rules.md`, `app_rules.md`, `testing_rules.md`) are in the same directory and load
automatically for the paths they cover.
