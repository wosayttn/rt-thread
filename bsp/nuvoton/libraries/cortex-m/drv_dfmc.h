/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_DFMC_H__
#define __DRV_DFMC_H__

#include <rtthread.h>

#if defined(RT_USING_FAL)
    #include <fal.h>
    extern const struct fal_flash_dev g_falDFMC_DF;
    #if defined(BSP_USING_DFMC_EEPROM)
        extern const struct fal_flash_dev g_falDFMC_EEPROM;
    #endif
#endif

int nu_dfmc_read(long offset, uint8_t *buf, size_t size);
int nu_dfmc_write(long offset, const uint8_t *buf, size_t size);
int nu_dfmc_erase(long offset, size_t size);

#endif /* __DRV_DFMC_H__ */
