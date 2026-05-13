/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_EQEI) && defined(BSP_USING_EQEI0)

#include "rtdevice.h"
#include "NuMicro.h"
#include "drv_gpio.h"
#include "drv_eqei.h"
#include "drv_common.h"

#define MOTOR_CW_MODE                  0
#define MOTOR_CCW_MODE                 1

#define EQEI0_A   NU_GET_PININDEX(NU_PE, 0)
#define EQEI0_B   NU_GET_PININDEX(NU_PE, 1)
#define EQEI0_IDX NU_GET_PININDEX(NU_PH, 8)

#define QEI_COMPARE_VAL_UTEST          100
#define QEI_MAX_VAL_UTEST              1000

/*
 * Quadrature Encoder Signal Emulator - CW
 *           -----       -----
 * QEI_A ___|     |_____|     |_____
 *
 *              -----       -----
 * QEI_B ______|     |_____|     |__
 *
 *       ------       -----       --
 * QEI_X       |_____|     |_____|
 *
 *
 *
 * Quadrature Encoder Signal Emulator - CCW
 *              -----       -----
 * QEI_A ______|     |_____|     |__
 *
 *           -----       -----
 * QEI_B ___|     |_____|     |_____
 *
 *       ---       -----       -----
 * QEI_X    |_____|     |_____|
 *
 */

static rt_uint8_t u8qei_ccw_cw_A = 0x63;
static rt_uint8_t u8qei_ccw_cw_B = 0x36;
static rt_uint8_t u8qei_ccw_cw_IDX = 0xC9;

static rt_device_t pulse_encoder_dev = RT_NULL;

static void eqei_dump_test_setting(void)
{
    rt_kprintf("\n[EQEI utest] description\n");
    rt_kprintf("  purpose               : Verify EQEI counting, direction, compare and max-value logic.\n");
    rt_kprintf("  compare value         : %d\n", QEI_COMPARE_VAL_UTEST);
    rt_kprintf("  max counter value     : %d\n", QEI_MAX_VAL_UTEST);
    rt_kprintf("  emulation modes       : CW=%d, CCW=%d\n\n", MOTOR_CW_MODE, MOTOR_CCW_MODE);
}

static void motor_encoder_emulator(rt_uint8_t u8mode, rt_uint32_t u32phase_num, rt_uint8_t u8Continous, rt_uint16_t u16Dlyms, rt_int32_t i32_EQEI_A, rt_int32_t i32_EQEI_B, rt_int32_t i32_EQEI_IDX)
{
    static rt_uint8_t u8wavcnt;

    if (u8Continous)
    {
        u8wavcnt++;
        if (u8wavcnt >= ((u8mode * 4) + 4))
            u8wavcnt = u8mode * 4;
    }
    else
    {
        if (u32phase_num == 0)
        {
            rt_pin_write(i32_EQEI_A, u8qei_ccw_cw_A >> ((u8mode * 4) + 3));
            rt_pin_write(i32_EQEI_B, u8qei_ccw_cw_B >> ((u8mode * 4) + 3));
            rt_pin_write(i32_EQEI_IDX, u8qei_ccw_cw_IDX >> ((u8mode * 4) + 3));
        }
    }

    while (u32phase_num)
    {
        for (u8wavcnt = ((u8Continous != 0) ? u8wavcnt : (u8mode * 4)) ; u8wavcnt < ((u8mode * 4) + 4) ; u8wavcnt++)
        {
            rt_pin_write(i32_EQEI_A, u8qei_ccw_cw_A >> u8wavcnt);
            rt_pin_write(i32_EQEI_B, u8qei_ccw_cw_B >> u8wavcnt);
            rt_pin_write(i32_EQEI_IDX, u8qei_ccw_cw_IDX >> u8wavcnt);
            u8Continous = 0;

            rt_thread_mdelay(u16Dlyms);
            u32phase_num--;
            if (u32phase_num == 0)
                return;
        }
    }
}

static rt_err_t utest_tc_init(void)
{

    rt_err_t ret = RT_EOK;

    eqei_dump_test_setting();

    rt_kprintf("Tips: To short the pin PD11 -> PE0 ; PD10 -> PE1 ; PD12 -> PH8\n");

    nu_pin_set_function(EQEI0_A, 0);
    nu_pin_set_function(EQEI0_B, 0);
    nu_pin_set_function(EQEI0_IDX, 0);

    /* Disable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PD, BIT11);

    SET_EQEI0_A_PD11();
    SET_EQEI0_B_PD10();
    SET_EQEI0_INDEX_PD12();

    /* set QEI_A & QEI_B pin output low */
    rt_pin_write(EQEI0_A, PIN_LOW);
    rt_pin_write(EQEI0_B, PIN_LOW);
    rt_pin_write(EQEI0_IDX, PIN_LOW);

    /* set QEI_A & QEI_B pin mode to output */
    rt_pin_mode(EQEI0_A, PIN_MODE_OUTPUT);
    rt_pin_mode(EQEI0_B, PIN_MODE_OUTPUT);
    rt_pin_mode(EQEI0_IDX, PIN_MODE_OUTPUT);


    pulse_encoder_dev = rt_device_find("eqei0");
    if (pulse_encoder_dev == RT_NULL)
    {
        rt_kprintf("pulse encoder sample run failed! can't find %s device!\n", "qei0");
        ret = RT_ERROR;
        return ret;
    }

    nu_eqei_set_cmpval(pulse_encoder_dev, 100);
    ret = rt_device_open(pulse_encoder_dev, RT_DEVICE_OFLAG_RDONLY);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", "eqei0");
        ret = RT_ERROR;
        return ret;
    }

    return ret;
}

static void up_counter_test(rt_device_t dev, rt_int32_t i32A, rt_int32_t i32B, rt_int32_t i32IDX)
{
    rt_uint32_t u32cnt;

    nu_eqei_set_maxval_type(dev, 1000, AB_PHASE_PULSE_ENCODER);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CW_MODE, 5, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 5);

    motor_encoder_emulator(MOTOR_CW_MODE, 3, 1, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 8);

    motor_encoder_emulator(MOTOR_CW_MODE, 5, 1, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 13);

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CW_MODE, 33, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 33);

    return;
}

static void down_counter_test(rt_device_t dev, rt_int32_t i32A, rt_int32_t i32B, rt_int32_t i32IDX)
{
    rt_uint32_t u32cnt;

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CCW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CCW_MODE, 3, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == (QEI_MAX_VAL_UTEST - 2));

    motor_encoder_emulator(MOTOR_CCW_MODE, 5, 1, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == (QEI_MAX_VAL_UTEST - 7));

    motor_encoder_emulator(MOTOR_CCW_MODE, 1, 1, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == (QEI_MAX_VAL_UTEST - 8));

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CCW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CCW_MODE, 27, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == (QEI_MAX_VAL_UTEST - 26));

    return;
}

static void on_off_test(rt_device_t dev, rt_int32_t i32A, rt_int32_t i32B, rt_int32_t i32IDX)
{
    rt_uint32_t u32cnt;

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_control(dev, PULSE_ENCODER_CMD_DISABLE, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CW_MODE, 3, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_ENABLE, RT_NULL);
    motor_encoder_emulator(MOTOR_CW_MODE, 7, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 7);

    return;
}

static void cmp_test(rt_device_t dev, rt_int32_t i32A, rt_int32_t i32B, rt_int32_t i32IDX)
{
    rt_uint32_t u32cnt;

    nu_eqei_set_cmpval(dev, QEI_COMPARE_VAL_UTEST);
    uassert_true(nu_eqei_get_cmpval(dev) == QEI_COMPARE_VAL_UTEST);

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    *(rt_uint8_t *)(dev->user_data) = 0;
    motor_encoder_emulator(MOTOR_CW_MODE, 101, 0, 10, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 101);
    uassert_true(*(rt_uint8_t *)((dev)->user_data) == 1);
    *(rt_uint8_t *)((dev)->user_data) = 0;

    return;
}

static void up_counter_type_test(rt_device_t dev, rt_int32_t i32A, rt_int32_t i32B, rt_int32_t i32IDX)
{
    rt_uint32_t u32cnt;

    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    nu_eqei_set_maxval_type(dev, 50, SINGLE_PHASE_PULSE_ENCODER);
    uassert_true(nu_eqei_get_type(dev) == EQEI_CTL_X2_COMPARE_COUNTING_MODE);

    uassert_true(nu_eqei_get_maxval(dev) == 50);

    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CW_MODE, 5, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 3);

    motor_encoder_emulator(MOTOR_CW_MODE, 3, 1, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 4);

    motor_encoder_emulator(MOTOR_CW_MODE, 5, 1, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 7);

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    motor_encoder_emulator(MOTOR_CW_MODE, 33, 0, 20, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 17);

    nu_eqei_set_cmpval(dev, 25);
    uassert_true(nu_eqei_get_cmpval(dev) == 25);

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    *(rt_uint8_t *)((dev)->user_data) = 0;
    motor_encoder_emulator(MOTOR_CW_MODE, 103, 0, 10, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 1);
    uassert_true(*(rt_uint8_t *)((dev)->user_data) == 1);
    *(rt_uint8_t *)((dev)->user_data) = 0;


    nu_eqei_set_cmpval(dev, 0);
    uassert_true(nu_eqei_get_cmpval(dev) == 0);

    /* reset emulator pin status */
    motor_encoder_emulator(MOTOR_CW_MODE, 0, 0, 0, i32A, i32B, i32IDX);
    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 0);

    *(rt_uint8_t *)((dev)->user_data) = 0;
    motor_encoder_emulator(MOTOR_CW_MODE, 103, 0, 10, i32A, i32B, i32IDX);
    rt_device_read(dev, 0, &u32cnt, 1);
    uassert_true(u32cnt == 1);
    uassert_true(*(rt_uint8_t *)((dev)->user_data) == 0);
    *(rt_uint8_t *)((dev)->user_data) = 0;

    return;
}

static void utest_up_counter(void)
{
    up_counter_test((rt_device_t)pulse_encoder_dev, EQEI0_A, EQEI0_B, EQEI0_IDX);

    return;
}

static void utest_down_counter(void)
{
    down_counter_test((rt_device_t)pulse_encoder_dev, EQEI0_A, EQEI0_B, EQEI0_IDX);

    return;
}


static void utest_on_off(void)
{
    on_off_test((rt_device_t)pulse_encoder_dev, EQEI0_A, EQEI0_B, EQEI0_IDX);

    return;
}

static void utest_cmp_test(void)
{
    cmp_test((rt_device_t)pulse_encoder_dev, EQEI0_A, EQEI0_B, EQEI0_IDX);

    return;
}

static void utest_up_counter_type_test(void)
{
    up_counter_type_test((rt_device_t)pulse_encoder_dev, EQEI0_A, EQEI0_B, EQEI0_IDX);

    return;
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_device_close(pulse_encoder_dev);
    pulse_encoder_dev = RT_NULL;

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(utest_up_counter);
    UTEST_UNIT_RUN(utest_down_counter);
    UTEST_UNIT_RUN(utest_on_off);
    UTEST_UNIT_RUN(utest_cmp_test);
    UTEST_UNIT_RUN(utest_up_counter_type_test);

    return;
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"eqei",
                utest_tc_init, utest_tc_cleanup, 5);

#endif /* BSP_USING_EQEI */
