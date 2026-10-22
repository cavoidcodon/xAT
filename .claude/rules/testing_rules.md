---
paths:
  - "tests/**"
---

# Testing rules

Framework: **Unity + CMock** (vendored in `third_party/`), built for the host with
the native gcc, using CMake (`tests/unit/CMakeLists.txt`).

Layout (mirrors `src/`):

```
tests/unit/<layer>/<Ma>/test_<Ma>.c        # one test file per module (split by API if large)
tests/unit/<layer>/<Ma>/CMakeLists.txt     # module under test + mocks of its dependencies
tests/unit/stubs/sdk/                      # minimal fake NXP SDK headers (types, prototypes)
tests/target/                              # on-board functional tests (later)
```

Rules:
- Every module has functional tests derived from its behaviour in the SWS and
  in `docs/modules/<Ma>/design.md`. Test the behaviour, not the implementation details.
- Only the module under test is real. Every dependency (`Det`, `SchM`, other modules,
  NXP SDK functions) is a CMock mock generated from its header, or from the stub
  header in `tests/unit/stubs/sdk/`.
- Name tests `test_<Api>_<Condition>_<ExpectedResult>`, for example
  `test_ReadChannel_NotInitialized_ReportsUninitDetError`.
- Each test has the IDs of all requirements it covers (SWS, SYS / SWR, task `REQ-...`) in
  a comment directly above it, in the format of `coding_rules.md` 8.6:
  `/* [SWS_Dio_00133] [REQ-007-03] */`. A test without a requirement tag tests nothing
  that was asked for. Update the matrix in `docs/autosar_compliance.md` (SWS) or
  `docs/specs/traceability.md` (SYS / SWR).
- Compile-only checks (`xat_add_compile_check`) and test `CMakeLists.txt` files carry the
  tags of what they check in a comment as well.
- Minimum per API: nominal case, each DET error (with DEV_ERROR_DETECT on),
  boundary values, and behaviour before `<Ma>_Init`.
- Tests must be independent: `setUp()` resets module state (a test-only
  `<Ma>_DeInit` or re-init), and tests do not depend on execution order.
- Test code follows `coding_rules.md`, except that Unity/CMock macros and the
  generated mock names are allowed as they are.
