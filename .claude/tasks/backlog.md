# Backlog

Status: TODO / IN PROGRESS / DONE. This table is the only place that holds task status;
TASK files do not repeat it. Each module gets its own TASK file (from `TASK_TEMPLATE.md`)
when it is started.

SWC tasks implement only `Approved` requirements from `docs/specs/` (spec-driven); BSW and
MCAL tasks implement their AUTOSAR SWS.

| Task | Title | Layer | Depends on | Status |
|---|---|---|---|---|
| TASK-001 | Build / flash / debug tooling (`scripts/operations.py`) | tooling | — | DONE |
| TASK-003 | Platform types (`Std_Types`, `Platform_Types`, `Compiler`), firmware entry `src/ecu`, CMake module helper, Unity + CMock host test build | platform | TASK-001 | TODO |
| TASK-004 | `Det` | bsw/services | TASK-003 | TODO |
| TASK-005 | `Mcu` (clocks, reset reason) | mcal | TASK-004 | TODO |
| TASK-006 | `Port` | mcal | TASK-005 | TODO |
| TASK-007 | `Dio` | mcal | TASK-006 | TODO |
| TASK-008 | `Gpt` | mcal | TASK-005 | TODO |
| TASK-009 | OS / `SchM` (periodic tasks 1 / 10 / 100 ms, exclusive areas) | os, bsw/services | TASK-008 | TODO |
| TASK-010 | `EcuM` (startup / shutdown sequence) | bsw/services | TASK-009 | TODO |
| TASK-011 | CAN gateway firmware on FRDM-MCXN236 (`tools/gateway/`, SIM-GW) and second board in `scripts/operations.py` | tools | TASK-001 | TODO |
| TASK-012 | `Can` (FlexCAN, 500 kbit/s, J10) | mcal | TASK-010, TASK-011 | TODO |
| TASK-013 | `CanIf` | bsw/com | TASK-012 | TODO |
| TASK-014 | `PduR` | bsw/com | TASK-013 | TODO |
| TASK-015 | `Com` (signals from `can_matrix.md`) | bsw/com | TASK-014 | TODO |
| TASK-016 | Plant model and console (`tools/sim/`, SIM-PLT, SIM-CON) | tools | TASK-011 | TODO |
| TASK-017 | `Rte` (handwritten, ports mapped to `Com`, DC-01) | rte | TASK-015 | TODO |
| TASK-018 | SWCs: gear selector, vehicle speed, shift control, solenoid control (with SWR specs) | app | TASK-017, TASK-016, TASK-026 | TODO |
| TASK-019 | `Wdg` (+ `WdgIf` / `WdgM` decision) | mcal | TASK-010 | TODO |
| TASK-020 | `CanTp` | bsw/com | TASK-014 | TODO |
| TASK-021 | `Dcm` (application UDS services, entry to the bootloader on `10 02`) | bsw/services | TASK-020 | TODO |
| TASK-022 | UDS tester (`tools/sim/`, SIM-TST) | tools | TASK-011 | TODO |
| TASK-023 | PBL (`src/boot/pbl`) | boot | TASK-012, TASK-022 | TODO |
| TASK-024 | SBL (`src/boot/sbl`) | boot | TASK-023 | TODO |
| TASK-025 | `Dem` and `Dcm` services `14` / `19` (`SYS-FLT-007`; until then faults only in `TCU_Status.FaultBits`) | bsw/services | TASK-018, TASK-021 | TODO |
| TASK-026 | System design: component and sequence diagrams for all approved system scenarios (PlantUML, `docs/architecture/system/`) | docs | — | DONE |
