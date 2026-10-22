/**
 * @file    test_CMock_Selftest.c
 * @brief   Self-test of the host unit test framework: CMock mock generation and expectations.
 * @details Kept permanently as a guard for the test infrastructure (TASK-003 open question 5).
 *          The unit under test is the small function below; its dependency dummy_dep.h is mocked.
 *
 *          Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
 */
#include "unity.h"
#include "mock_dummy_dep.h"

#define SELFTEST_FROM_CHANNEL (1U)
#define SELFTEST_TO_CHANNEL   (2U)
#define SELFTEST_VALUE        (0x5AU)

/* Copies one channel to another; skips the write when the read fails. */
static Std_ReturnType Selftest_CopyChannel(uint8 from_channel, uint8 to_channel)
{
    uint8 value = 0U;
    Std_ReturnType result = DummyDep_Read(from_channel, &value);

    if (result == E_OK)
    {
        DummyDep_Write(to_channel, value);
    }

    return result;
}

void setUp(void)
{
}

void tearDown(void)
{
}

/* [REQ-003-10] a mocked dependency returns a value through a pointer and checks the next call */
void test_CopyChannel_ReadOk_WritesReadValue(void)
{
    uint8 read_value = SELFTEST_VALUE;
    Std_ReturnType result = E_NOT_OK;

    DummyDep_Read_ExpectAnyArgsAndReturn(E_OK);
    DummyDep_Read_ReturnThruPtr_value(&read_value);
    DummyDep_Write_Expect(SELFTEST_TO_CHANNEL, SELFTEST_VALUE);

    result = Selftest_CopyChannel(SELFTEST_FROM_CHANNEL, SELFTEST_TO_CHANNEL);

    TEST_ASSERT_EQUAL_HEX8(E_OK, result);
}

/* [REQ-003-10] the read channel is checked; no write is expected after a failed read */
void test_CopyChannel_ReadFails_SkipsWrite(void)
{
    Std_ReturnType result = E_OK;

    DummyDep_Read_ExpectAndReturn(SELFTEST_FROM_CHANNEL, NULL_PTR, E_NOT_OK);
    DummyDep_Read_IgnoreArg_value();

    result = Selftest_CopyChannel(SELFTEST_FROM_CHANNEL, SELFTEST_TO_CHANNEL);

    TEST_ASSERT_EQUAL_HEX8(E_NOT_OK, result);
}
