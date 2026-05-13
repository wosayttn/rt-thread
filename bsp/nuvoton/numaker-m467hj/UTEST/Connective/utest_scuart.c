/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_SCUART0)

#include "rtdevice.h"

#include "NuMicro.h"

#define NU_UART_DEVNAME "scuart0"
#define NU_TEST_LEN 32

static void scuart_dump_test_setting(void)
{
    rt_kprintf("\n[SCUART utest] description\n");
    rt_kprintf("  purpose               : Verify SCUART loopback and basic serial configuration.\n");
    rt_kprintf("  uart device           : %s\n", NU_UART_DEVNAME);
    rt_kprintf("  loopback bytes        : %d\n", NU_TEST_LEN);
    rt_kprintf("  transfer modes        : interrupt + config checks\n\n");
}

static uint32_t g_original_mfp_setting;

static char txbuf[NU_TEST_LEN] = {0};
static char rxbuf[NU_TEST_LEN] = {0};

static struct serial_configure sUartConfig_New = RT_SERIAL_CONFIG_DEFAULT;
static struct serial_configure sUartConfig_Org = RT_SERIAL_CONFIG_DEFAULT;

static struct rt_semaphore txrx_sem;

static void testcase_loop_txrx_int(void)
{
    rt_device_t serial;
    rt_err_t ret;

    serial = rt_device_find(NU_UART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx;
    }

    rt_memset(&txbuf[0], 0x12, NU_TEST_LEN);
    rt_memset(&rxbuf[0], 0x34, NU_TEST_LEN);

    ret = rt_device_open(serial,  RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    ret = rt_device_write(serial, 0, &txbuf[0], NU_TEST_LEN);
    uassert_int_equal(ret, NU_TEST_LEN);

    ret = rt_device_read(serial, 0, &rxbuf[0], NU_TEST_LEN);
    uassert_int_equal(ret, NU_TEST_LEN);

    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);

    uassert_buf_equal(txbuf, rxbuf, NU_TEST_LEN);

exit_testcase_loop_txrx:

    return;
}


static void testcase_loop_txrx_int_zero_rxbuf(void)
{
    rt_device_t serial;
    rt_err_t ret;
    int count = 0;

    serial = rt_device_find(NU_UART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx_int_zero_rxbuf;
    }

    rt_memset(&txbuf[0], 0x33, NU_TEST_LEN);
    rt_memset(&rxbuf[0], 0x66, NU_TEST_LEN);

    /* Set config.sz to zero before open, in this RT_DEVICE_FLAG_INT_RX mode, bufsz must be >0. */
    sUartConfig_New.bufsz = 1;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Set tx complete function */
    ret = rt_device_set_tx_complete(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    /* Set rx indicate function */
    ret = rt_device_set_rx_indicate(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    /* Open */
    ret = rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    /* Write content in TX buffer */
    count = 0;
    do
    {
        ret = rt_device_write(serial, 0, &txbuf[count], 1);
        uassert_int_equal(ret, 1);
        ret = rt_device_read(serial, 0, &rxbuf[count], 1);
        uassert_int_equal(ret, 1);
        count++;
    }
    while (count < NU_TEST_LEN);

    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);

    /* Compare TX/RX buffer */
    uassert_buf_equal(txbuf, rxbuf, NU_TEST_LEN);

    /* Restore original UART setting. */
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_Org);
    uassert_int_equal(ret, RT_EOK);

exit_testcase_loop_txrx_int_zero_rxbuf:

    return;
}

static void testcase_uart_config(void)
{
    rt_device_t serial;
    rt_err_t ret;

    serial = rt_device_find(NU_UART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_uart_config;
    }

    ret = rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    /* Here can't get real return code of driver, but you can see message on console. */
    rt_memcpy((void *)&sUartConfig_New, (void *)&sUartConfig_Org, sizeof(struct serial_configure));
    sUartConfig_New.data_bits = DATA_BITS_9;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Here, Configure right stop_bits_2 */
    rt_memcpy((void *)&sUartConfig_New, (void *)&sUartConfig_Org, sizeof(struct serial_configure));
    sUartConfig_New.stop_bits = STOP_BITS_2;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Test wrong stop_bit_4 */
    /* Here can't get real return code of driver, but you can see message on console. */
    rt_memcpy((void *)&sUartConfig_New, (void *)&sUartConfig_Org, sizeof(struct serial_configure));
    sUartConfig_New.stop_bits = STOP_BITS_4;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Finally, Restore original setting. */
    rt_memcpy((void *)&sUartConfig_New, (void *)&sUartConfig_Org, sizeof(struct serial_configure));
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Close uart port */
    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);

exit_testcase_uart_config:

    return;
}

static rt_err_t utest_tc_init(void)
{
    scuart_dump_test_setting();
    // Backup original multiple function pin setting.
    g_original_mfp_setting = SYS->GPA_MFP0;

    // Set multiple function pin setting.
    SET_SC0_CLK_PA0();
    SET_SC0_DAT_PA1();

    rt_sem_init(&txrx_sem, "txrx_sem", 0, RT_IPC_FLAG_FIFO);
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_sem_detach(&txrx_sem);

    // Restore multiple function pin setting
    SYS->GPA_MFP0 = g_original_mfp_setting;

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(testcase_uart_config);
    UTEST_UNIT_RUN(testcase_loop_txrx_int);
    UTEST_UNIT_RUN(testcase_loop_txrx_int_zero_rxbuf);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"scuart", utest_tc_init, utest_tc_cleanup, 10);
#endif
