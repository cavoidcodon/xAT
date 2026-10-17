# Traceability

| Field    | Value |
|----------|-------|
| Version  | 0.2   |
| Status   | Approved |

## 1. Safety goals -> system requirements

| Safety goal | System requirements |
|---|---|
| SG-01 reverse while moving forward | SYS-SEL-004 |
| SG-02 downshift overspeed | SYS-SEL-005, SYS-SFT-003, SYS-SFT-005, SYS-SFT-006 |
| SG-03 drive torque in P / N / unknown | SYS-SEL-002, SYS-SEL-003, SYS-SEL-005 |
| SG-04 limp-home and warning | SYS-ACT-004, SYS-SEN-004, SYS-SEN-005, SYS-SEN-006, SYS-SFT-007, SYS-FLT-001 .. SYS-FLT-005, SYS-COM-002, SYS-COM-003, SYS-COM-006, SYS-STA-001, SYS-STA-002, SYS-HMI-001, SYS-BOOT-008 |
| SG-05 no reprogramming while driving | SYS-DIA-004, SYS-BOOT-014 |
| SG-06 no corrupted software | SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-004 .. SYS-BOOT-007, SYS-BOOT-011, SYS-BOOT-013 |

## 2. System requirements -> software / modules / tests

Filled when the SWC specifications (`software/`) and module tasks are written.

| SYS | SWR / module | Test | Status |
|---|---|---|---|
| | | | |

## 3. Simulation environment -> system requirements

| SIM | Supports verification of |
|---|---|
| SIM-PLT-004 | SYS-ACT-004, SYS-BOOT-008, SYS-STA-002 |
| SIM-PLT-005 | SYS-SEL-003 |
| SIM-CON-003 | SYS-SEL-002, SYS-SEN-004 .. SYS-SEN-006, SYS-SFT-007, SYS-COM-002, SYS-COM-003, SYS-FLT-* |
| SIM-TST-001 | SYS-BOOT-001, SYS-BOOT-002, SYS-BOOT-004 .. SYS-BOOT-011, SYS-BOOT-014 |
| SIM-TST-002 | SYS-DIA-004, SYS-BOOT-005 .. SYS-BOOT-007, SYS-BOOT-011, SYS-BOOT-013 |
| SIM-TST-003 | SYS-DIA-002, SYS-DIA-003, SYS-DIA-005, SYS-FLT-007 |
