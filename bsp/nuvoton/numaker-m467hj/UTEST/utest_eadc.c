/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_EADC) && defined(BSP_USING_EADC0)

#include "rtdevice.h"
#include "NuMicro.h"
#include "drv_gpio.h"
#include "drv_common.h"

#define UTEST_EADC_DEV_NAME      "eadc0"
#define UTEST_EADC_CH_NUM        16
#define UTEST_EADC_TESTCH_MSK     ((1<<UTEST_EADC_CH_NUM)-1)
static int i32PinArr[] = {0, 1, 8, 9, 14, 15};
static uint32_t s_u32MFP0, s_u32MFP1, s_u32MFP2, s_u32MFP3;

static void eadc_dump_test_setting(void)
{
    rt_kprintf("\n[EADC utest] description\n");
    rt_kprintf("  purpose               : Verify EADC channel readback and enable/disable flow.\n");
    rt_kprintf("  eadc device           : %s\n", UTEST_EADC_DEV_NAME);
    rt_kprintf("  channel count         : %d\n", UTEST_EADC_CH_NUM);
    rt_kprintf("  test channel mask     : 0x%08x\n", UTEST_EADC_TESTCH_MSK);
    rt_kprintf("  analog pin count      : %d\n\n", (int)(sizeof(i32PinArr) / sizeof(i32PinArr[0])));
}

static void test_eadc0(void)
{
    rt_adc_device_t eadc_dev;
    rt_uint32_t u32nuadccnt = 0;

    eadc_dev = (rt_adc_device_t)rt_device_find(UTEST_EADC_DEV_NAME);
    uassert_true(eadc_dev != RT_NULL);

    for (u32nuadccnt = 0; u32nuadccnt < UTEST_EADC_CH_NUM; u32nuadccnt++)
    {
        if (UTEST_EADC_TESTCH_MSK & (0x1U << u32nuadccnt))
        {
            uint32_t u32Value;
            uassert_true(rt_adc_enable(eadc_dev, u32nuadccnt) == RT_EOK);

            u32Value = rt_adc_read(eadc_dev, u32nuadccnt);
            rt_kprintf("CH#%d: %08x\n", u32nuadccnt, u32Value);
            uassert_true(u32Value != 0xFFFFFFFFUL);

            uassert_true(rt_adc_disable(eadc_dev, u32nuadccnt) == RT_EOK);

            uassert_true(rt_adc_read(eadc_dev, u32nuadccnt) == 0xFFFFFFFFUL);
        }
    }

    /* Test over max channel */
    uassert_true(rt_adc_enable(eadc_dev, UTEST_EADC_CH_NUM + 1) != RT_EOK);
    uassert_true(rt_adc_read(eadc_dev, UTEST_EADC_CH_NUM + 1) == 0xFFFFFFFFUL);

}

static rt_err_t utest_tc_init(void)
{
    eadc_dump_test_setting();
    s_u32MFP0 = SYS->GPB_MFP0;
    s_u32MFP1 = SYS->GPB_MFP1;
    s_u32MFP2 = SYS->GPB_MFP2;
    s_u32MFP3 = SYS->GPB_MFP3;

    for (int i = 0; i < sizeof(i32PinArr) / sizeof(i32PinArr[0]); i++)
    {
        int pin = NU_GET_PININDEX(NU_PB, i32PinArr[i]);
        GPIO_T *port = (GPIO_T *)(GPIOA_BASE + (0x40) * NU_GET_PORT(pin));

        /* Set EADC function on these EADC pins */
        nu_pin_func(pin, 1);

        /* Disable digital path on these EADC pins */
        GPIO_DISABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PINS(pin)));
    }

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    for (int i = 0; i < sizeof(i32PinArr) / sizeof(i32PinArr[0]); i++)
    {
        int pin = NU_GET_PININDEX(NU_PB, i32PinArr[i]);
        GPIO_T *port = (GPIO_T *)(GPIOA_BASE + (0x40) * NU_GET_PORT(pin));

        /* Disable digital path on these EADC pins */
        GPIO_ENABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PINS(pin)));
    }

    SYS->GPB_MFP0 = s_u32MFP0;
    SYS->GPB_MFP1 = s_u32MFP1;
    SYS->GPB_MFP2 = s_u32MFP2;
    SYS->GPB_MFP3 = s_u32MFP3;

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_eadc0);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"eadc",
                utest_tc_init, utest_tc_cleanup, 5);

#endif // #if defined(BSP_USING_EADC) && defined(BSP_USING_EADC0)

