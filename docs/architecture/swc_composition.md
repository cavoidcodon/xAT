# SWC composition

| Field | Value |
|---|---|
| Scope | All application SWCs in `src/app/`, their ports, the RTE mapping and the task schedule |
| Parent | `docs/specs/system/system_requirements.md` 0.4, `system_context.md` 0.2, `can_matrix.md` 1.3, `docs/architecture/system/system_design.md` 0.2 |
| Task | TASK-027 |
| Version | 0.1 |
| Status | Design (draft) |

Diagrams:

- [swc_composition.puml](swc_composition.puml): component diagram with two pages. Page 1
  shows the ports between SWCs. Page 2 shows the ports that the RTE maps to `Com`, `IoHwAb`
  and the BSW services.
- [swc_task_cycle.puml](swc_task_cycle.puml): one activation of the 10 ms task, with the
  preemption by the 1 ms task and the 100 ms activation.

This document is the detailed source. `overview.md` summarises it in its *Signal flow*
and *Scheduling* tables. The following tasks start from this document:

- TASK-017 `Rte`: ports, mapping and task bodies;
- TASK-018: one `swc-design <SwcName>` per SWC;
- TASK-009 OS / `SchM`: tasks, priorities and order;
- the BSW module designs: the design inputs in section 6.4.

AUTOSAR terms are taken from `AUTOSAR_SWS_RTE` R21-11 and `AUTOSAR_SWS_OS` R21-11
(`docs/references/autosar/`). They are used for naming and for the task model only. The RTE
is handwritten, so no requirement on an RTE generator applies.

## 1. SWCs and responsibilities

**Boundary between detection and decision (D4).**

- The SWC that owns a signal or a function detects the faults of that signal or function.
  It applies the debounce of the requirement. For the faults that clear by themselves
  (FaultBits 0, 5 and 11) it also applies the clear rule.
- It reports the current fault condition as a boolean.
- `SwcFaultMgr` only aggregates these conditions:
  - it latches the faults that enter limp-home (`SYS-FLT-004`, `SYS-FLT-008`);
  - it builds `FaultBits`;
  - it decides the operating mode;
  - it drives the fault lamp and the red LED;
  - it reports the faults to `Dem`.
- `SwcFaultMgr` does not evaluate any sensor value itself.

| SWC | Responsibility | Not its job |
|---|---|---|
| `SwcSensorInput` | Owns `SIM_Speed` and `SIM_Analog` (renamed from `SwcVehicleSpeed`, decision D1):<br>- shaft-speed range check and the shaft speed invalid fault (`SYS-SEN-002`);<br>- validated shaft speeds;<br>- vehicle speed and standstill (`SYS-SEN-001`);<br>- conversion of `OilTempVoltage` / `LinePressVoltage` and their range checks (`SYS-SEN-003`, `-004`);<br>- substitute oil temperature 80 degC and `TCU_Status.OilTemp` = 0xFF (`SYS-FLT-005`, can_matrix section 6);<br>- detection of the oil overtemperature (> 130 degC, cleared at <= 130 degC);<br>- communication-fault conditions of `SIM_Speed` and `SIM_Analog` | latching faults, mode, lamp; ratio and pressure plausibility (needs the commanded gear / PCS_Duty) |
| `SwcGearSelector` | Owns `SIM_Driver`.Selector and `.Brake`:<br>- 50 ms debounce, N before the first accepted position (`SYS-SEL-001`);<br>- invalid selector, selector fault and the condition "fault while D is selected" (`SYS-SEL-002`);<br>- shift lock SLS (`SYS-SEL-003`) and reverse lockout RLS (`SYS-SEL-004`);<br>- `TCU_Status.SelectorState`;<br>- programming preconditions mask (`SYS-DIA-004`);<br>- communication-fault condition of `SIM_Driver` | throttle, gear choice, limp-home decision |
| `SwcShiftCtrl` | - validation of `SIM_Driver.Throttle`;<br>- target gear: schedule, hysteresis tables, single-step shifts, 2000 ms minimum time, overspeed protection, drive engagement (`SYS-SFT-001` .. `-006`);<br>- prepared gear in P / R / N (`SYS-SEL-005`);<br>- shift completion monitoring and shift fault (`SYS-SFT-007`);<br>- cold-oil inhibit (`SYS-SFT-008`);<br>- ratio plausibility (`SYS-SEN-005`);<br>- `GearTarget`, `GearActual`, `ShiftInProgress`;<br>- GearTarget = 3 in limp-home | solenoid pattern, PCS_Duty, limp-home outputs |
| `SwcSolenoidCtrl` | - SS_A / SS_B from `GearTarget` (`SYS-ACT-001`);<br>- PCS_Duty table and shift pressure profile (`SYS-ACT-002`);<br>- reverse pressure (`SYS-ACT-003`);<br>- limp-home actuator outputs SS_A = SS_B = OFF, PCS_Duty = 0 % (`SYS-FLT-001`);<br>- line pressure plausibility and pressure fault (`SYS-SEN-006`) | gear choice, fault lamp |
| `SwcFaultMgr` (final name of the fault / mode manager) | - fault aggregation and latch (`SYS-FLT-004`, `-008`) and `FaultBits`;<br>- operating mode Init / Normal / Limp-home (`SYS-FLT-002`, rule D9 in section 5), `TCU_Status.Mode`;<br>- FaultLamp and red LED (`SYS-HMI-001`);<br>- bus-off fault after 1000 ms in bus-off (`SYS-COM-006`);<br>- Dem event reports (`SYS-FLT-007`, TASK-025);<br>- debug status line (`SYS-HMI-002`) | detecting sensor, selector, shift or pressure faults |

Runnables:

| SWC | Runnables (trigger) |
|---|---|
| `SwcSensorInput` | `SwcSensorInput_Init` (InitEvent), `SwcSensorInput_Run10ms` (TimingEvent 10 ms) |
| `SwcGearSelector` | `SwcGearSelector_Init`, `SwcGearSelector_Run10ms` |
| `SwcShiftCtrl` | `SwcShiftCtrl_Init`, `SwcShiftCtrl_Run10ms` |
| `SwcSolenoidCtrl` | `SwcSolenoidCtrl_Init`, `SwcSolenoidCtrl_Run10ms` |
| `SwcFaultMgr` | `SwcFaultMgr_Init`, `SwcFaultMgr_Run10ms`, `SwcFaultMgr_Run100ms` (TimingEvent 100 ms) |

System parameters (`system_context.md` 4.1) go in the `config/` of the SWC that uses them,
once each:

- `SwcShiftCtrl`: gear ratios.
- `SwcSensorInput`: final drive ratio and tyre radius.
- `SwcSolenoidCtrl`: PCS characteristic.

## 2. Allocation of system requirements

All 61 `Approved` SYS requirements of `system_requirements.md` 0.4 are listed once.
`SYS-BOOT-003` and `SYS-BOOT-012` are `Deleted` and are not allocated.

### 2.1 Application

33 requirements. Contributors are SWCs, or BSW modules marked "(BSW)".

| SYS | Requirement | Owning SWC | Contributing | Status of SYS |
|---|---|---|---|---|
| SYS-SEL-001 | Selector position | SwcGearSelector | - | Approved |
| SYS-SEL-002 | Invalid selector position | SwcGearSelector | SwcFaultMgr (bit 0, lamp; limp-home when in D) | Approved |
| SYS-SEL-003 | Shift lock (brake interlock) | SwcGearSelector | SwcSensorInput (Standstill) | Approved |
| SYS-SEL-004 | Reverse lockout | SwcGearSelector | SwcSensorInput (VehicleSpeed validity, Standstill) | Approved |
| SYS-SEL-005 | Outputs in P, R and N | SwcShiftCtrl (prepared gear) | SwcGearSelector (position), SwcSolenoidCtrl (pattern) | Approved |
| SYS-SFT-001 | Shift schedule | SwcShiftCtrl | SwcSensorInput (VehicleSpeed), SwcGearSelector (D) | Approved |
| SYS-SFT-002 | Shift hysteresis | SwcShiftCtrl (calibration tables) | - | Approved |
| SYS-SFT-003 | Single-step shifts | SwcShiftCtrl | - | Approved |
| SYS-SFT-004 | Minimum time between shifts | SwcShiftCtrl | - | Approved |
| SYS-SFT-005 | Downshift overspeed protection | SwcShiftCtrl | SwcSensorInput (shaft speeds) | Approved |
| SYS-SFT-006 | Drive engagement | SwcShiftCtrl | SwcSensorInput (Standstill), SwcGearSelector (D) | Approved |
| SYS-SFT-007 | Shift completion monitoring | SwcShiftCtrl | SwcFaultMgr (bit 1, limp-home) | Approved |
| SYS-SFT-008 | Cold oil | SwcShiftCtrl | SwcSensorInput (OilTempPhys, OilTempValid) | Approved |
| SYS-ACT-001 | Solenoid pattern | SwcSolenoidCtrl | SwcShiftCtrl (GearTarget) | Approved |
| SYS-ACT-002 | Line pressure control | SwcSolenoidCtrl | SwcShiftCtrl (GearTarget, ShiftInProgress, Throttle) | Approved |
| SYS-ACT-003 | Line pressure for reverse | SwcSolenoidCtrl | SwcGearSelector (R, RLS) | Approved |
| SYS-ACT-004 | Outputs before control is active | SwcFaultMgr (end of Init, rule D9) | BswM, Com (BSW: TCU_Tx I-PDU group stopped in Init) | Approved |
| SYS-SEN-001 | Vehicle speed | SwcSensorInput | - | Approved |
| SYS-SEN-002 | Shaft speeds | SwcSensorInput | SwcFaultMgr (bit 6, limp-home) | Approved |
| SYS-SEN-003 | Analog sensor conversion | SwcSensorInput | - | Approved |
| SYS-SEN-004 | Analog sensor range check | SwcSensorInput | SwcFaultMgr (bits 4, 5) | Approved |
| SYS-SEN-005 | Gear ratio plausibility | SwcShiftCtrl | SwcSensorInput (shaft speeds), SwcFaultMgr (bit 2) | Approved |
| SYS-SEN-006 | Line pressure plausibility | SwcSolenoidCtrl | SwcSensorInput (LinePress), SwcFaultMgr (bit 3; FaultBits 4 and 9 for the suspension) | Approved |
| SYS-FLT-001 | Limp-home behaviour | SwcSolenoidCtrl (SS_A, SS_B, PCS_Duty) | SwcFaultMgr (mode, FaultLamp), SwcGearSelector (interlocks stay active), SwcShiftCtrl (GearTarget 3) | Approved |
| SYS-FLT-002 | Limp-home entry | SwcFaultMgr | all detecting SWCs; BswM, CanSM (BSW: bus-off state) | Approved |
| SYS-FLT-003 | Fault reaction time | SwcFaultMgr | SwcSolenoidCtrl; Com, IoHwAb (BSW); task order, section 4.1 | Approved |
| SYS-FLT-004 | Limp-home latch | SwcFaultMgr | - | Approved |
| SYS-FLT-005 | Substitute oil temperature | SwcSensorInput | SwcFaultMgr (lamp, stays in Normal) | Approved |
| SYS-FLT-006 | Oil overtemperature | SwcFaultMgr (lamp, bit 11) | SwcSensorInput (detects > 130 degC / <= 130 degC) | Approved |
| SYS-FLT-008 | Fault recovery | SwcFaultMgr (latch rules) | SwcGearSelector (selector fault clear, 500 ms), SwcSensorInput (oil temperature sensor fault clear, 100 ms; overtemperature clear) | Approved |
| SYS-HMI-001 | Fault lamp | SwcFaultMgr | IoHwAb (BSW: red LED) | Approved |
| SYS-HMI-002 | Debug output | SwcFaultMgr (`SwcFaultMgr_Run100ms`) | DbgOut CDD (BSW, TASK-031) | Approved |
| SYS-DIA-004 | Programming preconditions | SwcGearSelector (`ProgPrecondMask`) | SwcSensorInput (VehicleSpeed, Standstill); Dcm (BSW: NRC `22`, RID 0x0203) | Approved |

### 2.2 Not application

28 requirements.

| SYS | Requirement | Owning module (BSW / boot / plant) | Reason | Application contribution |
|---|---|---|---|---|
| SYS-FLT-007 | Fault memory | Dem (TASK-025), Dcm (`14`, `19`, `85`) | DTC memory and services are BSW | SwcFaultMgr reports one event per FaultBits bit (D11) |
| SYS-COM-001 | Bus and reception | Can, CanIf, Com | bus parameters and Rx I-PDUs are configuration | - |
| SYS-COM-002 | Reception timeout | Com | deadline monitoring and substitute values are Com functions | input SWCs report the comm-fault conditions from the `Rte_Read` status (D5); SwcFaultMgr latches bits 7 .. 9 |
| SYS-COM-003 | Alive counter | Com | Rx and Tx I-PDU callouts; no SWC sees `AliveCtr` | - |
| SYS-COM-004 | Transmission | Com (periodic Tx modes), OS (10 ms task) | cycle time and tolerance come from Com and the task schedule | SwcFaultMgr decides control active (D9) |
| SYS-COM-005 | Communication control | Dcm, BswM, Com | `28` is a diagnostic service; BswM controls the TCU_Tx I-PDU group | - |
| SYS-COM-006 | Bus-off | CanSM (TASK-029) | bus-off recovery is CanSM's job | BswM forwards the state; SwcFaultMgr sets bit 10 after 1000 ms |
| SYS-TIM-001 | Cycle times | OS, Rte (TASK-009, TASK-017) | task periods and the runnable mapping of section 4 | every SWC runs its `Run10ms`; TCU_Status Tx period 100 ms (D6) |
| SYS-TIM-002 | CPU load | OS (TASK-009) | runtime property of the schedule | execution-time budgets of section 4.4 |
| SYS-STA-001 | Startup time | EcuM (TASK-010), PBL | start-up sequence | SwcFaultMgr ends Init within about 110 ms (D9) |
| SYS-STA-002 | Watchdog | Wdg / WdgM (TASK-019) | served as the last entry of the 10 ms task (D10) | - |
| SYS-DIA-001 | UDS transport | CanTp, Dcm; PBL, SBL | diagnostic stack | - |
| SYS-DIA-002 | Services in the application | Dcm (+ Dem for `14` / `19`) | diagnostic services | - |
| SYS-DIA-003 | Session timeout | Dcm (+ BswM for `28`, Dem for `85`) | session handling | - |
| SYS-DIA-005 | Identification data | Dcm; PBL, SBL | DID data come from the image header, boot info and MCU UID (Dcm configuration), not from SWCs | - |
| SYS-BOOT-001 | Application start | PBL | bootloader | - |
| SYS-BOOT-002 | No valid application | PBL | bootloader | - |
| SYS-BOOT-004 | Flash API usage | SBL (only caller) | the PBL and the application must not link the flash API (review of the map file) | no SWC uses flash |
| SYS-BOOT-005 | Security access | PBL, SBL | bootloader | - |
| SYS-BOOT-006 | PBL protection | SBL | bootloader | - |
| SYS-BOOT-007 | Interrupted programming | PBL, SBL | bootloader | - |
| SYS-BOOT-008 | Outputs in the bootloader | PBL, SBL | bootloader | - |
| SYS-BOOT-009 | Bootloader session timeout | PBL, SBL | bootloader | - |
| SYS-BOOT-010 | Programming time | PBL, SBL, UDS tester | bootloader and tester | application steps 1 .. 5 are Dcm |
| SYS-BOOT-011 | Boot window | PBL | bootloader | - |
| SYS-BOOT-013 | Fault-free application check | PBL | bootloader | - |
| SYS-BOOT-014 | Programming entry from the application | Dcm, EcuM / Mcu (reset) | `50 02`, silence and reset are diagnostic and BSW functions | SwcGearSelector provides the precondition mask (`SYS-DIA-004`) |
| SYS-BOOT-015 | Application header written last | SBL | bootloader | - |

## 3. Ports

### 3.1 Conventions

- **Names.** Ports are named `Pp<Name>` (provide) and `Rp<Name>` (require); this is a
  project convention, the RTE SWS only uses the port short name. An R-port connected to an
  SWC P-port has the same `<Name>` (`PpVehicleSpeed` -> `RpVehicleSpeed`). API names follow
  RTE SWS 5.2.6.4: `Rte_Read_<p>_<o>`, `Rte_Write_<p>_<o>`, `Rte_Call_<p>_<o>`,
  `Rte_Switch_<p>_<o>`, `Rte_Mode_<p>_<o>`, `Rte_IrvRead_<re>_<o>`, `Rte_IrvWrite_<re>_<o>`.
- **CAN data.** A data element mapped to a CAN signal keeps the `can_matrix.md` signal name
  and raw scaling (DC-03).
  - Conversion to physical units happens only in the SWC named in the port table (DC-02),
    never in the RTE.
  - Ports to sensors and actuators are named `*In` / `*Out`, independent of the signal source
    (DC-01).
- **Access.** Every S/R access is explicit (`Rte_Read` / `Rte_Write`, RTE SWS 4.3.1.5).
  All SWC runnables run one after the other in `OsTask_10ms`, so explicit access is
  consistent without copies (RTE SWS 4.2.6, sequential scheduling; decision D7).
- **Reception status (D5).** `Rte_Read` of a `*In` port returns the status of its `Com`
  signal:
  - `RTE_E_OK`;
  - `RTE_E_NEVER_RECEIVED`: before the first reception ([SWS_Rte_07643]);
  - `RTE_E_MAX_AGE_EXCEEDED`: after the Com reception timeout ([SWS_Rte_02703]).

  In both error cases the value is the Com init / substitute value. The SWC that owns the
  message turns the status into its comm-fault condition and applies the physical substitutes
  of `can_matrix.md` section 6.
- **Direct writes (D6).** Each `TCU_Actuators` / `TCU_Status` signal is written by the SWC that
  computes it, every 10 ms. `Com` transmits `TCU_Actuators` every 10 ms and `TCU_Status` every
  100 ms with the latest values. The 100 ms transmission period is the "status reporting with a
  100 ms cycle" of `SYS-TIM-001`.
- **Mode ports (D8).** `TcuMode` and `CanBusState` are AUTOSAR mode switch interfaces (RTE
  SWS 4.4). The handwritten RTE simplifies them:
  - no mode-switch-triggered runnables and no mode disabling;
  - queue length 1 ([SWS_Rte_02718]);
  - the transition completes inside `Rte_Switch`, so `Rte_Mode` returns the new mode from the
    next call on.
- **Period.** "10 ms" means written by the producer in every activation of its 10 ms runnable
  and read by every consumer in every activation of its 10 ms runnable.

### 3.2 Port table

Producer / consumer of a C/S port = client / server. `FALSE` = 0 = OFF.

| Port | Interface (S/R, C/S, mode) | Data element | Type | Unit / scaling | Range | Init value | Producer | Consumer(s) | Period | Notes |
|---|---|---|---|---|---|---|---|---|---|---|
| RpDriverIn | S/R | Selector | uint8 | enum 0 = P, 1 = R, 2 = N, 3 = D | 0 .. 3 valid, 4 .. 7 invalid | 7 | Com `SIM_Driver` | SwcGearSelector | 10 ms | init = substitute "invalid" (can_matrix section 6); debounced in SwcGearSelector |
| RpDriverIn | S/R | Brake | boolean | 0 = released, 1 = pressed | 0 .. 1 | FALSE | Com `SIM_Driver` | SwcGearSelector | 10 ms | substitute released |
| RpThrottleIn | S/R | Throttle | uint8 | 0.5 %/bit | 0 .. 200 valid, 201 .. 255 invalid | 0 | Com `SIM_Driver` | SwcShiftCtrl | 10 ms | substitute 0 %; validated in SwcShiftCtrl (G-02) |
| RpShaftSpeedIn | S/R | InputShaftSpeed | uint16 | 1 rpm/bit | 0 .. 8000 valid, > 8000 invalid | 0xFFFF | Com `SIM_Speed` | SwcSensorInput | 10 ms | init = substitute "invalid" (A-01) |
| RpShaftSpeedIn | S/R | OutputShaftSpeed | uint16 | 1 rpm/bit | 0 .. 8000 valid, > 8000 invalid | 0xFFFF | Com `SIM_Speed` | SwcSensorInput | 10 ms | as above |
| RpAnalogIn | S/R | OilTempVoltage | uint16 | 1 mV/bit | 0 .. 5000 valid, > 5000 invalid | 0xFFFF | Com `SIM_Analog` | SwcSensorInput | 10 ms | converted with can_matrix section 4 in SwcSensorInput; A-01 |
| RpAnalogIn | S/R | LinePressVoltage | uint16 | 1 mV/bit | 0 .. 5000 valid, > 5000 invalid | 0xFFFF | Com `SIM_Analog` | SwcSensorInput | 10 ms | as above |
| PpSolenoidOut | S/R | SS_A | boolean | 0 = OFF, 1 = ON | 0 .. 1 | FALSE | SwcSolenoidCtrl | Com `TCU_Actuators` | 10 ms | OFF = hydraulic default |
| PpSolenoidOut | S/R | SS_B | boolean | 0 = OFF, 1 = ON | 0 .. 1 | FALSE | SwcSolenoidCtrl | Com `TCU_Actuators` | 10 ms | |
| PpSolenoidOut | S/R | PCS_Duty | uint16 | 0.1 %/bit, 0 % = maximum line pressure | 0 .. 1000 | 0 | SwcSolenoidCtrl | Com `TCU_Actuators` | 10 ms | limp-home 0 % |
| PpLockSolenoidOut | S/R | RLS | boolean | 0 = OFF (locked), 1 = ON (reverse allowed) | 0 .. 1 | FALSE | SwcGearSelector | Com `TCU_Actuators`, SwcSolenoidCtrl | 10 ms | SwcSolenoidCtrl uses it for `SYS-ACT-003` |
| PpLockSolenoidOut | S/R | SLS | boolean | 0 = OFF (locked in P), 1 = ON | 0 .. 1 | FALSE | SwcGearSelector | Com `TCU_Actuators` | 10 ms | |
| PpFaultLampOut | S/R | FaultLamp | boolean | 0 = OFF, 1 = ON | 0 .. 1 | FALSE | SwcFaultMgr | Com `TCU_Actuators`, IoHwAb `FaultLampLed` | 10 ms | one write, two targets in the RTE: the LED has the same value (`SYS-HMI-001`) and does not depend on CAN |
| PpGearStatus | S/R | GearTarget | uint8 | enum of can_matrix 3.5 (gear of the commanded solenoid pattern) | 1 .. 4 | 3 | SwcShiftCtrl | Com `TCU_Status`, SwcSolenoidCtrl, SwcFaultMgr | 10 ms | 3 in limp-home; G-03 |
| PpGearStatus | S/R | GearActual | uint8 | 0 = none / N, 1 .. 4, 5 = R, 15 = unknown | 0 .. 5, 15 | 15 | SwcShiftCtrl | Com `TCU_Status`, SwcFaultMgr | 10 ms | G-03 |
| PpGearStatus | S/R | ShiftInProgress | boolean | 0 / 1 | 0 .. 1 | FALSE | SwcShiftCtrl | Com `TCU_Status`, SwcSolenoidCtrl | 10 ms | only in D (term "shift in progress") |
| PpSelectorState | S/R | SelectorState | uint8 | enum as `SIM_Driver.Selector`; 7 = invalid | 0 .. 3, 7 | 2 | SwcGearSelector | Com `TCU_Status`, SwcFaultMgr | 10 ms | 7 while the selector fault is set |
| PpVehicleSpeed | S/R | VehicleSpeed | uint16 | 0.1 km/h/bit | 0 .. 2500, 0xFFFF = invalid | 0xFFFF | SwcSensorInput | Com `TCU_Status`, SwcGearSelector, SwcShiftCtrl, SwcFaultMgr | 10 ms | from OutputShaftSpeed (`SYS-SEN-001`); G-05 |
| PpOilTempStatus | S/R | OilTemp | uint8 | 1 degC/bit, offset -40 | 0 .. 190, 0xFF = invalid | 0xFF | SwcSensorInput | Com `TCU_Status` | 10 ms | 0xFF while the substitute is used |
| PpModeStatus | S/R | Mode | uint8 | 0 = Normal, 1 = Limp-home | 0 .. 1 | 0 | SwcFaultMgr | Com `TCU_Status` | 10 ms | not transmitted in Init |
| PpFaultBits | S/R | FaultBits | uint16 | bit field of can_matrix 3.6 | 0x0000 .. 0x0FFF | 0x0000 | SwcFaultMgr | Com `TCU_Status`, SwcSolenoidCtrl | 10 ms | latched per `SYS-FLT-008`; SwcSolenoidCtrl suspends `SYS-SEN-006` on bits 4, 9 |
| PpStandstill | S/R | Standstill | boolean | TRUE = vehicle speed valid and < 2 km/h | 0 .. 1 | FALSE | SwcSensorInput | SwcGearSelector, SwcShiftCtrl | 10 ms | one place for the 2 km/h threshold |
| PpShaftSpeeds | S/R | InputShaftSpeed | uint16 | 1 rpm/bit | 0 .. 8000, 0xFFFF = invalid | 0xFFFF | SwcSensorInput | SwcShiftCtrl | 10 ms | validated (`SYS-SEN-002`); invalid also on `SIM_Speed` timeout |
| PpShaftSpeeds | S/R | OutputShaftSpeed | uint16 | 1 rpm/bit | 0 .. 8000, 0xFFFF = invalid | 0xFFFF | SwcSensorInput | SwcShiftCtrl | 10 ms | as above |
| PpOilTemp | S/R | OilTempPhys | sint16 | 0.1 degC/bit | -400 .. 1500 | 800 | SwcSensorInput | SwcShiftCtrl | 10 ms | 800 = substitute 80 degC (`SYS-FLT-005`, can_matrix section 6) |
| PpOilTemp | S/R | OilTempValid | boolean | TRUE = measured value, FALSE = substitute | 0 .. 1 | FALSE | SwcSensorInput | SwcShiftCtrl | 10 ms | first TRUE = start-up instant of `SYS-SFT-008` |
| PpLinePress | S/R | LinePress | uint16 | 0.01 bar/bit | 0 .. 2500 | 0 | SwcSensorInput | SwcSolenoidCtrl | 10 ms | |
| PpLinePress | S/R | LinePressValid | boolean | TRUE = `SIM_Analog` received and voltage in range | 0 .. 1 | FALSE | SwcSensorInput | SwcSolenoidCtrl | 10 ms | |
| PpSensorFaults | S/R | ShaftSpeedInvalid | boolean | condition of FaultBits bit 6 | 0 .. 1 | FALSE | SwcSensorInput | SwcFaultMgr | 10 ms | no debounce; not set on `SIM_Speed` timeout |
| PpSensorFaults | S/R | LinePressSensorFault | boolean | condition of bit 4 | 0 .. 1 | FALSE | SwcSensorInput | SwcFaultMgr | 10 ms | > 100 ms outside 200 .. 4800 mV |
| PpSensorFaults | S/R | OilTempSensorFault | boolean | state of bit 5 | 0 .. 1 | FALSE | SwcSensorInput | SwcFaultMgr | 10 ms | set after 100 ms, cleared after 100 ms in range |
| PpSensorFaults | S/R | OilOverTemp | boolean | state of bit 11 | 0 .. 1 | FALSE | SwcSensorInput | SwcFaultMgr | 10 ms | > 130 degC; cleared at <= 130 degC |
| PpSensorFaults | S/R | ComFaultSimSpeed | boolean | condition of bit 8 | 0 .. 1 | FALSE | SwcSensorInput | SwcFaultMgr | 10 ms | `Rte_Read` status `RTE_E_MAX_AGE_EXCEEDED` |
| PpSensorFaults | S/R | ComFaultSimAnalog | boolean | condition of bit 9 | 0 .. 1 | FALSE | SwcSensorInput | SwcFaultMgr | 10 ms | as above |
| PpSelectorPosition | S/R | SelectorPosition | uint8 | enum 0 = P, 1 = R, 2 = N, 3 = D | 0 .. 3 | 2 | SwcGearSelector | SwcShiftCtrl, SwcSolenoidCtrl | 10 ms | N before the first accepted position and while the selector fault is set |
| PpSelectorFaults | S/R | SelectorFault | boolean | state of bit 0 | 0 .. 1 | FALSE | SwcGearSelector | SwcFaultMgr | 10 ms | set after 100 ms invalid, cleared after 500 ms valid |
| PpSelectorFaults | S/R | SelectorFaultInD | boolean | TRUE = selector fault set and last valid position before it was D | 0 .. 1 | FALSE | SwcGearSelector | SwcFaultMgr | 10 ms | limp-home condition of `SYS-FLT-002` |
| PpSelectorFaults | S/R | SelectorKnown | boolean | TRUE = a position has been accepted or the selector fault has been set since reset | 0 .. 1 | FALSE | SwcGearSelector | SwcFaultMgr | 10 ms | end of Init (D9) |
| PpSelectorFaults | S/R | ComFaultSimDriver | boolean | condition of bit 7 | 0 .. 1 | FALSE | SwcGearSelector | SwcFaultMgr | 10 ms | `Rte_Read` status of `RpDriverIn.Selector` |
| PpThrottle | S/R | Throttle | uint8 | 0.5 %/bit | 0 .. 200 | 0 | SwcShiftCtrl | SwcSolenoidCtrl | 10 ms | validated throttle, one conversion (DC-02) |
| PpShiftFaults | S/R | ShiftFault | boolean | condition of bit 1 | 0 .. 1 | FALSE | SwcShiftCtrl | SwcFaultMgr | 10 ms | shift not complete within 800 ms |
| PpShiftFaults | S/R | RatioFault | boolean | condition of bit 2 | 0 .. 1 | FALSE | SwcShiftCtrl | SwcFaultMgr | 10 ms | > 10 % for > 500 ms |
| PpSolenoidFaults | S/R | PressureFault | boolean | condition of bit 3 | 0 .. 1 | FALSE | SwcSolenoidCtrl | SwcFaultMgr | 10 ms | > 3 bar for > 500 ms; read in the next cycle (section 4) |
| PpTcuMode | mode | TcuMode | mode declaration group `TcuMode` | `TCUMODE_INIT`, `TCUMODE_NORMAL`, `TCUMODE_LIMPHOME` | 3 modes | `TCUMODE_INIT` | SwcFaultMgr | SwcShiftCtrl, SwcSolenoidCtrl, BswM | switched on change, read every 10 ms | `Rte_Switch` / `Rte_Mode` (D8) |
| PpProgPrecond | S/R | ProgPrecondMask | uint8 | bit 0 not at standstill, bit 1 not in P, bit 2 speed invalid (RID 0x0203 result encoding) | 0x00 .. 0x07 | 0x07 | SwcGearSelector | Dcm | 10 ms | Dcm: `10 02` -> NRC `22` if != 0; `31 01 02 03` -> result = mask |
| RpCanBusState | mode | CanBusState | mode declaration group `CanBusState` | `CANBUS_ONLINE`, `CANBUS_BUSOFF` | 2 modes | `CANBUS_ONLINE` | BswM (from CanSM) | SwcFaultMgr | switched on change, read every 10 ms | bus-off fault = BUSOFF for 1000 ms without interruption |
| RpDebugOut | C/S | WriteLine (IN Line uint8[64], IN Length uint8; returns Std_ReturnType) | - | ASCII status line | Length 0 .. 64 | - | SwcFaultMgr | DbgOut CDD (TASK-031) | 500 ms | `E_NOT_OK` when busy; the line is dropped |
| RpDemEvt&lt;Fault&gt; | C/S | SetEventStatus (IN EventStatus `Dem_EventStatusType`; returns Std_ReturnType) | - | event status | `DEM_EVENT_STATUS_PASSED`, `DEM_EVENT_STATUS_FAILED` | - | SwcFaultMgr | Dem (TASK-025) | 10 ms | 12 ports, one per FaultBits bit (section 3.4); FAILED while the bit is set, else PASSED (D11) |

Inter-runnable variables of `SwcFaultMgr` (explicit, written by `SwcFaultMgr_Run10ms`, read
by `SwcFaultMgr_Run100ms`):

| IRV | Type | Init | Purpose |
|---|---|---|---|
| IrvTcuMode | uint8 (`TcuMode` value) | `TCUMODE_INIT` | mode for the debug line |
| IrvFaultBits | uint16 | 0x0000 | fault bits for the debug line |

`AliveCtr` of every message is handled inside `Com` (I-PDU callouts) and is not a port.

### 3.3 RTE mapping

Today the RTE maps every port in this table. A later hardware variant (DC-01) changes only
the targets in the last column. The SWCs stay unchanged, and the `IoHwAb` signals keep the
scaling of the Com signal they replace.

| Port.element | Target today | Message (ID) / signal or service | Hardware variant (DC-01) |
|---|---|---|---|
| RpDriverIn.Selector | Com, `Com_ReceiveSignal` + status | `SIM_Driver` (0x100) / Selector | IoHwAb selector inputs |
| RpDriverIn.Brake | Com | `SIM_Driver` (0x100) / Brake | IoHwAb brake switch |
| RpThrottleIn.Throttle | Com | `SIM_Driver` (0x100) / Throttle | IoHwAb pedal sensor |
| RpShaftSpeedIn.InputShaftSpeed | Com | `SIM_Speed` (0x101) / InputShaftSpeed | IoHwAb speed sensor |
| RpShaftSpeedIn.OutputShaftSpeed | Com | `SIM_Speed` (0x101) / OutputShaftSpeed | IoHwAb speed sensor |
| RpAnalogIn.OilTempVoltage | Com | `SIM_Analog` (0x102) / OilTempVoltage | IoHwAb analog input |
| RpAnalogIn.LinePressVoltage | Com | `SIM_Analog` (0x102) / LinePressVoltage | IoHwAb analog input |
| PpSolenoidOut.SS_A | Com, `Com_SendSignal` | `TCU_Actuators` (0x200) / SS_A | IoHwAb digital output |
| PpSolenoidOut.SS_B | Com | `TCU_Actuators` (0x200) / SS_B | IoHwAb digital output |
| PpSolenoidOut.PCS_Duty | Com | `TCU_Actuators` (0x200) / PCS_Duty | IoHwAb PWM output |
| PpLockSolenoidOut.RLS | Com + RTE buffer | `TCU_Actuators` (0x200) / RLS | IoHwAb digital output |
| PpLockSolenoidOut.SLS | Com | `TCU_Actuators` (0x200) / SLS | IoHwAb digital output |
| PpFaultLampOut.FaultLamp | Com + IoHwAb | `TCU_Actuators` (0x200) / FaultLamp; IoHwAb `FaultLampLed` | IoHwAb lamp output + `FaultLampLed` |
| PpGearStatus.GearTarget | Com + RTE buffer | `TCU_Status` (0x201) / GearTarget | unchanged |
| PpGearStatus.GearActual | Com + RTE buffer | `TCU_Status` (0x201) / GearActual | unchanged |
| PpGearStatus.ShiftInProgress | Com + RTE buffer | `TCU_Status` (0x201) / ShiftInProgress | unchanged |
| PpSelectorState.SelectorState | Com + RTE buffer | `TCU_Status` (0x201) / SelectorState | unchanged |
| PpVehicleSpeed.VehicleSpeed | Com + RTE buffer | `TCU_Status` (0x201) / VehicleSpeed | unchanged |
| PpOilTempStatus.OilTemp | Com | `TCU_Status` (0x201) / OilTemp | unchanged |
| PpModeStatus.Mode | Com | `TCU_Status` (0x201) / Mode | unchanged |
| PpFaultBits.FaultBits | Com + RTE buffer | `TCU_Status` (0x201) / FaultBits | unchanged |
| PpProgPrecond.ProgPrecondMask | RTE buffer, read by Dcm through its R-port (`Rte_Dcm.h`) | Dcm: `10 02`, RID 0x0203 | unchanged |
| PpTcuMode | RTE mode machine; users read with `Rte_Mode` (BswM through `Rte_BswM.h`) | BswM: TCU_Tx I-PDU group | unchanged |
| RpCanBusState | RTE mode machine; BswM switches with `Rte_Switch` (`Rte_BswM.h`) | BswM <- `BswM_CanSM_CurrentState` | unchanged |
| RpDebugOut.WriteLine | direct call of the CDD function | DbgOut CDD (TASK-031) | unchanged |
| RpDemEvt&lt;Fault&gt;.SetEventStatus | `Dem_SetEventStatus(<EventId>, <status>)` | Dem (TASK-025) | unchanged |
| all other ports (section 3.2, producer and consumer are SWCs) | RTE buffer (copy) | - | unchanged |

The reception status of the `*In` ports comes from the Com notifications to the RTE:

- the reception notification (`Rte_COMCbk`);
- the reception timeout notification (`Rte_COMCbkRxTOut`, RTE SWS 5.9.1).

The RTE keeps a never-received flag and a timeout flag per signal. Copying a status is not
control logic.

### 3.4 Interfaces that are not `Com` signals

These rows resolve the six needs listed in section 5.2 of TASK-027.

| Need | Requirement | Port | Mapping |
|---|---|---|---|
| Red LED ON in limp-home / bus-off | `SYS-HMI-001`, `SYS-FLT-003` | `SwcFaultMgr.PpFaultLampOut.FaultLamp` | RTE -> IoHwAb `FaultLampLed` -> Dio channel of the red LED (section 3.5). Synchronous and independent of CAN, so it also works in bus-off |
| Debug status line every 500 ms | `SYS-HMI-002` | `SwcFaultMgr.RpDebugOut.WriteLine`, called by `SwcFaultMgr_Run100ms` in every 5th activation | DbgOut CDD (new TASK-031; LPUART over the MCU-Link VCOM) |
| Programming preconditions for `10 02` | `SYS-DIA-004`, SG-05 | `SwcGearSelector.PpProgPrecond.ProgPrecondMask` (S/R) | Dcm R-port. One value serves `10 02` (NRC `22` if != 0) and RID 0x0203 (result = mask), so both always agree |
| Faults for the fault memory / DTCs | `SYS-FLT-007`, `SYS-DIA-002` | `SwcFaultMgr.RpDemEvt<Fault>` (12 C/S ports, below) | `Dem_SetEventStatus` (TASK-025). Until then the faults are visible in `TCU_Status.FaultBits` only |
| Communication state | `SYS-COM-002`, `-003`, `-005`, `-006` | Rx timeout: `Rte_Read` status of the `*In` ports (section 3.1). Alive counter: no port (Com Rx I-PDU callout; an ignored frame is no reception, so 3 ignored frames end in the timeout). `28`: no SWC port (Dcm -> BswM; the application does not need it). Bus-off: `SwcFaultMgr.RpCanBusState` (mode) | Com; BswM; CanSM (TASK-029) -> `BswM_CanSM_CurrentState` -> BswM (TASK-030) -> `Rte_Switch` |
| Watchdog supervision of the 10 ms cycle | `SYS-STA-002` | none (D10) | WdgM_MainFunction or the Wdg trigger is the last entry of `OsTask_10ms` (TASK-019) |

Dem event ports of `SwcFaultMgr` (one per `FaultBits` bit, DTCs of can_matrix 3.6):

| Port | Bit | DTC |
|---|---|---|
| RpDemEvtSelectorFault | 0 | P0705-29 |
| RpDemEvtShiftFault | 1 | P0780-00 |
| RpDemEvtRatioFault | 2 | P0730-00 |
| RpDemEvtPressureFault | 3 | P0840-64 |
| RpDemEvtLinePressSensorFault | 4 | P0840-1C |
| RpDemEvtOilTempSensorFault | 5 | P0710-1C |
| RpDemEvtShaftSpeedInvalid | 6 | P0715-29 |
| RpDemEvtComFaultSimDriver | 7 | U1100-87 |
| RpDemEvtComFaultSimSpeed | 8 | U1101-87 |
| RpDemEvtComFaultSimAnalog | 9 | U1102-87 |
| RpDemEvtBusOff | 10 | U0001-88 |
| RpDemEvtOilOverTemp | 11 | P0218-00 |

### 3.5 `IoHwAb` signals (input for TASK-028)

| Signal | Direction | Type | Init / safe value | Underlying `Dio` channel | Notes |
|---|---|---|---|---|---|
| `FaultLampLed` | output (digital) | boolean, TRUE = LED ON | OFF, set by `IoHwAb_Init` before `Rte_Start`; OFF until `SwcFaultMgr` writes ON | red LED, `GPIO0` pin 10 (P0_10), active low: TRUE -> `STD_LOW`; symbolic channel `DioConf_DioChannel_LED_RED` (name fixed in TASK-007) | written by the RTE in the same `Rte_Write` as `TCU_Actuators.FaultLamp`; the SWC never sees the pin or the polarity |

Rule for the later hardware variant: an `IoHwAb` signal that replaces a `Com` signal has the
type, scaling and invalid values of that `Com` signal.

## 4. Scheduling

**Task model.** OSEK/VDX OS terms are used as the AUTOSAR OS SWS 7.1 takes them over:

- basic tasks with static priorities and full-preemptive scheduling;
- conformance class BCC1 (one activation per task, no events, no `WaitEvent`);
- `TerminateTask` at the end of each task;
- activation by cyclic alarms on a 1 ms counter, or by one repeating schedule table
  (OS SWS 7.3); TASK-009 chooses.

AUTOSAR OS scalability class SC1 (single core, no protection; OS SWS 7.11). No hook is
required; an ErrorHook to report OS errors is optional (TASK-009). Inside a task the RTE
calls the runnables in the configured position (`RtePositionInTask`, [SWS_Rte_80076]). All
tasks are released at the same tick every 10 ms / 100 ms, and the priorities give the order.

| OS task | Period | Priority | Runnables and main functions in execution order |
|---|---|---|---|
| `OsTask_1ms` | 1 ms | 3 (highest) | `Can_MainFunction_BusOff`, `Can_MainFunction_Read`, `Can_MainFunction_Write`, `Can_MainFunction_Mode` (polling mode; if TASK-012 uses interrupts these move to the ISRs) |
| `OsTask_10ms` | 10 ms | 2 | 1 `Com_MainFunctionRx`<br>2 `CanSM_MainFunction`<br>3 `SwcSensorInput_Run10ms`<br>4 `SwcGearSelector_Run10ms`<br>5 `SwcShiftCtrl_Run10ms`<br>6 `SwcFaultMgr_Run10ms`<br>7 `SwcSolenoidCtrl_Run10ms`<br>8 `BswM_MainFunction`<br>9 `Com_MainFunctionTx`<br>10 `CanTp_MainFunction`<br>11 `Dcm_MainFunction`<br>12 `Dem_MainFunction` (from TASK-025)<br>13 watchdog service: `WdgM_MainFunction` or the `Wdg` trigger (TASK-019), always last |
| `OsTask_100ms` | 100 ms | 1 | `SwcFaultMgr_Run100ms` (debug line in every 5th activation) |
| initialisation (EcuM, TASK-010) | once | - | BSW init, then `Rte_Start`, which calls `SwcSensorInput_Init`, `SwcGearSelector_Init`, `SwcShiftCtrl_Init`, `SwcFaultMgr_Init`, `SwcSolenoidCtrl_Init`; periodic activation starts after `Rte_Start` returns (section 4.2) |

Every runnable is in exactly one task. Why this order:

- **Producer before consumer.** The order is sensors -> selector -> shift control -> fault
  decision -> actuators, so every same-cycle flow goes producer -> consumer:
  - VehicleSpeed, Standstill and shaft speeds go to SwcGearSelector and SwcShiftCtrl;
  - SelectorPosition and RLS go to SwcShiftCtrl and SwcSolenoidCtrl;
  - GearTarget, ShiftInProgress and Throttle go to SwcSolenoidCtrl;
  - all fault conditions except PressureFault go to SwcFaultMgr;
  - TcuMode and FaultBits go to SwcSolenoidCtrl.
- **Limp-home outputs in the cycle of the fault.** `SwcFaultMgr` runs before
  `SwcSolenoidCtrl`.
- **Flows that come from the previous cycle.** Two flows go against the order:
  - `SwcShiftCtrl` reads `TcuMode` written in the previous cycle. This is harmless:
    `SwcSolenoidCtrl` applies the limp-home outputs on its own, and GearTarget = 3 follows
    one cycle later.
  - `PressureFault` reaches `SwcFaultMgr` in the next cycle (latency in section 4.1).
- **BswM after the runnables, before `Com_MainFunctionTx`.** The first `TCU_Actuators` frame
  is transmitted in the same cycle in which `SwcFaultMgr` ends Init. Its values were computed
  in that cycle (`SYS-ACT-004`).
- **`Com_MainFunctionRx` and `CanSM_MainFunction` first.** Reception status, substitutes and
  the bus-off state are current for the runnables.
- **`Com_MainFunctionTx` before CanTp, Dcm and Dem.** The variable diagnostic load stays out
  of the `TCU_Actuators` jitter (section 4.4).
- **Watchdog last.** It is served only when the whole 10 ms task has run (D10).

### 4.1 End-to-end latency

Worst case, with the execution-time budgets of section 4.4.

- "Bus" means the bus access of the `TCU_Actuators` frame. A classic frame with 8 data bytes
  takes about 270 us at 500 kbit/s.
- Before the frame there can be one frame in progress and the three higher-priority frames
  0x100 .. 0x102, so the access takes up to about 1.4 ms.

| Path | From | To | Worst case | Requirement |
|---|---|---|---|---|
| Fault reaction | fault set in `SwcSensorInput`, `SwcGearSelector`, `SwcShiftCtrl` or by a Com timeout (`Com_MainFunctionRx`), cycle k | limp-home `TCU_Actuators` on the bus | same cycle: <= 1.0 ms to `Com_MainFunctionTx` + 1.4 ms bus ~ 2.4 ms | SYS-FLT-003 (100 ms) |
| Fault reaction, pressure fault | `PressureFault` set in `SwcSolenoidCtrl`, cycle k | limp-home `TCU_Actuators` on the bus | one cycle more: 10 + 2.4 ~ 12.4 ms | SYS-FLT-003 (100 ms) |
| Fault reaction in bus-off | communication faults (30 .. 40 ms after the last frame) or bus-off fault (1000 ms in `CANBUS_BUSOFF`), set in cycle k | red LED ON | same `SwcFaultMgr_Run10ms`: < 1 ms (IoHwAb write is synchronous) | SYS-FLT-003 (LED 100 ms) |
| Limp-home outputs after bus-off | bus available again (CanSM reports full communication) | limp-home `TCU_Actuators` on the bus | next `Com_MainFunctionTx`: <= 10 ms + 1.4 ms | SYS-FLT-003 ("as soon as") |
| Shift command | new `GearTarget` in `SwcShiftCtrl`, cycle k | `TCU_Actuators` SS_A / SS_B / PCS_Duty | same cycle ~ 2.4 ms | - (SYS-SFT-007 monitors from the gear change) |
| Selector -> actuator | new `SIM_Driver.Selector` value received by the TCU | `TCU_Actuators` of the accepted position | 1 ms (Can polling) + 10 ms (deferred Com Rx) + 50 ms (debounce) + 2.4 ms ~ 63 ms | SYS-SEL-001 (filter >= 50 ms; no maximum) |
| Brake / speed -> SLS, RLS | `SIM_Driver` / `SIM_Speed` received | `TCU_Actuators` SLS / RLS | 1 + 10 + 2.4 ~ 13 ms | - |
| Sensor timeout -> substitute value | last frame of `SIM_Driver`, `SIM_Speed` or `SIM_Analog` | substitute values and comm-fault condition in the SWCs; limp-home outputs | 30 .. 40 ms (deadline monitoring in the 10 ms `Com_MainFunctionRx`, A-04); outputs in the same cycle | SYS-COM-002 (30 ms) |
| Watchdog | last completed 10 ms task | MCU reset | <= watchdog timeout (TASK-019; must be <= 100 ms, proposal 50 ms) | SYS-STA-002 (100 ms) |
| Start-up | reset | control active | PBL check + 50 ms boot window + BSW init + Init (<= about 110 ms, D9) + 10 ms: estimate <= 270 ms | SYS-STA-001 (500 ms) |

All paths with a requirement meet it. The fault reaction worst case is about 12.4 ms against
100 ms.

### 4.2 Initialisation

- **EcuM init (TASK-010):**
  - `IoHwAb_Init` switches the LED OFF;
  - `Com_Init`; the TCU_Rx I-PDU group (`SIM_Driver`, `SIM_Speed`, `SIM_Analog`) is started
    before the first `Com_MainFunctionRx`, so deadline monitoring runs from the application
    start (`SYS-COM-002`);
  - the TCU_Tx group (`TCU_Actuators`, `TCU_Status`) stays stopped (`SYS-ACT-004`);
  - CanSM and BswM init.
- **`Rte_Start`** calls the five init runnables in the order of `OsTask_10ms`.
  - [SWS_Rte_06761]: InitEvent runnables run once at `Rte_Start`.
  - [SWS_Rte_06748] does not guarantee that they run before the periodic runnables. Here they
    do: they complete inside `Rte_Start`, and the periodic activation is released only after
    `Rte_Start` returns ([SWS_Rte_07575]).
- **Init values.** Every port has its init value from section 3.2 before the first periodic
  runnable runs.

### 4.3 Data consistency

- **SWC ports.** Every SWC port is written and read inside `OsTask_10ms`, and no runnable is
  preempted by another runnable, so the scheduling is sequential (RTE SWS 4.2.6). No exclusive
  area and no copy buffers are needed in the RTE.
- **`SwcFaultMgr_Run100ms`.** It is preempted by `OsTask_10ms`. It reads only elements of
  <= 32 bit and two explicit IRVs. Each access is atomic on the Cortex-M33. A line that mixes
  two consecutive 10 ms cycles is acceptable for the debug output.
- **BSW exclusive areas.** `OsTask_1ms` (Rx indication, Tx confirmation, bus-off indication)
  preempts the 10 ms main functions. The design input for `Com`, `CanTp`, `CanIf` and `CanSM`
  is an exclusive area (`SchM_Enter_<Ma>` / `SchM_Exit_<Ma>`) around their data shared
  between the two tasks.

### 4.4 Execution-time budget and jitter

Budgets (assumption A-03, measured in the target tests):

| Task | Budget (WCET per activation) | Load |
|---|---|---|
| `OsTask_1ms` | <= 0.1 ms | <= 10 % |
| `OsTask_10ms` | <= 2.0 ms, of which entries 1 .. 8 (up to `Com_MainFunctionTx`) <= 0.8 ms | <= 20 % |
| `OsTask_100ms` | <= 2.0 ms | <= 2 % |
| total | | <= 32 % (`SYS-TIM-002`: < 50 %) |

- **Tx jitter.** `TCU_Actuators` must be sent every 10 ms +/- 1 ms (`SYS-COM-004`). Its jitter
  is the variation of the start of `Com_MainFunctionTx` after the task release. That
  variation is at most the execution time of entries 1 .. 8 (<= 0.8 ms) plus the `OsTask_1ms`
  preemptions (<= 0.1 ms), so it stays below 1 ms.
- **Diagnostic load.** CanTp, Dcm and Dem run after the Tx main function and do not affect
  the jitter.

## 5. Operating modes

`SwcFaultMgr` is the only mode manager of `TcuMode`. The mode is published two ways:

- through `PpTcuMode` to `SwcShiftCtrl`, `SwcSolenoidCtrl` and BswM;
- as the `TCU_Status.Mode` signal through `PpModeStatus`.

| Mode | Decided by | Signalled through | Reaction per SWC |
|---|---|---|---|
| Init | SwcFaultMgr: initial mode after `Rte_Start` | `TcuMode` = `TCUMODE_INIT`; BswM keeps TCU_Tx stopped, so no `TCU_Actuators` / `TCU_Status` is sent (`SYS-ACT-004`, `SYS-COM-004` note) | All SWCs compute their outputs as in Normal, so the first transmitted values are computed values. SwcFaultMgr evaluates rule D9 in every cycle. The red LED follows the computed FaultLamp |
| Normal | SwcFaultMgr: leaves Init by rule D9 (b) | `TCUMODE_NORMAL`; BswM starts TCU_Tx (unless `28 01 01` is active); `Mode` = 0 | All functions |
| Limp-home | SwcFaultMgr: a fault of `SYS-FLT-002` is set, also from Init (rule D9 (a)); latched until reset (`SYS-FLT-004`) | `TCUMODE_LIMPHOME`; `Mode` = 1; FaultLamp ON | SwcSolenoidCtrl: SS_A = SS_B = OFF, PCS_Duty = 0 %, not delayed by `SYS-SFT-004` / `-005` (`SYS-FLT-001`).<br>SwcShiftCtrl: GearTarget = 3, no shift, ShiftInProgress = 0.<br>SwcGearSelector: unchanged; SLS / RLS interlocks stay active.<br>SwcSensorInput: unchanged.<br>SwcFaultMgr: FaultLamp ON |
| Bootloader | not application (PBL / SBL) | - | application not running |

**Rule D9 (end of Init).** Init ends in the first `SwcFaultMgr_Run10ms` in which either
condition holds:

- **(a)** A fault of `SYS-FLT-002` is set. The new mode is Limp-home, so control becomes
  active with the limp-home outputs.
  - Example: no plant messages. The communication faults are set after 30 .. 40 ms
    (`SYS-COM-002` note, `SYS-STA-001` note).
- **(b)** `SelectorKnown` is TRUE and `VehicleSpeed` is valid. The new mode is Normal.

This matches both alternatives of SC-STA-01. Init lasts at most about 110 ms after
`Rte_Start`:

- a missing message is a communication fault after <= 40 ms;
- an invalid selector is the selector fault after 100 ms;
- an invalid speed is always bit 6 or bit 8.

D9 also guarantees that the vehicle speed is valid while the mode is Normal. An invalid speed
always sets bit 6 (shaft speed invalid) or bit 8 (`SIM_Speed` timeout), and that is
limp-home in the same cycle.

**Latch and fault bits.**

- *Latched until the next power cycle:* the conditions of bits 1, 2, 3, 4, 6, 7, 8, 9 and 10.
  `SwcFaultMgr` latches them into `FaultBits` (`SYS-FLT-008`).
- *Passed through without a latch:* bits 0, 5 and 11. Their detecting SWC already applies the
  clear rule.
- *Limp-home condition:* any latched limp-home bit OR `SelectorFaultInD`. Bit 0 alone (P, R,
  N) is not a limp-home condition.
- *`Mode`:* 0 in Normal, 1 in Limp-home. In Init nothing is transmitted.

**Fault lamp.** FaultLamp = Limp-home OR bit 0 OR bit 5 OR bit 11 (`SYS-HMI-001`,
`SYS-FLT-005`, `SYS-FLT-006`). The same value is written to `TCU_Actuators.FaultLamp` and to
the red LED.

**Bus-off.**

- CanSM reports `CANSM_BSWM_BUS_OFF` / `CANSM_BSWM_FULL_COMMUNICATION` to BswM. BswM switches
  `CanBusState`.
- `SwcFaultMgr` sets the bus-off fault (bit 10) when `CanBusState` has been `CANBUS_BUSOFF`
  for 1000 ms without interruption (`SYS-COM-006`).
- During the bus-off the communication faults are set first (30 .. 40 ms), so limp-home and
  the red LED come from them (`SYS-FLT-003` note).

## 6. Open questions and assumptions

### 6.1 Decisions

| # | Decision | By |
|---|---|---|
| D1 | `SwcVehicleSpeed` owns `SIM_Speed` and `SIM_Analog` and is renamed `SwcSensorInput` | user, 2026-10-21 |
| D2 | Minimal `CanSM` (TASK-029, bus-off recovery) and `BswM` (TASK-030, TCU_Tx = control active AND not disabled by `28`; bus-off state to the application) | user, 2026-10-21 |
| D3 | `SYS-HMI-002`: port and 100 ms runnable now; debug UART CDD as TASK-031 | user, 2026-10-21 |
| D4 | Detecting SWC reports the fault condition; `SwcFaultMgr` only aggregates, latches and decides (section 1) | plan, approved 2026-10-21 |
| D5 | One SWC per received message evaluates its `Rte_Read` status and reports the comm fault | plan |
| D6 | `TCU_*` signals written directly by their producers; TCU_Status reported at the 100 ms Com period | plan |
| D7 | Explicit S/R everywhere | plan |
| D8 | `TcuMode` / `CanBusState` as mode switch ports with the RTE simplification of section 3.1 | plan |
| D9 | End of Init (section 5) | plan |
| D10 | No watchdog port; watchdog served last in `OsTask_10ms` | plan |
| D11 | `SwcFaultMgr` reports the 12 Dem events from the latched fault bits | plan |
| - | Fault / mode manager SWC, BSW order as design input, `swc_task_cycle.puml`, `IoHwAb` for the red LED, backlog dependencies | user, 2026-10-18 (TASK-027 open questions 1 .. 5) |

### 6.2 Findings in the specs, the system design and the task

The specs and the system design are not changed by this task. Each finding has a proposal for
the user.

| # | Finding | Proposal |
|---|---|---|
| G-01 | TASK-027 counted 62 `Approved` SYS requirements. There are 61 (63 defined, `SYS-BOOT-003` and `-012` `Deleted`), as `system_design.md` section 2 says. | Count corrected in TASK-027.md. |
| G-02 | `SIM_Driver.Throttle` 201 .. 255 is "invalid" (can_matrix 3.1), but no SYS requirement defines the TCU reaction. | Treat an invalid throttle as 0 % (the timeout substitute of can_matrix section 6), no fault bit (none is reserved). Either a derived SWR in `SwcShiftCtrl` (TASK-018) or a sentence in `SYS-COM-002` / can_matrix section 6. |
| G-03 | `TCU_Status.GearActual` ("engaged gear as seen by the TCU") and `GearTarget` ("commanded gear") are not defined for P / R / N, during a shift, in limp-home, or for the value 15 (unknown). | GearTarget = gear of the commanded solenoid pattern, 1 .. 4 (3 in limp-home, as SC-FLT-01). GearActual: 0 in P and N; 5 in R while RLS = ON, else 0; in D the commanded gear, or the previous gear while a shift is in progress; 15 while the shaft speeds are invalid. Fixed as SWRs of `SwcShiftCtrl` or in can_matrix 3.5. |
| G-04 | `SYS-FLT-003` has an exception for bus-off but none for a transmission stopped by `28 01 01` (`SYS-COM-005`). While Tx is stopped, the limp-home outputs cannot be transmitted within 100 ms. The plant applies the hydraulic default after 50 ms, which is the same state. | Add the bus-off exception for `28` as well: red LED within 100 ms, limp-home outputs as soon as the transmission is restarted (`28 00 01`, `10 01`, S3). The design already behaves like this. |
| G-05 | OutputShaftSpeed up to 8000 rpm (`SYS-SEN-002`) gives about 290 km/h with final drive 3.23 and radius 0.31 m. That is above the range 0 .. 250 km/h of `SYS-SEN-001` and `TCU_Status.VehicleSpeed` (0 .. 2500). | Clamp VehicleSpeed at 2500 (250.0 km/h) and keep it valid. Derived SWR in `SwcSensorInput`, or a range note in `SYS-SEN-001`. |
| G-06 | `layering.md` forbade BSW code to include RTE headers. Service modules with AUTOSAR service ports need their own `Rte_<Ma>.h`: Dcm reads `ProgPrecondMask`, BswM uses `Rte_Mode` / `Rte_Switch`, and Dem will have ports later. | Approved with the plan: `layering.md` now allows a BSW service module to include its own `Rte_<Ma>.h` (`BswM`, `Dcm`, `Dem`). |
| G-07 | `system_design.md` section 1 names TASK-018 as the task of the SWC composition (now TASK-027). SC-STA-01 draws "P accepted; control active" without a requirement that ties them together; rule D9 makes it the design rule. | Update `system_design.md` section 1 at its next revision. |
| G-08 | `SYS-SEN-006` compares with "the pressure expected for the commanded PCS_Duty" but names no characteristic. | Use the static PCS characteristic of `SYS-ACT-003` / `SIM-PLT-007` (0 % -> 20 bar, 100 % -> 3 bar, linear); assumption A-02. |

### 6.3 Assumptions

| # | Assumption | Check |
|---|---|---|
| A-01 | The Com init / substitute raw values of `InputShaftSpeed`, `OutputShaftSpeed`, `OilTempVoltage` and `LinePressVoltage` are 0xFFFF ("invalid"). can_matrix section 6 states the physical substitutes only, and the SWCs apply them from the `Rte_Read` status, not from the raw value. | Com design (TASK-015) |
| A-02 | Expected line pressure for `SYS-SEN-006` from the static PCS characteristic (G-08). | SWC design `SwcSolenoidCtrl` |
| A-03 | Execution-time budgets of section 4.4. | target test runtime measurement (`SYS-TIM-002`), `candump -t d` (`SYS-COM-004`) |
| A-04 | Com deadline monitoring runs in the 10 ms `Com_MainFunctionRx`, so a timeout is detected 30 .. 40 ms after the last accepted frame. The alive-counter rule "3 consecutive ignored messages are treated as a timeout" (`SYS-COM-003`) works because an ignored frame does not restart the deadline monitoring. At the 10 ms frame cycle this is the same timeout. | Com design (TASK-015), target test SC-COM-02 |

### 6.4 Design inputs to the BSW, RTE and OS tasks

The order of section 4 and the items below are design inputs. Each `module-design` confirms
them or reports a conflict, and this composition is then updated.

| Module (task) | Design input |
|---|---|
| OS / SchM (TASK-009) | <ul><li>Tasks of section 4:<ul><li>basic tasks, BCC1, full-preemptive, SC1;</li><li>priorities 3 / 2 / 1;</li><li>released together every 10 ms / 100 ms;</li><li>activation by alarms or a schedule table on a 1 ms counter.</li></ul></li><li>Exclusive areas for the BSW data shared between `OsTask_1ms` and `OsTask_10ms`.</li></ul> |
| EcuM (TASK-010) | <ul><li>Init order:<ol><li>`IoHwAb_Init` (LED OFF);</li><li>`Com_Init` with TCU_Rx started and TCU_Tx stopped;</li><li>CanSM / BswM init;</li><li>`Rte_Start` with the five init runnables;</li><li>start of the periodic activation.</li></ol></li><li>Control active within 500 ms after reset (section 4.1).</li></ul> |
| Can (TASK-012) | <ul><li>Polling main functions in `OsTask_1ms`, or interrupts.</li><li>On bus-off, pending Tx requests are aborted, so no stale `TCU_Actuators` frame is sent after the recovery.</li></ul> |
| CanIf (TASK-013) | <ul><li>`CanIf_ControllerBusOff` -> CanSM.</li><li>Tx buffers cleared when the controller is stopped.</li></ul> |
| Com (TASK-015) | <ul><li>Signals and scaling exactly as can_matrix.</li><li>Init values = substitute values (Selector 7, Brake 0, Throttle 0, A-01).</li><li>Reception timeout:<ul><li>`ComFirstTimeout` = `ComTimeout` = 30 ms on every `SIM_*` signal;</li><li>timeout action REPLACE;</li><li>reception and timeout notifications to the RTE.</li></ul></li><li>`SIM_*` I-PDUs with deferred signal processing.</li><li>I-PDU callouts:<ul><li>Rx: alive-counter check; a rejected frame does not restart the deadline monitoring (A-04);</li><li>Tx: writes `AliveCtr`.</li></ul></li><li>I-PDU groups:<ul><li>TCU_Rx, started at init;</li><li>TCU_Tx, stopped at init, controlled by BswM.</li></ul></li><li>`TCU_Actuators` periodic 10 ms; `TCU_Status` periodic 100 ms, first transmission when the group starts.</li></ul> |
| CanSM (TASK-029) | <ul><li>Bus-off recovery, repeated while the bus is not back.</li><li>`BswM_CanSM_CurrentState`:<ul><li>`CANSM_BSWM_BUS_OFF` at bus-off;</li><li>`CANSM_BSWM_FULL_COMMUNICATION` only after a confirmed transmission.</li></ul></li><li>`CanSM_MainFunction` before the runnables.</li><li>No ComM: the full-communication request at start-up comes from BswM or EcuM.</li></ul> |
| BswM (TASK-030) | <ul><li>Rule: TCU_Tx started if and only if `TcuMode` != `TCUMODE_INIT` AND the Dcm communication mode enables Tx (`BswM_Dcm_CommunicationMode_CurrentState`, immediate processing).</li><li>`BswM_CanSM_CurrentState` -> `Rte_Switch` of `CanBusState` (immediate).</li><li>Start-up actions: TCU_Rx start, CanSM full communication.</li><li>`BswM_MainFunction` after the runnables, before `Com_MainFunctionTx`.</li></ul> |
| CanTp (TASK-020) | <ul><li>`CanTp_MainFunction` in `OsTask_10ms` after `Com_MainFunctionTx`.</li></ul> |
| Dcm (TASK-021) | <ul><li>Reads `ProgPrecondMask` through its R-port:<ul><li>`10 02` -> NRC `22` if != 0;</li><li>RID 0x0203 -> result = mask.</li></ul></li><li>`28` and every return to the default session (`10 01`, S3) -> BswM.</li><li>After the transmit confirmation of `50 02`: reset within 20 ms, no response meanwhile (`SYS-BOOT-014`).</li><li>DID data from Dcm configuration callouts, no SWC port.</li></ul> |
| Wdg / WdgM (TASK-019) | <ul><li>Served only as the last entry of `OsTask_10ms`.</li><li>Timeout <= 100 ms (`SYS-STA-002`).</li><li>No SWC port.</li></ul> |
| Dem (TASK-025) | <ul><li>12 events of section 3.4, monitor-internal debouncing.</li><li>`Dem_MainFunction` after `Dcm_MainFunction`.</li></ul> |
| IoHwAb (TASK-028) | <ul><li>Signal `FaultLampLed` of section 3.5.</li></ul> |
| Rte (TASK-017) | <ul><li>Task bodies of section 4.</li><li>Init runnables inside `Rte_Start`.</li><li>Component-specific functions with the short API names as macros in `Rte_<Swc>.h`.</li><li>Reception status per `*In` signal from the Com notifications.</li><li>Mode simplification D8.</li><li>FaultLamp fan-out to Com and IoHwAb.</li><li>No exclusive areas (section 4.3).</li><li>Every port listed in `overview.md` (`app_rules.md`).</li></ul> |
| DbgOut CDD (TASK-031) | <ul><li>Server of `RpDebugOut.WriteLine`:<ul><li>non-blocking;</li><li>returns `E_NOT_OK` when a line is still being sent.</li></ul></li><li>LPUART over the MCU-Link VCOM.</li><li>Location in `src/bsw/` decided in TASK-031.</li></ul> |

### 6.5 Open questions

None open. G-02 .. G-05, G-07 and G-08 above wait for the user's decision. Until then the
proposals are the working assumptions of TASK-018. They do not block TASK-017 or TASK-009,
because they do not change any port, type or task.
