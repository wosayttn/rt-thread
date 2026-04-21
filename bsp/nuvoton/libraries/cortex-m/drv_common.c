/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "rtthread.h"
#include "board.h"
#include "drv_sys.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.common"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

/* Types / Structures ---------------------------------------------------------*/

/* Static Function Prototypes ------------------------------------------------*/

/* Static Variables ----------------------------------------------------------*/

/* Functions Implementation --------------------------------------------------*/
/**
 * This function will initial.
 */
rt_weak void rt_hw_board_init(void)
{
    /* Init System/modules clock */
    void nutool_modclkcfg_init();
    nutool_modclkcfg_init();

    /* Init all pin function setting */
    void nutool_pincfg_init(void);
    nutool_pincfg_init();

    /* Configure SysTick */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

#if defined(RT_USING_HEAP)
    rt_system_heap_init(HEAP_BEGIN, HEAP_END);
#endif /* RT_USING_HEAP */

#if defined(RT_USING_MEMHEAP) && defined(BSP_USING_LPSRAM)
    extern int rt_hw_lpsram_init(void);
    rt_hw_lpsram_init();
#endif

#if defined(BSP_USING_UART)
    rt_err_t rt_hw_uart_init(void);
    rt_hw_uart_init();
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#if defined(RT_USING_COMPONENTS_INIT)
    rt_components_board_init();
#endif
}

/**
 * The time delay function.
 *
 * @param microseconds.
 */
void rt_hw_us_delay(rt_uint32_t us)
{
    rt_uint32_t ticks;
    rt_uint32_t told, tnow, tcnt = 0;
    rt_uint32_t reload = SysTick->LOAD;

    ticks = us * reload / (1000000 / RT_TICK_PER_SECOND);
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

/**
 * This is the timer interrupt service routine.
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}


/**
 * This function will reboot the system.
 */
int reboot(int argc, char **argv)
{
    SYS_ResetChip();

    return 0;
}
MSH_CMD_EXPORT(reboot, Reboot System);

#if defined(RT_USING_CACHE) && defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
rt_weak void rt_hw_cpu_dcache_ops(int ops, void *addr, int size)
{
    if ((ops & (RT_HW_CACHE_FLUSH | RT_HW_CACHE_INVALIDATE)) == (RT_HW_CACHE_FLUSH | RT_HW_CACHE_INVALIDATE))
    {
        SCB_CleanInvalidateDCache_by_Addr(addr, size);
    }
    else if (ops & RT_HW_CACHE_FLUSH)
    {
        SCB_CleanDCache_by_Addr(addr, size);
    }
    else if (ops & RT_HW_CACHE_INVALIDATE)
    {
        SCB_InvalidateDCache_by_Addr(addr, size);
    }
}
#endif