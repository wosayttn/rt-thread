/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(RT_USING_CLOCK_TIME) && defined(BSP_USING_TIMER)

#include "rtdevice.h"
#include "drivers/clock_time.h"

static const char *const g_timer_dev_names[] =
{
    "timer0",
    "timer1",
    "timer2",
    "timer3",
};

static void clocktime_dump_test_setting(void)
{
    int index;

    rt_kprintf("\n[CLOCK_TIME utest] description\n");
    rt_kprintf("  purpose               : Verify hardware timer timeout, frequency and mode APIs.\n");

    for (index = 0; index < sizeof(g_timer_dev_names) / sizeof(g_timer_dev_names[0]); index++)
    {
        rt_device_t timer_dev = rt_device_find(g_timer_dev_names[index]);

        rt_kprintf("  %-20s: %s\n",
                   g_timer_dev_names[index],
                   timer_dev ? "found" : "not found, skip");
    }

    rt_kprintf("  supported modes       : oneshot + periodic\n\n");
}

static rt_err_t timer_timeout_cb(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("enter hardware timer isr\n");

    return RT_EOK;
}

static void test_timeout(rt_device_t timer_dev)
{
    rt_clock_timerval_t timer_timeout;
    rt_clock_timer_mode_t timer_mode;
    int timer_freq;
    struct rt_clock_timer_info timer_info;

    rt_kprintf("Testing %s Start\n", timer_dev->parent.name);

    uassert_true(rt_device_open(timer_dev, RT_DEVICE_OFLAG_RDWR) == RT_EOK);

    //timer callback
    rt_device_set_rx_indicate(timer_dev, timer_timeout_cb);

    //timer get info test
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_INFO_GET, RT_NULL) != RT_EOK);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_INFO_GET, &timer_info) == RT_EOK);

    //timer set freq test
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, RT_NULL) != RT_EOK);
    timer_freq = (timer_info.maxfreq + 1);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, &timer_freq) != RT_EOK);
    timer_freq = (timer_info.minfreq - 1);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, &timer_freq) != RT_EOK);
    timer_freq = (timer_info.maxfreq - 1);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, &timer_freq) == RT_EOK);
    uassert_in_range(timer_freq, timer_info.minfreq, timer_info.maxfreq);
    timer_freq = (timer_info.minfreq + 1);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, &timer_freq) == RT_EOK);
    uassert_in_range(timer_freq, timer_info.minfreq, timer_info.maxfreq);
    timer_freq = (timer_info.minfreq);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, &timer_freq) == RT_EOK);
    uassert_in_range(timer_freq, timer_info.minfreq, timer_info.maxfreq);
    timer_freq = (timer_info.maxfreq);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_FREQ_SET, &timer_freq) == RT_EOK);
    uassert_in_range(timer_freq, timer_info.minfreq, timer_info.maxfreq);

    //timer set mode test
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, RT_NULL) != RT_EOK);
    timer_freq = (CLOCK_TIMER_MODE_ONESHOT - 1);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_freq) != RT_EOK);
    timer_freq = (CLOCK_TIMER_MODE_PERIOD + 1);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_freq) != RT_EOK);
    timer_freq = CLOCK_TIMER_MODE_PERIOD;
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_freq) == RT_EOK);
    timer_freq = CLOCK_TIMER_MODE_ONESHOT;
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_freq) == RT_EOK);
    timer_mode = CLOCK_TIMER_MODE_PERIOD;
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_mode) == RT_EOK);
    timer_mode = CLOCK_TIMER_MODE_ONESHOT;
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_mode) == RT_EOK);

    //timer set timeout start run test
    uassert_true(rt_device_write(timer_dev, 0, &timer_mode, sizeof(timer_mode)) == 0);

    timer_timeout.sec = 4;
    timer_timeout.usec = 0;
    uassert_true(rt_device_write(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));

    rt_thread_delay(5 * RT_TICK_PER_SECOND);//wait 5 sec for isr stop

    timer_timeout.sec = 0;
    timer_timeout.usec = 123456;
    uassert_true(rt_device_write(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));
    rt_thread_delay(1 * RT_TICK_PER_SECOND);//wait 1 sec for oneshot stop

    timer_mode = CLOCK_TIMER_MODE_PERIOD;
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_mode) == RT_EOK);

    timer_timeout.sec = 3;
    timer_timeout.usec = 7654321;
    uassert_true(rt_device_write(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));

    //timer read
    rt_thread_delay(5 * RT_TICK_PER_SECOND);//wait 5 sec for user stop
    uassert_true(rt_device_read(timer_dev, 0, &timer_timeout.sec, sizeof(timer_timeout.sec)) == sizeof(timer_timeout.sec));
    rt_kprintf("wait 5 sec Read: Sec = %d\n", timer_timeout.sec);

    //timer stop
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_STOP, &timer_mode) == RT_EOK);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_STOP, RT_NULL) == RT_EOK);

    //timer read
    uassert_true(rt_device_read(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));
    rt_kprintf("Read: Sec = %d, Usec = %d\n", timer_timeout.sec, timer_timeout.usec);

    rt_thread_delay(1 * RT_TICK_PER_SECOND);//wait 1 sec for read CNT

    //timer read
    uassert_true(rt_device_read(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));
    rt_kprintf("Read: Sec = %d, Usec = %d\n", timer_timeout.sec, timer_timeout.usec);

    uassert_true(rt_device_close(timer_dev) == RT_EOK);

    rt_kprintf("Testing %s Done\n", timer_dev->parent.name);
}

static void test_timer_by_name(const char *timer_name)
{
    rt_device_t timer_dev = rt_device_find(timer_name);

    if (timer_dev == RT_NULL)
    {
        rt_kprintf("Skip %s: device not found\n", timer_name);
        return;
    }

    test_timeout(timer_dev);
}

static void test_timer(void)
{
    int index;

    for (index = 0; index < sizeof(g_timer_dev_names) / sizeof(g_timer_dev_names[0]); index++)
    {
        test_timer_by_name(g_timer_dev_names[index]);
    }

}

static rt_err_t utest_tc_init(void)
{
    clocktime_dump_test_setting();
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_timer);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"timer",
                utest_tc_init, utest_tc_cleanup, 1);

#endif   //#if (defined(BSP_USING_TIMER))