/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined (BSP_USING_CLK)

#include "rtdevice.h"

#include "NuMicro.h"


#ifndef NU_CLK_INVOKE_WKTMR
    #warning "no wake-up timer invoked. The standby and shutdown mode will not wakeup."
#endif


#define NU_UTEST_PM_NONE_RESET_CASE_ONLY        (0)
#define NU_UTEST_PM_ADD_STANDBY_RESET_CASE      (1)
#define NU_UTEST_PM_ADD_SHUTDOWN_RESET_CASE     (2)

/*--------------------------------------------------------------------------------- */
/* configure NU_UTEST_PM_CASE and NU_UTEST_PM_USE_TIMER_CHECK_SLEEP parameters      */
/*--------------------------------------------------------------------------------- */
/* select a test case */
//#define NU_UTEST_PM_CASE                        (NU_UTEST_PM_NONE_RESET_CASE_ONLY)
//#define NU_UTEST_PM_CASE                        (NU_UTEST_PM_ADD_STANDBY_RESET_CASE)
#define NU_UTEST_PM_CASE                        (NU_UTEST_PM_ADD_SHUTDOWN_RESET_CASE)

/* use a timer to check power down if it is needed */
#define NU_UTEST_PM_USE_TIMER_CHECK_SLEEP

/*--------------------------------------------------------------------------------- */


#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)

    #define _CONCAT2_(x, y)                 x##y
    #define _CONCAT3_(x, y, z)              x##y##z
    #define CONCAT2(x, y)                   _CONCAT2_(x, y)
    #define CONCAT3(x, y, z)                _CONCAT3_(x,y,z)

    /* The UTEST_TIMER_INSTANCE is a timer taken by utest_clk.c to auto-verify the pm sleep.
    * Take care that default TIMERn configuration is modified in utest_clk.c if this macro
    * is expanded. The utest will force the timer clock source to be fixed at HXT, and this
    * will cause the timer counter halt in power down mode. As a result, the counter value
    * can be used as an indicator to check whether system has indeed entered the power down or not.
    */
    #define UTEST_TIMER_INSTANCE            2
    #define UTEST_TIMER_PRESCALE            (119)
    #define UTEST_TIMER_TICK_PER_SECOND     (__HXT / (UTEST_TIMER_PRESCALE + 1))

    /* Concatenate the macros of timer instance for driver usage. */
    #define UTEST_TIMER                     CONCAT2(TIMER, UTEST_TIMER_INSTANCE)
    #define UTEST_TMR                       CONCAT2(TMR, UTEST_TIMER_INSTANCE)
    #define UTEST_TMR_MODULE                CONCAT2(UTEST_TMR, _MODULE)
    #define UTEST_TMR_SEL_HXT               CONCAT3(CLK_CLKSEL1_, UTEST_TMR, SEL_HXT)
    #define UTEST_TMR_RST                   CONCAT2(UTEST_TMR, _RST)
    #define UTEST_TMR_CKEN_Msk              CONCAT3(CLK_APBCLK0_, UTEST_TMR, CKEN_Msk)
    #define UTEST_TMR_CLKSEL_Msk            CONCAT3(CLK_CLKSEL1_, UTEST_TMR, SEL_Msk)
#endif



static void test_pm_sleep_mode(void);
static rt_err_t utest_tc_init(void);
static rt_err_t utest_tc_cleanup(void);
static void testcase(void);

#if (NU_UTEST_PM_CASE == NU_UTEST_PM_ADD_STANDBY_RESET_CASE)
    static void test_pm_standby_mode(void);
#endif

#if (NU_UTEST_PM_CASE == NU_UTEST_PM_ADD_SHUTDOWN_RESET_CASE)
    static void test_pm_shutdown_mode(void);
#endif

static void clk_dump_test_setting(void)
{
    rt_kprintf("\n[CLK utest] description\n");
    rt_kprintf("  purpose               : Verify PM run/sleep behavior and optional reset paths.\n");
    rt_kprintf("  pm test case          : %d\n", NU_UTEST_PM_CASE);
#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)
    rt_kprintf("  timer sleep check     : enabled\n");
    rt_kprintf("  timer instance        : %d\n", UTEST_TIMER_INSTANCE);
    rt_kprintf("  timer prescale        : %d\n", UTEST_TIMER_PRESCALE);
    rt_kprintf("  timer tick per sec    : %d\n", UTEST_TIMER_TICK_PER_SECOND);
#else
    rt_kprintf("  timer sleep check     : disabled\n");
#endif
#ifdef NU_CLK_INVOKE_WKTMR
    rt_kprintf("  wake-up timer         : enabled\n\n");
#else
    rt_kprintf("  wake-up timer         : disabled\n\n");
#endif
}


#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)

    static void utest_timer_init(void);
    static void utest_timer_deinit(void);
    static void utest_timer_start(void);
    static void utest_timer_stop(void);
    static void utest_timer_reset(void);
    static uint32_t utest_timer_get_tick(void);
#endif

struct backup
{
    uint32_t clko_mfp;
    uint32_t clko_clock_select;
    uint32_t clko_clock_enable;
    uint32_t clko_ctl;
    uint32_t timer_ctl;
    uint32_t timer_cmp;
    uint32_t timer_cnt;
    uint32_t timer_clock_select;
    uint32_t timer_clock_enable;
};

static struct backup backup;

static void utest_register_store(void)
{
    backup.clko_mfp = SYS->GPC_MFP3 & SYS_GPC_MFP3_PC13MFP_Msk;
    backup.clko_clock_enable = CLK->APBCLK0 & CLK_APBCLK0_CLKOCKEN_Msk;
    backup.clko_clock_select = CLK->CLKSEL1 & CLK_CLKSEL1_CLKOSEL_Msk;
    backup.clko_ctl = CLK->CLKOCTL;

#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)

    backup.timer_clock_enable = CLK->APBCLK0 & UTEST_TMR_CKEN_Msk;
    backup.timer_clock_select = CLK->CLKSEL1 & UTEST_TMR_CLKSEL_Msk;

    CLK_EnableModuleClock(UTEST_TMR_MODULE);
    backup.timer_ctl = UTEST_TIMER->CTL;
    backup.timer_cnt = UTEST_TIMER->CNT;
    backup.timer_cmp = UTEST_TIMER->CMP;
#endif

}

static void utest_register_recovery(void)
{
    SYS_UnlockReg();

    SYS->GPC_MFP3 = (SYS->GPC_MFP3 & ~SYS_GPC_MFP3_PC13MFP_Msk) | backup.clko_mfp;
    CLK->APBCLK0 = (CLK->APBCLK0 & ~CLK_APBCLK0_CLKOCKEN_Msk) | backup.clko_clock_enable;
    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_CLKOSEL_Msk) | backup.clko_clock_select;
    CLK->CLKOCTL = backup.clko_ctl;

#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)

    UTEST_TIMER->CTL = backup.timer_ctl;
    UTEST_TIMER->CNT = backup.timer_cnt;
    UTEST_TIMER->CMP = backup.timer_cmp;

    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~UTEST_TMR_CLKSEL_Msk) | backup.timer_clock_select;
    CLK->APBCLK0 = (CLK->APBCLK0 & ~UTEST_TMR_CKEN_Msk) | backup.timer_clock_enable;
#endif
}

static void add_test_delay(void)
{
    const rt_uint32_t elapse_tick = 100;

    rt_thread_delay(elapse_tick);
}

static void utest_enable_clko(void)
{
    CLK_EnableCKO(CLK_CLKSEL1_CLKOSEL_HCLK, 3, 0);
    SYS->GPC_MFP3 = (SYS->GPC_MFP3 & ~SYS_GPC_MFP3_PC13MFP_Msk) | SYS_GPC_MFP3_PC13MFP_CLKO;
}

static void utest_disable_clko(void)
{
    CLK_DisableCKO();
}

#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)

static void utest_timer_init(void)
{
    CLK_SetModuleClock(UTEST_TMR_MODULE, UTEST_TMR_SEL_HXT, 0);
    CLK_EnableModuleClock(UTEST_TMR_MODULE);
    TIMER_Open(UTEST_TIMER, TIMER_CONTINUOUS_MODE, 1);
    TIMER_SET_PRESCALE_VALUE(UTEST_TIMER, UTEST_TIMER_PRESCALE);
}

static void utest_timer_deinit(void)
{
    SYS_ResetModule(UTEST_TMR_RST);
}

static void utest_timer_start(void)
{
    TIMER_Start(UTEST_TIMER);
}

static void utest_timer_stop(void)
{
    TIMER_Stop(UTEST_TIMER);
}

static void utest_timer_reset(void)
{
    TIMER_ResetCounter(UTEST_TIMER);
}

static uint32_t utest_timer_get_tick(void)
{
    rt_uint32_t ret, timer_tick;


    timer_tick = TIMER_GetCounter(UTEST_TIMER);

    /* convert to the unit of os tick */
    ret = timer_tick * RT_TICK_PER_SECOND / UTEST_TIMER_TICK_PER_SECOND;

    return ret;
}
#endif


static void test_pm_run_mode(void)
{
    const uint32_t default_speed = __HSI;
    const uint32_t high_speed    = __HSI;
    const uint32_t normal_speed  = __HSI;
    const uint32_t medium_speed  = (__HSI / 2);
    const uint32_t low_speed     = (__HSI / 2);
    rt_uint8_t mode;
    rt_uint32_t hz, elapse_tick = 1000;

    mode = PM_RUN_MODE_HIGH_SPEED;
    rt_pm_run_enter(mode);
    rt_thread_delay(elapse_tick);
    hz = CLK_GetCPUFreq();
    rt_kprintf("hz=%d. speed = %d\n\r", hz, high_speed);
    uassert_true(hz == high_speed);

    mode = PM_RUN_MODE_NORMAL_SPEED;
    rt_pm_run_enter(mode);
    rt_thread_delay(elapse_tick);
    hz = CLK_GetCPUFreq();
    rt_kprintf("hz=%d. speed = %d\n\r", hz, normal_speed);
    uassert_true(hz == normal_speed);

    mode = PM_RUN_MODE_MEDIUM_SPEED;
    rt_pm_run_enter(mode);
    rt_thread_delay(elapse_tick);
    hz = CLK_GetCPUFreq();
    rt_kprintf("hz=%d. speed = %d\n\r", hz, medium_speed);
    uassert_true(hz == medium_speed);

    mode = PM_RUN_MODE_LOW_SPEED;
    rt_pm_run_enter(mode);
    rt_thread_delay(elapse_tick);
    hz = CLK_GetCPUFreq();
    rt_kprintf("hz=%d. speed = %d\n\r", hz, low_speed);
    uassert_true(hz == low_speed);

    /* Resume to default */
    mode = RT_PM_DEFAULT_RUN_MODE;
    rt_pm_run_enter(mode);
    rt_thread_delay(elapse_tick);
    hz = CLK_GetCPUFreq();
    rt_kprintf("hz=%d. speed = %d\n\r", hz, default_speed);
    uassert_true(hz == default_speed);
}


#if defined (NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)
static void test_pm_sleep_mode(void)
{
    rt_uint8_t mode;
    rt_tick_t tick_before, tick_after, timer_before, timer_after;
    const rt_uint32_t elapse_tick = 1000;
    const rt_uint32_t error_tolerance = 80;

    utest_timer_start();

    /* Test none sleep mode = run */
    mode = PM_SLEEP_MODE_NONE;
    rt_pm_request(mode);
    timer_before = utest_timer_get_tick();
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    timer_after = utest_timer_get_tick();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    uassert_in_range(timer_before + elapse_tick, timer_after - error_tolerance, timer_after + error_tolerance);
    rt_kprintf("timer_before:%d  timer_after:%d\n", timer_before, timer_after);
    utest_timer_reset();
    add_test_delay();

    /* Test idle mode = run */
    mode = PM_SLEEP_MODE_IDLE;
    rt_pm_request(mode);

    tick_before = rt_tick_get();
    timer_before = utest_timer_get_tick();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    timer_after = utest_timer_get_tick();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    uassert_in_range(timer_before + elapse_tick, timer_after - error_tolerance, timer_after + error_tolerance);
    rt_kprintf("timer_before:%d  timer_after:%d\n", timer_before, timer_after);
    utest_timer_reset();
    add_test_delay();


    /* Test light sleep mode = fast-wakeup power down.      */
    /* Test tick compensation from pm timer.                */
    /* Test timer tick is halted to auto verify power down. */
    mode = PM_SLEEP_MODE_LIGHT;
    rt_pm_request(mode);
    timer_before = utest_timer_get_tick();
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    timer_after = utest_timer_get_tick();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    uassert_not_in_range(timer_before, timer_after - error_tolerance, timer_after + error_tolerance);
    rt_kprintf("timer_before:%d  timer_after:%d\n", timer_before, timer_after);
    utest_timer_reset();
    add_test_delay();


    /* Test deep sleep mode = normal power down.            */
    /* Test tick compensation from pm timer.                */
    /* Test timer tick is halted to auto verify power down. */
    mode = PM_SLEEP_MODE_DEEP;
    rt_pm_request(mode);
    timer_before = utest_timer_get_tick();
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    timer_after = utest_timer_get_tick();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    uassert_not_in_range(timer_before, timer_after - error_tolerance, timer_after + error_tolerance);
    rt_kprintf("timer_before:%d  timer_after:%d\n", timer_before, timer_after);
    add_test_delay();

    utest_timer_stop();
}
#else

static void test_pm_sleep_mode(void)
{
    rt_uint8_t mode;
    rt_tick_t tick_before, tick_after;
    const rt_uint32_t elapse_tick = 1000;
    const rt_uint32_t error_tolerance = 5;


    /* Test none sleep mode = run */
    mode = PM_SLEEP_MODE_NONE;
    rt_pm_request(mode);
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    add_test_delay();

    /* Test idle mode = run */
    mode = PM_SLEEP_MODE_IDLE;
    rt_pm_request(mode);
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    add_test_delay();

    /* Test light sleep mode = fast-wakeup power down. Test tick compensation from pm timer. */
    mode = PM_SLEEP_MODE_LIGHT;
    rt_pm_request(mode);
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    add_test_delay();

    /* Test deep sleep mode = normal power down. Test tick compensation from pm timer. */
    mode = PM_SLEEP_MODE_DEEP;
    rt_pm_request(mode);
    tick_before = rt_tick_get();
    rt_thread_delay(elapse_tick);
    tick_after = rt_tick_get();
    rt_pm_release(mode);
    uassert_in_range(tick_before + elapse_tick, tick_after - error_tolerance, tick_after + error_tolerance);
    add_test_delay();
}
#endif


#if (NU_UTEST_PM_CASE == NU_UTEST_PM_ADD_STANDBY_RESET_CASE)
static void test_pm_standby_mode(void)
{
    rt_uint8_t mode;
    const rt_uint32_t elapse_tick = 1000;

    LOG_W("Test pm standby sleep mode (SPD). Wait about 4 seconds to check system reset...");

    /* Test standby sleep mode = SPD */
    mode = PM_SLEEP_MODE_STANDBY;
    rt_pm_request(mode);

    /* Release all boxes */
    for (int i = (mode - 1); i >= 0; i--)
        rt_pm_release_all(i);

    rt_thread_delay(elapse_tick);

    /* System should reset and never reach here. */
    uassert_true(0);
}
#endif


#if (NU_UTEST_PM_CASE == NU_UTEST_PM_ADD_SHUTDOWN_RESET_CASE)
static void test_pm_shutdown_mode(void)
{
    rt_uint8_t mode;
    const rt_uint32_t elapse_tick = 1000;

    LOG_W("Test pm shutdown sleep mode (DPD). Wait about 6.6 sec to check system reset...");

    /* Test shutdown sleep mode =  */
    mode = PM_SLEEP_MODE_SHUTDOWN;
    rt_pm_request(mode);

    /* Release all boxes */
    for (int i = (mode - 1); i >= 0; i--)
        rt_pm_release_all(i);

    rt_thread_delay(elapse_tick);

    /* System should reset and never reach here. */
    uassert_true(0);
}
#endif



static rt_err_t utest_tc_init(void)
{
    clk_dump_test_setting();
    utest_register_store();
    utest_enable_clko();

#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)
    utest_timer_init();
#endif

    return RT_EOK;
}


static rt_err_t utest_tc_cleanup(void)
{
#if defined(NU_UTEST_PM_USE_TIMER_CHECK_SLEEP)
    utest_timer_deinit();
#endif

    utest_disable_clko();
    utest_register_recovery();

    return RT_EOK;
}


static void testcase(void)
{
    UTEST_UNIT_RUN(test_pm_run_mode);
    UTEST_UNIT_RUN(test_pm_sleep_mode);

#if (NU_UTEST_PM_CASE == NU_UTEST_PM_ADD_STANDBY_RESET_CASE)
    UTEST_UNIT_RUN(test_pm_standby_mode);
#endif

#if (NU_UTEST_PM_CASE == NU_UTEST_PM_ADD_SHUTDOWN_RESET_CASE)
    UTEST_UNIT_RUN(test_pm_shutdown_mode);
#endif

}


UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"pm",
                utest_tc_init, utest_tc_cleanup, 10);


#endif /* BSP_USING_CLK */





