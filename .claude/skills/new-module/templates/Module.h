/**
 * @file    <Ma>.h
 * @brief   <Ma> public interface.
 * @details AUTOSAR Classic R21-11, <SWS_DOC>. Layer: <LAYER>.
 */
#ifndef <MA>_H
#define <MA>_H

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
#include "Std_Types.h"
#include "<Ma>_Cfg.h"

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
#define <MA>_VENDOR_ID                      (0xFFFFU)
#define <MA>_MODULE_ID                      (<MODULE_ID>U)
#define <MA>_AR_RELEASE_MAJOR_VERSION       (4U)
#define <MA>_AR_RELEASE_MINOR_VERSION       (7U)
#define <MA>_AR_RELEASE_REVISION_VERSION    (0U)
#define <MA>_SW_MAJOR_VERSION               (1U)
#define <MA>_SW_MINOR_VERSION               (0U)
#define <MA>_SW_PATCH_VERSION               (0U)

/*==================================================================================================
 *                                    SERVICE IDS (SWS)
 *================================================================================================*/
#define <MA>_SID_INIT                       (0x00U)

/*==================================================================================================
 *                                 DET ERROR CODES (SWS)
 *================================================================================================*/
#define <MA>_E_UNINIT                       (0x00U) /* TODO: value from SWS */
#define <MA>_E_PARAM_POINTER                (0x00U) /* TODO: value from SWS */

/*==================================================================================================
 *                                          TYPES
 *================================================================================================*/
/** @brief Post-build / link-time configuration set. Content defined in <Ma>_Cfg.h. */
typedef struct <Ma>_ConfigTag <Ma>_ConfigType;

/*==================================================================================================
 *                                     API PROTOTYPES
 *================================================================================================*/
/**
 * @brief      Initializes the module.
 * @param[in]  config_ptr  Pointer to the configuration set.
 */
void <Ma>_Init(const <Ma>_ConfigType *config_ptr);

#if (<MA>_VERSION_INFO_API == STD_ON)
/**
 * @brief      Returns the version information of this module.
 * @param[out] version_info  Destination of the version information.
 */
void <Ma>_GetVersionInfo(Std_VersionInfoType *version_info);
#endif

#endif /* <MA>_H */
