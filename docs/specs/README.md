# xAT specifications

This folder holds the **specifications** of xAT: what the system must do and why. Design
notes (how a module does it) live in `docs/modules/`, and the AUTOSAR SWS requirements
that a module implements are traced in `docs/autosar_compliance.md`.

xAT follows **spec-driven development**: behaviour is specified and approved here first,
and code implements only `Approved` requirements.

## Layout

| Path | Content |
|---|---|
| `templates/requirements_template.md` | template for every requirements document |
| `system/system_context.md` | system boundary, nodes, signals, assumptions, design constraints |
| `system/safety_goals.md` | safety goals `SG-nn` (simplified, educational) |
| `system/system_requirements.md` | system requirements `SYS-<AREA>-nnn` |
| `system/can_matrix.md` | CAN bus, messages, signals, timeouts, diagnostic IDs |
| `system/reprogramming.md` | UDS reprogramming sequence, memory map, image header |
| `simulation/sim_requirements.md` | requirements for the plant model, gateway, console and tester `SIM-<AREA>-nnn` |
| `software/<SwcName>.md` | software requirements `SWR-<SWC>-nnn` per SWC (written with the SWC design) |
| `traceability.md` | SG -> SYS -> SWR / SIM -> module / test |

BSW and MCAL modules do **not** get a spec here: their requirements are the AUTOSAR SWS.
What the project needs from a module (subset, configuration) is written in its task file
(section "Requirements", column "Parent") and in `docs/modules/<Ma>.md` ("Derived
requirements"). Each fact has one home:

| Link | Kept only in |
|---|---|
| SWS requirement -> code -> test | `docs/autosar_compliance.md` |
| SG -> SYS -> SWR / module -> test | `traceability.md` |

## Requirement IDs

| Prefix | Level | Example |
|---|---|---|
| `SG-nn` | safety goal | `SG-01` |
| `SYS-<AREA>-nnn` | system (TCU as a black box) | `SYS-SFT-003` |
| `SWR-<SWC>-nnn` | software, one SWC | `SWR-SHIFTCTRL-010` |
| `SIM-<AREA>-nnn` | simulation environment (not part of the TCU) | `SIM-PLT-002` |

System areas:

| AREA | Topic |
|---|---|
| `SEL` | gear selector, shift lock, reverse lockout |
| `SFT` | gear selection and shifting |
| `ACT` | actuator commands (solenoids, pressure control) |
| `SEN` | sensor acquisition and plausibility |
| `FLT` | fault reactions, limp-home |
| `COM` | CAN communication |
| `TIM` | timing |
| `STA` | startup, watchdog |
| `HMI` | fault lamp, debug output |
| `DIA` | diagnostics (UDS in the application) |
| `BOOT` | bootloader and reprogramming |

Simulation areas: `PLT` (plant model), `GW` (gateway), `CON` (console), `TST` (tester).

Rules:

1. An ID is never reused. A removed requirement keeps its ID with status `Deleted`.
2. One requirement = one testable statement, written as **"The TCU shall ..."** (or
   "The plant model shall ...", ...). It states *what*, not *how*.
3. Numbers are explicit and have units. An unknown value is written
   `[NEEDS CLARIFICATION: <question>]`, never guessed silently. A proposed value that
   still has to be confirmed is written `[NC: <value>]`.
4. Every requirement names its parent (`SG`, `SYS`, or `-` for a top-level functional
   requirement) and its verification method.

## Status

`Draft` -> `Approved` -> `Implemented` -> `Verified`, or `Deleted`.

- `Draft`: written, not reviewed.
- `Approved`: reviewed by the user; code may implement it.
- `Implemented`: code exists and references the ID.
- `Verified`: the test(s) listed in `traceability.md` pass.

## Verification methods

| Code | Method |
|---|---|
| `UT` | host unit test (Unity + CMock) |
| `TT` | target test on the board, with the plant model / tester on the Pi5 |
| `RV` | review (code, configuration, document) |
| `AN` | analysis (timing, memory map, calculation) |

## Workflow

1. Behaviour changes start here: edit the spec, get it approved, then change code.
2. A task file lists the requirement IDs it implements (section "Requirements", column
   "Parent").
3. Code and tests reference requirement IDs in comments, e.g. `/* [SYS-SFT-003] */`,
   the same way SWS IDs are referenced.
4. The `verify` step checks that the diff matches the approved requirements.
