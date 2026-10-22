/**
 * @file    test_Std_Types.c
 * @brief   Unit tests for Std_Types.h (Unity, host build).
 * @details AUTOSAR Classic R21-11, AUTOSAR_SWS_StandardTypes.
 *
 *          Requirement tags: [SWS_Std_xxxxx] = AUTOSAR_SWS_StandardTypes,
 *          [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
 */
#include "unity.h"
#include "Std_Types.h"

/* Size of one member of a structure type. */
#define MEMBER_SIZE(type, member) ((uint32)sizeof(((type *)NULL_PTR)->member))

/* Fixed results of the test protocol header parser below. */
#define TEST_PARSED_MESSAGE_TYPE   (STD_MESSAGETYPE_RESPONSE)
#define TEST_PARSED_MESSAGE_RESULT (STD_MESSAGERESULT_ERROR)

/* Function with the exact signature of Std_ExtractProtocolHeaderFieldsType: assigning it to a
 * variable of that type fails to compile if the type does not match the SWS. */
static Std_ReturnType Test_ExtractHeader(const uint8 *buffer,
                                         uint32 bufferLength,
                                         Std_MessageTypeType *messageType,
                                         Std_MessageResultType *messageResult)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (bufferLength > 0U))
    {
        *messageType = TEST_PARSED_MESSAGE_TYPE;
        *messageResult = TEST_PARSED_MESSAGE_RESULT;
        result = E_OK;
    }

    return result;
}

void setUp(void)
{
}

void tearDown(void)
{
}

/*==================================================================================================
 *                                       RETURN TYPE
 *================================================================================================*/
/* [SWS_Std_00005] [REQ-003-04] Std_ReturnType is uint8 */
void test_StdReturnType_Definition_IsUint8(void)
{
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32)sizeof(Std_ReturnType));
    TEST_ASSERT_TRUE((Std_ReturnType)-1 > (Std_ReturnType)0U);
    TEST_ASSERT_EQUAL_INT(1, _Generic((Std_ReturnType)0U, uint8: 1, default: 0));
}

/* [SWS_Std_00006] [REQ-003-04] E_OK = 0, E_NOT_OK = 1, E_OK shared with OSEK via STATUSTYPEDEFINED */
void test_StdReturnValues_Definitions_EOkZeroENotOkOne(void)
{
    Std_ReturnType ok = E_OK;
    Std_ReturnType not_ok = E_NOT_OK;

    TEST_ASSERT_EQUAL_HEX8(0x00U, ok);
    TEST_ASSERT_EQUAL_HEX8(0x01U, not_ok);
#ifdef STATUSTYPEDEFINED
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32)sizeof(StatusType));
#else
    TEST_FAIL_MESSAGE("STATUSTYPEDEFINED is not defined by Std_Types.h");
#endif
}

/*==================================================================================================
 *                                      SYMBOL DEFINITIONS
 *================================================================================================*/
/* [SWS_Std_00007] [REQ-003-04] */
void test_StdHighLow_Definitions_HighOneLowZero(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x01U, STD_HIGH);
    TEST_ASSERT_EQUAL_HEX8(0x00U, STD_LOW);
}

/* [SWS_Std_00013] [REQ-003-04] */
void test_StdActiveIdle_Definitions_ActiveOneIdleZero(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x01U, STD_ACTIVE);
    TEST_ASSERT_EQUAL_HEX8(0x00U, STD_IDLE);
}

/* [SWS_Std_00010] [REQ-003-04] */
void test_StdOnOff_Definitions_OnOneOffZero(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x01U, STD_ON);
    TEST_ASSERT_EQUAL_HEX8(0x00U, STD_OFF);
}

/*==================================================================================================
 *                                     VERSION INFORMATION
 *================================================================================================*/
/* [SWS_Std_00015] [REQ-003-05] members vendorID, moduleID (uint16) and sw_*_version (uint8) */
void test_StdVersionInfoType_Members_NamesAndTypesMatchSws(void)
{
    Std_VersionInfoType version_info = {0U, 0U, 0U, 0U, 0U};

    version_info.vendorID = 0xFFFFU;
    version_info.moduleID = 0x1234U;
    version_info.sw_major_version = 1U;
    version_info.sw_minor_version = 2U;
    version_info.sw_patch_version = 3U;

    TEST_ASSERT_EQUAL_UINT32(2U, MEMBER_SIZE(Std_VersionInfoType, vendorID));
    TEST_ASSERT_EQUAL_UINT32(2U, MEMBER_SIZE(Std_VersionInfoType, moduleID));
    TEST_ASSERT_EQUAL_UINT32(1U, MEMBER_SIZE(Std_VersionInfoType, sw_major_version));
    TEST_ASSERT_EQUAL_UINT32(1U, MEMBER_SIZE(Std_VersionInfoType, sw_minor_version));
    TEST_ASSERT_EQUAL_UINT32(1U, MEMBER_SIZE(Std_VersionInfoType, sw_patch_version));
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, version_info.vendorID);
    TEST_ASSERT_EQUAL_HEX16(0x1234U, version_info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(1U, version_info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(2U, version_info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(3U, version_info.sw_patch_version);
}

/*==================================================================================================
 *                              TRANSFORMER AND MESSAGE TYPES
 *================================================================================================*/
/* [SWS_Std_00021] [SWS_Std_00022] [SWS_Std_00023] [SWS_Std_00024] [SWS_Std_00025] [SWS_Std_00026] */
void test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses(void)
{
    Std_TransformerError error = {0U, STD_TRANSFORMER_UNSPECIFIED};

    error.errorCode = 0x7FU;
    error.transformerClass = STD_TRANSFORMER_SAFETY;

    TEST_ASSERT_EQUAL_UINT32(1U, (uint32)sizeof(Std_TransformerErrorCode));
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32)sizeof(Std_TransformerClass));
    TEST_ASSERT_EQUAL_HEX8(0x7FU, error.errorCode);
    TEST_ASSERT_EQUAL_HEX8(0x02U, error.transformerClass);
    TEST_ASSERT_EQUAL_HEX8(0x00U, STD_TRANSFORMER_UNSPECIFIED);
    TEST_ASSERT_EQUAL_HEX8(0x01U, STD_TRANSFORMER_SERIALIZER);
    TEST_ASSERT_EQUAL_HEX8(0x02U, STD_TRANSFORMER_SAFETY);
    TEST_ASSERT_EQUAL_HEX8(0x03U, STD_TRANSFORMER_SECURITY);
    TEST_ASSERT_EQUAL_HEX8(0xFFU, STD_TRANSFORMER_CUSTOM);
}

/* [SWS_Std_91001] [SWS_Std_91002] [SWS_Std_00017] [SWS_Std_00019] */
void test_MessageTypes_Definitions_Uint8WithSwsValues(void)
{
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32)sizeof(Std_MessageTypeType));
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32)sizeof(Std_MessageResultType));
    TEST_ASSERT_EQUAL_HEX8(0x00U, STD_MESSAGETYPE_REQUEST);
    TEST_ASSERT_EQUAL_HEX8(0x01U, STD_MESSAGETYPE_RESPONSE);
    TEST_ASSERT_EQUAL_HEX8(0x00U, STD_MESSAGERESULT_OK);
    TEST_ASSERT_EQUAL_HEX8(0x01U, STD_MESSAGERESULT_ERROR);
}

/* [SWS_Std_91003] the function pointer type takes a function with the SWS signature */
void test_ExtractProtocolHeaderFields_FunctionPointer_CallsParser(void)
{
    const uint8 header[1] = {0x00U};
    Std_ExtractProtocolHeaderFieldsType extract = &Test_ExtractHeader;
    Std_MessageTypeType message_type = STD_MESSAGETYPE_REQUEST;
    Std_MessageResultType message_result = STD_MESSAGERESULT_OK;
    Std_ReturnType result = E_NOT_OK;

    result = extract(header, (uint32)sizeof(header), &message_type, &message_result);

    TEST_ASSERT_EQUAL_HEX8(E_OK, result);
    TEST_ASSERT_EQUAL_HEX8(STD_MESSAGETYPE_RESPONSE, message_type);
    TEST_ASSERT_EQUAL_HEX8(STD_MESSAGERESULT_ERROR, message_result);
}

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
/* [REQ-003-06] AR release 4.7.0, SW version 1.0.0, module ID 197, unregistered vendor */
void test_PublishedInfo_StdTypes_Ar470Sw100(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, STD_VENDOR_ID);
    TEST_ASSERT_EQUAL_UINT16(197U, STD_MODULE_ID);
    TEST_ASSERT_EQUAL_UINT8(4U, STD_AR_RELEASE_MAJOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(7U, STD_AR_RELEASE_MINOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, STD_AR_RELEASE_REVISION_VERSION);
    TEST_ASSERT_EQUAL_UINT8(1U, STD_SW_MAJOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, STD_SW_MINOR_VERSION);
    TEST_ASSERT_EQUAL_UINT8(0U, STD_SW_PATCH_VERSION);
}
