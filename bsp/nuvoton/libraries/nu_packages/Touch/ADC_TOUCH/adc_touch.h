/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ADC_TOUCH_CALIBRATE_H__
#define __ADC_TOUCH_CALIBRATE_H__

#include <stdint.h>

#define DEF_CAL_POINT_NUM    5

typedef struct
{
    int32_t   x;
    int32_t   y;
} S_COORDINATE_POINT;

typedef struct
{
    int32_t   a;
    int32_t   b;
    int32_t   c;
    int32_t   d;
    int32_t   e;
    int32_t   f;
    int32_t   div;
} S_CALIBRATION_MATRIX;

int rt_hw_adc_touch_init(void);

#endif /* __ADC_TOUCH_CALIBRATE_H__ */
