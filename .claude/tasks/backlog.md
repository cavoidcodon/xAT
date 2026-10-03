# Backlog

Status: TODO / IN PROGRESS / DONE. Each module gets its own TASK file (from
`TASK_TEMPLATE.md`) when it is started.

| Task | Title | Layer | Depends on | Status |
|---|---|---|---|---|
| TASK-001 | Build / flash / debug tooling (`scripts/operations.py`) | tooling | — | DONE |
| TASK-002 | Project skeleton, Claude structure, coding rules | project | TASK-001 | DONE |
| TASK-003 | Platform types (`Std_Types`, `Platform_Types`, `Compiler`), firmware entry `src/ecu`, CMake module helper, Unity + CMock host test build | platform | TASK-002 | TODO |
| TASK-004 | `Det` | bsw/services | TASK-003 | TODO |
| TASK-005 | `Mcu` (clocks, reset reason) | mcal | TASK-004 | TODO |
| TASK-006 | `Port` | mcal | TASK-005 | TODO |
| TASK-007 | `Dio` | mcal | TASK-006 | TODO |
| TASK-008 | `Gpt` | mcal | TASK-005 | TODO |
| TASK-009 | OS / `SchM` (periodic tasks 1 / 10 / 100 ms, exclusive areas) | os, bsw/services | TASK-008 | TODO |
| TASK-010 | `EcuM` (startup / shutdown sequence) | bsw/services | TASK-009 | TODO |
| TASK-011 | `Adc` (oil temperature, line pressure) | mcal | TASK-010 | TODO |
| TASK-012 | `Pwm` (shift / pressure solenoids) | mcal | TASK-010 | TODO |
| TASK-013 | `Icu` (input / output shaft speed) | mcal | TASK-010 | TODO |
| TASK-014 | `IoHwAb` (sensor / actuator abstraction) | bsw/ecual | TASK-011..013 | TODO |
| TASK-015 | `Rte` (handwritten) | rte | TASK-014 | TODO |
| TASK-016 | SWCs: gear selector, vehicle speed, shift control, solenoid control | app | TASK-015 | TODO |
| TASK-017 | `Wdg` (+ `WdgIf`/`WdgM` decision) | mcal | TASK-010 | TODO |
| TASK-018 | Diagnostics (`Dem`), communication (`Can`/`CanIf`/`PduR`/`Com`) | bsw | TASK-016 | TODO |
