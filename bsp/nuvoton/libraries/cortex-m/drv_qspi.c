/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_spi.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.qspi"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#if defined(BSP_USING_SPI_PDMA)

#if defined(BSP_USING_QSPI0_PDMA)
#define QSPI0_PDMA_INIT                 \
    .pdma_perp_tx = PDMA_QSPI0_TX,      \
    .pdma_perp_rx = PDMA_QSPI0_RX,
#else
#define QSPI0_PDMA_INIT                 \
    .pdma_perp_tx = NU_PDMA_UNUSED,     \
    .pdma_perp_rx = NU_PDMA_UNUSED,
#endif

#if defined(BSP_USING_QSPI1_PDMA)
#define QSPI1_PDMA_INIT                 \
    .pdma_perp_tx = PDMA_QSPI1_TX,      \
    .pdma_perp_rx = PDMA_QSPI1_RX,
#else
#define QSPI1_PDMA_INIT                 \
    .pdma_perp_tx = NU_PDMA_UNUSED,     \
    .pdma_perp_rx = NU_PDMA_UNUSED,
#endif

#else
    #define QSPI0_PDMA_INIT
    #define QSPI1_PDMA_INIT
#endif

#define DEFINE_NU_QSPI(_idx, _pdma_init) \
    {                                     \
        .m_module = {                     \
            .name = "qspi" #_idx,        \
            .base = (SPI_T *)QSPI##_idx,  \
            .RstId = QSPI##_idx##_RST,    \
        },                                \
        _pdma_init                        \
    }


/* Types / Structures ---------------------------------------------------------*/
enum
{
    QSPI_START = -1,
#if defined(BSP_USING_QSPI0)
    QSPI0_IDX,
#endif
#if defined(BSP_USING_QSPI1)
    QSPI1_IDX,
#endif
    QSPI_CNT
};

/* Static Function Prototypes ------------------------------------------------*/
static rt_err_t nu_qspi_bus_configure(struct rt_spi_device *device, struct rt_spi_configuration *configuration);
static rt_ssize_t nu_qspi_bus_xfer(struct rt_spi_device *device, struct rt_spi_message *message);
static int nu_qspi_register_bus(struct nu_spi *qspi_bus, const char *name);

/* Static Variables ----------------------------------------------------------*/
static struct rt_spi_ops nu_qspi_poll_ops =
{
    .configure = nu_qspi_bus_configure,
    .xfer      = nu_qspi_bus_xfer,
};

static struct nu_spi nu_qspi_arr [] =
{
#if defined(BSP_USING_QSPI0)
    DEFINE_NU_QSPI(0, QSPI0_PDMA_INIT),
#endif
#if defined(BSP_USING_QSPI1)
    DEFINE_NU_QSPI(1, QSPI1_PDMA_INIT),
#endif
}; /* qspi nu_qspi */

/* Functions Implementation --------------------------------------------------*/
/**
  * @brief Get QSPI data width.
  * @param[in] qspi       Pointer to QSPI module.
  * @return The current data width of the QSPI module.
  */
static uint32_t QSPI_GetDataWidth(QSPI_T *qspi)
{
    uint32_t u32RegWidth = (qspi->CTL & QSPI_CTL_DWIDTH_Msk) >> QSPI_CTL_DWIDTH_Pos;

    if (u32RegWidth == 0UL)
    {
        return 32UL;
    }

    return u32RegWidth;
}

static rt_err_t nu_qspi_bus_configure(struct rt_spi_device *device,
                                      struct rt_spi_configuration *configuration)
{
    struct nu_spi *spi_bus;
    struct rt_qspi_configuration *qspi_cfg;
    rt_uint32_t u32SPIMode;
    rt_uint32_t u32BusClock;
    rt_err_t ret = RT_EOK;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    spi_bus = (struct nu_spi *) device->bus;

    /* rt_qspi_configure() stores the settings in rt_qspi_device.config, not in
       the rt_spi_device.config passed in here. Use the actual QSPI configuration. */
    qspi_cfg = &((struct rt_qspi_device *)device)->config;

    /* Check mode */
    switch (qspi_cfg->parent.mode & RT_SPI_MODE_3)
    {
    case RT_SPI_MODE_0:
        u32SPIMode = SPI_MODE_0;
        break;
    case RT_SPI_MODE_1:
        u32SPIMode = SPI_MODE_1;
        break;
    case RT_SPI_MODE_2:
        u32SPIMode = SPI_MODE_2;
        break;
    case RT_SPI_MODE_3:
        u32SPIMode = SPI_MODE_3;
        break;
    default:
        ret = RT_EIO;
        goto exit_nu_qspi_bus_configure;
    }
    if (!(qspi_cfg->parent.data_width == 8  ||
            qspi_cfg->parent.data_width == 16 ||
            qspi_cfg->parent.data_width == 24 ||
            qspi_cfg->parent.data_width == 32))
    {
        ret = RT_EINVAL;
        goto exit_nu_qspi_bus_configure;
    }
    u32BusClock = QSPI_SetBusClock((QSPI_T *)spi_bus->m_module.base, qspi_cfg->parent.max_hz);
    if (qspi_cfg->parent.max_hz > u32BusClock)
    {
        LOG_W("%s clock max frequency is %dHz ( != %dHz)", spi_bus->m_module.name, u32BusClock, qspi_cfg->parent.max_hz);
        qspi_cfg->parent.max_hz = u32BusClock;
    }

    QSPI_Open((QSPI_T *)spi_bus->m_module.base, SPI_MASTER, u32SPIMode, qspi_cfg->parent.data_width, u32BusClock);

    if (qspi_cfg->parent.mode & RT_SPI_CS_HIGH)
    {
        /* Set CS pin to LOW */
        SPI_SET_SS_LOW((SPI_T *)spi_bus->m_module.base);
    }
    else
    {
        /* Set CS pin to HIGH */
         SPI_SET_SS_HIGH((SPI_T *)spi_bus->m_module.base);
    }

    if (qspi_cfg->parent.mode & RT_SPI_MSB)
    {
        /* Set sequence to MSB first */
        SPI_SET_MSB_FIRST((SPI_T *)spi_bus->m_module.base);
    }
    else
    {
        /* Set sequence to LSB first */
        SPI_SET_LSB_FIRST((SPI_T *)spi_bus->m_module.base);
    }

    nu_spi_drain_rxfifo((SPI_T *)spi_bus->m_module.base);

exit_nu_qspi_bus_configure:

    return -(ret);
}

static int nu_qspi_mode_config(struct nu_spi *qspi_bus, rt_uint8_t *tx, rt_uint8_t *rx, int qspi_lines)
{
    QSPI_T *qspi_base = (QSPI_T *)qspi_bus->m_module.base;
    if (qspi_lines > 1)
    {
        if (tx)
        {
            switch (qspi_lines)
            {
            case 2:
                QSPI_ENABLE_DUAL_OUTPUT_MODE(qspi_base);
                break;
            case 4:
                QSPI_ENABLE_QUAD_OUTPUT_MODE(qspi_base);
                break;
            default:
                LOG_E("Data line is not supported.");
                break;
            }
        }
        else
        {
            switch (qspi_lines)
            {
            case 2:
                QSPI_ENABLE_DUAL_INPUT_MODE(qspi_base);
                break;
            case 4:
                QSPI_ENABLE_QUAD_INPUT_MODE(qspi_base);
                break;
            default:
                LOG_E("Data line is not supported.");
                break;
            }
        }
    }
    else
    {
        QSPI_DISABLE_DUAL_MODE(qspi_base);
        QSPI_DISABLE_QUAD_MODE(qspi_base);
    }
    return qspi_lines;
}

static rt_ssize_t nu_qspi_bus_xfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    struct nu_spi *qspi_bus;
    struct rt_qspi_configuration *qspi_configuration;
    struct rt_qspi_message *qspi_message;
    rt_uint8_t u8last = 1;
    rt_uint8_t bytes_per_word;
    QSPI_T *qspi_base;
    rt_uint32_t u32len = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    qspi_bus = (struct nu_spi *) device->bus;
    qspi_base = (QSPI_T *)qspi_bus->m_module.base;
    qspi_configuration = &((struct rt_qspi_device *)device)->config;

    bytes_per_word = QSPI_GetDataWidth(qspi_base) / 8;

    if (message->cs_take && !(qspi_configuration->parent.mode & RT_SPI_NO_CS))
    {
        if (qspi_configuration->parent.mode & RT_SPI_CS_HIGH)
        {
            QSPI_SET_SS_HIGH(qspi_base);
        }
        else
        {
            QSPI_SET_SS_LOW(qspi_base);
        }
    }

    qspi_message = (struct rt_qspi_message *)message;

    /* Command + Address + Dummy + Data */
    /* Command stage */
    if (qspi_message->instruction.content != 0)
    {
        u8last = nu_qspi_mode_config(qspi_bus, (rt_uint8_t *) &qspi_message->instruction.content, RT_NULL, qspi_message->instruction.qspi_lines);
        nu_spi_transfer((struct nu_spi *)qspi_bus,
                        (rt_uint8_t *) &qspi_message->instruction.content,
                        RT_NULL,
                        1,
                        1);
    }
    if (qspi_message->address.size > 0)
    {
        rt_uint32_t u32ReversedAddr = 0;
        rt_uint32_t u32AddrNumOfByte = qspi_message->address.size / 8;
        switch (u32AddrNumOfByte)
        {
        case 1:
            u32ReversedAddr = (qspi_message->address.content & 0xff);
            break;
        case 2:
            nu_set16_be((rt_uint8_t *)&u32ReversedAddr, qspi_message->address.content);
            break;
        case 3:
            nu_set24_be((rt_uint8_t *)&u32ReversedAddr, qspi_message->address.content);
            break;
        case 4:
            nu_set32_be((rt_uint8_t *)&u32ReversedAddr, qspi_message->address.content);
            break;
        default:
            RT_ASSERT(0);
            break;
        }
        u8last = nu_qspi_mode_config(qspi_bus, (rt_uint8_t *)&u32ReversedAddr, RT_NULL, qspi_message->address.qspi_lines);
        nu_spi_transfer((struct nu_spi *)qspi_bus,
                        (rt_uint8_t *) &u32ReversedAddr,
                        RT_NULL,
                        u32AddrNumOfByte,
                        1);
    }
    if ((qspi_message->alternate_bytes.size > 0) && (qspi_message->alternate_bytes.size <= 4))
    {
        rt_uint32_t u32AlternateByte = 0;
        rt_uint32_t u32NumOfByte = qspi_message->alternate_bytes.size / 8;
        switch (u32NumOfByte)
        {
        case 1:
            u32AlternateByte = (qspi_message->alternate_bytes.content & 0xff);
            break;
        case 2:
            nu_set16_be((rt_uint8_t *)&u32AlternateByte, qspi_message->alternate_bytes.content);
            break;
        case 3:
            nu_set24_be((rt_uint8_t *)&u32AlternateByte, qspi_message->alternate_bytes.content);
            break;
        case 4:
            nu_set32_be((rt_uint8_t *)&u32AlternateByte, qspi_message->alternate_bytes.content);
            break;
        default:
            RT_ASSERT(0);
            break;
        }
        u8last = nu_qspi_mode_config(qspi_bus, (rt_uint8_t *)&u32AlternateByte, RT_NULL, qspi_message->alternate_bytes.qspi_lines);
        nu_spi_transfer((struct nu_spi *)qspi_bus,
                        (rt_uint8_t *) &u32AlternateByte,
                        RT_NULL,
                        u32NumOfByte,
                        1);
    }
    if (qspi_message->dummy_cycles > 0)
    {
        qspi_bus->dummy[0] = 0x00;

        u8last = nu_qspi_mode_config(qspi_bus, (rt_uint8_t *) &qspi_bus->dummy[0], RT_NULL, u8last);
        nu_spi_transfer((struct nu_spi *)qspi_bus,
                        (rt_uint8_t *) &qspi_bus->dummy[0],
                        RT_NULL,
                        qspi_message->dummy_cycles / (8 / u8last),
                        1);
    }

    if (message->length > 0)
    {
        /* Data stage */
        nu_qspi_mode_config(qspi_bus, (rt_uint8_t *) message->send_buf, (rt_uint8_t *) message->recv_buf, qspi_message->qspi_data_lines);
        nu_spi_transfer((struct nu_spi *)qspi_bus,
                        (rt_uint8_t *) message->send_buf,
                        (rt_uint8_t *) message->recv_buf,
                        message->length,
                        bytes_per_word);
        u32len = message->length;
    }
    else
    {
        u32len = 1;
    }

    if (message->cs_release && !(qspi_configuration->parent.mode & RT_SPI_NO_CS))
    {
        if (qspi_configuration->parent.mode & RT_SPI_CS_HIGH)
        {
            QSPI_SET_SS_LOW(qspi_base);
        }
        else
        {
            QSPI_SET_SS_HIGH(qspi_base);
        }
    }

    return u32len;
}

static int nu_qspi_register_bus(struct nu_spi *qspi_bus, const char *name)
{
    return rt_qspi_bus_register(&qspi_bus->dev, name, &nu_qspi_poll_ops);
}

/**
 * Hardware SPI Initial
 */
static int rt_hw_qspi_init(void)
{
    rt_uint8_t i;

    for (i = (QSPI_START + 1); i < QSPI_CNT; i++)
    {
        SYS_ResetModule(nu_qspi_arr[i].m_module.RstId);

        nu_qspi_arr[i].dummy = rt_malloc_align(RT_ALIGN_SIZE, RT_ALIGN_SIZE);
        RT_ASSERT(nu_qspi_arr[i].dummy);
#if defined(BSP_USING_SPI_PDMA)
        nu_qspi_arr[i].pdma_chanid_tx = -1;
        nu_qspi_arr[i].pdma_chanid_rx = -1;
#endif
#if defined(BSP_USING_QSPI_PDMA)
        if ((nu_qspi_arr[i].pdma_perp_tx != NU_PDMA_UNUSED) && (nu_qspi_arr[i].pdma_perp_rx != NU_PDMA_UNUSED))
        {
            if (nu_hw_spi_pdma_allocate(&nu_qspi_arr[i]) != RT_EOK)
            {
                LOG_E("Failed to allocate DMA channels for %s. We will use poll-mode for this bus.", nu_qspi_arr[i].m_module.name);
            }
        }
#endif
        nu_qspi_register_bus(&nu_qspi_arr[i], nu_qspi_arr[i].m_module.name);
    }

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_qspi_init);