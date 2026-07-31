/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __QSPINOR_H__
#define __QSPINOR_H__

#include <rtdevice.h>

#if defined(RT_USING_SFUD)
    #include "dev_spi_flash.h"
    #include "dev_spi_flash_sfud.h"
#endif

#include "drv_qspi.h"

void SpiFlash_EnterQspiMode(struct rt_qspi_device *qspi_device);
void SpiFlash_ExitQspiMode(struct rt_qspi_device *qspi_device);
rt_err_t SpiFlash_WriteDisable(struct rt_qspi_device *qspi_device);
rt_err_t SpiFlash_Unprotect(struct rt_qspi_device *qspi_device);
rt_err_t SpiFlash_Protect(struct rt_qspi_device *qspi_device);
rt_bool_t SpiFlash_IsWriteEnabled(struct rt_qspi_device *qspi_device);
rt_uint8_t SpiFlash_GetStatus1(struct rt_qspi_device *qspi_device);

#endif /* __QSPINOR_H__ */
