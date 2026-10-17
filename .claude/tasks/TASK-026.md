# TASK-026: System design - component and sequence diagrams (PlantUML)

| Field        | Value                                                                    |
|--------------|--------------------------------------------------------------------------|
| Priority     | P0 (input for the SWC composition, the simulation tools and the bootloaders) |
| Depends on   | — (system specs approved; spec findings resolved on 2026-10-17)          |
| Module(s)    | System design (`docs/architecture/system/`), documentation only          |
| AUTOSAR doc  | —                                                                        |

---

## 0. Plan contents (task-specific)

Run with the `module-task` skill. No file is written before the plan is approved. The steps
build, unit test and AUTOSAR compliance of the skill do not apply (no code). The plan must
also contain:
- the final list of system elements and interfaces (section 3, item 2), with the source
  document of every interface;
- the final scenario catalogue (section 5.2): scenario ID, title, `.puml` file, SYS / SIM
  IDs covered; variants merged into one diagram are named;
- the requirement coverage check: every `Approved` `SYS-*` mapped to at least one diagram,
  or listed with the reason why no diagram shows it (e.g. a property of a calibration
  table, checked by `RV` / `AN`);
- every further inconsistency or gap found in `docs/specs/` while preparing the plan, with
  a proposal. The specs are not changed by this task.

## 1. Goal

Describe the **system architecture** of xAT: which system elements exist (TCU board and its
software parts, CAN gateway, host tools, CAN bus), how they are connected, which
requirements each element carries, and how they interact over time in **every case the
approved requirements define** - nominal operation, each fault and its reaction, diagnostics,
and every reprogramming path including the negative ones.

The result is a set of PlantUML **component diagrams**, **sequence diagrams** and one
**state diagram** of the TCU operating modes, with one design document that indexes them and holds the requirement allocation and coverage. It is
the reviewed input for the SWC composition (`swc-design composition`, TASK-018), the plant
model and console (TASK-016), the UDS tester (TASK-022) and the PBL / SBL (TASK-023/024), and
the scenario catalogue is the base for the target tests (`TT`).

**This is system design, not software design.** The TCU is seen from outside, or split only
into elements that the system specs already name (application, PBL and SBL images, board
I/O). The internal software structure - AUTOSAR modules, SWCs, RTE ports, runnables, OS
tasks, APIs - is designed later in `module-design` and `swc-design` and must not appear here.

## 2. Context (verified on 2026-10-17)

- **Inputs, all `Approved`:**
  - `docs/specs/system/system_context.md` - nodes, logical signals, transmission model,
    operating modes Init / Normal / Limp-home / Programming, design constraints DC-01..05;
  - `docs/specs/system/system_requirements.md` - `SYS-SEL/SFT/ACT/SEN/FLT/COM/TIM/STA/HMI/DIA/BOOT`;
    `SYS-BOOT-003` and `SYS-BOOT-012` are `Deleted` and are not designed;
  - `docs/specs/system/safety_goals.md` - SG-01..06;
  - `docs/specs/system/can_matrix.md` - messages 0x100/0x101/0x102 (plant -> TCU),
    0x200/0x201 (TCU -> plant), diagnostic IDs (section 5), timeouts and substitute values
    (section 6); single source for IDs and signals (DC-03);
  - `docs/specs/system/reprogramming.md` - software parts, the 19-step programming
    sequence (section 2), sessions and security (section 3), DIDs, memory map, image header,
    boot info, PBL start-up decision (section 9), robustness rules RB-01..06, routines,
    timing;
  - `docs/specs/simulation/sim_requirements.md` - plant model, gateway, console, tester
    (`SIM-PLT/GW/CON/TST`);
  - `docs/specs/traceability.md` - SG -> SYS.
- **Spec update 2026-10-17 (findings F-01 .. F-18 of the plan, see `docs/development_log.md`):**
  system_requirements 0.3 (new SYS-FLT-008, SYS-BOOT-015), can_matrix 1.2, reprogramming
  0.4, system_context 0.2 (mode "Programming" renamed "Bootloader"), traceability 0.3. Where
  section 5.2 below differs (e.g. `SIM_Analog` timeout now enters limp-home), the specs and
  the approved plan apply.
- **Existing architecture docs:** `docs/architecture/overview.md` (layers and module list -
  software view; it gets a link to the system design, nothing else). `docs/modules/` is
  empty.
- **PlantUML:** `/usr/bin/plantuml` version **1.2020.02** (old). Newer syntax (e.g. some
  `!function` / JSON features, recent `teoz` options) may not render; the diagrams must
  render with this version. Java is installed.
- **Diagram conventions** follow `module-design` / `swc-design`: `@startuml <name>`, a
  `title`, no colours or skin parameters, assumptions marked `ASSUMPTION:`, check with
  `plantuml -checkonly`.

## 3. Scope

### In scope

1. `docs/architecture/system/system_design.md`:
   - purpose, inputs (document + version), the rule "system level only" (section 5.1);
   - system elements: table with element, kind (HW / SW part / host tool / bus), node,
     responsibility, specified in;
   - external interfaces: table with interface, between, medium / protocol, content,
     source document (CAN messages, UDS over ISO-TP, SLCAN over USB VCOM, SocketCAN,
     debug UART, red LED, SWD debugger, keyboard / terminal);
   - operating modes and software parts over time: Init / Normal / Limp-home /
     Programming, and which image (PBL / application / SBL) runs in each - as a table that
     links to the scenarios;
   - requirement allocation: every `Approved` `SYS-*` and `SIM-*` allocated to the system
     element(s) that carry it;
   - scenario catalogue (section 5.2) with links to the `.puml` files;
   - coverage matrix: `SYS-*` -> scenario(s); requirements without a diagram with reason;
     SG-01..06 -> scenarios that show the safe state;
   - open questions and spec findings.
2. Component diagrams in `docs/architecture/system/diagrams/`:
   - `cmp_system_context.puml` - physical / deployment view: Pi5 host (plant model, console,
     UDS tester, `slcand` / SocketCAN `can0`), FRDM-MCXN236 gateway, CAN bus 500 kbit/s with
     terminations, FRDM-MCXN947 TCU (J10, TJA1057, red LED, debug UART, MCU-Link SWD);
     every link labelled with medium and protocol;
   - `cmp_system_interfaces.puml` - logical view: the TCU as one black box with its input and
     output interfaces (CAN messages and their signals, diagnostic request / response, red
     LED, debug UART) and the host tools that produce / consume them; the gateway shown as a
     transparent element (DC-05);
   - `cmp_tcu_software_parts.puml` - the TCU's system-level software parts PBL, SBL and
     application as separate images (DC-04), the memory areas they live in or write
     (PBL flash, application area, boot info, SBL load area in SRAMX), the boot info /
     image header as interfaces, and who may erase / write what (`SYS-BOOT-004`, `-006`).
3. Sequence diagrams `docs/architecture/system/diagrams/seq_<area>_<scenario>.puml`, one
   per scenario of section 5.2. Closely related variants may share one diagram as `alt` /
   `opt` fragments when it stays readable; the catalogue names them. The TCU is one black
   box; only the boot scenarios split it into the software parts PBL / SBL / application.
   Internal decisions are notes on the TCU lifeline (decision of open question 2).
4. State diagram `docs/architecture/system/diagrams/stm_tcu_modes.puml`: the system-level
   modes of the TCU - Reset, PBL (boot window, default session, programming session), SBL,
   application Init / Normal / Limp-home - with every transition labelled by its trigger,
   guard and requirement ID (e.g. "fault of SYS-FLT-002", "power cycle", "`10 02` accepted",
   "watchdog", "S3 timeout"). Each transition links to the scenario(s) that show it, so
   the state diagram is the index of the sequence diagrams.
5. `docs/architecture/overview.md`: a link to the system design (no other change).
6. `docs/development_log.md`, `.claude/tasks/backlog.md`.

### Out of scope

- Any change to `docs/specs/` (findings go to section 7 / the design's open questions; a
  spec change is a separate step approved by the user).
- Software design: AUTOSAR modules, SWC split, RTE ports, runnables, OS tasks, APIs, data
  types, fixed-point scaling, calibration table contents.
- Internal design of the plant model, console, tester and gateway firmware (their own tasks).
- Activity and timing diagrams; state diagrams other than the TCU modes.
- Rendered images (SVG / PNG) in the repository: only `.puml` sources are committed.
- Code, tests, `src/`.

## 4. Files (expected)

| File | Action |
|---|---|
| `docs/architecture/system/system_design.md` | add |
| `docs/architecture/system/diagrams/cmp_system_context.puml` | add |
| `docs/architecture/system/diagrams/cmp_system_interfaces.puml` | add |
| `docs/architecture/system/diagrams/cmp_tcu_software_parts.puml` | add |
| `docs/architecture/system/diagrams/seq_*.puml` (one per scenario, section 5.2) | add |
| `docs/architecture/system/diagrams/stm_tcu_modes.puml` | add |
| `docs/architecture/overview.md` | change (link only) |
| `docs/development_log.md`, `.claude/tasks/backlog.md` | change |

## 5. Requirements

### 5.1 Design rules for this task

| ID | Rule | Parent |
|---|---|---|
| REQ-026-01 | Lifelines and components are **system elements only**: driver / console user, console, plant model, UDS tester, host CAN interface (`slcand` / SocketCAN), gateway, CAN bus, TCU (black box), TCU software parts PBL / SBL / application, board I/O (red LED, debug UART), debugger. No AUTOSAR module, SWC, RTE port, runnable, OS task or C function appears. | CLAUDE.md, this task |
| REQ-026-02 | Messages are the system interfaces: CAN messages with their relevant signal values (`can_matrix.md` names), UDS requests / responses as bytes (`10 02`, `50 02`, `7F 10 22`, ...), physical effects in the plant (gear engaged, line pressure), lamp / LED state. Internal TCU decisions are shown as notes or self-messages on the TCU lifeline, worded as in the requirement (e.g. "selector fault set", "enter limp-home (latched)"). | SYS-*, DC-03 |
| REQ-026-03 | Every time constraint of a requirement shown in a scenario is drawn on the diagram (PlantUML `{t0}` / `<->` timing or a note), with its value and unit, e.g. debounce 50 ms, limp-home outputs <= 100 ms, control active <= 500 ms, boot window 50 ms, reset <= 20 ms after `50 02`, S3 5000 ms. | SYS-* |
| REQ-026-04 | Every diagram names the `SYS-*` / `SIM-*` / `SG-*` IDs it shows (in the title, a header note or next to the step). | traceability |
| REQ-026-05 | Every `Approved` `SYS-*` is covered by at least one diagram, or is listed in the coverage matrix with the reason why not. Every SG-01..06 has at least one scenario that ends in its safe state. | system_requirements.md, safety_goals.md |
| REQ-026-06 | A step that is not stated by a spec is marked `ASSUMPTION:` and listed in the open questions of `system_design.md`. Values are taken from the specs, never invented. | specs README rule 3 |
| REQ-026-07 | Every `.puml` file renders without error with the installed PlantUML 1.2020.02. | — |

### 5.2 Scenario catalogue (to be completed in the plan)

The plan completes this list; no case defined by an approved requirement may be missing.
File names are proposals.

**Start-up, modes, watchdog (`seq_sta_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-STA-01 | Power-on with a valid application, no tester: PBL check, 50 ms boot window without `10 02`, jump to the application, Init (no `TCU_Actuators`, plant in hydraulic default after 50 ms), control active <= 500 ms, cyclic `TCU_Actuators` 10 ms / `TCU_Status` 100 ms | SYS-BOOT-001, -011, -013, SYS-ACT-004, SYS-STA-001, SYS-COM-004, SYS-TIM-001, SIM-PLT-001, -004 |
| SC-STA-02 | Power-on with no valid application (erased / partially programmed / bad header or CRC): PBL check without processor fault, stays in default session without time limit, no `TCU_*` messages, plant in hydraulic default | SYS-BOOT-002, -008, -013, SG-06 |
| SC-STA-03 | Application stops its 10 ms cycle: watchdog reset <= 100 ms, plant falls back to hydraulic default, restart through the PBL (SC-STA-01) | SYS-STA-002, SIM-PLT-004 |
| SC-STA-04 | Normal cycle: plant frames every 10 ms, TCU computes vehicle speed and commands outputs every 10 ms, status every 100 ms, optional debug status line every 500 ms | SYS-SEN-001, SYS-TIM-001, SYS-HMI-002, SYS-COM-001 |

**Gear selector (`seq_sel_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-SEL-01 | Selector change accepted after 50 ms unchanged; a glitch shorter than 50 ms is ignored | SYS-SEL-001 |
| SC-SEL-02 | Invalid selector > 100 ms: treated as N, selector fault set; recovers after 500 ms valid. Variants: not in D (no limp-home) / in D (limp-home, stays latched after recovery) | SYS-SEL-002, SYS-FLT-002, -004, SG-03 |
| SC-SEL-03 | Shift lock: leaving P with brake pressed at standstill (SLS = ON, plant accepts); without brake or above standstill (SLS = OFF, plant ignores the console request) | SYS-SEL-003, SIM-PLT-005, SG-03 |
| SC-SEL-04 | Reverse: R at standstill with valid speed (RLS = ON, reverse line pressure >= 15 bar, plant engages reverse; RLS stays ON while R remains); R while moving >= 2 km/h or with invalid speed (RLS = OFF, plant stays in neutral), also in limp-home | SYS-SEL-004, SYS-ACT-003, SIM-PLT-002, SG-01 |
| SC-SEL-05 | Outputs in P / R / N: solenoid pattern of the gear that D would get (1st at standstill, otherwise schedule within the overspeed limit); plant engages no forward gear | SYS-SEL-005, SG-03 |

**Shifting (`seq_sft_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-SFT-01 | Drive engagement N/P -> D at standstill: 1st gear | SYS-SFT-006, SYS-ACT-001 |
| SC-SFT-02 | Drive engagement while rolling: scheduled gear, limited by the 6000 rpm prediction | SYS-SFT-006, -005, SG-02 |
| SC-SFT-03 | Upshifts 1 -> 4 while accelerating: target from the schedule, one step at a time, >= 2000 ms between shifts, shift pressure profile during the shift, completion when the ratio stays within +/-5 % for 100 ms (plant ratio change over 400 ms) | SYS-SFT-001, -003, -004, -007, SYS-ACT-001, -002, SIM-PLT-003 |
| SC-SFT-04 | Downshifts while decelerating (with hysteresis) and a multi-step target (e.g. kick-down 4 -> 2) done as single steps | SYS-SFT-001, -002, -003, -004 |
| SC-SFT-05 | Downshift refused by overspeed protection: current gear kept until the prediction is <= 6000 rpm | SYS-SFT-005, SG-02 |
| SC-SFT-06 | Shift not complete within 800 ms: shift fault, limp-home; no check while OutputShaftSpeed <= 200 rpm | SYS-SFT-007, SYS-FLT-002, -003 |
| SC-SFT-07 | Cold oil: 4th inhibited at start-up (<= -5 degC), inhibit set below -10 degC with downshift 4 -> 3, released above -5 degC | SYS-SFT-008 |

**Sensors and fault reactions (`seq_flt_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-FLT-01 | Generic limp-home entry: any fault of SYS-FLT-002 -> limp-home outputs within 100 ms (SS_A = SS_B = OFF, PCS_Duty = 0 %, FaultLamp = ON, red LED ON), interlocks still active, latched after the fault disappears until power cycle | SYS-FLT-001 .. -004, SYS-HMI-001, SG-04 |
| SC-FLT-02 | Shaft speed > 8000 rpm: invalid, limp-home, RLS = OFF | SYS-SEN-002, SYS-FLT-002 |
| SC-FLT-03 | Oil temperature sensor out of 200 .. 4800 mV > 100 ms: substitute 80 degC, lamp ON, stays Normal | SYS-SEN-003, -004, SYS-FLT-005 |
| SC-FLT-04 | Line pressure sensor out of range > 100 ms: sensor fault, limp-home | SYS-SEN-004, SYS-FLT-002 |
| SC-FLT-05 | Ratio plausibility (console injects solenoid stuck / speed offset): ratio fault after 500 ms, limp-home | SYS-SEN-005, SIM-CON-003 |
| SC-FLT-06 | Pressure plausibility (console injects line pressure stuck): pressure fault after 500 ms, limp-home | SYS-SEN-006, SIM-CON-003 |
| SC-FLT-07 | Oil overtemperature > 130 degC: fault lamp ON only (warning) | SYS-FLT-006 |
| SC-FLT-08 | Fault memory: fault sets DTC (testFailed, confirmedDTC), read with `19 01` / `19 02`, clear with `14 FF FF FF` (fault still present -> set again; limp-home stays), no update while `85 02`, cleared by power cycle | SYS-FLT-007, SYS-DIA-002, SIM-TST-003 |

**CAN communication (`seq_com_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-COM-01 | Reception timeout, one variant per message: `SIM_Driver` (substitutes, limp-home), `SIM_Speed` (speeds invalid, limp-home), `SIM_Analog` (substitute oil temperature, pressure plausibility suspended, no limp-home) | SYS-COM-002, can_matrix section 6 |
| SC-COM-02 | Alive counter frozen or jumped by > 2: message ignored; 3 consecutive ignored -> treated as timeout | SYS-COM-003 |
| SC-COM-03 | Communication control: `28` stops / restarts `TCU_Actuators` / `TCU_Status`, plant falls back to hydraulic default; S3 timeout restarts transmission | SYS-COM-005, SYS-DIA-003, SIM-PLT-004 |
| SC-COM-04 | Bus-off: recovered within 1000 ms (no fault) / not recovered (bus-off fault, limp-home); gateway reports bus-off | SYS-COM-006, SIM-GW-003 |

**Diagnostics in the application (`seq_dia_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-DIA-01 | Sessions `10 01` / `10 03`, tester present `3E`, S3 = 5000 ms timeout back to default undoing `28` / `85` | SYS-DIA-001 .. -003 |
| SC-DIA-02 | Read identification DIDs with `22` in the application and in the bootloader | SYS-DIA-005, SIM-TST-003 |
| SC-DIA-03 | Security access `27 01` / `27 02` in the extended session; ECU reset `11 01` | SYS-DIA-002 |
| SC-DIA-04 | `10 02` refused: from the default session (NRC `7E`); from extended while moving, not in P or speed invalid (NRC `22`) | SYS-DIA-004, SG-05, SIM-TST-002 |

**Bootloader and reprogramming (`seq_boot_*`)**

| ID | Scenario | Covers |
|---|---|---|
| SC-BOOT-01 | Nominal programming, steps 1-19 of `reprogramming.md` section 2: preconditions, `85`, `28`, `10 02` hand-over (app `50 02`, reset <= 20 ms, PBL `50 02` in the boot window), security, SBL download and activation, fingerprint, erase with `7F 31 78`, application download, checkMemory, dependencies, `11 01`, application starts; `3E 80` every 2 s; no `TCU_*` messages from PBL / SBL; total <= 60 s for 256 KiB | SYS-BOOT-001, -004, -005, -006, -008, -010, -011, -014, SYS-DIA-004, SIM-TST-001 |
| SC-BOOT-02 | Security access failures: wrong key; after 3 failures NRC `36` / `37` for 10 s; download refused without security | SYS-BOOT-005, SIM-TST-002 |
| SC-BOOT-03 | Wrong CRC at checkMemory (SBL or application) and failed checkProgrammingDependencies: negative result, application stays invalid, next reset stays in the PBL | SYS-BOOT-001, -002, SIM-TST-002 |
| SC-BOOT-04 | Erase / write outside the application and boot info areas refused | SYS-BOOT-006, SIM-TST-002 |
| SC-BOOT-05 | Programming interrupted (reset, power loss, tester stops) after each step: next reset ends in the PBL default session, a full new sequence succeeds | SYS-BOOT-007, -002, -013, SIM-TST-002 |
| SC-BOOT-06 | Session timeout in the bootloader (PBL or SBL programming session): S3 = 5000 ms -> reset; valid application -> application starts, invalid -> PBL default session | SYS-BOOT-009, -001, -002 |
| SC-BOOT-07 | Recovery through the boot window: application faults or resets at start-up; tester sends `10 02` every 5 ms, ECU reset, PBL answers `50 02` within the 50 ms window | SYS-BOOT-011, SIM-TST-002 |

## 6. Acceptance criteria

| AC | Criterion | How verified |
|---|---|---|
| AC0 | The plan (with the final element list, scenario catalogue and coverage check) was approved before any file was written | — |
| AC1 | Every `.puml` file renders without error | `plantuml -checkonly docs/architecture/system/diagrams/*.puml`; render to the scratchpad (`-tsvg -o <dir>`) and look at each diagram |
| AC2 | Every `Approved` `SYS-*` appears in the coverage matrix with a diagram or a reason; every SG-01..06 has a scenario ending in its safe state; every scenario of section 5.2 has a diagram | review of `system_design.md` against `system_requirements.md` |
| AC3 | No software-design element in the diagrams: no AUTOSAR module, SWC, RTE, runnable, task or API names | `grep -nwE 'CanIf\|PduR\|CanTp\|Dcm\|Dem\|Rte\|SchM\|EcuM\|Os\|Swc[A-Za-z]*\|runnable\|Rte_[A-Za-z_]*' docs/architecture/system/diagrams/*.puml` returns nothing, plus review |
| AC4 | CAN IDs, signal names, UDS bytes, NRCs, timings and limits match `can_matrix.md`, `reprogramming.md` and `system_requirements.md` | review (spot-check every diagram) |
| AC5 | `docs/specs/` is unchanged | `git diff --stat main -- docs/specs/` is empty for this task's commits |
| AC6 | Docs updated: `overview.md` link, development log, backlog | review |

## 7. Open questions

1. ~~Fault memory vs. application services.~~ **Resolved (user, 2026-10-17):** `14` /
   `19` added to `SYS-DIA-002`, DTC column in `can_matrix.md` 3.6, volatile DTC memory.
2. ~~How far to open the TCU in the sequences.~~ **Decided (user, 2026-10-17):** TCU as one
   black box, split into PBL / SBL / application only in the boot scenarios; internal
   decisions as notes. No logical function blocks (they overlap with the SWC composition).
3. ~~Extra diagram types.~~ **Decided (user, 2026-10-17):** add `stm_tcu_modes.puml`
   (section 3, item 4).
4. ~~Rendered images.~~ **Decided (user, 2026-10-17):** only `.puml` sources are committed.
5. ~~Bootloader S3 timeout with an invalid application.~~ **Resolved (user, 2026-10-17):**
   `SYS-BOOT-009` always resets; the PBL start-up decision then applies.
6. ~~Traceability §3 cites deleted IDs.~~ **Resolved (user, 2026-10-17):** explicit ID
   lists in `traceability.md` section 3.
7. ~~Backlog dependencies.~~ **Decided (user, 2026-10-17):** TASK-018 depends on TASK-026;
   TASK-016, TASK-022 and TASK-023 use the system design as input without a hard
   dependency.
