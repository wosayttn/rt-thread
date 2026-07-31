/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "drv_sys.h"

#include "lwipopts.h"
#include <netif/ethernetif.h>
#include <netif/etharp.h>
#include <lwip/sys.h>
#include <lwip/icmp.h>
#include <lwip/pbuf.h>

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.emac"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

/* EMAC instance index enumeration */
enum
{
    EMAC_START = -1,
#if defined(BSP_USING_EMAC0)
    EMAC0_IDX,
#endif
#if defined(BSP_USING_EMAC1)
    EMAC1_IDX,
#endif
    EMAC_CNT
};

/* Private typedef --------------------------------------------------------------*/

/**
 * @brief EMAC device instance structure.
 *        Inherits from eth_device (RT-Thread ethernet framework).
 */
struct nu_emac
{
    struct eth_device   eth;              /* RT-Thread ethernet device, must be first */
    char               *name;             /* Device name (e.g. "e0", "e1") */
    EMAC_MEMMGR_T       memmgr;           /* DMA descriptor & frame buffer manager */
    IRQn_Type           irqn_tx;          /* TX interrupt number */
    IRQn_Type           irqn_rx;          /* RX interrupt number */
    rt_timer_t          link_timer;       /* Periodic timer for PHY link monitoring */
    rt_uint8_t          mac_addr[8];      /* MAC address (6 bytes used) */
    uint32_t            link_status_last; /* Last known PHY link status */
    rt_bool_t           phy_inited;       /* PHY initialization flag */
};
typedef struct nu_emac *nu_emac_t;

/* Private functions ------------------------------------------------------------*/
#if LWIP_IPV4 && LWIP_IGMP
    static err_t nu_igmp_mac_filter(struct netif *netif, const ip4_addr_t *ip4_addr, enum netif_mac_filter_action action);
#endif
static void nu_emac_halt(nu_emac_t);
static void nu_emac_reinit(nu_emac_t);
static void nu_emac_link_monitor(void *param);
static rt_err_t nu_emac_init(rt_device_t dev);
static rt_err_t nu_emac_open(rt_device_t dev, rt_uint16_t oflag);
static rt_err_t nu_emac_close(rt_device_t dev);
static rt_ssize_t nu_emac_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
static rt_ssize_t nu_emac_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
static rt_err_t nu_emac_control(rt_device_t dev, int cmd, void *args);
static rt_err_t nu_emac_tx(rt_device_t dev, struct pbuf *p);
static struct pbuf *nu_emac_rx(rt_device_t dev);
static int rt_hw_emac_init(void);
static void nu_emac_tx_isr(void *param);
static void nu_emac_rx_isr(void *param);

/* Private variables ------------------------------------------------------------*/
static struct nu_emac nu_emac_arr[EMAC_CNT] =
{
#if defined(BSP_USING_EMAC0)
    {
        .name            =  "e0",
        .memmgr.psEmac   = (EMAC_T *)EMAC0_BASE,
        .irqn_tx         =  EMAC0_TX_IRQn,
        .irqn_rx         =  EMAC0_RX_IRQn,
        .mac_addr        = {0x82, 0x06, 0x21, 0x94, 0x53, 0x01},
    },
#endif
#if defined(BSP_USING_EMAC1)
    {
        .name            =  "e1",
        .memmgr.psEmac   = (EMAC_T *)EMAC1_BASE,
        .irqn_tx         =  EMAC1_TX_IRQn,
        .irqn_rx         =  EMAC1_RX_IRQn,
        .mac_addr        = {0x82, 0x06, 0x21, 0x94, 0x53, 0x02},
    },
#endif
};

/* IRQ Handlers ----------------------------------------------------------------*/

#if defined(BSP_USING_EMAC0)
/**
 * @brief EMAC0 RX interrupt vector. Dispatches to the shared RX ISR.
 */
void EMAC0_RX_IRQHandler(void)
{
    /* Handle RX interrupt for the EMAC0 instance */
    nu_emac_rx_isr((void *)&nu_emac_arr[EMAC0_IDX]);
}

/**
 * @brief EMAC0 TX interrupt vector. Dispatches to the shared TX ISR.
 */
void EMAC0_TX_IRQHandler(void)
{
    /* Handle TX interrupt for the EMAC0 instance */
    nu_emac_tx_isr((void *)&nu_emac_arr[EMAC0_IDX]);
}
#endif

#if defined(BSP_USING_EMAC1)
/**
 * @brief EMAC1 RX interrupt vector. Dispatches to the shared RX ISR.
 */
void EMAC1_RX_IRQHandler(void)
{
    /* Handle RX interrupt for the EMAC1 instance */
    nu_emac_rx_isr((void *)&nu_emac_arr[EMAC1_IDX]);
}

/**
 * @brief EMAC1 TX interrupt vector. Dispatches to the shared TX ISR.
 */
void EMAC1_TX_IRQHandler(void)
{
    /* Handle TX interrupt for the EMAC1 instance */
    nu_emac_tx_isr((void *)&nu_emac_arr[EMAC1_IDX]);
}
#endif

/**
 * @brief Stop EMAC TX and RX.
 */
static void nu_emac_halt(nu_emac_t psNuEMAC)
{
    EMAC_T *emac = psNuEMAC->memmgr.psEmac;

    /* Disable the MAC receiver */
    EMAC_DISABLE_RX(emac);
    /* Disable the MAC transmitter */
    EMAC_DISABLE_TX(emac);
}

/**
 * @brief Reinitialize EMAC while preserving CAM (MAC filter) entries.
 *        Called on bus error to recover from hardware fault.
 */
static void nu_emac_reinit(nu_emac_t psNuEMAC)
{
    rt_uint32_t EMAC_CAMxM[EMAC_CAMENTRY_NB];
    rt_uint32_t EMAC_CAMxL[EMAC_CAMENTRY_NB];
    rt_uint32_t EMAC_CAMEN;
    EMAC_T *emac = psNuEMAC->memmgr.psEmac;

    // Backup MAC address.
    EMAC_CAMEN = emac->CAMEN;
    for (rt_uint8_t index = 0 ; index < EMAC_CAMENTRY_NB; index ++)
    {
        rt_uint32_t *CAMxM = (rt_uint32_t *)((rt_uint32_t)&emac->CAM0M + (index * 8));
        rt_uint32_t *CAMxL = (rt_uint32_t *)((rt_uint32_t)&emac->CAM0L + (index * 8));

        EMAC_CAMxM[index] = *CAMxM;
        EMAC_CAMxL[index] = *CAMxL;
    }

    nu_emac_halt(psNuEMAC);
    /* Disable the EMAC engine */
    EMAC_Close(emac);
    /* Re-open EMAC with its descriptors and MAC address */
    EMAC_Open(&psNuEMAC->memmgr, (uint8_t *)&psNuEMAC->mac_addr[0]);
    /* Re-enable the transmitter */
    EMAC_ENABLE_TX(emac);
    /* Re-enable the receiver */
    EMAC_ENABLE_RX(emac);

    // Restore MAC address.
    for (rt_uint8_t index = 0 ; index < EMAC_CAMENTRY_NB; index ++)
    {
        rt_uint32_t *CAMxM = (rt_uint32_t *)((rt_uint32_t)&emac->CAM0M + (index * 8));
        rt_uint32_t *CAMxL = (rt_uint32_t *)((rt_uint32_t)&emac->CAM0L + (index * 8));

        *CAMxM = EMAC_CAMxM[index];
        *CAMxL = EMAC_CAMxL[index];
    }
    emac->CAMEN = EMAC_CAMEN;
}

/**
 * @brief IGMP multicast MAC filter callback.
 *        Converts IPv4 multicast address to MAC and programs CAM entry.
 */
#if LWIP_IPV4 && LWIP_IGMP
static err_t nu_igmp_mac_filter(struct netif *netif, const ip4_addr_t *ip4_addr, enum netif_mac_filter_action action)
{
    nu_emac_t psNuEMAC = (nu_emac_t)netif->state;
    rt_uint8_t mac[6];
    int32_t ret = 0;
    const uint8_t *p = (const uint8_t *)ip4_addr;

    mac[0] = 0x01;
    mac[1] = 0x00;
    mac[2] = 0x5E;
    mac[3] = *(p + 1) & 0x7F;
    mac[4] = *(p + 2);
    mac[5] = *(p + 3);

    /* Program the multicast MAC into a CAM filter entry */
    ret = EMAC_FillCamEntry(psNuEMAC->memmgr.psEmac, (uint8_t *)&mac[0]);
    if (ret >= 0)
    {
        LOG_D("%s %s %s ", __FUNCTION__, (action == NETIF_ADD_MAC_FILTER) ? "add" : "del", ip4addr_ntoa(ip4_addr));
        LOG_D("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    return (ret >= 0) ? 0 : -1;
}
#endif /* LWIP_IPV4 && LWIP_IGMP */

/**
 * @brief Periodic timer callback that monitors PHY link status.
 *        Performs deferred PHY initialization and notifies the stack on change.
 */
static void nu_emac_link_monitor(void *param)
{
    nu_emac_t psNuEMAC = (nu_emac_t)param;
    EMAC_T *emac = psNuEMAC->memmgr.psEmac;

    if (!psNuEMAC->phy_inited)
    {
        /* Initialize the PHY on first run (deferred out of init for faster boot) */
        EMAC_PhyInit(emac);
        psNuEMAC->phy_inited = RT_TRUE;
    }

    /* Poll the current PHY link status */
    uint32_t LinkStatus_Current = EMAC_CheckLinkStatus(emac);
    /* linkchange */
    if (psNuEMAC->link_status_last != LinkStatus_Current)
    {

        switch (LinkStatus_Current)
        {
        case EMAC_LINK_DOWN:
            LOG_I("[%s] Link status: Down", psNuEMAC->name);
            break;

        case EMAC_LINK_100F:
            LOG_I("[%s] Link status: 100F", psNuEMAC->name);
            break;

        case EMAC_LINK_100H:
            LOG_I("[%s] Link status: 100H", psNuEMAC->name);
            break;

        case EMAC_LINK_10F:
            LOG_I("[%s] Link status: 10F", psNuEMAC->name);
            break;

        case EMAC_LINK_10H:
            LOG_I("[%s] Link status: 10H", psNuEMAC->name);
            break;
        } /* switch( LinkStatus_Current ) */

        /* Send link status to upper layer. */
        if (LinkStatus_Current == EMAC_LINK_DOWN)
        {
            /* Notify the stack that the link went down */
            eth_device_linkchange(&psNuEMAC->eth, RT_FALSE);
        }
        else
        {
            /* Notify the stack that the link is up */
            eth_device_linkchange(&psNuEMAC->eth, RT_TRUE);
        }
        psNuEMAC->link_status_last = LinkStatus_Current;

    } /* if ( psNuEMAC->link_status_last != LinkStatus_Current ) */
}

/**
 * @brief Allocate and initialize DMA TX/RX descriptors and frame buffers.
 */
static void nu_memmgr_init(EMAC_MEMMGR_T *psMemMgr)
{
    psMemMgr->u32TxDescSize = EMAC_TX_DESC_SIZE;
    psMemMgr->u32RxDescSize = EMAC_RX_DESC_SIZE;

    /* Allocate a 32-byte aligned TX descriptor ring for the DMA */
    psMemMgr->psTXDescs = (EMAC_DESCRIPTOR_T *) rt_malloc_align(sizeof(EMAC_DESCRIPTOR_T) * psMemMgr->u32TxDescSize, 32);
    RT_ASSERT(psMemMgr->psTXDescs != RT_NULL);

    /* Allocate a 32-byte aligned RX descriptor ring for the DMA */
    psMemMgr->psRXDescs = (EMAC_DESCRIPTOR_T *) rt_malloc_align(sizeof(EMAC_DESCRIPTOR_T) * psMemMgr->u32RxDescSize, 32);
    RT_ASSERT(psMemMgr->psRXDescs != RT_NULL);

    /* Allocate 32-byte aligned TX frame buffers */
    psMemMgr->psTXFrames = (EMAC_FRAME_T *) rt_malloc_align(sizeof(EMAC_FRAME_T) * psMemMgr->u32TxDescSize, 32);
    RT_ASSERT(psMemMgr->psTXFrames != RT_NULL);

    /* Allocate 32-byte aligned RX frame buffers */
    psMemMgr->psRXFrames = (EMAC_FRAME_T *) rt_malloc_align(sizeof(EMAC_FRAME_T) * psMemMgr->u32RxDescSize, 32);
    RT_ASSERT(psMemMgr->psRXFrames != RT_NULL);

    LOG_I("TxDesc num: %d, RxDesc num: %d", EMAC_TX_DESC_SIZE, EMAC_RX_DESC_SIZE);
}

/**
 * @brief Initialize EMAC hardware, start DMA, and create link monitor timer.
 *        Called by RT-Thread device framework on first use.
 */
static rt_err_t nu_emac_init(rt_device_t dev)
{
    char szTmp[16];
    nu_emac_t psNuEMAC = (nu_emac_t)dev;
    EMAC_T *emac = psNuEMAC->memmgr.psEmac;
    rt_err_t ret = RT_EOK;

    /* Allocate DMA descriptors and frame buffers */
    nu_memmgr_init(&psNuEMAC->memmgr);

    /* Build the PHY timer name, e.g. "e0phy" */
    snprintf(szTmp, sizeof(szTmp), "%sphy", psNuEMAC->name);

    /* Reset the EMAC engine to a known state */
    EMAC_Reset(emac);

    /* Disable EMAC before reconfiguring */
    EMAC_Close(emac);
    /* Program descriptors and MAC address */
    EMAC_Open(&psNuEMAC->memmgr, (uint8_t *)&psNuEMAC->mac_addr[0]);

    /* Enable the receiver */
    EMAC_ENABLE_RX(emac);
    /* Enable the transmitter */
    EMAC_ENABLE_TX(emac);

    /* Kick the RX DMA to start receiving */
    EMAC_TRIGGER_RX(emac);

#if defined(LWIP_IPV4) && defined(LWIP_IGMP)
    netif_set_igmp_mac_filter(psNuEMAC->eth.netif, nu_igmp_mac_filter);
#endif /* LWIP_IPV4 && LWIP_IGMP */

    LOG_D("Create %s link monitor timer.", psNuEMAC->name);
    /* Create timer to monitor link status. */
    psNuEMAC->link_timer = rt_timer_create(szTmp,
                                           nu_emac_link_monitor,
                                           (void *)psNuEMAC,
                                           RT_TICK_PER_SECOND,
                                           RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    RT_ASSERT(psNuEMAC->link_timer != RT_NULL);

    /* Start the periodic link monitor timer */
    ret = rt_timer_start(psNuEMAC->link_timer);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

/**
 * @brief Device open handler. Nothing to do; the stack drives TX/RX directly.
 */
static rt_err_t nu_emac_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

/**
 * @brief Device close handler. No resources to release here.
 */
static rt_err_t nu_emac_close(rt_device_t dev)
{
    return RT_EOK;
}

/**
 * @brief Device read handler. Not supported for an ethernet device.
 */
static rt_ssize_t nu_emac_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    /* Reading through the device API is unsupported; report not-implemented */
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

/**
 * @brief Device write handler. Not supported for an ethernet device.
 */
static rt_ssize_t nu_emac_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    /* Writing through the device API is unsupported; report not-implemented */
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

/**
 * @brief Device control handler. Supports NIOCTL_GADDR to retrieve MAC address.
 */
static rt_err_t nu_emac_control(rt_device_t dev, int cmd, void *args)
{
    nu_emac_t psNuEMAC = (nu_emac_t)dev;
    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* Get MAC address */
        if (args)
            rt_memcpy(args, &psNuEMAC->mac_addr[0], NETIF_MAX_HWADDR_LEN);
        else
            return -RT_ERROR;

        break;

    default :
        break;
    }

    return RT_EOK;
}

/**
 * @brief Transmit a pbuf chain.
 *        Copies pbuf payload into EMAC DMA TX buffer and triggers send.
 */
static rt_err_t nu_emac_tx(rt_device_t dev, struct pbuf *p)
{
    nu_emac_t psNuEMAC = (nu_emac_t)dev;
    struct pbuf *q;
    rt_uint32_t offset = 0;
    rt_uint8_t *buf;

    buf = (rt_uint8_t *)EMAC_ClaimFreeTXBuf(&psNuEMAC->memmgr);
    /* Get free TX buffer */
    if (buf == RT_NULL)
        return -RT_ERROR;

    for (q = p; q != NULL; q = q->next)
    {
        rt_uint8_t *ptr;
        rt_uint32_t len;

        len = q->len;
        ptr = q->payload;

        /* Copy this pbuf segment into the contiguous TX buffer */
        rt_memcpy(&buf[offset], ptr, len);

        offset += len;
    }

    /* Hand the buffer to the DMA and start transmission */
    if (!EMAC_SendPktWoCopy(&psNuEMAC->memmgr, offset))
        return -RT_ERROR;

    /* Advance the TX descriptor and release the slot */
    EMAC_SendPktDone(&psNuEMAC->memmgr);

    /* Return SUCCESS? */
    return  RT_EOK;
}

/**
 * @brief Receive one packet from EMAC DMA RX buffer.
 *        Called by erx thread after eth_device_ready() notification.
 *        Re-enables RX interrupts when no more packets are available.
 *
 * @return pbuf containing received packet, or RT_NULL if none available.
 */
static struct pbuf *nu_emac_rx(rt_device_t dev)
{
    nu_emac_t psNuEMAC = (nu_emac_t)dev;
    struct pbuf *p = RT_NULL;
    uint8_t *pu8DataBuf = NULL;
    int s32PktLen;

    if ((s32PktLen = EMAC_GetAvailRXBufSize(&psNuEMAC->memmgr, &pu8DataBuf)) > 0)
    {
        /* Get current RX descriptor. */
        EMAC_DESCRIPTOR_T *cur_rx = EMAC_RecvPktDoneWoRxTrigger(&psNuEMAC->memmgr);

        /* Allocate a pbuf chain of pbufs from the pool. */
        p = pbuf_alloc(PBUF_RAW, s32PktLen, PBUF_RAM);
        if (p != RT_NULL)
        {
            /* Copy the DMA buffer contents into the pbuf */
            pbuf_take(p, pu8DataBuf, s32PktLen);
        }
        else
        {
            LOG_W("[%s] drop the packet %08x", psNuEMAC->name, pu8DataBuf);
        }

        /* Free or drop descriptor. */
        EMAC_RxTrigger(&psNuEMAC->memmgr, cur_rx);
    }
    else
    {
        /* No available RX packet, re-enable RX interrupts. */
        EMAC_T *emac = psNuEMAC->memmgr.psEmac;

        EMAC_CLEAR_INT_FLAG(emac, (EMAC_INTSTS_RDUIF_Msk | EMAC_INTSTS_RXGDIF_Msk));
        EMAC_ENABLE_INT(emac, (EMAC_INTEN_RDUIEN_Msk | EMAC_INTEN_RXGDIEN_Msk));
        EMAC_TRIGGER_RX(emac);
    }

    return p;
}

/**
 * @brief EMAC RX interrupt service routine.
 *        Disables RX interrupt and signals erx thread via eth_device_ready().
 *        Handles RX descriptor unavailable and bus error conditions.
 */
static void nu_emac_rx_isr(void *param)
{
    nu_emac_t psNuEMAC = (nu_emac_t)param;
    EMAC_T *emac = psNuEMAC->memmgr.psEmac;

    /* Enter interrupt */
    rt_interrupt_enter();

    uint32_t u32INTSTS = emac->INTSTS & 0x0000FFFFU;

    if (u32INTSTS & EMAC_INTSTS_RDUIF_Msk)
    {
        /* No RX descriptor available, we need to get data from RX pool */
        LOG_W("No RX descriptor available, INTEN=%08x, INTSTS=%08x", emac->INTEN, emac->INTSTS);
        EMAC_DISABLE_INT(emac, (EMAC_INTEN_RDUIEN_Msk | EMAC_INTEN_RXGDIEN_Msk));
        EMAC_CLEAR_INT_FLAG(emac, (EMAC_INTSTS_RDUIF_Msk | EMAC_INTSTS_RXGDIF_Msk));
        u32INTSTS &= ~(EMAC_INTSTS_RDUIF_Msk | EMAC_INTSTS_RXGDIF_Msk);

        eth_device_ready(&psNuEMAC->eth);
    }
    else if (u32INTSTS & EMAC_INTSTS_RXGDIF_Msk)
    {
        EMAC_DISABLE_INT(emac, EMAC_INTEN_RXGDIEN_Msk);
        EMAC_CLEAR_INT_FLAG(emac, (EMAC_INTSTS_RXGDIF_Msk));
        u32INTSTS &= ~EMAC_INTSTS_RXGDIF_Msk;

        /* A good packet ready. */
        eth_device_ready(&psNuEMAC->eth);
    }

    /* Receive Bus Error Interrupt */
    if (u32INTSTS & EMAC_INTSTS_RXBEIF_Msk)
    {
        LOG_W("Reinit Rx emac");
        EMAC_CLEAR_INT_FLAG(emac, (EMAC_INTSTS_RXBEIF_Msk));
        u32INTSTS &= ~EMAC_INTSTS_RXBEIF_Msk;

        nu_emac_reinit(psNuEMAC);
    }

    EMAC_CLEAR_INT_FLAG(emac, u32INTSTS);

    /* Leave interrupt */
    rt_interrupt_leave();
}

/**
 * @brief EMAC TX interrupt service routine.
 *        Handles TX complete and TX bus error conditions.
 */
static void nu_emac_tx_isr(void *param)
{
    nu_emac_t psNuEMAC = (nu_emac_t)param;
    EMAC_T *emac = psNuEMAC->memmgr.psEmac;

    /* Enter interrupt */
    rt_interrupt_enter();

    uint32_t u32INTSTS = emac->INTSTS & 0xFFFF0000U;

    /* Wake-up suspended process to send */
    if (u32INTSTS & EMAC_INTSTS_TXCPIF_Msk)
    {
        EMAC_CLEAR_INT_FLAG(emac, (EMAC_INTSTS_TXCPIF_Msk));
        u32INTSTS &= ~EMAC_INTSTS_TXCPIF_Msk;
    }

    if (u32INTSTS & EMAC_INTSTS_TXBEIF_Msk)
    {
        LOG_W("Reinit Tx emac");
        EMAC_CLEAR_INT_FLAG(emac, (EMAC_INTSTS_TXBEIF_Msk));
        u32INTSTS &= ~EMAC_INTSTS_TXBEIF_Msk;

        nu_emac_reinit(psNuEMAC);
    }

    EMAC_CLEAR_INT_FLAG(emac, u32INTSTS);

    /* Leave interrupt */
    rt_interrupt_leave();
}

/**
 * @brief Register one EMAC instance with the RT-Thread ethernet framework.
 *        Derives MAC address from chip UID, sets up device ops, enables IRQs.
 */
static rt_err_t rt_hw_emac_register(nu_emac_t psNuEMAC)
{
    rt_uint32_t value = 0;

    RT_ASSERT(psNuEMAC != RT_NULL);

    /* Read UID from FMC */
    SYS_UnlockReg();
    FMC_Open();
    for (rt_uint8_t i = 0; i < 3; i++)
    {
        value += FMC_ReadUID(i);
    }
    FMC_Close();

    /* Assign MAC address */
    psNuEMAC->mac_addr[0] = 0x82;
    psNuEMAC->mac_addr[1] = 0x06;
    psNuEMAC->mac_addr[2] = 0x21;
    psNuEMAC->mac_addr[3] = (value >> 16) & 0xff;
    psNuEMAC->mac_addr[4] = (value >> 8) & 0xff;
    psNuEMAC->mac_addr[5] = ((value) & 0xff) + (psNuEMAC - &nu_emac_arr[0]);

    LOG_I("MAC address: %02X:%02X:%02X:%02X:%02X:%02X", \
          psNuEMAC->mac_addr[0], \
          psNuEMAC->mac_addr[1], \
          psNuEMAC->mac_addr[2], \
          psNuEMAC->mac_addr[3], \
          psNuEMAC->mac_addr[4], \
          psNuEMAC->mac_addr[5]);

    /* Register member functions */
    psNuEMAC->eth.parent.init       = nu_emac_init;
    psNuEMAC->eth.parent.open       = nu_emac_open;
    psNuEMAC->eth.parent.close      = nu_emac_close;
    psNuEMAC->eth.parent.read       = nu_emac_read;
    psNuEMAC->eth.parent.write      = nu_emac_write;
    psNuEMAC->eth.parent.control    = nu_emac_control;
    psNuEMAC->eth.parent.user_data  = RT_NULL;
    psNuEMAC->eth.eth_rx            = nu_emac_rx;
    psNuEMAC->eth.eth_tx            = nu_emac_tx;

    /* Configure and enable the TX interrupt */
    NVIC_SetPriority(psNuEMAC->irqn_tx, 1);
    NVIC_EnableIRQ(psNuEMAC->irqn_tx);

    /* Configure and enable the RX interrupt */
    NVIC_SetPriority(psNuEMAC->irqn_rx, 1);
    NVIC_EnableIRQ(psNuEMAC->irqn_rx);

    /* Register with the RT-Thread ethernet framework */
    return eth_device_init(&psNuEMAC->eth, psNuEMAC->name);
}

/**
 * @brief Driver entry point. Registers all enabled EMAC instances.
 *        Exported via INIT_APP_EXPORT for automatic initialization.
 */
static int rt_hw_emac_init(void)
{
    rt_err_t ret = RT_EOK;
    int i;

    for (i = (EMAC_START + 1); i < EMAC_CNT; i++)
    {
        /* Register a single EMAC instance with the framework */
        ret = rt_hw_emac_register(&nu_emac_arr[i]);
        if (ret != RT_EOK)
        {
            LOG_E("Failed to register %s", nu_emac_arr[i].name);
        }
    }

    return ret;
}

INIT_APP_EXPORT(rt_hw_emac_init);