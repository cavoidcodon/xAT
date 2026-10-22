---
paths:
  - "src/**"
---

# Layering rules

Dependency direction (an arrow means "may include / call"):

```
app (SWC) -> rte -> bsw -> mcal -> NXP SDK
                     \-> os
platform (Std_Types, Platform_Types, Compiler) <- every layer
```

| Code in | May include | Must NOT include |
|---|---|---|
| `src/app/**` | `Rte_<Swc>.h`, `Rte_Type.h`, own SWC headers | any BSW, MCAL or OS header, or NXP SDK header (`fsl_*.h`, `board.h`, `MCXN947*.h`, `fsl_device_registers.h`) |
| `src/rte/**` | BSW service / ECU abstraction headers, `Os.h`, SWC runnable prototypes | MCAL headers, NXP SDK headers |
| `src/bsw/**` | other BSW headers, MCAL headers, `Os.h`, `SchM_<Ma>.h`; a service module with AUTOSAR service ports (`BswM`, `Dcm`, `Dem`) also its own `Rte_<Ma>.h` | NXP SDK headers, SWC headers, any other RTE header |
| `src/mcal/**` | NXP SDK headers, `Det.h`, `SchM_<Ma>.h` | BSW (other than `Det`/`SchM`), RTE, SWC headers |
| `src/os/**` | NXP SDK / CMSIS core headers, `Mcu.h` | RTE, SWC headers |
| `src/ecu/**` | everything (integration: `main.c`, EcuM callouts, board port) | — |
| `src/platform/**` | nothing but C standard headers | everything else |

Additional rules:
- A layer may only skip a layer through an official AUTOSAR path (`Det`, `SchM`, `Os`).
  The application reaches BSW only through RTE ports. The RTE maps them to `Com` signals,
  to `IoHwAb` signals or to BSW services (`BswM`, `Dcm`, `Dem`, CDDs). `IoHwAb` serves the
  board I/O today (red LED); a later hardware I/O variant adds the sensor and actuator
  signals to it and maps those ports to `IoHwAb` instead of `Com` (DC-01 in
  `docs/specs/system/system_context.md`). The port list is in
  `docs/architecture/swc_composition.md`.
- Only the integration layer (`src/ecu/`) sees the whole system. Modules must not know
  the board; board facts come from `config/`.
- Configuration values (pins, channels, timings, calibrations) belong in each module's
  `config/`, never in `src/`.
- Do not modify vendor SDK files (`workspace/mcuxsdk`) unless explicitly approved.
