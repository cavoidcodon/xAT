/**
 * @file    <Ma>_Cfg.h
 * @brief   <Ma> pre-compile configuration (handwritten).
 * @details AUTOSAR Classic R21-11, <SWS_DOC>.
 */
#ifndef <MA>_CFG_H
#define <MA>_CFG_H

#include "Std_Types.h"

/*==================================================================================================
 *                                 PRE-COMPILE PARAMETERS
 *================================================================================================*/
#define <MA>_DEV_ERROR_DETECT               (STD_ON)
#define <MA>_VERSION_INFO_API               (STD_ON)

/*==================================================================================================
 *                                  CONFIGURATION TYPES
 *================================================================================================*/
struct <Ma>_ConfigTag
{
    uint8 reserved; /* TODO: replace with the SWS configuration content */
};

/*==================================================================================================
 *                                CONFIGURATION SET(S)
 *================================================================================================*/
extern const struct <Ma>_ConfigTag <Ma>_Config;

#endif /* <MA>_CFG_H */
