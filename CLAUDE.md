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

# Coding rules
Refer the coding rules written in the `claude/rules/coding_rules.md` file.
