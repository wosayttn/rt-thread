/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(RT_USING_RTC) && defined(BSP_USING_RTC)

#include "sys/time.h"
#include "rtdevice.h"
#include "NuMicro.h"


#define ENCODE_TM(tm, year, mon, mday, hour, min, sec)      \
{                                                           \
    (tm).tm_year = (year) - 1900;                           \
    (tm).tm_mon  = (mon) - 1;                               \
    (tm).tm_mday = (mday);                                  \
    (tm).tm_hour = (hour);                                  \
    (tm).tm_min  = (min);                                   \
    (tm).tm_sec  = (sec);                                   \
}

static rt_device_t rtc;

static void rtc_dump_test_setting(void)
{
    rt_kprintf("\n[RTC utest] test configuration\n");
    rt_kprintf("  purpose               : Verify RTC timekeeping, corner cases and alarm flow.\n");
    rt_kprintf("  rtc device name       : rtc\n");
    rt_kprintf("  rtc device found      : %s\n", rtc ? "yes" : "no");
#if defined(RT_USING_ALARM)
    rt_kprintf("  alarm support         : enabled\n");
#else
    rt_kprintf("  alarm support         : disabled\n");
#endif
    rt_kprintf("  test cases            : time, corner");
#if defined(RT_USING_ALARM)
    rt_kprintf(", alarm");
#endif
    rt_kprintf("\n\n");
}

#if defined(RT_USING_ALARM)
    static rt_err_t alarm_compare(struct rt_alarm_setup *setup, struct rt_rtc_wkalarm *wkalarm);
    static void test_rtc_alarm(void);
#endif

static void test_rtc_time(void)
{
    struct tm tm;
    time_t tw = 0, tr = 0;

    ENCODE_TM(tm, 2020, 2, 10, 15, 15, 30);
    tw = timegm(&tm);

    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &tw) == RT_EOK);
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_GET_TIME, &tr) == RT_EOK);
    uassert_true(tw == tr);
}

#if defined(RT_USING_ALARM)
/*
 * Compare the programmed alarm setting with the value read back from RTC.
 */
static rt_err_t alarm_compare(struct rt_alarm_setup *setup, struct rt_rtc_wkalarm *wkalarm)
{
    return (((wkalarm->tm_sec == setup->wktime.tm_sec) &&
             (wkalarm->tm_min == setup->wktime.tm_min) &&
             (wkalarm->tm_hour == setup->wktime.tm_hour)) ? RT_EOK : RT_ERROR);
}

static volatile uint32_t bWaitAlarmNotify = 0;

static void testcase_rt_alarm_cb(rt_alarm_t alarm, time_t timestamp)
{
#if defined(RT_USING_PM)
    /* Request normal mode */
    rt_pm_request(PM_SLEEP_MODE_NONE);
#endif

    bWaitAlarmNotify = 1;

    rt_kprintf("[%s/%d] alarm=0x%08x, %08x\n", __func__, rt_tick_get(), alarm, timestamp);
}

/*
 * Verify RTC alarm callback flow and optional PM wakeup behavior.
 */
static void test_rtc_alarm(void)
{
    struct rt_rtc_wkalarm wkalarm;
    struct rt_alarm_setup alarm_setup;
    struct rt_alarm *alarm;

    struct tm tm;
    time_t tw = 0;

    rt_memset(&alarm_setup.wktime, RT_ALARM_TM_NOW, sizeof(struct tm));

    ENCODE_TM(tm, 2024, 5, 28, 15, 15, 0); // Set now date/time
    ENCODE_TM(alarm_setup.wktime, 2024, 5, 28, 15, 15, 10);  // Set wake up date/time

    tw = timegm(&tm);
    rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &tw);

    alarm_setup.flag = RT_ALARM_ONESHOT;

    alarm = rt_alarm_create(testcase_rt_alarm_cb, &alarm_setup);

    bWaitAlarmNotify = 0;

    if (alarm && (rt_alarm_start(alarm) == RT_EOK))
    {
        rt_kprintf("Sleep 10 seconds for waiting alarm occurred.\n");

        rt_device_control(rtc, RT_DEVICE_CTRL_RTC_GET_ALARM, &wkalarm);
        if (alarm_compare(&alarm_setup, &wkalarm) == RT_EOK)
        {
#if defined(RT_USING_PM)
            rt_uint8_t mode = PM_SLEEP_MODE_DEEP;
            rt_pm_request(mode);

            /* Release all boxes */
            for (int i = (mode - 1); i >= 0; i--)
                rt_pm_release_all(i);

            /* Enter idle task to do pm_sleep. */
            rt_thread_mdelay(10);

            /* Now, system enter Deep mode and wait-up from RTC alarm. */
#endif
        }

        /* Wait notification from alarm callback. */
        while (!bWaitAlarmNotify);

        uassert_true(rt_alarm_stop(alarm) == RT_EOK);
        rt_alarm_delete(alarm);

    }

}
#endif


/*
 * Verify RTC boundary handling near the supported minimum and maximum time.
 */
static void test_rtc_corner(void)
{
    time_t tr, tmax, tmin, t1, t2, t3, t4;
    struct tm tm_min, tm_max;


    ENCODE_TM(tm_min, 2000, 1, 1, 0, 0, 0);
    ENCODE_TM(tm_max, 2038, 1, 19, 3, 14, 07);

    tmin = timegm(&tm_min);
    tmax = timegm(&tm_max);

    t1 = tmin + 1;
    t2 = tmax - 1;
    t3 = tmin - 1;
    t4 = tmax + 1;

    //LOG_D("t1: %s", ctime(&t1));
    //LOG_D("t2: %s", ctime(&t2));
    //LOG_D("t3: %s", ctime(&t3));
    //LOG_D("t4: %s", ctime(&t4));

    /* t1 : case allowed to access. time is min + 1 */
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &t1) == RT_EOK);
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_GET_TIME, &tr) == RT_EOK);
    uassert_true(tr == t1);
    LOG_D("%x=%x", tr, t1);


    /* t2 : case allowed to access. time is max - 1 */
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &t2) == RT_EOK);
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_GET_TIME, &tr) == RT_EOK);
    uassert_true(tr == t2);
    LOG_D("%x=%x", tr, t2);


    /* t3 : case denied access. time is min - 1. tr keep at t2. */
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &t3) == -RT_ERROR);
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_GET_TIME, &tr) == RT_EOK);
    uassert_true(tr == t2);
    LOG_D("%x=%x", tr, t2);

    /* t4 : case denied access. time is max + 1 */
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &t4) == -RT_ERROR);
    uassert_true(rt_device_control(rtc, RT_DEVICE_CTRL_RTC_GET_TIME, &tr) == RT_EOK);
    uassert_true(tr == t2);
    LOG_D("%x=%x", tr, t2);

}

static rt_err_t utest_tc_init(void)
{
#if defined(RT_USING_ALARM)
    rt_thread_t tid;
#endif

    rtc = rt_device_find("rtc");
    uassert_not_null(rtc);

#if defined(RT_USING_ALARM)
    tid = rt_thread_find("alarmsvc");

    if (tid == NULL)
        rt_alarm_system_init();

    tid = rt_thread_find("alarmsvc");
    rt_kprintf("[RTC utest] alarm service    : %s\n", tid ? "ready" : "missing");
#endif

    rtc_dump_test_setting();

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
/*
 * Verify RTC time can be written and read back consistently.
 */
    UTEST_UNIT_RUN(test_rtc_time);
#if defined(RT_USING_ALARM)
    UTEST_UNIT_RUN(test_rtc_alarm);
#endif
    UTEST_UNIT_RUN(test_rtc_corner);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"rtc",
                utest_tc_init, utest_tc_cleanup, 10);

#endif /* RT_USING_RTC && BSP_USING_RTC */
