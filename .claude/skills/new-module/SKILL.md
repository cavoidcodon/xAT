---
name: new-module
description: Scaffold a new AUTOSAR module (MCAL, BSW, OS, RTE or SWC) in xAT from the project templates - header, source, config, unit test and design doc. Use when starting work on a module that has no files yet. Args - "<Ma> <layer path>", e.g. "Dio mcal" or "IoHwAb bsw/ecual".
---

# Scaffold an AUTOSAR module

Input: module abbreviation `<Ma>` (exact BSW module list spelling, e.g. `Dio`, `EcuM`,
`IoHwAb`) and the layer path under `src/` (e.g. `mcal`, `bsw/services`, `bsw/ecual`, `app`).

1. Check that `src/<layer>/<Ma>/` has no source files yet (only `.gitkeep`). If it does,
   stop and tell the user.
2. Look up in the AUTOSAR R21-11 documents (`docs/references/autosar/`, or ask the user if
   they are missing):
   - the module ID (`AUTOSAR_TR_BSWModuleList`);
   - the SWS document name, the public API list with service IDs, and the DET error codes.
3. Copy the templates from `templates/` and replace the placeholders:

   | Template | Destination |
   |---|---|
   | `Module.h` | `src/<layer>/<Ma>/include/<Ma>.h` |
   | `Module.c` | `src/<layer>/<Ma>/src/<Ma>.c` |
   | `Module_Cfg.h` | `src/<layer>/<Ma>/config/<Ma>_Cfg.h` |
   | `Module_Cfg.c` | `src/<layer>/<Ma>/config/<Ma>_Cfg.c` |
   | `test_Module.c` | `tests/unit/<layer>/<Ma>/test_<Ma>.c` |
   | `module_doc.md` | `docs/modules/<Ma>.md` |

   Placeholders: `<Ma>` (e.g. `Dio`), `<MA>` (e.g. `DIO`), `<MODULE_ID>`, `<SWS_DOC>`
   (e.g. `AUTOSAR_SWS_DIODriver`), `<LAYER>`.
4. Fill in only the skeleton: the published information, the DET error codes and service
   IDs from the SWS, and the API prototypes with `/* TODO */` bodies. **Do not implement
   behaviour.** Implementation is a separate task that the user approves.
5. Remove the `.gitkeep` files from the directories that now have content.
6. Report the created files and the information you could not find in the SWS.

All generated code must follow `.claude/rules/coding_rules.md`.
