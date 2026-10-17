# System design

| Field    | Value |
|----------|-------|
| Scope    | xAT system architecture: system elements, interfaces, requirement allocation, scenarios |
| Task     | TASK-026 |
| Version  | 0.2   |
| Status   | Draft (for review) |

## 1. Purpose

This document describes the **system architecture** of xAT. It covers:

- the system elements: the TCU board and its software parts, the CAN gateway, the host tools and the CAN bus;
- how the elements are connected, and which requirements each element carries;
- how the elements interact over time in every case that the approved requirements define:
  nominal operation, each fault and its reaction, diagnostics, and every reprogramming path,
  including the negative ones.

It is the index of the PlantUML diagrams in [`diagrams/`](diagrams/). It holds the requirement
allocation and the coverage, and it is the reviewed input for:

- the SWC composition (TASK-018);
- the plant model and the console (TASK-016);
- the UDS tester (TASK-022);
- the PBL and the SBL (TASK-023, TASK-024).

The scenario catalogue (section 8) is the base for the target tests (`TT`).

## 2. Inputs

All inputs are `Approved`. The baseline is commit `0fcaba6` (spec findings F-01 .. F-18 resolved by `79929ee`, section 10.1; N-01 .. N-11 by `0fcaba6`, section 10.2).

| Document | Version | Content used |
|---|---|---|
| [`system_context.md`](../../specs/system/system_context.md) | 0.2 | nodes, logical signals, transmission model, operating modes (section 5), DC-01 .. DC-05 |
| [`system_requirements.md`](../../specs/system/system_requirements.md) | 0.4 | 61 `Approved` SYS requirements; SYS-BOOT-003 and SYS-BOOT-012 are `Deleted` and are not designed |
| [`safety_goals.md`](../../specs/system/safety_goals.md) | 0.1 | SG-01 .. SG-06 and their safe states |
| [`can_matrix.md`](../../specs/system/can_matrix.md) | 1.3 | CAN IDs, signals, fault bits and DTCs, sensor characteristics, diagnostic IDs, timeouts and substitute values |
| [`reprogramming.md`](../../specs/system/reprogramming.md) | 0.5 | software parts, programming sequence, services per session, DIDs, memory map, image header, boot info, start-up decision, RB-01 .. RB-06, routines, request sequence rules, timing |
| [`sim_requirements.md`](../../specs/simulation/sim_requirements.md) | 0.2 | plant model, gateway, console, tester (18 SIM requirements) |
| [`traceability.md`](../../specs/traceability.md) | 0.3 | SG -> SYS |

## 3. Design rules

**System level only.** This is system design, not software design.

- The TCU is seen from outside as one black box.
- It is split only into the parts that the system specs already name:
  - the PBL, SBL and application images (DC-04);
  - the board I/O: red LED, debug UART, MCU-Link.
- The split into PBL / SBL / application is drawn only where the running part matters:
  - the start-up and boot scenarios (SC-STA-01 .. SC-STA-03, SC-BOOT-*);
  - the DID scenario SC-DIA-02.
- AUTOSAR modules, SWCs, RTE ports, runnables, OS tasks and APIs are not shown. They are designed
  later with `module-design` and `swc-design`.
- Internal TCU decisions appear as notes or self-messages, worded as in the requirement.

**Diagram conventions**:

- **File format:**
  - Every file starts with `@startuml <file name>` and has a `title` with the scenario ID.
  - A header note lists the SYS / SIM / SG IDs that the diagram covers (`Covers:`).
  - No colours, skin parameters or themes.
- **CAN messages** carry their `can_matrix.md` names and the relevant signal values.
- **UDS** is written as hex bytes:
  - requests on `DIAG_PhysReq` 0x7E1, responses on `DIAG_Resp` 0x7E9;
  - `3E 80` on `DIAG_FuncReq` 0x7DF (can_matrix section 5);
  - `50 xx 00 32 01 F4` carries P2 = 50 ms and P2* = 5000 ms in 10 ms units (reprogramming section 12).
- **NRCs and placeholders:**
  - An ISO 14229-1 NRC that no spec names is labelled `(ISO 14229-1)`.
  - Placeholders are written as `<seed>`, `<addr>`, `<memSize>`.
- **Timings** are drawn as `... 50 ms ...` delays, or as notes with value and unit.
  The PlantUML `teoz` engine is not used: version 1.2020.02 drops its timing labels.
- **CAN path:**
  - SC-STA-04 and SC-COM-04 draw the full chain `can0 / slcand -> gateway -> CAN bus`.
  - Every other diagram states that CAN frames pass can0 / slcand / gateway transparently
    (DC-05, SIM-GW-001).
- **Values and assumptions:**
  - Every value is taken from the specs.
  - A step without a spec basis is marked `ASSUMPTION:` and listed in section 10.3.
  - A gap that is not drawn is noted with a reference to its open question in section 10.2.

**Tooling.** All files render with the installed PlantUML 1.2020.02:

```
plantuml -checkonly docs/architecture/system/diagrams/*.puml
plantuml -tpng -o <scratch directory> docs/architecture/system/diagrams/*.puml
```

Only the `.puml` sources are committed; rendered images are not. SC-BOOT-01 renders to three
images (`newpage`).

## 4. System elements

| ID | Element | Kind | Node | Responsibility | Specified in |
|---|---|---|---|---|---|
| E-01 | FRDM-MCXN947 TCU board: MCXN947 MCU (Cortex-M33 core 0), CAN on J10 with TJA1057, MCU watchdog | HW | TCU | runs the TCU software parts; CAN node; watchdog reset (SYS-STA-002) | system_context section 2, can_matrix section 1 |
| E-02 | PBL (primary bootloader), flash 0x0000_0000 .. 0x0000_FFFF | SW part (separate image) | TCU | start-up decision and application check, boot window, minimal UDS, SBL download into SRAMX; never writes flash | reprogramming sections 1, 5, 9; SYS-BOOT |
| E-03 | SBL (secondary bootloader), SRAMX 0x0400_0000, downloaded for each programming session | SW part (separate image) | TCU | erase / write / verify of the application and boot info; only part that writes flash | reprogramming sections 1, 5, 11; SYS-BOOT-004, -006, -015 |
| E-04 | Application, header 0x0001_4000 + image 0x0001_4400 .. 0x000F_FFFF | SW part (separate image) | TCU | transmission control, fault handling, CAN communication, diagnostics, entry into programming | system_requirements; reprogramming section 1 |
| E-05 | Red LED on the TCU board | board I/O | TCU | shows the FaultLamp value | SYS-HMI-001 |
| E-06 | Debug UART (over MCU-Link VCOM) | board I/O | TCU | optional status line every 500 ms | SYS-HMI-002 |
| E-07 | MCU-Link of the TCU board (SWD debugger + VCOM) | HW | TCU board -> Pi5 USB | flashes the PBL and debugger-flashed applications (never a mass erase); carries the debug UART | SYS-BOOT-006, RB-06, `docs/setup_host.md` |
| E-08 | CAN bus, classic CAN 2.0B, 11-bit IDs, 500 kbit/s, sample point 87.5 %, 120 ohm at both ends | bus | - | carries every CAN frame between the TCU and the gateway (only two nodes) | can_matrix section 1 |
| E-09 | CAN gateway FRDM-MCXN236 with SLCAN <-> CAN firmware (NXP SDK application) | HW + SW (test equipment) | gateway | transparent bridge, error reporting, bus-off LED | SIM-GW-001 .. -003, DC-05 |
| E-10 | Raspberry Pi 5 host (Ubuntu aarch64) | HW | host | runs the host tools; builds, flashes and debugs | system_context section 2 |
| E-11 | slcand / SocketCAN `can0` | host SW (Linux) | host | exposes the gateway as a SocketCAN interface shared by all tools | SIM-GW-001, sim_requirements section 1 |
| E-12 | Plant model | host tool | host | driver, vehicle and transmission simulation; sends `SIM_*`, receives `TCU_*` | SIM-PLT-001 .. -007 |
| E-13 | Console | host tool | host | display, driver inputs, fault injection, scripted scenarios and logging | SIM-CON-001 .. -004 |
| E-14 | UDS tester | host tool | host | diagnostics and reprogramming over UDS / ISO-TP | SIM-TST-001 .. -004 |
| E-15 | Console user (driver, tester operator) | actor | - | drives, injects faults, starts the tester | SIM-CON-002 |

Diagram: [`cmp_system_context.puml`](diagrams/cmp_system_context.puml) (physical view, every link
labelled with medium and protocol).

## 5. Interfaces

| ID | Interface | Between | Medium / protocol | Content | Source |
|---|---|---|---|---|---|
| I-01 | `SIM_Driver` 0x100 | plant -> TCU | CAN, 10 ms, RX timeout 30 ms | Selector, Brake, Throttle, AliveCtr | can_matrix sections 2, 3.1, 6 |
| I-02 | `SIM_Speed` 0x101 | plant -> TCU | CAN, 10 ms, RX timeout 30 ms | InputShaftSpeed, OutputShaftSpeed, AliveCtr | can_matrix sections 2, 3.2, 6 |
| I-03 | `SIM_Analog` 0x102 | plant -> TCU | CAN, 10 ms, RX timeout 30 ms | OilTempVoltage, LinePressVoltage, AliveCtr | can_matrix sections 2, 3.3, 4, 6 |
| I-04 | `TCU_Actuators` 0x200 | TCU -> plant, console | CAN, 10 ms (plant timeout 50 ms) | SS_A, SS_B, RLS, SLS, FaultLamp, PCS_Duty, AliveCtr | can_matrix sections 2, 3.4; SIM-PLT-004 |
| I-05 | `TCU_Status` 0x201 | TCU -> plant, console | CAN, 100 ms | GearActual, GearTarget, Mode, SelectorState, ShiftInProgress, VehicleSpeed, FaultBits, OilTemp, AliveCtr | can_matrix sections 2, 3.5, 3.6 |
| I-06 | `DIAG_PhysReq` 0x7E1, `DIAG_FuncReq` 0x7DF | tester -> TCU | UDS over ISO-TP; 0x7DF single frame, only `3E 80` | UDS requests | can_matrix section 5; reprogramming sections 2 .. 4, 11, 12 |
| I-07 | `DIAG_Resp` 0x7E9 | TCU -> tester | UDS over ISO-TP, padding 0xCC, BS 0, STmin 0 ms, N_Bs / N_Cr 1000 ms | UDS responses | can_matrix section 5 |
| I-08 | CAN physical layer | TCU (J10), gateway | classic CAN 2.0B, 11-bit, 500 kbit/s, 87.5 %, TJA1057 | all CAN frames | can_matrix section 1 |
| I-09 | SLCAN | gateway <-> Pi5 | Lawicel ASCII over USB VCOM (MCU-Link of the gateway) | CAN frames, error-state and overflow status flags | SIM-GW-001, SIM-GW-003, system_context section 2 |
| I-10 | SocketCAN `can0` | host tools <-> Linux | raw sockets (plant, console), kernel `can-isotp` (tester) | CAN frames, ISO-TP messages | sim_requirements section 1, SIM-TST-004 |
| I-11 | Debug status line | TCU -> Pi5 | UART over the MCU-Link VCOM, every 500 ms | selector, gear, vehicle speed, mode, fault bits | SYS-HMI-002 |
| I-12 | Red LED; gateway bus-off LED | board -> user | light | FaultLamp value; gateway bus-off | SYS-HMI-001, SIM-GW-003 |
| I-13 | SWD flash / debug | Pi5 <-> TCU | MCU-Link CMSIS-DAP over USB | PBL and debugger-flashed application images; no mass erase | SYS-BOOT-006, RB-06 |
| I-14 | Keyboard / terminal | user <-> console | terminal | driver inputs, fault injection, scripted scenarios, logs | SIM-CON-001 .. -004 |
| I-15 | Console <-> plant | host internal | designed with the console and plant (TASK-016) | driver inputs, injected faults, plant state | SIM-CON-002, SIM-CON-003 |
| I-16 | Image files | file -> tester | `.bin` with 1 KiB header | SBL and application images | SIM-TST-001, reprogramming section 6 |
| I-17a | Application image header | SBL writes, PBL / application read | flash 0x0001_4000 .. 0x0001_43FF | magic, header_crc32, sw_version, hw_id, image_start, image_size, image_crc32, min_pbl_version | reprogramming section 6 |
| I-17b | Boot info record (slots A / B) | SBL writes, PBL / application / SBL read | flash 0x0001_0000 .. 0x0001_3FFF | magic, sequence, programming counter (FD00), fingerprint (F15A), record_crc32 | reprogramming section 7 |
| I-17c | PBL -> SBL hand-over | PBL -> SBL | RAM structure | programming session active, security unlocked, CAN configuration | reprogramming section 1 |

Diagrams:

- [`cmp_system_interfaces.puml`](diagrams/cmp_system_interfaces.puml): the TCU as a black box with
  I-01 .. I-07, I-11 and I-12. The CAN path (I-08 .. I-10, gateway) is drawn as one
  `<<transparent>>` element.
- [`cmp_tcu_software_parts.puml`](diagrams/cmp_tcu_software_parts.puml): the PBL, SBL and
  application with the memory regions of reprogramming section 5, I-17a .. I-17c, and who may
  erase or write what.

## 6. Operating modes and software parts over time

The modes are those of `system_context.md` section 5. The state diagram
[`stm_tcu_modes.puml`](diagrams/stm_tcu_modes.puml) shows every transition with its trigger,
guard, requirement and scenario. It is the index of the sequence diagrams.

| Mode | State | Running part | `TCU_*` transmitted | Entered when | Left when | Scenarios |
|---|---|---|---|---|---|---|
| - | Reset | none | no | power-on, watchdog, software reset (`11 01`, `10 02` hand-over, S3 in a bootloader programming session, `10 01` in a bootloader programming session) | always: PBL application check | SC-STA-01, SC-STA-03, SC-DIA-03, SC-BOOT-06 |
| Bootloader | PBL application check | PBL | no (SYS-BOOT-008) | after every reset | valid -> boot window; invalid -> PBL default session | SC-STA-01, SC-STA-02 |
| Bootloader | PBL boot window (50 ms) | PBL | no | application valid | `10 02` -> PBL programming session; 50 ms without it -> application Init | SC-STA-01, SC-BOOT-01, SC-BOOT-07 |
| Bootloader | PBL default session | PBL | no | application invalid (SYS-BOOT-002) | `10 02` -> PBL programming session (no reset); `11 01` -> Reset; no time limit | SC-STA-02, SC-BOOT-03, SC-BOOT-05, SC-BOOT-06 |
| Bootloader | PBL programming session | PBL | no | boot window or PBL default session | `31 01 03 01` -> SBL; S3 5000 ms, `11 01`, `10 01` -> Reset | SC-BOOT-01, SC-BOOT-02, SC-BOOT-04, SC-BOOT-06 |
| Bootloader | SBL programming session | SBL | no | `31 01 03 01` accepted | `11 01` (step 19), S3 5000 ms, `10 01` -> Reset | SC-BOOT-01, SC-BOOT-03, SC-BOOT-04, SC-BOOT-06 |
| Init | Init | application | no (SYS-ACT-004, SYS-COM-004 note) | jump from the PBL | control active <= 500 ms after reset (SYS-STA-001) | SC-STA-01 |
| Normal | Normal | application | `TCU_Actuators` 10 ms, `TCU_Status` 100 ms | control active, no fault of SYS-FLT-002 | fault of SYS-FLT-002 -> Limp-home; reset | SC-STA-04, SC-SEL-*, SC-SFT-*, SC-FLT-03, SC-FLT-07 |
| Limp-home | Limp-home | application | yes, limp-home outputs (bus-off: when the bus is back) | fault of SYS-FLT-002 (also from Init) | only a reset (SYS-FLT-004) | SC-FLT-01 .. SC-FLT-06, SC-SEL-02, SC-SFT-06, SC-COM-01, SC-COM-02, SC-COM-04 |

Timeline after a power-on with a valid application and no tester (SC-STA-01):

| Time | Event |
|---|---|
| t0 | Reset. Plant frames continue every 10 ms; the PBL does not use them. |
| t0 + a few ms | PBL application check (header, hw_id, min_pbl_version, image CRC32). |
| then 50 ms | Boot window; no `10 02` received. |
| next | Jump to the application, Init. |
| t0 + 50 ms | No `TCU_Actuators` for 50 ms: the plant applies the hydraulic default (SIM-PLT-004). |
| <= t0 + 500 ms | Control active (SYS-STA-001). `TCU_Actuators` every 10 ms, `TCU_Status` every 100 ms. Without plant messages, control becomes active in limp-home (communication faults, SYS-COM-002). |

## 7. Requirement allocation

Every `Approved` SYS and SIM requirement is allocated to the elements of section 4 that carry it.
SYS-BOOT-003 and SYS-BOOT-012 are `Deleted` and are not allocated.

### 7.1 System requirements

| ID | Requirement | Element(s) | Note |
|---|---|---|---|
| SYS-SEL-001 | Selector position | E-04 application |  |
| SYS-SEL-002 | Invalid selector position | E-04 application |  |
| SYS-SEL-003 | Shift lock (brake interlock) | E-04 application |  |
| SYS-SEL-004 | Reverse lockout | E-04 application |  |
| SYS-SEL-005 | Outputs in P, R and N | E-04 application |  |
| SYS-SFT-001 | Shift schedule | E-04 application |  |
| SYS-SFT-002 | Shift hysteresis | E-04 application |  |
| SYS-SFT-003 | Single-step shifts | E-04 application |  |
| SYS-SFT-004 | Minimum time between shifts | E-04 application |  |
| SYS-SFT-005 | Downshift overspeed protection | E-04 application |  |
| SYS-SFT-006 | Drive engagement | E-04 application |  |
| SYS-SFT-007 | Shift completion monitoring | E-04 application |  |
| SYS-SFT-008 | Cold oil | E-04 application |  |
| SYS-ACT-001 | Solenoid pattern | E-04 application |  |
| SYS-ACT-002 | Line pressure control | E-04 application |  |
| SYS-ACT-003 | Line pressure for reverse | E-04 application |  |
| SYS-ACT-004 | Outputs before control is active | E-04 application | from reset until the jump the PBL transmits nothing either (SYS-BOOT-008) |
| SYS-SEN-001 | Vehicle speed | E-04 application |  |
| SYS-SEN-002 | Shaft speeds | E-04 application |  |
| SYS-SEN-003 | Analog sensor conversion | E-04 application |  |
| SYS-SEN-004 | Analog sensor range check | E-04 application |  |
| SYS-SEN-005 | Gear ratio plausibility | E-04 application |  |
| SYS-SEN-006 | Line pressure plausibility | E-04 application |  |
| SYS-FLT-001 | Limp-home behaviour | E-04 application |  |
| SYS-FLT-002 | Limp-home entry | E-04 application |  |
| SYS-FLT-003 | Fault reaction time | E-04 application |  |
| SYS-FLT-004 | Limp-home latch | E-04 application |  |
| SYS-FLT-005 | Substitute oil temperature | E-04 application |  |
| SYS-FLT-006 | Oil overtemperature | E-04 application |  |
| SYS-FLT-007 | Fault memory | E-04 application | volatile DTC memory |
| SYS-FLT-008 | Fault recovery | E-04 application |  |
| SYS-COM-001 | Bus and reception | E-04 application; E-02 PBL, E-03 SBL (bus parameters); E-01 board (J10, TJA1057) | the PBL and SBL use the same bus parameters but do not receive `SIM_*` |
| SYS-COM-002 | Reception timeout | E-04 application |  |
| SYS-COM-003 | Alive counter | E-04 application |  |
| SYS-COM-004 | Transmission | E-04 application |  |
| SYS-COM-005 | Communication control | E-04 application |  |
| SYS-COM-006 | Bus-off | E-04 application; E-01 board (CAN controller) |  |
| SYS-TIM-001 | Cycle times | E-04 application |  |
| SYS-TIM-002 | CPU load | E-04 application | runtime property, no diagram (section 9.1) |
| SYS-STA-001 | Startup time | E-02 PBL, E-04 application | PBL check and 50 ms boot window are part of the 500 ms |
| SYS-STA-002 | Watchdog | E-04 application, E-01 board (MCU watchdog) | no watchdog in the PBL / SBL in v1 |
| SYS-HMI-001 | Fault lamp | E-04 application, E-05 red LED |  |
| SYS-HMI-002 | Debug output | E-04 application, E-06 debug UART | Could |
| SYS-DIA-001 | UDS transport | E-04 application, E-02 PBL, E-03 SBL | same identifiers and ISO-TP parameters in all parts |
| SYS-DIA-002 | Services in the application | E-04 application |  |
| SYS-DIA-003 | Session timeout | E-04 application |  |
| SYS-DIA-004 | Programming preconditions | E-04 application |  |
| SYS-DIA-005 | Identification data | E-04 application, E-02 PBL, E-03 SBL | FD01 only in the SBL |
| SYS-BOOT-001 | Application start | E-02 PBL |  |
| SYS-BOOT-002 | No valid application | E-02 PBL |  |
| SYS-BOOT-004 | Flash API usage | E-03 SBL (only caller); E-02 PBL, E-04 application (must not link the flash functions) | proof by review of the map files |
| SYS-BOOT-005 | Security access | E-02 PBL, E-03 SBL | the PBL hands the unlocked state to the SBL |
| SYS-BOOT-006 | PBL protection | E-03 SBL; E-07 MCU-Link (PBL update only by debugger) |  |
| SYS-BOOT-007 | Interrupted programming | E-02 PBL, E-03 SBL |  |
| SYS-BOOT-008 | Outputs in the bootloader | E-02 PBL, E-03 SBL |  |
| SYS-BOOT-009 | Bootloader session timeout | E-02 PBL, E-03 SBL |  |
| SYS-BOOT-010 | Programming time | E-04 application, E-02 PBL, E-03 SBL, E-14 tester, E-09 gateway | gateway throughput SIM-GW-002 |
| SYS-BOOT-011 | Boot window | E-02 PBL |  |
| SYS-BOOT-013 | Fault-free application check | E-02 PBL |  |
| SYS-BOOT-014 | Programming entry from the application | E-04 application |  |
| SYS-BOOT-015 | Application header written last | E-03 SBL |  |

### 7.2 Simulation requirements

| ID | Requirement | Element(s) |
|---|---|---|
| SIM-PLT-001 | Cycle and transmission | E-12 plant model |
| SIM-PLT-002 | Range and gear engagement | E-12 plant model |
| SIM-PLT-003 | Shift dynamics | E-12 plant model |
| SIM-PLT-004 | Hydraulic default on missing TCU | E-12 plant model |
| SIM-PLT-005 | Shift lock | E-12 plant model |
| SIM-PLT-006 | Vehicle and engine model | E-12 plant model |
| SIM-PLT-007 | Line pressure and oil temperature | E-12 plant model |
| SIM-GW-001 | Transparent SLCAN bridge | E-09 gateway, E-11 slcand / `can0` |
| SIM-GW-002 | Throughput and latency | E-09 gateway |
| SIM-GW-003 | Error reporting | E-09 gateway |
| SIM-CON-001 | Display | E-13 console |
| SIM-CON-002 | Driver inputs | E-13 console |
| SIM-CON-003 | Fault injection | E-13 console |
| SIM-CON-004 | Scenarios and logging | E-13 console |
| SIM-TST-001 | Flashing | E-14 UDS tester |
| SIM-TST-002 | Negative scenarios | E-14 UDS tester |
| SIM-TST-003 | Diagnostic services | E-14 UDS tester |
| SIM-TST-004 | Bus sharing | E-14 UDS tester, E-11 slcand / `can0` |

## 8. Scenario catalogue

Each row is one diagram. Closely related variants share one diagram as `alt` / `opt` fragments;
they are named in the *Variants* column. *Covers* lists the IDs of the diagram's header note.

| ID | Title | File | Variants | Covers |
|---|---|---|---|---|
| SC-STA-01 | Power-on with a valid application, no tester | [`seq_sta_poweron_valid_app.puml`](diagrams/seq_sta_poweron_valid_app.puml) | alt: plant model running (P accepted after 50 ms, Normal) / no plant message (communication faults after 30 ms, control active in limp-home) | SYS-BOOT-001, SYS-BOOT-011, SYS-BOOT-013, SYS-BOOT-008, SYS-ACT-004, SYS-STA-001, SYS-SEL-001, SYS-COM-002, SYS-COM-004, SYS-TIM-001, SYS-FLT-001, SYS-FLT-002, SYS-HMI-001, SIM-PLT-001, SIM-PLT-004 |
| SC-STA-02 | Power-on without a valid application | [`seq_sta_poweron_invalid_app.puml`](diagrams/seq_sta_poweron_invalid_app.puml) | alt: application area erased / partially programmed / header or image invalid; `22 F1 81` -> `FF FF FF FF` | SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-008, SYS-BOOT-013, SYS-DIA-005, SIM-PLT-004, SG-06 |
| SC-STA-03 | Application stops its 10 ms cycle: watchdog reset | [`seq_sta_watchdog_reset.puml`](diagrams/seq_sta_watchdog_reset.puml) | - | SYS-STA-002, SYS-STA-001, SYS-ACT-004, SYS-FLT-004, SYS-FLT-008, SYS-FLT-007, SIM-PLT-004, SG-04 |
| SC-STA-04 | Normal cycle over the full CAN path | [`seq_sta_normal_cycle.puml`](diagrams/seq_sta_normal_cycle.puml) | opt: debug status line every 500 ms (SYS-HMI-002, Could) | SYS-COM-001, SYS-COM-003, SYS-COM-004, SYS-SEN-001, SYS-TIM-001, SYS-HMI-002, SIM-PLT-001, SIM-PLT-006, SIM-GW-001, SIM-GW-002, SIM-CON-001, SIM-CON-002 |
| SC-SEL-01 | Selector debounce: change accepted after 50 ms, short glitch ignored | [`seq_sel_debounce.puml`](diagrams/seq_sel_debounce.puml) | change accepted after 50 ms; glitch shorter than 50 ms ignored | SYS-SEL-001, SIM-CON-002, SIM-PLT-002 |
| SC-SEL-02 | Invalid selector position | [`seq_sel_invalid_position.puml`](diagrams/seq_sel_invalid_position.puml) | alt: last valid position P / R / N (no limp-home, fault recovers after 500 ms) / last valid position D (limp-home stays latched) | SYS-SEL-002, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-CON-003, SG-03 |
| SC-SEL-03 | Shift lock (brake interlock) when leaving P | [`seq_sel_shift_lock.puml`](diagrams/seq_sel_shift_lock.puml) | alt: brake pressed and standstill (SLS = ON, lever leaves P) / brake released or moving (SLS = OFF, plant ignores the request) | SYS-SEL-003, SYS-FLT-001, SIM-PLT-005, SIM-CON-002, SG-03 |
| SC-SEL-04 | Reverse lockout and reverse line pressure | [`seq_sel_reverse_lockout.puml`](diagrams/seq_sel_reverse_lockout.puml) | alt: R at standstill (RLS = ON, PCS_Duty <= 29.4 %, RLS kept above 2 km/h) / R at >= 2 km/h (neutral; RLS = ON when the speed falls below 2 km/h) / speed invalid (RLS = OFF) | SYS-SEL-004, SYS-ACT-003, SYS-FLT-001, SYS-SEN-002, SIM-PLT-002, SIM-PLT-007, SG-01 |
| SC-SEL-05 | Solenoid outputs in P, R and N (prepared gear) | [`seq_sel_outputs_prn.puml`](diagrams/seq_sel_outputs_prn.puml) | alt: standstill (1st prepared) / rolling in N (schedule gear within 6000 rpm, follows directly); selecting D takes the prepared gear | SYS-SEL-005, SYS-SFT-005, SYS-SFT-006, SYS-ACT-001, SIM-PLT-002, SG-02, SG-03 |
| SC-SFT-01 | Drive engagement N / P -> D at standstill | [`seq_sft_engage_standstill.puml`](diagrams/seq_sft_engage_standstill.puml) | alt: from P (brake, SLS) / from N | SYS-SFT-006, SYS-ACT-001, SYS-SEL-001, SYS-SEL-005, SIM-PLT-002, SIM-CON-002 |
| SC-SFT-02 | Drive engagement N -> D while rolling | [`seq_sft_engage_rolling.puml`](diagrams/seq_sft_engage_rolling.puml) | - | SYS-SFT-006, SYS-SFT-005, SYS-SFT-001, SYS-SEL-005, SIM-PLT-002, SG-02 |
| SC-SFT-03 | Upshifts 1 -> 4 while accelerating | [`seq_sft_upshifts.puml`](diagrams/seq_sft_upshifts.puml) | opt: schedule asks for the next gear earlier than 2000 ms | SYS-SFT-001, SYS-SFT-003, SYS-SFT-004, SYS-SFT-007, SYS-ACT-001, SYS-ACT-002, SYS-SEN-005, SIM-PLT-003, SIM-PLT-006 |
| SC-SFT-04 | Downshifts with hysteresis and kick-down as single steps | [`seq_sft_downshifts_kickdown.puml`](diagrams/seq_sft_downshifts_kickdown.puml) | alt: coast-down with hysteresis (opt: speed oscillating) / kick-down 4 -> 3 -> 2 | SYS-SFT-001, SYS-SFT-002, SYS-SFT-003, SYS-SFT-004, SYS-SFT-005, SYS-SFT-007, SYS-ACT-001 |
| SC-SFT-05 | Downshift refused by the overspeed protection | [`seq_sft_overspeed_refused.puml`](diagrams/seq_sft_overspeed_refused.puml) | opt: frozen OutputShaftSpeed with valid InputShaftSpeed | SYS-SFT-005, SYS-SFT-003, SYS-SFT-001, SG-02 |
| SC-SFT-06 | Shift not complete within 800 ms: shift fault, limp-home | [`seq_sft_shift_timeout.puml`](diagrams/seq_sft_shift_timeout.puml) | opt: OutputShaftSpeed <= 200 rpm (shift complete, no fault) | SYS-SFT-007, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-CON-003, SIM-PLT-002, SG-04 |
| SC-SFT-07 | Cold oil: 4th gear inhibited | [`seq_sft_cold_oil.puml`](diagrams/seq_sft_cold_oil.puml) | start-up <= -5 degC, warm-up above -5 degC, inhibit below -10 degC in 4th; alt: 4 -> 3 / 4 -> 3 refused by SYS-SFT-005 | SYS-SFT-008, SYS-SFT-005, SYS-SFT-004, SYS-SEN-003, SIM-PLT-007, SIM-CON-003 |
| SC-FLT-01 | Limp-home entry (any fault of SYS-FLT-002) | [`seq_flt_limp_home_entry.puml`](diagrams/seq_flt_limp_home_entry.puml) | group: interlocks in limp-home; fault disappears; power cycle | SYS-FLT-001, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-HMI-001, SYS-SEL-003, SYS-SEL-004, SYS-SFT-004, SYS-SFT-005, SIM-PLT-002, SIM-PLT-007, SG-04 |
| SC-FLT-02 | Shaft speed above 8000 rpm: invalid, limp-home, reverse locked out | [`seq_flt_shaft_speed_invalid.puml`](diagrams/seq_flt_shaft_speed_invalid.puml) | opt: R selected with RLS = ON | SYS-SEN-002, SYS-SEN-001, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-SEL-004, SYS-HMI-001, SIM-CON-003, SG-01, SG-04 |
| SC-FLT-03 | Oil temperature sensor out of range: substitute 80 degC, mode Normal | [`seq_flt_oil_temp_sensor.puml`](diagrams/seq_flt_oil_temp_sensor.puml) | fault set after 100 ms, cleared after 100 ms in range | SYS-SEN-003, SYS-SEN-004, SYS-FLT-005, SYS-FLT-008, SYS-HMI-001, SIM-CON-003, SG-04 |
| SC-FLT-04 | Line pressure sensor out of range: limp-home | [`seq_flt_line_pressure_sensor.puml`](diagrams/seq_flt_line_pressure_sensor.puml) | - | SYS-SEN-004, SYS-SEN-006, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-CON-003, SG-04 |
| SC-FLT-05 | Gear ratio plausibility: ratio fault, limp-home | [`seq_flt_ratio_plausibility.puml`](diagrams/seq_flt_ratio_plausibility.puml) | alt: input shaft speed offset / solenoid stuck; opt: no check outside D, during a shift or <= 200 rpm | SYS-SEN-005, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-CON-003, SG-04 |
| SC-FLT-06 | Line pressure plausibility: pressure fault, limp-home | [`seq_flt_pressure_plausibility.puml`](diagrams/seq_flt_pressure_plausibility.puml) | - | SYS-SEN-006, SYS-SEN-003, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-PLT-007, SIM-CON-003, SG-04 |
| SC-FLT-07 | Oil overtemperature: fault lamp only | [`seq_flt_oil_overtemp.puml`](diagrams/seq_flt_oil_overtemp.puml) | alt: warm-up while driving / forced sensor voltage; cleared at <= 130 degC | SYS-FLT-006, SYS-FLT-008, SYS-HMI-001, SYS-SEN-003, SIM-PLT-007, SIM-CON-003 |
| SC-FLT-08 | Fault memory: read and clear DTCs | [`seq_flt_dtc_memory.puml`](diagrams/seq_flt_dtc_memory.puml) | read, clear with the fault present; opt: DTC setting off (`85 02`) and S3; power cycle | SYS-FLT-007, SYS-DIA-002, SYS-DIA-003, SYS-FLT-004, SYS-FLT-008, SIM-TST-003, SIM-CON-003 |
| SC-COM-01 | Reception timeout of SIM_Driver, SIM_Speed or SIM_Analog | [`seq_com_rx_timeout.puml`](diagrams/seq_com_rx_timeout.puml) | alt: `SIM_Driver` / `SIM_Speed` / `SIM_Analog` timeout | SYS-COM-002, SYS-SEL-002, SYS-SEL-004, SYS-SEN-006, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-005, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-CON-003, SG-04 |
| SC-COM-02 | Alive counter frozen or jumped | [`seq_com_alive_counter.puml`](diagrams/seq_com_alive_counter.puml) | alt: counter frozen (3 ignored -> timeout) / jumped by more than 2 / +2 accepted | SYS-COM-003, SYS-COM-002, SYS-FLT-002, SIM-PLT-001, SIM-CON-001, SIM-CON-003, SG-04 |
| SC-COM-03 | Communication control: stop and restart TCU transmission | [`seq_com_comm_control.puml`](diagrams/seq_com_comm_control.puml) | `28` in the default session refused; alt: restart with `28 00 01` / S3 timeout | SYS-COM-005, SYS-DIA-002, SYS-DIA-003, SYS-COM-004, SIM-PLT-004, SIM-TST-004 |
| SC-COM-04 | Bus-off of the TCU over the full CAN path | [`seq_com_bus_off.puml`](diagrams/seq_com_bus_off.puml) | alt: bus back within 30 ms / not within 30 ms (communication faults) with alt: back within 1000 ms / bus-off fault | SYS-COM-006, SYS-COM-002, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SYS-FLT-008, SYS-FLT-001, SYS-HMI-001, SIM-GW-001, SIM-GW-003, SIM-PLT-004, SG-04 |
| SC-DIA-01 | Diagnostic sessions, tester present and S3 timeout (application) | [`seq_dia_sessions.puml`](diagrams/seq_dia_sessions.puml) | `10 03`, `3E 00` / `3E 80`, `10 01`, S3 timeout with `28` and `85` active | SYS-DIA-001, SYS-DIA-002, SYS-DIA-003, SYS-COM-005, SYS-FLT-007, SIM-TST-003, SIM-TST-004 |
| SC-DIA-02 | Read identification DIDs in the application, PBL and SBL | [`seq_dia_read_dids.puml`](diagrams/seq_dia_read_dids.puml) | group: application / PBL / SBL | SYS-DIA-005, SYS-DIA-001, SYS-DIA-002, SIM-TST-003 |
| SC-DIA-03 | Security access level 0x01 and ECU reset (application) | [`seq_dia_security_reset.puml`](diagrams/seq_dia_security_reset.puml) | `27` in the default session refused; alt: correct key (opt: seed 0 when unlocked) / wrong key, lockout; `11 01` | SYS-DIA-002, SYS-STA-001, SYS-ACT-004, SYS-FLT-004, SYS-FLT-007, SIM-PLT-004 |
| SC-DIA-04 | Programming session refused by the application | [`seq_dia_programming_refused.puml`](diagrams/seq_dia_programming_refused.puml) | `10 02` from the default session (`7E`); alt: moving / not P / speed invalid (`22`) | SYS-DIA-004, SYS-DIA-002, SIM-TST-002, SIM-TST-004, SG-05 |
| SC-BOOT-01 | Nominal programming, steps 1 .. 19 | [`seq_boot_programming.puml`](diagrams/seq_boot_programming.puml) | three pages: application and hand-over (steps 1 .. 5), PBL (6 .. 11), SBL and restart (12 .. 19); `loop` for `36`; `7F 31 78` | SYS-BOOT-001, SYS-BOOT-004, SYS-BOOT-005, SYS-BOOT-006, SYS-BOOT-008, SYS-BOOT-010, SYS-BOOT-011, SYS-BOOT-015, SYS-BOOT-014, SYS-DIA-001, SYS-DIA-002, SYS-DIA-004, SYS-DIA-005, SYS-COM-005, SYS-STA-001, SIM-TST-001, SIM-TST-004, SIM-GW-002, SIM-PLT-004 |
| SC-BOOT-02 | Security access failures in the PBL | [`seq_boot_security_fail.puml`](diagrams/seq_boot_security_fail.puml) | download without security (`33`); wrong key (`35`), third failure (`36`), lockout 10 s (`37`) | SYS-BOOT-005, SYS-BOOT-009, SYS-DIA-001, SIM-TST-002 |
| SC-BOOT-03 | checkMemory or checkProgrammingDependencies failed | [`seq_boot_check_failed.puml`](diagrams/seq_boot_check_failed.puml) | alt: SBL CRC mismatch (opt: invalid SBL header) / application CRC mismatch / `FF01` failed; next reset ends in the PBL default session | SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-008, SYS-BOOT-013, SYS-BOOT-015, SIM-TST-001, SIM-TST-002, SIM-PLT-004, SG-06 |
| SC-BOOT-04 | Address and request sequence errors refused (PBL protection) | [`seq_boot_address_refused.puml`](diagrams/seq_boot_address_refused.puml) | group: PBL `34` outside the SBL load area / SBL `34` and eraseMemory outside the allowed areas / request sequence errors | SYS-BOOT-006, SYS-BOOT-004, SIM-TST-002, SG-06 |
| SC-BOOT-05 | Programming interrupted, then recovered | [`seq_boot_interrupted.puml`](diagrams/seq_boot_interrupted.puml) | alt: reset or power loss / tester stops; alt: interrupted in steps 5 .. 12 / after step 16 / in steps 13 .. 16 with recovery from the PBL default session | SYS-BOOT-007, SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-008, SYS-BOOT-009, SYS-BOOT-013, SYS-BOOT-015, SYS-DIA-005, SIM-TST-002, SIM-PLT-004, SG-06 |
| SC-BOOT-06 | Session timeout in the bootloader programming session | [`seq_boot_session_timeout.puml`](diagrams/seq_boot_session_timeout.puml) | alt: PBL / SBL programming session; alt: application valid / invalid | SYS-BOOT-009, SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-008, SIM-PLT-004 |
| SC-BOOT-07 | Recovery through the boot window | [`seq_boot_window_recovery.puml`](diagrams/seq_boot_window_recovery.puml) | alt: `11 01` / power cycle; opt: application resets in a loop | SYS-BOOT-011, SYS-BOOT-001, SYS-BOOT-005, SYS-BOOT-008, SIM-TST-002 |
| cmp_system_context | System context - physical view (elements E-01 .. E-15) | [`cmp_system_context.puml`](diagrams/cmp_system_context.puml) | physical view, E-01 .. E-15 | SYS-COM-001, SYS-HMI-001, SYS-HMI-002, SYS-BOOT-006, SIM-GW-001, SIM-GW-002, SIM-GW-003, SIM-CON-001, SIM-CON-002, SIM-CON-003, SIM-CON-004, SIM-TST-001, SIM-TST-004 |
| cmp_system_interfaces | System interfaces - logical view (TCU as one black box, I-01 .. I-12) | [`cmp_system_interfaces.puml`](diagrams/cmp_system_interfaces.puml) | logical view, I-01 .. I-12 | SYS-COM-001, SYS-COM-004, SYS-DIA-001, SYS-HMI-001, SYS-HMI-002, SIM-GW-001, SIM-CON-001, SIM-CON-002, SIM-TST-004 |
| cmp_tcu_software_parts | TCU software parts and memory regions (DC-04, reprogramming sections 1, 5 .. 7) | [`cmp_tcu_software_parts.puml`](diagrams/cmp_tcu_software_parts.puml) | software parts and memory regions, I-17a .. I-17c | SYS-BOOT-001, SYS-BOOT-004, SYS-BOOT-006, SYS-BOOT-013, SYS-BOOT-015 |
| stm_tcu_modes | TCU operating modes and running software part (system_context section 5) | [`stm_tcu_modes.puml`](diagrams/stm_tcu_modes.puml) | TCU modes and running part; index of the sequence diagrams | SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-007, SYS-BOOT-008, SYS-BOOT-009, SYS-BOOT-011, SYS-BOOT-013, SYS-BOOT-014, SYS-ACT-004, SYS-COM-002, SYS-COM-004, SYS-DIA-004, SYS-STA-001, SYS-STA-002, SYS-FLT-001, SYS-FLT-002, SYS-FLT-003, SYS-FLT-004, SIM-PLT-004 |

Notes:

- SC-BOOT-01 is drawn in **one file on three pages** (`newpage`): application and hand-over;
  PBL; SBL and restart. This replaces the split into two files that the plan (section 8) allowed.
  The catalogue keeps 39 sequence files, and each page stays readable.
- These variants were added to the task list in the plan:
  - SC-STA-02: `22 F1 81` -> `FF FF FF FF`;
  - SC-SEL-04: R held while the speed falls below 2 km/h; speed invalid;
  - SC-SFT-07: refused downshift 4 -> 3;
  - SC-BOOT-05: recovery from the PBL default session.
- Variants that follow from the spec text:
  - SC-COM-04 also shows the 30 ms reception timeouts during a bus-off (SYS-FLT-003, N-06).
  - SC-BOOT-05 separates the interruption before the erase, in steps 13 .. 18 (header sector still
    erased) and in step 19 (application accepted) (SYS-BOOT-007, SYS-BOOT-015, N-10).

## 9. Coverage

### 9.1 System requirements -> diagrams

Diagrams are listed by scenario ID; `cmp_*` and `stm_*` are the component and state diagrams.
*Primary proof* names the verification that the diagram does not replace.

| ID | Requirement | Diagrams | Primary proof / note |
|---|---|---|---|
| SYS-SEL-001 | Selector position | SC-STA-01, SC-SEL-01, SC-SFT-01 |  |
| SYS-SEL-002 | Invalid selector position | SC-SEL-02, SC-COM-01 |  |
| SYS-SEL-003 | Shift lock (brake interlock) | SC-SEL-03, SC-FLT-01 |  |
| SYS-SEL-004 | Reverse lockout | SC-SEL-04, SC-FLT-01, SC-FLT-02, SC-COM-01 |  |
| SYS-SEL-005 | Outputs in P, R and N | SC-SEL-05, SC-SFT-01, SC-SFT-02 |  |
| SYS-SFT-001 | Shift schedule | SC-SFT-02, SC-SFT-03, SC-SFT-04, SC-SFT-05 | table contents: UT |
| SYS-SFT-002 | Shift hysteresis | SC-SFT-04 | property of the calibration tables: RV of the tables, UT (diagram shows a note only) |
| SYS-SFT-003 | Single-step shifts | SC-SFT-03, SC-SFT-04, SC-SFT-05 |  |
| SYS-SFT-004 | Minimum time between shifts | SC-SFT-03, SC-SFT-04, SC-SFT-07, SC-FLT-01 |  |
| SYS-SFT-005 | Downshift overspeed protection | SC-SEL-05, SC-SFT-02, SC-SFT-04, SC-SFT-05, SC-SFT-07, SC-FLT-01 |  |
| SYS-SFT-006 | Drive engagement | SC-SEL-05, SC-SFT-01, SC-SFT-02 |  |
| SYS-SFT-007 | Shift completion monitoring | SC-SFT-03, SC-SFT-04, SC-SFT-06 |  |
| SYS-SFT-008 | Cold oil | SC-SFT-07 |  |
| SYS-ACT-001 | Solenoid pattern | SC-SEL-05, SC-SFT-01, SC-SFT-03, SC-SFT-04 |  |
| SYS-ACT-002 | Line pressure control | SC-SFT-03 | table contents: UT |
| SYS-ACT-003 | Line pressure for reverse | SC-SEL-04 |  |
| SYS-ACT-004 | Outputs before control is active | SC-STA-01, SC-STA-03, SC-DIA-03, stm_tcu_modes |  |
| SYS-SEN-001 | Vehicle speed | SC-STA-04, SC-FLT-02 | conversion accuracy: UT |
| SYS-SEN-002 | Shaft speeds | SC-SEL-04, SC-FLT-02 |  |
| SYS-SEN-003 | Analog sensor conversion | SC-SFT-07, SC-FLT-03, SC-FLT-06, SC-FLT-07 | conversion accuracy: UT |
| SYS-SEN-004 | Analog sensor range check | SC-FLT-03, SC-FLT-04 |  |
| SYS-SEN-005 | Gear ratio plausibility | SC-SFT-03, SC-FLT-05 |  |
| SYS-SEN-006 | Line pressure plausibility | SC-FLT-04, SC-FLT-06, SC-COM-01 |  |
| SYS-FLT-001 | Limp-home behaviour | SC-STA-01, SC-SEL-02, SC-SEL-03, SC-SEL-04, SC-SFT-06, SC-FLT-01, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-COM-01, SC-COM-04, stm_tcu_modes |  |
| SYS-FLT-002 | Limp-home entry | SC-STA-01, SC-SEL-02, SC-SFT-06, SC-FLT-01, SC-FLT-02, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-COM-01, SC-COM-02, SC-COM-04, stm_tcu_modes |  |
| SYS-FLT-003 | Fault reaction time | SC-SEL-02, SC-SFT-06, SC-FLT-01, SC-FLT-02, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-COM-01, SC-COM-04, stm_tcu_modes |  |
| SYS-FLT-004 | Limp-home latch | SC-STA-03, SC-SEL-02, SC-SFT-06, SC-FLT-01, SC-FLT-02, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-FLT-08, SC-COM-01, SC-COM-04, SC-DIA-03, stm_tcu_modes |  |
| SYS-FLT-005 | Substitute oil temperature | SC-FLT-03, SC-COM-01 |  |
| SYS-FLT-006 | Oil overtemperature | SC-FLT-07 |  |
| SYS-FLT-007 | Fault memory | SC-STA-03, SC-FLT-08, SC-DIA-01, SC-DIA-03 |  |
| SYS-FLT-008 | Fault recovery | SC-STA-03, SC-SEL-02, SC-SFT-06, SC-FLT-01, SC-FLT-02, SC-FLT-03, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-FLT-07, SC-FLT-08, SC-COM-01, SC-COM-04 |  |
| SYS-COM-001 | Bus and reception | SC-STA-04, cmp_system_context, cmp_system_interfaces |  |
| SYS-COM-002 | Reception timeout | SC-STA-01, SC-COM-01, SC-COM-02, SC-COM-04, stm_tcu_modes |  |
| SYS-COM-003 | Alive counter | SC-STA-04, SC-COM-02 |  |
| SYS-COM-004 | Transmission | SC-STA-01, SC-STA-04, SC-COM-03, cmp_system_interfaces, stm_tcu_modes |  |
| SYS-COM-005 | Communication control | SC-COM-03, SC-DIA-01, SC-BOOT-01 |  |
| SYS-COM-006 | Bus-off | SC-COM-04 |  |
| SYS-TIM-001 | Cycle times | SC-STA-01, SC-STA-04 | cycle configuration: AN, TT measurement |
| SYS-TIM-002 | CPU load | - | **no diagram**: CPU load and task deadlines are an internal runtime property with no externally visible interaction; verified by TT runtime measurement (timer or GPIO toggles) |
| SYS-STA-001 | Startup time | SC-STA-01, SC-STA-03, SC-DIA-03, SC-BOOT-01, stm_tcu_modes |  |
| SYS-STA-002 | Watchdog | SC-STA-03, stm_tcu_modes |  |
| SYS-HMI-001 | Fault lamp | SC-STA-01, SC-SEL-02, SC-SFT-06, SC-FLT-01, SC-FLT-02, SC-FLT-03, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-FLT-07, SC-COM-01, SC-COM-04, cmp_system_context, cmp_system_interfaces |  |
| SYS-HMI-002 | Debug output | SC-STA-04, cmp_system_context, cmp_system_interfaces |  |
| SYS-DIA-001 | UDS transport | SC-DIA-01, SC-DIA-02, SC-BOOT-01, SC-BOOT-02, cmp_system_interfaces |  |
| SYS-DIA-002 | Services in the application | SC-FLT-08, SC-COM-03, SC-DIA-01, SC-DIA-02, SC-DIA-03, SC-DIA-04, SC-BOOT-01 |  |
| SYS-DIA-003 | Session timeout | SC-FLT-08, SC-COM-03, SC-DIA-01 |  |
| SYS-DIA-004 | Programming preconditions | SC-DIA-04, SC-BOOT-01, stm_tcu_modes |  |
| SYS-DIA-005 | Identification data | SC-STA-02, SC-DIA-02, SC-BOOT-01, SC-BOOT-05 |  |
| SYS-BOOT-001 | Application start | SC-STA-01, SC-STA-02, SC-BOOT-01, SC-BOOT-03, SC-BOOT-05, SC-BOOT-06, SC-BOOT-07, cmp_tcu_software_parts, stm_tcu_modes |  |
| SYS-BOOT-002 | No valid application | SC-STA-02, SC-BOOT-03, SC-BOOT-05, SC-BOOT-06, stm_tcu_modes |  |
| SYS-BOOT-004 | Flash API usage | SC-BOOT-01, SC-BOOT-04, cmp_tcu_software_parts | RV of the PBL and application map files |
| SYS-BOOT-005 | Security access | SC-BOOT-01, SC-BOOT-02, SC-BOOT-07 |  |
| SYS-BOOT-006 | PBL protection | SC-BOOT-01, SC-BOOT-04, cmp_system_context, cmp_tcu_software_parts |  |
| SYS-BOOT-007 | Interrupted programming | SC-BOOT-05, stm_tcu_modes |  |
| SYS-BOOT-008 | Outputs in the bootloader | SC-STA-01, SC-STA-02, SC-BOOT-01, SC-BOOT-03, SC-BOOT-05, SC-BOOT-06, SC-BOOT-07, stm_tcu_modes |  |
| SYS-BOOT-009 | Bootloader session timeout | SC-BOOT-02, SC-BOOT-05, SC-BOOT-06, stm_tcu_modes |  |
| SYS-BOOT-010 | Programming time | SC-BOOT-01 | TT, measured by the tester |
| SYS-BOOT-011 | Boot window | SC-STA-01, SC-BOOT-01, SC-BOOT-07, stm_tcu_modes |  |
| SYS-BOOT-013 | Fault-free application check | SC-STA-01, SC-STA-02, SC-BOOT-03, SC-BOOT-05, cmp_tcu_software_parts, stm_tcu_modes | TT power loss during `36`; UT of the check logic |
| SYS-BOOT-014 | Programming entry from the application | SC-BOOT-01, stm_tcu_modes |  |
| SYS-BOOT-015 | Application header written last | SC-BOOT-01, SC-BOOT-03, SC-BOOT-05, cmp_tcu_software_parts |  |

### 9.2 Safety goals -> safe-state scenarios

| Goal | Safe state (safety_goals.md) | Scenario(s) ending in the safe state | Requirements (traceability.md) |
|---|---|---|---|
| SG-01 | neutral (reverse locked out) | SC-SEL-04 (R at >= 2 km/h; speed invalid), SC-FLT-02 (R with invalid speed) | SYS-SEL-004 |
| SG-02 | keep the current gear | SC-SFT-05 (2 -> 1 refused at 100 km/h); SC-SEL-05, SC-SFT-02 (no prepared gear above 6000 rpm) | SYS-SEL-005, SYS-SFT-003, SYS-SFT-005, SYS-SFT-006 |
| SG-03 | neutral | SC-SEL-02 (invalid selector treated as N), SC-SEL-03 (P kept without brake), SC-SEL-05 (no gear in P / N) | SYS-SEL-002, SYS-SEL-003, SYS-SEL-005 |
| SG-04 | limp-home (3rd in D, maximum line pressure, fault lamp ON) | SC-FLT-01 (generic), SC-FLT-02, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-SFT-06, SC-COM-01, SC-COM-02, SC-COM-04; warning: SC-FLT-03; hydraulic default after reset: SC-STA-03 | see traceability.md section 1 |
| SG-05 | reject the programming request | SC-DIA-04 (`7F 10 7E`, `7F 10 22`) | SYS-DIA-004, SYS-BOOT-014 |
| SG-06 | stay in the bootloader with outputs in the hydraulic default | SC-STA-02 (invalid application), SC-BOOT-03 (rejected image erased), SC-BOOT-05 (interrupted), SC-BOOT-04 (PBL protected) | SYS-BOOT-001, -002, -004 .. -007, -011, -013, -015 |

### 9.3 Simulation requirements -> diagrams

| ID | Requirement | Diagrams |
|---|---|---|
| SIM-PLT-001 | Cycle and transmission | SC-STA-01, SC-STA-04, SC-COM-02 |
| SIM-PLT-002 | Range and gear engagement | SC-SEL-01, SC-SEL-04, SC-SEL-05, SC-SFT-01, SC-SFT-02, SC-SFT-06, SC-FLT-01 |
| SIM-PLT-003 | Shift dynamics | SC-SFT-03 |
| SIM-PLT-004 | Hydraulic default on missing TCU | SC-STA-01, SC-STA-02, SC-STA-03, SC-COM-03, SC-COM-04, SC-DIA-03, SC-BOOT-01, SC-BOOT-03, SC-BOOT-05, SC-BOOT-06, stm_tcu_modes |
| SIM-PLT-005 | Shift lock | SC-SEL-03 |
| SIM-PLT-006 | Vehicle and engine model | SC-STA-04, SC-SFT-03 |
| SIM-PLT-007 | Line pressure and oil temperature | SC-SEL-04, SC-SFT-07, SC-FLT-01, SC-FLT-06, SC-FLT-07 |
| SIM-GW-001 | Transparent SLCAN bridge | SC-STA-04, SC-COM-04, cmp_system_context, cmp_system_interfaces |
| SIM-GW-002 | Throughput and latency | SC-STA-04, SC-BOOT-01, cmp_system_context |
| SIM-GW-003 | Error reporting | SC-COM-04, cmp_system_context |
| SIM-CON-001 | Display | SC-STA-04, SC-COM-02, cmp_system_context, cmp_system_interfaces |
| SIM-CON-002 | Driver inputs | SC-STA-04, SC-SEL-01, SC-SEL-03, SC-SFT-01, cmp_system_context, cmp_system_interfaces |
| SIM-CON-003 | Fault injection | SC-SEL-02, SC-SFT-06, SC-SFT-07, SC-FLT-02, SC-FLT-03, SC-FLT-04, SC-FLT-05, SC-FLT-06, SC-FLT-07, SC-FLT-08, SC-COM-01, SC-COM-02, cmp_system_context |
| SIM-CON-004 | Scenarios and logging | cmp_system_context |
| SIM-TST-001 | Flashing | SC-BOOT-01, SC-BOOT-03, cmp_system_context |
| SIM-TST-002 | Negative scenarios | SC-DIA-04, SC-BOOT-02, SC-BOOT-03, SC-BOOT-04, SC-BOOT-05, SC-BOOT-07 |
| SIM-TST-003 | Diagnostic services | SC-FLT-08, SC-DIA-01, SC-DIA-02 |
| SIM-TST-004 | Bus sharing | SC-COM-03, SC-DIA-01, SC-DIA-04, SC-BOOT-01, cmp_system_context, cmp_system_interfaces |

## 10. Open questions and findings

### 10.1 Spec findings F-01 .. F-18 (resolved)

The plan of TASK-026 found 18 spec findings. The user decided to fix the specs first. All of them
were resolved in the specs with commit `79929ee` (development log entry
"Spec findings from the system design (F-01 .. F-18, spec update)"). The diagrams draw the
updated spec text; no `ASSUMPTION` marker remains for them.

| # | Topic | Resolved in | Shown in |
|---|---|---|---|
| F-01 | `28` sub-function: stop with `28 01 01` (`68 01`), restart with `28 00 01` (`68 00`), reception continues | SYS-COM-005; reprogramming section 2 step 4 | SC-COM-03, SC-DIA-01, SC-BOOT-01 |
| F-02 | Shift and ratio monitoring only in D; in P / R / N the pattern follows directly | SYS-SEL-005, SYS-SFT-007, SYS-SEN-005, term "shift in progress" | SC-SEL-05, SC-SFT-03, SC-FLT-05 |
| F-03 | Shift complete at OutputShaftSpeed <= 200 rpm | SYS-SFT-007 | SC-SFT-06 |
| F-04 | Rejected images never start: start check with hw_id and min_pbl_version; the SBL erases the header sector after a failed `0202` / `FF01` | SYS-BOOT-001, new SYS-BOOT-015, reprogramming section 11 | SC-BOOT-03, SC-STA-02 |
| F-05 | Request sequence and address rules (NRC `24`, `31`, `73`, `22`) | reprogramming section 11.1, SYS-BOOT-006 | SC-BOOT-03, SC-BOOT-04, SC-BOOT-01 |
| F-06 | Services per software part and session; `10 01` in a bootloader programming session resets | reprogramming section 3 | SC-DIA-01, SC-DIA-03, SC-COM-03, SC-DIA-04, stm_tcu_modes |
| F-07 | Entry with an invalid application: `22 F1 81` -> `62 F1 81 FF FF FF FF`, then one `10 02` | reprogramming sections 2, 9 | SC-STA-02, SC-BOOT-05, stm_tcu_modes |
| F-08 | The application is silent between its `50 02` and the reset | SYS-BOOT-014 | SC-BOOT-01 |
| F-09 | Start-up values: selector N until accepted, substitute values until the first reception, control active within 500 ms also without plant | SYS-SEL-001, SYS-COM-002, SYS-STA-001 | SC-STA-01, SC-COM-01 |
| F-10 | Bus-off: red LED within 100 ms, limp-home outputs when the bus is back, recovery continues | SYS-FLT-003, SYS-COM-006 | SC-COM-04 |
| F-11 | Fault recovery; shaft speed invalid without debounce | new SYS-FLT-008, SYS-SEN-002 | SC-FLT-01 .. SC-FLT-07, SC-SEL-02, SC-COM-01, SC-COM-04 |
| F-12 | Fault bits per reception timeout (no bit 6 on `SIM_Speed`) | can_matrix section 6 | SC-COM-01 |
| F-13 | RLS stays ON only while R remains selected and the speed is valid | SYS-SEL-004 | SC-SEL-04, SC-FLT-02 |
| F-14 | `SIM_Analog` timeout enters limp-home (substitute 80 degC, pressure plausibility suspended, bit 9, latched) | SYS-FLT-002, can_matrix section 6 | SC-COM-01 |
| F-15 | No watchdog in the PBL / SBL in v1 | SYS-STA-002 note | SC-STA-03 |
| F-16 | Functional 0x7DF only for `3E 80` | can_matrix section 5 | SC-DIA-01, SC-BOOT-01 |
| F-17 | Lockout rule of security level 0x01 | reprogramming section 3 | SC-DIA-03 |
| F-18 | Mode "Bootloader"; no `TCU_Status` before control active | system_context section 5, SYS-COM-004 note | stm_tcu_modes, SC-STA-01 |

### 10.2 Spec findings N-01 .. N-11 (resolved)

These findings came up while drawing the diagrams (version 0.1 of this document). The user
decided them and the specs were updated with commit `0fcaba6` (development log entry
"Spec findings N-01 .. N-11 from the system design (spec update)"). The diagrams draw the
updated spec text.

| # | Topic | Decision | Resolved in | Shown in |
|---|---|---|---|---|
| N-01 | Fault lamp on a selector fault outside D | lamp and red LED ON while the selector fault is set | SYS-HMI-001 | SC-SEL-02 |
| N-02 | `TCU_Status.SelectorState` during the selector fault | 7 (invalid) | can_matrix section 3.5 | SC-SEL-02, SC-COM-01 |
| N-03 | `TCU_Status.OilTemp` while the substitute is used | 0xFF (invalid) | can_matrix section 3.5 | SC-FLT-03, SC-COM-01 |
| N-04 | Pressure plausibility with a line pressure sensor fault | suspended | SYS-SEN-006 | SC-FLT-04 |
| N-05 | Alive counter reference after an ignored message | last received message, ignored or not | SYS-COM-003 | SC-COM-02 |
| N-06 | Faults set during a bus-off | SYS-FLT-003 exception for every fault set in bus-off; outage > 30 ms enters limp-home through the communication faults | SYS-FLT-003 | SC-COM-04 |
| N-07 | Instant of the cold-oil start-up check | first oil temperature received after reset | SYS-SFT-008 | SC-SFT-07 |
| N-08 | `10 01` and active `28` / `85` | every return to the default session undoes them | SYS-DIA-003 | SC-DIA-01 |
| N-09 | Security lockout across a reset | not kept; accepted for v1 | reprogramming section 3 | SC-BOOT-02 |
| N-10 | Interrupted programming; unchecked image after a complete download | the SBL programs the application header sector last, after `0202` / `FF01` passed; SYS-BOOT-007 reworded | SYS-BOOT-007, SYS-BOOT-015, reprogramming sections 2, 11 | SC-BOOT-01, SC-BOOT-03, SC-BOOT-05, cmp_tcu_software_parts |
| N-11 | Gear at drive engagement when the schedule gear exceeds 6000 rpm | lowest higher gear within the limit | SYS-SFT-006, SYS-SEL-005 | SC-SFT-02, SC-SEL-05 |

### 10.3 Assumptions in the diagrams

None. Assumption A-01 of version 0.1 (cold-oil start-up check) is resolved by N-07. No step is
drawn without a spec basis.

### 10.4 Deviations from the approved plan

- SC-BOOT-01 is one file on three pages instead of two files (section 8).
- The TCU is also split into PBL / application in SC-STA-01 .. SC-STA-03, and into application /
  PBL / SBL in SC-DIA-02, because the running part is the subject of these scenarios.
- SYS-ACT-004 is allocated to the application only. From reset until the jump, SYS-BOOT-008
  (PBL / SBL) applies.
- SIM-CON-004 (scripted scenarios and logging) has no TCU interaction. It is shown in
  `cmp_system_context` (I-14) and is not drawn in a sequence diagram.
- New findings N-01 .. N-11 and assumption A-01 were found while drawing the diagrams and are
  resolved (sections 10.2, 10.3).
