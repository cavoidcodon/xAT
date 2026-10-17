---
name: module-design
description: Design one AUTOSAR module (MCAL, BSW, OS or RTE) for xAT from its AUTOSAR Classic R21-11 SWS before any code exists - scope for xAT, interfaces, behaviour, state, configuration and error handling, with PlantUML component, sequence, state and config (class) diagrams in docs/modules/<Ma>/. Use this whenever development of an AUTOSAR module starts (e.g. "design Dio", "start the CanIf module", "read the Gpt SWS and propose a design"), before the new-module scaffold, and also when the user asks for diagrams of a module that has no design yet. Args - "<Ma> <layer path>", e.g. "Dio mcal" or "CanIf bsw/com".
---

# Design an AUTOSAR module

The design is the first step of a module: it fixes *what* xAT takes from the SWS and *how*
the module behaves, so that `new-module` (scaffold) and the implementation task start from
a reviewed picture instead of from the 150-page SWS. The user reviews the design; nothing
in `src/` or `tests/` is written by this skill.

Input: module abbreviation `<Ma>` (exact BSW module list spelling, e.g. `Dio`, `CanIf`,
`EcuM`) and the layer path under `src/` (e.g. `mcal`, `bsw/com`, `bsw/services`, `os`, `rte`).

## 1. Gather the inputs

- **SWS.** `docs/references/autosar/AUTOSAR_SWS_<...>.pdf`, plus `AUTOSAR_SWS_BSWGeneral.pdf`
  for the general `SWS_BSW_xxxxx` rules. If the module's SWS PDF is missing, stop and ask the
  user to add it - a design without the SWS would be invented, not derived.
  Read PDFs in page ranges of at most 20 pages. Start with the table of contents, then read
  in this order, because each one narrows the next:
  1. Functional specification (ch. 7): states, init, main processing, error classification.
  2. API specification (ch. 8): types, functions with service IDs, callbacks, scheduled
     functions, expected interfaces (mandatory / optional / configurable).
  3. Sequence diagrams (ch. 9), if any: base your sequences on them and cite the figure.
  4. Configuration specification (ch. 10): containers, parameters, multiplicities.
- **What xAT needs.** `docs/specs/` (system requirements, `system/can_matrix.md`,
  `system/system_context.md`), `docs/architecture/overview.md`, `.claude/tasks/backlog.md`
  and the task file of the module if one exists. This decides the scope.
- **Neighbours.** Design notes of modules already designed (`docs/modules/*/design.md`) and
  their public headers in `src/**/include/`, so that the interfaces match.
- **Rules.** `.claude/rules/layering.md` (who may call whom), `.claude/rules/mcal_rules.md`
  for MCAL, `.claude/rules/coding_rules.md` §10 (project-wide deviations).
- **MCAL only.** The NXP SDK driver in `../mcuxsdk` (e.g. `fsl_gpio.h`, `fsl_flexcan.h`)
  and the FRDM-MCXN947 pins / instances the module will use.

## 2. Decide the scope

xAT implements a subset of the SWS. For each API, feature and configuration container, decide
`Supported` or `Not supported` and give the reason in terms of xAT (a requirement, a signal
in the CAN matrix, a dependent module, or "not needed: <why>"). Keep everything that another
xAT module or the SWS marks as mandatory; leave out optional features nobody in xAT uses.
If you cannot tell whether xAT needs something, list it under *Open questions* instead of
guessing.

## 3. Write the design

Create `docs/modules/<Ma>/` with these files. Start `design.md` from
`templates/design.md` and fill every section; write "None" rather than deleting a section,
so the reviewer can see it was considered.

| File | Content |
|---|---|
| `design.md` | Scope, interfaces, behaviour, state, configuration, error handling, deviations, open questions; links to every diagram |
| `component.puml` | Component diagram |
| `seq_<scenario>.puml` | One sequence diagram per scenario (lower-case snake case, e.g. `seq_init.puml`, `seq_rx_indication.puml`) |
| `state.puml` | State machine(s) |
| `config.puml` | Class diagram of the configuration |

What each diagram must show:

- **Component.** `<Ma>` in the middle; the interfaces it provides (its API, callbacks,
  main function) and requires (expected interfaces from SWS ch. 8.5/8.6); the neighbouring
  modules grouped by layer (`package "MCAL"`, `package "BSW"`, ...); `Det`, `SchM` and, for
  MCAL, the NXP SDK driver. Draw only dependencies allowed by `layering.md` - if the SWS
  needs one that the rules forbid, record it as an open question.
- **Sequence.** At least: initialisation (from `EcuM` or the caller in the SWS); every
  supported API with non-trivial behaviour; the main function if the module has one;
  callbacks / notifications in both directions; and one error path showing the Det report
  (development error) and the return value. Use the real caller (e.g. `CanIf`, `Os task`,
  `ISR`) as the first participant, not an abstract "User".
- **State.** The module state (e.g. `UNINIT` -> `INIT`) and any per-channel, per-PDU or
  per-controller state the SWS defines. Each transition is labelled with the triggering API
  or event and, where relevant, the guard.
- **Config.** `<Ma>_ConfigType` and the structures for each supported container, with the
  parameters as attributes (`name : type`), multiplicities as on the SWS, and references to
  other modules' configuration as associations. Mark each parameter's class
  (pre-compile / link-time / post-build) with a stereotype. Configuration in xAT is
  handwritten C, so the diagram is the structure that `config/<Ma>_Cfg.c` will fill.

PlantUML conventions (keep them consistent across modules, so diagrams of different modules
read the same way):

- `@startuml <Ma>_<file name>` and a `title`.
- Names exactly as in the SWS: API names, types, container and parameter names.
- Put the SWS requirement IDs on the element they justify, in a `note` or the message
  label, e.g. `Dio_ReadChannel(ChannelId) ' SWS_Dio_00027`. These IDs are how the reviewer
  checks the design against the SWS and how the compliance matrix is filled later.
- No colours or skin parameters; plain PlantUML renders the same everywhere.
- If a step is your assumption rather than an SWS statement, mark it `<<assumption>>` or in a
  note starting with `ASSUMPTION:` and list it in `design.md` under *Open questions*.

## 4. Check

- If `plantuml` is installed, run `plantuml -checkonly docs/modules/<Ma>/*.puml` and fix any
  error. If it is not installed, say so in the report; do not claim the diagrams render.
- Cross-check: every supported API appears in the component diagram and in at least one
  sequence; every state transition is caused by an API or event that exists; every
  `<<post-build>>` parameter appears in `<Ma>_ConfigType`.
- Add the link `docs/modules/<Ma>/design.md` to the *Design notes* column of the module list
  in `docs/architecture/overview.md`.

## 5. Report and stop

Report the files created, the scope summary (supported / not supported APIs), the
assumptions, and the open questions. Then stop: the user reviews the design. The next steps -
`new-module` to scaffold, then the implementation task - start only after that review.
