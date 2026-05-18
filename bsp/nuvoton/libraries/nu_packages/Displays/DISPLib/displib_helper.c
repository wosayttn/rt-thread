/**************************************************************************//**
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2024-7-8        Wayne        First version
*
******************************************************************************/

#include "displib_helper.h"

#define DBG_TAG "displib"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client displib_client = {0};

rt_err_t displib_i2c_write(rt_uint8_t reg, rt_uint8_t value)
{
    struct rt_i2c_client *dev = &displib_client;
    struct rt_i2c_msg msgs;
    rt_uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    msgs.addr  = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = buf;
    msgs.len   = sizeof(buf);

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }

    return -RT_ERROR;
}

rt_uint8_t displib_i2c_read(rt_uint8_t reg)
{
    struct rt_i2c_client *dev = &displib_client;
    struct rt_i2c_msg msgs[2];
    rt_uint8_t data;

    msgs[0].addr  = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = &data;
    msgs[1].len   = 1;

    if ((rt_i2c_transfer(dev->bus, &msgs[0], 1) == 1) &&
            (rt_i2c_transfer(dev->bus, &msgs[1], 1) == 1))
    {
        return data;
    }

    return 0xff;
}

rt_err_t displib_set_dev_addr(rt_uint16_t u16Addr)
{
    displib_client.client_addr = u16Addr;

    return RT_EOK;
}

static void displib_rst(rt_base_t rst_pin, rt_bool_t bActLow)
{
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);

    rt_pin_write(rst_pin, (bActLow ? PIN_LOW : PIN_HIGH));
    rt_thread_mdelay(100);

    rt_pin_write(rst_pin, (bActLow ? PIN_HIGH : PIN_LOW));
    rt_thread_mdelay(100);
}

rt_err_t displib_fini(const S_DISPLIB_CTX *psDispCtx)
{
    RT_ASSERT(psDispCtx != RT_NULL);

    /* Reset RGB Converter */
    displib_rst(psDispCtx->RstPinIdx, psDispCtx->bRstActLow);

    if (displib_client.bus != RT_NULL)
    {
        /* Close I2C bus device */
        rt_device_close((rt_device_t)displib_client.bus);

        displib_client.bus = RT_NULL;
    }

    return RT_EOK;
}

rt_err_t displib_init(const S_DISPLIB_CTX *psDispCtx)
{
    RT_ASSERT(psDispCtx != RT_NULL);

    /* Reset RGB Converter */
    displib_rst(psDispCtx->RstPinIdx, psDispCtx->bRstActLow);

    /* I2C client device binding */
    displib_client.bus = (struct rt_i2c_bus_device *)rt_device_find(psDispCtx->szI2CBusName);
    if (displib_client.bus == RT_NULL)
    {
        LOG_E("Can't find %s device", psDispCtx->szI2CBusName);
        return -RT_ERROR;
    }

    /* Open I2C bus device */
    if (rt_device_open((rt_device_t)displib_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        LOG_E("open %s device failed", psDispCtx->szI2CBusName);
        return -RT_ERROR;
    }

    return RT_EOK;
}
