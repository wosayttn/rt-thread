/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-12-16     Wayne        first version
 */

#ifndef SENSOR_ST_LPS22HH_H__
#define SENSOR_ST_LPS22HH_H__

#include "lps22hh.h"

#include <rtthread.h>
#include <rtdevice.h>

#define RT_SIZE_TYPE   rt_ssize_t

int rt_hw_lps22hh_init(const char *name, struct rt_sensor_config *cfg);

#endif
