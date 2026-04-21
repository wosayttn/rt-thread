/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "drv_sys.h"
#include "sys/time.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.rtc"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#define CONV_TO_TM_YEAR(year)           ((year) - 1900)
#define CONV_TO_TM_MON(mon)             ((mon) - 1)

#define CONV_FROM_TM_YEAR(tm_year)      ((tm_year) + 1900)
#define CONV_FROM_TM_MON(tm_mon)        ((tm_mon) + 1)

#define RTC_TM_UPPER_BOUND                                              \
{   .tm_year = CONV_TO_TM_YEAR(2038),                                   \
    .tm_mon  = CONV_TO_TM_MON(1),                                       \
    .tm_mday  = 19,                                                     \
    .tm_hour  = 3,                                                      \
    .tm_min = 14,                                                       \
    .tm_sec  = 07,                                                      \
}
#define RTC_TM_LOWER_BOUND                                              \
{   .tm_year = CONV_TO_TM_YEAR(2000),                                   \
    .tm_mon  = CONV_TO_TM_MON(1),                                       \
    .tm_mday  = 1,                                                      \
    .tm_hour  = 0,                                                      \
    .tm_min = 0,                                                        \
    .tm_sec  = 0,                                                       \
}

/* Types / Structures ---------------------------------------------------------*/

/* Static Function Prototypes ------------------------------------------------*/
static rt_err_t nu_rtc_init(void);
static rt_err_t nu_rtc_get_secs(time_t *sec);
static rt_err_t nu_rtc_set_secs(time_t *sec);
static rt_err_t nu_rtc_get_alarm(struct rt_rtc_wkalarm *alarm);
static rt_err_t nu_rtc_set_alarm(struct rt_rtc_wkalarm *alarm);
static rt_err_t nu_rtc_is_date_valid(const time_t t);
#if defined(RT_USING_ALARM)
static void nu_rtc_alarm_reset(void);
#endif

/* Static Variables ----------------------------------------------------------*/
static const struct rt_rtc_ops nu_rtc_ops =
{
    .init = nu_rtc_init,
    .get_secs = nu_rtc_get_secs,
    .set_secs = nu_rtc_set_secs,
    .get_alarm = nu_rtc_get_alarm,
    .set_alarm = nu_rtc_set_alarm,
    .get_timeval = RT_NULL,
    .set_timeval = RT_NULL,
};

static rt_rtc_dev_t device_rtc;

/* Functions Implementation --------------------------------------------------*/

static rt_err_t nu_rtc_init(void)
{
    /* hw rtc initialise */
    if (RTC_Open(NULL) < 0)
        return -RT_ERROR;

    RTC_DisableInt(RTC_INTEN_ALMIEN_Msk | RTC_INTEN_TICKIEN_Msk);
#if defined(RT_USING_ALARM)
    nu_rtc_alarm_reset();
    RTC_EnableInt(RTC_INTEN_ALMIEN_Msk);
    NVIC_EnableIRQ(RTC_IRQn);
#endif

    return RT_EOK;
}
#if defined(RT_USING_ALARM)
/* Reset alarm settings to avoid the unwanted values remain in rtc registers. */
static void nu_rtc_alarm_reset(void)
{
    S_RTC_TIME_DATA_T alarm;

    /* Reset alarm time and calendar. */
    alarm.u32Year       = RTC_YEAR2000;
    alarm.u32Month      = 0;
    alarm.u32Day        = 0;
    alarm.u32Hour       = 0;
    alarm.u32Minute     = 0;
    alarm.u32Second     = 0;
    alarm.u32TimeScale  = RTC_CLOCK_24;

    RTC_SetAlarmDateAndTime(&alarm);

    /* Reset alarm time mask and calendar mask. */
    RTC_SetAlarmDateMask(0, 0, 0, 0, 0, 0);
    RTC_SetAlarmTimeMask(0, 0, 0, 0, 0, 0);

    /* Clear alarm flag for safe */
    RTC_CLEAR_ALARM_INT_FLAG(RTC);
}
#endif

static rt_err_t nu_rtc_is_date_valid(const time_t t)
{
    static struct tm tm_upper = RTC_TM_UPPER_BOUND;
    static struct tm tm_lower = RTC_TM_LOWER_BOUND;
    static time_t t_upper, t_lower;
    static rt_bool_t initialised = RT_FALSE;

    if (!initialised)
    {
        t_upper = timegm((struct tm *)&tm_upper);
        t_lower = timegm((struct tm *)&tm_lower);
        initialised = RT_TRUE;
    }
    if ((t > t_upper) || (t < t_lower))
        return -(RT_EINVAL);

    return RT_EOK;
}

static rt_err_t nu_rtc_get_secs(time_t *sec)
{
    struct tm tm_out;
    S_RTC_TIME_DATA_T hw_time;

    if (sec == NULL)
        return -(RT_EINVAL);

    RTC_GetDateAndTime(&hw_time);

    tm_out.tm_year = CONV_TO_TM_YEAR(hw_time.u32Year);
    tm_out.tm_mon  = CONV_TO_TM_MON(hw_time.u32Month);
    tm_out.tm_mday = hw_time.u32Day;
    tm_out.tm_hour = hw_time.u32Hour;
    tm_out.tm_min  = hw_time.u32Minute;
    tm_out.tm_sec  = hw_time.u32Second;
    *sec = timegm(&tm_out);

    return RT_EOK;
}

static rt_err_t nu_rtc_set_secs(time_t *sec)
{
    struct tm tm_in;
    S_RTC_TIME_DATA_T hw_time;

    if (sec == NULL)
        return -(RT_EINVAL);

    if (nu_rtc_is_date_valid(*sec) != RT_EOK)
        return -(RT_ERROR);

    gmtime_r(sec, &tm_in);
    hw_time.u32Year = CONV_FROM_TM_YEAR(tm_in.tm_year);
    hw_time.u32Month = CONV_FROM_TM_MON(tm_in.tm_mon);
    hw_time.u32Day = tm_in.tm_mday;
    hw_time.u32Hour = tm_in.tm_hour;
    hw_time.u32Minute = tm_in.tm_min;
    hw_time.u32Second = tm_in.tm_sec;
    hw_time.u32TimeScale = RTC_CLOCK_24;
    hw_time.u32AmPm = 0;

    RTC_SetDateAndTime(&hw_time);
    return RT_EOK;
}

static rt_err_t nu_rtc_get_alarm(struct rt_rtc_wkalarm *alarm)
{
#if defined(RT_USING_ALARM)
    S_RTC_TIME_DATA_T hw_alarm;

    if (alarm == NULL)
        return -(RT_EINVAL);

    RTC_GetAlarmDateAndTime(&hw_alarm);

    alarm->tm_hour = hw_alarm.u32Hour;
    alarm->tm_min  = hw_alarm.u32Minute;
    alarm->tm_sec  = hw_alarm.u32Second;
    return RT_EOK;
#else
    return -(RT_ENOSYS);
#endif
}

static rt_err_t nu_rtc_set_alarm(struct rt_rtc_wkalarm *alarm)
{
#if defined(RT_USING_ALARM)
    S_RTC_TIME_DATA_T hw_alarm;

    if (alarm == NULL)
        return -(RT_EINVAL);

    hw_alarm.u32Year       = RTC_YEAR2000;
    hw_alarm.u32Month      = 0;
    hw_alarm.u32Day        = 0;
    hw_alarm.u32Hour       = alarm->tm_hour;
    hw_alarm.u32Minute     = alarm->tm_min;
    hw_alarm.u32Second     = alarm->tm_sec;
    hw_alarm.u32TimeScale  = RTC_CLOCK_24;

    RTC_SetAlarmDateMask(1, 1, 1, 1, 1, 1);
    RTC_SetAlarmDateAndTime(&hw_alarm);
    return RT_EOK;
#else
    return -(RT_ENOSYS);
#endif
}

#if defined(NU_RTC_SUPPORT_MSH_CMD)
/* Support "rtc_set_date" command line in msh mode */
static rt_err_t msh_rtc_set_date(int argc, char **argv)
{
    rt_uint32_t index, len, arg[3];

    rt_memset(arg, 0, sizeof(arg));
    len = (argc >= 4) ? 4 : argc;

    for (index = 0; index < (len - 1); index++)
    {
        arg[index] = atol(argv[index + 1]);
    }

    return set_date(arg[0], arg[1], arg[2]);
}
MSH_CMD_EXPORT_ALIAS(msh_rtc_set_date, rtc_set_date, e.g: rtc_set_date 2026 1 20);

/* Support "rtc_set_time" command line in msh mode */
static rt_err_t msh_rtc_set_time(int argc, char **argv)
{
    rt_uint32_t index, len, arg[3];

    rt_memset(arg, 0, sizeof(arg));
    len = (argc >= 4) ? 4 : argc;

    for (index = 0; index < (len - 1); index++)
    {
        arg[index] = atol(argv[index + 1]);
    }

    return set_time(arg[0], arg[1], arg[2]);
}
MSH_CMD_EXPORT_ALIAS(msh_rtc_set_time, rtc_set_time, e.g: rtc_set_time 18 30 00);
#endif

/* rtc device driver initialise. */
int rt_hw_rtc_init(void)
{
    rt_err_t ret;

    device_rtc.ops = &nu_rtc_ops;
    ret = rt_hw_rtc_register(&device_rtc, "rtc", RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (ret == RT_EOK)
        ret = rt_device_init(&device_rtc.parent);

    return (int)ret;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

/* rtc interrupt entry */
void RTC_IRQHandler(void)
{
    rt_interrupt_enter();

    if (RTC_GET_TICK_INT_FLAG(RTC))
    {
        RTC_CLEAR_TICK_INT_FLAG(RTC);
    }
#if defined(RT_USING_ALARM)
    if (RTC_GET_ALARM_INT_FLAG(RTC))
    {
        RTC_CLEAR_ALARM_INT_FLAG(RTC);

        /* Send an alarm event to notify rt-thread alarm service. */
        rt_alarm_update(&device_rtc.parent, 0);
    }
#endif

    rt_interrupt_leave();
}
