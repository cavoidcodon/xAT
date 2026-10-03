---
paths:
  - "src/mcal/**"
  - "tests/unit/mcal/**"
---

# MCAL rules

- MCAL is a **thin, deterministic wrapper** around the NXP SDK drivers (`fsl_*.h`), with
  the AUTOSAR API on top. Execution time must not depend on data: no unbounded loops, and
  every HW poll has a timeout.
- MCAL code is the only project code allowed to include NXP SDK headers. NXP SDK types
  (`status_t`, `GPIO_Type *`, …) must never appear in a public `<Ma>.h`.
- Map from symbolic AUTOSAR IDs (`Dio_ChannelType`, `Adc_GroupType`) to hardware
  (port/pin/instance) only in `config/<Ma>_Cfg.c`, with `const` tables.
- No floating point, no dynamic memory, no blocking delays (except the bounded waits the
  SWS requires, such as PLL lock in `Mcu`).
- Clock gating, pin muxing and resets belong to `Mcu`/`Port`. Other drivers assume their
  peripheral clock and pins are already set up.
- Each API follows the SWS: DET checks (see `coding_rules.md` §6), and the behaviour
  described in the SWS for uninitialized or invalid calls.
- Interrupt handlers live in `src/<Ma>_Irq.c`, call module-internal handlers, and invoke
  the configured notifications (`<Ma>_Cbk.h`).
- Host unit tests replace the SDK with stubs/mocks from `tests/unit/stubs/sdk/`. Keep SDK
  calls narrow so they are easy to mock.
