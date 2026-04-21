/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtthread.h"

#include "drv_sys.h"
#include "glue_nuvoton.h"

#if defined(RT_CHERRYUSB_HOST)
#include "usbh_core.h"
#include "usb_hc.h"
#endif

/* #undef LOG_TAG */
#define LOG_TAG "drv.cherryusb"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#if defined(RT_CHERRYUSB_HOST)
/* To execute msh command to bring up cherryusb stack. */
/* msh /> usbh_init 0 0x40009000 */

#if defined(BSP_USING_USBH)
/* Not support OHCI. */
#endif

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
            CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(2));
        }
        else
        {
            /* Enable HIRC48 clock */
            CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
            CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);

            CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_HIRC48, CLK_CLKDIV0_USB(1));
        }

    #if !defined(BSP_USING_OTG)
        /* Set Standard USB Host role */
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk | SYS_USBPHY_USBROLE_STD_USBH;
    #endif

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
    #if !defined(BSP_USING_OTG)
        SYS->USBPHY &= ~(SYS_USBPHY_USBROLE_Msk | SYS_USBPHY_OTGPHYEN_Msk);
    #endif

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
#endif

#if defined(RT_CHERRYUSB_DEVICE)
void usb_dc_low_level_init(uint8_t busid)
{
#if !defined(BSP_USING_OTG) && defined(BSP_USING_USBD)
    if (busid == DEF_DC_USBID_FS)
    {
        /* Enable USBD clock */
        CLK_EnableModuleClock(USBD_MODULE);

        if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
        {
            CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(2));
        }
        else
        {
            /* Enable HIRC48 clock */
            CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
            CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);

            CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC48, CLK_CLKDIV0_USB(1));
        }

        SYS_ResetModule(USBD_RST);

        /* Select USBD: role = Standard USB Device (0x0), enable OTG PHY, and set SBO */
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;
    }
#else
    (void)busid;
#endif
}

void usb_dc_low_level_deinit(uint8_t busid)
{
#if !defined(BSP_USING_OTG) && defined(BSP_USING_USBD)
    if (busid == DEF_DC_USBID_FS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_USBROLE_Msk | SYS_USBPHY_OTGPHYEN_Msk);

        CLK_DisableModuleClock(USBD_MODULE);
    }
#else
    (void)busid;
#endif
}
#endif

#if defined(BSP_USING_OTG)
/* OTG interrupt entry */
void USBOTG_IRQHandler(void)
{
    rt_interrupt_enter();

    __IO uint32_t reg = OTG->INTSTS;

    /* usb id pin status change */
    if (reg & OTG_INTSTS_IDCHGIF_Msk)
    {
        OTG_CLR_INT_FLAG(OTG_INTSTS_IDCHGIF_Msk);
        LOG_I("usb id change");
    }

    /* usb acts as host */
    if (reg & OTG_INTSTS_HOSTIF_Msk)
    {
        OTG_CLR_INT_FLAG(OTG_INTSTS_HOSTIF_Msk);
        LOG_I("usb acts as host");
    }

    /* usb acts as peripheral */
    if (reg & OTG_INTSTS_PDEVIF_Msk)
    {
        OTG_CLR_INT_FLAG(OTG_INTSTS_PDEVIF_Msk);
        LOG_I("usb acts as peripheral");
    }

    /* A-device session valid state change */
    if (reg & OTG_INTSTS_AVLDCHGIF_Msk)
    {
        OTG_CLR_INT_FLAG(OTG_INTSTS_AVLDCHGIF_Msk);
        LOG_I("usb a-device session valid state change");
    }

    /* B-device session valid state change */
    if (reg & OTG_INTSTS_BVLDCHGIF_Msk)
    {
        OTG_CLR_INT_FLAG(OTG_INTSTS_BVLDCHGIF_Msk);
        LOG_I("usb b-device session valid state change");
    }

    reg = OTG->INTSTS;
    (void)reg;

    rt_interrupt_leave();
}

static int otg_init(void)
{
    /* Enable USBD and USBH clocks */
    CLK_EnableModuleClock(USBD_MODULE);
    CLK_EnableModuleClock(USBH_MODULE);

    if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
    {
        CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(2));
        CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(2));
    }
    else
    {
        /* Enable HIRC48 clock */
        CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);

        CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC48, CLK_CLKDIV0_USB(1));
        CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_HIRC48, CLK_CLKDIV0_USB(1));
    }

    SYS_ResetModule(USBD_RST);

    /* Enable OTG module clock */
    CLK_EnableModuleClock(OTG_MODULE);

    /* Set OTG as ID dependent role */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk | SYS_USBPHY_USBROLE_ID_DEPH;

    /* Enable OTG and ID detection function */
    OTG_ENABLE_PHY();
    OTG_ENABLE_ID_DETECT();
    NVIC_EnableIRQ(USBOTG_IRQn);

    rt_thread_mdelay(1);

    /* clear interrupt and enable relative interrupts */
    OTG_ENABLE_INT(OTG_INTEN_IDCHGIEN_Msk | OTG_INTEN_HOSTIEN_Msk | OTG_INTEN_PDEVIEN_Msk |
                   OTG_INTEN_BVLDCHGIEN_Msk | OTG_INTEN_AVLDCHGIEN_Msk);

    return (int)RT_EOK;
}
INIT_PREV_EXPORT(otg_init);
#endif /* defined(BSP_USING_OTG) */
