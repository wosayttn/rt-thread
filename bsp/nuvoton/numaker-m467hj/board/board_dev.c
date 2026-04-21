/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtdevice.h>

#include "drv_gpio.h"

#if defined(BOARD_USING_NAU8822) && defined(NU_PKG_USING_NAU8822)
#include <acodec_nau8822.h>
S_NU_NAU8822_CONFIG sCodecConfig =
{
    .i2c_bus_name = "i2c2",

    .i2s_bus_name = "sound0",

    .pin_phonejack_en = NU_GET_PININDEX(NU_PD, 3),

    .pin_phonejack_det = NU_GET_PININDEX(NU_PD, 2),
};

int rt_hw_nau8822_port(void)
{
    if (nu_hw_nau8822_init(&sCodecConfig) != RT_EOK)
        return -1;

    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_nau8822_port);
#endif /* BOARD_USING_NAU8822 */

#if defined(BOARD_USING_LCD_FSA506) && defined(NU_PKG_USING_FSA506_EBI)
#include "drv_ebi.h"
#include "lcd_fsa506.h"

int rt_hw_fsa506_port(void)
{
    rt_err_t ret = RT_EOK;

    /* Open ebi BOARD_USING_FSA506_EBI_PORT */
    ret = nu_ebi_init(BOARD_USING_FSA506_EBI_PORT, EBI_BUSWIDTH_16BIT, EBI_TIMING_FAST, EBI_OPMODE_ADSEPARATE, EBI_CS_ACTIVE_LOW);
    if (ret != RT_EOK)
        return ret;

    #define EBI_8080_ACCESS_NS   43
    #define EBI_8080_WR_IDLE_NS  35
    #define EBI_8080_WR_AHD_NS   13
    #define EBI_8080_RD_AHD_NS   13
    #define EBI_8080_RD_IDLE_NS  0
    nu_ebi_apply_timing(BOARD_USING_FSA506_EBI_PORT,
                    EBI_8080_ACCESS_NS,
                    EBI_8080_WR_IDLE_NS,
                    EBI_8080_WR_AHD_NS,
                    EBI_8080_RD_AHD_NS,
                    EBI_8080_RD_IDLE_NS);

    if (rt_hw_lcd_fsa506_ebi_init(EBI_BANK0_BASE_ADDR + BOARD_USING_FSA506_EBI_PORT * EBI_MAX_SIZE) != RT_EOK)
        return -1;

    return rt_hw_lcd_fsa506_init();
}
INIT_COMPONENT_EXPORT(rt_hw_fsa506_port);
#endif /* BOARD_USING_LCD_FSA506 */


#if defined(BOARD_USING_ST1663I) && defined(NU_PKG_USING_TPC_ST1663I)
#include "tpc_st1663i.h"

#define ST1663I_RST_PIN   NU_GET_PININDEX(NU_PD, 10)
#define ST1663I_IRQ_PIN   NU_GET_PININDEX(NU_PG, 6)

extern int tpc_sample(const char *name);
int rt_hw_st1663i_port(void)
{
    struct rt_touch_config cfg;
    rt_base_t rst_pin = ST1663I_RST_PIN;
    cfg.dev_name = "i2c1";
    cfg.irq_pin.pin = ST1663I_IRQ_PIN;
    cfg.irq_pin.mode = PIN_MODE_INPUT_PULLUP;
    cfg.user_data = &rst_pin;

    rt_hw_st1663i_init("st1663i", &cfg);
    return tpc_sample("st1663i");

}
INIT_ENV_EXPORT(rt_hw_st1663i_port);
#endif /* if defined(BOARD_USING_ST1663I) && defined(NU_PKG_USING_TPC_ST1663I) */

#if defined(BOARD_USING_SENSOR0)
#include "ccap_sensor.h"

#define SENSOR0_RST_PIN    NU_GET_PININDEX(NU_PG, 11)
#define SENSOR0_PD_PIN     NU_GET_PININDEX(NU_PD, 12)

ccap_sensor_io sIo_sensor0 =
{
    .RstPin          = SENSOR0_RST_PIN,
    .PwrDwnPin       = SENSOR0_PD_PIN,
    .I2cName         = "i2c0"
};

int rt_hw_sensor0_port(void)
{
    return  nu_ccap_sensor_create(&sIo_sensor0, (ccap_sensor_id)BOARD_USING_SENSOR0_ID, "sensor0");
}
INIT_COMPONENT_EXPORT(rt_hw_sensor0_port);

#endif /* BOARD_USING_SENSOR0 */

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
