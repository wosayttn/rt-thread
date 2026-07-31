/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_sys.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.sys"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

/* Functions Implementation --------------------------------------------------*/

#if defined(SOC_SERIES_M480) || defined(SOC_SERIES_M2354)

void nu_pin_func(rt_base_t pin, int data)
{
    uint32_t pin_index      = NU_GET_PINS(pin);
    uint32_t port_index     = NU_GET_PORT(pin);
    __IO uint32_t *GPx_MFPx = ((__IO uint32_t *) &SYS->GPA_MFPL + (port_index * 2) + (pin_index / 8));
    uint32_t MFP_Msk        = NU_MFP_MSK(pin_index);

    *GPx_MFPx  = (*GPx_MFPx & (~MFP_Msk)) | data;
}

#else

void nu_pin_func(rt_base_t pin, int data)
{
    uint32_t GPx_MFPx_org;
    uint32_t pin_index      = NU_GET_PINS(pin);
    uint32_t port_index     = NU_GET_PORT(pin);
    __IO uint32_t *GPx_MFPx = ((__IO uint32_t *) &SYS->GPA_MFP0) + port_index * 4 + (pin_index / 4);
    uint32_t MFP_Msk        = NU_MFP_MSK(pin_index);

    GPx_MFPx_org = *GPx_MFPx;
    *GPx_MFPx    = (GPx_MFPx_org & (~MFP_Msk)) | data;
}

#endif

void nu_read_uid(uint32_t *id)
{
    /* Enable FMC ISP function */
    FMC_Open();

    /* Read Unique ID */
    id[0] = FMC_ReadUID(0);
    id[1] = FMC_ReadUID(1);
    id[2] = FMC_ReadUID(2);
    id[3] = 0;

    /* Disable FMC ISP function */
    FMC_Close();

}

void devmem(int argc, char *argv[])
{
    /**
     * @brief  This function may interact with critical system registers. Use with caution.
     *
     * @details
     * This function has the potential to access and modify important system registers
     * as part of its operation. Ensure proper validation and system state checks
     * before calling this function. Improper or careless usage may lead to system
     * instability or unintended behavior.
    */
    volatile unsigned int u32Addr;
    unsigned int value = 0, mode = 0;

    if (argc < 2 || argc > 3)
    {
        goto exit_devmem;
    }

    if (argc == 3)
    {
        if (rt_sscanf(argv[2], "0x%x", &value) != 1)
            goto exit_devmem;
        mode = 1; /*Write*/
    }

    if (rt_sscanf(argv[1], "0x%x", &u32Addr) != 1)
        goto exit_devmem;
    else if (!u32Addr || u32Addr & (4 - 1))
        goto exit_devmem;

    if (mode)
    {
        *((volatile uint32_t *)u32Addr) = value;
    }
    LOG_D("0x%08x", *((volatile uint32_t *)u32Addr));

    return;

exit_devmem:

    LOG_D("Read: devmem <physical address in hex>");
    LOG_D("Write: devmem <physical address in hex> <value in hex format>");

    return;
}
MSH_CMD_EXPORT(devmem, dump device registers);

#if defined(RT_USING_ULOG)
void devmem2(int argc, char *argv[])
{
    /**
     * @brief  This function may interact with critical system registers. Use with caution.
     *
     * @details
     * This function has the potential to access and modify important system registers
     * as part of its operation. Ensure proper validation and system state checks
     * before calling this function. Improper or careless usage may lead to system
     * instability or unintended behavior.
    */

    volatile unsigned int u32Addr;
    unsigned int value = 0, word_count = 1;

    if (argc < 2 || argc > 3)
    {
        goto exit_devmem;
    }

    if (argc == 3)
    {
        if (rt_sscanf(argv[2], "%u", &value) != 1)
            goto exit_devmem;
        word_count = value;
    }

    if (rt_sscanf(argv[1], "0x%x", &u32Addr) != 1)
        goto exit_devmem;
    else if (!u32Addr || u32Addr & (4 - 1))
        goto exit_devmem;

    if (word_count > 0)
    {
        LOG_HEX("devmem", 16, (void *)u32Addr, word_count * sizeof(rt_base_t));
    }

    return;

exit_devmem:

    LOG_D("devmem2: <physical address in hex> <count in dec>");

    return;
}
MSH_CMD_EXPORT(devmem2, dump device registers);
#endif

#if defined(RT_USING_SPI)
/**
  * Attach the spi device to SPI bus, this function must be used after initialization.
  */
rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs)
{
    rt_err_t result = RT_EOK;
    rt_base_t *cs_pin = RT_NULL;
    struct rt_spi_device *spi_device;

    RT_ASSERT(bus_name);
    RT_ASSERT(device_name);

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL)
    {
        LOG_E("no memory, spi bus attach device failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }
    rt_memset(spi_device, 0, sizeof(struct rt_spi_device));

    cs_pin = (rt_base_t *)rt_malloc(sizeof(rt_base_t));
    if (cs_pin == RT_NULL)
    {
        LOG_E("no memory, spi bus attach device failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }
    rt_memset(cs_pin, 0, sizeof(rt_base_t));

    *cs_pin = cs;
    rt_pin_mode(cs, PIN_MODE_OUTPUT);
    rt_pin_write(cs, PIN_HIGH);

    result = rt_spi_bus_attach_device(spi_device, device_name, bus_name, (void *)cs_pin);
    if(result != RT_EOK)
    {
        LOG_E("spi bus attach device failed!");
    }

__exit:

    if (result != RT_EOK)
    {
        if (spi_device)
        {
            rt_free(spi_device);
        }
        if (cs_pin)
        {
            rt_free(cs_pin);
        }
    }

    return  result;
}
#endif

#if defined(RT_USING_QSPI)
/**
  * Attach the qspi device to QSPI bus, this function must be used after initialization.
  */
rt_err_t rt_hw_qspi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin, rt_uint8_t data_line_width, void (*enter_qspi_mode)(), void (*exit_qspi_mode)())
{
    struct rt_qspi_device *qspi_device = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);
    RT_ASSERT(data_line_width == 1 || data_line_width == 2 || data_line_width == 4);

    qspi_device = (struct rt_qspi_device *)rt_malloc(sizeof(struct rt_qspi_device));
    if (qspi_device == RT_NULL)
    {
        LOG_E("no memory, qspi bus attach device failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }

    rt_memset(qspi_device, 0, sizeof(struct rt_qspi_device));

    qspi_device->enter_qspi_mode = enter_qspi_mode;
    qspi_device->exit_qspi_mode = exit_qspi_mode;
    qspi_device->config.qspi_dl_width = data_line_width;

    result = rt_spi_bus_attach_device_cspin(&qspi_device->parent, device_name, bus_name, cs_pin, RT_NULL);

__exit:
    if (result != RT_EOK)
    {
        if (qspi_device)
        {
            rt_free(qspi_device);
        }
    }

    return  result;
}
#endif