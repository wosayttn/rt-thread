/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>

#if defined(NU_PKG_USING_RV3032C7)

#include <rtdevice.h>
#include <sys/time.h>

#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME  "rv3032c7"
#define DBG_COLOR
#include <rtdbg.h>

#define RV3032_ADDR          0x51
#define TIME_ARRAY_LENGTH    8 // Total number of writable values in device

#define BCDtoDEC(val)        ((( (val) / 0x10) * 10 ) + ( (val) % 0x10 ))
#define DECtoBCD(val)        ((( (val) / 10 ) * 0x10 ) + ( (val) % 10 ))

#define RV3032_HUNDREDTHS            0x00
#define RV3032_SECONDS               0x01
#define RV3032_MINUTES               0x02
#define RV3032_HOURS                 0x03
#define RV3032_WEEKDAYS              0x04
#define RV3032_DATE                  0x05
#define RV3032_MONTHS                0x06
#define RV3032_YEARS                 0x07

enum time_order
{
    TIME_HUNDREDTHS,    // 0
    TIME_SECONDS,       // 1
    TIME_MINUTES,       // 2
    TIME_HOURS,         // 3
    TIME_WEEKDAY,       // 4
    TIME_DATE,          // 5
    TIME_MONTH,         // 6
    TIME_YEAR,          // 7
};

struct rtc_rv3032c7
{
    struct rt_device  dev;
    uint32_t u32MagicNum;
    struct rt_i2c_bus_device *i2cbus;
} ;
typedef struct rtc_rv3032c7  *rtc_rv3032c7_t;

#define DEF_MAGIC_NUM  0x94879487

static int rv3032c7_i2c_write(struct rt_i2c_bus_device *i2cbus, uint8_t *pu8Data,  uint8_t u8Len)
{
    struct rt_i2c_msg msg;

    RT_ASSERT(i2cbus);
    RT_ASSERT(pu8Data);
    RT_ASSERT(u8Len);

    msg.addr  = RV3032_ADDR;                  /* Slave address */
    msg.flags = RT_I2C_WR;                    /* Write flag */
    msg.buf   = (rt_uint8_t *)&pu8Data[0];  /* Slave register address */
    msg.len   = u8Len;                        /* Number of bytes sent */

    if (rt_i2c_transfer(i2cbus, &msg, 1) != 1)
    {
        rt_kprintf("[Failed] addr=%x\n", pu8Data[0]);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static int rv3032c7_i2c_read(struct rt_i2c_bus_device *i2cbus, uint8_t *pu8Data,  uint8_t u8Len)
{
    struct rt_i2c_msg msgs[2];

    RT_ASSERT(i2cbus);
    RT_ASSERT(pu8Data);
    RT_ASSERT(u8Len);

    msgs[0].addr  = RV3032_ADDR;                /* Slave address */
    msgs[0].flags = RT_I2C_WR;                  /* Write flag */
    msgs[0].buf   = (rt_uint8_t *)&pu8Data[0];  /* Number of bytes sent */
    msgs[0].len   = 1;                          /* Number of bytes read */

    msgs[1].addr  = RV3032_ADDR;                /* Slave address */
    msgs[1].flags = RT_I2C_RD;                  /* Read flag */
    msgs[1].buf   = (rt_uint8_t *)&pu8Data[1] ; /* Read data pointer */
    msgs[1].len   = u8Len;                      /* Number of bytes read */

    if (rt_i2c_transfer(i2cbus, &msgs[0], 2) != 2)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static int rv3032c7_eeprom_pmu_bsm_init(void)
{
    /*
    00 Switchover Disabled. – Default value on delivery
    01 Enables the Direct Switching Mode (DSM). Switchover when VDD < VBACKUP (PMU selects pin with the greater voltage (VDD or VBACKUP)
    10 Enables the Level Switching Mode (LSM). Switchover when VDD < VTH:LSM (2.0 V) AND VBACKUP > VTH:LSM (2.0 V). When VDD < VTH:LSM (2.0 V), PMU is in DSM Mode.
    11 Switchover Disabled
    */
#define BSM_Pos                    (4)
#define BSM_Msk                    (0x3 << BSM_Pos)
#define BSM_SWITCHOVER_DISABLED    (0 << BSM_Pos)
#define BSM_SWITCHING_DSM          (1 << BSM_Pos)
#define BSM_SWITCHING_LSM          (2 << BSM_Pos)

#define RTC_BSM_REG                 0xc0

    uint8_t pmu[4] = {0};
    rtc_rv3032c7_t rtc;

    rtc = (rtc_rv3032c7_t)rt_device_find("rtc");
    if (!rtc || (rtc->u32MagicNum != DEF_MAGIC_NUM))
    {
        rtc = (rtc_rv3032c7_t)rt_device_find("rv3032c7");
        if (!rtc || (rtc->u32MagicNum != DEF_MAGIC_NUM))
        {
            LOG_E("Failed to find RV3032C7");
            return -1;
        }
    }

    pmu[0] = RTC_BSM_REG; // PMU register address

    if (RT_EOK != rv3032c7_i2c_read(rtc->i2cbus, &pmu[0],  1))
    {
        LOG_E("Failed to read PMU register");
        return -1;
    }

    LOG_I("PMU register is %02x", pmu[1]);

    if ((pmu[1] & BSM_Msk) != BSM_SWITCHING_DSM)
    {
        /* Enables the Level Switching Mode (LSM).*/
        LOG_I("Enables the Level Switching Mode (LSM)");
        pmu[1] = (pmu[1] & ~BSM_Msk) | BSM_SWITCHING_DSM;

        if (RT_EOK != rv3032c7_i2c_write(rtc->i2cbus, &pmu[0], 2))
        {
            LOG_E("Failed to write PMU register");
            return -1;
        }
    }

    return 0;
}
INIT_APP_EXPORT(rv3032c7_eeprom_pmu_bsm_init);
MSH_CMD_EXPORT(rv3032c7_eeprom_pmu_bsm_init, RV3032 PMU);


static rt_err_t rv3032c7_set_time(rtc_rv3032c7_t rtc, time_t *time)
{
    struct tm sTm;
    uint8_t payload[TIME_ARRAY_LENGTH] = {0};

    if (*time < 946684800)    // 2000-01-01 00:00:00
    {
        return -RT_ERROR;
    }

    gmtime_r(time, &sTm);

    payload[0]            = RV3032_SECONDS;
    payload[TIME_SECONDS] = DECtoBCD(sTm.tm_sec);
    payload[TIME_MINUTES] = DECtoBCD(sTm.tm_min);
    payload[TIME_HOURS]   = DECtoBCD(sTm.tm_hour);
    payload[TIME_WEEKDAY] = 1 << sTm.tm_wday;
    payload[TIME_DATE]    = DECtoBCD(sTm.tm_mday);
    payload[TIME_MONTH]   = DECtoBCD(sTm.tm_mon + 1);
    payload[TIME_YEAR]    = DECtoBCD(sTm.tm_year - 100);

    // Set RTC
    return rv3032c7_i2c_write(rtc->i2cbus, &payload[0], TIME_ARRAY_LENGTH);
}

static rt_err_t rv3032c7_get_time(rtc_rv3032c7_t rtc, time_t *time)
{
    struct tm sTm;
    uint8_t payload[TIME_ARRAY_LENGTH] = {0};

    payload[0]  = RV3032_SECONDS;
    if (rv3032c7_i2c_read(rtc->i2cbus, &payload[0], TIME_ARRAY_LENGTH - 1) != RT_EOK)
    {
        goto exit_rv3032c7_get_time;
    }

    sTm.tm_isdst = -1;
    sTm.tm_yday = 0;
    sTm.tm_wday = 0;
    sTm.tm_sec  = BCDtoDEC(payload[TIME_SECONDS]);
    sTm.tm_min  = BCDtoDEC(payload[TIME_MINUTES]);
    sTm.tm_hour = BCDtoDEC(payload[TIME_HOURS]);
    sTm.tm_mday = BCDtoDEC(payload[TIME_DATE]);
    sTm.tm_mon  = BCDtoDEC(payload[TIME_MONTH]) - 1;
    sTm.tm_year = BCDtoDEC(payload[TIME_YEAR]) + 100;

    /* Return time */
    *time = timegm(&sTm);

    return RT_EOK;

exit_rv3032c7_get_time:

    return -RT_ERROR;
}

/* Register rt-thread device.control() entry. */
static rt_err_t rtc_rv3032c7_control(rt_device_t dev, int cmd, void *args)
{
    if (!dev || !args)
        return -(RT_EINVAL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
        return rv3032c7_get_time((rtc_rv3032c7_t)dev, (time_t *)args);
        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
        return rv3032c7_set_time((rtc_rv3032c7_t)dev, (time_t *)args);

    default:
        return -RT_EINVAL;
    }

    return RT_EOK;
}

int rt_hw_rv3032c7_init(const char *i2c_dev)
{
    rtc_rv3032c7_t rtc;

    RT_ASSERT(i2c_dev);

    rtc = (rtc_rv3032c7_t)rt_malloc(sizeof(struct rtc_rv3032c7));
    if (rtc == RT_NULL)
    {
        LOG_E("Can't allocate memory for rv3032c7 over %s.\n", i2c_dev);
        RT_ASSERT(rtc);
        goto exit_rt_hw_rv3032c7_init;
    }

    /* Find I2C bus */
    rtc->i2cbus = (struct rt_i2c_bus_device *)rt_device_find(i2c_dev);
    if (rtc->i2cbus == RT_NULL)
    {
        LOG_E("Can't found I2C bus - %s..!\n", i2c_dev);
        RT_ASSERT(rtc->i2cbus);
        goto exit_rt_hw_rv3032c7_init;
    }

    rt_memset((void *)&rtc->dev, 0, sizeof(struct rt_device));
    rtc->dev.type = RT_Device_Class_RTC;
    rtc->dev.control = rtc_rv3032c7_control;

    if (rt_device_register(&rtc->dev, "rtc", RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        LOG_E("Can't register rv3032 to rtc device name!, try to give rv3032c7");
        if (rt_device_register(&rtc->dev, "rv3032c7", RT_DEVICE_FLAG_RDWR) != RT_EOK)
        {
            LOG_E("Can't register rv3032 to rv3032c7 device name!");
            RT_ASSERT(0);
            goto exit_rt_hw_rv3032c7_init;
        }
    }

    rtc->u32MagicNum = DEF_MAGIC_NUM;

    return RT_EOK;

exit_rt_hw_rv3032c7_init:

    return -RT_ERROR;
}

#endif //#if defined(NU_PKG_USING_RV3032C7)
