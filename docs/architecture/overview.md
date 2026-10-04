# xAT architecture overview

Simplified automatic transmission control unit (TCU) on FRDM-MCXN947, AUTOSAR Classic
R21-11, single core.

The required behaviour, the system context (TCU, CAN gateway, plant model on the host) and
the CAN matrix are specified in [`docs/specs/`](../specs/README.md).

## Layers

```
+------------------------------------------------------------------+
| Application (src/app)  SwcGearSelector SwcVehicleSpeed           |
|                        SwcShiftCtrl    SwcSolenoidCtrl           |
+------------------------------------------------------------------+
| RTE (src/rte)  handwritten, no control logic, ports -> Com       |
+------------------------------------------------------------------+
| Services: Det EcuM SchM Dcm | Com stack: Com PduR CanIf CanTp | OS |
+------------------------------------------------------------------+
| MCAL (src/mcal): Mcu Port Dio Gpt Can Wdg                        |
+------------------------------------------------------------------+
| NXP MCUXpresso SDK drivers (fsl_*)  -  read-only                 |
+------------------------------------------------------------------+
```

Separate images: PBL (`src/boot/pbl`, flash) and SBL (`src/boot/sbl`, RAM), see
`docs/specs/system/reprogramming.md`. `IoHwAb` (ECU abstraction) is added only for a later
hardware I/O variant (DC-01).

Dependency rules: `.claude/rules/layering.md`.

## Module list

Task and status: `.claude/tasks/backlog.md`.

| Module | Layer | Design notes |
|---|---|---|
| Std_Types / Platform_Types / Compiler | platform | — |
| Det | bsw/services | — |
| Mcu | mcal | — |
| Port | mcal | — |
| Dio | mcal | — |
| Gpt | mcal | — |
| Os / SchM | os, bsw/services | — |
| EcuM | bsw/services | — |
| Can | mcal | — |
| CanIf | bsw/com | — |
| PduR | bsw/com | — |
| Com | bsw/com | — |
| Rte | rte | — |
| SWCs | app | — |
| Wdg | mcal | — |
| CanTp | bsw/com | — |
| Dcm | bsw/services | — |
| PBL / SBL | boot | — |

## Signal flow (to be detailed)

<!-- CAN -> Can -> CanIf -> PduR -> Com -> RTE ports -> SWCs -> RTE ports -> Com -> PduR
     -> CanIf -> Can -> CAN. Diagnostics: Can -> CanIf -> PduR -> CanTp -> Dcm.
     Signals, IDs and scaling: docs/specs/system/can_matrix.md (single source, DC-03).
     List every RTE port here: name, data type, unit, scaling, producer, consumer, period. -->

| RTE port | Type | Unit / scaling | Producer | Consumer | Period |
|---|---|---|---|---|---|

## Scheduling (to be detailed)

| OS task | Period | Runnables / main functions |
|---|---|---|
