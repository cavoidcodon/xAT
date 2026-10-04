# <Ma> - design notes

| Field | Value |
|---|---|
| Layer | <LAYER> |
| AUTOSAR document | <SWS_DOC> (R21-11) |
| Module ID | <MODULE_ID> |
| Status | Scaffolded |

## Purpose in xAT

<!-- Why the transmission ECU needs this module; which HW resources it uses. -->

## Derived requirements

<!-- What the system needs from this module (subset, values), e.g. "Com deadline
     monitoring 30 ms for SIM_Driver". The link to the parent SYS / SWR requirement and to
     the tests is kept only in docs/specs/traceability.md section 2. -->

-

## Supported API

| API | Service ID | Supported | Notes |
|---|---|---|---|
| `<Ma>_Init` | 0x00 | yes | |

## Configuration

| Parameter | Kind (pre-compile / link / post-build) | Value | Meaning |
|---|---|---|---|

## Hardware mapping (FRDM-MCXN947)

<!-- Peripheral instances, pins, NXP SDK driver functions used. MCAL only. -->

## Deviations from the SWS

| SWS requirement | Deviation | Reason |
|---|---|---|

SWS requirement -> code -> test: see `docs/autosar_compliance.md`.
