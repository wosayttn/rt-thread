/**************************************************************************//**
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2020-3-03       FYChou       First version
*
******************************************************************************/

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>
#include "fal.h"
#include "drv_fmc.h"

/* ===================== Flash device Configuration ========================= */
#if defined(FAL_PART_HAS_TABLE_CFG)

/* ---------------- FMC ---------------- */
#if defined(BSP_USING_FMC)
#define IFDEF_BOARD_USING_FMC   &g_falFMC_AP, &g_falFMC_LD,
#define FAL_PART_FMC  \
            {FAL_PART_MAGIC_WORD, "LDROM",     "FMC_LD",  0x0,    (8*1024),   0},  \
            {FAL_PART_MAGIC_WORD, "APROM",     "FMC_AP",  0x0,    (512*1024), 0},
#else
#define IFDEF_BOARD_USING_FMC
#define FAL_PART_FMC
#endif

/* ---------------- Device Table ---------------- */
#if defined(BSP_USING_FMC)
#define FAL_FLASH_DEV_TABLE     \
        {                               \
            IFDEF_BOARD_USING_FMC       \
        }
#else
#define FAL_FLASH_DEV_TABLE { }
#endif

/* ---------------- Partition Table ---------------- */
#if defined(BSP_USING_FMC)
#define FAL_PART_TABLE          \
        {                               \
            FAL_PART_FMC                \
        }
#else
#define FAL_PART_TABLE { }
#endif

#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
