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
(section "Requirements", column "Parent") and in `docs/modules/<Ma>/design.md` ("Derived
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

## Glossary

Terms used in the requirements that need a precise meaning.

### RLS (reverse lockout solenoid)

An actuator commanded by the TCU in `TCU_Actuators` that decides whether reverse can
engage. With RLS = OFF, selecting R does not engage reverse (the vehicle behaves as in N);
with RLS = ON, reverse may engage. It prevents reverse from engaging while the vehicle is
moving forward (`SG-01`).

| RLS | Meaning |
|---|---|
| OFF (0) | locked: R does not engage reverse |
| ON (1) | reverse allowed |

- The TCU commands RLS = ON only while the selector position is R and the vehicle speed
  is valid and below 2 km/h; otherwise OFF (`SYS-SEL-004`).
- Once ON, RLS stays ON while R remains selected, so reverse is not released when the
  speed rises slightly above the threshold during reversing.
- The plant model engages reverse only while R is selected and RLS = ON. If
  `TCU_Actuators` times out, the plant model uses RLS = OFF (fail-safe).

### Latched and not latched

These terms describe what happens to a state (a fault or a mode) after the condition that
caused it has disappeared.

| Term | Behaviour after the condition is gone | Cleared by |
|---|---|---|
| Latched | the state is kept | an explicit reset only (power cycle, ECU reset, ...) |
| Not latched | the state is cleared automatically, usually after a recovery (debounce) time | the disappearance of the condition |

Examples in this project:

- **Not latched:** the selector fault (`SYS-SEL-002`) is set after 100 ms of invalid
  selector input and clears after 500 ms of valid input. A short glitch in P, R or N
  therefore does not lock the vehicle until the next power cycle.
- **Latched:** limp-home (`SYS-FLT-004`) stays active until the next power cycle even if
  the fault disappears. This avoids toggling between Normal and limp-home on an
  intermittent fault.
- **Both together:** a selector fault while D is selected clears by itself, but the
  limp-home it caused stays latched.

Rule of thumb: latch a fault whose reaction is safety-relevant or whose cause may be
intermittent; do not latch a fault whose safe reaction is already in place and whose
recovery carries no risk. In an SWC design, the column "Latch / recovery" of the fault
table states which case applies and what clears it.

## Workflow

1. Behaviour changes start here: edit the spec, get it approved, then change code.
2. A task file lists the requirement IDs it implements (section "Requirements", column
   "Parent").
3. Code and tests reference requirement IDs in comments, e.g. `/* [SYS-SFT-003] */`,
   the same way SWS IDs are referenced (format and placement: `.claude/rules/coding_rules.md`
   8.6).
4. The `verify` step checks that the diff matches the approved requirements.
