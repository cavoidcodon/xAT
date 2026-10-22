/**
 * @file    Std_Types.h
 * @brief   AUTOSAR standard types and symbols shared by all BSW modules and the RTE.
 * @details AUTOSAR Classic R21-11, AUTOSAR_SWS_StandardTypes (document 49). Layer: platform.
 *
 *          Not implemented: Std_TransformerForward, Std_TransformerForwardCode and the safety
 *          transformer forward codes (SWS_Std_00027..00029, status "draft"); ComStack_Types.h
 *          (SWS_Std_00030), which comes with the communication stack.
 *
 *          Requirement tags: [SWS_Std_xxxxx] = AUTOSAR_SWS_StandardTypes,
 *          [REQ-003-xx] = .claude/tasks/TASK-003.md section 5.
 */
/* [SWS_Std_00004] no project or supplier specific extension is added to this file */
/* [SWS_Std_00014] protected against multiple inclusion with STD_TYPES_H */
#ifndef STD_TYPES_H
#define STD_TYPES_H

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
/* Std_Types.h includes the platform types and the compiler abstraction, so a module gets all
 * three by including Std_Types.h (SWS_CompilerAbstraction figure 1, include structure). */
#include "Platform_Types.h"
#include "Compiler.h"

/*==================================================================================================
 *                                   PUBLISHED INFORMATION
 *================================================================================================*/
/* [REQ-003-06] published information (SWS_BSWGeneral 10.3) */
#define STD_VENDOR_ID                           (0xFFFFU)   /* xAT: unregistered vendor */
#define STD_MODULE_ID                           (197U)      /* AUTOSAR_TR_BSWModuleList: Std */
#define STD_AR_RELEASE_MAJOR_VERSION            (4U)
#define STD_AR_RELEASE_MINOR_VERSION            (7U)
#define STD_AR_RELEASE_REVISION_VERSION         (0U)
#define STD_SW_MAJOR_VERSION                    (1U)
#define STD_SW_MINOR_VERSION                    (0U)
#define STD_SW_PATCH_VERSION                    (0U)

/*==================================================================================================
 *                                    SYMBOL DEFINITIONS
 *================================================================================================*/
/* [SWS_Std_00006] [REQ-003-04] E_OK / E_NOT_OK, defined exactly as the SWS requires: E_OK and
 * the OSEK StatusType are shared with the OS, so they are guarded by STATUSTYPEDEFINED. The OS
 * (TASK-009) must define StatusType inside the same guard. */
#ifndef STATUSTYPEDEFINED
#define STATUSTYPEDEFINED
#define E_OK 0x00u
typedef unsigned char StatusType; /* OSEK compliance */
#endif
#define E_NOT_OK 0x01u

/* [SWS_Std_00007] [REQ-003-04] physical signal levels */
#define STD_HIGH 0x01u /* Physical state 5V or 3.3V */
#define STD_LOW 0x00u  /* Physical state 0V */

/* [SWS_Std_00013] [REQ-003-04] logical states */
#define STD_ACTIVE 0x01u /* Logical state active */
#define STD_IDLE 0x00u   /* Logical state idle */

/* [SWS_Std_00010] [REQ-003-04] switches of pre-compile parameters */
#define STD_ON 0x01u
#define STD_OFF 0x00u

/* [SWS_Std_00024] [SWS_Std_00025] [SWS_Std_00026] transformer classes (values of
 * Std_TransformerClass); STD_TRANSFORMER_UNSPECIFIED is used when no transformer error occurred */
#define STD_TRANSFORMER_UNSPECIFIED             (0x00U)
#define STD_TRANSFORMER_SERIALIZER              (0x01U)
#define STD_TRANSFORMER_SAFETY                  (0x02U)
#define STD_TRANSFORMER_SECURITY                (0x03U)
#define STD_TRANSFORMER_CUSTOM                  (0xFFU)

/* [SWS_Std_91001] [SWS_Std_00017] message types in client/server communication */
#define STD_MESSAGETYPE_REQUEST                 (0x00U)
#define STD_MESSAGETYPE_RESPONSE                (0x01U)

/* [SWS_Std_91002] [SWS_Std_00019] results of response messages */
#define STD_MESSAGERESULT_OK                    (0x00U)
#define STD_MESSAGERESULT_ERROR                 (0x01U)

/*==================================================================================================
 *                                     TYPE DEFINITIONS
 *================================================================================================*/
/* [SWS_Std_00005] [REQ-003-04] standard API return type shared by the RTE and the BSW.
 * [SWS_Std_00011] E_OK / E_NOT_OK normally; module specific values (prefixed with the module
 * abbreviation) may use the 6 least significant bits, bits 6 and 7 are reserved for the RTE. */
typedef uint8 Std_ReturnType;

/* [SWS_Std_00015] [REQ-003-05] version information returned by <Ma>_GetVersionInfo() */
typedef struct
{
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

/* [SWS_Std_00022] error code of a transformer error; the values are specific to the transformer
 * class (AUTOSAR_ASWS_TransformerGeneral) */
typedef uint8 Std_TransformerErrorCode;

/* [SWS_Std_00023] [SWS_Std_00024] transformer class, underlying type uint8 (values above) */
typedef uint8 Std_TransformerClass;

/* [SWS_Std_00021] transformer error in the context of a transformer chain */
typedef struct
{
    Std_TransformerErrorCode errorCode;
    Std_TransformerClass transformerClass;
} Std_TransformerError;

/* [SWS_Std_91001] [SWS_Std_00017] message type (STD_MESSAGETYPE_*) */
typedef uint8 Std_MessageTypeType;

/* [SWS_Std_91002] [SWS_Std_00019] message result (STD_MESSAGERESULT_*) */
typedef uint8 Std_MessageResultType;

/* [SWS_Std_91003] function pointer that extracts the message type and result from the protocol
 * header of a transformer */
typedef Std_ReturnType (*Std_ExtractProtocolHeaderFieldsType)(const uint8 *buffer,
                                                              uint32 bufferLength,
                                                              Std_MessageTypeType *messageType,
                                                              Std_MessageResultType *messageResult);

#endif /* STD_TYPES_H */
