/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/
#include "rtconfig.h"
#include "utest.h"

#if defined(RT_USING_INPUT_CAPTURE)
#if defined(BSP_USING_EPWM_CAPTURE)

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
    /* Set PE13 multi-function pins for EPWM1(capture) Channel 0 */
    SYS->GPE_MFP3 &= ~SYS_GPE_MFP3_PE13MFP_Msk;
    SYS->GPE_MFP3 |= SYS_GPE_MFP3_PE13MFP_EPWM1_CH0;

    /* Set PB14 multi-function pins for EPWM1(capture) Channel 1 */
    SYS->GPB_MFP3 &= ~SYS_GPB_MFP3_PB14MFP_Msk;
    SYS->GPB_MFP3 |= SYS_GPB_MFP3_PB14MFP_EPWM1_CH1;

    /* Set PC10 multi-function pins for EPWM1(capture) Channel 2 */
    SYS->GPC_MFP2 &= ~SYS_GPC_MFP2_PC10MFP_Msk;
    SYS->GPC_MFP2 |= SYS_GPC_MFP2_PC10MFP_EPWM1_CH2;

    /* Internal Pull-up */
    GPIO_SetPullCtl(PE, BIT13, GPIO_PUSEL_PULL_UP);
    GPIO_SetPullCtl(PB, BIT14, GPIO_PUSEL_PULL_UP);
    GPIO_SetPullCtl(PC, BIT10, GPIO_PUSEL_PULL_UP);
}

#define NU_INPUTCAPTURE0_DEVNAME "epwm1i0"
#define NU_INPUTCAPTURE1_DEVNAME "epwm1i1"
#define NU_INPUTCAPTURE2_DEVNAME "epwm1i2"

static void inputcapture_epwm_dump_test_setting(void)
{
    rt_kprintf("\n[INPUTCAPTURE EPWM utest] description\n");
    rt_kprintf("  purpose               : Verify EPWM capture basic, watermark and multichannel paths.\n");
    rt_kprintf("  capture device 0      : %s\n", NU_INPUTCAPTURE0_DEVNAME);
    rt_kprintf("  capture device 1      : %s\n", NU_INPUTCAPTURE1_DEVNAME);
    rt_kprintf("  capture device 2      : %s\n\n", NU_INPUTCAPTURE2_DEVNAME);
}

static void nu_inputcapture_epwm_basic_test(void)
{
    nu_capture_basic_test(NU_INPUTCAPTURE0_DEVNAME);
}

static void nu_inputcapture_epwm_watermark_test(void)
{
    nu_capture_watermark_test(NU_INPUTCAPTURE0_DEVNAME);
}

static void nu_inputcapture_epwm_multichannel_test(void)
{
    nu_capture_multichannel_test(NU_INPUTCAPTURE0_DEVNAME, NU_INPUTCAPTURE1_DEVNAME, NU_INPUTCAPTURE2_DEVNAME);
}

static rt_err_t utest_tc_init(void)
{
    inputcapture_epwm_dump_test_setting();
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
    UTEST_UNIT_RUN(nu_inputcapture_epwm_basic_test);
    UTEST_UNIT_RUN(nu_inputcapture_epwm_watermark_test);
    UTEST_UNIT_RUN(nu_inputcapture_epwm_multichannel_test);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"inputcapture_epwm",
                utest_tc_init, utest_tc_cleanup, 30);
#endif  //BSP_USING_EPWM_CAPTURE
#endif  //RT_USING_INPUT_CAPTURE
