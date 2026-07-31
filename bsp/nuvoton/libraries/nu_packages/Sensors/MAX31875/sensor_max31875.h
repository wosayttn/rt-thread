/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_MAX31875_H__
#define __SENSOR_MAX31875_H__

#include "rtdevice.h"

#include "max31875_c.h"

int rt_hw_max31875_init(const char *name, struct rt_sensor_config *cfg);

#endif /* __SENSOR_MAX31875_H__ */
