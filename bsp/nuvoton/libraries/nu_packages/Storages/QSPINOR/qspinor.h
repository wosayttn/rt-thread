/**************************************************************************//**
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2025-10-30      Wayne        First version
*
******************************************************************************/

#ifndef __QSPINOR_H__
#define __QSPINOR_H__

#include <rtdevice.h>

#if defined(RT_USING_SFUD)
    #include "spi_flash.h"
    #include "spi_flash_sfud.h"
#endif

#include "drv_qspi.h"

void SpiFlash_EnterQspiMode(struct rt_qspi_device *qspi_device);
void SpiFlash_ExitQspiMode(struct rt_qspi_device *qspi_device);

#endif /* __QSPINOR_H__ */
