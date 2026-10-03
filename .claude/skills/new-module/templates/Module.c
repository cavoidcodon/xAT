/**
 * @file    <Ma>.c
 * @brief   <Ma> implementation.
 * @details AUTOSAR Classic R21-11, <SWS_DOC>. Layer: <LAYER>.
 */

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
#include "<Ma>.h"
#if (<MA>_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                      VERSION CHECK
 *================================================================================================*/
#if ((<MA>_AR_RELEASE_MAJOR_VERSION != 4U) || (<MA>_AR_RELEASE_MINOR_VERSION != 7U))
#error "<Ma>.c: AUTOSAR release version mismatch with <Ma>.h"
#endif

/*==================================================================================================
 *                                      LOCAL MACROS
 *================================================================================================*/
#define <MA>_INSTANCE_ID                    (0x00U)

/*==================================================================================================
 *                                    STATIC VARIABLES
 *================================================================================================*/
static boolean s_isInitialized = FALSE;
static const <Ma>_ConfigType *s_configPtr = NULL_PTR;

/*==================================================================================================
 *                                     GLOBAL FUNCTIONS
 *================================================================================================*/
void <Ma>_Init(const <Ma>_ConfigType *config_ptr)
{
#if (<MA>_DEV_ERROR_DETECT == STD_ON)
    if (config_ptr == NULL_PTR)
    {
        (void)Det_ReportError(<MA>_MODULE_ID, <MA>_INSTANCE_ID, <MA>_SID_INIT, <MA>_E_PARAM_POINTER);
    }
    else
#endif
    {
        s_configPtr = config_ptr;
        /* TODO: implement initialization per SWS */
        s_isInitialized = TRUE;
    }
}

#if (<MA>_VERSION_INFO_API == STD_ON)
void <Ma>_GetVersionInfo(Std_VersionInfoType *version_info)
{
    /* TODO: DET check for NULL_PTR, fill vendorID/moduleID/sw_*_version */
    (void)version_info;
}
#endif
