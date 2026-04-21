/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-19     Wayne        first version
 */

#include "st_lis3dh_sensor_v1.h"

#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME  "sensor.st.lis3dh"
#define DBG_COLOR
#include <rtdbg.h>

#define SENSOR_ACC_RANGE_2G   2000
#define SENSOR_ACC_RANGE_4G   4000
#define SENSOR_ACC_RANGE_8G   8000
#define SENSOR_ACC_RANGE_16G  16000

#include "lis3dh_reg.h"
#include "board.h"

static uint8_t whoamI = 0x00;
static struct rt_i2c_bus_device *i2c_dev_acce;
static LIS3DH_Object_t lis3dh_obj;

static int32_t rt_func_ok(void)
{
    return 0;
}

static int32_t get_tick(void)
{
    return rt_tick_get();
}

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

static rt_err_t _lis3dh_init(struct rt_sensor_intf *intf)
{
    rt_uint8_t  id;
    LIS3DH_IO_t io_ctx;

    i2c_dev_acce = (struct rt_i2c_bus_device *)rt_device_find(intf->dev_name);
    if (i2c_dev_acce == RT_NULL)
    {
        LOG_E("Can't find acce device");
        return -RT_ERROR;
    }

    /* Configure the accelero driver */
    io_ctx.BusType     = LIS3DH_I2C_BUS; /* I2C */
    io_ctx.Address     = 0x18;
    io_ctx.Init        = rt_func_ok;
    io_ctx.DeInit      = rt_func_ok;
    io_ctx.ReadReg     = rt_i2c_read_reg;
    io_ctx.WriteReg    = rt_i2c_write_reg;
    io_ctx.GetTick     = get_tick;

    if (LIS3DH_RegisterBusIO(&lis3dh_obj, &io_ctx) != LIS3DH_OK)
    {
        return -RT_ERROR;
    }
    else if (LIS3DH_ReadID(&lis3dh_obj, &id) != LIS3DH_OK)
    {
        LOG_E("read id failed");
        return -RT_ERROR;
    }
    LOG_D("acce id:%d", id);
    if (LIS3DH_Init(&lis3dh_obj) != LIS3DH_OK)
    {
        LOG_E("acce init failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t _lis3dh_set_range(rt_sensor_t sensor, rt_int32_t range)
{
    if (sensor->info.type == RT_SENSOR_CLASS_ACCE)
    {
        LIS3DH_SetFullScale(&lis3dh_obj, range / 1000);
        LOG_D("acce set range %d", range);
    }

    return RT_EOK;
}

static rt_err_t _lis3dh_set_odr(rt_sensor_t sensor, rt_uint16_t odr)
{
    if (sensor->info.type == RT_SENSOR_CLASS_ACCE)
    {
        LIS3DH_SetOutputDataRate(&lis3dh_obj, odr);
        LOG_D("acce set odr %d", odr);
    }

    return RT_EOK;
}

static rt_err_t _lis3dh_acc_set_mode(rt_sensor_t sensor, rt_uint8_t mode)
{
    if (mode == RT_SENSOR_MODE_POLLING)
    {
        lis3dh_fifo_mode_set(&lis3dh_obj.Ctx, LIS3DH_BYPASS_MODE);
        LOG_D("set mode to POLLING");
    }
    else if (mode == RT_SENSOR_MODE_INT)
    {
        LOG_D("set mode to RT_SENSOR_MODE_INT");
    }
    else if (mode == RT_SENSOR_MODE_FIFO)
    {
        lis3dh_fifo_mode_set(&lis3dh_obj.Ctx, LIS3DH_FIFO_MODE);
        lis3dh_fifo_trigger_event_set(&lis3dh_obj.Ctx, LIS3DH_INT1_GEN);
        LOG_D("set mode to RT_SENSOR_MODE_FIFO");
    }
    else
    {
        LOG_D("Unsupported mode, code is %d", mode);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t _lis3dh_set_power(rt_sensor_t sensor, rt_uint8_t power)
{
    if (power == RT_SENSOR_POWER_DOWN)
    {
        if (sensor->info.type == RT_SENSOR_CLASS_ACCE)
        {
            LIS3DH_Disable(&lis3dh_obj);
        }

        LOG_D("set power down");
    }
    else if (power == RT_SENSOR_POWER_NORMAL)
    {
        if (sensor->info.type == RT_SENSOR_CLASS_ACCE)
        {
            LIS3DH_Enable(&lis3dh_obj);
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

static RT_SIZE_TYPE _lis3dh_polling_get_data(rt_sensor_t sensor, rt_sensor_data_t data)
{
    if (sensor->info.type == RT_SENSOR_CLASS_ACCE)
    {
        LIS3DH_Axes_t acce;

        if (LIS3DH_OK == LIS3DH_GetAxes(&lis3dh_obj, &acce))
        {
            data->type = RT_SENSOR_CLASS_ACCE;
            data->data.acce.x = acce.x;
            data->data.acce.y = acce.y;
            data->data.acce.z = acce.z;
            data->timestamp = rt_sensor_get_ts();

            return 1;
        }
    }
    return 0;
}

static RT_SIZE_TYPE _lis3dh_fifo_get_data(rt_sensor_t sensor, struct rt_sensor_data *data, rt_size_t len)
{
    LIS3DH_Axes_t acce;
    rt_uint8_t i;

    for (i = 0; i < len; i++)
    {
        if (LIS3DH_GetAxes(&lis3dh_obj, &acce) == 0)
        {
            data[i].type = RT_SENSOR_CLASS_ACCE;
            data[i].data.acce.x = acce.x;
            data[i].data.acce.y = acce.y;
            data[i].data.acce.z = acce.z;
            data[i].timestamp = rt_sensor_get_ts();
        }
        else
            break;
    }
    return i;
}

static RT_SIZE_TYPE _lis3dh_interrupt_get_data(rt_sensor_t sensor, struct rt_sensor_data *data)
{
    /**
     * @TODO add interrupt method
     *
     */
    return RT_EOK;
}

static RT_SIZE_TYPE _lis3dh_get_id(void *args)
{
    LIS3DH_ReadID(&lis3dh_obj, args);
    if ((*(uint8_t *)args) != LIS3DH_ID)
    {
        LOG_D("Err: no sensor! whoamI = %d!\r\n", args);
        return -1;
    }
    return RT_EOK;
}

static RT_SIZE_TYPE lis3dh_fetch_data(struct rt_sensor_device *sensor, rt_sensor_data_t buf, rt_size_t len)
{
    RT_ASSERT(buf);

    if (sensor->config.mode == RT_SENSOR_MODE_POLLING)
    {
        return _lis3dh_polling_get_data(sensor, buf);
    }
    else if (sensor->config.mode == RT_SENSOR_MODE_INT)
    {
        return _lis3dh_interrupt_get_data(sensor, buf);
    }
    else if (sensor->config.mode == RT_SENSOR_MODE_FIFO)
    {
        return _lis3dh_fifo_get_data(sensor, buf, len);
    }
    else
        return 0;
}

static rt_err_t lis3dh_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;

    switch (cmd)
    {
    case RT_SENSOR_CTRL_GET_ID:
        result = _lis3dh_get_id(args);
        break;
    case RT_SENSOR_CTRL_SET_RANGE:
        result = _lis3dh_set_range(sensor, (rt_int32_t)args);
        break;
    case RT_SENSOR_CTRL_SET_ODR:
        result = _lis3dh_set_odr(sensor, (rt_uint32_t)args & 0xffff);
        break;
    case RT_SENSOR_CTRL_SET_MODE:
        result = _lis3dh_acc_set_mode(sensor, (rt_uint32_t)args & 0xff);
        break;
    case RT_SENSOR_CTRL_SET_POWER:
        result = _lis3dh_set_power(sensor, (rt_uint32_t)args & 0xff);
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
    lis3dh_fetch_data,
    lis3dh_control
};

int rt_hw_lis3dh_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor_acce = RT_NULL;

    /* accelerometer sensor register */
    {
        sensor_acce = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_acce == RT_NULL)
            return -1;

        sensor_acce->info.type       = RT_SENSOR_CLASS_ACCE;
        sensor_acce->info.vendor     = RT_SENSOR_VENDOR_STM;
        sensor_acce->info.model      = "lis3dh_acc";
        sensor_acce->info.unit       = RT_SENSOR_UNIT_MG;
        sensor_acce->info.intf_type  = RT_SENSOR_INTF_I2C;
        sensor_acce->info.range_max  = SENSOR_ACC_RANGE_16G;
        sensor_acce->info.range_min  = SENSOR_ACC_RANGE_2G;
        sensor_acce->info.period_min = 5;

        rt_memcpy(&sensor_acce->config, cfg, sizeof(struct rt_sensor_config));
        sensor_acce->ops = &sensor_ops;

        result = rt_hw_sensor_register(sensor_acce, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (result != RT_EOK)
        {
            LOG_E("device register err code: %d", result);
            goto __exit;
        }
    }

    result = _lis3dh_init(&cfg->intf);
    if (result != RT_EOK)
    {
        LOG_E("_lis3dh init err code: %d", result);
        goto __exit;
    }

    LOG_I("lis3dh init success");
    return RT_EOK;

__exit:
    if (sensor_acce != RT_NULL)
        rt_free(sensor_acce);

    return -RT_ERROR;
}
