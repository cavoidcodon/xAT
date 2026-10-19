# CAN matrix

| Field    | Value |
|----------|-------|
| Scope    | CAN bus between the TCU and the plant model (via the gateway) |
| Parent   | `system_requirements.md` (`SYS-COM`, `SYS-DIA`) |
| Version  | 1.3   |
| Status   | Approved |

This document is the **single source** for CAN identifiers, layouts and scaling
(design constraint DC-03). The TCU `Com` / `CanIf` / `CanTp` configuration and the host
DBC file (`xat.dbc`, created later) are derived from it.

## 1. Bus parameters

| Parameter | Value |
|---|---|
| Protocol | classic CAN 2.0B, only 11-bit identifiers used |
| Bit rate | 500 kbit/s |
| Sample point | 87.5 % |
| Nodes | TCU (FRDM-MCXN947, J10), gateway (FRDM-MCXN236) |
| Termination | 120 ohm at each end of the bus |
| Transceiver | TJA1057 on both boards |

## 2. Message overview

All cyclic messages have DLC 8. Unused bits are sent as 0.

| ID | Name | Sender -> Receiver | Cycle | RX timeout in TCU | Content |
|---|---|---|---|---|---|
| 0x100 | `SIM_Driver` | plant -> TCU | 10 ms | 30 ms | selector, brake, throttle |
| 0x101 | `SIM_Speed` | plant -> TCU | 10 ms | 30 ms | input / output shaft speed |
| 0x102 | `SIM_Analog` | plant -> TCU | 10 ms | 30 ms | oil temperature and line pressure sensor voltages |
| 0x200 | `TCU_Actuators` | TCU -> plant | 10 ms | (plant: 50 ms, `SIM-PLT-004`) | solenoids, pressure control, fault lamp |
| 0x201 | `TCU_Status` | TCU -> plant | 100 ms | - | gear, mode, vehicle speed, faults |
| 0x7DF | `DIAG_FuncReq` | tester -> TCU | event | - | UDS functional request |
| 0x7E1 | `DIAG_PhysReq` | tester -> TCU | event | - | UDS physical request |
| 0x7E9 | `DIAG_Resp` | TCU -> tester | event | - | UDS response |

## 3. Signal layouts

Byte order: big endian (Motorola) for all signals. Bit k of the frame is bit (k mod 8) of
byte (k div 8); bit 7 is the most significant bit of a byte. "Start bit" is the position of
the signal's **most significant bit** (DBC Motorola convention); the following bits run down
to bit 0 of that byte and continue at bit 7 of the next byte, so a multi-byte signal has its
most significant byte first. Example: `InputShaftSpeed` (start bit 7, length 16) = byte 0
(high byte) and byte 1 (low byte).
`AliveCtr` is incremented by 1 (mod 16) per transmission (`SYS-COM-003`).

### 3.1 `SIM_Driver` (0x100)

| Signal | Start bit | Length | Scaling | Range | Invalid / notes |
|---|---|---|---|---|---|
| Selector | 2 | 3 | enum: 0 = P, 1 = R, 2 = N, 3 = D | 0 .. 3 | 4 .. 7 = invalid (lever between positions, sensor fault) |
| Brake | 3 | 1 | 0 = released, 1 = pressed | - | - |
| Throttle | 15 | 8 | 0.5 %/bit | 0 .. 200 (0 .. 100 %) | 201 .. 255 = invalid |
| AliveCtr | 59 | 4 | - | 0 .. 15 | - |

### 3.2 `SIM_Speed` (0x101)

| Signal | Start bit | Length | Scaling | Range | Invalid / notes |
|---|---|---|---|---|---|
| InputShaftSpeed | 7 | 16 | 1 rpm/bit | 0 .. 8000 | > 8000 = invalid |
| OutputShaftSpeed | 23 | 16 | 1 rpm/bit | 0 .. 8000 | > 8000 = invalid |
| AliveCtr | 59 | 4 | - | 0 .. 15 | - |

Speeds are unsigned: in reverse the plant sends the magnitude.

### 3.3 `SIM_Analog` (0x102)

| Signal | Start bit | Length | Scaling | Range | Invalid / notes |
|---|---|---|---|---|---|
| OilTempVoltage | 7 | 16 | 1 mV/bit | 0 .. 5000 | > 5000 = invalid; conversion in section 4 |
| LinePressVoltage | 23 | 16 | 1 mV/bit | 0 .. 5000 | > 5000 = invalid; conversion in section 4 |
| AliveCtr | 59 | 4 | - | 0 .. 15 | - |

### 3.4 `TCU_Actuators` (0x200)

| Signal | Start bit | Length | Scaling | Range | Notes |
|---|---|---|---|---|---|
| SS_A | 0 | 1 | 0 = OFF, 1 = ON | - | shift solenoid A |
| SS_B | 1 | 1 | 0 = OFF, 1 = ON | - | shift solenoid B |
| RLS | 2 | 1 | 0 = OFF (locked), 1 = ON (reverse allowed) | - | reverse lockout solenoid |
| SLS | 3 | 1 | 0 = OFF (locked in P), 1 = ON (may leave P) | - | shift lock solenoid |
| FaultLamp | 4 | 1 | 0 = OFF, 1 = ON | - | - |
| PCS_Duty | 15 | 16 | 0.1 %/bit | 0 .. 1000 (0 .. 100 %) | 0 % = maximum line pressure |
| AliveCtr | 59 | 4 | - | 0 .. 15 | - |

### 3.5 `TCU_Status` (0x201)

| Signal | Start bit | Length | Scaling | Range | Notes |
|---|---|---|---|---|---|
| GearActual | 3 | 4 | 0 = none / N, 1 .. 4, 5 = R | - | engaged gear as seen by the TCU; 15 = unknown |
| GearTarget | 7 | 4 | same as GearActual | - | commanded gear |
| Mode | 9 | 2 | 0 = Normal, 1 = Limp-home | - | - |
| SelectorState | 12 | 3 | same as `SIM_Driver.Selector` | - | debounced position; 7 = invalid, sent while the selector fault (`SYS-SEL-002`) is set |
| ShiftInProgress | 13 | 1 | 0 / 1 | - | - |
| VehicleSpeed | 23 | 16 | 0.1 km/h/bit | 0 .. 2500 | 0xFFFF = invalid |
| FaultBits | 39 | 16 | bit field, section 3.6 | - | - |
| OilTemp | 55 | 8 | 1 degC/bit, offset -40 | 0 .. 190 (-40 .. 150 degC) | 0xFF = invalid, sent while the substitute oil temperature (`SYS-FLT-005`, section 6) is used |
| AliveCtr | 59 | 4 | - | 0 .. 15 | - |

### 3.6 `TCU_Status.FaultBits`

Bit numbers refer to the signal value (bit 0 = least significant bit, located in byte 5).

DTCs (`SYS-FLT-007`) use DTCFormatIdentifier 0x00 (ISO 15031-6 / SAE J2012: two-byte DTC
plus failure type byte). Generic SAE codes are used where one matches; `U11xx` are
project-specific codes named after the CAN ID of the lost message. Failure type bytes:
`1C` voltage out of range, `29` signal invalid, `64` signal plausibility failure,
`87` missing message, `88` bus off, `00` no sub-type. P0715 stands for both shaft speeds.

| Bit | Fault | Requirement | DTC | Bytes |
|---|---|---|---|---|
| 0 | selector fault | SYS-SEL-002 | P0705-29 | 07 05 29 |
| 1 | shift fault | SYS-SFT-007 | P0780-00 | 07 80 00 |
| 2 | ratio fault | SYS-SEN-005 | P0730-00 | 07 30 00 |
| 3 | pressure fault | SYS-SEN-006 | P0840-64 | 08 40 64 |
| 4 | line pressure sensor fault | SYS-SEN-004 | P0840-1C | 08 40 1C |
| 5 | oil temperature sensor fault | SYS-SEN-004 | P0710-1C | 07 10 1C |
| 6 | shaft speed invalid | SYS-SEN-002 | P0715-29 | 07 15 29 |
| 7 | communication fault `SIM_Driver` | SYS-COM-002 | U1100-87 | D1 00 87 |
| 8 | communication fault `SIM_Speed` | SYS-COM-002 | U1101-87 | D1 01 87 |
| 9 | communication fault `SIM_Analog` | SYS-COM-002 | U1102-87 | D1 02 87 |
| 10 | bus-off | SYS-COM-006 | U0001-88 | C0 01 88 |
| 11 | oil overtemperature | SYS-FLT-006 | P0218-00 | 02 18 00 |
| 12 .. 15 | reserved (0) | - | - | - |

## 4. Sensor characteristics

Both sensors are linear, ratiometric to 5 V. The electrical valid range is
200 .. 4800 mV (`SYS-SEN-004`); between the valid range limit and the measuring
range limit the value is clamped.

| Sensor | Measuring range | Characteristic |
|---|---|---|
| Oil temperature | 500 .. 4500 mV -> -40 .. 150 degC | T [degC] = (U [mV] - 500) x 190 / 4000 - 40 |
| Line pressure | 500 .. 4500 mV -> 0 .. 25 bar | p [bar] = (U [mV] - 500) x 25 / 4000 |

## 5. Diagnostic messages

| Parameter | Value |
|---|---|
| Physical request | 0x7E1 |
| Functional request | 0x7DF (single frame only); used by the tester only for `3E 80`, every other request is physical (0x7E1) |
| Response | 0x7E9 |
| ISO-TP addressing | normal, DLC always 8 |
| Padding byte | 0xCC |
| Flow control sent by the TCU | BS = 0, STmin = 0 ms |
| Max ISO-TP message length | 4095 bytes |
| N_Bs / N_Cr timeouts | 1000 ms |

The same identifiers and parameters are used by the application, the PBL and the SBL.

## 6. Timeout handling and substitute values (TCU)

| Message | On timeout (`SYS-COM-002`) |
|---|---|
| `SIM_Driver` | Selector = invalid (-> `SYS-SEL-002`), Brake = released, Throttle = 0 %; communication fault (bit 7) -> limp-home |
| `SIM_Speed` | both speeds invalid; communication fault (bit 8) -> limp-home; the shaft speed invalid fault (bit 6) is not set |
| `SIM_Analog` | oil temperature substitute value 80 degC (`SYS-FLT-005`); line pressure not available, pressure plausibility suspended; communication fault (bit 9) -> limp-home (fault lamp ON); the sensor faults (bits 4, 5) are not set |

The substitute values also apply from the start of the application until the first reception
of the message (`SYS-COM-002`).

Changed on 2026-10-17 (user, system design findings F-12, F-14, F-16): fault bits per
timeout named; a `SIM_Analog` timeout now enters limp-home like a line pressure sensor fault
(`SYS-FLT-002`); functional addressing only for `3E 80`.

Changed on 2026-10-17 (user, system design findings N-02, N-03): `TCU_Status.SelectorState`
is 7 while the selector fault is set; `TCU_Status.OilTemp` is 0xFF while the substitute oil
temperature is used.
