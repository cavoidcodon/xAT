/* =========================================================================
    CMock - Automatic Mock Generation for C
    ThrowTheSwitch.org
    Copyright (c) 2007-26 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT
========================================================================= */

#include "unity.h"
#include "cmock.h"
#include <setjmp.h>
#include <stdio.h>

extern void setUp(void);
extern void tearDown(void);

extern void test_MemNewWillReturnNullIfGivenIllegalSizes(void);
extern void test_MemNewWillNowSupportSizesGreaterThanTheDefinesCMockSize(void);
extern void test_MemChainWillReturnNullAndDoNothingIfGivenIllegalInformation(void);
extern void test_MemNextWillReturnNullIfGivenABadRoot(void);
extern void test_ThatWeCanClaimAndChainAFewElementsTogether(void);
extern void test_ThatWeCanAskForAllSortsOfSizes(void);

int main(void)
{
  UnityBegin("TestCMockDynamic.c");

  RUN_TEST(test_MemNewWillReturnNullIfGivenIllegalSizes, 26);
  RUN_TEST(test_MemNewWillNowSupportSizesGreaterThanTheDefinesCMockSize, 35);
  RUN_TEST(test_MemChainWillReturnNullAndDoNothingIfGivenIllegalInformation, 45);
  RUN_TEST(test_MemNextWillReturnNullIfGivenABadRoot, 59);
  RUN_TEST(test_ThatWeCanClaimAndChainAFewElementsTogether, 70);
  RUN_TEST(test_ThatWeCanAskForAllSortsOfSizes, 152);

  UnityEnd();
  CMock_Guts_MemFreeFinal();
  return 0;
}
