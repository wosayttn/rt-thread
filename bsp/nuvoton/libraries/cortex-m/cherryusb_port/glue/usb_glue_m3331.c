/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtthread.h"

#include "drv_sys.h"
#include "glue_nuvoton.h"
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

    #if !defined(BSP_USING_HSOTG)
        /* Enable USBH clock */
        CLK_EnableModuleClock(USBH_MODULE);

        /* Set USB Host role */
        SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_HSUSBROLE_Msk) | (0x1u << SYS_USBPHY_HSUSBROLE_Pos);
        SYS->USBPHY |= SYS_USBPHY_HSUSBEN_Msk | SYS_USBPHY_SBO_Msk;
        rt_thread_mdelay(1);
        SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

        HSUSBH->USBPCR0 = 0x160;         /* EHCI: CLKSEL=10b, refclk=1b, SUSPEND=1b */
        //HSUSBH->USBPCR1 = 0x520;         /* OHCI: CLKSEL=10b, NEGTX=1b, SUSPEND=1b */
    #endif

    }
#endif

}

void usb_hc_low_level2_init(struct usbh_bus *bus)
{
#if defined(BSP_USING_HSUSBH)
    if (bus->hcd.reg_base == HSUSBH_BASE)
    {
        HSUSBH->UPSCR[0] = HSUSBH_UPSCR_PP_Msk;      /* enable port 1 port power               */

        USBH->HcMiscControl |= USBH_HcMiscControl_OCAL_Msk; /* Over-current active low  */
        //USBH->HcMiscControl &= ~USBH_HcMiscControl_OCAL_Msk; /* Over-current active high  */

        /* Enable interrupt */
        NVIC_EnableIRQ(HSUSBH_EHCI_IRQn);
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
        NVIC_DisableIRQ(HSUSBH_EHCI_IRQn);

        /* Disable USBH clock */
        CLK_DisableModuleClock(USBH_MODULE);

        SYS->USBPHY &= ~SYS_USBPHY_HSUSBACT_Msk;
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

#if !defined(BSP_USING_HSOTG)
    if (busid==DEF_DC_USBID_HS)
    {
        CLK_EnableModuleClock(HSUSBD_MODULE);
        SYS_ResetModule(HSUSBD_RST);

        /* Set PHY*/
        SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSUSBEN_Msk;
        rt_thread_delay(20);

        SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
     }
#endif
}

void usb_dc_low_level_deinit(uint8_t busid)
{

#if !defined(BSP_USING_HSOTG) && defined(BSP_USING_HSUSBD)
    if (busid==DEF_DC_USBID_HS)
    {
        SYS->USBPHY &= ~(SYS_USBPHY_HSUSBACT_Msk | SYS_USBPHY_HSUSBEN_Msk);
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

    /* Set HSOTG as ID dependent role */
    SYS->USBPHY = SYS_USBPHY_HSUSBEN_Msk | (0x2 << SYS_USBPHY_HSUSBROLE_Pos) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk | (0x2 << SYS_USBPHY_USBROLE_Pos);

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

#if defined(BOARD_USING_USB_SWOTG) && defined(RT_USING_ADC) && defined(RT_USING_PIN)

#define DEF_ADC_DEV_NAME            ("eadc0")   // EADC device name
#define DEF_ADC_CC1_CHANNEL         (6)         // CC1
#define DEF_ADC_CC2_CHANNEL         (7)         // CC2
#define DEF_ADC_SAMPLING_DURATION   (1000)      // in ticks

/* defined the PA7 PIN */
#define DEF_MOS_G_S_PIN             NU_GET_PININDEX(NU_PA, 7)

#define THREAD_PRIORITY   5
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE  5

typedef enum
{
    evUSB_ROLE_DEVICE = 0,
    evUSB_ROLE_HOST,
    evUSB_ROLE_NONE
} E_USB_ROLE;

static const uint16_t s_swotg_ccx_threshold[evUSB_ROLE_NONE][2] =
{
    /* Min / Max CC1+CC2  */
    {3200,   3800},  /* evUSB_ROLE_DEVICE */
    {400,    600},   /* evUSB_ROLE_HOST */
};

static int isConnectedUSBRole(const rt_int16_t *cc_sum)
{
    /* Determine the USB role based on CC1+CC2 voltage. */
    for (E_USB_ROLE role = evUSB_ROLE_DEVICE; role < evUSB_ROLE_NONE; role++)
    {
        /* Check if CC1+CC2 voltage is within the threshold range. */
        if ((cc_sum[role] >= s_swotg_ccx_threshold[role][0]) &&
                (cc_sum[role] <= s_swotg_ccx_threshold[role][1]))
        {
            /* Found the connected USB role */
            return role;
        }
    }
    return evUSB_ROLE_NONE;
}

static void swotg_worker(void *parameter)
{
    static E_USB_ROLE last_role = evUSB_ROLE_NONE;
    static rt_adc_device_t s_swotg_adc_dev = RT_NULL;

    rt_err_t err = 0;

    /* Find EADC device */
    s_swotg_adc_dev = (rt_adc_device_t)rt_device_find(DEF_ADC_DEV_NAME);
    if (s_swotg_adc_dev == RT_NULL)
    {
        LOG_E("Failed to find EADC device for USB OTG Type-C detection!\n");
        return;
    }
    err = rt_adc_enable(s_swotg_adc_dev, DEF_ADC_CC1_CHANNEL);
    err |= rt_adc_enable(s_swotg_adc_dev, DEF_ADC_CC2_CHANNEL);
    if (err != RT_EOK)
    {
        LOG_E("Failed to enable EADC channel for USB OTG Type-C detection!\n");
        goto fail_init;
    }

    /* set LEDR pin mode to output. */
    rt_pin_mode(DEF_MOS_G_S_PIN, PIN_MODE_OUTPUT);

    /* Sample CC1 and CC2 voltages periodically. */
    while (1)
    {
        rt_int16_t cc_sum[2] = {0};

        /* Sample CC1 and CC2 voltages under both USB roles. */
        for (E_USB_ROLE role = evUSB_ROLE_DEVICE; role < evUSB_ROLE_NONE; role++)
        {
            rt_int16_t cc1_mv, cc2_mv;

            /* Set MOS_G_S pin to select USB role */
            rt_pin_write(DEF_MOS_G_S_PIN, role);
            rt_thread_mdelay(100);

            /* Read CC1 and CC2 voltages */
            cc1_mv = rt_adc_voltage((rt_adc_device_t)s_swotg_adc_dev, DEF_ADC_CC1_CHANNEL);  // CC1
            cc2_mv = rt_adc_voltage((rt_adc_device_t)s_swotg_adc_dev, DEF_ADC_CC2_CHANNEL);  // CC2

            /* Sum CC1 and CC2 voltages */
            cc_sum[role] = cc1_mv + cc2_mv;

            //LOG_D("%d: cc1:%04d + cc2:%04d = %04d(mv)\n", role, cc1_mv, cc2_mv, cc_sum[role]);
        }
        E_USB_ROLE role = isConnectedUSBRole(cc_sum);
        if (role != last_role)
        {
            //LOG_D("L: %d(mv), H:%d(mv)\n", cc_sum[evUSB_ROLE_DEVICE], cc_sum[evUSB_ROLE_HOST]);

            switch (role)
            {
            case evUSB_ROLE_DEVICE:
                LOG_D("=> Connected to Device. (Will switch USBPHY role to Host.)\n");

                SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_HSUSBROLE_Msk) | (0x1u << SYS_USBPHY_HSUSBROLE_Pos);  // Select HSUSBH
                SYS->USBPHY |= SYS_USBPHY_HSUSBEN_Msk | SYS_USBPHY_SBO_Msk;
                rt_thread_mdelay(1);
                SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

                break;

            case evUSB_ROLE_HOST:
                LOG_D("=> Connected to Host. (Will switch USBPHY role to Device.)\n");

                SYS->USBPHY  = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk)) | (0x0u << SYS_USBPHY_HSUSBROLE_Pos);  // Select HSUSBD
                SYS->USBPHY &= ~SYS_USBPHY_HSUSBACT_Msk;
                SYS->USBPHY |= (SYS_USBPHY_HSUSBEN_Msk);
                rt_thread_mdelay(1);
                SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

                break;

            case evUSB_ROLE_NONE:
            default:
                /* TODO: Add USB OTG de-initialization code here. */
                LOG_D("=> No connected.\n");

                SYS->USBPHY &= (~SYS_USBPHY_HSUSBEN_Msk);

                break;
            }

            last_role = role;
        }

        rt_thread_mdelay(DEF_ADC_SAMPLING_DURATION);
    }

fail_init:

    /* Disable EADC channels */
    if (s_swotg_adc_dev)
    {
        rt_adc_disable(s_swotg_adc_dev, DEF_ADC_CC1_CHANNEL);
        rt_adc_disable(s_swotg_adc_dev, DEF_ADC_CC2_CHANNEL);
    }

}

int swotg_init(void)
{
#define DEF_THREAD_NAME "SWOTG"

    rt_thread_t swotg_thread = rt_thread_find(DEF_THREAD_NAME);
    if (swotg_thread == RT_NULL)
    {
        swotg_thread = rt_thread_create(DEF_THREAD_NAME,
                                        swotg_worker,
                                        RT_NULL,
                                        THREAD_STACK_SIZE,
                                        THREAD_PRIORITY,
                                        THREAD_TIMESLICE);

        if (swotg_thread != RT_NULL)
            rt_thread_startup(swotg_thread);
    }

    return 0;
}

//MSH_CMD_EXPORT(swotg_init, enable ccx polling);
//INIT_APP_EXPORT(swotg_init);

#endif
