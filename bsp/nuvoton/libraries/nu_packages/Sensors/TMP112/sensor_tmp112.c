/**************************************************************************//**
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2022-9-1        Wayne        First version
*
******************************************************************************/

#include <rtthread.h>

#include <sys/time.h>
#include "sensor_tmp112.h"

#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME  "sensor.tmp112"
#define DBG_COLOR
#include <rtdbg.h>

#define TMP112_I2C_SLAVE_ADDR          0x48

#define TMP112_REG_TEMP 0x00
#define TMP112_REG_CONF 0x01
#define TMP112_REG_HYST 0x02
#define TMP112_REG_MAX  0x03

#define TMP112_RESOLUTION 12
#define TMP112_SHIFT1 (16 - TMP112_RESOLUTION)
#define TMP112_SHIFT2 (TMP112_RESOLUTION - 8)

static rt_err_t tmp112_i2c_read_reg(struct rt_i2c_bus_device *i2c_bus_dev, const uint8_t u8Reg, uint16_t *pu16Data)
{
    struct rt_i2c_msg msgs[2];
    uint8_t buf[2];

    msgs[0].addr  = TMP112_I2C_SLAVE_ADDR;        /* Slave address */
    msgs[0].flags = RT_I2C_WR;                      /* Write flag */
    msgs[0].buf   = (rt_uint8_t *)&u8Reg;              /* Slave register address */
    msgs[0].len   = 1;                     /* Number of bytes sent */

    msgs[1].addr  = TMP112_I2C_SLAVE_ADDR;        /* Slave address */
    msgs[1].flags = RT_I2C_RD;                      /* Read flag without READ_ACK */
    msgs[1].buf   = (rt_uint8_t *)&buf[0];             /* Read data pointer */
    msgs[1].len   = 2;                         /* Number of bytes read */

    if (rt_i2c_transfer(i2c_bus_dev, &msgs[0], 2) != 2)
    {
        return -RT_ERROR;
    }

    *pu16Data = (uint16_t)((buf[0] << 8) | buf[1]);

    return RT_EOK;
}

static rt_err_t tmp112_i2c_write_reg(struct rt_i2c_bus_device *i2c_bus_dev, uint8_t u8Reg, uint16_t u16Data)
{
    struct rt_i2c_msg msg;
    uint8_t reg_buf[3] = {0};

    reg_buf[0] = u8Reg;
    reg_buf[1] = (uint8_t)(u16Data >> 8);
    reg_buf[2] = (uint8_t)(u16Data & 0xFF);

    msg.addr  = TMP112_I2C_SLAVE_ADDR;        /* Slave address */
    msg.flags = RT_I2C_WR;                    /* Write flag */
    msg.buf   = (rt_uint8_t *)reg_buf;        /* Slave register address */
    msg.len   = 1 + 2;                        /* Number of bytes sent */

    if (rt_i2c_transfer(i2c_bus_dev, &msg, 1) != 1)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static inline rt_err_t tmp112_get_temp(struct rt_i2c_bus_device *i2c_bus_dev, int32_t *pi32Temp)
{
    rt_err_t ret;
    uint16_t raw16 = 0;

    ret = tmp112_i2c_read_reg(i2c_bus_dev, TMP112_REG_TEMP, &raw16);
    if (ret != RT_EOK)
        return ret;

    // TMP112 default: 12-bit, left-justified in 16-bit register (bits 15:4 valid)
    int16_t tmp = (int16_t)raw16;
    tmp >>= 4; // keep sign
    // Now tmp is a signed 12-bit value in a 16-bit signed container

    // Convert to Celsius: LSB = 0.0625
    *pi32Temp = (int32_t)((float)tmp * 0.0625f);

    return RT_EOK;
}

static inline int tmp112_reg_to_c(int16_t reg)
{
    return 0;
}

static rt_ssize_t tmp112_fetch_data(rt_sensor_t sensor, rt_sensor_data_t data, rt_size_t len)
{
    RT_ASSERT(data);

    if (sensor->info.type == RT_SENSOR_CLASS_TEMP)
    {
        int32_t i32TempValue;
        struct rt_i2c_bus_device *i2c_bus_dev = sensor->config.intf.arg;

        if (tmp112_get_temp(i2c_bus_dev, &i32TempValue) == RT_EOK)
        {
            data->type = RT_SENSOR_CLASS_TEMP;
            data->data.temp = i32TempValue;
            data->timestamp = rt_sensor_get_ts();

            return 1;
        }
    }
    return 0;
}

static rt_err_t tmp112_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    switch (cmd)
    {
    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

static struct rt_sensor_ops sensor_ops =
{
    tmp112_fetch_data,
    tmp112_control
};

int rt_hw_tmp112_temp_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor = RT_NULL;

    sensor = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor == RT_NULL)
        return -(RT_ENOMEM);

    sensor->info.type       = RT_SENSOR_CLASS_TEMP;
    sensor->info.vendor     = RT_SENSOR_VENDOR_TI;
    sensor->info.model      = "tmp112_temp";
    sensor->info.unit       = RT_SENSOR_UNIT_DCELSIUS;
    sensor->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor->info.range_max  = 127;
    sensor->info.range_min  = -128;
    sensor->info.period_min = 100; //100ms

    rt_memcpy(&sensor->config, cfg, sizeof(struct rt_sensor_config));
    sensor->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("device register: %d", result);
        rt_free(sensor);
        return -RT_ERROR;
    }

    return RT_EOK;
}

int rt_hw_tmp112_init(const char *name, struct rt_sensor_config *cfg)
{
    struct rt_sensor_intf *intf;
    struct rt_i2c_bus_device *i2c_bus_dev;

    RT_ASSERT(name != NULL);
    RT_ASSERT(cfg != NULL);

    intf = &cfg->intf;

    /* Find I2C bus */
    i2c_bus_dev = (struct rt_i2c_bus_device *)rt_device_find(intf->dev_name);
    if (i2c_bus_dev == RT_NULL)
    {
        goto exit_rt_hw_tmp112_init;
    }

    intf->arg = i2c_bus_dev;

    {
        uint16_t tmp = 0;
        rt_err_t ret = 0;

        ret = tmp112_i2c_read_reg(i2c_bus_dev, TMP112_REG_CONF, &tmp);
        rt_kprintf("TMP112 Config Reg: 0x%04X\n", tmp);
        if ((ret != RT_EOK) || (tmp != 0x60A0))
        {
            goto exit_rt_hw_tmp112_init;
        }
    }

    return rt_hw_tmp112_temp_init(name, cfg);

exit_rt_hw_tmp112_init:

    return -(RT_ERROR);
}
