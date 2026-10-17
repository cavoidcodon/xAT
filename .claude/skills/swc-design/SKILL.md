---
name: swc-design
description: Design the xAT application software components (SWCs) from the system requirements before any SWC code exists - first the SWC composition (split of responsibilities, allocation of SYS requirements, ports between SWCs and to the RTE, runnable-to-task mapping), then one SWC at a time (SWR software requirements, ports, runnables, state machines, algorithms, fixed-point data, calibration, fault handling, test design) with PlantUML component, runnable, state, activity and sequence diagrams. Use this whenever SWC work starts (e.g. "design the SWCs", "design SwcShiftCtrl", "write the SWR for the gear selector"), before the new-module scaffold of an SWC, and when the user asks for diagrams of an SWC that has no design yet. Args - "composition" or "<SwcName>", e.g. "SwcShiftCtrl".
---

# Design the application SWCs

SWCs hold all the transmission control logic, so their design is where the system
requirements (`SYS-*`) turn into software requirements (`SWR-*`) and into a reviewed picture
of the algorithm. The user reviews the design; nothing in `src/` or `tests/` is written by
this skill, and no requirement status is changed to `Approved` - approval is the user's.

Two modes, run in this order:

| Arg | Output | Run |
|---|---|---|
| `composition` | `docs/architecture/swc_composition.md` + `.puml`, port and scheduling tables in `docs/architecture/overview.md` | once, before the first SWC; again when the split or the ports change |
| `<SwcName>` | `docs/specs/software/<SwcName>.md`, `docs/modules/<SwcName>/` | once per SWC, after the composition is reviewed |

If `<SwcName>` is given and `docs/architecture/swc_composition.md` does not exist, stop and
suggest running `swc-design composition` first: the ports of one SWC depend on the others.

## 1. Gather the inputs (both modes)

- **Requirements.** `docs/specs/system/system_requirements.md` (the `SYS-*` to implement),
  `system/safety_goals.md`, `system/system_context.md` (operating modes, calibration
  parameters §4.1, design constraints DC-01..05), `system/can_matrix.md` (signals, scaling,
  timeouts - the single source per DC-03), `traceability.md`.
- **Architecture.** `docs/architecture/overview.md` (SWC list, OS tasks), the composition
  (in SWC mode), existing SWC designs `docs/modules/Swc*/design.md`, and the RTE design
  `docs/modules/Rte/design.md` if it exists.
- **Rules.** `.claude/rules/app_rules.md` (ports only via `Rte_<Swc>.h`, runnables without
  parameters, calibration in `config/` with units, fixed-point), `layering.md`,
  `coding_rules.md` §10.
- **Task.** `.claude/tasks/backlog.md` and the SWC task file (TASK-018) if it exists.

Note the status of every `SYS-*` you use. Designing from `Draft` requirements is allowed,
but list each one in the report: code may only implement `Approved` requirements.

## 2. Mode `composition`

1. **Responsibilities.** One paragraph per SWC (start from the four in `overview.md`:
   `SwcGearSelector`, `SwcVehicleSpeed`, `SwcShiftCtrl`, `SwcSolenoidCtrl`). Each SWC has one
   clear job. Propose a different split only with a reason, and put it under *Open questions*.
2. **Allocation.** Every functional `SYS-*` that the application implements goes to exactly
   one owning SWC (others may contribute; name them). Requirements met by BSW, boot or the
   plant (e.g. `SYS-COM-*`, `SYS-BOOT-*`, `SYS-DIA-*`) are listed as "not application" with
   the module that owns them. No `SYS-*` may be left unallocated.
3. **Ports.** Every data flow between SWCs and between an SWC and the RTE/`Com`:
   port name, interface type (sender/receiver, client/server, mode), data element, type,
   unit and scaling, range, init value, producer, consumer(s), period. Signals from the CAN
   matrix keep the matrix name and scaling; the SWC that converts them (DC-02) is named.
4. **Scheduling.** Each runnable -> OS task (1 / 10 / 100 ms, SYS-TIM-001) and the order of
   runnables inside a task, chosen so data flows producer -> consumer in the same cycle
   where the requirement needs it. State the worst-case end-to-end latency for the fault
   reaction (`SYS-FLT-003`) and shift paths.
5. **Operating modes.** How Init / Normal / Limp-home (`system_context.md` §5) are known by
   each SWC (mode port, status port) and who decides the mode.

Write `docs/architecture/swc_composition.md` from `templates/composition.md` and
`docs/architecture/swc_composition.puml` (component diagram: the SWCs with their ports, the
RTE, and `Com` behind it). Then fill the *RTE port* and *Scheduling* tables in
`docs/architecture/overview.md` (required by `app_rules.md`) - they summarise the
composition and link to it; the composition is the detailed source.

## 3. Mode `<SwcName>`

### 3.1 Software requirements

Write `docs/specs/software/<SwcName>.md` from `docs/specs/templates/requirements_template.md`,
following the rules in `docs/specs/README.md`:

- ID `SWR-<SWC>-nnn`, where `<SWC>` is the name without `Swc`, upper case
  (`SwcShiftCtrl` -> `SWR-SHIFTCTRL-010`). Number in steps of 10 to leave room.
- "The SWC shall ...", one testable statement each, with numbers and units, phrased in
  terms of the SWC's ports and calibration values, not CAN frames.
- Every SWR has a parent `SYS-*` (or `SG-*`); every `SYS-*` allocated to this SWC in the
  composition is covered by at least one SWR. Add derived SWRs (e.g. init values,
  invalid-input handling) with the parent of the requirement they serve.
- Unknown values: `[NEEDS CLARIFICATION: ...]` or `[NC: <value>]`, never a silent guess.
- Status `Draft`.

### 3.2 Design

Create `docs/modules/<SwcName>/` and start `design.md` from `templates/design.md`. Fill every
section; write "None" rather than deleting one, so the reviewer sees it was considered.

| File | Content |
|---|---|
| `design.md` | Purpose, requirements, ports, runnables, behaviour, data and scaling, calibration, fault handling, test design, open questions; links to every diagram |
| `component.puml` | The SWC with its ports and the peer SWCs / RTE they connect to |
| `runnables.puml` | Runnables, their triggers and the ports / IRVs each one reads and writes |
| `state.puml` | State machine(s) of the SWC |
| `activity_<runnable>.puml` | Algorithm of each runnable with control logic (lower-case snake case, e.g. `activity_shift_schedule.puml`) |
| `seq_<scenario>.puml` | One sequence per key scenario (e.g. `seq_upshift.puml`, `seq_limp_home_entry.puml`) |

What each part must show:

- **Ports.** Exactly the ports of this SWC in the composition (same names, types, scaling).
  For each required port: what the SWC does with an invalid value, a timeout or the init
  value before the first reception. A port that is missing from the composition is a change
  to the composition: update it too and say so in the report.
- **Runnables.** `<SwcName>_<Name>` (e.g. `SwcShiftCtrl_Run10ms`), no parameters, `void`.
  Trigger, period, task, estimated WCET, ports / IRVs accessed, and an `Init` runnable for
  the init values. Runnables of one SWC share data only through IRVs or static module data
  documented here.
- **State.** Every state machine the requirements imply (e.g. selected gear, shift phase,
  limp-home latch). Each transition is labelled `event [guard] / action` with the SWR ID;
  every state has a defined entry from init and a reaction to every fault in scope.
- **Activity.** For each runnable with control logic: the decisions in order, with the
  calibration values and SWR IDs at the decision nodes. This is the picture the reviewer
  checks the algorithm against - it must match the state diagram.
- **Sequence.** At least: initialisation (init runnable, first cycle with init values); the
  main functional scenario of the SWC; one fault scenario ending in the reaction (substitute
  value, limp-home, fault bit). Participants are the OS task, the runnables, `Rte` and the
  peer SWCs - not an abstract "User".
- **Data and scaling.** Every internal and port variable with type, LSB (e.g.
  `1 LSB = 0.25 rpm`), offset, physical range and raw range. Show for each arithmetic step
  that cannot overflow (widest intermediate value) and how it rounds. `float32` only with a
  stated reason (`app_rules.md`).
- **Calibration.** Every value for `config/<SwcName>_Cfg.c`: name with unit suffix
  (`_rpm`, `_kph`, `_degC`, `_ms`), type, value, raw value, source (`SYS-*`,
  `system_context.md` §4.1). Values shared with the plant model are marked.
- **Fault handling.** Each fault the SWC detects or reacts to: detection condition,
  debounce time, reaction, latch / recovery, fault bit, parent `SYS-FLT-*` / `SG-*`.
- **Test design.** Unit test cases (Unity + CMock, with `Rte_*` mocked) and target tests
  per SWR: input, expected output, type (nominal / boundary / invalid / fault /
  transition). Every SWR has at least one test; every state transition is covered.

PlantUML conventions (the same as `module-design`, so all diagrams read alike):

- `@startuml <SwcName>_<file name>` and a `title`.
- Names exactly as in the design: ports, runnables, calibration names.
- SWR IDs on the element they justify, in a `note` or label, e.g.
  `[speed > Upshift12_kph] ' SWR-SHIFTCTRL-020`.
- No colours or skin parameters.
- Assumptions marked `<<assumption>>` or in a note starting with `ASSUMPTION:`, and listed
  under *Open questions*.

### 3.3 Traceability and overview

- Add the SWRs to `docs/specs/traceability.md` section 2 (`SYS` -> `SWR` -> planned test,
  status `Draft`).
- Add the link `docs/modules/<SwcName>/design.md` to the *Design notes* column of
  `docs/architecture/overview.md`.

## 4. Check (both modes)

- If `plantuml` is installed, run `plantuml -checkonly` on the new `.puml` files and fix any
  error. If it is not installed, say so; do not claim the diagrams render.
- Composition: every application `SYS-*` allocated once; every port has one producer and at
  least one consumer; every runnable is in a task.
- SWC: every allocated `SYS-*` -> at least one SWR; every SWR -> design element and test;
  every port in the design matches the composition; every state transition appears in an
  activity or sequence diagram; every calibration value used in a diagram is in the
  calibration table; no arithmetic step without an overflow argument.

## 5. Report and stop

Report the files created or changed, the allocation (composition) or the SWR list (SWC),
the `Draft` `SYS-*` the design depends on, the assumptions and the open questions. Then
stop: the user reviews the design and approves the SWRs. Next steps - the next SWC design,
then `new-module <SwcName> app` to scaffold, then the implementation task.
