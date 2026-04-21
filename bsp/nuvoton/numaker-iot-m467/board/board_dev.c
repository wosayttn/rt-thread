/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtdevice.h>

#include "drv_gpio.h"

#if defined(BOARD_USING_NCT7717U)

#include "sensor_nct7717u.h"

int rt_hw_nct7717u_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name = "i2c2";
    cfg.irq_pin.pin = PIN_IRQ_PIN_NONE;

    return rt_hw_nct7717u_init("nct7717u", &cfg);
}
INIT_APP_EXPORT(rt_hw_nct7717u_port);
#endif /* BOARD_USING_NCT7717U */


#if defined(BOARD_USING_MPU6500) && defined(PKG_USING_MPU6XXX)

#include "sensor_inven_mpu6xxx.h"

int rt_hw_mpu6xxx_port(void)
{
    struct rt_sensor_config cfg;
    rt_base_t mpu_int = NU_GET_PININDEX(NU_PD, 2);

    cfg.intf.dev_name = "i2c2";
    cfg.intf.arg = (void *)MPU6XXX_ADDR_DEFAULT;
    cfg.irq_pin.pin = mpu_int;

    return rt_hw_mpu6xxx_init("mpu", &cfg);
}
INIT_APP_EXPORT(rt_hw_mpu6xxx_port);
#endif /* BOARD_USING_MPU6500 */


#if defined(BOARD_USING_ESP8266)

static int rt_hw_esp8266_port(void)
{
    rt_base_t esp_rst_pin = NU_GET_PININDEX(NU_PC, 4);

    /* ESP8266 reset pin PC.4 */
    rt_pin_mode(esp_rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(esp_rst_pin, 1);

    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_esp8266_port);

#endif /* BOARD_USING_ESP8266  */

#if defined(BOARD_USING_LCD_ILI9341) && defined(NU_PKG_USING_ILI9341_SPI)
#include "lcd_ili9341.h"

#if defined(NU_PKG_USING_ADC_TOUCH_SW)

#include "adc_touch.h"
#include "touch_sw.h"

#define NU_MFP_POS(PIN)          ((PIN % 4) * 8)
#define NU_MFP_MSK(PIN)          (0x1ful << NU_MFP_POS(PIN))

S_CALIBRATION_MATRIX g_sCalMat = { 97, 6214, -3216652, 4844, -30, -2333200, 65536 };

static void nu_pin_func(rt_base_t pin, int data)
{
    uint32_t pin_index      = NU_GET_PINS(pin);
    uint32_t port_index     = NU_GET_PORT(pin);
    __IO uint32_t *GPx_MFPx = ((__IO uint32_t *) &SYS->GPA_MFP0) + port_index * 4 + (pin_index / 4);
    uint32_t MFP_Msk        = NU_MFP_MSK(pin_index);

    *GPx_MFPx  = (*GPx_MFPx & (~MFP_Msk)) | data;
}

static void tp_switch_to_analog(rt_base_t pin)
{
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + (0x40) * NU_GET_PORT(pin));

    if (pin == NU_GET_PININDEX(NU_PB, 6))
        nu_pin_func(pin, SYS_GPB_MFP1_PB6MFP_EADC0_CH6);
    else if (pin == NU_GET_PININDEX(NU_PB, 9))
        nu_pin_func(pin, SYS_GPB_MFP2_PB9MFP_EADC0_CH9);

    GPIO_DISABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PINS(pin)));
}

static void tp_switch_to_digital(rt_base_t pin)
{
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + (0x40) * NU_GET_PORT(pin));

    nu_pin_func(pin, 0);

    /* Enable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PINS(pin)));
}

static S_TOUCH_SW sADCTP =
{
    .adc_name    = "eadc0",
    .i32ADCChnYU = 6,
    .i32ADCChnXR = 9,
    .pin =
    {
        NU_GET_PININDEX(NU_PB, 7), // XL
        NU_GET_PININDEX(NU_PB, 6), // YU
        NU_GET_PININDEX(NU_PB, 9), // XR
        NU_GET_PININDEX(NU_PB, 8), // YD
    },
    .switch_to_analog  = tp_switch_to_analog,
    .switch_to_digital = tp_switch_to_digital,
};

#endif

int rt_hw_ili9341_port(void)
{
    if (rt_hw_lcd_ili9341_spi_init("spi2", RT_NULL) != RT_EOK)
        return -1;

    rt_hw_lcd_ili9341_init();

#if defined(NU_PKG_USING_ADC_TOUCH_SW)
    nu_adc_touch_sw_register(&sADCTP);
#endif

    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_ili9341_port);
#endif /* if defined(BOARD_USING_LCD_ILI9341) && defined(NU_PKG_USING_ILI9341_SPI) */
