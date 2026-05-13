/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if (defined(BSP_USING_EPWM) && defined(RT_USING_PWM))

#include "rtdevice.h"
#include "NuMicro.h"
#define NUPWMCNR             48000
#define NUPWMCMR             4800

static void epwm_dump_test_setting(void)
{
    rt_kprintf("\n[EPWM utest] description\n");
    rt_kprintf("  purpose               : Verify EPWM period, pulse and channel control.\n");
    rt_kprintf("  target period (ns)    : %d\n", NUPWMCNR);
    rt_kprintf("  target pulse (ns)     : %d\n", NUPWMCMR);
    rt_kprintf("  channel count         : %d\n", EPWM_CHANNEL_NUM);
    rt_kprintf("  tolerance (sec/tick)  : %d / %d\n\n", 100, 1);
}

static void cal_epwm_convert(rt_uint32_t *u32nupwmcheckcnr, rt_uint32_t *u32nupwmcheckcmr)
{
    rt_uint32_t u32EPWMClockSrc, u32Prescale, i;
    rt_uint32_t u32Frequency, u32DutyCycle;
    rt_uint32_t u32nupwmcnr, u32nupwmcmr;

    u32nupwmcnr = NUPWMCNR;
    u32nupwmcmr = NUPWMCMR;
    u32DutyCycle = (u32nupwmcmr * 100) / u32nupwmcnr;
    u32Frequency = 1000000000 / u32nupwmcnr;

    u32EPWMClockSrc = CLK_GetPCLK0Freq();

    for (u32Prescale = 1U; u32Prescale < 0xFFFU; u32Prescale++)  /* prescale could be 0~0xFFF */
    {
        i = (u32EPWMClockSrc / u32Frequency) / u32Prescale;
        /* If target value is larger than CNR, need to use a larger prescaler */
        if (i < (0x10000U))
        {
            *u32nupwmcheckcnr = i;
            break;
        }
    }

    *u32nupwmcheckcmr = u32DutyCycle * ((*u32nupwmcheckcnr) + 1U) / 100U;
}

static void test_epwmout(EPWM_T *epwm, struct rt_device_pwm *epwm_dev)
{
    const int tolerance_sec = 100, tolerance_tick = 1;
    struct rt_pwm_configuration epwm_check_config = {0};
    rt_uint32_t u32nupwmch = 0, u32nupwmcheckcnr = 0, u32nupwmcheckcmr = 0;

    u32nupwmcheckcnr = 0;
    u32nupwmcheckcmr = 0;
    cal_epwm_convert(&u32nupwmcheckcnr, &u32nupwmcheckcmr);

    for (u32nupwmch = 0; u32nupwmch < EPWM_CHANNEL_NUM; u32nupwmch++)
    {
        uassert_true(rt_pwm_set(epwm_dev, u32nupwmch, NUPWMCNR, NUPWMCMR) == RT_EOK);
        epwm_check_config.channel = u32nupwmch;
        rt_device_control(&epwm_dev->parent, PWM_CMD_GET, &epwm_check_config);
        uassert_in_range(epwm_check_config.period, NUPWMCNR - tolerance_sec, NUPWMCNR + tolerance_sec);
        uassert_in_range(epwm_check_config.pulse, NUPWMCMR - tolerance_sec, NUPWMCMR + tolerance_sec);
        uassert_in_range(epwm->PERIOD[u32nupwmch], u32nupwmcheckcnr - tolerance_tick, u32nupwmcheckcnr + tolerance_tick);
        uassert_in_range(epwm->CMPDAT[u32nupwmch], u32nupwmcheckcmr - tolerance_tick, u32nupwmcheckcmr + tolerance_tick);
        uassert_true(rt_pwm_enable(epwm_dev, u32nupwmch) == RT_EOK);
        uassert_true(rt_pwm_disable(epwm_dev, u32nupwmch) == RT_EOK);
    }

    /* Test over max channel */
    uassert_true(rt_pwm_enable(epwm_dev, EPWM_CHANNEL_NUM) != RT_EOK);
    for (u32nupwmch = 0; u32nupwmch < EPWM_CHANNEL_NUM; u32nupwmch++)
    {
        uassert_true(rt_pwm_set(epwm_dev, u32nupwmch, 0, 0) != RT_EOK);
    }
}

static void test_epwm(void)
{
    rt_device_t epwm_dev;
    EPWM_T *epwm;
    char szdev[8];
    int i = 0;
    while (1)
    {
        rt_snprintf(szdev, sizeof(szdev), "epwm%d", i);
        epwm_dev = rt_device_find(szdev);
        if (epwm_dev == RT_NULL)
            break;
        epwm = (EPWM_T *)(EPWM0_BASE + (i * 0x1000));
        test_epwmout(epwm, (struct rt_device_pwm *)epwm_dev);
        i++;
    }
}

static rt_err_t utest_tc_init(void)
{
    epwm_dump_test_setting();
    return RT_EOK;
}


static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_epwm);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"epwm", utest_tc_init, utest_tc_cleanup, 5);
#endif
