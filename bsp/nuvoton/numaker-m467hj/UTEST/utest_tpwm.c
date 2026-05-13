/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if (defined(BSP_USING_TPWM) && defined(RT_USING_PWM))

#include "rtdevice.h"
#include "NuMicro.h"

#define NU_TPWM_PERIOD      48000 /* unit:ns 1ns~4.29s:1Ghz~0.23hz */
#define NU_TPWM_PULSE       4800  /* unit:ns (pulse<=period) */

static void tpwm_dump_test_setting(void)
{
    rt_kprintf("\n[TPWM utest] description\n");
    rt_kprintf("  purpose               : Verify timer-PWM period, pulse and channel control.\n");
    rt_kprintf("  target period (ns)    : %d\n", NU_TPWM_PERIOD);
    rt_kprintf("  target pulse (ns)     : %d\n", NU_TPWM_PULSE);
    rt_kprintf("  pwm channel count     : %d\n", 1);
    rt_kprintf("  tolerance (sec/tick)  : %d / %d\n\n", 100, 1);
}

static void cal_tpwm_convert(TIMER_T *tpwm, rt_uint32_t *pu32PwmCheckPeriod, rt_uint32_t *pu32PwmCheckCmpdat)
{
    rt_uint32_t u32TPWMClockFreq, u32Prescale;
    rt_uint32_t u32Frequency, u32DutyCycle;
    rt_uint32_t u32PwmPeriod, u32PwmCmpdat;

    u32PwmPeriod = NU_TPWM_PERIOD;
    u32PwmCmpdat = NU_TPWM_PULSE;
    u32DutyCycle = (u32PwmCmpdat * 100) / u32PwmPeriod;
    u32Frequency = 1000000000 / u32PwmPeriod;

    u32TPWMClockFreq = TIMER_GetModuleClock(tpwm);

    for (u32Prescale = 1U; u32Prescale < 0xFFFU; u32Prescale++)  /* prescale could be 0~0xFFF */
    {
        u32PwmPeriod = (u32TPWMClockFreq / u32Frequency) / u32Prescale;
        /* If target value is larger than CNR, need to use a larger prescaler */
        if (u32PwmPeriod < (0x10000U))
        {
            *pu32PwmCheckPeriod = u32PwmPeriod;
            break;
        }
    }

    *pu32PwmCheckCmpdat = u32DutyCycle * ((*pu32PwmCheckPeriod) + 1U) / 100U;
}

static void test_tpwmout(TIMER_T *tpwm, struct rt_device_pwm *tpwm_dev)
{
    const int tolerance_sec = 100, tolerance_tick = 1;
    struct rt_pwm_configuration tpwm_check_config = {0};
    rt_uint32_t u32PwmCh = 0, u32PwmCheckPeriod = 0, u32PwmCheckCmpdat = 0;

    rt_kprintf("tpwm=%08x\n", (uint32_t)tpwm);

    cal_tpwm_convert(tpwm, &u32PwmCheckPeriod, &u32PwmCheckCmpdat);

    for (u32PwmCh = 0; u32PwmCh < 1; u32PwmCh++)
    {
        uassert_true(rt_pwm_set(tpwm_dev, u32PwmCh, NU_TPWM_PERIOD, NU_TPWM_PULSE) == RT_EOK);

        tpwm_check_config.channel = u32PwmCh;
        uassert_true(rt_device_control(&tpwm_dev->parent, PWM_CMD_GET, &tpwm_check_config) == RT_EOK);

        uassert_in_range(tpwm_check_config.period, NU_TPWM_PERIOD - tolerance_sec, NU_TPWM_PERIOD + tolerance_sec);
        uassert_in_range(tpwm_check_config.pulse, NU_TPWM_PULSE - tolerance_sec, NU_TPWM_PULSE + tolerance_sec);

        uassert_in_range(tpwm->PWMPERIOD, u32PwmCheckPeriod - tolerance_tick, u32PwmCheckPeriod + tolerance_tick);
        uassert_in_range(tpwm->PWMCMPDAT, u32PwmCheckCmpdat - tolerance_tick, u32PwmCheckCmpdat + tolerance_tick);

        uassert_true(rt_pwm_enable(tpwm_dev, u32PwmCh) == RT_EOK);
        uassert_true(rt_pwm_disable(tpwm_dev, u32PwmCh) == RT_EOK);
    }

    /* Test over max channel */
    u32PwmCh = 2;
    uassert_true(rt_pwm_enable(tpwm_dev, u32PwmCh) != RT_EOK);

    /* Test error Period */
    for (u32PwmCh = 0; u32PwmCh < 2; u32PwmCh++)
    {
        uassert_true(rt_pwm_set(tpwm_dev, u32PwmCh, 0, 0) != RT_EOK);
    }
}

static void test_tpwm(void)
{
    rt_device_t tpwm_dev;
    TIMER_T *tpwm;
    char szdev[8];
    int i = 0;
    while (1)
    {
        rt_snprintf(szdev, sizeof(szdev), "tpwm%d", i);
        tpwm_dev = rt_device_find(szdev);
        if (tpwm_dev == RT_NULL)
            break;

//#define TIMER0_BASE             (APBPERIPH_BASE + 0x10000UL)
//#define TIMER1_BASE             (APBPERIPH_BASE + 0x10100UL)
//#define TIMER2_BASE             (APBPERIPH_BASE + 0x11000UL)
//#define TIMER3_BASE             (APBPERIPH_BASE + 0x11100UL)

        tpwm = (TIMER_T *)(TIMER0_BASE + ((i / 2) * 0x1000) + ((i % 2) * 0x100));
        rt_kprintf("%d %08x\n", i, tpwm);
        test_tpwmout(tpwm, (struct rt_device_pwm *)tpwm_dev);
        i++;
    }
}

static rt_err_t utest_tc_init(void)
{
    tpwm_dump_test_setting();
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_tpwm);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"tpwm", utest_tc_init, utest_tc_cleanup, 1);

#endif //#if (defined(BSP_USING_TPWM) && defined(RT_USING_PWM))
