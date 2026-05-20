/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_ebi.h"
#include "nu_miscutil.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.ebi"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#define MAX_BANK    EBI_BANK2

/* Static Variables ----------------------------------------------------------*/
static uint8_t nu_ebi_bank_mask = 0;

/* Functions Implementation --------------------------------------------------*/
rt_err_t nu_ebi_init(uint32_t u32Bank,
                     uint32_t u32DataWidth,
                     uint32_t u32TimingClass,
                     uint32_t u32BusMode,
                     uint32_t u32CSActiveLevel)
{
    if (u32Bank > MAX_BANK)
        return -(RT_ERROR);

    /* Check this bank is not used */
    if ((1 << u32Bank) & nu_ebi_bank_mask)
        return -(RT_ERROR);

    /* Initialize EBI */
    EBI_Open(u32Bank, u32DataWidth, u32TimingClass, u32BusMode, u32CSActiveLevel);
    EBI_ENABLE_WRITE_BUFFER();

    nu_ebi_bank_mask |= (1 << u32Bank);

    return RT_EOK;
}

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
 * @return 0 on success, -1 if no valid timing configuration found
 */
rt_err_t nu_ebi_apply_timing(uint32_t u32Bank,
                    int acc_ns,
                    int wr_idle_ns, int wr_ahd_ns,
                    int rd_ahd_ns, int rd_idle_ns)
{
    /* Macros for timing calculations */
#define TAHD_MAX(a, b)   ((a) > (b) ? (a) : (b))
#define TASU_CYCLE       (1)    /* Address setup time in cycles */

    /* Try from fastest MCLK (Div 0) to slowest (Div 8) */
    for (int i32MCLKDiv = EBI_MCLKDIV_1; i32MCLKDiv <= EBI_MCLKDIV_8; i32MCLKDiv++)
    {
        /* Calculate actual EBI master clock frequency for this divisor */
        double fEBI_MCLK_hz = (double)CLK_GetHCLKFreq() / (i32MCLKDiv + 1);

        /* Calculate required timing in EBI clock cycles */
        uint32_t TACC = ns_to_cycles_ceil(acc_ns,     fEBI_MCLK_hz) - TASU_CYCLE;
        uint32_t W2X  = ns_to_cycles_ceil(wr_idle_ns, fEBI_MCLK_hz);
        uint32_t TAHD = ns_to_cycles_ceil(TAHD_MAX(wr_ahd_ns, rd_ahd_ns), fEBI_MCLK_hz);
        uint32_t R2R  = ns_to_cycles_ceil(rd_idle_ns, fEBI_MCLK_hz);

        /* Debug: Print EBI clock information */
        LOG_I("EBI_MCLK_hz: %f", fEBI_MCLK_hz);
        LOG_I("EBI_MCLK_ns: %f", hz_to_ns(fEBI_MCLK_hz));
        LOG_D("acc_ns: %d ns", acc_ns);
        LOG_D("wr_idle_ns: %d ns", wr_idle_ns);
        LOG_D("wr_ahd_ns: %d ns", wr_ahd_ns);
        LOG_D("rd_ahd_ns: %d ns", rd_ahd_ns);
        LOG_D("rd_idle_ns: %d ns", rd_idle_ns);
        LOG_I("Calculated: TACC:%d, W2X:%d, TAHD:%d, R2R:%d", TACC, W2X, TAHD, R2R);

        // Hardware register range check
        if ((TACC > 31) || (W2X > 15) || (TAHD > 7) || (R2R > 15)) continue;

        uint32_t WAHDOFF = (wr_ahd_ns == 0) ? 1 : 0;
        uint32_t RAHDOFF = (rd_ahd_ns == 0) ? 1 : 0;

        EBI_SetBusTiming(u32Bank,
                         (RAHDOFF << EBI_TCTL_RAHDOFF_Pos) |
                         (WAHDOFF << EBI_TCTL_WAHDOFF_Pos) |
                         (W2X      << EBI_TCTL_W2X_Pos)    |
                         (R2R      << EBI_TCTL_R2R_Pos)    |
                         (TAHD     << EBI_TCTL_TAHD_Pos)   |
                         (TACC     << EBI_TCTL_TACC_Pos),
                         i32MCLKDiv);

        LOG_I("Applied: TACC:%d, W2X:%d, TAHD:%d, R2R:%d", TACC, W2X, TAHD, R2R);
        LOG_D("Verify EBI->CTL0: 0x%08X", EBI->CTL0);
        LOG_D("Verify EBI->TCTL0: 0x%08X", EBI->TCTL0);

        return RT_EOK;
    }

    return -(RT_ERROR);
}
