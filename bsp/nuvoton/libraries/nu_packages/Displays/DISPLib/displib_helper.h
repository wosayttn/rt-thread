/**************************************************************************//**
*
* @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2024-7-8        Wayne        First version
*
******************************************************************************/

#ifndef _DISPLIB_HELPER
#define _DISPLIB_HELPER

#include "rtthread.h"
#include "rtdevice.h"
#include "NuMicro.h"

//#define CONFIG_DISPLIB_DEBUG

typedef struct
{
    const char *szI2CBusName;
    rt_base_t RstPinIdx;
    rt_bool_t bRstActLow;
    void *user_data;
} S_DISPLIB_CTX;

rt_err_t displib_init(const S_DISPLIB_CTX *psDispCtx);
rt_err_t displib_fini(const S_DISPLIB_CTX *psDispCtx);

rt_err_t displib_set_dev_addr(rt_uint16_t u16Addr);
rt_err_t displib_i2c_write(rt_uint8_t reg, rt_uint8_t value);
rt_uint8_t displib_i2c_read(rt_uint8_t reg);

#endif /* _DISPLIB_HELPER */
