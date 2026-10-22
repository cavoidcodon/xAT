/**
 * @file    test_CMock_Guard.c
 * @brief   Guard tests of the host unit test framework: both tests here MUST fail.
 * @details CTest runs this executable with PASS_REGULAR_EXPRESSION (see CMakeLists.txt): it
 *          passes only when Unity reports both failures with the expected messages. If a failing
 *          assertion or an unmet CMock expectation ever stopped being detected, this test would
 *          fail. The manual check of REQ-003-11 is recorded in docs/development_log.md.
 *
 *          Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
 */
#include "unity.h"
#include "mock_dummy_dep.h"

#define GUARD_CHANNEL (3U)
#define GUARD_VALUE   (0x11U)

void setUp(void)
{
}

void tearDown(void)
{
}

/* [REQ-003-11] a failing assertion is reported as a failure */
void test_Guard_FailingAssertion_IsReportedAsFailure(void)
{
    uint8 actual = 2U;

    TEST_ASSERT_EQUAL_UINT8(1U, actual);
}

/* [REQ-003-11] an expectation that is never called is reported as a failure */
void test_Guard_UnmetExpectation_IsReportedAsFailure(void)
{
    DummyDep_Write_Expect(GUARD_CHANNEL, GUARD_VALUE);
}
