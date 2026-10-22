/**
 * @file    test_<Ma>.c
 * @brief   Functional unit tests for <Ma> (Unity + CMock, host build).
 */
#include "unity.h"
#include "<Ma>.h"
#include "mock_Det.h"

void setUp(void)
{
    /* TODO: reset module state */
}

void tearDown(void)
{
}

/* [SWS_<Ma>_xxxxx] [REQ-<task>-nn] TODO: IDs of every requirement this test covers (coding_rules.md 8.6) */
void test_Init_NullConfig_ReportsParamPointerDetError(void)
{
    Det_ReportError_ExpectAndReturn(<MA>_MODULE_ID, 0U, <MA>_SID_INIT, <MA>_E_PARAM_POINTER, E_OK);

    <Ma>_Init(NULL_PTR);
}
