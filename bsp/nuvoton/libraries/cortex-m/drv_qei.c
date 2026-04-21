/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include <rtthread.h>

#if defined(BSP_USING_QEI)

#include <rtdevice.h>
#include <rthw.h>
#include "drv_sys.h"
#include "drv_qei.h"

/* Defines / Macros ----------------------------------------------------------*/
#undef LOG_TAG
#define LOG_TAG "drv.qei"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#define DEFINE_NU_QEI(_idx)         \
    {                               \
        .name = "qei" #_idx,        \
        .qei_base = QEI##_idx,      \
        .max_cntval = 1000,         \
        .cmp_val = 100,             \
        .IRQn = QEI##_idx##_IRQn,   \
        .qei_type = AB_PHASE_PULSE_ENCODER, \
        .qei_flag = 0,              \
    }

#define DEFINE_QEI_IRQ_HANDLER(_idx)        \
void QEI##_idx##_IRQHandler(void)           \
{                                           \
    rt_interrupt_enter();                   \
    nu_qei_isr(&nu_qei_arr[QEI##_idx##_IDX]); \
    rt_interrupt_leave();                   \
}

/* Types / Structures ---------------------------------------------------------*/
enum
{
    QEI_START = -1,
#if defined(BSP_USING_QEI0)
    QEI0_IDX,
#endif
#if defined(BSP_USING_QEI1)
    QEI1_IDX,
#endif
    QEI_CNT
};

struct nu_qei
{
    struct rt_pulse_encoder_device dev;
    char *name;
    QEI_T *qei_base;
    rt_uint32_t max_cntval;
    rt_uint32_t cmp_val;
    IRQn_Type IRQn;
    rt_uint8_t qei_type;
    rt_uint8_t qei_flag;
};
typedef struct nu_qei *nu_qei_t;

/* Static Function Prototypes ------------------------------------------------*/
static rt_uint32_t nu_qei_type(struct rt_pulse_encoder_device *pulse_encoder);
static rt_err_t nu_qei_init(struct rt_pulse_encoder_device *pulse_encoder);
static rt_int32_t nu_qei_get_count(struct rt_pulse_encoder_device *pulse_encoder);
static rt_err_t nu_qei_clear_count(struct rt_pulse_encoder_device *pulse_encoder);
static rt_err_t nu_qei_control(struct rt_pulse_encoder_device *pulse_encoder, rt_uint32_t cmd, void *args);
static void nu_qei_isr(nu_qei_t psNuQEI);
static int rt_hw_qei_init(void);

/* Static Variables ----------------------------------------------------------*/
static struct nu_qei nu_qei_arr[] =
{
#if defined(BSP_USING_QEI0)
    DEFINE_NU_QEI(0),
#endif
#if defined(BSP_USING_QEI1)
    DEFINE_NU_QEI(1),
#endif
};

static const struct rt_pulse_encoder_ops nu_qei_ops =
{
    .init = nu_qei_init,
    .get_count = nu_qei_get_count,
    .clear_count = nu_qei_clear_count,
    .control = nu_qei_control,
};

/* Functions Implementation --------------------------------------------------*/
#if defined(BSP_USING_QEI0)
    DEFINE_QEI_IRQ_HANDLER(0)
#endif

#if defined(BSP_USING_QEI1)
    DEFINE_QEI_IRQ_HANDLER(1)
#endif

static void nu_qei_isr(nu_qei_t psNuQEI)
{
    if (QEI_GET_INT_FLAG(psNuQEI->qei_base, QEI_STATUS_CMPF_Msk))
    {
        psNuQEI->qei_flag = 1;
        QEI_CLR_INT_FLAG(psNuQEI->qei_base, QEI_STATUS_CMPF_Msk);
        LOG_D("%s CMP flag rising", psNuQEI->name);
    }
}

static rt_uint32_t nu_qei_type(struct rt_pulse_encoder_device *pulse_encoder)
{
    rt_uint32_t u32type;
    switch (pulse_encoder->type)
    {
    case SINGLE_PHASE_PULSE_ENCODER:
        u32type = (((nu_qei_t)pulse_encoder)->cmp_val) ? QEI_CTL_X2_COMPARE_COUNTING_MODE : QEI_CTL_X2_FREE_COUNTING_MODE;
        break;

    case UNKNOWN_PULSE_ENCODER_TYPE:
    case AB_PHASE_PULSE_ENCODER:
    default:
        u32type = (((nu_qei_t)pulse_encoder)->cmp_val) ? QEI_CTL_X4_COMPARE_COUNTING_MODE : QEI_CTL_X4_FREE_COUNTING_MODE;
        break;
    }

    return u32type;
}

static rt_err_t nu_qei_init(struct rt_pulse_encoder_device *pulse_encoder)
{
    rt_uint32_t u32type;
    QEI_T *qui_base;

    RT_ASSERT(pulse_encoder != RT_NULL);
    qui_base = ((nu_qei_t)pulse_encoder)->qei_base;

    /* enable noise filter */
    QEI_ENABLE_NOISE_FILTER(qui_base, QEI_CTL_NFCLKSEL_DIV2);

    /* set qei mode */
    u32type = nu_qei_type(pulse_encoder);

    /* set compare value and interrupt */
    if (((nu_qei_t)pulse_encoder)->cmp_val)
    {
        QEI_SET_CNT_CMP(qui_base, ((nu_qei_t)pulse_encoder)->cmp_val);
        QEI_ENABLE_INT(qui_base, QEI_CTL_CMPIEN_Msk);
        QEI_ENABLE_CNT_CMP(qui_base);
        NVIC_EnableIRQ(((nu_qei_t)pulse_encoder)->IRQn);
    }
    QEI_Open(qui_base, u32type, ((nu_qei_t)pulse_encoder)->max_cntval);

    return RT_EOK;
}

static rt_int32_t nu_qei_get_count(struct rt_pulse_encoder_device *pulse_encoder)
{
    QEI_T *qui_base;
    RT_ASSERT(pulse_encoder != RT_NULL);
    qui_base = ((nu_qei_t)pulse_encoder)->qei_base;
    return (rt_int32_t)QEI_GET_CNT_VALUE(qui_base);
}

static rt_err_t nu_qei_clear_count(struct rt_pulse_encoder_device *pulse_encoder)
{
    QEI_T *qui_base;
    RT_ASSERT(pulse_encoder != RT_NULL);
    qui_base = ((nu_qei_t)pulse_encoder)->qei_base;
    QEI_Stop(qui_base);
    QEI_SET_CNT_VALUE(qui_base, 0);
    QEI_Start(qui_base);
    return RT_EOK;
}

static rt_err_t nu_qei_control(struct rt_pulse_encoder_device *pulse_encoder, rt_uint32_t cmd, void *args)
{
    rt_err_t result = RT_EOK;
    QEI_T *qui_base;

    RT_ASSERT(pulse_encoder != RT_NULL);
    qui_base = ((nu_qei_t)pulse_encoder)->qei_base;

    switch (cmd)
    {
    case PULSE_ENCODER_CMD_ENABLE:
        QEI_Start(qui_base);
        if (((nu_qei_t)pulse_encoder)->cmp_val)
        {
            QEI_ENABLE_INT(qui_base, QEI_CTL_CMPIEN_Msk);
            QEI_ENABLE_CNT_CMP(qui_base);
            NVIC_EnableIRQ(((nu_qei_t)pulse_encoder)->IRQn);
        }
        break;
    case PULSE_ENCODER_CMD_DISABLE:
        if (((nu_qei_t)pulse_encoder)->cmp_val)
        {
            QEI_DISABLE_INT(qui_base, QEI_CTL_CMPIEN_Msk);
            QEI_DISABLE_CNT_CMP(qui_base);
            NVIC_DisableIRQ(((nu_qei_t)pulse_encoder)->IRQn);
        }
        QEI_Stop(qui_base);
        break;
    default:
        result = -RT_ENOSYS;
        break;
    }

    return result;
}

rt_int32_t nu_qei_get_maxval(rt_device_t *pulse_encoder)
{
    return ((nu_qei_t)*pulse_encoder)->max_cntval;
}

rt_int32_t nu_qei_get_cmpval(rt_device_t *pulse_encoder)
{
    return ((nu_qei_t)*pulse_encoder)->cmp_val;
}

rt_int32_t nu_qei_get_type(rt_device_t *pulse_encoder)
{
    return ((nu_qei_t)*pulse_encoder)->qei_type;
}

void nu_qei_set_maxval_type(rt_device_t *pulse_encoder, rt_uint32_t u32val, rt_uint8_t u8type)
{
    QEI_T *qui_base;
    RT_ASSERT(pulse_encoder != RT_NULL);
    qui_base = ((nu_qei_t)*pulse_encoder)->qei_base;

    RT_ASSERT(u8type <= AB_PHASE_PULSE_ENCODER);
    ((nu_qei_t)*pulse_encoder)->qei_type = u8type;
    ((nu_qei_t)*pulse_encoder)->dev.type = (enum rt_pulse_encoder_type)u8type;
    ((nu_qei_t)*pulse_encoder)->max_cntval = u32val;
    QEI_Open(qui_base, nu_qei_type(&(((nu_qei_t)*pulse_encoder)->dev)), u32val);
}

void nu_qei_set_cmpval(rt_device_t *pulse_encoder, rt_uint32_t u32val)
{
    QEI_T *qui_base;
    RT_ASSERT(pulse_encoder != RT_NULL);
    qui_base = ((nu_qei_t)*pulse_encoder)->qei_base;

    ((nu_qei_t)*pulse_encoder)->cmp_val = u32val;
    QEI_SET_CNT_CMP(qui_base, ((nu_qei_t)*pulse_encoder)->cmp_val);

    if (((nu_qei_t)*pulse_encoder)->cmp_val)
    {
        QEI_ENABLE_INT(qui_base, QEI_CTL_CMPIEN_Msk);
        QEI_ENABLE_CNT_CMP(qui_base);
        NVIC_EnableIRQ(((nu_qei_t)*pulse_encoder)->IRQn);
    }
    else
    {
        QEI_DISABLE_INT(qui_base, QEI_CTL_CMPIEN_Msk);
        QEI_DISABLE_CNT_CMP(qui_base);
        NVIC_DisableIRQ(((nu_qei_t)*pulse_encoder)->IRQn);
    }
}

static int rt_hw_qei_init(void)
{
    int result = RT_EOK;
    int i;

    for (i = (QEI_START + 1); i < QEI_CNT; i++)
    {
        nu_qei_arr[i].dev.type = (enum rt_pulse_encoder_type)nu_qei_arr[i].qei_type;
        nu_qei_arr[i].dev.ops = &nu_qei_ops;

        result = rt_device_pulse_encoder_register((void *)&nu_qei_arr[i], nu_qei_arr[i].name, &(nu_qei_arr[i].qei_flag));
        RT_ASSERT(result == RT_EOK);
    }

    return result;
}
INIT_APP_EXPORT(rt_hw_qei_init);

#endif /* BSP_USING_QEI */
