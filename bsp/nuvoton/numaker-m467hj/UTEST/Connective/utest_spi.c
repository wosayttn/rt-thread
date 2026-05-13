/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(RT_USING_SPI) && defined(BSP_USING_SPI)

#include "rtdevice.h"
#include "NuMicro.h"


#define UTEST_SPI_DEVNAME   "spi2"
#define UTEST_SPISLAVE_DEVNAME   UTEST_SPI_DEVNAME"0"
#define NU_TEST_LEN (1*2*3*4)
/* Limit max SPI clock to 45MHz */
#define NU_SPI_MAX_MHZ  (45)
#define NU_SPI_MAX_HZ   (NU_SPI_MAX_MHZ*1000000)
#define NU_TEST_PATTERN 0xa5u

#define NU_UTEST_DEFAULT_SPI_CFG            \
{                                           \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,     \
    .data_width = 8,                        \
    .max_hz = NU_SPI_MAX_HZ,                \
}

static uint32_t g_original_mfp_setting;
static struct rt_spi_device spi_device;
static struct rt_spi_configuration cfg = NU_UTEST_DEFAULT_SPI_CFG;

static void spi_dump_test_setting(void)
{
    rt_kprintf("\n[SPI utest] description\n");
    rt_kprintf("  purpose               : Verify SPI loopback alignment, speed and mode coverage.\n");
    rt_kprintf("  spi bus               : %s\n", UTEST_SPI_DEVNAME);
    rt_kprintf("  spi device            : %s\n", UTEST_SPISLAVE_DEVNAME);
    rt_kprintf("  transfer length       : %d\n", NU_TEST_LEN);
    rt_kprintf("  max clock (Hz)        : %d\n\n", NU_SPI_MAX_HZ);
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

static void test_spi_loopback(int align_move)
{
    struct rt_spi_message msg;
    uint8_t txbuf[NU_TEST_LEN + 4];
    uint8_t rxbuf[NU_TEST_LEN + 4];
    int i;

    struct rt_spi_device *spi_dev;

    spi_dev = (struct rt_spi_device *)rt_device_find(UTEST_SPISLAVE_DEVNAME);
    uassert_true(spi_dev != RT_NULL);

    rt_kprintf("cfg.max_hz: %d\n", cfg.max_hz);

    for (i = 8; i <= 32; i += 8)
    {
        uint8_t bytes_per_word;
        cfg.data_width = i;
        bytes_per_word = cfg.data_width / 8;

        if (rt_spi_configure(spi_dev, &cfg) == RT_EOK)
        {
            int j;
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
                rt_spi_transfer_message(spi_dev, &msg);

                // Compare all bytes in used TX & RX buffer.
                uassert_buf_equal((const char *)&txbuf[align_move], (const char *)&rxbuf[align_move], j);

                // Check all bytes in unused RX Buffer are NU_TEST_PATTERN .
                uassert_false(find_diff_in_buffer(NU_TEST_PATTERN, (void *)&rxbuf[align_move + j], (NU_TEST_LEN - j)));
            }

        }//if
    }

    return;
}

static void test_spi_loopback_mode(void)
{
    int i;
    for (i = 0; i <= RT_SPI_MODE_MASK; i++)
    {
        cfg.mode &= (rt_uint8_t)~RT_SPI_MODE_MASK;

        cfg.mode |= i;

        /* Not support Slave mode. */
        cfg.mode &= ~RT_SPI_SLAVE;

        rt_kprintf("cfg.mode: %d\n", cfg.mode);

        test_spi_loopback(0);
    }
}

static void test_spi_loopback_speed(void)
{
    int i;
    for (i = 1; i <= NU_SPI_MAX_MHZ; i++)
    {
        cfg.max_hz = i * 1000000ul ;
        test_spi_loopback(0);
    }
}

static void test_spi_loopback_alignment(void)
{
    test_spi_loopback(0);
    test_spi_loopback(1);
    test_spi_loopback(2);
    test_spi_loopback(3);
}

static rt_err_t utest_tc_init(void)
{
    spi_dump_test_setting();
    // Backup original multiple function pin setting.
    g_original_mfp_setting = SYS->GPA_MFP2;

    if (RT_NULL == (struct rt_spi_device *)rt_device_find(UTEST_SPISLAVE_DEVNAME))
        rt_spi_bus_attach_device(&spi_device, UTEST_SPISLAVE_DEVNAME, UTEST_SPI_DEVNAME, RT_NULL);

    SYS->GPA_MFP2 &= ~(SYS_GPA_MFP2_PA11MFP_Msk | SYS_GPA_MFP2_PA10MFP_Msk | SYS_GPA_MFP2_PA9MFP_Msk | SYS_GPA_MFP2_PA8MFP_Msk);
    SYS->GPA_MFP2 |= (SYS_GPA_MFP2_PA11MFP_SPI2_SS | SYS_GPA_MFP2_PA10MFP_SPI2_CLK | SYS_GPA_MFP2_PA9MFP_SPI2_MISO | SYS_GPA_MFP2_PA8MFP_SPI2_MOSI);

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    SYS->GPA_MFP2 = g_original_mfp_setting;

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_spi_loopback_alignment);
    UTEST_UNIT_RUN(test_spi_loopback_speed);
    UTEST_UNIT_RUN(test_spi_loopback_mode);
}
UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"spi", utest_tc_init, utest_tc_cleanup, 10);

#endif
