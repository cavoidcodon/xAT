# Reprogramming over UDS

| Field    | Value |
|----------|-------|
| Scope    | Application -> PBL -> SBL hand-over, UDS sequence, memory map, image formats |
| Parent   | `system_requirements.md` (`SYS-DIA`, `SYS-BOOT`) |
| Version  | 0.4   |
| Status   | Approved |

## 1. Software parts

| Part | Location | Updated by | Contains |
|---|---|---|---|
| PBL (primary bootloader) | flash, from 0x0000_0000 | debugger only | start-up decision (section 9), application check, CAN + ISO-TP + minimal UDS, download of the SBL into RAM. **Does not call the flash API.** |
| SBL (secondary bootloader) | downloaded into RAM for each programming session | tester (each time) | CAN + ISO-TP + UDS, calls of the flash API, erase / write / verify of the application and boot info |
| Application | flash, application area | tester via SBL | AUTOSAR stack incl. `CanTp` + `Dcm`; enters the bootloader on `10 02` |

The PBL hands its state (programming session active, security unlocked, CAN
configuration) to the SBL in a RAM structure, so the SBL does not repeat `27`.

The MCXN947 flash erase / program functions are a **ROM API**
(`devices/MCX/MCXN/MCXN947/drivers/romapi/flash/fsl_flash.h` in the SDK). They exist in
every image's address space; `SYS-BOOT-004` requires that only the SBL calls them.

## 2. Programming sequence

| # | Running | Request | Positive response | Notes |
|---|---|---|---|---|
| 1 | App | `10 03` | `50 03 ...` | extended session |
| 2 | App | `31 01 02 03` | `71 01 02 03 00` | checkProgrammingPreconditions (`SYS-DIA-004`) |
| 3 | App | `85 02` | `C5 02` | DTC setting off |
| 4 | App | `28 01 01` | `68 01` | enableRxAndDisableTx: stop `TCU_Actuators` / `TCU_Status`, reception continues (`SYS-COM-005`) |
| 5 | App -> PBL | `10 02`, repeated every 5 ms after the first `50 02` | `50 02 ...` (sent by the app), then `50 02 ...` (sent by the PBL) | the app resets (`SYS-BOOT-014`); the PBL answers in the boot window (`SYS-BOOT-011`, section 9) |
| 6 | PBL | `27 11` / `27 12 <key>` | `67 11 <seed>` / `67 12` | programming security level |
| 7 | PBL | `34 00 44 0400_0000 <size>` | `74 20 <maxBlockLen>` | download SBL to the SBL load area in SRAMX (section 5) |
| 8 | PBL | `36 <bsc> <data>` ... | `76 <bsc>` | |
| 9 | PBL | `37` | `77` | |
| 10 | PBL | `31 01 02 02 <crc32>` | `71 01 02 02 00` | checkMemory of the SBL |
| 11 | PBL -> SBL | `31 01 03 01` | `71 01 03 01 00` (sent by the SBL) | activateSecondaryBootloader |
| 12 | SBL | `2E F1 5A <fingerprint>` | `6E F1 5A` | write fingerprint to boot info |
| 13 | SBL | `31 01 FF 00 44 <addr> <size>` | `71 01 FF 00 00` | eraseMemory; `7F 31 78` while busy |
| 14 | SBL | `34 00 44 <addr> <size>` | `74 20 <maxBlockLen>` | download application |
| 15 | SBL | `36 <bsc> <data>` ... | `76 <bsc>` | |
| 16 | SBL | `37` | `77` | |
| 17 | SBL | `31 01 02 02 <crc32>` | `71 01 02 02 00` | checkMemory of the application |
| 18 | SBL | `31 01 FF 01` | `71 01 FF 01 00` | checkProgrammingDependencies: header, HW id, versions |
| 19 | SBL | `11 01` | `51 01` | reset; PBL checks the application (`SYS-BOOT-001`) and starts it |

`3E 80` (tester present, no response) is sent every 2 s during the whole sequence.

**Entry with an invalid application.** When the application is not valid, the PBL runs in
the default session (`SYS-BOOT-002`) and steps 1 .. 5 do not apply. The tester first reads
`22 F1 81`: the PBL answers `62 F1 81 FF FF FF FF` (section 4), the application answers its
version. On `FF FF FF FF` the tester sends `10 02` once; the PBL answers `50 02 ...` without
a reset and the tester continues at step 6. Added on 2026-10-17 (user, system design
finding F-07).

## 3. Sessions and security

| Session | App | PBL | SBL |
|---|---|---|---|
| 01 default | yes | yes | - |
| 02 programming | `10 02` only from 03 (else NRC `7E`) and only if `SYS-DIA-004` holds | yes, from 01 (no reset) or in the boot window | yes (always) |
| 03 extended | yes | - | - |

Services per software part and session (`x` = supported; S = needs the security level
unlocked):

| Service | App 01 | App 03 | PBL 01 | PBL 02 | SBL 02 |
|---|---|---|---|---|---|
| `10` DiagnosticSessionControl | 01, 03 | 01, 02, 03 | 01, 02 | 01, 02 | 01, 02 |
| `11 01` ECUReset | x | x | x | x | x |
| `14` ClearDiagnosticInformation | x | x | - | - | - |
| `19 01` / `19 02` ReadDTCInformation | x | x | - | - | - |
| `22` ReadDataByIdentifier (section 4) | x | x | x | x | x |
| `27` SecurityAccess | - | 01 / 02 | - | 11 / 12 | - (state handed over) |
| `28` CommunicationControl | - | x | - | - | - |
| `2E F1 5A` WriteDataByIdentifier | - | - | - | - | S |
| `31 01` RoutineControl | - | 0203 | - | 0202, 0301 (S) | 0202, FF00, FF01 (S) |
| `34` / `36` / `37` download | - | - | - | S | S |
| `3E` TesterPresent | x | x | x | x | x |
| `85` ControlDTCSetting | - | x | - | - | - |

- A request that is not supported is answered with the ISO 14229-1 NRC `11`
  (serviceNotSupported), `12` (subFunctionNotSupported), `7F` / `7E` (service /
  sub-function not supported in the active session) or `33` (securityAccessDenied).
- `10 01` in the programming session of the PBL or the SBL: positive response `50 01 ...`,
  then reset, as for `11 01`; the PBL start-up decision (section 9) then applies.
- Failed security access, both levels: after 3 failed `27` key attempts, the third one is
  answered with NRC `36` and every `27` within the next 10 s with NRC `37` (`SYS-BOOT-005`
  for level 0x11, the same rule for level 0x01).

Added on 2026-10-17 (user, system design findings F-06, F-17).

| Level | `27` sub-functions | Where | Unlocks |
|---|---|---|---|
| 0x01 | `27 01` / `27 02` | App, extended session | reserved for later writes (`2E`), nothing in v1 |
| 0x11 | `27 11` / `27 12` | PBL, programming session | `34` / `36` / `37` / `31` in PBL; state handed to SBL |

**Security algorithm v1 (educational, not secure):** 4-byte seed,
`key = rotl32(seed, 7) XOR 0x5A3C_C3A5`. The constants live in the bootloader
`config/`. A real algorithm (e.g. CMAC or signature-based) is a later change.
The seed is the value of a free-running 32-bit timer captured when the seed request
arrives; a captured value of 0 is replaced by 1, because seed 0 means "already unlocked".
The TRNG is not used.

## 4. Data identifiers

| DID | Name | Length | App | PBL | SBL | Access |
|---|---|---|---|---|---|---|
| 0xF180 | PBL software version | 4 | R | R | R | major, minor, patch, 0 |
| 0xF181 | Application software version | 4 | R | R | R | from the application header; 0xFFFFFFFF if invalid |
| 0xF186 | Active diagnostic session | 1 | R | R | R | |
| 0xF187 | ECU part number | 16 | R | R | R | ASCII `xAT-TCU` padded with spaces |
| 0xF18C | ECU serial number | 16 | R | R | R | MCU unique ID |
| 0xF15A | Fingerprint | 16 | R | R | R / W | tester id (2), date YYMMDD BCD (3), reserved (11) |
| 0xFD00 | Programming counter | 2 | R | R | R | incremented by the SBL at step 12 |
| 0xFD01 | SBL software version | 4 | - | - | R | |

## 5. Memory map (MCXN947)

Flash: 2 MiB, two banks of 1 MiB, 8 KiB sectors, 16-byte phrase (from the SDK
`MCXN947_cm33_core0_features.h`). Single core, so the SDK default `m_core1_image` region
at 0x000C_0000 is not used.

| Region | Start | End | Size | Erased / written by |
|---|---|---|---|---|
| PBL | 0x0000_0000 | 0x0000_FFFF | 64 KiB | debugger only |
| Boot info A | 0x0001_0000 | 0x0001_1FFF | 8 KiB | SBL |
| Boot info B | 0x0001_2000 | 0x0001_3FFF | 8 KiB | SBL |
| Application header | 0x0001_4000 | 0x0001_43FF | 1 KiB | SBL |
| Application (vector table first) | 0x0001_4400 | 0x000F_FFFF | 943 KiB | SBL |
| Bank 1 (reserved: NvM / Fee, A/B images later) | 0x0010_0000 | 0x001F_FFFF | 1 MiB | - |

| RAM region | Address | Size | Use |
|---|---|---|---|
| SBL load area (SRAMX) | 0x0400_0000 .. 0x0400_FFFF | 64 KiB | SBL image: vector table, code, initialized data |
| SBL bss and stack (SRAMX) | 0x0401_0000 .. 0x0401_7FFF | 32 KiB | SBL zero-initialized data and stack |
| PBL / application RAM | 0x2000_0000 .. 0x2006_7FFF | 416 KiB | data, bss, heap and stack of the PBL and of the application |

Notes:
- RAM layout from the SDK linker scripts: SRAMX (code bus) 0x0400_0000, 96 KiB, is the
  SDK region for code executed from RAM (`MCXN947_cm33_core0_ram.ld`); system RAM
  0x2000_0000 .. 0x2004_DFFF is core0 `m_data`, 0x2004_E000 .. 0x2006_7FFF is core1
  `m_data` and is free on this single-core design.
- The PBL uses only system RAM, so it can write the SBL into SRAMX with `36` without
  touching its own data and stack. The SBL runs entirely from SRAMX (RB-04) and does not
  use system RAM.
- If data access to SRAMX over the code bus is too slow for the SBL, its bss and stack may
  move to 0x2006_0000 .. 0x2006_7FFF; the PBL must then not use that range.
- The application vector table is at 0x0001_4400, aligned to 1 KiB because the
  MCXN947 vector table is 0x400 bytes (SDK linker script `m_interrupts`).
- No RAM content is kept across a reset (no no-init area). The reference manual is not
  available, so whether the boot ROM or a reset clears RAM is unknown; the design does not
  depend on it.

## 6. Image header

The application image starts with a 1 KiB header; the SBL image uses the same format with
another magic value. A post-build script fills `image_size`, `image_crc32` and
`header_crc32`.

| Offset | Field | Type | Value |
|---|---|---|---|
| 0x00 | magic | uint32 | `0x41544178` ("xATA", application) / `0x53544178` ("xATS", SBL) |
| 0x04 | header_version | uint16 | 1 |
| 0x06 | header_size | uint16 | 64 (bytes covered by `header_crc32`) |
| 0x08 | sw_version | uint32 | major << 16 \| minor << 8 \| patch |
| 0x0C | hw_id | uint32 | 0x0947_0001 (MCXN947 TCU); checked by `FF01` |
| 0x10 | image_start | uint32 | address of the vector table (0x0001_4400) |
| 0x14 | image_size | uint32 | bytes from `image_start` |
| 0x18 | image_crc32 | uint32 | CRC-32/ISO-HDLC over `image_size` bytes |
| 0x1C | min_pbl_version | uint32 | lowest PBL version this image accepts |
| 0x20 .. 0x3B | reserved | - | 0xFF |
| 0x3C | header_crc32 | uint32 | CRC-32/ISO-HDLC over bytes 0x00 .. 0x3B |
| 0x40 .. 0x3FF | padding | - | 0xFF |

## 7. Boot info record

Two 8 KiB sectors (A / B) hold the record alternately. The SBL writes the slot that is not
the newest, and the PBL uses the newest slot with a valid CRC, so a power loss while
writing keeps the previous record.

| Field | Type | Notes |
|---|---|---|
| magic | uint32 | "xATB" |
| sequence | uint32 | incremented by 1 at each write |
| programming_counter | uint16 | DID 0xFD00 |
| fingerprint | uint8[16] | DID 0xF15A |
| record_crc32 | uint32 | over the fields above |

The application validity is **not** stored here: the PBL checks the application header
and CRC at every start (`SYS-BOOT-001`).

## 8. RAM boot record (removed)

Removed on 2026-10-17 (user) together with `SYS-BOOT-003` and `SYS-BOOT-012`. No state is
passed between the application, PBL and SBL in RAM. The section number is kept so that
the later section numbers do not change.

## 9. PBL start-up decision

Evaluated once after every reset, in this order:

| # | Condition | Action |
|---|---|---|
| 1 | Application check fails (section 10) | stay in the bootloader, default session (`SYS-BOOT-002`) |
| 2 | Boot window 50 ms: `10 02` received on 0x7E1 | programming session, send `50 02` (`SYS-BOOT-011`) |
| 3 | otherwise | de-initialize CAN and clocks used by the PBL, set VTOR, jump to the application |

The tester's entry procedure: send `10 02` on 0x7E1 every 5 ms until the PBL answers
`50 02`. The application does not respond between its own `50 02` and its reset
(`SYS-BOOT-014`), so the next `50 02` is the PBL's. When the PBL already runs in the default
session (invalid application), a single `10 02` is enough (section 2, entry with an invalid
application). Normal entry: start after the application's `50 02` (`SYS-BOOT-014`). Recovery
(used by `SIM-TST-002`): start, then reset the ECU (power cycle, or `11 01` if the
application still answers).

## 10. Robustness rules

| ID | Rule | Reason |
|---|---|---|
| RB-01 | RAM ECC configuration (`SYSCON->ECC_ENABLE_CTRL`) is identical in application, PBL and SBL. Default: disabled, as the SDK `SystemInit()` does. | Data written with one ECC setting and read with another produces errors; with ECC disabled no RAM initialization is needed after reset. |
| RB-02 | The PBL application check sets `SYSCON->NVM_CTRL.DIS_MBECC_ERR_DATA = 1` while it reads the application, restores it afterwards, and checks the header (magic, `header_crc32`, `image_size` within the application area) before it reads the image. A BusFault / HardFault during the check is reported as "application invalid", not as a reset. | `SYS-BOOT-013`: interrupted programming must not cause a fault loop. The FRDM-MCXN947 `pflash` SDK example also disables multi-bit ECC bus errors before reading programmed flash. |
| RB-03 | The SBL clears the flash cache (`SYSCON->NVM_CTRL.CLR_FLASH_CACHE`) and disables flash speculation before every `checkMemory` (`0202`) and `checkProgrammingDependencies` (`FF01`). | Avoid stale data from cache / speculation after programming. |
| RB-04 | While it erases or programs bank 0, the SBL executes **only from RAM**: code, constant data, CAN driver and vector table (VTOR in RAM). No access to bank 0 except through the flash API. | The bank being erased / programmed cannot be read; a fetch from it stalls or faults. |
| RB-05 | During long erase operations the SBL sends `7F 31 78` at least every P2* - 500 ms and keeps processing CAN between sector erases. | Keep the tester session alive. |
| RB-06 | An application image flashed with a debugger must also contain a valid header (post-build step), and debugger flashing must never mass-erase the device. | The PBL starts only images with a valid header; a mass erase removes the PBL. |

## 11. Routines

| RID | Name | Where | Parameters | Result |
|---|---|---|---|---|
| 0x0203 | checkProgrammingPreconditions | App | - | 0 = OK, else bit mask: bit 0 not at standstill, bit 1 not in P, bit 2 speed invalid |
| 0x0202 | checkMemory | PBL, SBL | CRC32 (4) of the last download | 0 = OK, 1 = CRC mismatch |
| 0x0301 | activateSecondaryBootloader | PBL | - | 0 = OK (answered by the SBL) |
| 0xFF00 | eraseMemory | SBL | `44`, address (4), size (4); sector-aligned | 0 = OK, 1 = failed |
| 0xFF01 | checkProgrammingDependencies | SBL | - | 0 = OK, 1 = header invalid, 2 = hw_id mismatch, 3 = PBL version too old |

`min_pbl_version` and the PBL version (DID 0xF180) are compared as
major << 16 | minor << 8 | patch. After a CRC mismatch of the application at `0202` or a
failed `FF01`, the SBL erases the sector that holds the application header before it
responds (`SYS-BOOT-015`).

### 11.1 Request sequence and address rules

| Request | Refused when | NRC |
|---|---|---|
| `34` in the PBL | the range start .. start + size - 1 is not inside the SBL load area 0x0400_0000 .. 0x0400_FFFF | `31` |
| `34`, `31 01 FF 00` in the SBL | the range is not inside the application area (0x0001_4000 .. 0x000F_FFFF) or the boot info area (0x0001_0000 .. 0x0001_3FFF); eraseMemory also when not sector-aligned | `31` (`SYS-BOOT-006`) |
| `36` | no download accepted by `34`, or the transfer is already complete | `24` |
| `36` | wrong block sequence counter | `73` |
| `37` | no download accepted by `34` | `24` |
| `31 01 02 02` | no download completed with `37` since the last `34` | `24` |
| `31 01 03 01` | the last `31 01 02 02` of the SBL did not return 0 (OK) | `24` |
| `31 01 03 01` | the SBL image header is invalid (magic `0x53544178` "xATS", `header_crc32`) | `22` |
| `31 01 FF 01` | the last `31 01 02 02` of the application did not return 0 (OK) | `24` |

Added on 2026-10-17 (user, system design finding F-05).

## 12. Transfer parameters and timing

| Parameter | Value |
|---|---|
| dataFormatIdentifier (`34`) | 0x00 (no compression, no encryption) |
| addressAndLengthFormatIdentifier | 0x44 |
| maxNumberOfBlockLength | 0x0402 (1024 data bytes + SID + block sequence counter) |
| Write granularity | 16-byte phrase; the last block is padded with 0xFF by the tester |
| Erase granularity | 8 KiB sector |
| P2server / P2*server | 50 ms / 5000 ms |
| S3server | 5000 ms |

## 13. Open questions

| # | Question | Affects | Answer |
|---|---|---|---|
| 1 | SBL load address | section 5 | SBL in SRAMX: load area 0x0400_0000, 64 KiB; bss and stack 0x0401_0000, 32 KiB. Whether RAM survives a reset is not checked (user decision 2026-10-04); no RAM content is kept across a reset (section 8 removed). Closed (user, 2026-10-17). |
| 2 | PBL size 64 KiB enough? (CAN + ISO-TP + UDS + CRC, no flash driver) | section 5 | Yes. Measured 2026-10-17 (GCC 15.2, `-Os`, `--gc-sections`, newlib-nano): startup, vector table, SDK FlexCAN, clock, reset and CRC drivers with a minimal CAN echo loop = 7.2 KiB flash, 3 KiB RAM (2 KiB stack, 1 KiB heap). Estimated additions: ISO-TP 3 - 4 KiB, UDS server (`10`, `11`, `22`, `27`, `31`, `34` / `36` / `37`, `3E`, NRCs, P2 / S3) 6 - 8 KiB, application check with fault recovery (RB-02) 1 - 2 KiB, boot window, timer, seed / key, DIDs 1 - 2 KiB: about 20 - 25 KiB for a lean PBL, about 35 - 40 KiB if the AUTOSAR stack modules (CanIf, PduR, CanTp, Dcm, Det) are reused with their configuration tables. 64 KiB is kept; to be confirmed with the map file of the first PBL build. Closed. |
| 3 | Seed source: TRNG or timer-based? | section 3 | Timer-based (section 3). Closed (user, 2026-10-17). |
| 4 | Use the MCXN947 CRC engine or a software table CRC in the PBL? (start-up time `SYS-STA-001`) | SYS-BOOT-001 | CRC engine (`CRC0`, SDK driver `fsl_crc`): the polynomial, seed, input / output reflection and final complement are programmable, so it computes CRC-32/ISO-HDLC (poly 0x04C11DB7, seed 0xFFFF_FFFF, reflect in / out, complement). Used in the PBL and the SBL; the result must match `zlib.crc32` of the tester (unit test against a known vector). Closed (user, 2026-10-17). |
| 5 | System design findings F-01, F-04 .. F-08, F-17 (TASK-026 plan): sub-function of `28`, rejected images, sequence and address rules, services per session, entry with an invalid application, application `50 02` vs. PBL `50 02`, security lockout of level 0x01 | sections 2, 3, 9, 11 | Step 4 is `28 01 01`; section 2 entry with an invalid application; section 3 services table; section 11 header invalidation and 11.1 sequence rules. Closed (user, 2026-10-17). |
