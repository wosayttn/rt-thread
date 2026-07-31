/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtdevice.h>

#define LEDR      "PH.4"

int main(int argc, char **argv)
{

#if defined(RT_USING_PIN)
    /* Get LED pin and set to output mode */
    rt_base_t pin_ledr = rt_pin_get(LEDR);

    /* set pin mode to output */
    rt_pin_mode(pin_ledr, PIN_MODE_OUTPUT);

    while (1)
    {
        /* Toggle LED every 500ms */
        rt_pin_write(pin_ledr, PIN_HIGH);
        rt_thread_mdelay(500);

        /* Toggle LED every 500ms */
        rt_pin_write(pin_ledr, PIN_LOW);
        rt_thread_mdelay(500);
    }
#endif

    return 0;
}
