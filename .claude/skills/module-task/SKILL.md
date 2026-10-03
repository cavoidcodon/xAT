---
name: module-task
description: Run one xAT task file (.claude/tasks/TASK-xxx.md) end to end with the mandatory planning gate - read, investigate, plan, wait for approval, implement, build, test, review diff, update development log, report. Use when the user says to start or continue a TASK.
---

# Execute a TASK

Follow the workflow in `CLAUDE.md`. Steps:

1. **Read.** Read the task file, `CLAUDE.md`, the rules in `.claude/rules/` that apply, and
   `docs/modules/<Ma>.md` and `docs/architecture/overview.md` if they exist. Read the
   relevant SWS sections in `docs/references/autosar/`.
2. **Investigate.** Use read-only actions only: inspect the existing code, the
   dependencies' headers, and the NXP SDK driver API (for MCAL).
3. **Plan.** Present the plan:
   - the files to add or change;
   - the APIs and SWS requirements covered, and the ones left out;
   - the configuration values and where they come from;
   - the test cases (mapped to requirements);
   - the verification steps mapped to the task's acceptance criteria;
   - risks and open questions.
4. **Stop and wait for approval.** Change no files before approval. If the plan changes,
   present it again.
5. **Implement.** Change only what the task requires. If something forces a deviation
   from the plan, stop and ask.
6. **Build.** Build the firmware: `python3 scripts/operations.py build`.
7. **Test.** Run the unit tests of the module (see the `verify` skill).
8. **Review.** Review `git diff`: layering, coding rules, no stray changes, no SDK
   changes.
9. **Log.** Add an entry to `docs/development_log.md` (date, task, summary, decisions,
   deviations, verification table). Update `docs/modules/<Ma>.md`,
   `docs/autosar_compliance.md` and the task status in `.claude/tasks/backlog.md`.
10. **Report.** Report the changed files, the verification results (with real
    output), and the remaining risks.
