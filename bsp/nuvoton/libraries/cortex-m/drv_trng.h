/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_TRNG_H__
#define __DRV_TRNG_H__

#include <rtdevice.h>

rt_err_t nu_trng_init(void);
rt_uint32_t nu_trng_rand(struct hwcrypto_rng *ctx);

#endif
