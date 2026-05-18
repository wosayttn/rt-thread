/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-12-16     Wayne        first version
 */

#include "st_lps22hh_sensor_v1.h"

#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME  "sensor.st.lps22hh"
#define DBG_COLOR
#include <rtdbg.h>

#include "lps22hh_reg.h"
#include "board.h"

static uint8_t whoamI = 0x00;
//static struct rt_i2c_bus_device *i2c_dev_acce;
static LPS22HH_Object_t lps22hh_obj;

static int32_t rt_func_ok(void)
{
    return 0;
}

static int32_t get_tick(void)
{
    return rt_tick_get();
}

#if 0
static int32_t rt_i2c_write_reg(uint16_t address, uint16_t reg, const uint8_t *data, uint16_t len)
{
    static uint8_t reg_buf[16] = {0};
    struct rt_i2c_msg msg;

    reg_buf[0] = (uint8_t)reg;
    rt_memcpy(&reg_buf[1], data, len);

    msg.addr  = address;                      /* Slave address */
    msg.flags = RT_I2C_WR;                    /* Write flag */
    msg.buf   = (rt_uint8_t *)reg_buf;        /* Slave register address */
    msg.len   = 1 + len;                      /* Number of bytes sent */

    if (rt_i2c_transfer(i2c_dev_acce, &msg, 1) != 1)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static int32_t rt_i2c_read_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr  = address;                  /* Slave address */
    msgs[0].flags = RT_I2C_WR;                /* Write flag */
    msgs[0].buf   = (rt_uint8_t *)&reg;       /* Slave register address */
    msgs[0].len   = 1;                        /* Number of bytes sent */

    msgs[1].addr  = address;                  /* Slave address */
    msgs[1].flags = RT_I2C_RD;                /* Read flag without READ_ACK */
    msgs[1].buf   = (rt_uint8_t *)&data[0];   /* Read data pointer */
    msgs[1].len   = len;                      /* Number of bytes read */

    if (rt_i2c_transfer(i2c_dev_acce, msgs, 2) != 2)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}
#else

int32_t nu_i3c_init(void);
int32_t nu_i3c_write_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t len);
int32_t nu_i3c_read_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t len);

#endif

static rt_err_t _lps22hh_init(struct rt_sensor_intf *intf)
{
    rt_uint8_t  id;
    LPS22HH_IO_t io_ctx;

    if (RT_EOK != nu_i3c_init())
    {
        LOG_E("I3C init failed");
        return -RT_ERROR;
    }

    /* Configure the accelero driver */
    io_ctx.BusType     = LPS22HH_I3C_BUS; /* I3C */
    io_ctx.Address     = 0x5D;          //SA0 pin is connected to VDD.
    io_ctx.Init        = rt_func_ok;
    io_ctx.DeInit      = rt_func_ok;
    io_ctx.ReadReg     = nu_i3c_read_reg;
    io_ctx.WriteReg    = nu_i3c_write_reg;
    io_ctx.GetTick     = get_tick;

    if (LPS22HH_RegisterBusIO(&lps22hh_obj, &io_ctx) != LPS22HH_OK)
    {
        return -RT_ERROR;
    }
    else if (LPS22HH_ReadID(&lps22hh_obj, &id) != LPS22HH_OK)
    {
        LOG_E("read id failed");
        return -RT_ERROR;
    }

    LOG_D("baro id:%d", id);

    if (LPS22HH_Init(&lps22hh_obj) != LPS22HH_OK)
    {
        LOG_E("baro init failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t _lps22hh_set_odr(rt_sensor_t sensor, rt_uint16_t odr)
{
    if (sensor->info.type == RT_SENSOR_CLASS_BARO)
    {
        LPS22HH_PRESS_SetOutputDataRate(&lps22hh_obj, odr);
        LOG_D("baro set odr %d", odr);
    }

    return RT_EOK;
}

static rt_err_t _lps22hh_acc_set_mode(rt_sensor_t sensor, rt_uint8_t mode)
{
    if (mode == RT_SENSOR_MODE_POLLING)
    {
        lps22hh_fifo_mode_set(&lps22hh_obj.Ctx, LPS22HH_BYPASS_MODE);
        LOG_D("set mode to POLLING");
    }
    else
    {
        LOG_D("Unsupported mode, code is %d", mode);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t _lps22hh_set_power(rt_sensor_t sensor, rt_uint8_t power)
{
    if (power == RT_SENSOR_POWER_DOWN)
    {
        if (sensor->info.type == RT_SENSOR_CLASS_BARO)
        {
            LPS22HH_PRESS_Disable(&lps22hh_obj);
        }

        LOG_D("set power down");
    }
    else if (power == RT_SENSOR_POWER_NORMAL)
    {
        if (sensor->info.type == RT_SENSOR_CLASS_BARO)
        {
            LPS22HH_PRESS_Enable(&lps22hh_obj);
        }

        LOG_D("set power normal");
    }
    else
    {
        LOG_W("Unsupported mode, code is %d", power);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static RT_SIZE_TYPE _lps22hh_polling_get_data(rt_sensor_t sensor, rt_sensor_data_t data)
{
    if (sensor->info.type == RT_SENSOR_CLASS_BARO)
    {
        float value;

        if (LPS22HH_OK == LPS22HH_PRESS_GetPressure(&lps22hh_obj, &value))
        {
            data->type = RT_SENSOR_CLASS_BARO;
            data->data.baro = value;
            data->timestamp = rt_sensor_get_ts();

            return 1;
        }
    }

    return 0;
}

static RT_SIZE_TYPE _lps22hh_fifo_get_data(rt_sensor_t sensor, struct rt_sensor_data *data, rt_size_t len)
{
    rt_uint8_t i;

    for (i = 0; i < len; i++)
    {
        float value;

        if (LPS22HH_OK == LPS22HH_PRESS_GetPressure(&lps22hh_obj, &value))
        {
            data[i].type = RT_SENSOR_CLASS_BARO;
            data[i].data.baro = value;
            data[i].timestamp = rt_sensor_get_ts();
        }
        else
            break;
    }

    return i;
}

static RT_SIZE_TYPE _lps22hh_interrupt_get_data(rt_sensor_t sensor, struct rt_sensor_data *data)
{
    /**
     * @TODO add interrupt method
     *
     */
    return RT_EOK;
}

static RT_SIZE_TYPE _lps22hh_get_id(void *args)
{
    LPS22HH_ReadID(&lps22hh_obj, args);
    if ((*(uint8_t *)args) != LPS22HH_ID)
    {
        LOG_D("Err: no sensor! whoamI = %d!\r\n", args);
        return -1;
    }
    return RT_EOK;
}

static RT_SIZE_TYPE lps22hh_fetch_data(struct rt_sensor_device *sensor, rt_sensor_data_t buf, rt_size_t len)
{
    RT_ASSERT(buf);

    if (sensor->config.mode == RT_SENSOR_MODE_POLLING)
    {
        return _lps22hh_polling_get_data(sensor, buf);
    }
    else if (sensor->config.mode == RT_SENSOR_MODE_INT)
    {
        return _lps22hh_interrupt_get_data(sensor, buf);
    }
    else if (sensor->config.mode == RT_SENSOR_MODE_FIFO)
    {
        return _lps22hh_fifo_get_data(sensor, buf, len);
    }
    else
        return 0;
}

static rt_err_t lps22hh_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;

    switch (cmd)
    {
    case RT_SENSOR_CTRL_GET_ID:
        result = _lps22hh_get_id(args);
        break;
    case RT_SENSOR_CTRL_SET_ODR:
        result = _lps22hh_set_odr(sensor, (rt_uint32_t)args & 0xffff);
        break;
    case RT_SENSOR_CTRL_SET_MODE:
        result = _lps22hh_acc_set_mode(sensor, (rt_uint32_t)args & 0xff);
        break;
    case RT_SENSOR_CTRL_SET_POWER:
        result = _lps22hh_set_power(sensor, (rt_uint32_t)args & 0xff);
        break;
    case RT_SENSOR_CTRL_SELF_TEST:
        break;
    default:
        return -RT_ERROR;
    }
    return result;
}

static struct rt_sensor_ops sensor_ops =
{
    lps22hh_fetch_data,
    lps22hh_control
};

int rt_hw_lps22hh_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor_baro = RT_NULL;

    /* accelerometer sensor register */
    {
        sensor_baro = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_baro == RT_NULL)
            return -1;

        sensor_baro->info.type       = RT_SENSOR_CLASS_BARO;
        sensor_baro->info.vendor     = RT_SENSOR_VENDOR_STM;
        sensor_baro->info.model      = "lps22hh";
        sensor_baro->info.unit       = RT_SENSOR_UNIT_PA;
        sensor_baro->info.intf_type  = RT_SENSOR_INTF_I2C;
        //sensor_baro->info.range_max  = SENSOR_ACC_RANGE_16G;
        //sensor_baro->info.range_min  = SENSOR_ACC_RANGE_2G;
        sensor_baro->info.period_min = 5;

        rt_memcpy(&sensor_baro->config, cfg, sizeof(struct rt_sensor_config));
        sensor_baro->ops = &sensor_ops;

        result = rt_hw_sensor_register(sensor_baro, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (result != RT_EOK)
        {
            LOG_E("device register err code: %d", result);
            goto __exit;
        }
    }

    result = _lps22hh_init(&cfg->intf);
    if (result != RT_EOK)
    {
        LOG_E("_lps22hh init err code: %d", result);
        goto __exit;
    }

    LOG_I("lps22hh init success");
    return RT_EOK;

__exit:
    if (sensor_baro != RT_NULL)
        rt_free(sensor_baro);

    return -RT_ERROR;
}
