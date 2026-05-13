/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_WDT)

#include "rtdevice.h"
#include "NuMicro.h"

/*---------------------------------------------------------------------------*/
/* open this macro to test the wdt reset cases.                              */
/*---------------------------------------------------------------------------*/
#define NU_UTEST_ENABLE_WDT_RESET_STANDALONE

/*---------------------------------------------------------------------------*/

#if defined(RT_USING_PM) && defined(BSP_USING_CLK)
    #define NU_UTEST_PM_OPS
#endif

static rt_device_t wdt;

static void wdt_dump_test_setting(void)
{
    rt_kprintf("\n[WDT utest] description\n");
    rt_kprintf("  purpose               : Verify watchdog timeout, keepalive and PM interaction.\n");
    rt_kprintf("  device name           : wdt\n");
    rt_kprintf("  default timeout (s)   : %d\n", 5);
#if defined(NU_UTEST_PM_OPS)
    rt_kprintf("  pm operation tests    : enabled\n");
#else
    rt_kprintf("  pm operation tests    : disabled\n");
#endif
#if defined(NU_UTEST_ENABLE_WDT_RESET_STANDALONE)
    rt_kprintf("  reset starvation test : enabled\n\n");
#else
    rt_kprintf("  reset starvation test : disabled\n\n");
#endif
}

#if defined(NU_UTEST_PM_OPS)
    static void test_wdt_pm_suspend_resume(void);
    static void test_wdt_pm_frequency_change(void);
#endif

static void test_wdt_control(void)
{
    const int tolerance_sec = 1;
    int i, set_sec = 5, get_sec = 0;

    /* Test set/get timeout */
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &set_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &get_sec) == RT_EOK);
    uassert_true(set_sec == get_sec);

    /* Test get time left */
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_SET_TIMEOUT,  &set_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);

    /* Test start */
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_START, RT_NULL) == RT_EOK);

    for (i = set_sec; i > 0; i--)
    {
        /* Test feed dog */
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL) == RT_EOK);
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
        uassert_in_range(get_sec, i - tolerance_sec, i + tolerance_sec);
        LOG_I("time_left: %d", get_sec);

        rt_thread_mdelay(RT_TICK_PER_SECOND - 1);
    }

    /* Test stop */
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_STOP, RT_NULL) == RT_EOK);
}


static void test_wdt_corner(void)
{
    int set_sec = 0;

    /* Test time=0 protect */
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &set_sec) == (-RT_EINVAL));
}


#if defined(NU_UTEST_ENABLE_WDT_RESET_STANDALONE)
static void test_wdt_starve_and_wait_reset(void)
{
    int i, get_sec, set_sec = 5;

    /* Test starve dog */
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &set_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &get_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_START, RT_NULL) == RT_EOK);

    for (i = (set_sec + 1); i > 0; i--)
    {
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
        LOG_W("wdt will reset in next %d seconds", get_sec);

        rt_thread_mdelay(RT_TICK_PER_SECOND - 1);
    }

    /* The code should not reach here. Assert fail anyway. */
    uassert_true(0);
}
#endif

#if defined(NU_UTEST_PM_OPS)

static void test_wdt_pm_suspend_resume(void)
{
    int tolerance_sec = 1, set_sec = 10, sleep_at_sec = 6;
    int i, get_sec, last_sec;
    rt_uint8_t mode;

    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &set_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &get_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_START, RT_NULL) == RT_EOK);

    for (i = set_sec; i > sleep_at_sec ; i--)
    {
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL) == RT_EOK);
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
        uassert_in_range(get_sec, i - tolerance_sec, i + tolerance_sec);
        LOG_I("time_left: %d", get_sec);

        rt_thread_mdelay(RT_TICK_PER_SECOND - 1);
    }

    /* test wdt pm suspend(). */
    LOG_W("enter pm sleep for 5 seconds");

    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
    last_sec = get_sec;

    /* short delay, wait for the uassert printf compelete. */
    rt_thread_mdelay(RT_TICK_PER_SECOND / 10);

    mode = PM_SLEEP_MODE_DEEP;
    rt_pm_request(mode);
    rt_thread_mdelay(5 * RT_TICK_PER_SECOND - 1);
    rt_pm_release(mode);

    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
    uassert_in_range(get_sec, last_sec - tolerance_sec, last_sec + tolerance_sec);

    LOG_I("time_left:%d.  last:%d", get_sec, last_sec);

    /* test wdt pm resume() */
    for (i = get_sec; i > 0 ; i--)
    {
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL) == RT_EOK);
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
        uassert_in_range(get_sec, i - tolerance_sec, i + tolerance_sec);
        LOG_I("time_left: %d", get_sec);

        rt_thread_mdelay(RT_TICK_PER_SECOND - 1);
    }

    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_STOP, RT_NULL) == RT_EOK);
}

static void test_wdt_pm_frequency_change(void)
{
    rt_uint8_t mode_1, mode_2;
    int i, get_sec, tolerance_sec = 1, set_sec = 10, change_freq_at_sec = 5;


    CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_HCLK_DIV2048, 0);


    mode_1 = PM_RUN_MODE_HIGH_SPEED;
    mode_2 = PM_RUN_MODE_LOW_SPEED;

    rt_pm_run_enter(mode_1);

    /* let idle thread run and change to high frequency */
    rt_thread_mdelay(RT_TICK_PER_SECOND / 10);

    LOG_I("cpu freq: %d Hz", CLK_GetCPUFreq());
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &set_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &get_sec) == RT_EOK);
    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_START, RT_NULL) == RT_EOK);


    for (i = get_sec; i > 0 ; i--)
    {
        /* Test feed dog */
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL) == RT_EOK);
        uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, &get_sec) == RT_EOK);
        uassert_in_range(get_sec, i - tolerance_sec, i + tolerance_sec);
        LOG_I("time_left: %d", get_sec);

        if (i == change_freq_at_sec)
        {
            LOG_W("pm run mode frequency change.");
            rt_pm_run_enter(mode_2);
        }

        rt_thread_mdelay(RT_TICK_PER_SECOND - 1);
        LOG_I("cpu freq: %d Hz", CLK_GetCPUFreq());
    }

    uassert_true(rt_device_control(wdt, RT_DEVICE_CTRL_WDT_STOP, RT_NULL) == RT_EOK);

    /* resume to default */
    rt_pm_run_enter(RT_PM_DEFAULT_RUN_MODE);
    rt_thread_mdelay(10);
}
#endif

static rt_err_t utest_tc_init(void)
{
    wdt_dump_test_setting();
    wdt = rt_device_find("wdt");
    rt_device_open(wdt, RT_DEVICE_FLAG_RDWR);

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_device_close(wdt);

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_wdt_control);
    UTEST_UNIT_RUN(test_wdt_corner);

#if defined(NU_UTEST_PM_OPS)
    UTEST_UNIT_RUN(test_wdt_pm_suspend_resume);
    UTEST_UNIT_RUN(test_wdt_pm_frequency_change);
#endif

#if defined(NU_UTEST_ENABLE_WDT_RESET_STANDALONE)
    UTEST_UNIT_RUN(test_wdt_starve_and_wait_reset);
#endif
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"wdt",
                utest_tc_init, utest_tc_cleanup, 10);
#endif


