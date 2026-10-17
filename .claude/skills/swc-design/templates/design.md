# <SwcName> - design

| Field | Value |
|---|---|
| Layer | app |
| Software requirements | [`docs/specs/software/<SwcName>.md`](../../specs/software/<SwcName>.md) |
| Composition | [`docs/architecture/swc_composition.md`](../../architecture/swc_composition.md) |
| Status | Design (draft) |

## 1. Purpose

<!-- What this SWC does in the TCU and what it explicitly does not do. -->

## 2. Requirements

| SWR | Short title | Parent (SYS / SG) | Design element | Test |
|---|---|---|---|---|
| | | | | |

## 3. Ports

Diagram: [component.puml](component.puml)

| Port | Direction | Interface | Data element | Type | Unit / scaling | Peer | On invalid / timeout / before first value |
|---|---|---|---|---|---|---|---|
| | | | | | | | |

## 4. Runnables

Diagram: [runnables.puml](runnables.puml)

| Runnable | Trigger | Period | OS task | WCET (estimate) | Reads | Writes |
|---|---|---|---|---|---|---|
| `<SwcName>_Init` | init | - | | | | |
| | | | | | | |

### Inter-runnable variables and static data

| Name | Type | Written by | Read by | Purpose |
|---|---|---|---|---|
| | | | | |

## 5. Behaviour

### 5.1 State machine

Diagram: [state.puml](state.puml)

| State | Meaning | Entered by (event [guard]) | Left by | Outputs in this state |
|---|---|---|---|---|
| | | | | |

### 5.2 Algorithms

<!-- One subsection per runnable with control logic, with its activity diagram. -->

#### <Runnable>

Diagram: [activity_<runnable>.puml](activity_<runnable>.puml)

### 5.3 Scenarios

| Scenario | Diagram | Requirements |
|---|---|---|
| Initialisation | [seq_init.puml](seq_init.puml) | |
| | | |

## 6. Data and scaling

Fixed-point unless stated otherwise (`app_rules.md`). `float32` used: <no / yes - reason>.

| Variable | Type | LSB | Offset | Physical range | Raw range |
|---|---|---|---|---|---|
| | | | | | |

### Arithmetic and overflow

| Calculation | Widest intermediate value | Type used | Rounding |
|---|---|---|---|
| | | | |

## 7. Calibration

Values for `config/<SwcName>_Cfg.c`. *Shared* = the plant model keeps its own copy
(`system_context.md` §4.1).

| Name | Type | Physical value | Raw value | Source | Shared |
|---|---|---|---|---|---|
| | | | | | |

## 8. Fault handling

| Fault | Detection condition | Debounce | Reaction | Latch / recovery | Fault bit | Parent |
|---|---|---|---|---|---|---|
| | | | | | | |

## 9. Test design

Unit tests: Unity + CMock with `Rte_<SwcName>` mocked, in `tests/unit/app/<SwcName>/`.
Target tests: with the plant model and console on the Pi5.

| Test ID | SWR | Kind (UT / TT) | Type (nominal / boundary / invalid / fault / transition) | Input | Expected output |
|---|---|---|---|---|---|
| | | | | | |

## 10. Open questions and assumptions

-

SYS -> SWR -> test: see `docs/specs/traceability.md`.
