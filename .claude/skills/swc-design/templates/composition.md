# SWC composition

| Field | Value |
|---|---|
| Scope | All application SWCs in `src/app/` |
| Parent | `docs/specs/system/system_requirements.md` |
| Version | 0.1 |
| Status | Design (draft) |

Diagram: [swc_composition.puml](swc_composition.puml)

## 1. SWCs and responsibilities

| SWC | Responsibility | Not its job |
|---|---|---|
| | | |

## 2. Allocation of system requirements

### 2.1 Application

| SYS | Owning SWC | Contributing SWCs | Status of SYS |
|---|---|---|---|
| | | | |

### 2.2 Not application

| SYS | Owning module (BSW / boot / plant) | Reason |
|---|---|---|
| | | |

## 3. Ports

Signals from the CAN matrix keep the matrix name and scaling (DC-03); the SWC that converts
them to physical units is named in *Notes* (DC-02).

| Port | Interface (S/R, C/S, mode) | Data element | Type | Unit / scaling | Range | Init value | Producer | Consumer(s) | Period | Notes |
|---|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | | |

## 4. Scheduling

| OS task | Period | Runnables in execution order |
|---|---|---|
| | | |

### 4.1 End-to-end latency

| Path | From | To | Worst case | Requirement |
|---|---|---|---|---|
| Fault reaction | | | | SYS-FLT-003 |
| Shift command | | | | |

## 5. Operating modes

<!-- How Init / Normal / Limp-home (system_context.md §5) are decided and how each SWC
     learns the current mode. -->

| Mode | Decided by | Signalled through | Reaction per SWC |
|---|---|---|---|
| | | | |

## 6. Open questions and assumptions

-
