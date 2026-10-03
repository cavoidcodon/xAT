# xAT architecture overview

Simplified automatic transmission control unit (TCU) on FRDM-MCXN947, AUTOSAR Classic
R21-11, single core.

## Layers

```
+------------------------------------------------------------------+
| Application (src/app)  SwcGearSelector SwcVehicleSpeed           |
|                        SwcShiftCtrl    SwcSolenoidCtrl           |
+------------------------------------------------------------------+
| RTE (src/rte)  handwritten, no control logic                     |
+------------------------------------------------------------------+
| BSW services: Det EcuM SchM   | ECU abstraction: IoHwAb | OS     |
+------------------------------------------------------------------+
| MCAL (src/mcal): Mcu Port Dio Gpt Adc Pwm Icu Wdg                |
+------------------------------------------------------------------+
| NXP MCUXpresso SDK drivers (fsl_*)  -  read-only                 |
+------------------------------------------------------------------+
```

Dependency rules: `.claude/rules/layering.md`.

## Module list

| Module | Layer | Status | Design notes |
|---|---|---|---|
| Std_Types / Platform_Types / Compiler | platform | TODO | — |
| Det | bsw/services | TODO | — |
| Mcu | mcal | TODO | — |
| Port | mcal | TODO | — |
| Dio | mcal | TODO | — |
| Gpt | mcal | TODO | — |
| Os / SchM | os, bsw/services | TODO | — |
| EcuM | bsw/services | TODO | — |
| Adc | mcal | TODO | — |
| Pwm | mcal | TODO | — |
| Icu | mcal | TODO | — |
| IoHwAb | bsw/ecual | TODO | — |
| Rte | rte | TODO | — |
| SWCs | app | TODO | — |
| Wdg | mcal | TODO | — |

## Signal flow (to be detailed)

<!-- sensors -> MCAL -> IoHwAb -> RTE ports -> SWCs -> RTE -> IoHwAb -> MCAL -> actuators.
     List every RTE port here: name, data type, unit, scaling, producer, consumer, period. -->

| RTE port | Type | Unit / scaling | Producer | Consumer | Period |
|---|---|---|---|---|---|

## Scheduling (to be detailed)

| OS task | Period | Runnables / main functions |
|---|---|---|
