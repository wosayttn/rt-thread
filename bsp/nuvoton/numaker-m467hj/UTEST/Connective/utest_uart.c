/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_UART) && defined(BSP_USING_UART1)

#include "rtdevice.h"
#include "NuMicro.h"

#define NU_UART_DEVNAME "uart1"
#define NU_TEST_LEN 32

static uint32_t g_original_mfp_setting;

static char txbuf[NU_TEST_LEN] = {0};
static char rxbuf[NU_TEST_LEN] = {0};

static struct serial_configure sUartConfig_New = RT_SERIAL_CONFIG_DEFAULT;
static struct serial_configure sUartConfig_Org = RT_SERIAL_CONFIG_DEFAULT;

static struct rt_semaphore txrx_sem;

static void uart_dump_test_setting(void)
{
    rt_kprintf("\n[UART utest] description\n");
    rt_kprintf("  purpose               : Verify UART loopback in interrupt and DMA modes.\n");
    rt_kprintf("  uart device           : %s\n", NU_UART_DEVNAME);
    rt_kprintf("  loopback bytes        : %d\n", NU_TEST_LEN);
    rt_kprintf("  transfer modes        : interrupt + dma + config checks\n\n");
}

static rt_err_t uart_tx_done(rt_device_t dev, void *buffer)
{
    rt_sem_release(&txrx_sem);
    return RT_EOK;
}

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

    ret = rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
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

static rt_err_t uart_rx_done(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&txrx_sem);
    return RT_EOK;
}

static void testcase_dump_buffer(int i32LID, uint8_t *pu8Buf, int i32BufLen)
{
    int i;
    rt_kprintf("\n==============[%d]===============\n", i32LID);
    for (i = 0; i < i32BufLen; i++)
    {
        rt_kprintf("%02x ", pu8Buf[i]);
    }
    rt_kprintf("\n===============================\n");
}

static void testcase_loop_txrx_dma(void)
{
    rt_device_t serial;
    rt_err_t ret;
    int off = 0;
    int retry = 0;

    rt_kprintf("Will open %s!\n", NU_UART_DEVNAME);
    serial = rt_device_find(NU_UART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx_dma;
    }

    rt_memset(&txbuf[0], 0x33, NU_TEST_LEN);
    rt_memset(&rxbuf[0], 0x66, NU_TEST_LEN);

    testcase_dump_buffer(__LINE__, (uint8_t *)&txbuf[0], NU_TEST_LEN);
    testcase_dump_buffer(__LINE__, (uint8_t *)&rxbuf[0], NU_TEST_LEN);

    ret = rt_device_set_tx_complete(serial, uart_tx_done);
    uassert_int_equal(ret, RT_EOK);

    sUartConfig_New.bufsz = NU_TEST_LEN;

    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_Org);
    uassert_int_equal(ret, RT_EOK);

    ret = rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
    uassert_int_equal(ret, RT_EOK);

    ret = rt_device_write(serial, 0, &txbuf[0], NU_TEST_LEN);
    uassert_int_equal(ret, NU_TEST_LEN);

    // Wait TX Done
    rt_sem_take(&txrx_sem, RT_WAITING_FOREVER);

    // Non-blocking read
    do
    {
        ret = rt_device_read(serial, 0, &rxbuf[off], NU_TEST_LEN - off);
        off += ret;
        retry++;
    }
    while (((NU_TEST_LEN - off) > 0) && (retry < 300));

    testcase_dump_buffer(__LINE__, (uint8_t *)&txbuf[0], NU_TEST_LEN);
    testcase_dump_buffer(__LINE__, (uint8_t *)&rxbuf[0], NU_TEST_LEN);

    uassert_int_equal(off, NU_TEST_LEN);

    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);

    ret = rt_device_set_tx_complete(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    uassert_buf_equal(txbuf, rxbuf, NU_TEST_LEN);

exit_testcase_loop_txrx_dma:

    return;
}

static void testcase_loop_txrx_dma_zero_rxbuf(void)
{
    rt_device_t serial;
    rt_err_t ret;
    int i;

    serial = rt_device_find(NU_UART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx_dma_zero_rxbuf;
    }

    for (i = 0; i < NU_TEST_LEN; i++)
    {
        txbuf[i] = i;
        rxbuf[i] = 0;
    }

    testcase_dump_buffer(__LINE__, (uint8_t *)&txbuf[0], NU_TEST_LEN);
    testcase_dump_buffer(__LINE__, (uint8_t *)&rxbuf[0], NU_TEST_LEN);


    /* Set config.sz to zero before open */
    sUartConfig_New.bufsz = 0;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Set tx complete function */
    ret = rt_device_set_tx_complete(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    /* Set rx indicate function */
    ret = rt_device_set_rx_indicate(serial, uart_rx_done);
    uassert_int_equal(ret, RT_EOK);

    /* Open */
    ret = rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
    uassert_int_equal(ret, RT_EOK);

    /* Wait message to read */
    ret = rt_device_read(serial, 0, &rxbuf[0], NU_TEST_LEN);
    uassert_int_equal(ret, NU_TEST_LEN);

    /* Write content in TX buffer */
    ret = rt_device_write(serial, 0, &txbuf[0], NU_TEST_LEN);
    uassert_int_equal(ret, NU_TEST_LEN);

    rt_sem_take(&txrx_sem, RT_WAITING_FOREVER);

    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);
    testcase_dump_buffer(__LINE__, (uint8_t *)&txbuf[0], NU_TEST_LEN);
    testcase_dump_buffer(__LINE__, (uint8_t *)&rxbuf[0], NU_TEST_LEN);

    /* Compare TX/RX buffer */
    uassert_buf_equal(txbuf, rxbuf, NU_TEST_LEN);

    /* Restore original UART setting. */

    ret = rt_device_set_rx_indicate(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUartConfig_Org);
    uassert_int_equal(ret, RT_EOK);

exit_testcase_loop_txrx_dma_zero_rxbuf:

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

    ret = rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
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
    uart_dump_test_setting();
    // Backup original multiple function pin setting.
    g_original_mfp_setting = SYS->GPA_MFP2;

    // Set multiple function pin setting.
    SYS->GPA_MFP2 &= ~(SYS_GPA_MFP2_PA8MFP_Msk | SYS_GPA_MFP2_PA9MFP_Msk);
    SYS->GPA_MFP2 |= (SYS_GPA_MFP2_PA8MFP_UART1_RXD | SYS_GPA_MFP2_PA9MFP_UART1_TXD);

    rt_sem_init(&txrx_sem, "txrx_sem", 0, RT_IPC_FLAG_FIFO);
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_sem_detach(&txrx_sem);

    // Restore multiple function pin setting
    SYS->GPA_MFP2 = g_original_mfp_setting;

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(testcase_uart_config);
    UTEST_UNIT_RUN(testcase_loop_txrx_dma);
    UTEST_UNIT_RUN(testcase_loop_txrx_dma_zero_rxbuf);
    UTEST_UNIT_RUN(testcase_loop_txrx_int);
    UTEST_UNIT_RUN(testcase_loop_txrx_int_zero_rxbuf);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"uart", utest_tc_init, utest_tc_cleanup, 10);

#endif //#if defined(BSP_USING_UART) && defined(BSP_USING_UART1)
