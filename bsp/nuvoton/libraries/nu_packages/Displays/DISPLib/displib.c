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

#include "displib.h"

int displib_lt8618_init(const S_DISPLIB_CTX *psDispCtx);
int displib_lt8718_init(const S_DISPLIB_CTX *psDispCtx);
int displib_lt8918_init(const S_DISPLIB_CTX *psDispCtx);

int displib_lt8618_fini(const S_DISPLIB_CTX *psDispCtx);
int displib_lt8718_fini(const S_DISPLIB_CTX *psDispCtx);
int displib_lt8918_fini(const S_DISPLIB_CTX *psDispCtx);

static const S_DISPLIB_CTX sDispCtx =
{
    .szI2CBusName = NU_PKG_USING_DISPLIB_I2C_DEVNAME,
    .RstPinIdx    = NU_PKG_USING_DISPLIB_RST_IDXNUM,
    .bRstActLow   = RT_TRUE,
};

int disp_converter_init(void)
{

#if defined(NU_PKG_USING_DISPLIB_LT8918)
    rt_kprintf("%s: Use LT8918 driver.\n", __func__);
    return displib_lt8918_init(&sDispCtx);
#elif defined(NU_PKG_USING_DISPLIB_LT8718)
    rt_kprintf("%s: Use LT8718 driver.\n", __func__);
    return displib_lt8718_init(&sDispCtx);
#elif defined(NU_PKG_USING_DISPLIB_LT8618)
    rt_kprintf("%s: Use LT8618 driver.\n", __func__);
    return displib_lt8618_init(&sDispCtx);
#endif

    return -1;
}

int disp_converter_fini(void)
{

#if defined(NU_PKG_USING_DISPLIB_LT8918)
    rt_kprintf("%s: Use LT8918 driver.\n", __func__);
    return displib_lt8918_fini(&sDispCtx);
#elif defined(NU_PKG_USING_DISPLIB_LT8718)
    rt_kprintf("%s: Use LT8718 driver.\n", __func__);
    return displib_lt8718_fini(&sDispCtx);
#elif defined(NU_PKG_USING_DISPLIB_LT8618)
    rt_kprintf("%s: Use LT8618 driver.\n", __func__);
    return displib_lt8618_fini(&sDispCtx);
#endif

    return -1;
}
