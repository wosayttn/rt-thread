/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "rtconfig.h"
#include "utest.h"

#if defined(RT_USING_INPUT_CAPTURE) && defined(BSP_USING_ECAP0)

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
    /* Unlock protected registers */


    /* Set PA10 multi-function pins for ECAP0(capture) Channel 0 */
    /* Set PA9 multi-function pins for ECAP0(capture) Channel 1 */
    /* Set PA8 multi-function pins for ECAP0(capture) Channel 2 */
    SYS->GPA_MFP2 &= ~(SYS_GPA_MFP2_PA10MFP_Msk | SYS_GPA_MFP2_PA9MFP_Msk | SYS_GPA_MFP2_PA8MFP_Msk);
    SYS->GPA_MFP2 |= (SYS_GPA_MFP2_PA10MFP_ECAP0_IC0 | SYS_GPA_MFP2_PA9MFP_ECAP0_IC1 | SYS_GPA_MFP2_PA8MFP_ECAP0_IC2);

    /* Internal Pull-up */
    GPIO_SetPullCtl(PA, (BIT8 | BIT9 | BIT10), GPIO_PUSEL_PULL_UP);
}

#define NU_INPUTCAPTURE0_DEVNAME "ecap0i0"
#define NU_INPUTCAPTURE1_DEVNAME "ecap0i1"
#define NU_INPUTCAPTURE2_DEVNAME "ecap0i2"

static void inputcapture_ecap_dump_test_setting(void)
{
    rt_kprintf("\n[INPUTCAPTURE ECAP utest] description\n");
    rt_kprintf("  purpose               : Verify ECAP capture basic, watermark and multichannel paths.\n");
    rt_kprintf("  capture device 0      : %s\n", NU_INPUTCAPTURE0_DEVNAME);
    rt_kprintf("  capture device 1      : %s\n", NU_INPUTCAPTURE1_DEVNAME);
    rt_kprintf("  capture device 2      : %s\n\n", NU_INPUTCAPTURE2_DEVNAME);
}

static void nu_inputcapture_ecap_basic_test(void)
{
    nu_capture_basic_test(NU_INPUTCAPTURE0_DEVNAME);
}

static void nu_inputcapture_ecap_watermark_test(void)
{
    nu_capture_watermark_test(NU_INPUTCAPTURE0_DEVNAME);
}

static void nu_inputcapture_ecap_multichannel_test(void)
{
    nu_capture_multichannel_test(NU_INPUTCAPTURE0_DEVNAME, NU_INPUTCAPTURE1_DEVNAME, NU_INPUTCAPTURE2_DEVNAME);
}

static rt_err_t utest_tc_init(void)
{
    inputcapture_ecap_dump_test_setting();
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
    UTEST_UNIT_RUN(nu_inputcapture_ecap_basic_test);
    UTEST_UNIT_RUN(nu_inputcapture_ecap_watermark_test);
    UTEST_UNIT_RUN(nu_inputcapture_ecap_multichannel_test);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"inputcapture_ecap",
                utest_tc_init, utest_tc_cleanup, 30);

#endif  //BSP_USING_ECAP && RT_USING_INPUT_CAPTURE
