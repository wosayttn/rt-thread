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

#define UTEST_CLOCK_TIMER_ONESHOT_USEC    20000
#define UTEST_CLOCK_TIMER_PERIODIC_USEC   30000
#define UTEST_CLOCK_TIMER_ONESHOT_WAIT_MS 60
#define UTEST_CLOCK_TIMER_PERIOD_WAIT_MS  120
#define UTEST_CLOCK_TIMER_STOP_WAIT_MS    20

static volatile rt_uint32_t g_timer_timeout_count;

static rt_tick_t clocktime_ms_to_tick(rt_uint32_t ms)
{
    return ((ms * RT_TICK_PER_SECOND) + 999) / 1000;
}

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

    rt_kprintf("  oneshot timeout (us)  : %d\n", UTEST_CLOCK_TIMER_ONESHOT_USEC);
    rt_kprintf("  period timeout (us)   : %d\n", UTEST_CLOCK_TIMER_PERIODIC_USEC);
    rt_kprintf("  supported modes       : oneshot + periodic\n\n");
}

static rt_err_t timer_timeout_cb(rt_device_t dev, rt_size_t size)
{
    RT_UNUSED(dev);
    RT_UNUSED(size);

    g_timer_timeout_count ++;

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

    g_timer_timeout_count = 0;
    timer_timeout.sec = 0;
    timer_timeout.usec = UTEST_CLOCK_TIMER_ONESHOT_USEC;
    uassert_true(rt_device_write(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));
    rt_thread_delay(clocktime_ms_to_tick(UTEST_CLOCK_TIMER_ONESHOT_WAIT_MS));
    uassert_int_equal(g_timer_timeout_count, 1);

    timer_mode = CLOCK_TIMER_MODE_PERIOD;
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_MODE_SET, &timer_mode) == RT_EOK);

    g_timer_timeout_count = 0;
    timer_timeout.sec = 0;
    timer_timeout.usec = UTEST_CLOCK_TIMER_PERIODIC_USEC;
    uassert_true(rt_device_write(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));

    rt_thread_delay(clocktime_ms_to_tick(UTEST_CLOCK_TIMER_PERIOD_WAIT_MS));
    uassert_true(g_timer_timeout_count >= 2);

    //timer read
    uassert_true(rt_device_read(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));
    rt_kprintf("Read before stop: Sec = %d, Usec = %d, callback count = %d\n",
               timer_timeout.sec,
               timer_timeout.usec,
               g_timer_timeout_count);

    //timer stop
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_STOP, &timer_mode) == RT_EOK);
    uassert_true(rt_device_control(timer_dev, CLOCK_TIMER_CTRL_STOP, RT_NULL) == RT_EOK);
    rt_thread_delay(clocktime_ms_to_tick(UTEST_CLOCK_TIMER_STOP_WAIT_MS));
    uassert_true(g_timer_timeout_count >= 2);

    //timer read
    uassert_true(rt_device_read(timer_dev, 0, &timer_timeout, sizeof(timer_timeout)) == sizeof(timer_timeout));
    rt_kprintf("Read after stop: Sec = %d, Usec = %d\n", timer_timeout.sec, timer_timeout.usec);

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
                utest_tc_init, utest_tc_cleanup, 5);

#endif   //#if (defined(BSP_USING_TIMER))