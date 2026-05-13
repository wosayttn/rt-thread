/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_USCI0) && defined(BSP_USING_USPI0)

#include "rtdevice.h"

#include "NuMicro.h"

#define UTEST_USPI_DEVNAME   "uspi0"
#define UTEST_USPISLAVE_DEVNAME   UTEST_USPI_DEVNAME"0"
#define NU_TEST_LEN (1*2*3*4)

/* Limit max SPI clock to 24MHz */
#define NU_USPI_MAX_MHZ  (24)  //Due engine clock div 2
#define NU_USPI_MAX_HZ (NU_USPI_MAX_MHZ*1000000)
#define NU_TEST_PATTERN 0xa5u

#define NU_UTEST_DEFAULT_USPI_CFG           \
{                                           \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,     \
    .data_width = 8,                        \
    .max_hz = NU_USPI_MAX_HZ,               \
}

static uint32_t g_original_mfp0_setting;
static uint32_t g_original_mfp1_setting;
static struct rt_spi_device uspi_device;
static struct rt_spi_configuration cfg = NU_UTEST_DEFAULT_USPI_CFG;

static void uspi_dump_test_setting(void)
{
    rt_kprintf("\n[USPI utest] description\n");
    rt_kprintf("  purpose               : Verify USPI loopback speed and alignment coverage.\n");
    rt_kprintf("  uspi bus              : %s\n", UTEST_USPI_DEVNAME);
    rt_kprintf("  uspi device           : %s\n", UTEST_USPISLAVE_DEVNAME);
    rt_kprintf("  transfer length       : %d\n", NU_TEST_LEN);
    rt_kprintf("  max clock (Hz)        : %d\n\n", NU_USPI_MAX_HZ);
}

static rt_bool_t find_diff_in_buffer(uint8_t pattern, uint8_t *puBuf, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (puBuf[i] != pattern)
            return RT_TRUE;
    }
    return RT_FALSE;
}

static void test_uspi_loopback(int align_move)
{
    struct rt_spi_message msg;
    uint8_t txbuf[NU_TEST_LEN + 4];
    uint8_t rxbuf[NU_TEST_LEN + 4];
    int i, j;

    struct rt_spi_device *uspi_dev;

    uspi_dev = (struct rt_spi_device *)rt_device_find(UTEST_USPISLAVE_DEVNAME);
    uassert_true(uspi_dev != RT_NULL);

    rt_kprintf("cfg.max_hz: %d\n", cfg.max_hz);

    for (i = 8; i <= 16; i += 8)
    {
        uint8_t bytes_per_word;
        cfg.data_width = i;
        bytes_per_word = cfg.data_width / 8;

        if (rt_spi_configure(uspi_dev, &cfg) == RT_EOK)
        {
            rt_memset(&txbuf[0], ~NU_TEST_PATTERN, NU_TEST_LEN + 4);
            rt_memset(&rxbuf[0], NU_TEST_PATTERN, NU_TEST_LEN + 4);

            for (j = bytes_per_word; j <= NU_TEST_LEN; j += bytes_per_word)
            {
                // Write new pattern in TX.
                rt_memset(&txbuf[align_move + (j - bytes_per_word)], NU_TEST_PATTERN >> 1, bytes_per_word);

                msg.send_buf   = (void *)&txbuf[align_move];
                msg.recv_buf   = (void *)&rxbuf[align_move];
                msg.length     = j;   /* total byte */
                msg.cs_take    = 1;
                msg.cs_release = 1;
                msg.next       = RT_NULL;
                rt_spi_transfer_message(uspi_dev, &msg);

                // Compare all bytes in used TX & RX buffer.
                uassert_buf_equal((const char *)&txbuf[align_move], (const char *)&rxbuf[align_move], j);

                // Check all bytes in unused RX Buffer are NU_TEST_PATTERN .
                uassert_false(find_diff_in_buffer(NU_TEST_PATTERN, (void *)&rxbuf[align_move + j], (NU_TEST_LEN - j)));
            }

        }//if
    }

    return;
}

static void test_uspi_loopback_mode(void)
{
    int i;
    for (i = 0; i <= RT_SPI_MODE_MASK; i++)
    {
        cfg.mode &= (rt_uint8_t)~RT_SPI_MODE_MASK;

        cfg.mode |= i;

        test_uspi_loopback(0);
    }
}

static void test_uspi_loopback_speed(void)
{
    int i;
    for (i = 1; i <= NU_USPI_MAX_MHZ; i++)
    {
        cfg.max_hz = i * 1000000ul ;
        test_uspi_loopback(0);
    }
}


static void test_uspi_loopback_alignment(void)
{
    test_uspi_loopback(0);
    test_uspi_loopback(1);
    test_uspi_loopback(2);
    test_uspi_loopback(3);
}

static rt_err_t utest_tc_init(void)
{
    uspi_dump_test_setting();
    if (RT_NULL == (struct rt_USPI_device *)rt_device_find(UTEST_USPISLAVE_DEVNAME))
        rt_spi_bus_attach_device(&uspi_device, UTEST_USPISLAVE_DEVNAME, UTEST_USPI_DEVNAME, RT_NULL);

    /* Backup original multiple function pin setting */
    g_original_mfp0_setting = SYS->GPE_MFP0;
    g_original_mfp1_setting = SYS->GPE_MFP1;

    /* Set PE multi-function pins for USCI0_DAT0(PE1) and USCI0_DAT1(PE2) */
    SYS->GPE_MFP0 &= ~(SYS_GPE_MFP0_PE2MFP_Msk | SYS_GPE_MFP0_PE3MFP_Msk);
    SYS->GPE_MFP1 &= ~(SYS_GPE_MFP1_PE4MFP_Msk | SYS_GPE_MFP1_PE5MFP_Msk);

    SYS->GPE_MFP0 |= (SYS_GPE_MFP0_PE2MFP_USCI0_CLK | SYS_GPE_MFP0_PE3MFP_USCI0_DAT0);
    SYS->GPE_MFP1 |= (SYS_GPE_MFP1_PE4MFP_USCI0_DAT1 | SYS_GPE_MFP1_PE5MFP_USCI0_CTL1);


    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    // Restore multiple function pin setting
    SYS->GPE_MFP0 = g_original_mfp0_setting;
    SYS->GPE_MFP1 = g_original_mfp1_setting;

    return RT_EOK;
}

static void testcase(void)
{
    //UTEST_UNIT_RUN(test_uspi_loopback_alignment);
    UTEST_UNIT_RUN(test_uspi_loopback_speed);
    //UTEST_UNIT_RUN(test_uspi_loopback_mode);
}
UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"uspi", utest_tc_init, utest_tc_cleanup, 10);
#endif
