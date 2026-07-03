/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtthread.h"
#include "NuMicro.h"
#include "glue_nuvoton.h"

#include "drv_sys.h"
#include "usbh_core.h"
#include "usb_hc.h"

//#undef LOG_TAG
#define LOG_TAG "drv.cherryusb"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#if defined(RT_CHERRYUSB_HOST)
// To execute msh command to bring up cherryusb stack.
// msh /> usbh_init 0 0x4001A000

#if defined(BSP_USING_HSUSBH)
void EHCI_IRQHandler(void)
{
    rt_interrupt_enter();

    USBH_IRQHandler(0);

    rt_interrupt_leave();
}
#endif

#if defined(BSP_USING_USBH)
/* Not support OHCI. */
#endif

void usb_hc_low_level_init(struct usbh_bus *bus)
{
    void *reg_base = (void*)bus->hcd.reg_base;

#if defined(BSP_USING_HSUSBH)
    if (reg_base == (void*)HSUSBH_BASE)
    {
        /* Enable USBH clock */
        CLK_EnableModuleClock(USBH_MODULE);
        CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_PLL_DIV2, CLK_CLKDIV0_USB(2));

    #if !defined(BSP_USING_HSOTG)
        {
            /* Set USB Host role */
            SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_HSUSBROLE_Msk) | (0x1u << SYS_USBPHY_HSUSBROLE_Pos);
            SYS->USBPHY |= SYS_USBPHY_HSUSBEN_Msk | SYS_USBPHY_SBO_Msk;
            rt_thread_delay(20);
            SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
        }
    #endif

        HSUSBH->USBPCR0 = 0x160;                /* enable PHY 0          */
        HSUSBH->USBPCR1 = 0x520;                /* enable PHY 1          */

        USBH->HcMiscControl |= USBH_HcMiscControl_OCAL_Msk; /* Over-current active low  */
        //USBH->HcMiscControl &= ~USBH_HcMiscControl_OCAL_Msk; /* Over-current active high  */

        /* Enable interrupt */
        NVIC_EnableIRQ(HSUSBH_IRQn);
    }
#endif

#if defined(BSP_USING_USBH)
    if (reg_base == (void*)USBH_BASE)
    {
    /* Not support OHCI. */
    }
#endif
}

void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
    void *reg_base = (void*)bus->hcd.reg_base;

#if defined(BSP_USING_HSUSBH)
    if (reg_base == (void*)HSUSBH_BASE)
    {
        /* Disable interrupt */
        NVIC_DisableIRQ(HSUSBH_IRQn);

        /* Disable USBH clock */
        CLK_DisableModuleClock(USBH_MODULE);

        SYS->USBPHY &= ~SYS_USBPHY_HSUSBACT_Msk;
    }
#endif

#if defined(BSP_USING_USBH)
    if (reg_base == (void*)USBH_BASE)
    {
        /* Disable interrupt */
        NVIC_DisableIRQ(USBH_IRQn);

        /* Disable USBH clock */
        CLK_DisableModuleClock(USBH_MODULE);
    }
#endif
}

uint8_t usbh_get_port_speed(struct usbh_bus *bus, const uint8_t port)
{
    return USB_SPEED_HIGH;
}
#endif

#if defined(RT_CHERRYUSB_DEVICE)
void usb_dc_low_level_init(uint8_t busid)
{
#if !defined(BSP_USING_OTG)
    if (busid==DEF_DC_USBID_FS)
    {
        CLK_EnableModuleClock(USBD_MODULE);
        CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL_DIV2, CLK_CLKDIV0_USB(2));
        SYS_ResetModule(SYS_USBD0RST);

        /* Select USBD */
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk;
    }
#endif

#if !defined(BSP_USING_HSOTG)
    if (busid==DEF_DC_USBID_HS)
    {
        CLK_EnableModuleClock(HSUSBD_MODULE);
        SYS_ResetModule(SYS_HSUSBD0RST);

        /* Set PHY*/
        SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSOTGPHYEN_Msk;
        rt_thread_delay(20);

        SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
     }
#endif
}

void usb_dc_low_level_deinit(uint8_t busid)
{
#if !defined(BSP_USING_OTG) && defined(BSP_USING_USBD)
    if (busid==DEF_DC_USBID_FS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_USBROLE_Msk | SYS_USBPHY_OTGPHYEN_Msk) ;
        CLK_DisableModuleClock(USBD_MODULE);
    }
#endif

#if !defined(BSP_USING_HSOTG) && defined(BSP_USING_HSUSBD)
    if (busid==DEF_DC_USBID_HS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_HSUSBACT_Msk | SYS_USBPHY_HSOTGPHYEN_Msk);
        CLK_DisableModuleClock(HSUSBD_MODULE);
    }
#endif
}
#endif

#if defined(BSP_USING_HSOTG)
/* HSOTG interrupt entry */
void HSOTG_IRQHandler(void)
{
    __IO uint32_t reg = HSOTG->INTSTS;

    /* usb id pin status change */
    if (reg & HSOTG_INTSTS_IDCHGIF_Msk)
    {
        HSOTG_CLR_INT_FLAG(HSOTG_INTSTS_IDCHGIF_Msk);
        LOG_I("hsusb id change");
    }

    /* usb acts as host */
    if (reg & HSOTG_INTSTS_HOSTIF_Msk)
    {
        HSOTG_CLR_INT_FLAG(HSOTG_INTSTS_HOSTIF_Msk);
        LOG_I("hsusb acts as host");
    }

    /* usb acts as peripheral */
    if (reg & HSOTG_INTSTS_PDEVIF_Msk)
    {
        HSOTG_CLR_INT_FLAG(HSOTG_INTSTS_PDEVIF_Msk);
        LOG_I("hsusb acts as peripheral");
    }

    /* A-device session valid state change */
    if (reg & HSOTG_INTSTS_AVLDCHGIF_Msk)
    {
        HSOTG_CLR_INT_FLAG(HSOTG_INTSTS_AVLDCHGIF_Msk);
        LOG_I("hsusb a-device session valid state change");
    }

    /* B-device session valid state change */
    if (reg & HSOTG_INTSTS_BVLDCHGIF_Msk)
    {
        HSOTG_CLR_INT_FLAG(HSOTG_INTSTS_BVLDCHGIF_Msk);
        LOG_I("hsusb b-device session valid state change");
    }

    reg = HSOTG->INTSTS;
}

static int hsotg_init(void)
{
    /* Enable HSUSHD clock */
    CLK_EnableModuleClock(HSUSBD_MODULE);
    SYS_ResetModule(HSUSBD_RST);

    /* Enable USBH clock */
    CLK_EnableModuleClock(USBH_MODULE);
    CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_PLL_DIV2, CLK_CLKDIV0_USB(2));

    /* Enable HSOTG module clock */
    CLK_EnableModuleClock(HSOTG_MODULE);

    SYS->USBPHY = (0x1ul << SYS_USBPHY_HSOTGPHYEN_Pos) |
                  (0x2ul << SYS_USBPHY_HSUSBROLE_Pos) |
                  (0x1ul << SYS_USBPHY_OTGPHYEN_Pos) |
                  (0x2UL << SYS_USBPHY_USBROLE_Pos);

    rt_thread_mdelay(1);

    /* user should keep HSUSB PHY at reset mode at lease 10 us before changing to active mode */
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;//Set HSUSB PHY Active.

    /* Enable OTG and ID detection function */
    HSOTG_ENABLE_PHY();
    HSOTG_ENABLE_ID_DETECT();
    NVIC_EnableIRQ(HSOTG_IRQn);

    /* clear interrupt and enable relative interrupts */
    HSOTG_ENABLE_INT(HSOTG_INTEN_IDCHGIEN_Msk | HSOTG_INTEN_HOSTIEN_Msk | HSOTG_INTEN_PDEVIEN_Msk |
                     HSOTG_INTEN_BVLDCHGIEN_Msk | HSOTG_INTEN_AVLDCHGIEN_Msk);

    return (int)RT_EOK;
}
INIT_PREV_EXPORT(hsotg_init);
#endif /* defined(BSP_USING_HSOTG) */