/**
 * @file    check_TrueFalsePredefined.c
 * @brief   Compile-only check: TRUE / FALSE defined before Platform_Types.h are not redefined.
 * @details Without the #ifndef guards of Platform_Types.h, the different spelling below would
 *          raise a macro redefinition warning, which -Werror turns into a build failure.
 */

/* [SWS_Platform_00054] [SWS_Platform_00056] in-built (predefined) TRUE / FALSE are kept */
#define TRUE (1)
#define FALSE (0)

#include "Platform_Types.h"

typedef boolean CompileCheck_TrueFalseType;
