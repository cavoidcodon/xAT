/**
 * @file    dummy_dep.h
 * @brief   Dummy dependency for the CMock self-test of the unit test framework.
 * @details Never implemented: the tests in this directory link only its CMock mock
 *          (mock_dummy_dep.c), generated at build time.
 */
#ifndef DUMMY_DEP_H
#define DUMMY_DEP_H

#include "Std_Types.h"

/**
 * @brief      Reads the value of a channel.
 * @param[in]  channel  Channel to read.
 * @param[out] value    Value read.
 * @return     E_OK when the value is valid, else E_NOT_OK.
 */
Std_ReturnType DummyDep_Read(uint8 channel, uint8 *value);

/**
 * @brief      Writes the value of a channel.
 * @param[in]  channel  Channel to write.
 * @param[in]  value    Value to write.
 */
void DummyDep_Write(uint8 channel, uint8 value);

#endif /* DUMMY_DEP_H */
