/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_EBI_H__
#define __DRV_EBI_H__

#include "drv_sys.h"

/**
  * @brief      Initialize EBI for specify Bank
  *
  * @param[in]  u32Bank             Bank number for EBI. Valid values are:
  *                                     - \ref EBI_BANK0
  *                                     - \ref EBI_BANK1
  *                                     - \ref EBI_BANK2
  * @param[in]  u32DataWidth        Data bus width. Valid values are:
  *                                     - \ref EBI_BUSWIDTH_8BIT
  *                                     - \ref EBI_BUSWIDTH_16BIT
  * @param[in]  u32TimingClass      Default timing configuration. Valid values are:
  *                                     - \ref EBI_TIMING_FASTEST
  *                                     - \ref EBI_TIMING_VERYFAST
  *                                     - \ref EBI_TIMING_FAST
  *                                     - \ref EBI_TIMING_NORMAL
  *                                     - \ref EBI_TIMING_SLOW
  *                                     - \ref EBI_TIMING_VERYSLOW
  *                                     - \ref EBI_TIMING_SLOWEST
  * @param[in]  u32BusMode          Set EBI bus operate mode. Valid values are:
  *                                     - \ref EBI_OPMODE_NORMAL
  *                                     - \ref EBI_OPMODE_CACCESS
  *                                     - \ref EBI_OPMODE_ADSEPARATE
  * @param[in]  u32CSActiveLevel    CS is active High/Low. Valid values are:
  *                                     - \ref EBI_CS_ACTIVE_HIGH
  *                                     - \ref EBI_CS_ACTIVE_LOW
  *
  * @return     RT_EOK/RT_ERROR     Bank is used or not
  */
rt_err_t nu_ebi_init(uint32_t u32Bank, uint32_t u32DataWidth, uint32_t u32TimingClass, uint32_t u32BusMode, uint32_t u32CSActiveLevel);

/**
 * @brief Configure EBI timing parameters for LCD interface.
 *
 * Calculates and applies optimal EBI (External Bus Interface) timing parameters
 * to meet the LCD controller's access time requirements. Tests different MCLK
 * divisors to find the fastest clock that still meets timing constraints.
 *
 * @param acc_ns[in]       Access time requirement in nanoseconds
 * @param wr_idle_ns[in]   Write cycle idle time in nanoseconds
 * @param wr_ahd_ns[in]    Write address hold time in nanoseconds
 * @param rd_ahd_ns[in]    Read address hold time in nanoseconds
 * @param rd_idle_ns[in]   Read cycle idle time in nanoseconds
 *
 * @return     RT_EOK/RT_ERROR     Bank is used or not
 */
rt_err_t nu_ebi_apply_timing(uint32_t u32Bank,
                            int acc_ns,
                            int wr_idle_ns, int wr_ahd_ns,
                            int rd_ahd_ns, int rd_idle_ns);

#endif // __DRV_EBI_H___
