/**************************************************************************//**
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2020-1-16       Wayne        First version
*
******************************************************************************/

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include "rtconfig.h"
#include "board.h"
#include "fal.h"
#include "drv_rmc.h"

/* ===================== Flash device Configuration ========================= */
#if defined(FAL_PART_HAS_TABLE_CFG)

/* ---------------- RMC ---------------- */
#if defined(BSP_USING_RMC)
#define IFDEF_BOARD_USING_RMC   &g_falRMC_AP, &g_falRMC_LD,
#define FAL_PART_RMC  \
            {FAL_PART_MAGIC_WORD, "LDROM",     "RMC_LD",  0x0,    (8*1024),   0},  \
            {FAL_PART_MAGIC_WORD, "APROM",     "RMC_AP",  0x0,    (512*1024), 0},
#else
#define IFDEF_BOARD_USING_RMC
#define FAL_PART_RMC
#endif

/* ---------------- Device Table ---------------- */
#if defined(BSP_USING_RMC)
#define FAL_FLASH_DEV_TABLE     \
        {                               \
            IFDEF_BOARD_USING_RMC       \
        }
#else
#define FAL_FLASH_DEV_TABLE { }
#endif

/* ---------------- Partition Table ---------------- */
#if defined(BSP_USING_RMC)
#define FAL_PART_TABLE          \
        {                               \
            FAL_PART_RMC                \
        }
#else
#define FAL_PART_TABLE { }

#endif

#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
