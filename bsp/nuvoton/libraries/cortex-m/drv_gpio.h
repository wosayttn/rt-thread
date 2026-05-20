/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include "NuMicro.h"

typedef enum
{
    NU_PA,
    NU_PB,
    NU_PC,
    NU_PD,
    NU_PE,
    NU_PF,
    NU_PG,
#if defined(GPIOH_BASE)
    NU_PH,
#endif
#if defined(GPIOI_BASE)
    NU_PI,
#endif
#if defined(GPIOJ_BASE)
    NU_PJ,
#endif
#if defined(GPIOK_BASE)
    NU_PK,
#endif
#if defined(GPIOL_BASE)
    NU_PL,
#endif
#if defined(GPIOM_BASE)
    NU_PM,
#endif
#if defined(GPION_BASE)
    NU_PN,
#endif
    NU_PORT_CNT,
} nu_gpio_port;

#define NU_GET_PININDEX(port, pin)        ((port)*16+(pin))
#define NU_GET_PINS(rt_pin_index)         ((rt_pin_index) & 0x0000000F)
#define NU_GET_PORT(rt_pin_index)         (((rt_pin_index)>>4) & 0x0000000F)
#define NU_GET_PIN_MASK(nu_gpio_pin)      (1 << (nu_gpio_pin))
#define NU_PIN_UNUSED                     0xFFFFFFFF

#endif /* __DRV_GPIO_H__ */
