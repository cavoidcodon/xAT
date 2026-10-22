# Self-test of the unit test framework (Unity + CMock), kept permanently.
#
# Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.

# [REQ-003-10] one call: test file + mocked dependency
xat_add_unit_test(NAME test_CMock_Selftest
    SOURCES test_CMock_Selftest.c
    MOCKS ${CMAKE_CURRENT_SOURCE_DIR}/dummy_dep.h)

# [REQ-003-11] guard: both tests must fail, each for its own reason. CTest passes only when the
# output has the assertion failure, the unmet expectation and the Unity summary, in this order.
xat_add_unit_test(NAME test_CMock_Guard
    SOURCES test_CMock_Guard.c
    MOCKS ${CMAKE_CURRENT_SOURCE_DIR}/dummy_dep.h
    PASS_REGEX "FAIL: Expected 1 Was 2.*DummyDep_Write:Called fewer times than expected.*2 Tests 2 Failures 0 Ignored")
