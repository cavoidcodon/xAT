/**
 * @file    main.c
 * @brief   xAT firmware entry (integration layer, FRDM-MCXN947 cm33_core0).
 * @details Initialises the board through the SDK board port, prints the boot banner on the
 *          debug console (MCU-Link VCOM, 115200 8N1) and idles. EcuM takes over the start-up
 *          sequence in TASK-010. The integration layer may use the NXP SDK (layering.md).
 *
 *          Requirement tags: [REQ-003-xx] = .claude/tasks/TASK-003.md section 5,
 *          [SWS_Platform_xxxxx] = AUTOSAR_SWS_PlatformTypes.
 */

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
/* [REQ-003-08] the platform headers (Std_Types.h -> Platform_Types.h, Compiler.h, Compiler_Cfg.h)
 * compile on the target with the strict project flags */
#include "Std_Types.h"

#include "fsl_debug_console.h"
#include "app.h"

/*==================================================================================================
 *                                TARGET PLATFORM TYPE CHECKS
 *================================================================================================*/
/* The host unit tests check the same properties on the host (LP64); these checks prove them for
 * the target compiler (ILP32) at every firmware build. */

/* [SWS_Platform_00013] [SWS_Platform_00014] [SWS_Platform_00015] [SWS_Platform_00066] [REQ-003-01] */
_Static_assert((sizeof(uint8) == 1U) && (sizeof(uint16) == 2U) && (sizeof(uint32) == 4U) &&
                   (sizeof(uint64) == 8U),
               "unsigned AUTOSAR integer types must have exact widths on the target");

/* [SWS_Platform_00016] [SWS_Platform_00017] [SWS_Platform_00018] [SWS_Platform_00067] [REQ-003-01] */
_Static_assert((sizeof(sint8) == 1U) && (sizeof(sint16) == 2U) && (sizeof(sint32) == 4U) &&
                   (sizeof(sint64) == 8U),
               "signed AUTOSAR integer types must have exact widths on the target");

/* [SWS_Platform_00013] [SWS_Platform_00016] [REQ-003-01] signedness on the target */
_Static_assert(((uint32)0U - 1U) > 0U, "uint32 must be unsigned on the target");
_Static_assert((sint32)-1 < 0, "sint32 must be signed on the target");

/* [SWS_Platform_00027] [REQ-003-02] boolean is 8-bit unsigned on the target */
_Static_assert((sizeof(boolean) == 1U) && ((boolean)(-1) > (boolean)0), "boolean must be 8-bit unsigned");

/* [SWS_Platform_00045] [SWS_Platform_00051] [REQ-003-03] the symbols match the target compiler */
_Static_assert((CPU_TYPE == CPU_TYPE_32) && (sizeof(void *) == 4U), "Cortex-M33 is a 32-bit CPU");
_Static_assert((CPU_BYTE_ORDER == LOW_BYTE_FIRST) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__),
               "the MCXN947 is little endian");

/*==================================================================================================
 *                                     LOCAL MACROS
 *================================================================================================*/
#define ECU_BOOT_BANNER "xAT ecu: boot OK\r\n"

/*==================================================================================================
 *                                     GLOBAL FUNCTIONS
 *================================================================================================*/
/* MISRA C:2012 Directive 4.6 / coding_rules.md 3.1 deviation: the C standard requires 'int' as
 * the return type of main(). */
int main(void)
{
    BOARD_InitHardware();

    (void)PRINTF(ECU_BOOT_BANNER);

    /* TASK-010: EcuM_Init() is called here and takes over the start-up sequence. */

    /* coding_rules.md 7.7 deviation: the idle loop of the firmware entry has no exit by design. */
    for (;;)
    {
    }
}
