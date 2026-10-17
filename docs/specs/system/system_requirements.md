# System requirements (SYS)

| Field    | Value |
|----------|-------|
| Scope    | TCU (FRDM-MCXN947) as a black box |
| Parent   | `safety_goals.md`, `system_context.md` |
| Version  | 0.1   |
| Status   | Approved |

## 1. Purpose and scope

This document specifies the externally visible behaviour of the TCU: the application,
the PBL and the SBL. Signals, modes and system parameters are defined in
`system_context.md`; CAN layouts in `can_matrix.md`; the reprogramming sequence and
memory map in `reprogramming.md`.

The behaviour of the simulation environment (plant model, gateway, console, tester) is
specified in `../simulation/sim_requirements.md`.

## 2. Terms

| Term | Meaning |
|---|---|
| Vehicle speed | speed computed by the TCU from OutputShaftSpeed (`SYS-SEN-001`) |
| Standstill | vehicle speed < 2 km/h |
| Control active | the TCU sends `TCU_Actuators` with values computed by the control functions |
| Shift in progress | from the change of the commanded gear until shift completion (`SYS-SFT-007`) |
| Fault | a condition detected by a requirement of this document; each fault has a bit in `TCU_Status.FaultBits` |
| Power cycle | MCU reset (power-on, watchdog or software reset) |

## 3. Requirements

### 3.1 SEL - gear selector

### SYS-SEL-001 - Selector position

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall determine the selector position P, R, N or D from `SIM_Driver.Selector`, accepting a new position only after it has been received unchanged for 50 ms. |
| Rationale    | Filter short glitches while the lever moves between positions. |
| Parent       | - |
| Verification | UT - step the input and check the delay; TT - move the selector from the console |
| Priority     | Must |
| Status       | Approved |

### SYS-SEL-002 - Invalid selector position

| Attribute    | Value |
|--------------|-------|
| Statement    | When `SIM_Driver.Selector` is invalid for longer than 100 ms, the TCU shall treat the selector position as N, set the selector fault and keep it until a valid position has been received for 500 ms. |
| Rationale    | An unknown position must not produce drive torque. |
| Parent       | SG-03 |
| Verification | UT; TT - inject an invalid selector value from the console |
| Priority     | Must |
| Status       | Approved |

Note: the selector fault itself is not latched; it clears as stated above, so that a
short glitch in P, R or N does not leave the vehicle locked in P or N until the next
power cycle. A selector fault while D is selected is also a limp-home condition
(`SYS-FLT-002`); "D is selected" means that the last valid selector position before the
fault was D. In that case limp-home stays latched by `SYS-FLT-004`, even after the
selector fault has cleared.

### SYS-SEL-003 - Shift lock (brake interlock)

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall command SLS = ON only while the selector position is P, the brake switch is pressed and the vehicle is at standstill; otherwise SLS = OFF. |
| Rationale    | The driver must press the brake to leave P. |
| Parent       | SG-03 |
| Verification | UT; TT - try to leave P with and without brake |
| Priority     | Must |
| Status       | Approved |

### SYS-SEL-004 - Reverse lockout

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall command RLS = ON only while the selector position is R and the vehicle speed is valid and below 2 km/h; otherwise RLS = OFF. Once ON, RLS stays ON while R remains selected. |
| Rationale    | Reverse must not engage while moving forward. |
| Parent       | SG-01 |
| Verification | UT; TT - select R at 1 km/h (engages) and at 20 km/h (stays neutral) |
| Priority     | Must |
| Status       | Approved |

Note: the speed threshold is checked only while RLS is OFF. It is evaluated continuously,
so if R is already selected and the speed falls below 2 km/h, RLS turns ON then; it does
not require selecting R again. Once ON, a speed above 2 km/h does not turn RLS OFF.

Note: with an invalid vehicle speed, reverse is locked out, also in limp-home. Without a valid speed the TCU cannot know that the vehicle
is slow enough, so the safe reaction (no reverse) takes priority over availability.

### SYS-SEL-005 - Outputs in P, R and N

| Attribute    | Value |
|--------------|-------|
| Statement    | While the selector position is P, R or N, the TCU shall command the solenoid pattern (`SYS-ACT-001`) of the gear that it would command if D were selected (`SYS-SFT-006`): 1st gear at standstill, otherwise the gear given by the shift schedule within the limit of `SYS-SFT-005`. |
| Rationale    | The gear is already prepared when D is selected. The plant engages the gear of SS_A / SS_B as soon as the lever reaches D, before the TCU has accepted D (`SYS-SEL-001`, 50 ms); a prepared low gear while rolling would overspeed the input shaft. |
| Parent       | SG-02, SG-03 |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

Note: changed on 2026-10-17 (user). The former statement commanded 1st gear in P, R and N
at every speed. Selecting D at 100 km/h would then engage 1st gear in the plant for at
least the 50 ms selector debounce (predicted input shaft speed about 8460 rpm), which
violates `SG-02`.

### 3.2 SFT - gear selection and shifting

### SYS-SFT-001 - Shift schedule

| Attribute    | Value |
|--------------|-------|
| Statement    | In D and in mode Normal, the TCU shall determine the target gear (1..4) from the vehicle speed and the throttle position using calibrated upshift and downshift tables. |
| Rationale    | Core function of the TCU. |
| Parent       | - |
| Verification | UT - table points and interpolation; TT - acceleration drive with the plant |
| Priority     | Must |
| Status       | Approved |

### SYS-SFT-002 - Shift hysteresis

| Attribute    | Value |
|--------------|-------|
| Statement    | For every throttle position, the downshift vehicle speed from gear n+1 to n shall be at least 5 km/h lower than the upshift vehicle speed from gear n to n+1. |
| Rationale    | Avoid shift hunting around a threshold. |
| Parent       | - |
| Verification | RV of the calibration tables; UT - speed oscillating around a threshold causes one shift |
| Priority     | Must |
| Status       | Approved |

Note: the requirement prevents shift hunting. If the upshift 2->3 and the downshift
3->2 happened at the same speed (for example 40 km/h), a vehicle cruising near that speed
would change gear on every small speed oscillation. With the downshift speed at least
5 km/h lower (for example upshift at 40 km/h, downshift at 35 km/h), the gear is kept
between 35 and 40 km/h, so one oscillation causes at most one shift.

Without hysteresis (upshift and downshift both at 40 km/h): a shift on every oscillation.

```
speed [km/h]:  39.8   40.2   39.9   40.1   39.8
gear:            2      3      2      3      2     <- gear changes four times
```

With hysteresis (upshift 2->3 at 40 km/h, downshift 3->2 at 35 km/h): one shift only.

```
speed [km/h]:  39.8   40.2   39.9   40.1   39.8
gear:            2      3      3      3      3     <- gear changes once
```

- The constraint applies to the calibration data, not to the code: the code does not add
  the 5 km/h itself, and it must hold for every throttle position (every row of the tables).
- It is verified by reviewing the tables (every pair of thresholds at least 5 km/h apart)
  and by a unit test with the speed oscillating around a threshold.
- `SYS-SFT-004` (minimum time between shifts) also limits shift busyness but works
  differently: it is a time limit after a shift, while this requirement is a speed
  hysteresis.

### SYS-SFT-003 - Single-step shifts

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall change the commanded gear by at most one step at a time. |
| Rationale    | Simplification; avoids large engine speed jumps. |
| Parent       | SG-02 |
| Verification | UT - kickdown from 4th reaches 2nd via 3rd |
| Priority     | Must |
| Status       | Approved |

### SYS-SFT-004 - Minimum time between shifts

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall not start a shift earlier than 2000 ms after the completion of the previous shift. |
| Rationale    | Avoid shift busyness; let the plant settle. |
| Parent       | - |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

Note: the limp-home outputs (`SYS-FLT-001`) are not delayed by this requirement.

### SYS-SFT-005 - Downshift overspeed protection

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall not start a downshift, and shall not select a gear at drive engagement (`SYS-SFT-006`, `SYS-SEL-005`), if the predicted input shaft speed in the new gear exceeds the downshift speed limit of 6000 rpm. The prediction is OutputShaftSpeed x ratio of the new gear; for a downshift in D it is the larger of this value and InputShaftSpeed x ratio of the new gear / ratio of the current gear. |
| Rationale    | Engine overspeed and sudden engine braking. The limit is below the maximum input shaft speed (6500 rpm, `system_context.md` 4.1) so that the engine does not exceed the maximum when the vehicle accelerates after the downshift while `SYS-SFT-004` blocks the next upshift. |
| Parent       | SG-02 |
| Verification | UT - in 2nd at 100 km/h a 2->1 request is refused (predicted about 8460 rpm), boundary at the limit +/- 1 rpm, frozen OutputShaftSpeed with valid InputShaftSpeed is refused, N -> D at 100 km/h selects no gear below 2nd; TT - kickdown at 100 km/h with the plant stays in 2nd |
| Priority     | Must |
| Status       | Approved |

### SYS-SFT-006 - Drive engagement

| Attribute    | Value |
|--------------|-------|
| Statement    | When D is selected at standstill, the TCU shall command 1st gear. When D is selected above standstill, the TCU shall command the gear given by the shift schedule within the limit of `SYS-SFT-005`. |
| Rationale    | Defined start gear; N -> D while rolling must not select a wrong gear. |
| Parent       | SG-02 |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

### SYS-SFT-007 - Shift completion monitoring

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall consider a shift complete when InputShaftSpeed / OutputShaftSpeed stays within +/-5 % of the target gear ratio for 100 ms. If a shift is not complete within 800 ms, the TCU shall set the shift fault. The check is done only when OutputShaftSpeed > 200 rpm. |
| Rationale    | Detect a stuck solenoid or a slipping clutch. |
| Parent       | SG-04 |
| Verification | UT; TT - console injects a stuck solenoid |
| Priority     | Must |
| Status       | Approved |

### SYS-SFT-008 - Cold oil

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall inhibit 4th gear when the oil temperature falls below -10 degC and release the inhibit when it rises above -5 degC; at startup, 4th gear is inhibited if the oil temperature is at or below -5 degC. While 4th gear is inhibited, the TCU shall not command 4th gear, and if 4th gear is commanded when the inhibit becomes active, the TCU shall downshift to 3rd. |
| Rationale    | Protect the transmission when the oil is viscous. |
| Parent       | - |
| Verification | UT - thresholds -10 degC and -5 degC +/- 1 K, startup at -7 degC (inhibited), inhibit becoming active in 4th (downshift to 3rd), refused downshift from 4th (`SYS-SFT-005`) keeps 4th |
| Priority     | Should |
| Status       | Approved |

Note: `SYS-SFT-005` takes priority; if the downshift from 4th is refused, 4th is kept.

### 3.3 ACT - actuators

### SYS-ACT-001 - Solenoid pattern

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall command SS_A and SS_B according to the solenoid table in `system_context.md` section 4 for the commanded gear. |
| Rationale    | Interface to the hydraulic model. |
| Parent       | - |
| Verification | UT; TT - plant engages the expected gear |
| Priority     | Must |
| Status       | Approved |

### SYS-ACT-002 - Line pressure control

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall compute PCS_Duty from a calibrated table of throttle position and commanded gear, and use a separate calibrated pressure profile while a shift is in progress. |
| Rationale    | Clutch capacity must follow input torque. |
| Parent       | - |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

### SYS-ACT-003 - Line pressure for reverse

| Attribute    | Value |
|--------------|-------|
| Statement    | While the selector position is R and RLS = ON, the TCU shall command a PCS_Duty that gives a line pressure of at least 15 bar with the PCS characteristic 0 % -> 20 bar, 100 % -> 3 bar, linear in between (the static characteristic of `SIM-PLT-007`). |
| Rationale    | Reverse needs higher clutch capacity. |
| Parent       | - |
| Verification | UT |
| Priority     | Should |
| Status       | Approved |

Note: The TCU cannot observe the engaged gear and commands
PCS_Duty, not a pressure, so the condition and the output are now stated in TCU terms.
With the values above, 15 bar corresponds to PCS_Duty <= 29.4 %.

### SYS-ACT-004 - Outputs before control is active

| Attribute    | Value |
|--------------|-------|
| Statement    | From reset until control is active, the TCU shall not transmit `TCU_Actuators`. |
| Rationale    | The plant then applies the hydraulic default (`SIM-PLT-004`), which is the safe state. The TCU never sends values that were not computed by the control functions. |
| Parent       | SG-04 |
| Verification | TT - observe the bus after reset |
| Priority     | Must |
| Status       | Approved |

### 3.4 SEN - sensors

### SYS-SEN-001 - Vehicle speed

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall compute the vehicle speed from OutputShaftSpeed, the final drive ratio and the tyre radius, with a range of 0 .. 250 km/h and a resolution of 0.1 km/h, every 10 ms. |
| Rationale    | Input of the shift schedule and of the interlocks. |
| Parent       | - |
| Verification | UT - conversion at several points |
| Priority     | Must |
| Status       | Approved |

### SYS-SEN-002 - Shaft speeds

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall use InputShaftSpeed and OutputShaftSpeed in the range 0 .. 8000 rpm and treat larger values as invalid. |
| Rationale    | Defined valid range. |
| Parent       | - |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

### SYS-SEN-003 - Analog sensor conversion

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall convert OilTempVoltage to oil temperature (-40 .. 150 degC) and LinePressVoltage to line pressure (0 .. 25 bar) with the sensor characteristics in `can_matrix.md` section 4. |
| Rationale    | Analog sensors are transmitted as voltages (`system_context.md` 3.1). |
| Parent       | - |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

### SYS-SEN-004 - Analog sensor range check

| Attribute    | Value |
|--------------|-------|
| Statement    | When an analog sensor voltage is outside 200 .. 4800 mV for longer than 100 ms, the TCU shall set the fault of that sensor. |
| Rationale    | Detect open and short circuits. |
| Parent       | SG-04 |
| Verification | UT; TT - inject 0 mV and 5000 mV from the console |
| Priority     | Must |
| Status       | Approved |

### SYS-SEN-005 - Gear ratio plausibility

| Attribute    | Value |
|--------------|-------|
| Statement    | When no shift is in progress, a forward gear is engaged and OutputShaftSpeed > 200 rpm, the TCU shall set the ratio fault if InputShaftSpeed / OutputShaftSpeed differs from the commanded gear ratio by more than 10 % for longer than 500 ms. |
| Rationale    | Detect a wrong gear, a slipping clutch or a speed sensor fault. |
| Parent       | SG-04 |
| Verification | UT; TT - console injects a wrong input shaft speed |
| Priority     | Must |
| Status       | Approved |

### SYS-SEN-006 - Line pressure plausibility

| Attribute    | Value |
|--------------|-------|
| Statement    | When the measured line pressure differs from the pressure expected for the commanded PCS_Duty by more than 3 bar for longer than 500 ms, the TCU shall set the pressure fault. |
| Rationale    | Detect a PCS or pump failure. |
| Parent       | SG-04 |
| Verification | UT; TT - console injects a stuck pressure |
| Priority     | Should |
| Status       | Approved |

### 3.5 FLT - fault reactions

### SYS-FLT-001 - Limp-home behaviour

| Attribute    | Value |
|--------------|-------|
| Statement    | In limp-home, the TCU shall command SS_A = OFF, SS_B = OFF, PCS_Duty = 0 % (maximum pressure) and FaultLamp = ON. The interlocks `SYS-SEL-003` and `SYS-SEL-004` stay active. |
| Rationale    | 3rd gear in D is the hydraulic default; the vehicle stays drivable. |
| Parent       | SG-04 |
| Verification | UT; TT |
| Priority     | Must |
| Status       | Approved |

Note: the limp-home outputs are applied without regard to `SYS-SFT-004` (minimum time
between shifts) and `SYS-SFT-005` (downshift overspeed protection). 3rd gear is the
hydraulic default, so the plant reaches it anyway when the TCU loses control.

### SYS-FLT-002 - Limp-home entry

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall enter limp-home when any of these faults is set: shift fault (`SYS-SFT-007`), ratio fault (`SYS-SEN-005`), pressure fault (`SYS-SEN-006`), line pressure sensor fault (`SYS-SEN-004`), shaft speed invalid (`SYS-SEN-002`), selector fault while D is selected (`SYS-SEL-002`), communication fault of `SIM_Driver` or `SIM_Speed` (`SYS-COM-002`), bus-off not recovered (`SYS-COM-006`). |
| Rationale    | Faults that make gear control unreliable. |
| Parent       | SG-04 |
| Verification | UT - one test per entry condition |
| Priority     | Must |
| Status       | Approved |

### SYS-FLT-003 - Fault reaction time

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall transmit the limp-home outputs within 100 ms after a fault of `SYS-FLT-002` is set. |
| Rationale    | Bounded reaction time. |
| Parent       | SG-04 |
| Verification | TT - timestamp of the fault injection vs. first limp-home `TCU_Actuators` frame |
| Priority     | Must |
| Status       | Approved |

### SYS-FLT-004 - Limp-home latch

| Attribute    | Value |
|--------------|-------|
| Statement    | Once entered, the TCU shall stay in limp-home until the next power cycle, even if the fault disappears. |
| Rationale    | Avoid toggling between normal and limp-home on an intermittent fault. |
| Parent       | SG-04 |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

### SYS-FLT-005 - Substitute oil temperature

| Attribute    | Value |
|--------------|-------|
| Statement    | When the oil temperature sensor fault is set, the TCU shall use a substitute oil temperature of 80 degC, switch the fault lamp ON and stay in mode Normal. |
| Rationale    | Oil temperature only affects comfort functions; limp-home is not needed. |
| Parent       | SG-04 |
| Verification | UT |
| Priority     | Must |
| Status       | Approved |

### SYS-FLT-006 - Oil overtemperature

| Attribute    | Value |
|--------------|-------|
| Statement    | While the oil temperature is above 130 degC, the TCU shall switch the fault lamp ON. |
| Rationale    | Warn the driver. |
| Parent       | - |
| Verification | UT |
| Priority     | Should |
| Status       | Approved |

Note: overtemperature is a warning only; it does not limit the gear or the line pressure

### SYS-FLT-007 - Fault memory

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall store every fault of this document as a DTC with its status, readable with UDS `19` and clearable with UDS `14`. |
| Rationale    | Diagnostics after the event. |
| Parent       | - |
| Verification | TT - tester reads DTCs after a fault injection |
| Priority     | Could |
| Status       | Approved |

Note: needs `Dem` (backlog). Until then, faults are visible only in `TCU_Status.FaultBits`.

### 3.6 COM - CAN communication

### SYS-COM-001 - Bus and reception

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall communicate on classic CAN at 500 kbit/s with 11-bit identifiers and shall receive the messages `SIM_Driver`, `SIM_Speed` and `SIM_Analog` defined in `can_matrix.md`. |
| Rationale    | Single interface to the plant. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-COM-002 - Reception timeout

| Attribute    | Value |
|--------------|-------|
| Statement    | When a received message has not arrived for its timeout (`can_matrix.md`, default 3 x cycle time), the TCU shall use the substitute values of its signals and set the communication fault of that message. |
| Rationale    | Defined behaviour on a lost message. |
| Parent       | SG-04 |
| Verification | UT (Com deadline monitoring); TT - console stops one message |
| Priority     | Must |
| Status       | Approved |

### SYS-COM-003 - Alive counter

| Attribute    | Value |
|--------------|-------|
| Statement    | Every cyclic message of `can_matrix.md` shall contain a 4-bit alive counter incremented by 1 (mod 16) per transmission. A received message whose counter did not change, or jumped by more than 2, shall be ignored; 3 consecutive ignored messages are treated as a timeout (`SYS-COM-002`). |
| Rationale    | Detect a frozen sender. |
| Parent       | SG-04 |
| Verification | UT; TT - console freezes a counter |
| Priority     | Must |
| Status       | Approved |

### SYS-COM-004 - Transmission

| Attribute    | Value |
|--------------|-------|
| Statement    | While control is active, the TCU shall transmit `TCU_Actuators` every 10 ms and `TCU_Status` every 100 ms, with a cycle time tolerance of +/-1 ms. |
| Rationale    | The plant needs fresh actuator values. |
| Parent       | - |
| Verification | TT - measure with `candump -t d` |
| Priority     | Must |
| Status       | Approved |

### SYS-COM-005 - Communication control

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall support stopping and restarting the transmission of `TCU_Actuators` and `TCU_Status` with UDS `28` (CommunicationControl). |
| Rationale    | Free the bus before reprogramming. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-COM-006 - Bus-off

| Attribute    | Value |
|--------------|-------|
| Statement    | After a bus-off, the TCU shall try to recover; if the bus is not back within 1000 ms, it shall set the bus-off fault. |
| Rationale    | Defined behaviour on a bus failure. |
| Parent       | SG-04 |
| Verification | TT - short CANH/CANL briefly |
| Priority     | Should |
| Status       | Approved |

### 3.7 TIM - timing

### SYS-TIM-001 - Cycle times

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall process sensor inputs, selector logic, shift control, actuator commands and fault detection with a 10 ms cycle, and status reporting with a 100 ms cycle. |
| Rationale    | Response time of the control functions. |
| Parent       | - |
| Verification | AN - OS configuration; TT - cycle time measurement |
| Priority     | Must |
| Status       | Approved |

### SYS-TIM-002 - CPU load

| Attribute    | Value |
|--------------|-------|
| Statement    | Every task shall finish within its period, and the average CPU load shall stay below 50 %. |
| Rationale    | Reserve for later functions (Dem, diagnostics). |
| Parent       | - |
| Verification | TT - runtime measurement with a timer or GPIO toggles |
| Priority     | Should |
| Status       | Approved |

### 3.8 STA - startup and watchdog

### SYS-STA-001 - Startup time

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall make control active within 500 ms after reset, including the PBL application check. |
| Rationale    | Fast availability after power-on. |
| Parent       | SG-04 |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-STA-002 - Watchdog

| Attribute    | Value |
|--------------|-------|
| Statement    | If the application stops executing its 10 ms cycle, a watchdog shall reset the MCU within 100 ms. |
| Rationale    | After the reset, `SYS-ACT-004` applies and the plant falls back to the hydraulic default. |
| Parent       | SG-04 |
| Verification | TT - debug hook that blocks the scheduler |
| Priority     | Must |
| Status       | Approved |

### 3.9 HMI - fault lamp and debug output

### SYS-HMI-001 - Fault lamp

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall command FaultLamp = ON while it is in limp-home or while any fault of section 3.5 requires it, and shall also drive the red LED of the board with the same value. |
| Rationale    | Driver warning, visible on the console and on the board. |
| Parent       | SG-04 |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-HMI-002 - Debug output

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU may print a status line (selector, gear, vehicle speed, mode, fault bits) on the debug UART every 500 ms. |
| Rationale    | Development aid. The console on the Pi5 is the main display. |
| Parent       | - |
| Verification | TT |
| Priority     | Could |
| Status       | Approved |

### 3.10 DIA - diagnostics (application)

### SYS-DIA-001 - UDS transport

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall provide UDS (ISO 14229-1) over ISO-TP (ISO 15765-2) with the diagnostic CAN identifiers of `can_matrix.md` section 5, in the application, PBL and SBL. |
| Rationale    | One tester for diagnostics and reprogramming. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-DIA-002 - Services in the application

| Attribute    | Value |
|--------------|-------|
| Statement    | The application shall support the services `10` (sessions 01, 02, 03), `11` (01), `22`, `27`, `28`, `31`, `3E` and `85`, with the sessions and security levels of `reprogramming.md` section 3. |
| Rationale    | Diagnostic access and entry into programming. |
| Parent       | - |
| Verification | TT - tester script per service, including negative responses |
| Priority     | Must |
| Status       | Approved |

### SYS-DIA-003 - Session timeout

| Attribute    | Value |
|--------------|-------|
| Statement    | When no request is received for S3 = 5000 ms in a non-default session, the TCU shall return to the default session and undo the effects of `28` and `85`. |
| Rationale    | ISO 14229 S3 timer; the ECU recovers when the tester disappears. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-DIA-004 - Programming preconditions

| Attribute    | Value |
|--------------|-------|
| Statement    | The application shall accept `10 02` (programming session) only when the vehicle is at standstill, the selector is P and the vehicle speed is valid; otherwise it shall respond with NRC `22` (conditionsNotCorrect). |
| Rationale    | No reprogramming while driving. |
| Parent       | SG-05 |
| Verification | UT; TT - request at 20 km/h is rejected |
| Priority     | Must |
| Status       | Approved |

### SYS-DIA-005 - Identification data

| Attribute    | Value |
|--------------|-------|
| Statement    | The TCU shall provide the DIDs of `reprogramming.md` section 4 (software versions, fingerprint, active session) with service `22`, in the application and in the bootloader. |
| Rationale    | Identify what is installed before and after programming. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### 3.11 BOOT - bootloader and reprogramming

### SYS-BOOT-001 - Application start

| Attribute    | Value |
|--------------|-------|
| Statement    | After reset, the PBL shall start the application only if no programming request is pending and the application image header and CRC32 (`reprogramming.md` section 6) are valid. |
| Rationale    | Never run a corrupted or incomplete image. |
| Parent       | SG-06 |
| Verification | TT - flash an image with a wrong CRC |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-002 - No valid application

| Attribute    | Value |
|--------------|-------|
| Statement    | If the application is not valid, the PBL shall stay in the bootloader in the default session and wait for a tester without time limit. |
| Rationale    | The ECU can always be recovered over CAN. |
| Parent       | SG-06 |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-003 - Programming request hand-over

| Attribute    | Value |
|--------------|-------|
| Statement    | When the application accepts `10 02`, it shall store a programming request in the RAM boot record in no-init RAM (`reprogramming.md` section 8) and perform a software reset. The PBL shall accept the request only if the record is valid (magic, inverted magic, CRC) and `CMC->SRS` reports a software reset; it shall then clear the request, enter the programming session and send the positive response `50 02`. In every other case the request is ignored. |
| Rationale    | Standard application -> bootloader hand-over. The RAM content after reset is not documented for this MCU (reference manual not available), so random or stale RAM must never be taken as a request. If the request is lost, `SYS-BOOT-011` is the fallback. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Deleted |

Note: deleted on 2026-10-17 (user). The PBL enters programming only through the boot
window (`SYS-BOOT-011`); the application resets after `10 02` (`SYS-BOOT-014`). No
programming request is stored in no-init RAM.

### SYS-BOOT-004 - Flash API usage

| Attribute    | Value |
|--------------|-------|
| Statement    | Only the SBL shall call the flash erase and program functions. The PBL and the application shall not link or call them. |
| Rationale    | No accidental erase from a fault in the PBL or the application. On the MCXN947 the flash driver is a ROM API (`fsl_flash.h` in `drivers/romapi/flash/`), so the code itself is always present in the chip; the protection is that only the SBL uses it. |
| Parent       | SG-06 |
| Verification | RV - map files of PBL and application contain no `FLASH_Erase` / `FLASH_Program` / flash API tree reference |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-005 - Security access

| Attribute    | Value |
|--------------|-------|
| Statement    | The PBL shall accept the download of the SBL, and the SBL shall accept erase and write requests, only after a successful security access (`27`) at the programming level. After 3 failed attempts, `27` shall be refused with NRC `36` / `37` for 10 s. |
| Rationale    | Protect against unintended programming. |
| Parent       | SG-06 |
| Verification | TT - wrong key, delay timer |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-006 - PBL protection

| Attribute    | Value |
|--------------|-------|
| Statement    | The SBL shall refuse to erase or write any address outside the application area and the boot info area of the memory map (`reprogramming.md` section 5). The PBL can be updated only with a debugger. |
| Rationale    | The PBL must always survive. |
| Parent       | SG-06 |
| Verification | TT - `34` to a PBL address returns NRC `31` |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-007 - Interrupted programming

| Attribute    | Value |
|--------------|-------|
| Statement    | If programming is interrupted at any step (reset, power loss, tester stops), the next reset shall end in the PBL (`SYS-BOOT-002`) and a full new programming sequence shall succeed. |
| Rationale    | No bricked ECU. |
| Parent       | SG-06 |
| Verification | TT - tester aborts at every step of the sequence |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-008 - Outputs in the bootloader

| Attribute    | Value |
|--------------|-------|
| Statement    | The PBL and the SBL shall not transmit `TCU_Actuators` or `TCU_Status`. |
| Rationale    | The plant applies the hydraulic default (`SIM-PLT-004`). |
| Parent       | SG-04 |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-009 - Bootloader session timeout

| Attribute    | Value |
|--------------|-------|
| Statement    | In the programming session, when no request is received for S3 = 5000 ms and the application is valid, the bootloader shall reset the MCU. |
| Rationale    | Return to the application when the tester disappears before erasing. |
| Parent       | - |
| Verification | TT |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-010 - Programming time

| Attribute    | Value |
|--------------|-------|
| Statement    | Programming a 256 KiB application, from `10 02` to the end of `11 01`, shall take at most 60 s. |
| Rationale    | Usable development loop. |
| Parent       | - |
| Verification | TT - measured by the tester |
| Priority     | Should |
| Status       | Approved |

### SYS-BOOT-011 - Boot window

| Attribute    | Value |
|--------------|-------|
| Statement    | After every reset, before starting a valid application, the PBL shall listen for 50 ms for a physical request `10 02` (0x7E1). If it is received, the PBL shall stay in the bootloader in the programming session and respond `50 02`. |
| Rationale    | Single entry into programming, without state kept across a reset. Normal path: the application resets after `10 02` (`SYS-BOOT-014`). Recovery path, when a valid but faulty application never processes `10 02`: the tester resets the ECU (power cycle or `11 01`). In both cases the tester sends `10 02` repeatedly until the PBL responds. The window fits in the start-up time of `SYS-STA-001`. |
| Parent       | SG-06 |
| Verification | TT - normal entry through `SYS-BOOT-014`; catch the PBL by power cycle with an application that hard-faults at start-up |
| Priority     | Must |
| Status       | Approved |

Note: the window does not bypass security; `SYS-BOOT-005` still applies.

Note: the PBL does not check the vehicle speed in the boot window (user, 2026-10-17). The
PBL has no plant signals, and a programming entry needs the tester and the security
access of `SYS-BOOT-005`. `SG-05` is covered on the normal path by `SYS-DIA-004` and
`SYS-BOOT-014`; a tester that resets the ECU while driving and enters through the window
is a misuse outside this protection.

### SYS-BOOT-012 - Reset loop detection

| Attribute    | Value |
|--------------|-------|
| Statement    | The PBL shall count consecutive abnormal resets (watchdog, lockup, fatal, as reported by `CMC->SRS`) in the RAM boot record. After 3 consecutive abnormal resets, the PBL shall not start the application and shall stay in the bootloader (`SYS-BOOT-002`). The counter shall be cleared by a power-on, pin or software reset, and by the application after control has been active for 10 s. |
| Rationale    | A valid but faulty application must not cause an endless reset loop. If the counter is lost with the RAM content, detection only restarts from 0; `SYS-BOOT-011` still works. |
| Parent       | SG-04, SG-06 |
| Verification | TT - application with a forced watchdog reset at start-up |
| Priority     | Must |
| Status       | Deleted |

Note: deleted on 2026-10-17 (user), together with the RAM boot record in no-init RAM, for
simplicity. A valid but faulty application that resets in a loop is recovered through the
boot window (`SYS-BOOT-011`). During such a loop `TCU_Actuators` is missing after every
reset (`SYS-ACT-004`), so the plant applies the hydraulic default for that time.

### SYS-BOOT-013 - Fault-free application check

| Attribute    | Value |
|--------------|-------|
| Statement    | The PBL application check (`SYS-BOOT-001`) shall complete without a processor fault even when the application area is erased or only partially programmed, and shall then treat the application as invalid. |
| Rationale    | The flash has ECC and `SystemInit()` enables a bus error on multi-bit ECC errors; reading an interrupted phrase could otherwise end in a fault and reset loop instead of `SYS-BOOT-002`. |
| Parent       | SG-06 |
| Verification | TT - power loss / reset during `36`, then reset; UT of the check logic with a fault-reporting read stub |
| Priority     | Must |
| Status       | Approved |

### SYS-BOOT-014 - Programming entry from the application

| Attribute    | Value |
|--------------|-------|
| Statement    | When the application accepts `10 02` (`SYS-DIA-004`), it shall send the positive response `50 02` and then perform a software reset within 20 ms after the response has been transmitted. The application shall not store any programming request across the reset. |
| Rationale    | Simple hand-over: after the reset the PBL enters the programming session only through the boot window (`SYS-BOOT-011`), on the tester's repeated `10 02`. The preconditions of `SYS-DIA-004` (standstill, P, valid speed) still protect `SG-05` on this path. |
| Parent       | SG-05 |
| Verification | TT - `10 02` at standstill in P: `50 02` from the application, reset, `50 02` from the PBL |
| Priority     | Must |
| Status       | Approved |

Note: the tester sends `10 02` on 0x7E1 every 5 ms after the application's `50 02` until the
PBL responds `50 02` (`reprogramming.md` section 9).

## 4. Open questions

| # | Question | Affects | Answer |
|---|---|---|---|
| 1 | Latch the selector fault like the other limp-home faults? | SYS-SEL-002 | No: the selector fault recovers; limp-home entered from it stays latched (`SYS-FLT-004`). Closed (user, 2026-10-06). |
| 2 | No reverse when the vehicle speed is invalid, also in limp-home? | SYS-SEL-004 | Yes: RLS = OFF when the speed is invalid, also in limp-home. Closed (user, 2026-10-17). |
| 3 | Overtemperature: warning only, or also limit gear / pressure? | SYS-FLT-006 | Warning only. Closed (user, 2026-10-17). |
| 4 | All `[NC: ...]` values: confirm or change | all | All values confirmed as proposed. Closed (user, 2026-10-17). |
| 5 | MCXN947 reference manual not available: RAM content after reset, ROM RAM usage and erased-flash read behaviour are unknown. Decision (user, 2026-10-04): no board experiment; use the robust design `SYS-BOOT-011`, `-013`; no state is kept in RAM across a reset (`SYS-BOOT-003` and `-012` deleted, user, 2026-10-17). | SYS-BOOT | closed |
