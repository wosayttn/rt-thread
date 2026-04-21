/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include <rtthread.h>

#include <rtdevice.h>
#include <rthw.h>
#include "drv_sys.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.can"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#define RX_MSG_ID_INDEX        16
#define IS_CAN_STDID(STDID)   ((STDID) <= 0x7FFU)
#define IS_CAN_EXTID(EXTID)   ((EXTID) <= 0x1FFFFFFFU)
#define IS_CAN_DLC(DLC)       ((DLC) <= 8U)

/* Default config for can_configure structure */
#define NU_CAN_CONFIG_DEFAULT                  \
{                                              \
    CAN1MBaud,           /* 1M bits/s       */ \
    RT_CANMSG_BOX_SZ,    /* message box max size */ \
    RT_CANSND_BOX_NUM,   /* message box number   */ \
    RT_CAN_MODE_NORMAL,  /* Normal mode     */ \
    0,                   /* privmode        */ \
    0,                   /* reserved        */ \
    100,                 /* Timeout Tick    */ \
}

#define DEFINE_NU_CAN(_idx)         \
    {                               \
        .name = "can" #_idx,        \
        .base = CAN##_idx,          \
        .rstidx = CAN##_idx##_RST,  \
        .irqn = CAN##_idx##_IRQn,   \
    }

#define DEFINE_CAN_IRQ_HANDLER(_idx)        \
void CAN##_idx##_IRQHandler(void)           \
{                                           \
    rt_interrupt_enter();                   \
    nu_can_isr(&nu_can_arr[CAN##_idx##_IDX]); \
    rt_interrupt_leave();                   \
}

/* Types / Structures ---------------------------------------------------------*/
enum
{
    CAN_START = -1,
#if defined(BSP_USING_CAN0)
    CAN0_IDX,
#endif
#if defined(BSP_USING_CAN1)
    CAN1_IDX,
#endif
#if defined(BSP_USING_CAN2)
    CAN2_IDX,
#endif
    CAN_CNT
};

struct nu_can
{
    struct rt_can_device dev;
    char *name;
    CAN_T *base;
    IRQn_Type irqn;
    uint32_t rstidx;
    uint32_t int_flag;
};
typedef struct nu_can *nu_can_t;

/* Static Function Prototypes ------------------------------------------------*/
static rt_err_t nu_can_configure(struct rt_can_device *can, struct can_configure *cfg);
static rt_err_t nu_can_control(struct rt_can_device *can, int cmd, void *arg);
static int nu_can_sendmsg(struct rt_can_device *can, const void *buf, rt_uint32_t boxno);
static int nu_can_recvmsg(struct rt_can_device *can, void *buf, rt_uint32_t boxno);
static void nu_can_isr(nu_can_t can);

/* Static Variables ----------------------------------------------------------*/
static struct nu_can nu_can_arr[] =
{
#if defined(BSP_USING_CAN0)
    DEFINE_NU_CAN(0),
#endif
#if defined(BSP_USING_CAN1)
    DEFINE_NU_CAN(1),
#endif
#if defined(BSP_USING_CAN2)
    DEFINE_NU_CAN(2),
#endif
};

static const struct rt_can_ops nu_can_ops =
{
    .configure = nu_can_configure,
    .control = nu_can_control,
    .sendmsg = nu_can_sendmsg,
    .recvmsg = nu_can_recvmsg,
};

static const struct can_configure nu_can_default_config = NU_CAN_CONFIG_DEFAULT;

/* Functions Implementation --------------------------------------------------*/
#if defined(BSP_USING_CAN0)
    DEFINE_CAN_IRQ_HANDLER(0)
#endif

#if defined(BSP_USING_CAN1)
    DEFINE_CAN_IRQ_HANDLER(1)
#endif

#if defined(BSP_USING_CAN2)
    DEFINE_CAN_IRQ_HANDLER(2)
#endif

static void nu_can_isr(nu_can_t psNuCAN)
{
    /* Get base address of CAN register */
    CAN_T *base = psNuCAN->base;

    /* Get interrupt event */
    uint32_t u32IIDRstatus = CAN_GET_INT_PENDING_STATUS(base) & CAN_IIDR_INTID_Msk;

    /* Check Status Interrupt Flag (Error status Int and Status change Int) */
    if (u32IIDRstatus == 0x00008000)
    {
        /* Status Change interrupt */
        if (base->STATUS & CAN_STATUS_TXOK_Msk)
        {
            base->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status */
#ifndef RT_CAN_USING_HDR
            if (psNuCAN->int_flag & RT_DEVICE_FLAG_INT_TX)
            {
                rt_hw_can_isr(&psNuCAN->dev, RT_CAN_EVENT_TX_DONE);
            }
#endif
        }

        if (base->STATUS & CAN_STATUS_RXOK_Msk)
        {
            base->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status */
#ifndef RT_CAN_USING_HDR
            if (psNuCAN->int_flag & RT_DEVICE_FLAG_INT_RX)
            {
                rt_hw_can_isr(&psNuCAN->dev, RT_CAN_EVENT_RX_IND);
            }
#endif
        }

        /* Error Status interrupt */
        if (base->STATUS & CAN_STATUS_EWARN_Msk)
        {
            LOG_W("[%s] EWARN INT", psNuCAN->name);
        }

        if (base->STATUS & CAN_STATUS_BOFF_Msk)
        {
            LOG_W("[%s] BUSOFF INT", psNuCAN->name);

            /* To release busoff pin */
            CAN_EnterInitMode(base, CAN_CON_INIT_Msk | CAN_CON_CCE_Msk);
            CAN_LeaveInitMode(base);
        }

        if (base->STATUS & CAN_STATUS_LEC_Msk)
        {
            LOG_D("[%s] Last Error Code %03x", psNuCAN->name, base->STATUS & CAN_STATUS_LEC_Msk);
        }
    }
#ifdef RT_CAN_USING_HDR
    /* IntId: 0x0001-0x0020, Number of Message Object which caused the interrupt. */
    else if (u32IIDRstatus > 0 && u32IIDRstatus <= 32)
    {
        if ((psNuCAN->int_flag & RT_DEVICE_FLAG_INT_TX) &&
                (u32IIDRstatus <= RX_MSG_ID_INDEX))
        {
            /* Message RAM 0~RX_MSG_ID_INDEX for CAN Tx using */
            rt_hw_can_isr(&psNuCAN->dev, RT_CAN_EVENT_TX_DONE);
        }
        else if (psNuCAN->int_flag & RT_DEVICE_FLAG_INT_RX)
        {
            /* Message RAM RX_MSG_ID_INDEX~31 for CAN Rx using */
            rt_hw_can_isr(&psNuCAN->dev, (RT_CAN_EVENT_RX_IND | ((u32IIDRstatus - 1) << 8)));
        }
        CAN_CLR_INT_PENDING_BIT(base, (u32IIDRstatus - 1));     /* Clear Interrupt Pending */
    }
#endif
}

static void nu_can_ie(nu_can_t psNuCAN)
{
    uint32_t u32CanIE = CAN_CON_IE_Msk;

    if (psNuCAN->int_flag & (RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX))
    {
        u32CanIE |= CAN_CON_SIE_Msk;
    }
    else
    {
        u32CanIE &= ~CAN_CON_SIE_Msk;
    }

    if (psNuCAN->int_flag & RT_DEVICE_CAN_INT_ERR)
    {
        u32CanIE |= CAN_CON_EIE_Msk;
    }
    else
    {
        u32CanIE &= ~CAN_CON_EIE_Msk;
    }

    if (u32CanIE & (CAN_CON_SIE_Msk | CAN_CON_EIE_Msk))
    {
        CAN_EnableInt(psNuCAN->base, u32CanIE);
        NVIC_EnableIRQ(psNuCAN->irqn);
    }
    else
    {
        u32CanIE |= (CAN_CON_IE_Msk | CAN_CON_SIE_Msk);
        CAN_DisableInt(psNuCAN->base, u32CanIE);
        NVIC_DisableIRQ(psNuCAN->irqn);
    }
}

static rt_err_t nu_can_configure(struct rt_can_device *can, struct can_configure *cfg)
{
    nu_can_t psNuCAN = (nu_can_t)can;
    uint32_t u32CANMode;

    RT_ASSERT(can);
    RT_ASSERT(cfg);

    CAN_T *base = psNuCAN->base;

    /* Reset this module */
    SYS_ResetModule(psNuCAN->rstidx);

    u32CANMode = (cfg->mode == RT_CAN_MODE_NORMAL) ? CAN_NORMAL_MODE : CAN_BASIC_MODE;

    /* Set the CAN Bit Rate and Operating mode */
    if (CAN_Open(base, cfg->baud_rate, u32CANMode) != cfg->baud_rate)
        goto exit_nu_can_configure;

    switch (cfg->mode)
    {
    case RT_CAN_MODE_NORMAL:
#ifdef RT_CAN_USING_HDR
        CAN_LeaveTestMode(base);
#else
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk);
#endif
        break;
    case RT_CAN_MODE_LISTEN:
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk | CAN_TEST_SILENT_Msk);
        break;
    case RT_CAN_MODE_LOOPBACK:
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk | CAN_TEST_LBACK_Msk);
        break;
    case RT_CAN_MODE_LOOPBACKANLISTEN:
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk | CAN_TEST_SILENT_Msk | CAN_TEST_LBACK_Msk);
        break;
    default:
        LOG_E("Unsupported Operating mode");
        goto exit_nu_can_configure;
    }

    nu_can_ie(psNuCAN);

    return RT_EOK;

exit_nu_can_configure:

    CAN_Close(base);

    return -(RT_ERROR);
}

static rt_err_t nu_can_control(struct rt_can_device *can, int cmd, void *arg)
{
    rt_uint32_t argval = (rt_uint32_t)arg;
    nu_can_t psNuCAN = (nu_can_t)can;

    RT_ASSERT(can);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_SET_INT:
        psNuCAN->int_flag |= argval;
        nu_can_ie(psNuCAN);
        break;

    case RT_DEVICE_CTRL_CLR_INT:
        psNuCAN->int_flag &= ~argval;
        nu_can_ie(psNuCAN);
        break;

    case RT_CAN_CMD_SET_FILTER:
    {
        struct rt_can_filter_config *filter_cfg = (struct rt_can_filter_config *)arg;

        for (int i = 0; i < filter_cfg->count; i++)
        {
            if (filter_cfg->items[i].mode == 1)
            {
                if (CAN_SetRxMsgObjAndMsk(psNuCAN->base, MSG(filter_cfg->items[i].hdr_bank + RX_MSG_ID_INDEX), filter_cfg->items[i].ide, filter_cfg->items[i].id, filter_cfg->items[i].mask, FALSE) == FALSE)
                {
                    return -(RT_ERROR);
                }
            }
            else
            {
                if (CAN_SetRxMsgAndMsk(psNuCAN->base, MSG(filter_cfg->items[i].hdr_bank + RX_MSG_ID_INDEX), filter_cfg->items[i].ide, filter_cfg->items[i].id, filter_cfg->items[i].mask) == FALSE)
                {
                    return -(RT_ERROR);
                }
            }
        }
    }
    break;

    case RT_CAN_CMD_SET_MODE:
        if ((argval == RT_CAN_MODE_NORMAL) ||
                (argval == RT_CAN_MODE_LISTEN) ||
                (argval == RT_CAN_MODE_LOOPBACK) ||
                (argval == RT_CAN_MODE_LOOPBACKANLISTEN))
        {
            if (argval != can->config.mode)
            {
                can->config.mode = argval;
                return nu_can_configure(can, &can->config);
            }
        }
        else
        {
            return -(RT_ERROR);
        }
        break;

    case RT_CAN_CMD_SET_BAUD:
    {
        if ((argval == CAN1MBaud) ||
                (argval == CAN800kBaud) ||
                (argval == CAN500kBaud) ||
                (argval == CAN250kBaud) ||
                (argval == CAN125kBaud) ||
                (argval == CAN100kBaud) ||
                (argval == CAN50kBaud) ||
                (argval == CAN20kBaud) ||
                (argval == CAN10kBaud))
        {
            if (argval != can->config.baud_rate)
            {
                can->config.baud_rate = argval;
                return nu_can_configure(can, &can->config);
            }
        }
        else
        {
            return -(RT_ERROR);
        }
    }
    break;

    case RT_CAN_CMD_SET_PRIV:
        if (argval != RT_CAN_MODE_PRIV &&
                argval != RT_CAN_MODE_NOPRIV)
        {
            return -(RT_ERROR);
        }
        if (argval != can->config.privmode)
        {
            can->config.privmode = argval;
            return nu_can_configure(can, &can->config);
        }
        break;

    case RT_CAN_CMD_GET_STATUS:
    {
        rt_uint32_t errtype = psNuCAN->base->ERR;

        RT_ASSERT(arg);

        can->status.rcverrcnt = (errtype >> 8);
        can->status.snderrcnt = (errtype & 0xFF);
        can->status.lasterrtype = CAN_GET_INT_STATUS(psNuCAN->base) & 0x8000;
        can->status.errcode = CAN_GET_INT_STATUS(psNuCAN->base) & 0x07;

        rt_memcpy(arg, &can->status, sizeof(struct rt_can_status));
    }
    break;

    default:
        return -(RT_EINVAL);
    }

    return RT_EOK;
}

static int nu_can_sendmsg(struct rt_can_device *can, const void *buf, rt_uint32_t boxno)
{
    STR_CANMSG_T tMsg;
    struct rt_can_msg *pmsg;
    nu_can_t psNuCAN = (nu_can_t)can;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pmsg = (struct rt_can_msg *)buf;

    if (pmsg->ide == RT_CAN_STDID && IS_CAN_STDID(pmsg->id))
    {
        tMsg.IdType = CAN_STD_ID;
        tMsg.Id  = pmsg->id;
    }
    else if (pmsg->ide == RT_CAN_EXTID && IS_CAN_EXTID(pmsg->id))
    {
        tMsg.IdType = CAN_EXT_ID;
        tMsg.Id = pmsg->id;
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    if (pmsg->rtr == RT_CAN_DTR)
    {
        tMsg.FrameType = CAN_DATA_FRAME;
    }
    else if (pmsg->rtr == RT_CAN_RTR)
    {
        tMsg.FrameType = CAN_REMOTE_FRAME;
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    if (IS_CAN_DLC(pmsg->len))
    {
        tMsg.DLC = pmsg->len;
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    if (pmsg->data && pmsg->len)
    {
        rt_memcpy(&tMsg.Data[0], pmsg->data, pmsg->len);
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    if (CAN_Transmit(psNuCAN->base, MSG(boxno), &tMsg) == FALSE)
    {
        goto exit_nu_can_sendmsg;
    }

    return RT_EOK;

exit_nu_can_sendmsg:

    return -(RT_ERROR);
}

static int nu_can_recvmsg(struct rt_can_device *can, void *buf, rt_uint32_t boxno)
{
    STR_CANMSG_T tMsg;
    struct rt_can_msg *pmsg;
    nu_can_t psNuCAN = (nu_can_t)can;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pmsg = (struct rt_can_msg *)buf;

    if (CAN_Receive(psNuCAN->base, boxno, &tMsg) == FALSE)
    {
        LOG_D("No available RX Msg.");
        return -(RT_ERROR);
    }

#ifdef RT_CAN_USING_HDR
    pmsg->hdr_index = boxno - RX_MSG_ID_INDEX;
    can->hdr[pmsg->hdr_index].connected = 1;
#endif

    pmsg->ide = (tMsg.IdType == CAN_STD_ID) ? RT_CAN_STDID : RT_CAN_EXTID;
    pmsg->rtr = (tMsg.FrameType == CAN_DATA_FRAME) ? RT_CAN_DTR : RT_CAN_RTR;
    pmsg->id  = tMsg.Id;
    pmsg->len = tMsg.DLC;

    if (pmsg->data && pmsg->len)
        rt_memcpy(pmsg->data, &tMsg.Data[0], pmsg->len);

    return RT_EOK;
}

static int rt_hw_can_init(void)
{
    int i;
    rt_err_t ret = RT_EOK;

    for (i = (CAN_START + 1); i < CAN_CNT; i++)
    {
        nu_can_arr[i].dev.config = nu_can_default_config;

#ifdef RT_CAN_USING_HDR
        nu_can_arr[i].dev.config.maxhdr = RT_CANMSG_BOX_SZ;
#endif
        ret = rt_hw_can_register(&nu_can_arr[i].dev, nu_can_arr[i].name, &nu_can_ops, NULL);
        RT_ASSERT(ret == RT_EOK);
    }

    return (int)ret;
}
INIT_DEVICE_EXPORT(rt_hw_can_init);