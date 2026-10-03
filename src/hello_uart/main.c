/*
 * xAT hello_uart - bring-up smoke test for FRDM-MCXN947 (cm33_core0).
 *
 * Prints "xAT hello_uart: boot OK" once on the debug UART (LPFLEXCOMM4 / MCU-Link VCOM,
 * 115200 8N1), then "xAT tick <n>" about once per second.
 *
 * NOTE: This is a bring-up program, NOT AUTOSAR application code. As an approved
 * exception to the CLAUDE.md layering rule it includes NXP SDK headers directly
 * (TASK-001 section 5.4).
 */

#include <stdint.h>

#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"

/* Delay between two tick messages, in microseconds. */
#define HELLO_TICK_PERIOD_US (1000000U)

/* Number of tick messages printed since boot; inspected from GDB. */
volatile uint32_t g_tickCount = 0U;

int main(void)
{
    BOARD_InitHardware();

    (void)PRINTF("xAT hello_uart: boot OK\r\n");

    for (;;)
    {
        SDK_DelayAtLeastUs(HELLO_TICK_PERIOD_US, SystemCoreClock);
        g_tickCount++;
        (void)PRINTF("xAT tick %u\r\n", (unsigned int)g_tickCount);
    }
}
