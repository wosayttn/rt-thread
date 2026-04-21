/****************************************************************************
 * @file     nutool_pincfg.c
 * @version  V1.28.0009
 * @Date     Tue Jan 16 2024 17:34:02 GMT+0800 (Taipei Standard Time)
 * @brief    NuMicro generated code file
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2013-2024 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

/********************
MCU:M5531H2ES(LQFP176)
********************/

#include "NuMicro.h"
#include "rtconfig.h"

void nutool_pincfg_init_hsusb(void)
{
    SET_HSUSB_VBUS_EN_PJ13();
    SET_HSUSB_VBUS_ST_PJ12();

    return;
}

void nutool_pincfg_deinit_hsusb(void)
{
    SET_GPIO_PJ13();
    SET_GPIO_PJ12();

    return;
}

void nutool_pincfg_init_qspi0(void)
{
    SET_QSPI0_SS_PA3();
    SET_QSPI0_CLK_PA2();
    SET_QSPI0_MISO0_PA1();
    SET_QSPI0_MOSI0_PA0();
    SET_QSPI0_MISO1_PI12();
    SET_QSPI0_MOSI1_PI13();

    return;
}

void nutool_pincfg_deinit_qspi0(void)
{
    SET_GPIO_PA3();
    SET_GPIO_PA2();
    SET_GPIO_PA1();
    SET_GPIO_PA0();
    SET_GPIO_PI12();
    SET_GPIO_PI13();

    return;
}

void nutool_pincfg_init_spi2(void)
{
    SET_SPI2_SS_PA11();
    SET_SPI2_CLK_PA10();
    SET_SPI2_MISO_PA9();
    SET_SPI2_MOSI_PA8();

    return;
}

void nutool_pincfg_deinit_spi2(void)
{
    SET_GPIO_PA11();
    SET_GPIO_PA10();
    SET_GPIO_PA9();
    SET_GPIO_PA8();

    return;
}

void nutool_pincfg_init_uart0(void)
{
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();

    return;
}

void nutool_pincfg_deinit_uart0(void)
{
    SET_GPIO_PB12();
    SET_GPIO_PB13();

    return;
}

void nutool_pincfg_init_usb(void)
{
    SET_USB_VBUS_PA12();
    SET_USB_D_MINUS_PA13();
    SET_USB_D_PLUS_PA14();
    SET_USB_OTG_ID_PA15();

    SET_USB_VBUS_ST_PB14();
    SET_USB_VBUS_EN_PB15();

    return;
}

void nutool_pincfg_deinit_usb(void)
{
    SET_GPIO_PA15();
    SET_GPIO_PA12();
    SET_GPIO_PB14();
    SET_GPIO_PB15();

    return;
}

void nutool_pincfg_init_x32(void)
{
    SET_X32_IN_PF5();
    SET_X32_OUT_PF4();

    return;
}

void nutool_pincfg_deinit_x32(void)
{
    SET_GPIO_PF5();
    SET_GPIO_PF4();

    return;
}

void nutool_pincfg_init_xt1(void)
{
    SET_XT1_IN_PF3();
    SET_XT1_OUT_PF2();

    return;
}

void nutool_pincfg_deinit_xt1(void)
{
    SET_GPIO_PF3();
    SET_GPIO_PF2();

    return;
}


#if defined(BOARD_USING_NUTFT)
void expansion_nutft_pin_init(void)
{
#if defined(BOARD_USING_LCD_ILI9341)
    nutool_pincfg_init_spi2();

    SET_GPIO_PB2();
    SET_GPIO_PB3();
    SET_GPIO_PB5();

    /* Disable digital path on these ADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT2 | BIT3 | BIT5);
#endif

#if defined(BOARD_USING_NUTFT_ADC_TOUCH)
    GPIO_SetMode(PB, BIT6 | BIT7 | BIT8 | BIT9, GPIO_MODE_INPUT);

    SET_EADC0_CH6_PB6();
    SET_EADC0_CH7_PB7();
    SET_EADC0_CH8_PB8();
    SET_EADC0_CH9_PB9();

    /* Disable digital path on these ADC pins */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT6 | BIT7 | BIT8 | BIT9);
#endif

#if defined(BOARD_USING_QSPI_FLASH)
    nutool_pincfg_init_qspi0();
#endif
}
#endif

void nutool_pincfg_init(void)
{
    nutool_pincfg_init_uart0();
    nutool_pincfg_init_hsusb();
    nutool_pincfg_init_usb();
    nutool_pincfg_init_x32();
    nutool_pincfg_init_xt1();

#if defined(BOARD_USING_NUTFT)
    expansion_nutft_pin_init();
#endif

    /* Vref connect to extern pin */
    //SYS->VREFCTL = (SYS->VREFCTL & ~SYS_VREFCTL_VREFCTL_Msk) | SYS_VREFCTL_VREF_PIN;

    /* Vref connect to internal */
    SYS->VREFCTL = (SYS->VREFCTL & ~SYS_VREFCTL_VREFCTL_Msk) | SYS_VREFCTL_VREF_3_072V;

    return;
}

void nutool_pincfg_deinit(void)
{
    nutool_pincfg_deinit_uart0();
    nutool_pincfg_deinit_hsusb();
    nutool_pincfg_deinit_usb();
    nutool_pincfg_deinit_x32();
    nutool_pincfg_deinit_xt1();

    return;
}

/*** (C) COPYRIGHT 2013-2024 Nuvoton Technology Corp. ***/
