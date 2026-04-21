/****************************************************************************
 * @file     nutool_pincfg.c
 * @version  v1.35.3
 * @Date     Mon Oct 27 2025 19:42:48 GMT+0800 (Taipei Standard Time)
 * @brief    NuMicro generated code file
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2013-2025 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

/********************
MCU:M3351KJCAE(LQFP128)
********************/

#include "rtconfig.h"
#include "NuMicro.h"

#if defined(BOARD_USING_NUTFT)
void expansion_nutft_pin_init(void)
{
#if defined(BOARD_USING_LCD_ILI9341)
    SET_SPI1_MISO_PC7();
    SET_SPI1_CLK_PA7();
    SET_SPI1_SS_PA6();
    SET_SPI1_MOSI_PC6();

    SET_GPIO_PB2();
    SET_GPIO_PB3();
    SET_GPIO_PA10();
#endif

#if defined(BOARD_USING_NUTFT_ADC_TOUCH)
    GPIO_SetMode(PB, BIT4 | BIT5 | BIT6 | BIT7, GPIO_MODE_INPUT);

    /* EADC Analog Pin: UNO_A0, UNO_A1, UNO_A2, UNO_A3 */
    SET_EADC0_CH4_PB4();
    SET_EADC0_CH5_PB5();
    SET_EADC0_CH6_PB6();
    SET_EADC0_CH7_PB7();

    /* Disable digital path on these EADC pins */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT4 | BIT5 | BIT6 | BIT7);
#endif

#if defined(BOARD_USING_QSPI_FLASH)
    SET_QSPI0_MOSI0_PA0();
    SET_QSPI0_MISO0_PA1();
    SET_QSPI0_CLK_PA2();
    SET_QSPI0_SS_PA3();
    SET_QSPI0_MOSI1_PA4();
    SET_QSPI0_MISO1_PA5();
#endif

#if defined(BOARD_USING_NUTFT_BUTTON)
    SYS->GPB_MFP0 &= ~(SYS_GPB_MFP0_PB0MFP_Msk | SYS_GPB_MFP0_PB1MFP_Msk);

    /* Disable digital path on these ADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT0 | BIT1);
#endif

}
#endif

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

void nutool_pincfg_init_i3c0(void)
{
    /* Set multi-function pins for I3C pin */
    GPIO_ENABLE_SCHMITT_TRIGGER(PA, (BIT0 | BIT1));
    SET_I3C0_SDA_PA0();
    SET_I3C0_SCL_PA1();

    /* Use TYPE-A Resistance Connection */
    PA->PUSEL = ((GPIO_PUSEL_PULL_UP << (0 << 1)) | (GPIO_PUSEL_PULL_UP << (1 << 1)));
    SET_I3C0_PUPEN_PA2();

    return;
}

void nutool_pincfg_deinit_i3c0(void)
{
    SYS->GPA_MFP0 &= ~(SYS_GPA_MFP0_PA1MFP_Msk | SYS_GPA_MFP0_PA0MFP_Msk);
    SYS->GPA_MFP0 &= ~(SYS_GPA_MFP0_PA2MFP_Msk);

    return;
}


void nutool_pincfg_init(void)
{
    /* Vref connect to internal */
    SYS_SetVRef(SYS_VREFCTL_VREF_3_0V);

    nutool_pincfg_init_uart0();

#if defined(BOARD_USING_NUTFT)
    expansion_nutft_pin_init();
#endif

//    nutool_pincfg_init_i3c0();
//    SET_LLSI0_OUT_PB15();
//    SET_PWM0_CH0_PA5();
//    SET_PWM1_CH0_PC5();
//    SET_TM0_PB5();

//    SET_CANFD0_RXD_PC4();
//    SET_CANFD0_TXD_PC5();

    return;
}

void nutool_pincfg_deinit(void)
{
    nutool_pincfg_deinit_uart0();

    return;
}

/*** (C) COPYRIGHT 2013-2025 Nuvoton Technology Corp. ***/
