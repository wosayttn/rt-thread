/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_sys.h"
#if defined(SOC_SERIES_M2354)
#include "nu_rng.h"
#else
#include "nu_trng.h"
#endif

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG                 "drv.trng"
#define DBG_TAG                 LOG_TAG
#include "drv_log.h"

#define NU_CRYPTO_TRNG_NAME "nu_TRNG"

/* Types / Structures ---------------------------------------------------------*/

/* Static Function Prototypes ------------------------------------------------*/

/* Static Variables ----------------------------------------------------------*/

/* Functions Implementation --------------------------------------------------*/
rt_err_t nu_trng_init(void)
{
    CLK_EnableModuleClock(TRNG_MODULE);
    SYS_ResetModule(TRNG_RST);

#if defined(SOC_SERIES_M2354)
    RNG_Open();
#else
    TRNG_Open();
#endif

    return RT_EOK;
}

rt_uint32_t nu_trng_rand(struct hwcrypto_rng *ctx)
{
    uint32_t u32RNGValue = 0;

#if defined(SOC_SERIES_M2354)
    RNG_Random(&u32RNGValue, 1);
#else
    TRNG_GenWord(&u32RNGValue);
#endif

    return u32RNGValue;
}
