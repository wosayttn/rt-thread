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

#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdint.h>

#if defined(__ARMCC_VERSION)
    extern int Image$$SRAM_BOUNDARY_START$$Base;
    extern int Image$$SRAM_BOUNDARY_END$$Base;
    #define SRAM_START     ((uintptr_t)&Image$$SRAM_BOUNDARY_START$$Base)
    #define SRAM_END       ((uintptr_t)&Image$$SRAM_BOUNDARY_END$$Base)
#elif defined(__GNUC__)
    extern int __sram_start__;
    extern int __sram_end__;
    #define SRAM_START     ((uintptr_t)&__sram_start__)
    #define SRAM_END       ((uintptr_t)&__sram_end__)
#else
    #error "Unsupported toolchain"
#endif

#if defined(__ARMCC_VERSION)
    extern int Image$$RW_RAM$$ZI$$Limit;
    #define HEAP_BEGIN      ((void *)&Image$$RW_RAM$$ZI$$Limit)
#elif __ICCARM__
    #pragma section="CSTACK"
    #define HEAP_BEGIN      (__segment_end("CSTACK"))
#else
    extern int __bss_end__;
    #define HEAP_BEGIN      ((void *)&__bss_end__)
#endif

#define HEAP_END        ((void *)SRAM_END)


void rt_hw_board_init(void);
void rt_hw_cpu_reset(void);

#endif /* BOARD_H_ */
