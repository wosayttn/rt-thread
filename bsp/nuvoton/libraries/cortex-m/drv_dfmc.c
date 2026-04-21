/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "rtconfig.h"

#if defined(BSP_USING_DFMC)

#include "NuMicro.h"
#include "drv_dfmc.h"
#include "rtdevice.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG                 "drv.dfmc"
#define DBG_TAG                 LOG_TAG
#include "drv_log.h"

#define NU_SUPPORT_NONALIGN

/* Get byte offset within a word (0, 8, 16, 24) */
#define NU_GETBYTE_OFST(addr)         (((addr)&0x3)*8)
/* Get word-aligned address (4-byte aligned) */
#define NU_GET_WALIGN(addr)           ((addr)&~0x3)
/* Get double-word-aligned address (8-byte aligned) */
#define NU_GET_DWALIGN(addr)          ((addr)&~0x7)
/* Get lowest 2 bits (for word alignment checks) */
#define NU_GET_LSB2BIT(addr)          ((addr)&0x3)
/* Get lowest 3 bits (for double-word alignment checks) */
#define NU_GET_LSB3BIT(addr)          ((addr)&0x7)

#define DEF_EEPROM_IS_DISABLED        (BIT5 | BIT1)

/* Types / Structures ---------------------------------------------------------*/

/* Static Function Prototypes ------------------------------------------------*/
#if defined(RT_USING_FAL)
    static int dataflash_read(long offset, uint8_t *buf, size_t size);
    static int dataflash_write(long offset, const uint8_t *buf, size_t size);
    static int dataflash_erase(long offset, size_t size);

    #if defined(BSP_USING_DFMC_EEPROM)
        static int eeprom_read(long offset, uint8_t *buf, size_t size);
        static int eeprom_write(long offset, const uint8_t *buf, size_t size);
        static int eeprom_erase(long offset, size_t size);
    #endif
#endif /* RT_USING_FAL */

/* Static Variables ----------------------------------------------------------*/
static rt_mutex_t g_mutex_dfmc = RT_NULL;

#if defined(RT_USING_FAL)
const struct fal_flash_dev g_falDFMC_DF =
{
    "DFMC_DF",
    DFMC_DATA_FLASH_BASE,
#if defined(BSP_USING_DFMC_EEPROM)
    /* Adjust size to exclude simulated EEPROM region */
    (DFMC_DATA_FLASH_SIZE - (DFMC_DATA_FLASH_END - DFMC_EEPROM_BASE)),
#else
    DFMC_DATA_FLASH_SIZE,
#endif
    DFMC_FLASH_PAGE_SIZE,
    {
        NULL,
        dataflash_read,
        dataflash_write,
        dataflash_erase
    }
};

#if defined(BSP_USING_DFMC_EEPROM)
const struct fal_flash_dev g_falDFMC_EEPROM =
{
    "DFMC_EEPROM",
    DFMC_EEPROM_BASE,
    DFMC_EEPROM_SIZE,
    1,
    {
        NULL,
        eeprom_read,
        eeprom_write,
        eeprom_erase    /* EEPROM does not require erase operation */
    }
};
#endif
#endif /* RT_USING_FAL */

/* Functions Implementation --------------------------------------------------*/
static int nu_dfmc_is_eeprom_region(long addr)
{
#if defined(BSP_USING_DFMC_EEPROM)
    if ((addr >= DFMC_EEPROM_BASE) && (addr < DFMC_EEPROM_END))
    {
        return 1;
    }
    else
#endif
    {
        return 0;
    }
}

static int nu_dfmc_is_df_region(long addr)
{
#if defined(BSP_USING_DFMC_EEPROM)
    if ((addr >= DFMC_DATA_FLASH_BASE) && (addr < DFMC_EEPROM_BASE))
#else
    if ((addr >= DFMC_DATA_FLASH_BASE) && (addr < DFMC_DATA_FLASH_END))
#endif
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static uint32_t nu_region_read(long addr)
{
    if (nu_dfmc_is_eeprom_region(addr))
    {
        return DFMC_EEPROM_Read(addr);
    }
    else if (nu_dfmc_is_df_region(addr))
    {
        return DFMC_Read(addr);
    }
    else
    {
        return 0xffffffff;
    }
}

static uint32_t nu_region_write(long addr, uint32_t data)
{
    if (nu_dfmc_is_eeprom_region(addr))
    {
        return DFMC_EEPROM_Write(addr, data);
    }
    else if (nu_dfmc_is_df_region(addr))
    {
        return DFMC_Write(addr, data);
    }
    else
    {
        return 0xffffffff;
    }
}

int nu_dfmc_read(long addr, uint8_t *buf, size_t size)
{
    size_t   read_size = 0;
    uint32_t addr_end  = addr + size;
    uint32_t isp_rdata = 0;
    uint32_t cur_word_addr = 0xFFFFFFFF;

    rt_mutex_take(g_mutex_dfmc, RT_WAITING_FOREVER);

    while ((uint32_t)addr < addr_end)
    {
        uint32_t word_addr = NU_GET_WALIGN(addr);

        if (word_addr != cur_word_addr)
        {
            isp_rdata = nu_region_read(word_addr);
            cur_word_addr = word_addr;
        }

        *buf = (uint8_t)(isp_rdata >> (NU_GET_LSB2BIT(addr) << 3));

        addr++;
        buf++;
        read_size++;
    }

    rt_mutex_release(g_mutex_dfmc);

    return read_size;
}

int nu_dfmc_write(long addr, const uint8_t *buf, size_t size)
{
    size_t   write_size = 0;
    uint32_t addr_end   = (uint32_t)addr + size;
    uint32_t isp_rdata  = 0;
    uint32_t cur_word_addr = 0xFFFFFFFF;

    rt_mutex_take(g_mutex_dfmc, RT_WAITING_FOREVER);

    DFMC_ENABLE_UPDATE();

    while ((uint32_t)addr < addr_end)
    {
        uint32_t word_addr = NU_GET_WALIGN(addr);
        uint32_t byte_ofs  = NU_GET_LSB2BIT(addr);

        if (word_addr != cur_word_addr)
        {
            if (cur_word_addr != 0xFFFFFFFF)
            {
                nu_region_write(cur_word_addr, isp_rdata);
            }

            isp_rdata = nu_region_read(word_addr);
            cur_word_addr = word_addr;
        }

        isp_rdata &= ~((uint32_t)0xFF << (byte_ofs << 3));
        isp_rdata |= ((uint32_t)(*buf) << (byte_ofs << 3));

        addr++;
        buf++;
        write_size++;
    }

    if (cur_word_addr != 0xFFFFFFFF)
    {
        nu_region_write(cur_word_addr, isp_rdata);
    }

    DFMC_DISABLE_UPDATE();
    rt_mutex_release(g_mutex_dfmc);

    return write_size;
}

int nu_dfmc_erase(long addr, size_t size)
{
    size_t erased_size = 0;
    uint32_t addrptr;
    uint32_t addr_end = addr + size;

#if defined(NU_SUPPORT_NONALIGN)
    uint8_t *page_sdtemp = RT_NULL;
    uint8_t *page_edtemp = RT_NULL;

    addrptr = addr & (DFMC_FLASH_PAGE_SIZE - 1);
    if (addrptr)
    {
        page_sdtemp = rt_malloc(addrptr);
        if (page_sdtemp == RT_NULL)
        {
            erased_size = 0;
            goto Exit3;
        }
        if (nu_dfmc_read(addr & ~(DFMC_FLASH_PAGE_SIZE - 1), page_sdtemp, addrptr) != addrptr)
        {
            erased_size = 0;
            goto Exit3;
        }
    }

    addrptr = addr_end & (DFMC_FLASH_PAGE_SIZE - 1);
    if (addrptr)
    {
        page_edtemp = rt_malloc(DFMC_FLASH_PAGE_SIZE - addrptr);
        if (page_edtemp == RT_NULL)
        {
            erased_size = 0;
            goto Exit3;
        }
        if (nu_dfmc_read(addr_end, page_edtemp, DFMC_FLASH_PAGE_SIZE - addrptr) != DFMC_FLASH_PAGE_SIZE - addrptr)
        {
            erased_size = 0;
            goto Exit3;
        }
    }
#endif

    rt_mutex_take(g_mutex_dfmc, RT_WAITING_FOREVER);

    DFMC_ENABLE_UPDATE();

    addrptr = (addr & ~(DFMC_FLASH_PAGE_SIZE - 1));
    while (addrptr < addr_end)
    {
        if (DFMC_Erase(addrptr) != RT_EOK)
        {
            goto Exit1;
        }

        erased_size += DFMC_FLASH_PAGE_SIZE;
        addrptr += DFMC_FLASH_PAGE_SIZE;
    }

Exit1:
    DFMC_DISABLE_UPDATE();
    rt_mutex_release(g_mutex_dfmc);

#if defined(NU_SUPPORT_NONALIGN)
    if (erased_size >= size)
    {
        addrptr = addr & (DFMC_FLASH_PAGE_SIZE - 1);
        if (addrptr)
        {
            if (nu_dfmc_write(addr & ~(DFMC_FLASH_PAGE_SIZE - 1), page_sdtemp, addrptr) != addrptr)
            {
                goto Exit3;
            }

            erased_size += addrptr;
        }
        addrptr = addr_end & (DFMC_FLASH_PAGE_SIZE - 1);
        if (addrptr)
        {
            if (nu_dfmc_write(addr_end, page_edtemp, DFMC_FLASH_PAGE_SIZE - addrptr) != DFMC_FLASH_PAGE_SIZE - addrptr)
            {
                goto Exit3;
            }

            erased_size += DFMC_FLASH_PAGE_SIZE - addrptr;
        }
    }
    else
    {
        erased_size = 0;
    }

Exit3:
    if (page_sdtemp != RT_NULL)
    {
        rt_free(page_sdtemp);
    }

    if (page_edtemp != RT_NULL)
    {
        rt_free(page_edtemp);
    }
#endif

    return erased_size;
}

#if defined(RT_USING_FAL)
static int dataflash_read(long offset, uint8_t *buf, size_t size)
{
    return nu_dfmc_read(g_falDFMC_DF.addr + offset, buf, size);
}

static int dataflash_write(long offset, const uint8_t *buf, size_t size)
{
    return nu_dfmc_write(g_falDFMC_DF.addr + offset, buf, size);
}

static int dataflash_erase(long offset, size_t size)
{
    return nu_dfmc_erase(g_falDFMC_DF.addr + offset, size);
}

#if defined(BSP_USING_DFMC_EEPROM)
static int eeprom_read(long offset, uint8_t *buf, size_t size)
{
    return nu_dfmc_read(g_falDFMC_EEPROM.addr + offset, buf, size);
}

static int eeprom_write(long offset, const uint8_t *buf, size_t size)
{
    return nu_dfmc_write(g_falDFMC_EEPROM.addr + offset, buf, size);
}

static int eeprom_erase(long offset, size_t size)
{
    return size;
}
#endif
#endif /* RT_USING_FAL */

static int nu_dfmc_init(void)
{
    uint32_t u32Config;

    DFMC_ENABLE_ISP();
    FMC_Open();

    u32Config = FMC_Read(FMC_USER_CONFIG_14);
    rt_kprintf("Before CONFIG14 0x%08X\n", u32Config);

#if defined(BSP_USING_DFMC_EEPROM)
    if ((u32Config & DEF_EEPROM_IS_DISABLED) == DEF_EEPROM_IS_DISABLED)
    {
        rt_kprintf("EEPROM is disabled. Please wait chip reset to enable it ...\n");
        FMC_ENABLE_CFG_UPDATE();
        FMC_WriteConfig(FMC_USER_CONFIG_14, u32Config & ~DEF_EEPROM_IS_DISABLED);
        FMC_DISABLE_CFG_UPDATE();
        SYS_ResetChip();
    }
#else
    if ((u32Config & ~DEF_EEPROM_IS_DISABLED) == 0)
    {
        rt_kprintf("EEPROM is enabled. Please wait chip reset to disable it ...\n");
        FMC_ENABLE_CFG_UPDATE();
        FMC_WriteConfig(FMC_USER_CONFIG_14, u32Config | DEF_EEPROM_IS_DISABLED);
        FMC_DISABLE_CFG_UPDATE();
        SYS_ResetChip();
    }
#endif

    u32Config = FMC_Read(FMC_USER_CONFIG_14);
    rt_kprintf("After CONFIG14 0x%08X\n", u32Config);

    FMC_Close();
    DFMC_Open();

#if defined(BSP_USING_DFMC_EEPROM)
    if (DFMC_EEPROM_Init() != DFMC_OK)
    {
        rt_kprintf("DFMC_EEPROM_Init failed!\n");
        RT_ASSERT(0);
    }
#endif

    g_mutex_dfmc = rt_mutex_create("nu_dfmc_lock", RT_IPC_FLAG_PRIO);
    RT_ASSERT(g_mutex_dfmc);

    return (int)RT_EOK;
}
INIT_DEVICE_EXPORT(nu_dfmc_init);

#endif /* BSP_USING_DFMC */
