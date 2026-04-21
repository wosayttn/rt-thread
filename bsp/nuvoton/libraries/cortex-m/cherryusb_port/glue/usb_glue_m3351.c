/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "rtconfig.h"
#include "drv_sys.h"
#include "glue_nuvoton.h"
#if defined(RT_CHERRYUSB_HOST)
#include "usbh_core.h"
#include "usb_hc.h"
#endif
#include "rtthread.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG                 "drv.cherryusb"
#define DBG_TAG                 LOG_TAG
#include "drv_log.h"

/* Types / Structures ---------------------------------------------------------*/

/* Static Function Prototypes ------------------------------------------------*/

/* Static Variables ----------------------------------------------------------*/

/* Functions Implementation --------------------------------------------------*/
#if defined(RT_CHERRYUSB_HOST)
void usb_hc_low_level_init(struct usbh_bus *bus)
{
#if defined(BSP_USING_USBH)
    void *reg_base = (void *)bus->hcd.reg_base;

    if (reg_base == (void *)USBH_BASE)
    {
        /* Enable USBH clock */
        CLK_EnableModuleClock(USBH_MODULE);

        if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
        {
            /* Select PLL (144 MHz) / 3 = 48 MHz */
            CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(3));
        }
        else
        {
            /* Fallback to HIRC 48 MHz / 1 */
            CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
            CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
            CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_HIRC, CLK_CLKDIV0_USB(1));
        }

        /* Set Standard USB Host role and enable USB PHY */
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_USBROLE_STD_USBH;

        NVIC_EnableIRQ(USBH_IRQn);
    }
#else
    (void)bus;
#endif
}

void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
#if defined(BSP_USING_USBH)
    void *reg_base = (void *)bus->hcd.reg_base;

    if (reg_base == (void *)USBH_BASE)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_USBROLE_Msk | SYS_USBPHY_USBEN_Msk);

        NVIC_DisableIRQ(USBH_IRQn);
        CLK_DisableModuleClock(USBH_MODULE);
    }
#else
    (void)bus;
#endif
}

uint8_t usbh_get_port_speed(struct usbh_bus *bus, const uint8_t port)
{
    (void)bus;
    (void)port;

    return USB_SPEED_FULL;
}
#endif /* defined(RT_CHERRYUSB_HOST) */

#if defined(RT_CHERRYUSB_DEVICE)
void usb_dc_low_level_init(uint8_t busid)
{
#if defined(BSP_USING_USBD)
    if (busid == DEF_DC_USBID_FS)
    {
        /* Enable USBD clock */
        CLK_EnableModuleClock(USBD_MODULE);

        if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
        {
            /* Select PLL (144 MHz) / 3 = 48 MHz */
            CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(3));
        }
        else
        {
            /* Fallback to HIRC 48 MHz / 1 */
            CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
            CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
            CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC, CLK_CLKDIV0_USB(1));
        }

        SYS_ResetModule(USBD_RST);

        /* Select Standard USB Device role and enable USB PHY */
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_USBROLE_STD_USBD;
    }
#else
    (void)busid;
#endif
}

void usb_dc_low_level_deinit(uint8_t busid)
{
#if defined(BSP_USING_USBD)
    if (busid == DEF_DC_USBID_FS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_USBROLE_Msk | SYS_USBPHY_USBEN_Msk);

        CLK_DisableModuleClock(USBD_MODULE);
    }
#else
    (void)busid;
#endif
}
#endif /* defined(RT_CHERRYUSB_DEVICE) */
