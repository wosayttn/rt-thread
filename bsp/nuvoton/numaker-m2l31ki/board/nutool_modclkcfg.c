/****************************************************************************
 * @file     nutool_modclkcfg.c
 * @version  V1.05
 * @Date     2020/11/11-11:43:32
 * @brief    NuMicro generated code file
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2013-2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "rtconfig.h"
#include "NuMicro.h"

void nutool_modclkcfg_init_acmp01(void)
{
    CLK_EnableModuleClock(ACMP01_MODULE);

    return;
}

void nutool_modclkcfg_deinit_acmp01(void)
{
    CLK_DisableModuleClock(ACMP01_MODULE);

    return;
}

void nutool_modclkcfg_init_canfd0(void)
{
    CLK_EnableModuleClock(CANFD0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_canfd0(void)
{
    CLK_DisableModuleClock(CANFD0_MODULE);

    return;
}

void nutool_modclkcfg_init_canfd1(void)
{
    CLK_EnableModuleClock(CANFD1_MODULE);

    return;
}

void nutool_modclkcfg_deinit_canfd1(void)
{
    CLK_DisableModuleClock(CANFD1_MODULE);

    return;
}

void nutool_modclkcfg_init_crc(void)
{
    CLK_EnableModuleClock(CRC_MODULE);

    return;
}

void nutool_modclkcfg_deinit_crc(void)
{
    CLK_DisableModuleClock(CRC_MODULE);

    return;
}

void nutool_modclkcfg_init_crpt(void)
{
    CLK_EnableModuleClock(CRYPTO_MODULE);

    return;
}

void nutool_modclkcfg_deinit_crpt(void)
{
    CLK_DisableModuleClock(CRYPTO_MODULE);

    return;
}

void nutool_modclkcfg_init_dac(void)
{
    CLK_EnableModuleClock(DAC_MODULE);

    return;
}

void nutool_modclkcfg_deinit_dac(void)
{
    CLK_DisableModuleClock(DAC_MODULE);

    return;
}

void nutool_modclkcfg_init_eadc(void)
{
    CLK_EnableModuleClock(EADC0_MODULE);
    CLK_SetModuleClock(EADC0_MODULE, CLK_CLKSEL0_EADC0SEL_HIRC, CLK_CLKDIV0_EADC0(2));

    return;
}

void nutool_modclkcfg_deinit_eadc(void)
{
    CLK_DisableModuleClock(EADC0_MODULE);

    return;
}

void nutool_modclkcfg_init_ebi(void)
{
    CLK_EnableModuleClock(EBI_MODULE);

    return;
}

void nutool_modclkcfg_deinit_ebi(void)
{
    CLK_DisableModuleClock(EBI_MODULE);

    return;
}

void nutool_modclkcfg_init_ecap0(void)
{
    CLK_EnableModuleClock(ECAP0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_ecap0(void)
{
    CLK_DisableModuleClock(ECAP0_MODULE);

    return;
}

void nutool_modclkcfg_init_ecap1(void)
{
    CLK_EnableModuleClock(ECAP1_MODULE);

    return;
}

void nutool_modclkcfg_deinit_ecap1(void)
{
    CLK_DisableModuleClock(ECAP1_MODULE);

    return;
}

void nutool_modclkcfg_init_epwm0(void)
{
    CLK_EnableModuleClock(EPWM0_MODULE);
    CLK_SetModuleClock(EPWM0_MODULE, CLK_CLKSEL2_EPWM0SEL_PCLK0, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_epwm0(void)
{
    CLK_DisableModuleClock(EPWM0_MODULE);

    return;
}

void nutool_modclkcfg_init_epwm1(void)
{
    CLK_EnableModuleClock(EPWM1_MODULE);
    CLK_SetModuleClock(EPWM1_MODULE, CLK_CLKSEL2_EPWM1SEL_PCLK1, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_epwm1(void)
{
    CLK_DisableModuleClock(EPWM1_MODULE);

    return;
}


void nutool_modclkcfg_init_pwm0(void)
{
    CLK_EnableModuleClock(PWM0_MODULE);
    CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL3_PWM0SEL_PCLK0, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_pwm0(void)
{
    CLK_DisableModuleClock(PWM0_MODULE);

    return;
}

void nutool_modclkcfg_init_pwm1(void)
{
    CLK_EnableModuleClock(PWM1_MODULE);
    CLK_SetModuleClock(PWM1_MODULE, CLK_CLKSEL3_PWM1SEL_PCLK1, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_pwm1(void)
{
    CLK_DisableModuleClock(PWM1_MODULE);

    return;
}

void nutool_modclkcfg_init_gpa(void)
{
    CLK_EnableModuleClock(GPA_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpa(void)
{
    CLK_DisableModuleClock(GPA_MODULE);

    return;
}

void nutool_modclkcfg_init_gpb(void)
{
    CLK_EnableModuleClock(GPB_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpb(void)
{
    CLK_DisableModuleClock(GPB_MODULE);

    return;
}

void nutool_modclkcfg_init_gpc(void)
{
    CLK_EnableModuleClock(GPC_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpc(void)
{
    CLK_DisableModuleClock(GPC_MODULE);

    return;
}

void nutool_modclkcfg_init_gpd(void)
{
    CLK_EnableModuleClock(GPD_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpd(void)
{
    CLK_DisableModuleClock(GPD_MODULE);

    return;
}

void nutool_modclkcfg_init_gpe(void)
{
    CLK_EnableModuleClock(GPE_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpe(void)
{
    CLK_DisableModuleClock(GPE_MODULE);

    return;
}

void nutool_modclkcfg_init_gpf(void)
{
    CLK_EnableModuleClock(GPF_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpf(void)
{
    CLK_DisableModuleClock(GPF_MODULE);

    return;
}

void nutool_modclkcfg_init_gpg(void)
{
    CLK_EnableModuleClock(GPG_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gpg(void)
{
    CLK_DisableModuleClock(GPG_MODULE);

    return;
}

void nutool_modclkcfg_init_gph(void)
{
    CLK_EnableModuleClock(GPH_MODULE);

    return;
}

void nutool_modclkcfg_deinit_gph(void)
{
    CLK_DisableModuleClock(GPH_MODULE);

    return;
}

void nutool_modclkcfg_init_i2c0(void)
{
    CLK_EnableModuleClock(I2C0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_i2c0(void)
{
    CLK_DisableModuleClock(I2C0_MODULE);

    return;
}

void nutool_modclkcfg_init_i2c1(void)
{
    CLK_EnableModuleClock(I2C1_MODULE);

    return;
}

void nutool_modclkcfg_deinit_i2c1(void)
{
    CLK_DisableModuleClock(I2C1_MODULE);

    return;
}

void nutool_modclkcfg_init_i2c2(void)
{
    CLK_EnableModuleClock(I2C2_MODULE);

    return;
}

void nutool_modclkcfg_deinit_i2c2(void)
{
    CLK_DisableModuleClock(I2C2_MODULE);

    return;
}

void nutool_modclkcfg_init_isp(void)
{
    CLK_EnableModuleClock(ISP_MODULE);

    return;
}

void nutool_modclkcfg_deinit_isp(void)
{
    CLK_DisableModuleClock(ISP_MODULE);

    return;
}

void nutool_modclkcfg_init_ks(void)
{
    CLK_EnableModuleClock(KS_MODULE);

    return;
}

void nutool_modclkcfg_deinit_ks(void)
{
    CLK_DisableModuleClock(KS_MODULE);

    return;
}

void nutool_modclkcfg_init_otg(void)
{
    CLK_EnableModuleClock(OTG_MODULE);
    CLK_SetModuleClock(OTG_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(3));

    return;
}

void nutool_modclkcfg_deinit_otg(void)
{
    CLK_DisableModuleClock(OTG_MODULE);

    return;
}

void nutool_modclkcfg_init_pdma0(void)
{
    CLK_EnableModuleClock(PDMA0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_pdma0(void)
{
    CLK_DisableModuleClock(PDMA0_MODULE);

    return;
}

void nutool_modclkcfg_init_lppdma0(void)
{
    CLK_EnableModuleClock(LPPDMA0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_lppdma0(void)
{
    CLK_DisableModuleClock(LPPDMA0_MODULE);

    return;
}

void nutool_modclkcfg_init_eqei0(void)
{
    CLK_EnableModuleClock(EQEI0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_eqei0(void)
{
    CLK_DisableModuleClock(EQEI0_MODULE);

    return;
}

void nutool_modclkcfg_init_eqei1(void)
{
    CLK_EnableModuleClock(EQEI1_MODULE);

    return;
}

void nutool_modclkcfg_deinit_eqei1(void)
{
    CLK_DisableModuleClock(EQEI1_MODULE);

    return;
}

void nutool_modclkcfg_init_qspi0(void)
{
    CLK_EnableModuleClock(QSPI0_MODULE);
    CLK_SetModuleClock(QSPI0_MODULE, CLK_CLKSEL2_QSPI0SEL_PCLK0, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_qspi0(void)
{
    CLK_DisableModuleClock(QSPI0_MODULE);

    return;
}

void nutool_modclkcfg_init_rtc(void)
{
    CLK_EnableModuleClock(RTC_MODULE);
    RTC->LXTCTL = RTC->LXTCTL & ~RTC_LXTCTL_RTCCKSEL_Msk;

    return;
}

void nutool_modclkcfg_deinit_rtc(void)
{
    CLK_DisableModuleClock(RTC_MODULE);

    return;
}

void nutool_modclkcfg_init_spi0(void)
{
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_spi0(void)
{
    CLK_DisableModuleClock(SPI0_MODULE);

    return;
}

void nutool_modclkcfg_init_spi1(void)
{
    CLK_EnableModuleClock(SPI1_MODULE);
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_spi1(void)
{
    CLK_DisableModuleClock(SPI1_MODULE);

    return;
}

void nutool_modclkcfg_init_spi2(void)
{
    CLK_EnableModuleClock(SPI2_MODULE);
    CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL3_SPI2SEL_PCLK1, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_spi2(void)
{
    CLK_DisableModuleClock(SPI2_MODULE);

    return;
}

void nutool_modclkcfg_init_spi3(void)
{
    CLK_EnableModuleClock(SPI3_MODULE);
    CLK_SetModuleClock(SPI3_MODULE, CLK_CLKSEL3_SPI3SEL_PCLK0, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_spi3(void)
{
    CLK_DisableModuleClock(SPI3_MODULE);

    return;
}


void nutool_modclkcfg_init_systick(void)
{
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, 0);

    return;
}

void nutool_modclkcfg_deinit_systick(void)
{
    CLK_DisableSysTick();

    return;
}

void nutool_modclkcfg_init_tmr0(void)
{
    CLK_EnableModuleClock(TMR0_MODULE);

#if defined(BSP_USING_TPWM0)
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_PCLK0, MODULE_NoMsk);
#else
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, MODULE_NoMsk);
#endif

    return;
}

void nutool_modclkcfg_deinit_tmr0(void)
{
    CLK_DisableModuleClock(TMR0_MODULE);

    return;
}

void nutool_modclkcfg_init_tmr1(void)
{
    CLK_EnableModuleClock(TMR1_MODULE);

#if defined(BSP_USING_TPWM1)
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_PCLK0, MODULE_NoMsk);
#else
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, MODULE_NoMsk);
#endif

    return;
}

void nutool_modclkcfg_deinit_tmr1(void)
{
    CLK_DisableModuleClock(TMR1_MODULE);

    return;
}

void nutool_modclkcfg_init_tmr2(void)
{
    CLK_EnableModuleClock(TMR2_MODULE);

#if defined(BSP_USING_TPWM2)
    CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_PCLK1, MODULE_NoMsk);
#else
    CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HIRC, MODULE_NoMsk);
#endif

    return;
}

void nutool_modclkcfg_deinit_tmr2(void)
{
    CLK_DisableModuleClock(TMR2_MODULE);

    return;
}

void nutool_modclkcfg_init_tmr3(void)
{
    CLK_EnableModuleClock(TMR3_MODULE);

#if defined(BSP_USING_TPWM3)
    CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_PCLK1, MODULE_NoMsk);
#else
    CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_HIRC, MODULE_NoMsk);
#endif

    return;
}

void nutool_modclkcfg_deinit_tmr3(void)
{
    CLK_DisableModuleClock(TMR3_MODULE);

    return;
}

void nutool_modclkcfg_init_trng(void)
{
    CLK_EnableModuleClock(TRNG_MODULE);

    return;
}

void nutool_modclkcfg_deinit_trng(void)
{
    CLK_DisableModuleClock(TRNG_MODULE);

    return;
}

void nutool_modclkcfg_init_uart0(void)
{
    CLK_EnableModuleClock(UART0_MODULE);

    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL4_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    return;
}

void nutool_modclkcfg_deinit_uart0(void)
{
    CLK_DisableModuleClock(UART0_MODULE);

    return;
}

void nutool_modclkcfg_init_uart1(void)
{
    CLK_EnableModuleClock(UART1_MODULE);
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL4_UART1SEL_HIRC, CLK_CLKDIV0_UART1(1));

    return;
}

void nutool_modclkcfg_deinit_uart1(void)
{
    CLK_DisableModuleClock(UART1_MODULE);

    return;
}

void nutool_modclkcfg_init_uart2(void)
{
    CLK_EnableModuleClock(UART2_MODULE);
    CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL4_UART2SEL_HIRC, CLK_CLKDIV4_UART2(1));

    return;
}

void nutool_modclkcfg_deinit_uart2(void)
{
    CLK_DisableModuleClock(UART2_MODULE);

    return;
}

void nutool_modclkcfg_init_uart3(void)
{
    CLK_EnableModuleClock(UART3_MODULE);
    CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL4_UART3SEL_HIRC, CLK_CLKDIV4_UART3(1));

    return;
}

void nutool_modclkcfg_deinit_uart3(void)
{
    CLK_DisableModuleClock(UART3_MODULE);

    return;
}

void nutool_modclkcfg_init_uart4(void)
{
    CLK_EnableModuleClock(UART4_MODULE);
    CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL4_UART4SEL_HIRC, CLK_CLKDIV4_UART4(1));

    return;
}

void nutool_modclkcfg_deinit_uart4(void)
{
    CLK_DisableModuleClock(UART4_MODULE);

    return;
}

void nutool_modclkcfg_init_uart5(void)
{
    CLK_EnableModuleClock(UART5_MODULE);
    CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL4_UART5SEL_HIRC, CLK_CLKDIV4_UART5(1));

    return;
}

void nutool_modclkcfg_deinit_uart5(void)
{
    CLK_DisableModuleClock(UART5_MODULE);

    return;
}

void nutool_modclkcfg_init_uart6(void)
{
    CLK_EnableModuleClock(UART6_MODULE);
    CLK_SetModuleClock(UART6_MODULE, CLK_CLKSEL4_UART6SEL_HIRC, CLK_CLKDIV4_UART6(1));

    return;
}

void nutool_modclkcfg_deinit_uart6(void)
{
    CLK_DisableModuleClock(UART6_MODULE);

    return;
}

void nutool_modclkcfg_init_uart7(void)
{
    CLK_EnableModuleClock(UART7_MODULE);
    CLK_SetModuleClock(UART7_MODULE, CLK_CLKSEL4_UART7SEL_HIRC, CLK_CLKDIV4_UART7(1));

    return;
}

void nutool_modclkcfg_deinit_uart7(void)
{
    CLK_DisableModuleClock(UART7_MODULE);

    return;
}

void nutool_modclkcfg_init_usbd(void)
{
    CLK_EnableModuleClock(USBD_MODULE);
    //CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC48M, CLK_CLKDIV0_USB(1));
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(3)); // PLL must be (48*(1+N)) MHz

    return;
}

void nutool_modclkcfg_deinit_usbd(void)
{
    CLK_DisableModuleClock(USBD_MODULE);
}

void nutool_modclkcfg_init_usbh(void)
{
    CLK_EnableModuleClock(USBH_MODULE);
    //CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC48M, CLK_CLKDIV0_USB(1));
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(3)); // PLL must be (48*(1+N)) MHz

    return;
}

void nutool_modclkcfg_deinit_usbh(void)
{
    CLK_DisableModuleClock(USBH_MODULE);

    return;
}

void nutool_modclkcfg_init_usci0(void)
{
    CLK_EnableModuleClock(USCI0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_usci0(void)
{
    CLK_DisableModuleClock(USCI0_MODULE);

    return;
}

void nutool_modclkcfg_init_usci1(void)
{
    CLK_EnableModuleClock(USCI1_MODULE);

    return;
}

void nutool_modclkcfg_deinit_usci1(void)
{
    CLK_DisableModuleClock(USCI1_MODULE);

    return;
}

void nutool_modclkcfg_init_wdt(void)
{
    CLK_EnableModuleClock(WDT_MODULE);
    CLK_SetModuleClock(WDT_MODULE, LPSCC_CLKSEL0_WDTSEL_LXT, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_wdt(void)
{
    CLK_DisableModuleClock(WDT_MODULE);

    return;
}

void nutool_modclkcfg_init_lpadc0(void)
{
    CLK_EnableModuleClock(LPADC0_MODULE);
    CLK_SetModuleClock(LPADC0_MODULE, LPSCC_CLKSEL0_LPADC0SEL_HIRC, LPSCC_CLKDIV0_LPADC0(8));

    return;
}

void nutool_modclkcfg_deinit_lpadc0(void)
{
    CLK_DisableModuleClock(LPADC0_MODULE);

    return;
}

void nutool_modclkcfg_init_lpgpio(void)
{
    CLK_EnableModuleClock(LPGPIO_MODULE);

    return;
}

void nutool_modclkcfg_deinit_lpgpio(void)
{
    CLK_DisableModuleClock(LPGPIO_MODULE);

    return;
}

void nutool_modclkcfg_init_lpi2c0(void)
{
    CLK_EnableModuleClock(LPI2C0_MODULE);

    return;
}

void nutool_modclkcfg_deinit_lpi2c0(void)
{
    CLK_DisableModuleClock(LPI2C0_MODULE);

    return;
}

void nutool_modclkcfg_init_lpspi0(void)
{
    CLK_EnableModuleClock(LPSPI0_MODULE);
    CLK_SetModuleClock(LPSPI0_MODULE, LPSCC_CLKSEL0_LPSPI0SEL_HIRC, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_lpspi0(void)
{
    CLK_DisableModuleClock(LPSPI0_MODULE);

    return;
}

void nutool_modclkcfg_init_lptmr0(void)
{
    CLK_EnableModuleClock(LPTMR0_MODULE);
    CLK_SetModuleClock(LPTMR0_MODULE, LPSCC_CLKSEL0_LPTMR0SEL_HIRC, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_lptmr0(void)
{
    CLK_DisableModuleClock(LPTMR0_MODULE);

    return;
}

void nutool_modclkcfg_init_lptmr1(void)
{
    CLK_EnableModuleClock(LPTMR1_MODULE);
    CLK_SetModuleClock(LPTMR1_MODULE, LPSCC_CLKSEL0_LPTMR1SEL_HIRC, MODULE_NoMsk);

    return;
}

void nutool_modclkcfg_deinit_lptmr1(void)
{
    CLK_DisableModuleClock(LPTMR1_MODULE);

    return;
}

void nutool_modclkcfg_init_lpuart0(void)
{
    CLK_EnableModuleClock(LPUART0_MODULE);
    CLK_SetModuleClock(LPUART0_MODULE, LPSCC_CLKSEL0_LPUART0SEL_HIRC, LPSCC_CLKDIV0_LPUART0(1));

    return;
}

void nutool_modclkcfg_deinit_lpuart0(void)
{
    CLK_DisableModuleClock(LPUART0_MODULE);

    return;
}

void nutool_modclkcfg_init_base(void)
{
    /* Enable HIRC/HXT/LXT/LIRC/HIRC48M clocks */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk | CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_HIRC48MEN_Msk);

    /* Wait for HIRC/HXT/LXT/LIRC/HIRC48M clocks ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_LIRCSTB_Msk | CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_HIRC48MSTB_Msk);

    /* Set PLL to 144MHz */
    CLK_EnablePLL(CLK_PLLCTL_PLLSRC_HXT, FREQ_144MHZ);

    /* Set CPU clock to 72MHz */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));

    /* Set HCLK1 source to HIRC */
    CLK_SetModuleClock(HCLK1_MODULE, CLK_CLKSEL0_HCLK1SEL_HIRC48M_DIV2, LPSCC_CLKDIV0_HCLK1(1));

    /* Enable HCLK1 clock */
    CLK_EnableModuleClock(HCLK1_MODULE);

    /* Set PCLK0 = PCLK1 = HCLK */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV1 | CLK_PCLKDIV_APB1DIV_DIV1);

    /* Set PCLK0 = PCLK1 = HCLK/2 */
    //CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    SystemCoreClockUpdate();

    return;
}

void Reset_Handler_PreInit(void)
{
    nutool_modclkcfg_init_base();
}

void nutool_modclkcfg_init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

#if defined(BSP_USING_GPIO)
    nutool_modclkcfg_init_gpa();
    nutool_modclkcfg_init_gpb();
    nutool_modclkcfg_init_gpc();
    nutool_modclkcfg_init_gpd();
    nutool_modclkcfg_init_gpe();
    nutool_modclkcfg_init_gpf();
    nutool_modclkcfg_init_gpg();
    nutool_modclkcfg_init_gph();
#endif
#if defined(BSP_USING_ACMP01)
    nutool_modclkcfg_init_acmp01();
#endif
#if defined(BSP_USING_CANFD0)
    nutool_modclkcfg_init_canfd0();
#endif
#if defined(BSP_USING_CANFD1)
    nutool_modclkcfg_init_canfd1();
#endif
#if defined(BSP_USING_CRC)
    nutool_modclkcfg_init_crc();
#endif
#if defined(BSP_USING_CRYPTO)
    nutool_modclkcfg_init_crpt();
#endif
#if defined(BSP_USING_DAC)
    nutool_modclkcfg_init_dac();
#endif
#if defined(BSP_USING_EADC)
    nutool_modclkcfg_init_eadc();
#endif
#if defined(BSP_USING_EBI)
    nutool_modclkcfg_init_ebi();
#endif
#if defined(BSP_USING_ECAP0)
    nutool_modclkcfg_init_ecap0();
#endif
#if defined(BSP_USING_ECAP1)
    nutool_modclkcfg_init_ecap1();
#endif
#if defined(BSP_USING_EPWM0)
    nutool_modclkcfg_init_epwm0();
#endif
#if defined(BSP_USING_EPWM1)
    nutool_modclkcfg_init_epwm1();
#endif
#if defined(BSP_USING_PWM0)
    nutool_modclkcfg_init_pwm0();
#endif
#if defined(BSP_USING_PWM1)
    nutool_modclkcfg_init_pwm1();
#endif
#if defined(BSP_USING_I2C0)
    nutool_modclkcfg_init_i2c0();
#endif
#if defined(BSP_USING_I2C1)
    nutool_modclkcfg_init_i2c1();
#endif
#if defined(BSP_USING_I2C2)
    nutool_modclkcfg_init_i2c2();
#endif
#if defined(BSP_USING_OTG)
    nutool_modclkcfg_init_otg();
#endif
#if defined(BSP_USING_PDMA)
    nutool_modclkcfg_init_pdma0();
    nutool_modclkcfg_init_lppdma0();
#endif
#if defined(BSP_USING_EQEI0)
    nutool_modclkcfg_init_eqei0();
#endif
#if defined(BSP_USING_EQEI1)
    nutool_modclkcfg_init_eqei1();
#endif
#if defined(BSP_USING_QSPI0)
    nutool_modclkcfg_init_qspi0();
#endif
#if defined(BSP_USING_RTC)
    nutool_modclkcfg_init_rtc();
#endif
#if defined(BSP_USING_SCUART0)
    nutool_modclkcfg_init_sc0();
#endif
#if defined(BSP_USING_SPI0) || defined(BSP_USING_SPII2S0)
    nutool_modclkcfg_init_spi0();
#endif
#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPII2S1)
    nutool_modclkcfg_init_spi1();
#endif
#if defined(BSP_USING_SPI2) || defined(BSP_USING_SPII2S2)
    nutool_modclkcfg_init_spi2();
#endif
#if defined(BSP_USING_SPI3) || defined(BSP_USING_SPII2S3)
    nutool_modclkcfg_init_spi3();
#endif
#if defined(BSP_USING_TMR0)
    nutool_modclkcfg_init_tmr0();
#endif
#if defined(BSP_USING_TMR1)
    nutool_modclkcfg_init_tmr1();
#endif
#if defined(BSP_USING_TMR2)
    nutool_modclkcfg_init_tmr2();
#endif
#if defined(BSP_USING_TMR3)
    nutool_modclkcfg_init_tmr3();
#endif
#if defined(BSP_USING_TRNG)
    nutool_modclkcfg_init_trng();
#endif
#if defined(BSP_USING_UART0)
    nutool_modclkcfg_init_uart0();
#endif
#if defined(BSP_USING_UART1)
    nutool_modclkcfg_init_uart1();
#endif
#if defined(BSP_USING_UART2)
    nutool_modclkcfg_init_uart2();
#endif
#if defined(BSP_USING_UART3)
    nutool_modclkcfg_init_uart3();
#endif
#if defined(BSP_USING_UART4)
    nutool_modclkcfg_init_uart4();
#endif
#if defined(BSP_USING_UART5)
    nutool_modclkcfg_init_uart5();
#endif
#if defined(BSP_USING_UART6)
    nutool_modclkcfg_init_uart6();
#endif
#if defined(BSP_USING_UART7)
    nutool_modclkcfg_init_uart7();
#endif
#if defined(BSP_USING_USBD) || defined(BSP_USING_USBH)
    nutool_modclkcfg_init_usbd();
#endif
#if defined(BSP_USING_USBH)
    nutool_modclkcfg_init_usbh();
#endif
#if defined(BSP_USING_USCI0)
    nutool_modclkcfg_init_usci0();
#endif
#if defined(BSP_USING_USCI1)
    nutool_modclkcfg_init_usci1();
#endif
#if defined(BSP_USING_WDT)
    nutool_modclkcfg_init_wdt();
#endif
#if defined(BSP_USING_LPADC0)
    nutool_modclkcfg_init_lpadc0();
#endif
#if defined(BSP_USING_LPGPIO)
    nutool_modclkcfg_init_lpgpio();
#endif
#if defined(BSP_USING_LPI2C0)
    nutool_modclkcfg_init_lpi2c0();
#endif
#if defined(BSP_USING_LPSPI0)
    nutool_modclkcfg_init_lpspi0();
#endif
#if defined(BSP_USING_LPTMR0)
    nutool_modclkcfg_init_lptmr0();
#endif
#if defined(BSP_USING_LPTMR1)
    nutool_modclkcfg_init_lptmr1();
#endif
#if defined(BSP_USING_LPUART0)
    nutool_modclkcfg_init_lpuart0();
#endif

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    return;
}

/*** (C) COPYRIGHT 2013-2020 Nuvoton Technology Corp. ***/
