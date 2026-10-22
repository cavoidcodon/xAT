---
name: new-module
description: Scaffold a new AUTOSAR module (MCAL, BSW, OS, RTE or SWC) in xAT from the project templates - header, source, config and unit test, following the module's reviewed design. Use when starting work on a module that has no files yet. Args - "<Ma> <layer path>", e.g. "Dio mcal" or "IoHwAb bsw/ecual".
---

# Scaffold an AUTOSAR module

Input: module abbreviation `<Ma>` (exact BSW module list spelling, e.g. `Dio`, `EcuM`,
`IoHwAb`) and the layer path under `src/` (e.g. `mcal`, `bsw/services`, `bsw/ecual`, `app`).

1. Check that `src/<layer>/<Ma>/` has no source files yet (only `.gitkeep`). If it does,
   stop and tell the user.
   For MCAL, BSW, OS and RTE modules, check that `docs/modules/<Ma>/design.md` exists (made
   with the `module-design` skill and reviewed by the user). If it does not, stop and
   suggest running `module-design` first: the scaffold follows the design's scope.
   For SWCs, check that `docs/modules/<SwcName>/design.md` exists (made with the
   `swc-design` skill and reviewed by the user); if not, suggest running `swc-design` first.
2. Look up in the AUTOSAR R21-11 documents (`docs/references/autosar/`, or ask the user if
   they are missing):
   - the module ID (`AUTOSAR_TR_BSWModuleList`);
   - the SWS document name, the public API list with service IDs, and the DET error codes.
   Scaffold only the APIs and containers listed as supported in `design.md`.
3. Copy the templates from `templates/` and replace the placeholders:

   | Template | Destination |
   |---|---|
   | `Module.h` | `src/<layer>/<Ma>/include/<Ma>.h` |
   | `Module.c` | `src/<layer>/<Ma>/src/<Ma>.c` |
   | `Module_Cfg.h` | `src/<layer>/<Ma>/config/<Ma>_Cfg.h` |
   | `Module_Cfg.c` | `src/<layer>/<Ma>/config/<Ma>_Cfg.c` |
   | `test_Module.c` | `tests/unit/<layer>/<Ma>/test_<Ma>.c` |
   | `CMakeLists_test.txt` | `tests/unit/<layer>/<Ma>/CMakeLists.txt` |

   Placeholders: `<Ma>` (e.g. `Dio`), `<MA>` (e.g. `DIO`), `<MODULE_ID>`, `<SWS_DOC>`
   (e.g. `AUTOSAR_SWS_DIODriver`), `<LAYER>` (the layer path, e.g. `mcal`, `bsw/ecual`).
   Adjust `MOCKS` to the module's real dependencies (for MCAL, the fake SDK headers in
   `tests/unit/stubs/sdk/`).

   Register the module in both builds (`docs/unit_testing.md`, `docs/operations.md`):
   - firmware: one line in `src/ecu/CMakeLists.txt`,
     `xat_add_module(NAME <Ma> DIR ${XAT_ROOT}/src/<layer>/<Ma>)`;
   - host tests: `add_subdirectory(<layer>/<Ma>)` in `tests/unit/CMakeLists.txt`.
4. Fill in only the skeleton: the published information, the DET error codes and service
   IDs from the SWS, and the API prototypes with `/* TODO */` bodies. **Do not implement
   behaviour.** Implementation is a separate task that the user approves.
5. Remove the `.gitkeep` files from the directories that now have content.
6. Report the created files and the information you could not find in the SWS.

All generated code must follow `.claude/rules/coding_rules.md`.
