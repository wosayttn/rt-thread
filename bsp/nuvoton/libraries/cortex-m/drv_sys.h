/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_SYS_H__
#define __DRV_SYS_H__

#include "rtdevice.h"
#include "NuMicro.h"
#include "nu_bitutil.h"
#include "drv_gpio.h"

struct nu_module
{
    char      *name;
    void      *base;
    uint32_t   RstId;
    uint32_t   ModId;
    IRQn_Type  eIRQn;
    IRQn_Type  eIRQn1;
} ;
typedef struct nu_module* nu_module_t;

#define NU_MFP_POS(PIN)   ((PIN % 4) * 8)
#define NU_MFP_MSK(PIN)   (0x1ful << NU_MFP_POS(PIN))

void nu_pin_func(rt_base_t pin, int data);
void nu_read_uid(uint32_t *id);
rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs);
rt_err_t rt_hw_qspi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin, rt_uint8_t data_line_width, void (*enter_qspi_mode)(), void (*exit_qspi_mode)());

#endif /* __DRV_SYS_H__ */
