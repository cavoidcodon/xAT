# xAT architecture overview

Simplified automatic transmission control unit (TCU) on FRDM-MCXN947, AUTOSAR Classic
R21-11, single core.

The required behaviour, the system context (TCU, CAN gateway, plant model on the host) and
the CAN matrix are specified in [`docs/specs/`](../specs/README.md).

System design (elements, interfaces, requirement allocation, scenario diagrams):
[`system/system_design.md`](system/system_design.md).

SWC composition (SWCs, requirement allocation, ports, RTE mapping, task schedule):
[`swc_composition.md`](swc_composition.md).

## Layers

```
+--------------------------------------------------------------------------+
| Application (src/app)  SwcSensorInput SwcGearSelector SwcShiftCtrl       |
|                        SwcFaultMgr    SwcSolenoidCtrl                    |
+--------------------------------------------------------------------------+
| RTE (src/rte)  handwritten, no control logic;                            |
|                ports -> Com, IoHwAb, BSW services                        |
+--------------------------------------------------------------------------+
| Services: Det EcuM SchM BswM Dcm Dem | Com stack: Com PduR CanIf CanSM   |
| ECU abstraction: IoHwAb              |            CanTp                  |
| CDD: DbgOut (debug UART)             |                              | OS |
+--------------------------------------------------------------------------+
| MCAL (src/mcal): Mcu Port Dio Gpt Can Wdg                                |
+--------------------------------------------------------------------------+
| NXP MCUXpresso SDK drivers (fsl_*)  -  read-only                         |
+--------------------------------------------------------------------------+
```

Separate images: PBL (`src/boot/pbl`, flash) and SBL (`src/boot/sbl`, RAM), see
`docs/specs/system/reprogramming.md`.

`IoHwAb` (ECU abstraction, `src/bsw/ecual`) drives the board I/O: the red LED of the fault
lamp (TASK-028). The later hardware I/O variant adds the sensor and actuator signals to
`IoHwAb`. The RTE then maps the `*In` / `*Out` ports to `IoHwAb` instead of `Com`, and the
SWCs do not change (DC-01).

Dependency rules: `.claude/rules/layering.md`.

## Module list

Task and status: `.claude/tasks/backlog.md`.

| Module | Layer | Design notes |
|---|---|---|
| Std_Types / Platform_Types / Compiler | platform | [Platform](../modules/Platform/design.md) |
| Det | bsw/services | — |
| Mcu | mcal | — |
| Port | mcal | — |
| Dio | mcal | — |
| Gpt | mcal | — |
| Os / SchM | os, bsw/services | — |
| EcuM | bsw/services | — |
| BswM | bsw/services | — |
| Can | mcal | — |
| CanIf | bsw/com | — |
| CanSM | bsw/com | — |
| PduR | bsw/com | — |
| Com | bsw/com | — |
| IoHwAb | bsw/ecual | — |
| Rte | rte | — |
| SWCs | app | [swc_composition.md](swc_composition.md) |
| Wdg | mcal | — |
| CanTp | bsw/com | — |
| Dcm | bsw/services | — |
| Dem | bsw/services | — |
| DbgOut (debug UART CDD) | bsw (location in TASK-031) | — |
| PBL / SBL | boot | — |

## Signal flow

```
CAN -> Can -> CanIf -> PduR -> Com -> RTE ports -> SWCs -> RTE ports -> Com -> PduR -> CanIf -> Can -> CAN
Red LED: SwcFaultMgr -> RTE -> IoHwAb -> Dio
Diagnostics: Can -> CanIf -> PduR -> CanTp -> Dcm (reads PpProgPrecond through the RTE)
Modes: SwcFaultMgr -> TcuMode -> BswM -> Com TCU_Tx group; CanSM -> BswM -> CanBusState -> SwcFaultMgr
```

Signals, IDs and scaling come from `docs/specs/system/can_matrix.md`, the single source
(DC-03). CAN-mapped data elements keep the matrix name and raw scaling. This table lists
every RTE port; [`swc_composition.md`](swc_composition.md) section 3 has the data elements,
ranges, init values and the mapping. Periods are those of the producer's runnable.

| RTE port | Type | Unit / scaling | Producer | Consumer | Period |
|---|---|---|---|---|---|
| RpDriverIn (Selector, Brake) | uint8, boolean | enum 0 P, 1 R, 2 N, 3 D (4 .. 7 invalid); 0 / 1 | Com `SIM_Driver` | SwcGearSelector | 10 ms |
| RpThrottleIn (Throttle) | uint8 | 0.5 %/bit (201 .. 255 invalid) | Com `SIM_Driver` | SwcShiftCtrl | 10 ms |
| RpShaftSpeedIn (InputShaftSpeed, OutputShaftSpeed) | uint16 | 1 rpm/bit (> 8000 invalid) | Com `SIM_Speed` | SwcSensorInput | 10 ms |
| RpAnalogIn (OilTempVoltage, LinePressVoltage) | uint16 | 1 mV/bit (> 5000 invalid) | Com `SIM_Analog` | SwcSensorInput | 10 ms |
| PpSolenoidOut (SS_A, SS_B, PCS_Duty) | boolean, boolean, uint16 | 0 / 1; 0.1 %/bit | SwcSolenoidCtrl | Com `TCU_Actuators` | 10 ms |
| PpLockSolenoidOut (RLS, SLS) | boolean | 0 / 1 | SwcGearSelector | Com `TCU_Actuators`; SwcSolenoidCtrl (RLS) | 10 ms |
| PpFaultLampOut (FaultLamp) | boolean | 0 / 1 | SwcFaultMgr | Com `TCU_Actuators`; IoHwAb `FaultLampLed` | 10 ms |
| PpGearStatus (GearTarget, GearActual, ShiftInProgress) | uint8, uint8, boolean | enum of can_matrix 3.5; 0 / 1 | SwcShiftCtrl | Com `TCU_Status`; SwcSolenoidCtrl; SwcFaultMgr | 10 ms |
| PpSelectorState (SelectorState) | uint8 | enum, 7 = invalid | SwcGearSelector | Com `TCU_Status`; SwcFaultMgr | 10 ms |
| PpVehicleSpeed (VehicleSpeed) | uint16 | 0.1 km/h/bit, 0xFFFF = invalid | SwcSensorInput | Com `TCU_Status`; SwcGearSelector; SwcShiftCtrl; SwcFaultMgr | 10 ms |
| PpOilTempStatus (OilTemp) | uint8 | 1 degC/bit, offset -40, 0xFF = invalid | SwcSensorInput | Com `TCU_Status` | 10 ms |
| PpModeStatus (Mode) | uint8 | 0 Normal, 1 Limp-home | SwcFaultMgr | Com `TCU_Status` | 10 ms |
| PpFaultBits (FaultBits) | uint16 | bit field of can_matrix 3.6 | SwcFaultMgr | Com `TCU_Status`; SwcSolenoidCtrl | 10 ms |
| PpStandstill (Standstill) | boolean | TRUE = speed valid and < 2 km/h | SwcSensorInput | SwcGearSelector; SwcShiftCtrl | 10 ms |
| PpShaftSpeeds (InputShaftSpeed, OutputShaftSpeed) | uint16 | 1 rpm/bit, 0xFFFF = invalid | SwcSensorInput | SwcShiftCtrl | 10 ms |
| PpOilTemp (OilTempPhys, OilTempValid) | sint16, boolean | 0.1 degC/bit | SwcSensorInput | SwcShiftCtrl | 10 ms |
| PpLinePress (LinePress, LinePressValid) | uint16, boolean | 0.01 bar/bit | SwcSensorInput | SwcSolenoidCtrl | 10 ms |
| PpSensorFaults (ShaftSpeedInvalid, LinePressSensorFault, OilTempSensorFault, OilOverTemp, ComFaultSimSpeed, ComFaultSimAnalog) | boolean | fault conditions | SwcSensorInput | SwcFaultMgr | 10 ms |
| PpSelectorPosition (SelectorPosition) | uint8 | enum 0 P, 1 R, 2 N, 3 D | SwcGearSelector | SwcShiftCtrl; SwcSolenoidCtrl | 10 ms |
| PpSelectorFaults (SelectorFault, SelectorFaultInD, SelectorKnown, ComFaultSimDriver) | boolean | fault conditions | SwcGearSelector | SwcFaultMgr | 10 ms |
| PpThrottle (Throttle) | uint8 | 0.5 %/bit, validated | SwcShiftCtrl | SwcSolenoidCtrl | 10 ms |
| PpShiftFaults (ShiftFault, RatioFault) | boolean | fault conditions | SwcShiftCtrl | SwcFaultMgr | 10 ms |
| PpSolenoidFaults (PressureFault) | boolean | fault condition | SwcSolenoidCtrl | SwcFaultMgr | 10 ms |
| PpTcuMode (mode `TcuMode`) | mode | INIT, NORMAL, LIMPHOME | SwcFaultMgr | SwcShiftCtrl; SwcSolenoidCtrl; BswM | on change |
| PpProgPrecond (ProgPrecondMask) | uint8 | bit mask of RID 0x0203 | SwcGearSelector | Dcm | 10 ms |
| RpCanBusState (mode `CanBusState`) | mode | ONLINE, BUSOFF | BswM | SwcFaultMgr | on change |
| RpDebugOut (C/S WriteLine) | C/S | ASCII status line | SwcFaultMgr | DbgOut CDD | 500 ms |
| RpDemEvt&lt;Fault&gt; (12 ports, C/S SetEventStatus) | C/S | PASSED / FAILED | SwcFaultMgr | Dem | 10 ms |

## Scheduling

Basic tasks (OSEK BCC1), full-preemptive, AUTOSAR OS SC1. Every task is released at the
same tick every 10 ms / 100 ms, and the priority gives the order. The order inside a task is
a design input for TASK-009 and the BSW module designs; details, latency and budgets are in
[`swc_composition.md`](swc_composition.md) section 4.

| OS task | Period | Priority | Runnables / main functions |
|---|---|---|---|
| OsTask_1ms | 1 ms | 3 | Can_MainFunction_BusOff, Can_MainFunction_Read, Can_MainFunction_Write, Can_MainFunction_Mode (polling) |
| OsTask_10ms | 10 ms | 2 | Com_MainFunctionRx, CanSM_MainFunction, SwcSensorInput_Run10ms, SwcGearSelector_Run10ms, SwcShiftCtrl_Run10ms, SwcFaultMgr_Run10ms, SwcSolenoidCtrl_Run10ms, BswM_MainFunction, Com_MainFunctionTx, CanTp_MainFunction, Dcm_MainFunction, Dem_MainFunction (from TASK-025), watchdog service (last) |
| OsTask_100ms | 100 ms | 1 | SwcFaultMgr_Run100ms |
| initialisation | once | - | `Rte_Start`: SwcSensorInput_Init, SwcGearSelector_Init, SwcShiftCtrl_Init, SwcFaultMgr_Init, SwcSolenoidCtrl_Init |
