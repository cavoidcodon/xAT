# Reprogramming over UDS

| Field    | Value |
|----------|-------|
| Scope    | Application -> PBL -> SBL hand-over, UDS sequence, memory map, image formats |
| Parent   | `system_requirements.md` (`SYS-DIA`, `SYS-BOOT`) |
| Version  | 0.2   |
| Status   | Draft |

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
| 4 | App | `28 03 01` | `68 03` | stop `TCU_Actuators` / `TCU_Status` (`SYS-COM-005`) |
| 5 | App -> PBL | `10 02` | `50 02 ...` (sent by the PBL) | request stored in no-init RAM, reset (`SYS-BOOT-003`) |
| 6 | PBL | `27 11` / `27 12 <key>` | `67 11 <seed>` / `67 12` | programming security level |
| 7 | PBL | `34 00 44 <addr> <size>` | `74 20 <maxBlockLen>` | download SBL to RAM |
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

`3E 80` (tester present, no response) is sent every [NC: 2] s during the whole sequence.

## 3. Sessions and security

| Session | App | PBL | SBL |
|---|---|---|---|
| 01 default | yes | yes | - |
| 02 programming | `10 02` only from 03 (else NRC `7E`) and only if `SYS-DIA-004` holds | yes | yes (always) |
| 03 extended | yes | - | - |

| Level | `27` sub-functions | Where | Unlocks |
|---|---|---|---|
| 0x01 | `27 01` / `27 02` | App, extended session | reserved for later writes (`2E`), nothing in v1 |
| 0x11 | `27 11` / `27 12` | PBL, programming session | `34` / `36` / `37` / `31` in PBL; state handed to SBL |

**Security algorithm v1 (educational, not secure):** 4-byte seed,
`key = rotl32(seed, [NC: 7]) XOR [NC: 0x5A3C_C3A5]`. The constants live in the bootloader
`config/`. A real algorithm (e.g. CMAC or signature-based) is a later change.

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
| PBL | 0x0000_0000 | 0x0000_FFFF | [NC: 64 KiB] | debugger only |
| Boot info A | 0x0001_0000 | 0x0001_1FFF | 8 KiB | SBL |
| Boot info B | 0x0001_2000 | 0x0001_3FFF | 8 KiB | SBL |
| Application header | 0x0001_4000 | 0x0001_43FF | 1 KiB | SBL |
| Application (vector table first) | 0x0001_4400 | 0x000F_FFFF | 943 KiB | SBL |
| Bank 1 (reserved: NvM / Fee, A/B images later) | 0x0010_0000 | 0x001F_FFFF | 1 MiB | - |

| RAM region | Address | Size | Use |
|---|---|---|---|
| SBL load area | [NEEDS CLARIFICATION: address] | [NC: 64 KiB] | SBL code and data |
| No-init area | [NC: last 32 bytes of the PBL / App RAM region, same address in all three images] | [NC: 32 bytes] | RAM boot record (section 8) |

Notes:
- The application vector table is at 0x0001_4400, aligned to 1 KiB because the
  MCXN947 vector table is 0x400 bytes (SDK linker script `m_interrupts`).
- The reference manual is not available, so it is **unknown** whether the boot ROM or a
  reset clears the no-init area. The design does not depend on it: a lost RAM boot record
  only disables the fast path (sections 8 and 9).

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
| 0x0C | hw_id | uint32 | [NC: 0x0947_0001] (MCXN947 TCU); checked by `FF01` |
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

## 8. RAM boot record (no-init RAM)

Shared by the application, PBL and SBL at the same fixed address, placed in a `NOLOAD`
linker section that the start-up code does not zero.

| Field | Type | Notes |
|---|---|---|
| magic | uint32 | "xATR" (`0x52544178`) |
| magic_inv | uint32 | bitwise inverse of `magic` |
| request | uint8 | 0 = none, 1 = programming (`SYS-BOOT-003`) |
| abnormal_resets | uint8 | consecutive abnormal resets (`SYS-BOOT-012`) |
| reserved | uint8[2] | 0 |
| crc32 | uint32 | CRC-32/ISO-HDLC over the fields above |

The PBL treats the record as **absent** unless `magic`, `magic_inv` and `crc32` are all
correct. An absent record is re-initialized with `request = 0`, `abnormal_resets = 0`.

## 9. PBL start-up decision

Evaluated once after every reset, in this order:

| # | Condition | Action |
|---|---|---|
| 1 | Read `CMC->SRS`, clear the sticky flags (`CMC->SSRS`); read and validate the RAM boot record | - |
| 2 | Record valid, `request = 1` **and** `SRS.SW = 1` | clear `request`, programming session, send `50 02` (`SYS-BOOT-003`) |
| 3 | `SRS` reports WWDT0, LOCKUP or FATAL | `abnormal_resets` + 1, else set it to 0 (POR, PIN, SW, ...) |
| 4 | `abnormal_resets` >= [NC: 3] | stay in the bootloader, default session (`SYS-BOOT-012`) |
| 5 | Application check fails (section 10) | stay in the bootloader, default session (`SYS-BOOT-002`) |
| 6 | Boot window [NC: 50] ms: `10 02` received on 0x7E1 | programming session, send `50 02` (`SYS-BOOT-011`) |
| 7 | otherwise | de-initialize CAN and clocks used by the PBL, set VTOR, jump to the application |

The tester's recovery procedure (used by `SIM-TST-002`): send `10 02` on 0x7E1 every
[NC: 5] ms, then reset the ECU (power cycle, or `11 01` if the application still answers),
until `50 02` arrives.

## 10. Robustness rules

| ID | Rule | Reason |
|---|---|---|
| RB-01 | RAM ECC configuration (`SYSCON->ECC_ENABLE_CTRL`) is identical in application, PBL and SBL. Default: disabled, as the SDK `SystemInit()` does. | Data written with one ECC setting and read with another produces errors; with ECC disabled no RAM initialization is needed after reset. |
| RB-02 | The PBL application check sets `SYSCON->NVM_CTRL.DIS_MBECC_ERR_DATA = 1` while it reads the application, restores it afterwards, and checks the header (magic, `header_crc32`, `image_size` within the application area) before it reads the image. A BusFault / HardFault during the check is reported as "application invalid", not as a reset. | `SYS-BOOT-013`: interrupted programming must not cause a fault loop. The FRDM-MCXN947 `pflash` SDK example also disables multi-bit ECC bus errors before reading programmed flash. |
| RB-03 | The SBL clears the flash cache (`SYSCON->NVM_CTRL.CLR_FLASH_CACHE`) and disables flash speculation before every `checkMemory` (`0202`) and `checkProgrammingDependencies` (`FF01`). | Avoid stale data from cache / speculation after programming. |
| RB-04 | While it erases or programs bank 0, the SBL executes **only from RAM**: code, constant data, CAN driver and vector table (VTOR in RAM). No access to bank 0 except through the flash API. | The bank being erased / programmed cannot be read; a fetch from it stalls or faults. |
| RB-05 | During long erase operations the SBL sends `7F 31 78` at least every P2* - [NC: 500] ms and keeps processing CAN between sector erases. | Keep the tester session alive. |
| RB-06 | An application image flashed with a debugger must also contain a valid header (post-build step), and debugger flashing must never mass-erase the device. | The PBL starts only images with a valid header; a mass erase removes the PBL. |

## 11. Routines

| RID | Name | Where | Parameters | Result |
|---|---|---|---|---|
| 0x0203 | checkProgrammingPreconditions | App | - | 0 = OK, else bit mask: bit 0 not at standstill, bit 1 not in P, bit 2 speed invalid |
| 0x0202 | checkMemory | PBL, SBL | CRC32 (4) of the last download | 0 = OK, 1 = CRC mismatch |
| 0x0301 | activateSecondaryBootloader | PBL | - | 0 = OK (answered by the SBL) |
| 0xFF00 | eraseMemory | SBL | `44`, address (4), size (4); sector-aligned | 0 = OK, 1 = failed |
| 0xFF01 | checkProgrammingDependencies | SBL | - | 0 = OK, 1 = header invalid, 2 = hw_id mismatch, 3 = PBL version too old |

## 12. Transfer parameters and timing

| Parameter | Value |
|---|---|
| dataFormatIdentifier (`34`) | 0x00 (no compression, no encryption) |
| addressAndLengthFormatIdentifier | 0x44 |
| maxNumberOfBlockLength | [NC: 0x0402] (1024 data bytes + SID + block sequence counter) |
| Write granularity | 16-byte phrase; the last block is padded with 0xFF by the tester |
| Erase granularity | 8 KiB sector |
| P2server / P2*server | 50 ms / 5000 ms |
| S3server | 5000 ms |

## 13. Open questions

| # | Question | Affects | Answer |
|---|---|---|---|
| 1 | SBL load address and no-init RAM address | section 5 | Whether RAM survives a reset is not checked (no reference manual, no experiment, user decision 2026-10-04); covered by sections 8 - 10. |
| 2 | PBL size 64 KiB enough? (CAN + ISO-TP + UDS + CRC, no flash driver) | section 5 | |
| 3 | Seed source: TRNG or timer-based? | section 3 | |
| 4 | Use the MCXN947 CRC engine or a software table CRC in the PBL? (start-up time `SYS-STA-001`) | SYS-BOOT-001 | |
