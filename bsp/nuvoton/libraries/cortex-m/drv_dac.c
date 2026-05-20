/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include "drv_sys.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.dac"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

enum
{
    DAC_START = -1,
#if defined(BSP_USING_DAC0)
    DAC0_IDX,
#endif
#if defined(BSP_USING_DAC1)
    DAC1_IDX,
#endif
    DAC_CNT
};

/* Types / Structures ---------------------------------------------------------*/
struct nu_dac
{
    struct rt_dac_device dev;
    const struct nu_module m_module;
    uint32_t  chn_msk;
    uint32_t  max_chn_num;
};
typedef struct nu_dac *nu_dac_t;

/* Static Function Prototypes ------------------------------------------------*/
static rt_err_t nu_dac_enabled(struct rt_dac_device *device, rt_uint32_t channel);
static rt_err_t nu_dac_disabled(struct rt_dac_device *device, rt_uint32_t channel);
static rt_err_t nu_dac_convert(struct rt_dac_device *device, rt_uint32_t channel, rt_uint32_t *value);

/* Function Prototypes -------------------------------------------------------*/
int rt_hw_dac_init(void);

/* Static Variables ----------------------------------------------------------*/

static struct nu_dac nu_dac_arr [] =
{
#if defined(BSP_USING_DAC0)
    {
        .m_module = { .name = "dac0", .base = DAC0 }, .chn_msk = 0, .max_chn_num = 1,
    },
#endif
#if defined(BSP_USING_DAC1)
    {
        .m_module = { .name = "dac1", .base = DAC1 }, .chn_msk = 0, .max_chn_num = 1,
    },
#endif
};

/* nu_dac_enabled - Enable DAC engine and wait for ready */

/* Functions Implementation --------------------------------------------------*/
static rt_err_t nu_dac_enabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    nu_dac_t psNuDAC = (nu_dac_t)device;

    RT_ASSERT(device);

    if (channel >= psNuDAC->max_chn_num)
        return -(RT_EINVAL);

    if (!(psNuDAC->chn_msk & (0x1 << channel)))
    {
        DAC_Open((DAC_T *)psNuDAC->m_module.base, channel, DAC_SOFTWARE_TRIGGER);

        /* The DAC conversion settling time is 1us */
        DAC_SetDelayTime((DAC_T *)psNuDAC->m_module.base, 1);

        DAC_ENABLE_RIGHT_ALIGN((DAC_T *)psNuDAC->m_module.base);

        psNuDAC->chn_msk |= (0x1 << channel);
    }

    return RT_EOK;
}

static rt_err_t nu_dac_disabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    nu_dac_t psNuDAC = (nu_dac_t)device;

    RT_ASSERT(device);

    if (channel >= psNuDAC->max_chn_num)
        return -(RT_EINVAL);

    if (psNuDAC->chn_msk & (0x1 << channel))
    {
        DAC_Close((DAC_T *)psNuDAC->m_module.base, channel);

        psNuDAC->chn_msk &= ~(0x1 << channel);
    }

    return RT_EOK;
}

static rt_err_t nu_dac_convert(struct rt_dac_device *device, rt_uint32_t channel, rt_uint32_t *value)
{
    nu_dac_t psNuDAC = (nu_dac_t)device;
    rt_err_t ret = -RT_ERROR;

    RT_ASSERT(device);
    RT_ASSERT(value);

    if (channel >= psNuDAC->max_chn_num)
    {
        ret = -RT_EINVAL;
        goto exit_nu_dac_convert;
    }

    if (!(psNuDAC->chn_msk & (1 << channel)))
    {
        goto exit_nu_dac_convert;
    }

    /* Set DAC 12-bit holding data */
    DAC_WRITE_DATA((DAC_T *)psNuDAC->m_module.base, 0, (uint16_t)*value);

    /* Start A/D conversion */
    DAC_START_CONV((DAC_T *)psNuDAC->m_module.base);

    ret = RT_EOK;

exit_nu_dac_convert:

    return -(ret);
}

static const struct rt_dac_ops nu_dac_ops =
{
    .disabled = nu_dac_disabled,
    .enabled  = nu_dac_enabled,
    .convert  = nu_dac_convert,
};

int rt_hw_dac_init(void)
{
    int i;
    rt_err_t result;

    for (i = (DAC_START + 1); i < DAC_CNT; i++)
    {
        result = rt_hw_dac_register(&nu_dac_arr[i].dev, nu_dac_arr[i].m_module.name, &nu_dac_ops, NULL);
        RT_ASSERT(result == RT_EOK);
    }

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_dac_init);
