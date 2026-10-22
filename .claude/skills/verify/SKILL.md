---
name: verify
description: Verify xAT changes - firmware build, host unit tests (Unity + CMock), layering include check, git diff review. Use after implementing a change or when the user asks to verify.
---

# Verify

Run each step and report the real result. Do not summarize a failure as a success.

1. **Firmware build.**
   `python3 scripts/operations.py build` (add `--clean` after CMake changes). Must exit 0
   without warnings from project code.
2. **Unit tests (host).**
   ```sh
   export PATH=$PWD/.venv/bin:$PATH
   cmake -G Ninja -S tests/unit -B build/unit && cmake --build build/unit && ctest --test-dir build/unit --output-on-failure
   ```
   Uses the venv's cmake/ninja and Ruby (CMock, Unity runner generator); see
   `docs/unit_testing.md`. The build also runs the compile checks (each header alone,
   include guards); a build error is a failed verification. All tests must pass, including
   the guard test `test_CMock_Guard`, which passes only when Unity and CMock detect its two
   deliberate failures.
3. **Layering.** Check that no file under `src/app`, `src/rte` or `src/bsw` includes
   an NXP SDK header, and that `src/app` includes only RTE and its own headers:
   ```sh
   grep -rnE '#include\s+[<"](fsl_|board\.h|MCXN947|fsl_device_registers)' src/app src/rte src/bsw
   grep -rnE '#include\s+"' src/app | grep -vE '"(Rte_|Swc)'
   ```
   Both must print nothing.
4. **On target (only when the task needs it).** Run
   `python3 scripts/operations.py run --expect "<text>"` or the module's target test.
5. **Diff.** Read `git status` and `git diff`. Look for unrelated changes and files
   outside the task's scope.
6. **Requirement tags.** New or changed code and tests carry the IDs of the requirements
   they implement or cover (`coding_rules.md` 8.6). Every requirement marked `Implemented`
   or `Partial` in `docs/autosar_compliance.md` (or covered in `docs/specs/traceability.md`)
   for the changed module has a tag in `src/` and in `tests/`; list the tags with the
   `grep` command of `coding_rules.md` 8.6 and report any requirement without one.
