# Simulation environment requirements (SIM)

| Field    | Value |
|----------|-------|
| Scope    | Plant model, console and UDS tester on the Pi5; CAN gateway on the FRDM-MCXN236 |
| Parent   | `../system/system_context.md`, `../system/can_matrix.md`, `../system/reprogramming.md` |
| Version  | 0.1   |
| Status   | Draft |

## 1. Purpose and scope

The simulation environment replaces the vehicle, the driver and the transmission, and
acts as the diagnostic tester. It is **test equipment, not part of the TCU**: the AUTOSAR
coding rules do not apply to it. It lives in `tools/sim/` (host tools) and
`tools/gateway/` (gateway firmware).

| Component | Runs on | Technology (proposal) |
|---|---|---|
| Plant model | Pi5 | Python 3, `python-can` on SocketCAN `can0` |
| Console | Pi5 | Python 3 terminal UI |
| Tester | Pi5 | Python 3, `udsoncan` + kernel `can-isotp` |
| Gateway | FRDM-MCXN236 | C, NXP MCUXpresso SDK application (not AUTOSAR) |

## 2. Requirements

### 2.1 PLT - plant model

### SIM-PLT-001 - Cycle and transmission

| Attribute    | Value |
|--------------|-------|
| Statement    | The plant model shall compute its state every 10 ms and transmit `SIM_Driver`, `SIM_Speed` and `SIM_Analog` every 10 ms with incrementing alive counters, as defined in `can_matrix.md`. |
| Rationale    | Inputs of the TCU. |
| Parent       | SYS-COM-001 |
| Verification | TT - `candump -t d` |
| Priority     | Must |
| Status       | Draft |

### SIM-PLT-002 - Range and gear engagement

| Attribute    | Value |
|--------------|-------|
| Statement    | The plant model shall engage no gear in P and N, reverse in R only while RLS = ON, and in D the gear given by SS_A / SS_B with the solenoid table of `system_context.md` section 4. |
| Rationale    | Manual valve and hydraulic model. |
| Parent       | - |
| Verification | RV; TT |
| Priority     | Must |
| Status       | Draft |

### SIM-PLT-003 - Shift dynamics

| Attribute    | Value |
|--------------|-------|
| Statement    | When the engaged gear changes, the plant model shall move the input shaft speed from the old ratio to the new ratio over [NC: 400] ms. |
| Rationale    | Gives `SYS-SFT-007` something realistic to monitor. |
| Parent       | - |
| Verification | RV |
| Priority     | Must |
| Status       | Draft |

### SIM-PLT-004 - Hydraulic default on missing TCU

| Attribute    | Value |
|--------------|-------|
| Statement    | When `TCU_Actuators` has not been received for [NC: 50] ms, the plant model shall use SS_A = SS_B = OFF, PCS_Duty = 0 %, RLS = OFF, SLS = OFF, FaultLamp = OFF. |
| Rationale    | Models the behaviour of an unpowered transmission (`system_context.md` section 4, item 3). |
| Parent       | SYS-ACT-004, SYS-BOOT-008 |
| Verification | TT - reset the TCU while driving |
| Priority     | Must |
| Status       | Draft |

### SIM-PLT-005 - Shift lock

| Attribute    | Value |
|--------------|-------|
| Statement    | The plant model shall ignore a selector change out of P requested by the console while SLS = OFF. |
| Rationale    | Models the mechanical shift lock. |
| Parent       | SYS-SEL-003 |
| Verification | TT |
| Priority     | Must |
| Status       | Draft |

### SIM-PLT-006 - Vehicle and engine model

| Attribute    | Value |
|--------------|-------|
| Statement    | The plant model shall compute engine torque from throttle and engine speed, vehicle acceleration from wheel torque, driving resistance and brake, and the shaft speeds from the vehicle speed and the engaged gear, with the system parameters of `system_context.md` 4.1. Engine speed shall not drop below idle (no stall). |
| Rationale    | Closed loop with the TCU. |
| Parent       | - |
| Verification | RV - plots of an acceleration drive |
| Priority     | Must |
| Status       | Draft |

### SIM-PLT-007 - Line pressure and oil temperature

| Attribute    | Value |
|--------------|-------|
| Statement    | The plant model shall compute the line pressure from PCS_Duty (0 % -> [NC: 20] bar, 100 % -> [NC: 3] bar, first-order lag [NC: 50] ms) and the oil temperature from a start value set in the console with a slow warm-up while driving, and convert both to sensor voltages with the characteristics of `can_matrix.md` section 4. |
| Rationale    | Inputs for `SYS-SEN-003` .. `SYS-SEN-006`. |
| Parent       | - |
| Verification | RV |
| Priority     | Must |
| Status       | Draft |

### 2.2 GW - gateway (FRDM-MCXN236)

### SIM-GW-001 - Transparent SLCAN bridge

| Attribute    | Value |
|--------------|-------|
| Statement    | The gateway shall implement the SLCAN (Lawicel) ASCII protocol on its USB virtual COM port and forward every CAN frame in both directions without filtering or changing it. |
| Rationale    | Linux `slcand` exposes it as SocketCAN; the gateway does not depend on the CAN matrix (DC-05). |
| Parent       | - |
| Verification | TT - `cansend` / `candump` loop with the TCU |
| Priority     | Must |
| Status       | Draft |

### SIM-GW-002 - Throughput and latency

| Attribute    | Value |
|--------------|-------|
| Statement    | The gateway shall forward frames without loss up to [NC: 60] % bus load at 500 kbit/s, with a latency of at most [NC: 2] ms per direction. |
| Rationale    | Cyclic traffic (about 300 frames/s) plus ISO-TP downloads. |
| Parent       | SYS-BOOT-010 |
| Verification | TT - frame counters on both ends |
| Priority     | Must |
| Status       | Draft |

### SIM-GW-003 - Error reporting

| Attribute    | Value |
|--------------|-------|
| Statement    | The gateway shall report CAN error states (error passive, bus-off) and RX buffer overflow through SLCAN status flags and show bus-off on a board LED. |
| Rationale    | Debug of the physical bus. |
| Parent       | - |
| Verification | TT |
| Priority     | Should |
| Status       | Draft |

### 2.3 CON - console

### SIM-CON-001 - Display

| Attribute    | Value |
|--------------|-------|
| Statement    | The console shall display every signal of `can_matrix.md` (inputs, outputs, status, fault bits as text), the message cycle times and the alive counter errors, refreshed at least every [NC: 100] ms. |
| Rationale    | Observe the TCU. |
| Parent       | - |
| Verification | RV |
| Priority     | Must |
| Status       | Draft |

### SIM-CON-002 - Driver inputs

| Attribute    | Value |
|--------------|-------|
| Statement    | The console shall let the user set the selector position, brake and throttle from the keyboard. |
| Rationale    | Manual driving. |
| Parent       | - |
| Verification | RV |
| Priority     | Must |
| Status       | Draft |

### SIM-CON-003 - Fault injection

| Attribute    | Value |
|--------------|-------|
| Statement    | The console shall inject at least these faults: invalid selector value, sensor voltage forced to a value (open / short), shaft speed offset or freeze, stop of one message, frozen alive counter, solenoid stuck ON or OFF in the plant, line pressure stuck. |
| Rationale    | Verification of `SYS-SEL-002`, `SYS-SEN-*`, `SYS-COM-*`, `SYS-SFT-007`, `SYS-FLT-*`. |
| Parent       | - |
| Verification | RV |
| Priority     | Must |
| Status       | Draft |

### SIM-CON-004 - Scenarios and logging

| Attribute    | Value |
|--------------|-------|
| Statement    | The console shall run scripted scenarios (timed driver inputs and fault injections) and log all CAN traffic and plant states to a file with timestamps. |
| Rationale    | Repeatable target tests (`TT`) and evidence. |
| Parent       | - |
| Verification | RV |
| Priority     | Must |
| Status       | Draft |

### 2.4 TST - UDS tester

### SIM-TST-001 - Flashing

| Attribute    | Value |
|--------------|-------|
| Statement    | The tester shall run the programming sequence of `reprogramming.md` section 2 for a given SBL image and application image (`.bin` with header), show the progress, and stop with a clear message on any negative response or timeout. |
| Rationale    | Firmware update over CAN. |
| Parent       | SYS-BOOT-001 .. SYS-BOOT-010 |
| Verification | TT |
| Priority     | Must |
| Status       | Draft |

### SIM-TST-002 - Negative scenarios

| Attribute    | Value |
|--------------|-------|
| Statement    | The tester shall provide scripted negative scenarios: wrong key, wrong CRC, programming while driving, address outside the application area, abort after each step of the sequence, and recovery through the boot window (`reprogramming.md` section 9) with an application that faults or resets at start-up. |
| Rationale    | Verification of `SYS-DIA-004`, `SYS-BOOT-005` .. `SYS-BOOT-007`, `SYS-BOOT-011` .. `SYS-BOOT-013`. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Draft |

### SIM-TST-003 - Diagnostic services

| Attribute    | Value |
|--------------|-------|
| Statement    | The tester shall read all DIDs of `reprogramming.md` section 4, change sessions, send tester present, and log every request and response with timestamps. |
| Rationale    | General diagnostics. |
| Parent       | SYS-DIA-002, SYS-DIA-005 |
| Verification | TT |
| Priority     | Must |
| Status       | Draft |

### SIM-TST-004 - Bus sharing

| Attribute    | Value |
|--------------|-------|
| Statement    | The tester shall run while the plant model is running, on the same SocketCAN interface. |
| Rationale    | Reprogramming at standstill with the plant model still providing inputs (`SYS-DIA-004`). |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Draft |

## 3. Open questions

| # | Question | Affects | Answer |
|---|---|---|---|
| 1 | Repository location of the sim tools and the gateway firmware (`tools/sim/`?) | all | `tools/sim/` and `tools/gateway/` (user, 2026-10-04) |
| 2 | Terminal UI library for the console | SIM-CON | |
