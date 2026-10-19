# TASK-028: `IoHwAb` - I/O hardware abstraction for board I/O (red LED), extensible to sensors and actuators

| Field        | Value                                                                    |
|--------------|--------------------------------------------------------------------------|
| Priority     | P1 (needed by `Rte` TASK-017 for the fault lamp / red LED)               |
| Depends on   | TASK-007 (`Dio`), TASK-027 (list of `IoHwAb` signals)                     |
| Module(s)    | `IoHwAb` (`src/bsw/ecual/IoHwAb`)                                        |
| AUTOSAR doc  | `AUTOSAR_SWS_IOHardwareAbstraction` R21-11                               |

---

## 0. Plan contents (task-specific)

Run with the `module-task` skill, after `module-design IoHwAb bsw/ecual` and the
`new-module` scaffold. The plan must also contain:
- the `IoHwAb` signal list taken from TASK-027 (name, direction, type, init / safe value,
  `Dio` channel, polarity);
- the API shape and why it stays unchanged when the later hardware I/O variant adds
  analog / PWM / ICU signals (section 1);
- which SWS requirements apply to an ECU-specific module like this one, and which are left
  out with the reason (the SWS standardises the structure, not the signal APIs).

## 1. Goal

`IoHwAb` exists and drives the **red LED** of the board for the fault lamp
(`SYS-HMI-001`), so that the SWC reaches the LED only through an RTE port that the RTE maps
to `IoHwAb` (decision of TASK-027, open question 4). SWCs never see the pin, the port or the
active-low polarity.

The module is built so that it **can be extended without API change**: a configuration
table of ECU signals (signal ID -> MCAL channel, direction, polarity, init value), one
access function per signal class (e.g. digital output now; digital input, analog input and
PWM output later), and a place for the physical-to-electrical conversion that belongs in
`IoHwAb`. The later hardware I/O variant (DC-01) then adds signals and MCAL drivers
(`Adc`, `Pwm`, `Icu`) to this module, and only the RTE mapping of the sensor / actuator
ports changes from `Com` to `IoHwAb`.

## 2. Context (verified on 2026-10-18)

- **Red LED:** FRDM-MCXN947 `GPIO0` pin 10, **active low** (SDK
  `examples/_boards/frdmmcxn947/board.h`: `BOARD_LED_RED_GPIO`, `BOARD_LED_RED_GPIO_PIN`,
  `LOGIC_LED_ON 0U`). Pin mux comes from `Port` (TASK-006), the channel from `Dio`
  (TASK-007).
- **Requirement:** `SYS-HMI-001` - the red LED follows `FaultLamp`; `SYS-FLT-003` - LED ON
  within 100 ms also while in bus-off (the LED path does not depend on CAN).
- **Current docs:** `CLAUDE.md`, `layering.md` and `overview.md` describe `IoHwAb` as
  "later hardware variant only"; TASK-027 updates that statement.
- **SWS:** `docs/references/autosar/` holds no PDF yet (git-ignored);
  `AUTOSAR_SWS_IOHardwareAbstraction.pdf` must be downloaded before `module-design`.
- **Layering:** `src/bsw/ecual/` may include MCAL headers, `Det.h`, `SchM_IoHwAb.h`; never
  NXP SDK headers or RTE / SWC headers.

## 3. Scope

### In scope
1. `docs/modules/IoHwAb/design.md` and diagrams (`module-design`).
2. `src/bsw/ecual/IoHwAb/` (`include/`, `src/`, `config/`): `IoHwAb_Init`,
   `IoHwAb_GetVersionInfo`, the digital output access for the red LED signal, `Det`
   checks; configuration table with the red LED signal.
3. Host unit tests with `Dio` mocked (CMock).
4. Call of `IoHwAb_Init` in the EcuM driver init list (or a note for TASK-010 if EcuM does
   not exist yet).
5. On-board check: LED ON / OFF through `IoHwAb` (debug build or target test).

### Out of scope
- Analog, PWM and capture signals and the `Adc` / `Pwm` / `Icu` drivers (later hardware
  variant).
- The RTE port and its mapping (TASK-017); the fault lamp logic (fault / mode manager SWC,
  TASK-018).

## 4. Files (expected)

| File | Action |
|---|---|
| `docs/modules/IoHwAb/design.md`, `docs/modules/IoHwAb/*.puml` | add |
| `src/bsw/ecual/IoHwAb/include/IoHwAb.h` | add |
| `src/bsw/ecual/IoHwAb/src/IoHwAb.c` | add |
| `src/bsw/ecual/IoHwAb/config/IoHwAb_Cfg.h`, `IoHwAb_Cfg.c` | add |
| `tests/unit/bsw/ecual/IoHwAb/test_IoHwAb.c` | add |
| `docs/autosar_compliance.md`, `docs/architecture/overview.md` (design-notes link) | change |

## 5. Requirements

| ID | Requirement | Parent (SYS / SWR) |
|---|---|---|
| SWS | SWS IDs from `AUTOSAR_SWS_IOHardwareAbstraction` R21-11, selected in the plan | - |
| REQ-028-01 | `IoHwAb` sets the red LED to the commanded logical value (ON / OFF); the active-low polarity is a configuration value, not code. | SYS-HMI-001 |
| REQ-028-02 | After `IoHwAb_Init` the red LED is OFF (configured init value) until the first command. | SYS-HMI-001, SYS-ACT-004 |
| REQ-028-03 | Adding a signal of an existing class needs only a configuration entry; adding a signal class adds an access function without changing the existing ones. | DC-01 |
| REQ-028-04 | Invalid signal ID or call before init: `Det` error, no `Dio` access. | SWS (general BSW) |

## 6. Acceptance criteria

| AC | Criterion | How verified |
|---|---|---|
| AC0 | Plan approved before any change | — |
| AC1 | Firmware builds without warnings | `ops build --clean` |
| AC2 | Unit tests pass, every API covered | `ctest --test-dir build/unit` |
| AC3 | No layering violation | `verify` skill step 3 |
| AC4 | Red LED ON / OFF on the board through `IoHwAb` | target check |
| AC5 | Docs updated (`docs/modules/IoHwAb/design.md`, compliance matrix, overview, development log) | review |

## 7. Open questions

1. API style: per-signal functions (e.g. `IoHwAb_Set_RedLed(value)`) generated by hand
   from the config, or generic class functions with a signal ID
   (`IoHwAb_DigitalWrite(IoHwAb_SignalType, value)`)? To be proposed in `module-design`.
2. Does the RTE call `IoHwAb` directly (S/R port mapped to a function) or through a C/S
   port to an `IoHwAb` service component? Decided together with TASK-017.
