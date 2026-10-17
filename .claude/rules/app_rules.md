---
paths:
  - "src/app/**"
  - "src/rte/**"
  - "tests/unit/app/**"
  - "tests/unit/rte/**"
---

# Application (SWC) and RTE rules

- Each SWC lives in `src/app/<SwcName>/` (`include/`, `src/`, `config/`) and talks to
  the rest of the system **only** through `Rte_<SwcName>.h`
  (`Rte_Read_*`, `Rte_Write_*`, `Rte_Call_*`, `Rte_IrvRead_*`, …).
- SWCs never include NXP SDK headers or BSW/MCAL headers.
- The transmission control algorithm (shift schedule, gear selection, solenoid
  control, plausibility) belongs only in SWCs. The RTE contains **no** control logic.
  It only copies data, maps ports to BSW services and calls runnables.
- Runnables have no parameters and return `void`. They are triggered by the RTE/OS on
  timing or data events and must finish within their period.
- Calibration values (shift maps, thresholds, hysteresis) belong in the SWC's
  `config/`, as `const` tables with units in their names or comments
  (`_rpm`, `_kph`, `_degC`, `_ms`).
- Use fixed-point (scaled integer) arithmetic unless a SWC design doc explicitly
  allows `float32`. Document scaling and ranges in `docs/modules/<SwcName>/design.md`.
- RTE files (`Rte.c`, `Rte_<Swc>.h`, `Rte_Type.h`) are handwritten, following
  `AUTOSAR_SWS_RTE` naming. Every port added must be listed in
  `docs/architecture/overview.md`.
