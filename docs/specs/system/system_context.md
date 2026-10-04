# System context

| Field    | Value  |
|----------|--------|
| Scope    | xAT system (TCU and simulation environment) |
| Version  | 0.1    |
| Status   | Draft  |

## 1. Purpose

xAT is a simplified **transmission control unit (TCU)** for a **4-speed automatic
transmission** with hydraulic clutches. The TCU runs on the FRDM-MCXN947 and follows the
AUTOSAR Classic R21-11 architecture.

There is no real transmission. All sensor inputs and actuator outputs are exchanged over
**CAN** with a **plant model** that simulates the driver, the vehicle and the
transmission. The TCU can be **reprogrammed over CAN with UDS** from the same host.

## 2. Nodes

```
+-------------------------------+
| Raspberry Pi 5 (host)         |
|  - plant model (Python)       |
|  - console (terminal UI)      |
|  - UDS tester                 |
|  - SocketCAN can0 via slcand  |
+---------------+---------------+
                | USB (MCU-Link VCOM), SLCAN protocol
+---------------+---------------+
| FRDM-MCXN236: CAN gateway     |
|  SLCAN <-> CAN, no app logic  |
|  NXP SDK application          |
+---------------+---------------+
                | CAN, 500 kbit/s, TJA1057 transceivers, 120 ohm at both ends
+---------------+---------------+
| FRDM-MCXN947: TCU             |
|  AUTOSAR app + PBL + SBL      |
|  CAN on J10 (TJA1057)         |
+-------------------------------+
```

| Node | Role | Specified in |
|---|---|---|
| TCU (FRDM-MCXN947) | system under development | `system_requirements.md` |
| Gateway (FRDM-MCXN236) | test environment | `../simulation/sim_requirements.md` (`SIM-GW`) |
| Host (Pi5) | test environment: plant model, console, tester | `../simulation/sim_requirements.md` (`SIM-PLT`, `SIM-CON`, `SIM-TST`) |

## 3. TCU interface (logical signals)

All signals are carried in the CAN messages defined in `can_matrix.md`.

### 3.1 Inputs (plant model -> TCU)

| Signal | Meaning | Unit | Range | Message |
|---|---|---|---|---|
| Selector | driver selector position P / R / N / D | enum | P, R, N, D, invalid | `SIM_Driver` |
| Brake | brake pedal switch | bool | 0 / 1 | `SIM_Driver` |
| Throttle | accelerator pedal position | % | 0 .. 100 | `SIM_Driver` |
| InputShaftSpeed | turbine / input shaft speed | rpm | 0 .. 8000 | `SIM_Speed` |
| OutputShaftSpeed | output shaft speed | rpm | 0 .. 8000 | `SIM_Speed` |
| OilTempVoltage | oil temperature sensor output | mV | 0 .. 5000 | `SIM_Analog` |
| LinePressVoltage | line pressure sensor output | mV | 0 .. 5000 | `SIM_Analog` |

Analog sensors are sent as **voltages**, not physical values, so that the TCU performs
the conversion and the electrical range checks (`SYS-SEN-004`) and the console can inject
open / short circuit faults.

### 3.2 Outputs (TCU -> plant model)

| Signal | Meaning | Unit | Range | Message |
|---|---|---|---|---|
| SS_A, SS_B | shift solenoids A / B | bool | OFF / ON | `TCU_Actuators` |
| PCS_Duty | pressure control solenoid duty; higher duty = lower line pressure | % | 0.0 .. 100.0 | `TCU_Actuators` |
| RLS | reverse lockout solenoid; ON = reverse may engage | bool | OFF / ON | `TCU_Actuators` |
| SLS | shift lock solenoid; ON = selector may leave P | bool | OFF / ON | `TCU_Actuators` |
| FaultLamp | transmission fault lamp | bool | OFF / ON | `TCU_Actuators` |
| Status | actual / target gear, mode, vehicle speed, fault bits | - | - | `TCU_Status` |
| Diagnostic responses | UDS over ISO-TP | - | - | `DIAG_Resp` |

## 4. Transmission model (assumptions shared by TCU and plant)

1. **Manual valve.** The selector mechanically selects the range: P and N give no drive,
   R gives reverse (only if RLS = ON), D gives the forward gear set by the solenoids.
2. **Solenoid table in D.**

   | Gear | SS_A | SS_B |
   |---|---|---|
   | 1 | ON  | ON  |
   | 2 | OFF | ON  |
   | 3 | OFF | OFF |
   | 4 | ON  | OFF |

3. **Hydraulic default.** With every output OFF (no power, TCU in reset, no CAN), the
   transmission is in **3rd gear in D**, line pressure is **maximum**, reverse is
   **locked out** and the selector is **locked in P**. This is the basis of limp-home.
4. **No torque converter slip** is modelled: engine speed = input shaft speed, never
   below idle. Torque converter lockup is out of scope.

### 4.1 System parameters (calibration, shared by TCU and plant)

| Parameter | Value | Unit |
|---|---|---|
| Ratio 1st / 2nd / 3rd / 4th | [NC: 3.06 / 1.63 / 1.00 / 0.70] | - |
| Ratio reverse | [NC: 2.29] | - |
| Final drive ratio | [NC: 3.23] | - |
| Tyre dynamic radius | [NC: 0.31] | m |
| Max input shaft speed | [NC: 6500] | rpm |
| Engine idle speed | [NC: 750] | rpm |

The TCU keeps these values in SWC `config/`; the plant model keeps its own copy. A
mismatch shows up as a ratio plausibility fault (`SYS-SEN-005`).

## 5. Operating modes of the TCU

| Mode | Entered when | Behaviour |
|---|---|---|
| Init | reset, until control is active | no `TCU_Actuators` sent; plant applies the hydraulic default |
| Normal | control active, no relevant fault | shift schedule, all functions |
| Limp-home | a fault listed in `SYS-FLT-002` | fixed 3rd in D, maximum pressure, fault lamp ON; latched until reset |
| Programming | UDS programming request accepted | application stopped; PBL / SBL running (`reprogramming.md`) |

## 6. Out of scope (v1)

- Torque converter and lockup clutch.
- Manual / tiptronic mode, adaptive shift learning, shift quality (clutch-to-clutch)
  control.
- Real sensors and actuators (Adc / Pwm / Icu drivers). See design constraint DC-01.
- Communication with other ECUs (engine, ABS); the only bus partner is the plant model.
- Cryptographic signature checks of downloaded software (CRC32 only in v1).
- ISO 26262 compliance. Safety goals are used for structure, not for certification.

## 7. Design constraints

| ID | Constraint | Reason |
|---|---|---|
| DC-01 | SWCs access sensor and actuator values **only through RTE ports**, independent of the signal source. Today the RTE maps the ports to `Com` signals; a later hardware variant maps them to `IoHwAb`, without changing the SWCs. | Easy change from simulated to real I/O |
| DC-02 | Physical conversions (voltage -> degC / bar, rpm -> km/h) are done in SWCs with calibration values from `config/`, not in the RTE or BSW. | Keep the RTE free of control logic; one place per conversion |
| DC-03 | `can_matrix.md` is the single source for CAN IDs, layouts and scaling. The `Com` configuration and the host DBC file are derived from it. | No divergence between TCU and plant |
| DC-04 | The application, PBL and SBL are **separate images** with separate builds. Only SBL contains flash erase / write code. | Robust reprogramming, see `SYS-BOOT-004` |
| DC-05 | The gateway is transparent: it has no knowledge of xAT messages or UDS. | The gateway never needs changing when the CAN matrix changes |
