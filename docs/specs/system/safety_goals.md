# Safety goals

| Field    | Value |
|----------|-------|
| Scope    | xAT system |
| Version  | 0.1   |
| Status   | Approved |

These safety goals give the requirements a structure and a reason. xAT is an educational
project: there is no hazard analysis and risk assessment (HARA), no ASIL and no claim of
ISO 26262 compliance.

| ID | Safety goal | Hazard it addresses | Safe state |
|---|---|---|---|
| SG-01 | Avoid engagement of reverse while the vehicle moves forward. | drivetrain damage, loss of vehicle control | Neutral (reverse locked out) |
| SG-02 | Avoid a downshift that drives the engine / input shaft above its maximum speed. | engine overspeed, sudden engine braking, wheel lock on low grip | Keep the current gear |
| SG-03 | Avoid unintended drive torque while P or N is selected, or when the selector position is unknown. | unintended vehicle motion | Neutral |
| SG-04 | When a fault is detected, keep the vehicle drivable in a defined gear and warn the driver. | loss of drive without warning | Limp-home (3rd in D, maximum line pressure, fault lamp ON) |
| SG-05 | Avoid reprogramming of the TCU while the vehicle is being driven. | loss of transmission control while moving | Reject the programming request |
| SG-06 | Never execute software that is corrupted, incomplete or not accepted by the bootloader. | undefined actuator behaviour | Stay in the bootloader with outputs in the hydraulic default |

The requirements that implement each goal are listed in `../traceability.md`.
