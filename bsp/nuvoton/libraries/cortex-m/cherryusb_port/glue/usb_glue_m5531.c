/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtconfig.h"
#include "NuMicro.h"
#include "drv_sys.h"
#include "glue_nuvoton.h"
#include "rthw.h"
#include "rtthread.h"
#include "usbh_core.h"
#include "usb_hc.h"
#include "usb_dcache.h"

#undef LOG_TAG
#define LOG_TAG "drv.cherryusb"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#if defined(CONFIG_USB_DCACHE_ENABLE)
void usb_dcache_clean(uintptr_t addr, size_t size)
{
    SCB_CleanDCache_by_Addr((void *)addr, (int32_t)size);
}

void usb_dcache_invalidate(uintptr_t addr, size_t size)
{
    SCB_InvalidateDCache_by_Addr((void *)addr, (int32_t)size);
}

void usb_dcache_flush(uintptr_t addr, size_t size)
{
    SCB_CleanInvalidateDCache_by_Addr((void *)addr, (int32_t)size);
}
#endif

#if defined(RT_CHERRYUSB_HOST)
// To execute msh command to bring up cherryusb stack.
// msh /> usbh_init 0 0x4001A000

#if defined(BSP_USING_HSUSBH)
void HSUSBH_IRQHandler(void)
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
        CLK_EnableModuleClock(USBH0_MODULE);
        CLK_SetModuleClock(USBH0_MODULE, CLK_USBSEL_USBSEL_APLL1_DIV2, CLK_USBDIV_USBDIV(1));
        CLK_EnableModuleClock(HSUSBH0_MODULE);

    #if !defined(BSP_USING_HSOTG)
        {
            rt_base_t level;

            CLK_EnableModuleClock(HSOTG0_MODULE);
            HSOTG_SET_PHY_REF_CLK(HSOTG_PHYCTL_FSEL_24_0M);

            /* Set USB Host role atomically to avoid racing with FS USBD init */
            level = rt_hw_interrupt_disable();
            SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) |
                          (0x1u << SYS_USBPHY_HSUSBROLE_Pos) | SYS_USBPHY_HSOTGPHYEN_Msk;
            rt_hw_interrupt_enable(level);

            rt_thread_delay(20);

            level = rt_hw_interrupt_disable();
            SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
            rt_hw_interrupt_enable(level);
        }
    #endif

        USBH0->HcMiscControl |= USBH_HcMiscControl_OCAL_Msk; /* Over-current active low */

        /* Enable interrupt */
        NVIC_EnableIRQ(HSUSBH_IRQn);
    }
#endif

#if defined(BSP_USING_USBH)
    if (reg_base == (void*)USBH0_BASE)
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
        CLK_DisableModuleClock(HSUSBH0_MODULE);
        CLK_DisableModuleClock(USBH0_MODULE);

        SYS->USBPHY &= ~SYS_USBPHY_HSUSBACT_Msk;
    }
#endif

#if defined(BSP_USING_USBH)
    if (reg_base == (void*)USBH0_BASE)
    {
        /* Disable interrupt */
        NVIC_DisableIRQ(USBH0_IRQn);

        /* Disable USBH clock */
        CLK_DisableModuleClock(USBH0_MODULE);
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
#if !defined(BSP_USING_OTG) && defined(BSP_USING_USBD)
    if (busid==DEF_DC_USBID_FS)
    {
        rt_base_t level;

        CLK_EnableModuleClock(USBD0_MODULE);
        CLK_SetModuleClock(USBD0_MODULE, CLK_USBSEL_USBSEL_APLL1_DIV2, CLK_USBDIV_USBDIV(1));
        SYS_ResetModule(SYS_USBD0RST);

        /* Select USBD atomically to avoid racing with HSUSBH init */
        level = rt_hw_interrupt_disable();
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk;
        rt_hw_interrupt_enable(level);
    }
#endif

#if !defined(BSP_USING_HSOTG) && defined(BSP_USING_HSUSBD)
    if (busid==DEF_DC_USBID_HS)
    {
        rt_base_t level;

        CLK_EnableModuleClock(HSOTG0_MODULE);
        HSOTG_SET_PHY_REF_CLK(HSOTG_PHYCTL_FSEL_24_0M);
        CLK_EnableModuleClock(HSUSBD0_MODULE);
        SYS_ResetModule(SYS_HSUSBD0RST);

        /* Set PHY atomically */
        level = rt_hw_interrupt_disable();
        SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSOTGPHYEN_Msk;
        rt_hw_interrupt_enable(level);

        rt_thread_delay(20);

        level = rt_hw_interrupt_disable();
        SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
        rt_hw_interrupt_enable(level);
     }
#endif
}

void usb_dc_low_level_deinit(uint8_t busid)
{
#if !defined(BSP_USING_OTG) && defined(BSP_USING_USBD)
    if (busid==DEF_DC_USBID_FS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_USBROLE_Msk | SYS_USBPHY_OTGPHYEN_Msk) ;
        CLK_DisableModuleClock(USBD0_MODULE);
    }
#endif

#if !defined(BSP_USING_HSOTG) && defined(BSP_USING_HSUSBD)
    if (busid==DEF_DC_USBID_HS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_HSUSBACT_Msk | SYS_USBPHY_HSOTGPHYEN_Msk);
        CLK_DisableModuleClock(HSUSBD0_MODULE);
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
    /* Enable HSUSBD clock */
    CLK_EnableModuleClock(HSUSBD0_MODULE);
    SYS_ResetModule(SYS_HSUSBD0RST);

    /* Enable USBH clock */
    CLK_EnableModuleClock(USBH0_MODULE);
    CLK_SetModuleClock(USBH0_MODULE, CLK_USBSEL_USBSEL_APLL1_DIV2, CLK_USBDIV_USBDIV(1));
    CLK_EnableModuleClock(HSUSBH0_MODULE);

    /* Enable HSOTG module clock */
    CLK_EnableModuleClock(HSOTG0_MODULE);

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