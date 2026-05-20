/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_sys.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.timer"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#define NU_TIMER_DEVICE(timer) (nu_timer_t)(timer)
#define DEFINE_NU_TIMER(_idx)                   \
    {                                           \
        .m_module = {                           \
            .name = "timer" #_idx,             \
            .base = TIMER##_idx,                \
            .eIRQn = TMR##_idx##_IRQn,          \
            .RstId = TMR##_idx##_RST,           \
            .ModId = TMR##_idx##_MODULE         \
        }                                       \
    }
#define DEFINE_TIMER_IRQ_HANDLER(_idx)            \
void TMR##_idx##_IRQHandler(void)                 \
{                                                 \
    rt_interrupt_enter();                         \
                                                  \
    nu_timer_isr((void *)&nu_timer_arr[TIMER##_idx##_IDX]); \
                                                  \
    rt_interrupt_leave();                         \
}


/* Types / Structures ---------------------------------------------------------*/
enum
{
    TIMER_START = -1,
#if defined(BSP_USING_TIMER0)
    TIMER0_IDX,
#endif
#if defined(BSP_USING_TIMER1)
    TIMER1_IDX,
#endif
#if defined(BSP_USING_TIMER2)
    TIMER2_IDX,
#endif
#if defined(BSP_USING_TIMER3)
    TIMER3_IDX,
#endif
    TIMER_CNT
};

struct nu_timer
{
    rt_clock_timer_t  parent;
    const struct nu_module m_module;
};
typedef struct nu_timer *nu_timer_t;

/* Static Function Prototypes ------------------------------------------------*/
static void nu_timer_init(rt_clock_timer_t *timer, rt_uint32_t state);
static rt_err_t nu_timer_start(rt_clock_timer_t *timer, rt_uint32_t cnt, rt_clock_timer_mode_t opmode);
static void nu_timer_stop(rt_clock_timer_t *timer);
static rt_uint32_t nu_timer_count_get(rt_clock_timer_t *timer);
static rt_err_t nu_timer_control(rt_clock_timer_t *timer, rt_uint32_t cmd, void *args);

/* Static Variables ----------------------------------------------------------*/
static struct nu_timer nu_timer_arr [] =
{
#if defined(BSP_USING_TIMER0)
    DEFINE_NU_TIMER(0),
#endif
#if defined(BSP_USING_TIMER1)
    DEFINE_NU_TIMER(1),
#endif
#if defined(BSP_USING_TIMER2)
    DEFINE_NU_TIMER(2),
#endif
#if defined(BSP_USING_TIMER3)
    DEFINE_NU_TIMER(3),
#endif
};

static struct rt_clock_timer_info nu_timer_info =
{
    12000000,            /* maximum count frequency */
    46875,               /* minimum count frequency */
    0xFFFFFF,            /* the maximum counter value */
    CLOCK_TIMER_CNTMODE_UP,  /* Increment or Decreasing count mode */
};

static struct rt_clock_timer_ops nu_timer_ops =
{
    nu_timer_init,
    nu_timer_start,
    nu_timer_stop,
    nu_timer_count_get,
    nu_timer_control
};

/* Functions Implementation --------------------------------------------------*/
static void nu_timer_init(rt_clock_timer_t *timer, rt_uint32_t state)
{
    nu_timer_t psNuTmr = NU_TIMER_DEVICE(timer);
    RT_ASSERT(psNuTmr != RT_NULL);

    if (1 == state)
    {
        uint32_t timer_clk;
        struct rt_clock_timer_info *info = &nu_timer_info;

        timer_clk = TIMER_GetModuleClock((TIMER_T *)psNuTmr->m_module.base);
        info->maxfreq = timer_clk;
        info->minfreq = timer_clk / 256;
        TIMER_Open((TIMER_T *)psNuTmr->m_module.base, TIMER_ONESHOT_MODE, 1);
        TIMER_EnableInt((TIMER_T *)psNuTmr->m_module.base);
        NVIC_EnableIRQ(psNuTmr->m_module.eIRQn);
    }
    else
    {
        NVIC_DisableIRQ(psNuTmr->m_module.eIRQn);
        TIMER_DisableInt((TIMER_T *)psNuTmr->m_module.base);
        TIMER_Close((TIMER_T *)psNuTmr->m_module.base);
    }
}

static rt_err_t nu_timer_start(rt_clock_timer_t *timer, rt_uint32_t cnt, rt_clock_timer_mode_t opmode)
{
    rt_err_t ret = RT_EINVAL;
    rt_uint32_t u32OpMode;

    nu_timer_t psNuTmr = NU_TIMER_DEVICE(timer);
    RT_ASSERT(psNuTmr != RT_NULL);

    if (cnt <= 1 || cnt > 0xFFFFFF)
    {
        goto exit_nu_timer_start;
    }

    switch (opmode)
    {
    case CLOCK_TIMER_MODE_PERIOD:
        u32OpMode = TIMER_PERIODIC_MODE;
        break;

    case CLOCK_TIMER_MODE_ONESHOT:
        u32OpMode = TIMER_ONESHOT_MODE;
        break;

    default:
        goto exit_nu_timer_start;
    }

    TIMER_SET_CMP_VALUE((TIMER_T *)psNuTmr->m_module.base, cnt);
    TIMER_SET_OPMODE((TIMER_T *)psNuTmr->m_module.base, u32OpMode);
    TIMER_EnableInt((TIMER_T *)psNuTmr->m_module.base);
    NVIC_EnableIRQ(psNuTmr->m_module.eIRQn);

    TIMER_Start((TIMER_T *)psNuTmr->m_module.base);

    ret = RT_EOK;

exit_nu_timer_start:

    return -(ret);
}

static void nu_timer_stop(rt_clock_timer_t *timer)
{
    nu_timer_t psNuTmr = NU_TIMER_DEVICE(timer);
    RT_ASSERT(psNuTmr != RT_NULL);

    NVIC_DisableIRQ(psNuTmr->m_module.eIRQn);
    TIMER_DisableInt((TIMER_T *)psNuTmr->m_module.base);
    TIMER_Stop((TIMER_T *)psNuTmr->m_module.base);
    TIMER_ResetCounter((TIMER_T *)psNuTmr->m_module.base);
}

static rt_uint32_t nu_timer_count_get(rt_clock_timer_t *timer)
{
    nu_timer_t psNuTmr = NU_TIMER_DEVICE(timer);
    RT_ASSERT(psNuTmr != RT_NULL);

    return TIMER_GetCounter((TIMER_T *)psNuTmr->m_module.base);
}

static rt_err_t nu_timer_control(rt_clock_timer_t *timer, rt_uint32_t cmd, void *args)
{
    rt_err_t ret = RT_EOK;
    nu_timer_t psNuTmr = NU_TIMER_DEVICE(timer);
    RT_ASSERT(psNuTmr != RT_NULL);

    switch (cmd)
    {
    case CLOCK_TIMER_CTRL_FREQ_SET:
    {
        uint32_t clk;
        uint32_t pre;

        clk = TIMER_GetModuleClock((TIMER_T *)psNuTmr->m_module.base);
        pre = clk / *((uint32_t *)args) - 1;
        TIMER_SET_PRESCALE_VALUE((TIMER_T *)psNuTmr->m_module.base, pre);
        *((uint32_t *)args) = clk / (pre + 1) ;
    }
    break;

    case CLOCK_TIMER_CTRL_STOP:
        TIMER_Stop((TIMER_T *)psNuTmr->m_module.base);
        break;

    default:
        ret = RT_EINVAL;
        break;
    }

    return -(ret);
}

/**
 * All UART interrupt service routine
 */
static void nu_timer_isr(nu_timer_t psNuTmr)
{
    RT_ASSERT(psNuTmr != RT_NULL);

    if (TIMER_GetIntFlag((TIMER_T *)psNuTmr->m_module.base))
    {
        TIMER_ClearIntFlag((TIMER_T *)psNuTmr->m_module.base);
        rt_clock_timer_isr(&psNuTmr->parent);
    }
}

static int rt_hw_timer_init(void)
{
    int i;
    rt_err_t ret = RT_EOK;
    for (i = (TIMER_START + 1); i < TIMER_CNT; i++)
    {
        CLK_EnableModuleClock(nu_timer_arr[i].m_module.ModId);

        SYS_ResetModule(nu_timer_arr[i].m_module.RstId);

        /* Register Timer information. */
        nu_timer_arr[i].parent.info = &nu_timer_info;

        /* Register Timer operation. */
        nu_timer_arr[i].parent.ops = &nu_timer_ops;

        /* Register RT clock_timer device. */
        ret = rt_clock_timer_register(&nu_timer_arr[i].parent, nu_timer_arr[i].m_module.name, &nu_timer_arr[i]);
        RT_ASSERT(ret == RT_EOK);
    }
    return 0;
}

INIT_BOARD_EXPORT(rt_hw_timer_init);
#if defined(BSP_USING_TIMER0)
    DEFINE_TIMER_IRQ_HANDLER(0)
#endif

#if defined(BSP_USING_TIMER1)
    DEFINE_TIMER_IRQ_HANDLER(1)
#endif

#if defined(BSP_USING_TIMER2)
    DEFINE_TIMER_IRQ_HANDLER(2)
#endif

#if defined(BSP_USING_TIMER3)
    DEFINE_TIMER_IRQ_HANDLER(3)
#endif
