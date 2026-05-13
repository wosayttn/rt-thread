/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/
#include "utest.h"

#if defined(BSP_USING_I2C) && defined(BSP_USING_I2C0) && !defined(BSP_USING_I2C1)

#include "rtdevice.h"
#include "NuMicro.h"
#include "nu_i2c_slave_echo.h"

#define DBG_TAG "utest_i2c"
#define DBG_LVL DBG_INFO
#include "rtdbg.h"

static uint32_t g_original_mfp_setting;

static void i2c_dump_test_setting(void)
{
    rt_kprintf("\n[I2C utest] description\n");
    rt_kprintf("  purpose               : Verify I2C master/slave echo in 7-bit and 10-bit modes.\n");
    rt_kprintf("  master bus            : i2c0\n");
    rt_kprintf("  slave bus             : i2c1\n");
    rt_kprintf("  7-bit slave address   : 0x%02x\n", SLAVE_ADDR);
    rt_kprintf("  10-bit slave address  : 0x%03x\n\n", SLAVE_ADDR_10BIT);
}

static rt_err_t utest_tc_init(void)
{
    i2c_dump_test_setting();
    // Backup original multiple function pin setting.
    g_original_mfp_setting = SYS->GPA_MFP1;

    // I2C0 master role
    SYS->GPA_MFP1 &= ~(SYS_GPA_MFP1_PA4MFP_Msk | SYS_GPA_MFP1_PA5MFP_Msk);
    SYS->GPA_MFP1 |= (SYS_GPA_MFP1_PA4MFP_I2C0_SDA | SYS_GPA_MFP1_PA5MFP_I2C0_SCL);

    // Pull up
    GPIO_SetPullCtl(PA, (BIT4 | BIT5), GPIO_PUSEL_PULL_UP);

    // I2C1 slave role
    SYS->GPA_MFP1 &= ~(SYS_GPA_MFP1_PA6MFP_Msk | SYS_GPA_MFP1_PA7MFP_Msk);
    SYS->GPA_MFP1 |= (SYS_GPA_MFP1_PA6MFP_I2C1_SDA | SYS_GPA_MFP1_PA7MFP_I2C1_SCL);

    nu_i2c_slave_echo_init();

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    nu_i2c_slave_echo_cleanup();

    SYS->GPA_MFP1 = g_original_mfp_setting;

    return RT_EOK;
}

static void thread_entry_i2c_master_rw(uint32_t u32AddressMode)
{
    // Initial slave.
    nu_i2c_slave_echo_run(u32AddressMode);
    rt_uint16_t slave_addr = (u32AddressMode == 0) ? SLAVE_ADDR : SLAVE_ADDR_10BIT;

    int i;

    rt_uint8_t buf_tx[] = {0x55, 0xaa, 0x40, 0xEF, 0x66};
    rt_uint8_t buf_rx[sizeof(buf_tx)] = {0x00, 0x00, 0x00, 0x00, 0x00};
    struct rt_i2c_bus_device *i2c_bus;
    struct rt_i2c_msg msg[2];
    rt_uint32_t ret;
    rt_err_t err = RT_EOK;

    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find("i2c0");

    /*WR data*/
    msg[0].addr = slave_addr;
    msg[0].flags = RT_I2C_WR | ((u32AddressMode == 0) ? 0 : RT_I2C_ADDR_10BIT) ;
    msg[0].len = sizeof(buf_tx);
    msg[0].buf = buf_tx;

    /*RD data*/
    msg[1].addr = slave_addr;
    msg[1].flags = RT_I2C_RD | ((u32AddressMode == 0) ? 0 : RT_I2C_ADDR_10BIT) ;
    msg[1].len = sizeof(buf_rx);
    msg[1].buf = buf_rx;

    ret = rt_i2c_transfer(i2c_bus, &msg[0], 2);
    uassert_true(ret == 2);

    for (i = 0; i < sizeof(buf_tx); i++)
    {
        if (buf_tx[i] != buf_rx[i])
        {
            LOG_E("FAIL: tx[%d]= %02x, rx[%d]= %02x", i, buf_tx[i], i, buf_rx[i]);
            err = RT_ERROR;
        }
    }
    uassert_true(err == RT_EOK);
}


static void test_i2c_read_write()
{
    thread_entry_i2c_master_rw(0);
}

static void test_i2c_read_write_10bit(void)
{
    thread_entry_i2c_master_rw(1);
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_i2c_read_write);
    UTEST_UNIT_RUN(test_i2c_read_write_10bit);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX "i2c",
                utest_tc_init, utest_tc_cleanup, 5);

#endif  // #if defined(BSP_USING_I2C) && defined(BSP_USING_I2C0) && defined(BSP_USING_I2C1)
