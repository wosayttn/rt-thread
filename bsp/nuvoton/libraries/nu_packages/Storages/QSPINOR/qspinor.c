/*
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
**/

#include "qspinor.h"

#define W25X_REG_READSTATUS    (0x05)
#define W25X_REG_READSTATUS2   (0x35)
#define W25X_REG_WRITEENABLE   (0x06)
#define W25X_REG_WRITEDISABLE  (0x04)
#define W25X_REG_WRITESTATUS   (0x01)
#define W25X_REG_QUADENABLE    (0x02)

#define W25X_SR_WIP            (1U << 0)
#define W25X_SR_WEL            (1U << 1)
#define W25X_SR_BP0            (1U << 2)
#define W25X_SR_BP1            (1U << 3)
#define W25X_SR_BP2            (1U << 4)
#define W25X_SR_SRWD           (1U << 7)

static rt_uint8_t SpiFlash_ReadStatusReg(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t u8Val;
    rt_err_t result = RT_EOK;
    rt_uint8_t w25x_txCMD1 = W25X_REG_READSTATUS;

    result = rt_qspi_send_then_recv(qspi_device, &w25x_txCMD1, 1, &u8Val, 1);
    RT_ASSERT(result > 0);

    return u8Val;
}

static rt_uint8_t SpiFlash_ReadStatusReg2(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t u8Val;
    rt_err_t result = RT_EOK;
    rt_uint8_t w25x_txCMD1 = W25X_REG_READSTATUS2;

    result = rt_qspi_send_then_recv(qspi_device, &w25x_txCMD1, 1, &u8Val, 1);
    RT_ASSERT(result > 0);

    return u8Val;
}

static rt_err_t SpiFlash_WriteEnable(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t cmd = W25X_REG_WRITEENABLE;

    if (rt_qspi_send(qspi_device, &cmd, 1) != 1)
    {
        return -RT_ERROR;
    }

    if (!SpiFlash_IsWriteEnabled(qspi_device))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t SpiFlash_WriteStatusReg(struct rt_qspi_device *qspi_device, uint8_t u8Value1, uint8_t u8Value2)
{
    rt_uint8_t au8Val[2];
    rt_err_t result;
    struct rt_qspi_message qspi_message = {0};

    result = SpiFlash_WriteEnable(qspi_device);
    if (result != RT_EOK)
    {
        goto exit_SpiFlash_WriteStatusReg;
    }

    /* Prepare status-1, 2 data */
    au8Val[0] = u8Value1;
    au8Val[1] = u8Value2;

    /* 1-bit mode: Instruction+payload */
    qspi_message.instruction.content = W25X_REG_WRITESTATUS;
    qspi_message.instruction.qspi_lines = 1;

    qspi_message.qspi_data_lines   = 1;
    qspi_message.parent.cs_take    = 1;
    qspi_message.parent.cs_release = 1;
    qspi_message.parent.send_buf   = &au8Val[0];
    qspi_message.parent.length     = sizeof(au8Val);
    qspi_message.parent.next       = RT_NULL;

    if (rt_qspi_transfer_message(qspi_device, &qspi_message) != sizeof(au8Val))
    {
        result = -RT_ERROR;
        goto exit_SpiFlash_WriteStatusReg;
    }

    result  = RT_EOK;

exit_SpiFlash_WriteStatusReg:

    return result;
}

rt_uint8_t SpiFlash_GetStatus1(struct rt_qspi_device *qspi_device)
{
    return SpiFlash_ReadStatusReg(qspi_device);
}

rt_bool_t SpiFlash_IsWriteEnabled(struct rt_qspi_device *qspi_device)
{
    return (SpiFlash_ReadStatusReg(qspi_device) & W25X_SR_WEL) != 0;
}

rt_err_t SpiFlash_WriteDisable(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t cmd = W25X_REG_WRITEDISABLE;

    if (rt_qspi_send(qspi_device, &cmd, 1) != 1)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void SpiFlash_WaitReady(struct rt_qspi_device *qspi_device)
{
    volatile uint8_t u8ReturnValue;

    do
    {
        u8ReturnValue = SpiFlash_ReadStatusReg(qspi_device);
        u8ReturnValue = u8ReturnValue & 1;
    }
    while (u8ReturnValue != 0);   // check the BUSY bit
}

void SpiFlash_EnterQspiMode(struct rt_qspi_device *qspi_device)
{
    rt_err_t result = RT_EOK;

    uint8_t u8Status1 = SpiFlash_ReadStatusReg(qspi_device);
    uint8_t u8Status2 = SpiFlash_ReadStatusReg2(qspi_device);

    u8Status2 |= W25X_REG_QUADENABLE;

    result = SpiFlash_WriteStatusReg(qspi_device, u8Status1, u8Status2);
    RT_ASSERT(result == RT_EOK);

    SpiFlash_WaitReady(qspi_device);
}

void SpiFlash_ExitQspiMode(struct rt_qspi_device *qspi_device)
{
    rt_err_t result = RT_EOK;
    uint8_t u8Status1 = SpiFlash_ReadStatusReg(qspi_device);
    uint8_t u8Status2 = SpiFlash_ReadStatusReg2(qspi_device);

    u8Status2 &= ~W25X_REG_QUADENABLE;

    result = SpiFlash_WriteStatusReg(qspi_device, u8Status1, u8Status2);
    RT_ASSERT(result == RT_EOK);

    SpiFlash_WaitReady(qspi_device);
}

rt_err_t SpiFlash_Unprotect(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t status1 = SpiFlash_ReadStatusReg(qspi_device);
    rt_uint8_t status2 = SpiFlash_ReadStatusReg2(qspi_device);
    rt_uint8_t new_status1;
    rt_err_t result;
    int retry;

    for (retry = 0; retry < 3; retry++)
    {
        status1 = SpiFlash_ReadStatusReg(qspi_device);

        /* Clear BP0, BP1, BP2, and SRP(bit7) to unlock */
        new_status1 = status1 & ~((1U << 7) | (1U << 4) | (1U << 3) | (1U << 2));

        result = SpiFlash_WriteStatusReg(qspi_device, new_status1, status2);
        if (result != RT_EOK)
        {
            rt_kprintf("  [retry %d] WRSR failed\n", retry);
            rt_thread_mdelay(10);
            continue;
        }

        rt_thread_mdelay(1);
        SpiFlash_WaitReady(qspi_device);

        status1 = SpiFlash_ReadStatusReg(qspi_device);
        rt_kprintf("  [retry %d] after WRSR, status1 = 0x%02X\n", retry, status1);

        if ((status1 & ((1U << 7) | (1U << 4) | (1U << 3) | (1U << 2))) == 0)
        {
            return RT_EOK;
        }
    }

    return -RT_ERROR;
}

rt_err_t SpiFlash_Protect(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t status1 = SpiFlash_ReadStatusReg(qspi_device);
    rt_uint8_t status2 = SpiFlash_ReadStatusReg2(qspi_device);
    rt_uint8_t new_status1;
    rt_err_t result;
    int retry;

    for (retry = 0; retry < 3; retry++)
    {
        status1 = SpiFlash_ReadStatusReg(qspi_device);

        /* Set BP0, BP1, BP2, and SRWD(bit7) to protect all blocks and lock status register */
        new_status1 = status1 | ((1U << 7) | (1U << 4) | (1U << 3) | (1U << 2));

        result = SpiFlash_WriteStatusReg(qspi_device, new_status1, status2);
        if (result != RT_EOK)
        {
            rt_kprintf("  [retry %d] WRSR failed\n", retry);
            rt_thread_mdelay(10);
            continue;
        }

        rt_thread_mdelay(1);
        SpiFlash_WaitReady(qspi_device);

        status1 = SpiFlash_ReadStatusReg(qspi_device);
        rt_kprintf("  [retry %d] after WRSR, status1 = 0x%02X\n", retry, status1);

        if ((status1 & ((1U << 7) | (1U << 4) | (1U << 3) | (1U << 2))) == ((1U << 7) | (1U << 4) | (1U << 3) | (1U << 2)))
        {
            return RT_EOK;
        }
    }

    return -RT_ERROR;
}

int qspi_unprotect(int argc, char *argv[])
{
    const char *qspi_dev_name = "qspi01";
    rt_device_t dev;
    struct rt_qspi_device *qspi_dev;
    rt_err_t result;
    rt_uint8_t status;

    if (argc > 1)
    {
        qspi_dev_name = argv[1];
    }

    dev = rt_device_find(qspi_dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("qspi unprotect: device '%s' not found\n", qspi_dev_name);
        return -RT_ERROR;
    }

    qspi_dev = (struct rt_qspi_device *)dev;
    result = SpiFlash_Unprotect(qspi_dev);
    if (result != RT_EOK)
    {
        status = SpiFlash_GetStatus1(qspi_dev);
        rt_kprintf("qspi unprotect: failed, status1 still = 0x%02X\n", status);
        return result;
    }

    status = SpiFlash_GetStatus1(qspi_dev);
    rt_kprintf("%s unprotected, status1 = 0x%02X\n", qspi_dev_name, status);
    rt_kprintf("  WIP=%u, WEL=%u, BP=%u, SRWD=%u\n",
               !!(status & W25X_SR_WIP),
               !!(status & W25X_SR_WEL),
               (status & (W25X_SR_BP0 | W25X_SR_BP1 | W25X_SR_BP2)) >> 2,
               !!(status & W25X_SR_SRWD));

    return 0;
}
MSH_CMD_EXPORT_ALIAS(qspi_unprotect, qunp, unprotect qspi status register);

int qspi_protect(int argc, char *argv[])
{
    const char *qspi_dev_name = "qspi01";
    rt_device_t dev;
    struct rt_qspi_device *qspi_dev;
    rt_err_t result;
    rt_uint8_t status;

    if (argc > 1)
    {
        qspi_dev_name = argv[1];
    }

    dev = rt_device_find(qspi_dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("qspi protect: device '%s' not found\n", qspi_dev_name);
        return -RT_ERROR;
    }

    qspi_dev = (struct rt_qspi_device *)dev;
    result = SpiFlash_Protect(qspi_dev);
    if (result != RT_EOK)
    {
        status = SpiFlash_GetStatus1(qspi_dev);
        rt_kprintf("qspi protect: failed, status1 = 0x%02X\n", status);
        return result;
    }

    status = SpiFlash_GetStatus1(qspi_dev);
    rt_kprintf("%s protected, status1 = 0x%02X\n", qspi_dev_name, status);
    rt_kprintf("  WIP=%u, WEL=%u, BP=%u, SRWD=%u\n",
               !!(status & W25X_SR_WIP),
               !!(status & W25X_SR_WEL),
               (status & (W25X_SR_BP0 | W25X_SR_BP1 | W25X_SR_BP2)) >> 2,
               !!(status & W25X_SR_SRWD));

    return 0;
}

MSH_CMD_EXPORT_ALIAS(qspi_protect, qpro, protect qspi status register);

int qspi_status(int argc, char *argv[])
{
    const char *qspi_dev_name = "qspi01";
    rt_device_t dev;
    struct rt_qspi_device *qspi_dev;
    rt_uint8_t status;

    if (argc > 1)
    {
        qspi_dev_name = argv[1];
    }

    dev = rt_device_find(qspi_dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("qspi status: device '%s' not found\n", qspi_dev_name);
        return -RT_ERROR;
    }

    qspi_dev = (struct rt_qspi_device *)dev;
    status = SpiFlash_GetStatus1(qspi_dev);

    rt_kprintf("%s status1 = 0x%02X\n", qspi_dev_name, status);
    rt_kprintf("  WIP=%u, WEL=%u, BP=%u, SRWD=%u\n",
               !!(status & W25X_SR_WIP),
               !!(status & W25X_SR_WEL),
               (status & (W25X_SR_BP0 | W25X_SR_BP1 | W25X_SR_BP2)) >> 2,
               !!(status & W25X_SR_SRWD));

    return 0;
}

MSH_CMD_EXPORT_ALIAS(qspi_status, qstat, read qspi status register);