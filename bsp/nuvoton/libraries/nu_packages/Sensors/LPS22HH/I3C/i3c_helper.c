
/**************************************************************************//**
 * @file     i3c_ControllerRole.c
 * @version  V3.00
 * @brief    Functions for I3C Controller Role.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "i3c_DeviceFunc.h"

static int i3c_ccc(void)
{
    /* Initialize the device as I3C Controller Role */
    I3C_ControllerRole(&g_I3CDev, 1);

    while (1)
    {
        /* Device has switched to I3C Target Role */
        I3C_TargetRole(&g_I3CDev, 0);

        /* Device has switched to I3C Controller Role */
        I3C_ControllerRole(&g_I3CDev, 0);
    }

    return 0;
}
MSH_CMD_EXPORT(i3c_ccc, i3c CCC);

static uint8_t s_u8tgt = 255;
static void I3C_ControllerRoleEx(I3C_DEVICE_T *dev, uint32_t u32IsInit)
{
    int stage;

    if (u32IsInit == 1)
    {
        /* Initializes the I3C device */
        I3C_Init(dev);
    }
    /* Device has switched to I3C Controller Role */
    dev->device_role = I3C_CONTROLLER;

    while (s_u8tgt == 255)
    {
        I3C_FuncDAAssign(dev, I3C_CCC_RSTDAA, 0, 0);

        rt_thread_mdelay(10);

        if (dev->intsts != 0)
        {
            if (dev->intsts & I3C_INTSTS_IBI_RECEIVED)
            {
                I3C_FuncIBIReceived(dev);
                dev->intsts &= ~I3C_INTSTS_IBI_RECEIVED;
            }

            dev->intsts = 0;
        }

        if (I3C_FuncDAAssign(dev, I3C_CCC_ENTDAA, 7, 0) > 0)
        {
            s_u8tgt = 0;
        }

        rt_thread_mdelay(10);

        if (dev->intsts != 0)
        {
            if (dev->intsts & I3C_INTSTS_IBI_RECEIVED)
            {
                dev->intsts &= ~I3C_INTSTS_IBI_RECEIVED;
                I3C_FuncIBIReceived(dev);
            }

            dev->intsts = 0;
        }

        rt_thread_mdelay(10);
    }


}

int32_t nu_i3c_init(void)
{
    /* Initialize the device as I3C Controller Role */
    I3C_ControllerRoleEx(&g_I3CDev, 1);

    return RT_EOK;
}


int32_t nu_i3c_write_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t len)
{
    int i = 0;
    int32_t iRet = I3C_STS_NO_ERR;
    uint8_t write_buf[16] = {0};

    write_buf[0] = (uint8_t)reg;
    rt_memcpy(&write_buf[1], data, len);

    if (s_u8tgt > 6)
    {
        rt_kprintf("Target index-%d NOT at I3C mode.\n", s_u8tgt);
        return -RT_ERROR;
    }

    iRet = I3C_FuncSDRWrite(&g_I3CDev, s_u8tgt, write_buf, len + 1);
    if (iRet != I3C_STS_NO_ERR)
    {
        rt_kprintf("I3C_FuncSDRWrite failed: %d\n", iRet);
        return -RT_ERROR;
    }

    return RT_EOK;
}

int32_t nu_i3c_read_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t len)
{
    int i = 0;
    int32_t iRet = I3C_STS_NO_ERR;
    uint16_t rlen = 0;

    if (s_u8tgt > 6)
    {
        rt_kprintf("Target index-%d NOT at I3C mode.\n", s_u8tgt);
        return -RT_ERROR;
    }

    iRet = I3C_FuncSDRWrite(&g_I3CDev, s_u8tgt, (uint8_t *)&reg, 1);
    if (iRet != I3C_STS_NO_ERR)
    {
        rt_kprintf("I3C_FuncSDRWrite failed: %d\n", iRet);
        return -RT_ERROR;
    }

    rlen = len;
    iRet = I3C_FuncSDRRead(&g_I3CDev, s_u8tgt, data, &rlen);
    if (iRet != I3C_STS_NO_ERR)
    {
        rt_kprintf("I3C_FuncSDRRead failed: %d\n", iRet);
        return -RT_ERROR;
    }

    return RT_EOK;
}

