# TASK-XXX: <title>

| Field        | Value                                              |
|--------------|----------------------------------------------------|
| Status       | TODO                                               |
| Priority     | P0 / P1 / P2                                       |
| Depends on   | TASK-YYY                                           |
| Module(s)    | `<Ma>` (`src/<layer>/<Ma>`)                        |
| AUTOSAR doc  | `AUTOSAR_SWS_<...>` R21-11                         |

---

## 0. Planning gate (mandatory, do this first)

Do **not** create, modify or delete any file until the user has approved a written plan.
Follow the `module-task` skill (`.claude/skills/module-task/SKILL.md`).

## 1. Goal

<!-- One paragraph: what exists when this task is done, and why the ECU needs it. -->

## 2. Context

<!-- Verified facts: SDK driver(s) and example(s) to look at, board pins/peripherals,
     existing modules this depends on. -->

## 3. Scope

### In scope
1.

### Out of scope
-

## 4. Files (expected)

| File | Action |
|---|---|
| `src/<layer>/<Ma>/include/<Ma>.h` | add |

## 5. Requirements

<!-- SWS requirement IDs / APIs to implement, plus project-specific requirements. -->

| ID | Requirement |
|---|---|

## 6. Acceptance criteria

| AC | Criterion | How verified |
|---|---|---|
| AC0 | Plan approved before any change | — |
| AC1 | Firmware builds without warnings | `ops build --clean` |
| AC2 | Unit tests pass, every API covered | `ctest --test-dir build/unit` |
| AC3 | No layering violation | `verify` skill step 3 |
| AC4 | Docs updated (`docs/modules/<Ma>.md`, compliance matrix, development log) | review |

## 7. Open questions

-
