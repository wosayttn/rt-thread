/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_SLCD_H__
#define __DRV_SLCD_H__

#include "drv_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

struct nu_slcd_pixel
{
    uint32_t m_u32Com;
    uint32_t m_u32Seg;
    uint32_t m_u32OnFlag;
};
typedef struct nu_slcd_pixel *nu_slcd_pixel_t;

typedef enum
{
    NU_SLCD_CMD_SET_LCD_CFG,       /* SLCD configuration */
    NU_SLCD_CMD_SET_CP_VOLTAGE,    /* Internal charge pump voltage */
    NU_SLCD_CMD_CNT
} NU_SLCD_CMD;

int rt_hw_slcd_init(void);

rt_err_t rt_hw_slcd_register(struct rt_device *device,
                             const char       *name,
                             rt_uint32_t       flag,
                             void             *data);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_SLCD_H__ */

