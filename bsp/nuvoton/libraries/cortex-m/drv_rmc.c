/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_rmc.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.rmc"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#define NU_SUPPORT_NONALIGN

/* Get byte offset within a word (0, 8, 16, 24) */
#define NU_GETBYTE_OFST(addr)         (((addr) & 0x3) * 8)
/* Get word-aligned address (4-byte aligned) */
#define NU_GET_WALIGN(addr)           ((addr) & ~0x3)
/* Get double-word-aligned address (8-byte aligned) */
#define NU_GET_DWALIGN(addr)          ((addr) & ~0x7)
/* Get lowest 2 bits (for word alignment checks) */
#define NU_GET_LSB2BIT(addr)          ((addr) & 0x3)
/* Get lowest 3 bits (for double-word alignment checks) */
#define NU_GET_LSB3BIT(addr)          ((addr) & 0x7)

/* Types / Structures ---------------------------------------------------------*/

/* Static Function Prototypes ------------------------------------------------*/
#if defined(RT_USING_FAL)
    static int aprom_read(long offset, uint8_t *buf, size_t size);
    static int aprom_write(long offset, const uint8_t *buf, size_t size);
    static int aprom_erase(long offset, size_t size);

    static int ldrom_read(long offset, uint8_t *buf, size_t size);
    static int ldrom_write(long offset, const uint8_t *buf, size_t size);
    static int ldrom_erase(long offset, size_t size);

#if defined(RMC_DATA_FLASH_BASE)
    static int dataflash_read(long offset, uint8_t *buf, size_t size);
    static int dataflash_write(long offset, const uint8_t *buf, size_t size);
    static int dataflash_erase(long offset, size_t size);
#endif

#endif  /* RT_USING_FAL */
static int nu_rmc_init(void);

/* Static Variables ----------------------------------------------------------*/
static rt_mutex_t g_mutex_rmc = RT_NULL;

#if defined(RT_USING_FAL)
const struct fal_flash_dev g_falRMC_AP =
{
    "RMC_AP",
    RMC_APROM_BASE,
    RMC_APROM_SIZE,
    RMC_FLASH_PAGE_SIZE,
    {
        NULL,
        aprom_read,
        aprom_write,
        aprom_erase
    }
};

const struct fal_flash_dev g_falRMC_LD =
{
    "RMC_LD",
    RMC_LDROM_BASE,
    RMC_LDROM_SIZE,
    RMC_FLASH_PAGE_SIZE,
    {
        NULL,
        ldrom_read,
        ldrom_write,
        ldrom_erase
    }
};

#if defined(RMC_DATA_FLASH_BASE)
const struct fal_flash_dev g_falRMC_DF =
{
    "RMC_DF",
    RMC_DATA_FLASH_BASE,
    RMC_DATA_FLASH_SIZE,
    RMC_FLASH_PAGE_SIZE,
    {
        NULL,
        dataflash_read,
        dataflash_write,
        dataflash_erase
    }
};
#endif

#endif  /* RT_USING_FAL */

/* Functions Implementation --------------------------------------------------*/

/**
 * @brief  Read data from RMC flash memory.
 * @param  addr: Start address to read from
 * @param  buf:  Buffer to store read data
 * @param  size: Number of bytes to read
 * @return Number of bytes actually read
 */
int nu_rmc_read(long addr, uint8_t *buf, size_t size)
{
    size_t read_size = 0;
    uint32_t addr_end = addr + size;
    uint32_t isp_rdata = 0;
    rt_mutex_take(g_mutex_rmc, RT_WAITING_FOREVER);

    // If address is not word-aligned, read the aligned word first
    if (NU_GET_LSB2BIT(addr))
        isp_rdata = RMC_Read(NU_GET_WALIGN(addr));

    for (; addr < addr_end ;)
    {
        if (NU_GET_LSB2BIT(addr) == 0)
        {
            isp_rdata = RMC_Read(addr);
            if (addr_end - addr >= 4)
            {
                *(uint32_t *)buf = isp_rdata;
                addr += 4;
                buf += 4;
                read_size += 4;
                continue;
            }
        }
        *buf = (uint8_t)(isp_rdata >> NU_GETBYTE_OFST(addr));
        addr++;
        buf++;
        read_size++;
    }

    rt_mutex_release(g_mutex_rmc);
    return read_size;
}

/**
 * @brief  Write data to RMC flash memory.
 * @param  addr: Start address to write to
 * @param  buf:  Buffer containing data to write
 * @param  size: Number of bytes to write
 * @return Number of bytes actually written
 */
int nu_rmc_write(long addr, const uint8_t *buf, size_t size)
{
    size_t write_size = 0;
    uint32_t addr_end = addr + size;
    uint32_t isp_rdata = 0;

    rt_mutex_take(g_mutex_rmc, RT_WAITING_FOREVER);

    // Enable APROM, LDROM or DATAFLASH update depending on address
    if (addr < RMC_APROM_END)
        RMC_ENABLE_AP_UPDATE();
    else if ((addr < RMC_LDROM_END) && addr >= RMC_LDROM_BASE)
        RMC_ENABLE_LD_UPDATE();
#if defined(RMC_DATA_FLASH_BASE)
    else if ((addr < RMC_DATA_FLASH_END) && addr >= RMC_DATA_FLASH_BASE)
        RMC_ENABLE_DF_UPDATE();
#endif
    else
        goto Exit2;
    // If address is not word-aligned, read the aligned word first
    if (NU_GET_LSB2BIT(addr))
        isp_rdata = RMC_Read(NU_GET_WALIGN(addr));

    for (; addr < addr_end ;)
    {
        if (addr_end - addr >= 4 && NU_GET_LSB2BIT(addr) == 0)
        {
            RMC_Write(addr, *((uint32_t *)buf));
            addr += 4;
            buf += 4;
            write_size += 4;
            continue;
        }

        if ((NU_GET_LSB2BIT(addr)) == 0x0)
            isp_rdata = RMC_Read(NU_GET_WALIGN(addr));

        isp_rdata = (isp_rdata & ~(0xFF << NU_GETBYTE_OFST(addr))) | ((*buf) << NU_GETBYTE_OFST(addr));

        if ((NU_GET_LSB2BIT(addr)) == 0x3)
            RMC_Write(NU_GET_WALIGN(addr), isp_rdata);

        addr++;
        buf++;
        write_size++;
    }
    if (NU_GET_LSB2BIT(addr))
        RMC_Write(NU_GET_WALIGN(addr), isp_rdata);

    RMC_DISABLE_AP_UPDATE();
    RMC_DISABLE_LD_UPDATE();
#if defined(RMC_DATA_FLASH_BASE)
    RMC_DISABLE_DF_UPDATE();
#endif

Exit2:

    rt_mutex_release(g_mutex_rmc);
    return write_size;
}

/**
 * @brief  Erase flash memory region.
 * @param  addr: Start address to erase
 * @param  size: Number of bytes to erase
 * @return Number of bytes actually erased
 */
int nu_rmc_erase(long addr, size_t size)
{
    size_t erased_size = 0;
    uint32_t addrptr;
    uint32_t addr_end = addr + size;
#if defined(NU_SUPPORT_NONALIGN)
    uint8_t *page_sdtemp = RT_NULL;
    uint8_t *page_edtemp = RT_NULL;

    // Save data before the first page boundary if not aligned
    addrptr = addr & (RMC_FLASH_PAGE_SIZE - 1);
    if (addrptr)
    {
        page_sdtemp = rt_malloc(addrptr);
        if (page_sdtemp == RT_NULL)
        {
            erased_size = 0;
            goto Exit3;
        }
        if (nu_rmc_read(addr & ~(RMC_FLASH_PAGE_SIZE - 1), page_sdtemp, addrptr) != addrptr)
        {
            erased_size = 0;
            goto Exit3;
        }
    }
    addrptr = addr_end & (RMC_FLASH_PAGE_SIZE - 1);
    if (addrptr)
    {
        page_edtemp = rt_malloc(RMC_FLASH_PAGE_SIZE - addrptr);
        if (page_edtemp == RT_NULL)
        {
            erased_size = 0;
            goto Exit3;
        }
        if (nu_rmc_read(addr_end, page_edtemp, RMC_FLASH_PAGE_SIZE - addrptr) != RMC_FLASH_PAGE_SIZE - addrptr)
        {
            erased_size = 0;
            goto Exit3;
        }
    }
#endif

    rt_mutex_take(g_mutex_rmc, RT_WAITING_FOREVER);

    // Enable APROM, LDROM or DATAFLASH update depending on address
    if (addr <= RMC_APROM_END)
        RMC_ENABLE_AP_UPDATE();
    else if ((addr < RMC_LDROM_END) && addr >= RMC_LDROM_BASE)
        RMC_ENABLE_LD_UPDATE();
#if defined(RMC_DATA_FLASH_BASE)
    else if ((addr < RMC_DATA_FLASH_END) && addr >= RMC_DATA_FLASH_BASE)
        RMC_ENABLE_DF_UPDATE();
#endif
    else
        goto Exit2;

    // Erase each page in the region
    addrptr = (addr & ~(RMC_FLASH_PAGE_SIZE - 1));
    while (addrptr < addr_end)
    {
        if (RMC_Erase(addrptr) != RT_EOK)
            goto Exit1;
        erased_size += RMC_FLASH_PAGE_SIZE;
        addrptr += RMC_FLASH_PAGE_SIZE;
    }

Exit1:

    RMC_DISABLE_AP_UPDATE();
    RMC_DISABLE_LD_UPDATE();
Exit2:

    rt_mutex_release(g_mutex_rmc);
#if defined(NU_SUPPORT_NONALIGN)
    // Restore saved data if needed
    if (erased_size >= size)
    {
        addrptr = addr & (RMC_FLASH_PAGE_SIZE - 1);
        if (addrptr)
        {
            if (nu_rmc_write(addr & ~(RMC_FLASH_PAGE_SIZE - 1), page_sdtemp, addrptr) != addrptr)
                goto Exit3;
            erased_size += addrptr;
        }
        addrptr = addr_end & (RMC_FLASH_PAGE_SIZE - 1);
        if (addrptr)
        {
            if (nu_rmc_write(addr_end, page_edtemp, RMC_FLASH_PAGE_SIZE - addrptr) != RMC_FLASH_PAGE_SIZE - addrptr)
                goto Exit3;
            erased_size += RMC_FLASH_PAGE_SIZE - addrptr;
        }
    }
    else
        erased_size = 0;

Exit3:
    if (page_sdtemp != RT_NULL)
        rt_free(page_sdtemp);
    if (page_edtemp != RT_NULL)
        rt_free(page_edtemp);
#endif
    return erased_size;
}
#if defined(RT_USING_FAL)

static int aprom_read(long offset, uint8_t *buf, size_t size)
{
    return nu_rmc_read(g_falRMC_AP.addr + offset, buf, size);
}

static int aprom_write(long offset, const uint8_t *buf, size_t size)
{
    return nu_rmc_write(g_falRMC_AP.addr + offset, buf, size);
}

static int aprom_erase(long offset, size_t size)
{
    return nu_rmc_erase(g_falRMC_AP.addr + offset, size);
}

static int ldrom_read(long offset, uint8_t *buf, size_t size)
{
    return nu_rmc_read(g_falRMC_LD.addr + offset, buf, size);
}

static int ldrom_write(long offset, const uint8_t *buf, size_t size)
{
    return nu_rmc_write(g_falRMC_LD.addr + offset, buf, size);
}

static int ldrom_erase(long offset, size_t size)
{
    return nu_rmc_erase(g_falRMC_LD.addr + offset, size);
}

#if defined(RMC_DATA_FLASH_BASE)
static int dataflash_read(long offset, uint8_t *buf, size_t size)
{
    return nu_rmc_read(g_falRMC_DF.addr + offset, buf, size);
}

static int dataflash_write(long offset, const uint8_t *buf, size_t size)
{
    return nu_rmc_write(g_falRMC_DF.addr + offset, buf, size);
}

static int dataflash_erase(long offset, size_t size)
{
    return nu_rmc_erase(g_falRMC_DF.addr + offset, size);
}
#endif

#endif /* RT_USING_FAL */

static int nu_rmc_init(void)
{
    RMC_ENABLE_ISP();

    g_mutex_rmc = rt_mutex_create("nu_rmc_lock", RT_IPC_FLAG_PRIO);
    RT_ASSERT(g_mutex_rmc);

#if defined(RT_USING_FAL)
    /* RT_USING_FAL */
    extern int fal_init_check(void);
    if (!fal_init_check())
        fal_init();
#endif

    return (int)RT_EOK;
}

INIT_APP_EXPORT(nu_rmc_init);
