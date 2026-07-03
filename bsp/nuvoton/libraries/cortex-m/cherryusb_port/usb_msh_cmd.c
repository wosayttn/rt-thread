/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /* Includes ------------------------------------------------------------------*/
#include "msh.h"
#include "drv_sys.h"
#include "glue_nuvoton.h"

/* Defines / Macros ----------------------------------------------------------*/
#define LOG_TAG "cherryusb.msh"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

static void cherryusb_init(void)
{
    char usbh_init_cmd[32];

#if defined(RT_CHERRYUSB_HOST)
    LOG_I("CherryUSB Host stack init.");
    rt_snprintf(usbh_init_cmd, sizeof(usbh_init_cmd), "usbh_init 0 0x%08X", HSUSBH_BASE);
    msh_exec(usbh_init_cmd, sizeof(usbh_init_cmd) - 1);
#endif

#if defined(RT_CHERRYUSB_DEVICE) && defined(RT_CHERRYUSB_DEVICE_TEMPLATE_HID_MOUSE)
    void hid_mouse_init(uint8_t busid, uintptr_t reg_base);

#if defined(BSP_USING_USBD)
    LOG_I("CherryUSB Device FS stack HID Mouse init.");
    hid_mouse_init(DEF_DC_USBID_FS, USBD_BASE);
#endif

#if defined(BSP_USING_HSUSBD)
    LOG_I("CherryUSB Device HS stack HID Mouse init.");
    hid_mouse_init(DEF_DC_USBID_HS, HSUSBD_BASE);
#endif

#endif
}
MSH_CMD_EXPORT(cherryusb_init, start cherryusb stack);

static void cherryusb_test(void)
{
#if defined(RT_CHERRYUSB_DEVICE) && defined(RT_CHERRYUSB_DEVICE_TEMPLATE_HID_MOUSE)
    void hid_mouse_test(uint8_t busid);

#if defined(BSP_USING_USBD) 
    LOG_I("CherryUSB Device FS stack HID Mouse TEST");
    hid_mouse_test(DEF_DC_USBID_FS);
#endif

#if defined(BSP_USING_HSUSBD)
    LOG_I("CherryUSB Device HS stack HID Mouse TEST");
    hid_mouse_test(DEF_DC_USBID_HS);
#endif

#endif
}
MSH_CMD_EXPORT(cherryusb_test, test cherryusb device stack);

#if defined(BSP_USING_HSOTG)
/* Check current usb role */
static void hsusb_role(void)
{
    uint32_t status = (HSOTG->STATUS) & (HSOTG_STATUS_ASHOST_Msk | HSOTG_STATUS_ASPERI_Msk | HSOTG_STATUS_IDSTS_Msk);

    if (status == (HSOTG_STATUS_IDSTS_Msk | HSOTG_STATUS_ASPERI_Msk))
    {
        LOG_I("hsusb frame acts as peripheral");
    }
    else if (status == HSOTG_STATUS_ASHOST_Msk)
    {
        LOG_I("hsusb frame acts as host");
    }
    else
    {
        LOG_I("hsusb frame is unknown state: 0x%x", status);
    }

    return;
}
MSH_CMD_EXPORT(hsusb_role, check hsusb role);
#endif