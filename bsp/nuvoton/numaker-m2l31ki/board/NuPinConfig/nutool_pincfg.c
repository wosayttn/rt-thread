/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtconfig.h"
#include "NuMicro.h"

void nutool_pincfg_init_uart0(void)
{
    SYS->GPB_MFP3 = (SYS->GPB_MFP3 & ~(SYS_GPB_MFP3_PB12MFP_Msk | SYS_GPB_MFP3_PB13MFP_Msk)) |
                    (SYS_GPB_MFP3_PB12MFP_UART0_RXD | SYS_GPB_MFP3_PB13MFP_UART0_TXD);
}

#if defined(BSP_USING_USB)
void nutool_pincfg_init_usb(void)
{
    SYS->GPA_MFP3 = (SYS->GPA_MFP3 & ~(SYS_GPA_MFP3_PA12MFP_Msk | SYS_GPA_MFP3_PA13MFP_Msk |
                                       SYS_GPA_MFP3_PA14MFP_Msk | SYS_GPA_MFP3_PA15MFP_Msk)) |
                    (SYS_GPA_MFP3_PA12MFP_USB_VBUS | SYS_GPA_MFP3_PA13MFP_USB_D_N |
                     SYS_GPA_MFP3_PA14MFP_USB_D_P | SYS_GPA_MFP3_PA15MFP_USB_OTG_ID);
}
#endif

void nutool_pincfg_init(void)
{
#if defined(BSP_USING_UART0)
    nutool_pincfg_init_uart0();
#endif

#if defined(BSP_USING_USB)
    nutool_pincfg_init_usb();
#endif
}

void nutool_pincfg_deinit(void)
{
}
