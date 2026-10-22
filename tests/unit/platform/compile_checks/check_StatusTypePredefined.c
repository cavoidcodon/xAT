/**
 * @file    check_StatusTypePredefined.c
 * @brief   Compile-only check: an OSEK OS header that defined StatusType and E_OK first does not
 *          clash with Std_Types.h.
 * @details Without the STATUSTYPEDEFINED guard of Std_Types.h, the different spelling of E_OK
 *          below would raise a macro redefinition warning, which -Werror turns into a failure.
 */

/* [SWS_Std_00006] E_OK and StatusType shared with the OS: the OS defines them first.
 * coding_rules.md 3.1: 'unsigned char' imitates the OSEK definition, as the SWS writes it. */
#define STATUSTYPEDEFINED
typedef unsigned char StatusType;
#define E_OK ((StatusType)0x00U)

#include "Std_Types.h"

typedef Std_ReturnType CompileCheck_StatusTypeType;
