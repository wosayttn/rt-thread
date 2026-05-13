/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(RT_USING_INPUT_CAPTURE)

#include "rtdevice.h"
#include "NuMicro.h"
#include "utest_inputcapture.h"

typedef enum
{
    DEV_CAP0 = 0,
    DEV_CAP1 = 1,
    DEV_CAP2 = 2,
    DEV_END  = 3
} cap_dev;

#define WATERMARK_DEFAULT   (RT_INPUT_CAPTURE_RB_SIZE / 2)
#define GET_PULSEWIDTH_US(freq, duty) (((1000000 / (freq)) * (duty)) / 100)
#define WATERMARK_TEST  ((WATERMARK_DEFAULT * 4) / 5) //WATERMARK_DEFAULT * 80%

static void inputcapture_dump_test_setting(void)
{
    rt_kprintf("\n[INPUTCAPTURE helper] description\n");
    rt_kprintf("  purpose               : Provide shared capture tests driven by EPWM wave generation.\n");
    rt_kprintf("  watermark default     : %d\n", WATERMARK_DEFAULT);
    rt_kprintf("  watermark test        : %d\n", WATERMARK_TEST);
    rt_kprintf("  generator frequencies : %d / %d / %d Hz\n\n", 1000, 1000, 1000);
}

static uint32_t volatile g_u32DataInBuffer = 0;
static struct rt_inputcapture_data capture_data_basic_test[WATERMARK_DEFAULT];
static struct rt_inputcapture_data capture_data_watermark_test[WATERMARK_TEST];
static volatile uint32_t g_u32DataSize[DEV_END] = {0};
static struct rt_inputcapture_data capture_data_multichannel_test[DEV_END][WATERMARK_DEFAULT] = {0};

static void nu_init_functionGen()
{
    /* EPWM clock frequency is set double to PCLK: select EPWM module clock source as PLL */
    CLK_EnableModuleClock(EPWM0_MODULE);
    CLK_SetModuleClock(EPWM0_MODULE, CLK_CLKSEL2_EPWM0SEL_PCLK0, 0);

    /* Set PA5 multi-function pins for EPWM0(output waveform) Channel 0 */
    SYS->GPA_MFP1 &= ~SYS_GPA_MFP1_PA5MFP_Msk;
    SYS->GPA_MFP1 |= SYS_GPA_MFP1_PA5MFP_EPWM0_CH0;

    /* Set PA3 multi-function pins for EPWM0(output waveform) Channel 2 */
    SYS->GPA_MFP0 &= ~SYS_GPA_MFP0_PA3MFP_Msk;
    SYS->GPA_MFP0 |= SYS_GPA_MFP0_PA3MFP_EPWM0_CH2;

    /* Set PA1 multi-function pins for EPWM0(output waveform) Channel 4 */
    SYS->GPA_MFP0 &= ~SYS_GPA_MFP0_PA1MFP_Msk;
    SYS->GPA_MFP0 |= SYS_GPA_MFP0_PA1MFP_EPWM0_CH4;
}

static void nu_deinit_functionGen()
{
    CLK_DisableModuleClock(EPWM0_MODULE);
}

void nu_init_inputcapture_test()
{
    inputcapture_dump_test_setting();
    nu_init_functionGen();
}

void nu_deinit_inputcapture_test()
{
    nu_deinit_functionGen();
}

static void nu_open_functionGen(EPWM_T *epwm, uint32_t u32ChannelNum, uint32_t u32Freq, uint32_t u32Duty)
{
    rt_kprintf("%s %d %d %d\n", __func__, u32ChannelNum, u32Freq, u32Duty);

    /* Set EPWM output configuration */
    EPWM_ConfigOutputChannel(epwm, u32ChannelNum, u32Freq, u32Duty);

    /* Enable EPWM Output path for EPWM */
    EPWM_EnableOutput(epwm, 0x1 << u32ChannelNum);

    /* Enable Timer for EPWM */
    EPWM_Start(epwm, 0x1 << u32ChannelNum);
}

static void nu_close_functionGen(EPWM_T *epwm, uint32_t u32ChannelNum)
{
    EPWM_ForceStop(epwm, 0x1 << u32ChannelNum);
}

static rt_err_t capture_rx(rt_device_t dev, rt_size_t size)
{
    g_u32DataInBuffer = size;

    nu_close_functionGen(EPWM0, 0);

    return RT_EOK;
}

void nu_capture_basic_test(char *pcDeviceName)
{
    rt_device_t capture;
    rt_err_t ret;
    int i = 0, read_count;
    const uint32_t c_u32Freq = 1000; /*10000;*/
    const uint8_t c_u8Duty = 30;
    rt_uint32_t u32Counter = 0;

    capture = rt_device_find(pcDeviceName);
    if (!capture)
    {
        uassert_not_null(capture);
        goto exit_testcase_loop;
    }

    rt_device_control(capture, INPUTCAPTURE_CMD_CLEAR_BUF, 0);

    /* Set rx indicate function */
    ret = rt_device_set_rx_indicate(capture, capture_rx);
    uassert_int_equal(ret, RT_EOK);

    /* Open this device */
    ret = rt_device_open(capture, RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    rt_kprintf("%s %d\n", __func__, __LINE__);

    /* Freq.=10000Hz duty=c_u8Duty%, low level:96us, high level:c_u8Dutyus */
    nu_open_functionGen(EPWM0, 0, c_u32Freq, c_u8Duty);

    rt_kprintf("%s %d\n", __func__, __LINE__);

    while (g_u32DataInBuffer == 0)
    {
        rt_thread_mdelay(1);
        u32Counter++;

        if (u32Counter > 3000)
        {
            uassert_true(0);
            goto exit_testcase_loop;
        }
    }

    rt_kprintf("%s %d\n", __func__, __LINE__);

    /* Read data */
    read_count = rt_device_read(capture, 0, &capture_data_basic_test, g_u32DataInBuffer);

    rt_kprintf("%s %d\n", __func__, __LINE__);

    g_u32DataInBuffer = 0;

    /* Skip first two initial data */
    for (i = 2; i < read_count; i++)
    {
        //rt_kprintf("[%d]Level:%c, Pulse(us):%d\n", i, capture_data_basic_test[i].is_high ? 'H' : 'L', capture_data_basic_test[i].pulsewidth_us);

        /* Tolerance +-1 because H/W limitation */
        if (capture_data_basic_test[i].is_high)
        {
            //rt_kprintf("high: %d - %d\n", GET_PULSEWIDTH_US(c_u32Freq, c_u8Duty) - 1, GET_PULSEWIDTH_US(c_u32Freq, c_u8Duty) + 1);
            uassert_in_range(capture_data_basic_test[i].pulsewidth_us, GET_PULSEWIDTH_US(c_u32Freq, c_u8Duty) - 1, GET_PULSEWIDTH_US(c_u32Freq, c_u8Duty) + 1);
        }
        else
        {
            //rt_kprintf("low: %d - %d\n", GET_PULSEWIDTH_US(c_u32Freq, 100 - c_u8Duty) - 1, GET_PULSEWIDTH_US(c_u32Freq, 100 - c_u8Duty) + 1);
            uassert_in_range(capture_data_basic_test[i].pulsewidth_us, GET_PULSEWIDTH_US(c_u32Freq, 100 - c_u8Duty) - 1, GET_PULSEWIDTH_US(c_u32Freq, 100 - c_u8Duty) + 1);
        }
    }

exit_testcase_loop:

    if (capture)
    {
        rt_device_control(capture, INPUTCAPTURE_CMD_CLEAR_BUF, 0);

        ret = rt_device_close(capture);
    }

    return;
}

void nu_capture_watermark_test(char *pcDeviceName)
{
    const uint32_t c_u32WaterMark = WATERMARK_TEST;
    const uint32_t c_u32WaterMark_default = WATERMARK_DEFAULT;
    const uint32_t c_u32Freq = 100;
    const uint8_t c_u8Duty = 1;
    rt_device_t capture = 0;
    rt_err_t ret;
    int i = 0, read_count;

    capture = rt_device_find(pcDeviceName);
    if (!capture)
    {
        uassert_not_null(capture);
        goto exit_testcase_loop;
    }

    rt_device_control(capture, INPUTCAPTURE_CMD_CLEAR_BUF, 0);

    /* Set rx indicate function */
    ret = rt_device_set_rx_indicate(capture, capture_rx);
    uassert_int_equal(ret, RT_EOK);

    /* Open this device */
    ret = rt_device_open(capture, RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    rt_device_control(capture, INPUTCAPTURE_CMD_SET_WATERMARK, (void *)&c_u32WaterMark);

    /* Freq.=100Hz duty=1%, low level:9900us, high level:100us */
    nu_open_functionGen(EPWM0, 0, c_u32Freq, c_u8Duty);

    while (g_u32DataInBuffer == 0) {};

    /* Read data */
    read_count = rt_device_read(capture, 0, &capture_data_watermark_test[0], g_u32DataInBuffer);
    uassert_int_equal(c_u32WaterMark, g_u32DataInBuffer);
    g_u32DataInBuffer = 0;

    /* Skip first two initial data */
    for (i = 2; i < read_count; i++)
    {
        //rt_kprintf("[%d]Level:%c, Pulse(us):%d\n", i, capture_data_watermark_test[i].is_high ? 'H' : 'L', capture_data_watermark_test[i].pulsewidth_us);

        /* Tolerance +-1 because H/W limitation */
        if (capture_data_watermark_test[i].is_high)
            uassert_in_range(capture_data_watermark_test[i].pulsewidth_us, GET_PULSEWIDTH_US(c_u32Freq, c_u8Duty) - 1, GET_PULSEWIDTH_US(c_u32Freq, c_u8Duty) + 1);
        else
            uassert_in_range(capture_data_watermark_test[i].pulsewidth_us, GET_PULSEWIDTH_US(c_u32Freq, 100 - c_u8Duty) - 1, GET_PULSEWIDTH_US(c_u32Freq, 100 - c_u8Duty) + 1);
    }

exit_testcase_loop:

    if (capture)
    {
        rt_device_control(capture, INPUTCAPTURE_CMD_SET_WATERMARK, (void *)&c_u32WaterMark_default);

        rt_device_control(capture, INPUTCAPTURE_CMD_CLEAR_BUF, 0);

        rt_device_close(capture);
    }

    return;
}

static rt_err_t multi_capture0_rx(rt_device_t dev, rt_size_t size)
{
    nu_close_functionGen(EPWM0, 0);
    g_u32DataSize[DEV_CAP0] = size;

    return RT_EOK;;
}

static rt_err_t multi_capture1_rx(rt_device_t dev, rt_size_t size)
{
    nu_close_functionGen(EPWM0, 2);

    g_u32DataSize[DEV_CAP1] = size;

    return RT_EOK;
}

static rt_err_t multi_capture2_rx(rt_device_t dev, rt_size_t size)
{
    nu_close_functionGen(EPWM0, 4);

    g_u32DataSize[DEV_CAP2] = size;

    return RT_EOK;
}

void nu_capture_multichannel_test(char *pcDeviceName0, char *pcDeviceName1, char *pcDeviceName2)
{
    rt_err_t ret;
    rt_device_t capture[DEV_END];
    int i = 0, dev_no = 0, read_count[DEV_END];
    uint32_t c_u32Freq[DEV_END] = {1000, 500, 1000};
    uint8_t c_u8Duty[DEV_END] = {20, 10, 50};

    capture[DEV_CAP0] = rt_device_find(pcDeviceName0);
    if (!capture[DEV_CAP0])
    {
        uassert_not_null(capture[DEV_CAP0]);
        goto exit_testcase_loop;
    }

    capture[DEV_CAP1] = rt_device_find(pcDeviceName1);
    if (!capture[DEV_CAP1])
    {
        uassert_not_null(capture[DEV_CAP1]);
        goto exit_testcase_loop;
    }

    capture[DEV_CAP2] = rt_device_find(pcDeviceName2);
    if (!capture[DEV_CAP2])
    {
        uassert_not_null(capture[DEV_CAP2]);
        goto exit_testcase_loop;
    }

    /* Set rx indicate function */
    ret = rt_device_set_rx_indicate(capture[DEV_CAP0], multi_capture0_rx);
    uassert_int_equal(ret, RT_EOK);
    ret = rt_device_set_rx_indicate(capture[DEV_CAP1], multi_capture1_rx);
    uassert_int_equal(ret, RT_EOK);
    ret = rt_device_set_rx_indicate(capture[DEV_CAP2], multi_capture2_rx);
    uassert_int_equal(ret, RT_EOK);

    /* Open this device */
    for (dev_no = 0; dev_no < DEV_END; dev_no++)
    {
        rt_kprintf("%s clear/open DEV%d_HDL: %08x\n", __func__, dev_no, capture[dev_no]);
        g_u32DataSize[dev_no] = 0 ;
        rt_device_control(capture[dev_no], INPUTCAPTURE_CMD_CLEAR_BUF, 0);
        ret = rt_device_open(capture[dev_no], RT_DEVICE_FLAG_INT_RX);
        uassert_int_equal(ret, RT_EOK);
    }

    /* Freq.=1000Hz duty=20%, low level:800us, high level:200us */
    nu_open_functionGen(EPWM0, 0, c_u32Freq[0], c_u8Duty[0]);
    /* Freq.=500Hz duty=10%, low level:1800us, high level:200us */
    nu_open_functionGen(EPWM0, 2, c_u32Freq[1], c_u8Duty[1]);
    /* Freq.=1000Hz duty=50%, low level:500us, high level:500us */
    nu_open_functionGen(EPWM0, 4, c_u32Freq[2], c_u8Duty[2]);

    for (dev_no = 0; dev_no < DEV_END; dev_no++)
    {
        rt_kprintf("[%s %d] %d %d\n", __func__, __LINE__, dev_no, g_u32DataSize[dev_no]);
        while (g_u32DataSize[dev_no] == 0) {};
    }

    /* Read data */
    for (dev_no = 0; dev_no < DEV_END; dev_no++)
    {
        rt_kprintf("%s %d %d\n", __func__, dev_no, g_u32DataSize[dev_no]);

        read_count[dev_no] = rt_device_read(capture[dev_no], 0, &capture_data_multichannel_test[dev_no][0], g_u32DataSize[dev_no]);

        uassert_int_equal(read_count[dev_no], WATERMARK_DEFAULT);

        /* Skip first two initial data */
        for (i = 2; i < read_count[dev_no]; i++)
        {
            //rt_kprintf("[%d]Dev_no:%d, Level:%c, Pulse(us):%d\n", i, dev_no, capture_data_multichannel_test[dev_no][i].is_high ? 'H' : 'L', capture_data_multichannel_test[dev_no][i].pulsewidth_us);
            /* Tolerance +-1 because H/W limitation */
            if (capture_data_multichannel_test[dev_no][i].is_high)
            {
                uassert_in_range(capture_data_multichannel_test[dev_no][i].pulsewidth_us, GET_PULSEWIDTH_US(c_u32Freq[dev_no], c_u8Duty[dev_no]) - 1, GET_PULSEWIDTH_US(c_u32Freq[dev_no], c_u8Duty[dev_no]) + 1);
            }
            else
            {
                uassert_in_range(capture_data_multichannel_test[dev_no][i].pulsewidth_us, GET_PULSEWIDTH_US(c_u32Freq[dev_no], 100 - c_u8Duty[dev_no]) - 1, GET_PULSEWIDTH_US(c_u32Freq[dev_no], 100 - c_u8Duty[dev_no]) + 1);
            }
        }
    }

exit_testcase_loop:

    for (dev_no = 0; dev_no < DEV_END; dev_no++)
    {
        rt_kprintf("%s %d DEV%d_HDL: %08x\n", __func__, __LINE__, dev_no, capture[dev_no]);
        rt_device_control(capture[dev_no], INPUTCAPTURE_CMD_CLEAR_BUF, 0);
        ret = rt_device_close(capture[dev_no]);
        capture[dev_no] = NULL;
    }

    return;
}
#endif  //RT_USING_INPUT_CAPTURE
