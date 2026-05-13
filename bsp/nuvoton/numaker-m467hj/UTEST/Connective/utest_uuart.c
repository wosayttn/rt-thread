/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_USCI0) && defined(BSP_USING_UUART0)

#include "rtdevice.h"
#include "NuMicro.h"

#define NU_UUART_DEVNAME "uuart0"
#define NU_TEST_LEN 32

static uint32_t g_original_mfp0_setting;
static uint32_t g_original_mfp1_setting;

char txbuf[NU_TEST_LEN] = {0};
char rxbuf[NU_TEST_LEN] = {0};

struct serial_configure sUUartConfig_New = RT_SERIAL_CONFIG_DEFAULT;
struct serial_configure sUUartConfig_Org = RT_SERIAL_CONFIG_DEFAULT;

static struct rt_semaphore txrx_sem;

static void uuart_dump_test_setting(void)
{
    rt_kprintf("\n[UUART utest] description\n");
    rt_kprintf("  purpose               : Verify UUART loopback in interrupt and DMA modes.\n");
    rt_kprintf("  uuart device          : %s\n", NU_UUART_DEVNAME);
    rt_kprintf("  loopback bytes        : %d\n", NU_TEST_LEN);
    rt_kprintf("  transfer modes        : interrupt + dma + config checks\n\n");
}

static rt_err_t uuart_tx_done(rt_device_t dev, void *buffer)
{
    rt_sem_release(&txrx_sem);
    return RT_EOK;
}

static void testcase_loop_txrx_int(void)
{
    rt_device_t serial;
    rt_err_t ret;

    serial = rt_device_find(NU_UUART_DEVNAME);
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

static rt_err_t uuart_rx_done(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&txrx_sem);
    return RT_EOK;
}

static void testcase_loop_txrx_dma(void)
{
    rt_device_t serial;
    rt_err_t ret;
    rt_size_t len = 0;
    int off = 0;

    rt_kprintf("Will open %s!\n", NU_UUART_DEVNAME);
    serial = rt_device_find(NU_UUART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx_dma;
    }

    rt_memset(&txbuf[0], 0x12, NU_TEST_LEN);
    rt_memset(&rxbuf[0], 0x34, NU_TEST_LEN);

    ret = rt_device_set_tx_complete(serial, uuart_tx_done);
    uassert_int_equal(ret, RT_EOK);

    sUUartConfig_New.bufsz = NU_TEST_LEN;

    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
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
        len = rt_device_read(serial, 0, &rxbuf[off], NU_TEST_LEN - off);
        rt_kprintf("got %d bytes, put it at %d\n", len, off);
        off += len;
    }
    while ((NU_TEST_LEN - off) > 0);

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

    serial = rt_device_find(NU_UUART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx_dma_zero_rxbuf;
    }

    rt_memset(&txbuf[0], 0x12, NU_TEST_LEN);
    rt_memset(&rxbuf[0], 0x34, NU_TEST_LEN);

    /* Set config.sz to zero before open */
    sUUartConfig_New.bufsz = 0;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Set tx complete function */
    ret = rt_device_set_tx_complete(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    /* Set rx indicate function */
    ret = rt_device_set_rx_indicate(serial, uuart_rx_done);
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

    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);


    /* Compare TX/RX buffer */
    uassert_buf_equal(txbuf, rxbuf, NU_TEST_LEN);

    /* Restore original UUART setting. */

    ret = rt_device_set_rx_indicate(serial, NULL);
    uassert_int_equal(ret, RT_EOK);

    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_Org);
    uassert_int_equal(ret, RT_EOK);

exit_testcase_loop_txrx_dma_zero_rxbuf:

    return;
}
static void testcase_loop_txrx_int_zero_rxbuf(void)
{
    rt_device_t serial;
    rt_err_t ret;
    int count = 0;

    serial = rt_device_find(NU_UUART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_loop_txrx_int_zero_rxbuf;
    }

    rt_memset(&txbuf[0], 0x12, NU_TEST_LEN);
    rt_memset(&rxbuf[0], 0x34, NU_TEST_LEN);

    /* Set config.sz to zero before open, in this RT_DEVICE_FLAG_INT_RX mode, bufsz must be >0. */
    sUUartConfig_New.bufsz = 1;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
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

    /* Restore original UUART setting. */
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_Org);
    uassert_int_equal(ret, RT_EOK);

exit_testcase_loop_txrx_int_zero_rxbuf:

    return;
}

static void testcase_uuart_config(void)
{
    rt_device_t serial;
    rt_err_t ret;

    serial = rt_device_find(NU_UUART_DEVNAME);
    if (!serial)
    {
        uassert_not_null(serial);
        goto exit_testcase_uuart_config;
    }


    ret = rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
    uassert_int_equal(ret, RT_EOK);
    /* Here can't get real return code of driver, but you can see message on console. */
    rt_memcpy((void *)&sUUartConfig_New, (void *)&sUUartConfig_Org, sizeof(struct serial_configure));
    sUUartConfig_New.data_bits = DATA_BITS_9;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
    uassert_int_equal(ret, RT_EOK);


    /* Here, Configure right stop_bits_2 */
    rt_memcpy((void *)&sUUartConfig_New, (void *)&sUUartConfig_Org, sizeof(struct serial_configure));
    sUUartConfig_New.stop_bits = STOP_BITS_2;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Test wrong stop_bit_4 */
    /* Here can't get real return code of driver, but you can see message on console. */
    rt_memcpy((void *)&sUUartConfig_New, (void *)&sUUartConfig_Org, sizeof(struct serial_configure));
    sUUartConfig_New.stop_bits = STOP_BITS_4;
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Finally, Restore original setting. */
    rt_memcpy((void *)&sUUartConfig_New, (void *)&sUUartConfig_Org, sizeof(struct serial_configure));
    ret = rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &sUUartConfig_New);
    uassert_int_equal(ret, RT_EOK);

    /* Close uart port */
    ret = rt_device_close(serial);
    uassert_int_equal(ret, RT_EOK);

exit_testcase_uuart_config:

    return;
}

static rt_err_t utest_tc_init(void)
{
    uuart_dump_test_setting();

    /* Backup original multiple function pin setting */
    g_original_mfp0_setting = SYS->GPE_MFP0;
    g_original_mfp1_setting = SYS->GPE_MFP1;

    /* Set PE multi-function pins for USCI0_DAT0(PE1) and USCI0_DAT1(PE2) */
    SYS->GPE_MFP0 &= ~(SYS_GPE_MFP0_PE2MFP_Msk | SYS_GPE_MFP0_PE3MFP_Msk);
    SYS->GPE_MFP1 &= ~(SYS_GPE_MFP1_PE4MFP_Msk | SYS_GPE_MFP1_PE5MFP_Msk);

    SYS->GPE_MFP0 |= (SYS_GPE_MFP0_PE2MFP_USCI0_CLK | SYS_GPE_MFP0_PE3MFP_USCI0_DAT0);
    SYS->GPE_MFP1 |= (SYS_GPE_MFP1_PE4MFP_USCI0_DAT1 | SYS_GPE_MFP1_PE5MFP_USCI0_CTL1);

    rt_sem_init(&txrx_sem, "txrx_sem", 0, RT_IPC_FLAG_FIFO);
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_sem_detach(&txrx_sem);

    // Restore multiple function pin setting
    SYS->GPE_MFP0 = g_original_mfp0_setting;
    SYS->GPE_MFP1 = g_original_mfp1_setting;

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(testcase_uuart_config);
    UTEST_UNIT_RUN(testcase_loop_txrx_dma);
    UTEST_UNIT_RUN(testcase_loop_txrx_dma_zero_rxbuf);
    UTEST_UNIT_RUN(testcase_loop_txrx_int);
    UTEST_UNIT_RUN(testcase_loop_txrx_int_zero_rxbuf);
}
UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"uuart", utest_tc_init, utest_tc_cleanup, 5);

#endif //#if defined(BSP_USING_USCI0) && defined(BSP_USING_UUART0)
