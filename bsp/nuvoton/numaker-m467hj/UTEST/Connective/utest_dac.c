/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_DAC) && defined(BSP_USING_DAC0) && defined(BSP_USING_EADC0)

#include "rtdevice.h"

#include "NuMicro.h"

#define UTEST_DAC_DEV_NAME     "dac0"
#define UTEST_ADC_DEV_NAME     "eadc0"

#define UTEST_DAC_CH_NUM       1
#define UTEST_DAC_TESTCH_MSK  (BIT0)

#define UTEST_USED_ADC_CH_IDX  7

static void dac_dump_test_setting(void)
{
    rt_kprintf("\n[DAC utest] description\n");
    rt_kprintf("  purpose               : Verify DAC output and EADC loopback on UNO_A0.\n");
    rt_kprintf("  dac device            : %s\n", UTEST_DAC_DEV_NAME);
    rt_kprintf("  eadc device           : %s\n", UTEST_ADC_DEV_NAME);
    rt_kprintf("  dac channel count     : %d\n", UTEST_DAC_CH_NUM);
    rt_kprintf("  dac test mask         : 0x%08x\n", UTEST_DAC_TESTCH_MSK);
    rt_kprintf("  adc loopback channel  : %d\n\n", UTEST_USED_ADC_CH_IDX);
}

static void test_dac0(void)
{
    rt_dac_device_t dac_dev;
    rt_int32_t i;
    rt_uint32_t u32Value = 1000;

    dac_dev = (rt_dac_device_t)rt_device_find(UTEST_DAC_DEV_NAME);
    uassert_true(dac_dev != RT_NULL);

    for (i = 0; i < UTEST_DAC_CH_NUM; i++)
    {
        if (UTEST_DAC_TESTCH_MSK & (0x1U << i))
        {
            uassert_true(rt_dac_enable(dac_dev, i) == RT_EOK);
            uassert_true(rt_dac_write(dac_dev, i, u32Value) == RT_EOK);
            uassert_true(rt_dac_disable(dac_dev, i) == RT_EOK);
            uassert_true(rt_dac_write(dac_dev, i, u32Value) != RT_EOK);
        }
    }

    /* Test over max channel */
    uassert_true(rt_dac_enable(dac_dev, UTEST_DAC_CH_NUM + 1) != RT_EOK);
    uassert_true(rt_dac_write(dac_dev, UTEST_DAC_CH_NUM + 1, u32Value) != 1);
}

static void test_dac0_unoa0(void)
{
    int i;

    rt_dac_device_t dac_dev;
    rt_adc_device_t adc_dev;

    rt_uint16_t u16WValue;
    rt_uint16_t u16RValue;

    rt_uint16_t au32Cases[3] = { 1, 2047, 4095 };

    dac_dev = (rt_dac_device_t)rt_device_find(UTEST_DAC_DEV_NAME);
    uassert_true(dac_dev != RT_NULL);

    adc_dev = (rt_adc_device_t)rt_device_find(UTEST_ADC_DEV_NAME);
    uassert_true(adc_dev != RT_NULL);

    for (i = 0; i < 3; i++)
    {
        u16WValue = au32Cases[i];
        u16RValue = ~au32Cases[i];

        rt_kprintf("DAC set %08x\n", u16WValue);

        uassert_true(rt_dac_enable(dac_dev, 0) == RT_EOK);
        uassert_true(rt_dac_write(dac_dev, 0, u16WValue) == RT_EOK);

        rt_thread_mdelay(500);

        uassert_true(rt_adc_enable(adc_dev, UTEST_USED_ADC_CH_IDX) == RT_EOK);
        u16RValue = rt_adc_read(adc_dev, UTEST_USED_ADC_CH_IDX);

        rt_kprintf("ADC get %08x\n", u16RValue);

        rt_kprintf("W=%08x, R=%08x\n", u16WValue, u16RValue);
        uassert_in_range(u16RValue, u16WValue - 32, u16WValue + 32);
    }

    uassert_true(rt_dac_disable(dac_dev, 0) == RT_EOK);
    uassert_true(rt_adc_disable(adc_dev, UTEST_USED_ADC_CH_IDX) == RT_EOK);

}

static rt_err_t utest_tc_init(void)
{
    dac_dump_test_setting();
    rt_kprintf("Sleep 5 seconds, please change RXD switch of VCOM DIP SWITCH to OFF.\n");
    rt_thread_mdelay(5000);
    rt_kprintf("Go\n");

    /* Set multi-function pin for DAC voltage output */
    SYS->GPB_MFP3 &= ~(SYS_GPB_MFP3_PB12MFP_Msk);
    SYS->GPB_MFP3 |= (SYS_GPB_MFP3_PB12MFP_DAC0_OUT);

    /* Set PB.12 to input mode */
    PB->MODE &= ~(GPIO_MODE_MODE12_Msk) ;

    SYS->GPB_MFP1 &= ~(SYS_GPB_MFP1_PB7MFP_Msk);
    SYS->GPB_MFP1 |= (SYS_GPB_MFP1_PB7MFP_EADC0_CH7);

    /* Disable digital input path of analog pin DAC0_OUT to prevent leakage */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT7 | BIT12);

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_kprintf("Sleep 5 seconds, please change RXD switch of VCOM DIP SWITCH to ON.\n");
    rt_thread_mdelay(5000);
    rt_kprintf("Go\n");

    /* Set multi-function pin for DAC voltage output */
    SYS->GPB_MFP3 &= ~(SYS_GPB_MFP3_PB12MFP_Msk);
    SYS->GPB_MFP3 |= (SYS_GPB_MFP3_PB12MFP_UART0_RXD);

    /* Set PB.12 to input mode */
    PB->MODE |= GPIO_MODE_MODE12_Msk;

    SYS->GPB_MFP1 &= ~(SYS_GPB_MFP1_PB7MFP_Msk);

    /* Enable digital input path of analog pin */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT7 | BIT12);

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_dac0);
    UTEST_UNIT_RUN(test_dac0_unoa0);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"dac",
                utest_tc_init, utest_tc_cleanup, 5);

#endif //BSP_USING_DAC && defined(BSP_USING_DAC0) && defined(BSP_USING_EADC0)

