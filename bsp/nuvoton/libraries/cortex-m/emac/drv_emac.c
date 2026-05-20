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

#include "synopGMAC_Host.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.emac"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

enum
{
    EMAC_START = -1,
#if defined(BSP_USING_EMAC0)
    EMAC0_IDX,
#endif
    EMAC_CNT
};

/* Private typedef --------------------------------------------------------------*/

struct nu_emac_lwip_pbuf
{
    struct pbuf_custom p;  // lwip pbuf
    PKT_FRAME_T *psPktFrameDataBuf; // emac descriptor
    synopGMACdevice *emacdev;
    const struct memp_desc *pool;
};

typedef struct nu_emac_lwip_pbuf *nu_emac_lwip_pbuf_t;

struct nu_emac
{
    struct eth_device   eth;
    const struct nu_module m_module;
    rt_timer_t         link_timer;
    rt_uint8_t         mac_addr[8];
    synopGMACNetworkAdapter *adapter;
    const struct memp_desc *memp_rx_pool;
};
typedef struct nu_emac *nu_emac_t;

/* Private variables ------------------------------------------------------------*/
#if defined(BSP_USING_EMAC0)
LWIP_MEMPOOL_DECLARE(emac0_rx, RECEIVE_DESC_SIZE, sizeof(struct nu_emac_lwip_pbuf), "GMAC0 RX PBUF pool");
#endif

static struct nu_emac nu_emac_arr[] =
{
#if defined(BSP_USING_EMAC0)
    {
        .m_module        =  {
            .name        =  "e0",
            .base        =  (void *)EMAC0_BASE,
            .eIRQn       =  EMAC0_IRQn,
        },
        .memp_rx_pool    =  &memp_emac0_rx
    },
#endif
};

static int nu_emac_mdio_read(void *adapter, int addr, int reg)
{
    synopGMACdevice *emacdev = ((synopGMACNetworkAdapter *)adapter)->m_gmacdev;
    u16 data;
    synopGMAC_read_phy_reg(emacdev->MacBase, addr, reg, &data);
    return data;
}

static void nu_emac_mdio_write(void *adapter, int addr, int reg, int data)
{
    synopGMACdevice *emacdev = ((synopGMACNetworkAdapter *)adapter)->m_gmacdev;
    synopGMAC_write_phy_reg(emacdev->MacBase, addr, reg, data);
}

s32 synopGMAC_check_phy_init(synopGMACNetworkAdapter *adapter)
{
    struct ethtool_cmd cmd;
    synopGMACdevice *emacdev = adapter->m_gmacdev;

    if (!mii_link_ok(&adapter->m_mii))
    {
        emacdev->DuplexMode = FULLDUPLEX;
        emacdev->Speed      =   SPEED100;
        return 0;
    }
    else
    {
        mii_ethtool_gset(&adapter->m_mii, &cmd);
        emacdev->DuplexMode = (cmd.duplex == DUPLEX_FULL)  ? FULLDUPLEX : HALFDUPLEX ;
        if (cmd.speed == SPEED_1000)
            emacdev->Speed = SPEED1000;
        else if (cmd.speed == SPEED_100)
            emacdev->Speed = SPEED100;
        else
            emacdev->Speed = SPEED10;
    }

    return emacdev->Speed | (emacdev->DuplexMode << 4);
}

static void nu_emac_isr(int irqno, void *param)
{
    nu_emac_t psNuEMAC = (nu_emac_t)param;

    synopGMACNetworkAdapter *adapter = psNuEMAC->adapter;
    synopGMACdevice *emacdev = (synopGMACdevice *)adapter->m_gmacdev;

    u32 interrupt, dma_status_reg;
    s32 status;
    u32 u32GmacIntSts;
    u32 u32GmacDmaIE = DmaIntEnable;

    // Check GMAC interrupt
    u32GmacIntSts = synopGMACReadReg(emacdev->MacBase, GmacInterruptStatus);
    if (u32GmacIntSts & GmacTSIntSts)
    {
        emacdev->synopGMACNetStats.ts_int = 1;
        status = synopGMACReadReg(emacdev->MacBase, GmacTSStatus);
        if (!(status & (1 << 1)))
        {
            LOG_D("TS alarm flag not set??");
        }
        else
        {
            LOG_D("TS alarm!!!!!!!!!!!!!!!!");
        }
    }

    synopGMACWriteReg(emacdev->MacBase, GmacInterruptStatus, u32GmacIntSts);

    dma_status_reg = synopGMACReadReg(emacdev->DmaBase, DmaStatus);
    if (dma_status_reg == 0)
    {
        return;
    }

    if (dma_status_reg & GmacPmtIntr)
    {
        LOG_I("Interrupt due to PMT module");
        synopGMAC_powerup_mac(emacdev);
    }

    if (dma_status_reg & GmacLineIntfIntr)
    {
        LOG_I("Interrupt due to GMAC LINE module");
    }

    interrupt = synopGMAC_get_interrupt_type(emacdev);
    LOG_D("Interrupts to be handled: 0x%08x  %08x", interrupt, synopGMAC_get_ie(emacdev));

    if (interrupt & synopGMACDmaError)
    {
        LOG_E("Fatal Bus Error Inetrrupt Seen");
        synopGMAC_disable_dma_tx(emacdev);
        synopGMAC_disable_dma_rx(emacdev);

        synopGMAC_take_desc_ownership_tx(emacdev);
        synopGMAC_take_desc_ownership_rx(emacdev);

        synopGMAC_init_tx_rx_desc_queue(emacdev);

        synopGMAC_reset(emacdev);

        synopGMAC_set_mac_addr(emacdev, GmacAddr0High, GmacAddr0Low, &psNuEMAC->mac_addr[0]);
        synopGMAC_dma_bus_mode_init(emacdev, DmaBurstLength32 | DmaDescriptorSkip0/*DmaDescriptorSkip2*/ | DmaDescriptor8Words);
        synopGMAC_dma_control_init(emacdev, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
        synopGMAC_init_rx_desc_base(emacdev);
        synopGMAC_init_tx_desc_base(emacdev);
        synopGMAC_mac_init(emacdev);
        synopGMAC_enable_dma_rx(emacdev);
        synopGMAC_enable_dma_tx(emacdev);

    }

    if ((interrupt & synopGMACDmaRxNormal) ||
            (interrupt & synopGMACDmaRxAbnormal))
    {
        if (interrupt & synopGMACDmaRxNormal)
        {
            LOG_D("Rx Normal");
            u32GmacDmaIE &= ~DmaIntRxNormMask;
        }
        if (interrupt & synopGMACDmaRxAbnormal)
        {
            LOG_W("Abnormal Rx Interrupt Seen %08x", dma_status_reg);

            if (emacdev->GMAC_Power_down == 0)
            {
                emacdev->synopGMACNetStats.rx_over_errors++;
                u32GmacDmaIE &= ~DmaIntRxAbnMask;
                //synopGMAC_resume_dma_rx(emacdev);
            }
        }
        eth_device_ready(&psNuEMAC->eth);
    }

    if (interrupt & synopGMACDmaRxStopped)
    {
        LOG_W("Receiver stopped seeing Rx interrupts"); //Receiver gone in to stopped state
        if (emacdev->GMAC_Power_down == 0)   // If Mac is not in powerdown
        {
            emacdev->synopGMACNetStats.rx_over_errors++;
            synopGMAC_enable_dma_rx(emacdev);
        }
    }

    if (interrupt & synopGMACDmaTxNormal)
    {
        LOG_D("Finished Normal Transmission");
        synop_handle_transmit_over(emacdev);//Do whatever you want after the transmission is over
    }

    if (interrupt & synopGMACDmaTxAbnormal)
    {
        LOG_W("Abnormal Tx Interrupt Seen");
        if (emacdev->GMAC_Power_down == 0)   // If Mac is not in powerdown
        {
            synop_handle_transmit_over(emacdev);
        }
    }

    if (interrupt & synopGMACDmaTxStopped)
    {
        LOG_W("Transmitter stopped sending the packets");
        if (emacdev->GMAC_Power_down == 0)    // If Mac is not in powerdown
        {
            synopGMAC_disable_dma_tx(emacdev);
            synopGMAC_take_desc_ownership_tx(emacdev);
            synopGMAC_enable_dma_tx(emacdev);
            LOG_D("Transmission Resumed");
        }
    }

    /* Enable the interrrupt before returning from ISR*/
    synopGMAC_enable_interrupt(emacdev, u32GmacDmaIE);
}

#if defined(BSP_USING_EMAC0)
void EMAC0_IRQHandler(void)
{
    rt_interrupt_enter();
    nu_emac_isr(EMAC0_IRQn, &nu_emac_arr[EMAC0_IDX]);
    rt_interrupt_leave();
}
#endif

void nu_emac_link_monitor(void *pvData)
{
    s32 data;
    nu_emac_t psNuEMAC = (nu_emac_t)pvData;

    synopGMACNetworkAdapter *adapter = psNuEMAC->adapter;
    synopGMACdevice         *emacdev = adapter->m_gmacdev;
    if (!mii_link_ok(&adapter->m_mii))
    {
        if (emacdev->LinkState)
        {
            eth_device_linkchange(&psNuEMAC->eth, RT_FALSE);
            LOG_W("No Link");
        }
        emacdev->DuplexMode = 0;
        emacdev->Speed = 0;
        emacdev->LoopBackMode = 0;
        emacdev->LinkState = 0;
    }
    else
    {
        data = synopGMAC_check_phy_init(adapter);
        if (emacdev->LinkState != data)
        {
            emacdev->LinkState = data;
            synopGMAC_mac_init(emacdev);
            LOG_I("Link is up in %s mode", (emacdev->DuplexMode == FULLDUPLEX) ? "FULL DUPLEX" : "HALF DUPLEX");
            if (emacdev->Speed == SPEED1000)
            {
                LOG_I("Link is with 1000M Speed");
                synopGMAC_set_mode(emacdev, 0);
            }
            if (emacdev->Speed == SPEED100)
            {
                LOG_I("Link is with 100M Speed");
                synopGMAC_set_mode(emacdev, 1);
            }
            if (emacdev->Speed == SPEED10)
            {
                LOG_I("Link is with 10M Speed");
                synopGMAC_set_mode(emacdev, 2);
            }
            eth_device_linkchange(&psNuEMAC->eth, RT_TRUE);
        }
    }
}

static void nu_memmgr_init(GMAC_MEMMGR_T *psMemMgr)
{
    psMemMgr->u32TxDescSize = TRANSMIT_DESC_SIZE;
    psMemMgr->u32RxDescSize = RECEIVE_DESC_SIZE;

    psMemMgr->psTXDescs = (DmaDesc *) rt_malloc_align(sizeof(DmaDesc) * psMemMgr->u32TxDescSize, 32);
    RT_ASSERT(psMemMgr->psTXDescs != RT_NULL);
    LOG_D("First TXDescAddr= %08x", psMemMgr->psTXDescs);

    psMemMgr->psRXDescs = (DmaDesc *) rt_malloc_align(sizeof(DmaDesc) * psMemMgr->u32RxDescSize, 32);
    RT_ASSERT(psMemMgr->psRXDescs != RT_NULL);
    LOG_D("First RXDescAddr= %08x", psMemMgr->psRXDescs);
    psMemMgr->psTXFrames = (PKT_FRAME_T *) rt_malloc_align(sizeof(PKT_FRAME_T) * psMemMgr->u32TxDescSize, 32);
    RT_ASSERT(psMemMgr->psTXFrames != RT_NULL);
    LOG_D("First TXFrameAddr= %08x", psMemMgr->psTXFrames);

    psMemMgr->psRXFrames = (PKT_FRAME_T *) rt_malloc_align(sizeof(PKT_FRAME_T) * psMemMgr->u32RxDescSize, 32);
    RT_ASSERT(psMemMgr->psRXFrames != RT_NULL);
    LOG_D("First RXFrameAddr= %08x", psMemMgr->psRXFrames);
}

static void nu_mii_init(synopGMACNetworkAdapter *adapter)
{
    /* MII setup */
    adapter->m_mii.phy_id_mask   = 0x1F;
    adapter->m_mii.reg_num_mask  = 0x1F;
    adapter->m_mii.adapter       = (void *)adapter;
    adapter->m_mii.mdio_read     = nu_emac_mdio_read;
    adapter->m_mii.mdio_write    = nu_emac_mdio_write;
    adapter->m_mii.phy_id        = adapter->m_gmacdev->PhyBase;
    adapter->m_mii.supports_gmii = mii_check_gmii_support(&adapter->m_mii);
}

static rt_err_t nu_emac_init(rt_device_t device)
{
    rt_err_t ret;
    s32 status = 0;
    int count;

    nu_emac_t psNuEMAC = (nu_emac_t)device;
    RT_ASSERT(psNuEMAC != RT_NULL);

    synopGMACNetworkAdapter *adapter = psNuEMAC->adapter;
    synopGMACdevice *emacdev = (synopGMACdevice *)adapter->m_gmacdev;
    GMAC_MEMMGR_T *psemacmemmgr = (GMAC_MEMMGR_T *)adapter->m_gmacmemmgr;

    RT_ASSERT(emacdev != RT_NULL);
    RT_ASSERT(psemacmemmgr != RT_NULL);

    LOG_D("Init %s", psNuEMAC->m_module.name);

    synopGMAC_attach(emacdev,
                     ((uint32_t)psNuEMAC->m_module.base + MACBASE),
                     ((uint32_t)psNuEMAC->m_module.base + DMABASE),
                     DEFAULT_PHY_BASE,
                     &psNuEMAC->mac_addr[0]);
    nu_mii_init(adapter);

    /* Reset to make RGMII/RMII setting take affect. */
    synopGMAC_reset(emacdev);
    synopGMAC_read_version(emacdev);

    /*Check for Phy initialization*/
    synopGMAC_set_mdc_clk_div(emacdev, GmiiCsrClk2);
    emacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(emacdev);
    status = synopGMAC_check_phy_init(adapter);

    /*Set up the tx and rx descriptor queue/ring*/
    LOG_D("tx desc_queue");
    synopGMAC_setup_tx_desc_queue(emacdev, &psemacmemmgr->psTXDescs[0], TRANSMIT_DESC_SIZE, RINGMODE);
    synopGMAC_init_tx_desc_base(emacdev);
    LOG_D("DmaTxBaseAddr = %08x", synopGMACReadReg(emacdev->DmaBase, DmaTxBaseAddr));

    LOG_D("rx desc_queue");
    synopGMAC_setup_rx_desc_queue(emacdev, &psemacmemmgr->psRXDescs[0], RECEIVE_DESC_SIZE, RINGMODE);
    synopGMAC_init_rx_desc_base(emacdev);
    LOG_D("DmaRxBaseAddr = %08x", synopGMACReadReg(emacdev->DmaBase, DmaRxBaseAddr));

    /*Initialize the dma interface*/
    synopGMAC_dma_bus_mode_init(emacdev, DmaBurstLength32 | DmaDescriptorSkip0/*DmaDescriptorSkip2*/ | DmaDescriptor8Words);
    synopGMAC_dma_control_init(emacdev, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);

    /*Initialize the mac interface*/
    synopGMAC_mac_init(emacdev);
    //synopGMAC_promisc_enable(emacdev);

    synopGMAC_pause_control(emacdev); // This enables the pause control in Full duplex mode of operation

#if defined(RT_LWIP_USING_HW_CHECKSUM)
    /*IPC Checksum offloading is enabled for this driver. Should only be used if Full Ip checksumm offload engine is configured in the hardware*/
    synopGMAC_enable_rx_chksum_offload(emacdev);    //Enable the offload engine in the receive path
    synopGMAC_rx_tcpip_chksum_drop_enable(emacdev); // This is default configuration, DMA drops the packets if error in encapsulated ethernet payload
#endif

    /* Set all RX frame buffers. */
    count = 0;
    do
    {
        LOG_D("Set %d pkt frame buffer address - 0x%08x, size=%d", count, (u32)(&psemacmemmgr->psRXFrames[count]), PKT_FRAME_BUF_SIZE);
        status = synopGMAC_set_rx_qptr(emacdev, (u32)(&psemacmemmgr->psRXFrames[count]), PKT_FRAME_BUF_SIZE, 0);
        if (status < 0)
        {
            LOG_E("status < 0!!");
            break;
        }
        count++;
    }
    while (count < RECEIVE_DESC_SIZE);

    synopGMAC_clear_interrupt(emacdev);

    synopGMAC_disable_interrupt_all(emacdev);
    synopGMAC_enable_interrupt(emacdev, DmaIntEnable);
    LOG_D("get_ie: %08x", synopGMAC_get_ie(emacdev));

    synopGMAC_enable_dma_rx(emacdev);
    synopGMAC_enable_dma_tx(emacdev);

    synopGMAC_set_mac_addr(emacdev, GmacAddr0High, GmacAddr0Low, &psNuEMAC->mac_addr[0]);

    synopGMAC_set_mode(emacdev, 0);

    LOG_D("Create %s link monitor timer. ", psNuEMAC->m_module.name);
    /* Create timer to monitor link status. */
    psNuEMAC->link_timer = rt_timer_create("link_timer",
                                           nu_emac_link_monitor,
                                           (void *)psNuEMAC,
                                           RT_TICK_PER_SECOND,
                                           RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(psNuEMAC->link_timer != RT_NULL);

    ret = rt_timer_start(psNuEMAC->link_timer);
    RT_ASSERT(ret == RT_EOK);

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(psNuEMAC->m_module.eIRQn);

    LOG_D("Init %s done", psNuEMAC->m_module.name);

    return RT_EOK;
}

static rt_err_t nu_emac_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t nu_emac_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_ssize_t nu_emac_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static rt_ssize_t nu_emac_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static rt_err_t nu_emac_control(rt_device_t device, int cmd, void *args)
{
    nu_emac_t psNuEMAC = (nu_emac_t)device;
    RT_ASSERT(device != RT_NULL);

    switch (cmd)
    {
    case NIOCTL_GADDR:
        if (args) rt_memcpy(args, &psNuEMAC->mac_addr[0], 6);
        else return -RT_ERROR;
        break;

    default :
        break;
    }

    return RT_EOK;
}

rt_err_t nu_emac_tx(rt_device_t device, struct pbuf *p)
{
    rt_err_t ret = -RT_ERROR;
    s32 status;

    nu_emac_t psNuEMAC = (nu_emac_t)device;
    synopGMACNetworkAdapter *adapter;
    synopGMACdevice *emacdev;
    GMAC_MEMMGR_T *psemacmemmgr;

    RT_ASSERT(device);

    adapter = (synopGMACNetworkAdapter *) psNuEMAC->adapter;
    RT_ASSERT(adapter);

    emacdev = (synopGMACdevice *) adapter->m_gmacdev;
    RT_ASSERT(emacdev);

    psemacmemmgr = (GMAC_MEMMGR_T *)adapter->m_gmacmemmgr;
    RT_ASSERT(psemacmemmgr);

    if (!synopGMAC_is_desc_owned_by_dma(emacdev->TxNextDesc))
    {
        u32 offload_needed;
#if defined(RT_LWIP_USING_HW_CHECKSUM)
        offload_needed = 1;
#else
        offload_needed = 0;
#endif
        u32 index = emacdev->TxNext;
        u8 *pu8PktData = (u8 *)((u32)&psemacmemmgr->psTXFrames[index]);
        struct pbuf *q;
        rt_uint32_t offset = 0;

        LOG_D("Transmitting data(%08x-%d).", (u32)&psemacmemmgr->psTXFrames[index], p->tot_len);

        /* Copy to TX data buffer. */
        for (q = p; q != NULL; q = q->next)
        {
            rt_uint8_t *ptr = q->payload;
            rt_uint32_t len = q->len;
            rt_memcpy(&pu8PktData[offset], ptr, len);
            offset += len;
        }

        status = synopGMAC_xmit_frames(emacdev, (u8 *)&psemacmemmgr->psTXFrames[index], offset, offload_needed, 0);
        if (status != 0)
        {
            LOG_E("No More Free Tx skb");
            ret = -RT_ERROR;
            goto exit_nu_emac_tx;
        }
    }
    else
    {
        LOG_E("No avaialbe TX descriptor.");
        ret = -RT_ERROR;
        goto exit_nu_emac_tx;
    }

    ret = RT_EOK;

exit_nu_emac_tx:

    return ret;
}

void nu_emac_pbuf_free(struct pbuf *p)
{
    nu_emac_lwip_pbuf_t my_buf = (nu_emac_lwip_pbuf_t)p;
    s32 status;

    SYS_ARCH_DECL_PROTECT(old_level);
    SYS_ARCH_PROTECT(old_level);
    status = synopGMAC_set_rx_qptr(my_buf->emacdev, (u32)my_buf->psPktFrameDataBuf, PKT_FRAME_BUF_SIZE, 0);
    if (status < 0)
    {
        LOG_E("synopGMAC_set_rx_qptr: status < 0!!");
    }
    memp_free_pool(my_buf->pool, my_buf);
    SYS_ARCH_UNPROTECT(old_level);
}

struct pbuf *nu_emac_rx(rt_device_t device)
{
    nu_emac_t psNuEMAC = (nu_emac_t)device;
    synopGMACNetworkAdapter *adapter;
    synopGMACdevice *emacdev;
    struct pbuf *pbuf = RT_NULL;
    PKT_FRAME_T *psPktFrame;
    s32  s32PktLen;

    RT_ASSERT(device);

    adapter = psNuEMAC->adapter;
    RT_ASSERT(adapter);

    emacdev = (synopGMACdevice *) adapter->m_gmacdev;
    RT_ASSERT(emacdev);

    if ((s32PktLen = synop_handle_received_data(emacdev, &psPktFrame)) > 0)
    {
        nu_emac_lwip_pbuf_t my_pbuf  = (nu_emac_lwip_pbuf_t)memp_malloc_pool(psNuEMAC->memp_rx_pool);
        if (my_pbuf != RT_NULL)
        {
            my_pbuf->p.custom_free_function = nu_emac_pbuf_free;
            my_pbuf->psPktFrameDataBuf      = psPktFrame;
            my_pbuf->emacdev                = emacdev;
            my_pbuf->pool                   = psNuEMAC->memp_rx_pool;

            pbuf = pbuf_alloced_custom(PBUF_RAW,
                                       s32PktLen,
                                       PBUF_REF,
                                       &my_pbuf->p,
                                       psPktFrame,
                                       PKT_FRAME_BUF_SIZE);
            if (pbuf == RT_NULL)
            {
                LOG_E("failed to alloted %08x", pbuf);
            }
        }
        else
        {
            LOG_E("LWIP_MEMPOOL_ALLOC < 0!!");
        }
    }
    else
    {
        synopGMAC_enable_interrupt(emacdev, DmaIntEnable);
        goto exit_nu_emac_rx;
    }

exit_nu_emac_rx:

    return pbuf;
}

static void nu_emac_assign_macaddr(nu_emac_t psNuEMAC)
{
    static rt_uint32_t value = 0x94539452;
    uint32_t uid[4] = {0};

    /* Read UID from FMC for unique MAC address. */
    void nu_read_uid(uint32_t *id);
    nu_read_uid((uint32_t *)&uid);

    value = value ^ uid[0] ^ uid[1] ^ uid[2] ^ uid[3];

    LOG_I("UID: %08X-%08X-%08X-%08X, value: %08x", uid[0], uid[1], uid[2], uid[3], value);

    /* Assign MAC address */
    psNuEMAC->mac_addr[0] = 0x82;
    psNuEMAC->mac_addr[1] = 0x06;
    psNuEMAC->mac_addr[2] = 0x21;
    psNuEMAC->mac_addr[3] = (value >> 16) & 0xff;
    psNuEMAC->mac_addr[4] = (value >> 8) & 0xff;
    psNuEMAC->mac_addr[5] = (value) & 0xff;

    LOG_I("MAC address: %02X:%02X:%02X:%02X:%02X:%02X", \
          psNuEMAC->mac_addr[0], \
          psNuEMAC->mac_addr[1], \
          psNuEMAC->mac_addr[2], \
          psNuEMAC->mac_addr[3], \
          psNuEMAC->mac_addr[4], \
          psNuEMAC->mac_addr[5]);
    value++;
}

int32_t nu_emac_adapter_init(nu_emac_t psNuEMAC)
{
    synopGMACNetworkAdapter *adapter;

    RT_ASSERT(psNuEMAC != RT_NULL);


    /* Allocate net adapter */
    adapter = (synopGMACNetworkAdapter *)rt_malloc_align(sizeof(synopGMACNetworkAdapter), 4);
    RT_ASSERT(adapter != RT_NULL);
    rt_memset((void *)adapter, 0, sizeof(synopGMACNetworkAdapter));

    /* Allocate device */
    adapter->m_gmacdev = (synopGMACdevice *) rt_malloc_align(sizeof(synopGMACdevice), 4);
    RT_ASSERT(adapter->m_gmacdev != RT_NULL);
    rt_memset((char *)adapter->m_gmacdev, 0, sizeof(synopGMACdevice));

    /* Allocate memory management */
    adapter->m_gmacmemmgr = (GMAC_MEMMGR_T *) rt_malloc_align(sizeof(GMAC_MEMMGR_T), 4);
    RT_ASSERT(adapter->m_gmacmemmgr != RT_NULL);
    nu_memmgr_init(adapter->m_gmacmemmgr);

    /* Store adapter to priv */
    psNuEMAC->adapter = adapter;

    return 0;
}

int rt_hw_emac_init(void)
{
    int i;
    rt_err_t ret = RT_EOK;

    for (i = (EMAC_START + 1); i < EMAC_CNT; i++)
    {
        nu_emac_t psNuEMAC = (nu_emac_t)&nu_emac_arr[i];

        /* Register member functions */
        psNuEMAC->eth.parent.type       = RT_Device_Class_NetIf;
        psNuEMAC->eth.parent.init       = nu_emac_init;
        psNuEMAC->eth.parent.open       = nu_emac_open;
        psNuEMAC->eth.parent.close      = nu_emac_close;
        psNuEMAC->eth.parent.read       = nu_emac_read;
        psNuEMAC->eth.parent.write      = nu_emac_write;
        psNuEMAC->eth.parent.control    = nu_emac_control;
        psNuEMAC->eth.parent.user_data  = psNuEMAC;
        psNuEMAC->eth.eth_rx            = nu_emac_rx;
        psNuEMAC->eth.eth_tx            = nu_emac_tx;

        /* Set MAC address */
        nu_emac_assign_macaddr(psNuEMAC);

        /* Initial GMAC adapter */
        nu_emac_adapter_init(psNuEMAC);

        /* Initial zero_copy rx pool */
        memp_init_pool(psNuEMAC->memp_rx_pool);

        /* Register eth device */
        ret = eth_device_init(&psNuEMAC->eth, psNuEMAC->m_module.name);
        RT_ASSERT(ret == RT_EOK);
    }

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_emac_init);

#if 0
/*
    Remeber src += lwipiperf_SRCS in components\net\lwip-*\SConscript
*/
#include "lwip/apps/lwiperf.h"

static void
lwiperf_report(void *arg, enum lwiperf_report_type report_type,
               const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
               u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(local_addr);
    LWIP_UNUSED_ARG(local_port);

    rt_kprintf("IPERF report: type=%d, remote: %s:%d, total bytes: %"U32_F", duration in ms: %"U32_F", kbits/s: %"U32_F"\n",
               (int)report_type, ipaddr_ntoa(remote_addr), (int)remote_port, bytes_transferred, ms_duration, bandwidth_kbitpsec);
}

void lwiperf_example_init(void)
{
    lwiperf_start_tcp_server_default(lwiperf_report, NULL);
}
MSH_CMD_EXPORT(lwiperf_example_init, start lwip tcp server);
#endif
