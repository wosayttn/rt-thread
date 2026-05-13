/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include <rtconfig.h>

#if (defined(BSP_USING_TRNG) && defined(RT_HWCRYPTO_USING_RNG))

#include "NuMicro.h"
#include <rtdevice.h>

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

    TRNG_Open();

    return RT_EOK;
}

rt_uint32_t nu_trng_rand(struct hwcrypto_rng *ctx)
{
    uint32_t u32RNGValue;

    TRNG_GenWord(&u32RNGValue);

    return u32RNGValue;
}

#endif //#if (defined(BSP_USING_TRNG) && defined(RT_HWCRYPTO_USING_RNG))
