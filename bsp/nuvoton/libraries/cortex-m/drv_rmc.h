/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_RMC_H__
#define __DRV_RMC_H__

#include "drv_sys.h"

#if defined(RT_USING_FAL)
    #include "fal.h"

    extern const struct fal_flash_dev g_falRMC_AP;
    extern const struct fal_flash_dev g_falRMC_LD;

#if defined(RMC_DATA_FLASH_BASE)
    extern const struct fal_flash_dev g_falRMC_DF;
#endif

#endif

int nu_rmc_read(long offset, uint8_t *buf, size_t size);
int nu_rmc_write(long offset, const uint8_t *buf, size_t size);
int nu_rmc_erase(long offset, size_t size);

#endif // __DRV_RMC_H___
