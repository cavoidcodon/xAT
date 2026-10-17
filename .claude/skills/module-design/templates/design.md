# <Ma> - design

| Field | Value |
|---|---|
| Layer | <LAYER> |
| AUTOSAR document | <SWS_DOC> (R21-11) |
| Module ID | <MODULE_ID> |
| Status | Design (draft) |

## Purpose in xAT

<!-- Why the transmission ECU needs this module; which HW resources or PDUs / signals it
     handles. -->

## Derived requirements

<!-- What the system needs from this module (subset, values), e.g. "Com deadline
     monitoring 30 ms for SIM_Driver". The link to the parent SYS / SWR requirement and to
     the tests is kept only in docs/specs/traceability.md section 2. -->

-

## Scope

### Supported API

| API | Service ID | Reason (xAT need) | SWS IDs |
|---|---|---|---|

### Not supported

| API / feature / container | Reason |
|---|---|

## Interfaces

Diagram: [component.puml](component.puml)

| Direction | Module | Interface (functions / callbacks) | Notes |
|---|---|---|---|
| provided to | | | |
| required from | | | |

## Behaviour

<!-- One subsection per scenario, with a short description of the steps and the link to its
     diagram. -->

### Initialisation

Diagram: [seq_init.puml](seq_init.puml)

## State

Diagram: [state.puml](state.puml)

| State | Meaning | Entered by | Left by |
|---|---|---|---|

## Configuration

Diagram: [config.puml](config.puml)

| Container / parameter | Class (pre-compile / link / post-build) | xAT value or source | SWS ID |
|---|---|---|---|

## Hardware mapping (FRDM-MCXN947)

<!-- MCAL only: peripheral instances, pins, interrupts, NXP SDK driver functions used.
     Other layers: "None". -->

## Error handling

| Error | Kind (development / runtime / production) | Raised when | SWS ID |
|---|---|---|---|

## Reentrancy and exclusive areas

<!-- Which APIs are reentrant per SWS, which data is shared with ISRs or the main function,
     and the SchM exclusive areas planned. -->

## Deviations from the SWS

| SWS requirement | Deviation | Reason |
|---|---|---|

## Open questions and assumptions

-

SWS requirement -> code -> test: see `docs/autosar_compliance.md`.
