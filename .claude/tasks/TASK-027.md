# TASK-027: SWC composition - RTE ports (signal flow) and runnable-to-task mapping (scheduling)

| Field        | Value                                                                                 |
|--------------|---------------------------------------------------------------------------------------|
| Priority     | P0 (input for `Rte` TASK-017, the SWC designs TASK-018 and the OS / `SchM` configuration TASK-009) |
| Depends on   | TASK-026 (system design, DONE)                                                        |
| Module(s)    | SWC composition (`docs/architecture/`), documentation only                            |
| AUTOSAR doc  | `docs/references/autosar/AUTOSAR_SWS_RTE.md` R21-11 (port / runnable terms only), `docs/references/autosar/AUTOSAR_SWS_OS.md` R21-11 (task model); chapters in section 2 |

---

## 0. Plan contents (task-specific)

Run with the `module-task` skill; the design work follows `swc-design composition`
(`.claude/skills/swc-design/SKILL.md` section 2). No file is written before the plan is
approved. The steps build, unit test and AUTOSAR compliance of `module-task` do not apply
(no code). The plan must also contain:
- the SWC split (section 3, item 1) with one line per SWC: the four SWCs in `overview.md`
  plus the fault / mode manager (open question 1), and the boundary between them (which SWC
  detects a fault, which one decides the mode);
- the allocation of all 61 `Approved` `SYS-*` (application owner + contributors, or "not
  application" with the owning module);
- the draft port list (name, interface, producer, consumer, period) - types and scaling may
  stay open in the plan but must be complete in the result;
- the draft task / runnable table with the execution order inside the 10 ms task and the
  end-to-end latency estimate for the fault reaction (`SYS-FLT-003`, 100 ms);
- the interfaces between SWCs and BSW services that are not `Com` signals (section 5.2),
  including the `IoHwAb` signal list for TASK-028;
- every inconsistency or gap found in `docs/specs/` or in the system design, with a
  proposal. The specs are not changed by this task.

## 1. Goal

Define the **software structure of the application** at composition level: which SWCs
exist, which `SYS-*` each one owns, every data flow between SWCs and between an SWC and the
RTE (and through the RTE to `Com` or a BSW service), and which runnable runs in which OS
task in which order.

The result fills the two open sections of `docs/architecture/overview.md` - **Signal flow**
(RTE port table) and **Scheduling** (OS task table) - and is the reviewed input for:
- TASK-017 `Rte`: the port list, the port -> `Com` signal mapping (DC-01) and the runnable
  calls per task;
- TASK-018 SWCs: one `swc-design <SwcName>` per SWC starts from these ports
  (the skill stops if the composition is missing);
- TASK-009 OS / `SchM`: the task set, priorities and the order of runnables and BSW main
  functions.

Until now this step was part of TASK-018, which comes after TASK-017; the RTE would have
been built without its port list. This task moves the composition in front of both.

## 2. Context (verified on 2026-10-18)

- **Inputs, all `Approved`:** `system_requirements.md` (61 `Approved`, `SYS-BOOT-003` and
  `SYS-BOOT-012` `Deleted`), `system_context.md` (logical signals §3, parameters §4.1,
  operating modes §5, design constraints DC-01..05), `safety_goals.md`, `can_matrix.md` 1.3
  (single source for signals and scaling, DC-03; timeouts and substitute values §6),
  `traceability.md`.
- **System design (TASK-026):** `docs/architecture/system/system_design.md` - requirement
  allocation to system elements, 42 scenario diagrams, `stm_tcu_modes.puml`. The scenarios
  give the required data flows and timings (e.g. SC-STA-04 normal cycle, SC-FLT-01 limp-home
  entry <= 100 ms, SC-SFT-03 shift sequence).
- **Timing:** `SYS-TIM-001` - sensor inputs, selector logic, shift control, actuator commands
  and fault detection every 10 ms, status reporting every 100 ms. `SYS-TIM-002` - every task
  within its period, average CPU load < 50 %. Backlog TASK-009 names periodic tasks
  1 / 10 / 100 ms.
- **Starting split** (`overview.md`): `SwcGearSelector`, `SwcVehicleSpeed`, `SwcShiftCtrl`,
  `SwcSolenoidCtrl`; plus a fault / mode manager SWC (decided, open question 1).
- **`IoHwAb`:** `CLAUDE.md`, `layering.md` and `overview.md` say `IoHwAb` comes only with a
  later hardware I/O variant. Decided (open question 4): `IoHwAb` is created now for the
  board I/O (red LED), as module task TASK-028. DC-01 in `system_context.md` stays true
  (sensor and actuator values still come from `Com`) and is not changed.
- **AUTOSAR references (added 2026-10-19):** `docs/references/autosar/AUTOSAR_SWS_RTE.md`
  (doc 84, ~80 000 lines) and `docs/references/autosar/AUTOSAR_SWS_OS.md` (doc 34,
  ~18 000 lines), both CP R21-11, converted from the PDF. The headings have no Markdown `#`
  and the tables are broken by the conversion; find a chapter with
  `grep -nE '^<chapter number> '` and use the table of contents at the top of each file
  (RTE lines ~300..900, OS lines ~270..520). Read only the chapters below; they are used for
  terms, naming and the task model, not as requirements on a generator (the RTE is
  handwritten).
  - RTE: 4.2.2.6 mapping of runnables to OS tasks, 4.2.2.8 `TimingEvent` activated runnables,
    4.2.4 cyclic execution, 4.2.6 data consistency (exclusive areas, inter-runnable
    variables), 4.3.1 sender-receiver (4.3.1.5 implicit / explicit, 4.3.1.7 time-out,
    4.3.1.8 invalidation), 4.3.3.1 inter-runnable variables, 4.4 modes (mode user / mode
    manager), 4.6.2 initialisation of SWCs, 5.2.6.4 API naming conventions, 5.6 API
    (`Rte_Read` / `Rte_Write`, `Rte_IRead` / `Rte_IWrite`, `Rte_Switch`, `Rte_Call`),
    5.7.5 triggering events (5.7.5.1 `TimingEvent`, 5.7.5.14 `InitEvent`), 5.8 `Rte_Start`,
    5.9.1 RTE-COM naming.
  - OS: 4.3 interaction with the RTE, 7.1 core OS (OSEK OS basis, restrictions and
    extensions), 7.3 schedule tables (periodic activation), 7.11 scalability classes (SC1
    expected: single core, no protection), 7.12 hook functions, 8.4 schedule-table and
    counter API. The OSEK task model itself (basic / extended tasks, priorities,
    `ActivateTask`, alarms, conformance classes) is defined in OSEK/VDX OS 2.2.3 (ISO
    17356-3), which is not in `docs/references/`; the plan states which OSEK terms are used.
- **Template:** `.claude/skills/swc-design/templates/composition.md`.
- **Rules:** `.claude/rules/app_rules.md` (ports only via `Rte_<Swc>.h`, RTE without control
  logic, every port listed in `overview.md`), `layering.md`.
- **Existing:** no `docs/architecture/swc_composition.md`, no `docs/modules/`, no
  `docs/specs/software/`, no code in `src/app` or `src/rte`.
- **PlantUML** 1.2020.02; conventions as in TASK-026 / `swc-design` (`@startuml <name>`,
  `title`, no colours, `ASSUMPTION:` notes, `plantuml -checkonly`).

## 3. Scope

### In scope

1. `docs/architecture/swc_composition.md` from the template:
   - SWCs and responsibilities ("job" and "not its job");
   - allocation of every `Approved` `SYS-*`: application (owner, contributors) or not
     application (owning BSW module / boot / plant, with reason);
   - ports: every S/R, C/S or mode port between SWCs and to the RTE, with data element,
     type, unit / scaling, range, init value, producer, consumer(s), period; CAN signals keep
     the matrix name and scaling, the converting SWC is named (DC-02);
   - **RTE mapping table**: each RTE port -> `Com` signal (name and message from
     `can_matrix.md`), `IoHwAb` signal (red LED, open question 4) or BSW service
     (section 5.2); this is what TASK-017 implements and what a later hardware variant
     changes from `Com` to `IoHwAb` (DC-01);
   - the `IoHwAb` signals the application needs (name, direction, type, init / safe value,
     underlying `Dio` channel) - the input for TASK-028;
   - scheduling: OS tasks (period, priority) and, in execution order, the runnables and the
     BSW main functions (`Com` Rx / Tx, `CanTp`, `Dcm`, `Can` polling, watchdog); the BSW
     order is fixed here as a design input that each BSW `module-design` confirms or reports
     a conflict against (open question 2);
   - end-to-end latency: fault reaction (`SYS-FLT-003`), selector -> actuator, shift
     command -> `TCU_Actuators`, sensor timeout -> substitute value; worst case with the
     task order and the `Com` Rx / Tx main function placement;
   - operating modes Init / Normal / Limp-home: who decides, how each SWC learns the mode,
     reaction per SWC; how the limp-home latch and `TCU_Status` mode are produced;
   - open questions and assumptions.
2. Diagrams in `docs/architecture/`:
   - `swc_composition.puml` - component diagram: SWCs with their ports, the RTE, and
     `Com`, `IoHwAb` and the BSW services behind it;
   - `swc_task_cycle.puml` - sequence of one 10 ms task activation (and the 100 ms
     activation): `Com` Rx main function, runnables in order, `Com` Tx main function and
     the other BSW main functions, showing the producer -> consumer order inside one cycle
     (open question 3).
3. `docs/architecture/overview.md`: fill **Signal flow** (RTE port table, summary with a
   link to the composition) and **Scheduling** (OS task table); drop "(to be detailed)".
   Add the fault / mode manager SWC to the layer picture, `IoHwAb` (`bsw/ecual`) to the
   layer picture and the module list, and `swc_composition.md` to the *Design notes*
   column of the SWCs row; replace the sentence "`IoHwAb` ... is added only for a later
   hardware I/O variant".
4. The same `IoHwAb` statement in `CLAUDE.md` (architecture and repository layout) and
   `.claude/rules/layering.md`: `IoHwAb` exists for board I/O; the later hardware variant
   adds the sensor and actuator signals to it.
5. `.claude/tasks/backlog.md` (dependencies, status), `docs/development_log.md`.

### Out of scope

- SWR software requirements and the per-SWC design (`swc-design <SwcName>`, TASK-018):
  state machines, algorithms, calibration contents, fixed-point details beyond the port
  scaling.
- RTE implementation (TASK-017), OS / `SchM` implementation (TASK-009), BSW module designs
  (`module-design`); the BSW main function placement here is a design input they confirm.
- `IoHwAb` module design and code (TASK-028); this task only lists the signals the
  application needs.
- Any change to `docs/specs/` or to the system design (findings go to the open questions).
- Code, tests, `src/`.

## 4. Files (expected)

| File | Action |
|---|---|
| `docs/architecture/swc_composition.md` | add |
| `docs/architecture/swc_composition.puml` | add |
| `docs/architecture/swc_task_cycle.puml` | add |
| `docs/architecture/overview.md` | change (Signal flow, Scheduling, SWC and `IoHwAb` entries, design-notes link) |
| `CLAUDE.md`, `.claude/rules/layering.md` | change (`IoHwAb` statement only) |
| `.claude/tasks/backlog.md`, `docs/development_log.md` | change |

## 5. Requirements

### 5.1 Design rules for this task

| ID | Rule | Parent |
|---|---|---|
| REQ-027-01 | Every `Approved` `SYS-*` is allocated exactly once: one owning SWC (others named as contributors) or "not application" with the owning module. | `swc-design` §2.2 |
| REQ-027-02 | Every port has exactly one producer and at least one consumer, a period, a type with unit / scaling, a range and an init value. | `swc-design` §2.3 |
| REQ-027-03 | Port data from or to CAN keeps the `can_matrix.md` signal name and raw scaling; physical conversion happens in a named SWC, never in the RTE (DC-02, DC-03). | DC-02, DC-03 |
| REQ-027-04 | SWCs reach sensors, actuators and BSW services only through RTE ports; every port has an RTE mapping entry (`Com` signal or BSW service) so that a hardware variant changes only the mapping (DC-01). | DC-01, `app_rules.md` |
| REQ-027-05 | Every runnable is in exactly one task; the order inside a task lets every same-cycle data flow go producer -> consumer. The 10 ms / 100 ms split follows `SYS-TIM-001`. | SYS-TIM-001 |
| REQ-027-06 | The worst-case end-to-end latency is shown for the fault reaction and is <= 100 ms (`SYS-FLT-003`), including the bus-off case (red LED). Any path that cannot meet its requirement is a finding. | SYS-FLT-003 |
| REQ-027-07 | Names follow AUTOSAR RTE naming: ports `<Pp|Rp><Name>`, runnables `<SwcName>_<Name>`; the names are the ones TASK-017 / TASK-018 will use. | `AUTOSAR_SWS_RTE`, `app_rules.md` |
| REQ-027-08 | Values are taken from the specs; anything not stated is marked `ASSUMPTION:` and listed in the open questions. | specs README rule 3 |
| REQ-027-09 | Every `.puml` file renders without error with PlantUML 1.2020.02. | — |

### 5.2 Interfaces that are not `Com` signals (to be resolved in the plan)

The plan proposes a port and an RTE mapping for each:

| Need | Requirement | Candidate |
|---|---|---|
| Red LED ON in limp-home / bus-off | SYS-HMI-001, SYS-FLT-003 | RTE port -> `IoHwAb` signal -> `Dio` channel (decided, open question 4) |
| Debug status line every 500 ms | SYS-HMI-002 | RTE port -> debug UART service (not an AUTOSAR module today) |
| Programming preconditions for `10 02` (standstill, P, speed valid) | SYS-DIA-004, SG-05 | SWC P-port read by `Dcm` (C/S or S/R through the RTE) |
| Faults for the fault memory / DTCs | SYS-FLT-007, SYS-DIA-002 | SWC -> `Dem` event ports (TASK-025); until then `TCU_Status.FaultBits` only |
| Communication state (`28` disable, bus-off, Rx timeout, alive counter) | SYS-COM-002, -003, -005, -006 | `Com` signal status / I-PDU group state -> SWC ports |
| Watchdog supervision of the 10 ms cycle | SYS-STA-002 | `WdgM` checkpoint / alive supervision or direct `Wdg` trigger (TASK-019 decision) |

## 6. Acceptance criteria

| AC | Criterion | How verified |
|---|---|---|
| AC0 | The plan (split, allocation, draft ports, draft scheduling, latency estimate) was approved before any file was written | — |
| AC1 | Every `.puml` file renders without error | `plantuml -checkonly docs/architecture/*.puml`; render to the scratchpad and look at each diagram |
| AC2 | All 61 `Approved` `SYS-*` appear once in the allocation tables | review against `system_requirements.md` (scripted ID diff) |
| AC3 | Every port has one producer, >= 1 consumer, period, type, scaling, range, init value and an RTE mapping entry; every runnable is in a task | review of `swc_composition.md` |
| AC4 | CAN signal names, scaling, cycle times and timeouts match `can_matrix.md` | review (every `Com`-mapped port) |
| AC5 | Worst-case fault reaction latency documented and <= 100 ms | review of section 4.1 |
| AC6 | `overview.md` Signal flow and Scheduling tables filled and consistent with the composition | review |
| AC7 | `docs/specs/` and `docs/architecture/system/` unchanged | `git diff --stat main -- docs/specs/ docs/architecture/system/` empty for this task's commits |
| AC8 | `IoHwAb` statement updated in `CLAUDE.md`, `layering.md`, `overview.md`; backlog and development log updated | review |

## 7. Open questions

1. ~~SWC split.~~ **Decided (user, 2026-10-18):** add a fault / mode manager SWC (working
   name `SwcFaultMgr`; final name in the plan): limp-home decision and latch, operating
   mode, fault bits, fault lamp / red LED. No separate analog sensor SWC; the plan allocates
   `SYS-SEN-003/004/006` to one of the five SWCs.
2. ~~BSW main functions in scheduling.~~ **Decided (user, 2026-10-18):** this task fixes the
   order of the runnables and the BSW main functions in each task as a design input; each
   BSW `module-design` confirms it or reports a conflict.
3. ~~Task cycle diagram.~~ **Decided (user, 2026-10-18):** add `swc_task_cycle.puml`.
4. ~~Red LED path.~~ **Decided (user, 2026-10-18):** create `IoHwAb` now as a usable,
   extensible module (TASK-028); the lamp / red LED port maps to an `IoHwAb` signal. The
   later hardware variant extends the same module with the sensor and actuator signals.
5. ~~Backlog dependencies.~~ **Decided (user, 2026-10-18):** TASK-017 and
   TASK-018 depend on TASK-027 (TASK-018 no longer runs `swc-design composition`). TASK-009
   uses it as input without a hard dependency: its 1 / 10 / 100 ms tasks do not depend on
   the runnable list.
