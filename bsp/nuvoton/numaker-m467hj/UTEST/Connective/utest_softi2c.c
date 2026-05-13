/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_SOFT_I2C) && defined(BSP_USING_GPIO) && defined(RT_USING_I2C_BITOPS) && defined(RT_USING_I2C) && defined(RT_USING_PIN) && !defined(BSP_USING_I2C1)

#include "rtdevice.h"
#include "NuMicro.h"
#include "nu_i2c_slave_echo.h"

#define DBG_TAG "utest_softi2c"
#define DBG_LVL DBG_INFO
#include "rtdbg.h"
static rt_uint32_t s_u32GPB_MFP0_origin;
static rt_sem_t sem_test_done      = RT_NULL;

static void softi2c_dump_test_setting(void)
{
    rt_kprintf("\n[SOFTI2C utest] description\n");
    rt_kprintf("  purpose               : Verify software I2C master/slave echo in 7-bit and 10-bit modes.\n");
    rt_kprintf("  master bus            : softi2c0\n");
    rt_kprintf("  slave bus             : i2c1\n");
    rt_kprintf("  7-bit slave address   : 0x%02x\n", SLAVE_ADDR);
    rt_kprintf("  10-bit slave address  : 0x%03x\n\n", SLAVE_ADDR_10BIT);
}

static rt_err_t utest_tc_init(void)
{
    softi2c_dump_test_setting();
    s_u32GPB_MFP0_origin = SYS->GPB_MFP0;

    SYS_UnlockReg();
    /* Set UI2C0 multi-function pins */
    SYS->GPB_MFP0 &= ~(SYS_GPB_MFP0_PB3MFP_Msk | SYS_GPB_MFP0_PB2MFP_Msk);

    nu_i2c_slave_echo_init();
    sem_test_done       = rt_sem_create("test is done", 0, RT_IPC_FLAG_FIFO);
    uassert_not_null(sem_test_done);

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    SYS_UnlockReg();

    SYS->GPB_MFP0 = s_u32GPB_MFP0_origin;

    nu_i2c_slave_echo_cleanup();

    rt_sem_delete(sem_test_done);
    sem_test_done = RT_NULL;

    return RT_EOK;
}

void thread_entry_softi2c_master_rw(void *parameter)
{
    // Initial slave.
    uint32_t u32AddressMode = *((uint32_t *)parameter);
    nu_i2c_slave_echo_run(u32AddressMode);
    rt_uint16_t slave_addr = (u32AddressMode == 0) ? SLAVE_ADDR : SLAVE_ADDR_10BIT;

    int i;

    rt_uint8_t buf_tx[] = {0x55, 0xaa, 0x40, 0xEF, 0x66};
    rt_uint8_t buf_rx[sizeof(buf_tx)] = {0x00, 0x00, 0x00, 0x00, 0x00};
    struct rt_i2c_bus_device *i2c_bus;
    struct rt_i2c_msg msg[2];
    rt_uint32_t ret;
    rt_err_t err = RT_EOK;

    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find("softi2c0");

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

    rt_sem_release(sem_test_done);
}

static void test_softi2c_read_write(void)
{
    rt_thread_t thread_master_ptr;
    uint32_t u32AddressMode = 0;

    thread_master_ptr = rt_thread_create("softi2c master thread",
                                         thread_entry_softi2c_master_rw, &u32AddressMode,
                                         2048, 20, 5);

    if (thread_master_ptr != RT_NULL) rt_thread_startup(thread_master_ptr);

    rt_sem_take(sem_test_done, RT_WAITING_FOREVER);
}

static void test_softi2c_read_write_10bit(void)
{
    rt_thread_t thread_master_ptr;
    uint32_t u32AddressMode = 1;

    thread_master_ptr = rt_thread_create("softi2c master thread",
                                         thread_entry_softi2c_master_rw, &u32AddressMode,
                                         2048, 20, 5);

    if (thread_master_ptr != RT_NULL) rt_thread_startup(thread_master_ptr);

    rt_sem_take(sem_test_done, RT_WAITING_FOREVER);
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_softi2c_read_write);
    UTEST_UNIT_RUN(test_softi2c_read_write_10bit);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"softi2c",
                utest_tc_init, utest_tc_cleanup, 1);
#endif //#if (defined(BSP_USING_SOFT_I2C) && defined(BSP_USING_GPIO) && defined(RT_USING_I2C_BITOPS) && defined(RT_USING_I2C) && defined(RT_USING_PIN))
