/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/
#include "rtconfig.h"
#include "utest.h"

#if defined(RT_USING_INPUT_CAPTURE) && defined(BSP_USING_BPWM_CAPTURE)

#include "rtdevice.h"
#include "NuMicro.h"
#include "utest_inputcapture.h"

static void nu_store_mfp(void)
{
    /* Store multifunction pin setting */
}

static void nu_restore_mfp(void)
{
    /* Restore multifunction pin setting */
}

static void nu_configure_capture_pins(void)
{
    /* Set PA0 multi-function pins for BPWM0 */
    SET_BPWM0_CH0_PA0();

    /* Set PA10 multi-function pins for BPWM0 */
    SET_BPWM0_CH1_PA10();

    /* Set PB multi-function pins for BPWM1 */
    SET_BPWM1_CH2_PB9();

    /* Internal Pull-up */
    GPIO_SetPullCtl(PA, BIT10 | BIT0, GPIO_PUSEL_PULL_UP);
    GPIO_SetPullCtl(PB, BIT9, GPIO_PUSEL_PULL_UP);

    /* Enable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT9);
}

#define NU_INPUTCAPTURE0_DEVNAME "bpwm0i0"
#define NU_INPUTCAPTURE1_DEVNAME "bpwm0i1"
#define NU_INPUTCAPTURE2_DEVNAME "bpwm1i2"

static void inputcapture_bpwm_dump_test_setting(void)
{
    rt_kprintf("\n[INPUTCAPTURE BPWM utest] description\n");
    rt_kprintf("  purpose               : Verify BPWM capture basic, watermark and multichannel paths.\n");
    rt_kprintf("  capture device 0      : %s\n", NU_INPUTCAPTURE0_DEVNAME);
    rt_kprintf("  capture device 1      : %s\n", NU_INPUTCAPTURE1_DEVNAME);
    rt_kprintf("  capture device 2      : %s\n\n", NU_INPUTCAPTURE2_DEVNAME);
}

static void nu_inputcapture_bpwm_basic_test(void)
{
    nu_capture_basic_test(NU_INPUTCAPTURE0_DEVNAME);
}

static void nu_inputcapture_bpwm_watermark_test(void)
{
    nu_capture_watermark_test(NU_INPUTCAPTURE0_DEVNAME);
}

static void nu_inputcapture_bpwm_multichannel_test(void)
{
    nu_capture_multichannel_test(NU_INPUTCAPTURE0_DEVNAME, NU_INPUTCAPTURE1_DEVNAME, NU_INPUTCAPTURE2_DEVNAME);
}

static rt_err_t utest_tc_init(void)
{
    inputcapture_bpwm_dump_test_setting();
    /* Store multiple function pin setting */
    nu_store_mfp();

    /* Initialize pin, prevent other IPs from changing multifunction pin when board init. */
    nu_configure_capture_pins();

    nu_init_inputcapture_test();

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    // Restore multiple function pin setting to original setting in here if necessary.
    nu_restore_mfp();

    nu_deinit_inputcapture_test();

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(nu_inputcapture_bpwm_basic_test);
    UTEST_UNIT_RUN(nu_inputcapture_bpwm_watermark_test);
    UTEST_UNIT_RUN(nu_inputcapture_bpwm_multichannel_test);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"inputcapture_bpwm",
                utest_tc_init, utest_tc_cleanup, 30);

#endif  //BSP_USING_BPWM_CAPTURE & RT_USING_INPUT_CAPTURE
