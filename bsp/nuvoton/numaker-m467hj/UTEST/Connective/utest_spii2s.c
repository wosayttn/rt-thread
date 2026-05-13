/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(NU_PKG_USING_NAU88L25) && defined(PKG_USING_WAVPLAYER) && defined(BSP_USING_SPII2S1)

#include "rtdevice.h"
#include "NuMicro.h"
#include "wavrecorder.h"
#include "wavplayer.h"
#include "dfs_posix.h"
#include "acodec_nau88l25.h"

#define DEF_SMPLRATE 16000
#define DEF_SMPLBIT 16
#define DEF_CHNUM 2
#define DEF_RECORDING_TIME  (10*RT_TICK_PER_SECOND)

S_NU_NAU88L25_CONFIG sCodecConfig_SPII2S =
{
    .i2c_bus_name = "i2c1",
    .i2s_bus_name = "spii2s1",
    .pin_phonejack_en = 0,
    .pin_phonejack_det = 0,
};

static void spii2s_dump_test_setting(void)
{
    rt_kprintf("\n[SPII2S utest] description\n");
    rt_kprintf("  purpose               : Verify audio record/playback through SPII2S and NAU88L25.\n");
    rt_kprintf("  sample rate           : %d\n", DEF_SMPLRATE);
    rt_kprintf("  sample bits           : %d\n", DEF_SMPLBIT);
    rt_kprintf("  channel count         : %d\n", DEF_CHNUM);
    rt_kprintf("  recording time (tick) : %d\n\n", DEF_RECORDING_TIME);
}

static void test_audio_record(void)
{
    struct wavrecord_info info;
    char strbuf[64];
    rt_bool_t bIsActived = FALSE;

    info.samplerate = DEF_SMPLRATE;
    info.samplebits = DEF_SMPLBIT;
    info.channels = DEF_CHNUM;

    snprintf(strbuf, sizeof(strbuf), "/%d_%d_%d.wav", info.samplerate, info.samplebits, info.channels);
    info.uri = strbuf;

    wavrecorder_start(&info);

    rt_thread_mdelay(RT_TICK_PER_SECOND);

    bIsActived = wavrecorder_is_actived();

    uassert_true(bIsActived);
    if (bIsActived)
    {
        rt_kprintf("Recording file: %s\n", strbuf);
        rt_thread_mdelay(DEF_RECORDING_TIME);
        wavrecorder_stop();
    }
}

static void test_audio_playback(void)
{
    char strbuf[64];
    struct wavrecord_info info;
    struct stat stat_buf;
    rt_bool_t bIsExist = FALSE;

    info.samplerate = DEF_SMPLRATE;
    info.samplebits = DEF_SMPLBIT;
    info.channels = DEF_CHNUM;

    snprintf(strbuf, sizeof(strbuf), "/%d_%d_%d.wav", info.samplerate, info.samplebits, info.channels);
    info.uri = strbuf;

    bIsExist = (stat((const char *)strbuf, &stat_buf) >= 0);
    uassert_true(bIsExist);
    if (bIsExist)
    {

        rt_kprintf("Playback file: %s\n", strbuf);

        wavplayer_play(strbuf);

        rt_thread_mdelay(DEF_RECORDING_TIME);

        wavplayer_stop();
    }
}

static rt_err_t utest_tc_init(void)
{
    spii2s_dump_test_setting();
    // Backup original multiple function pin setting.
    SET_I2C1_SDA_PG3();
    SET_I2C1_SCL_PG2();

    //GPIO_SetPullCtl(PG, BIT2 | BIT3, GPIO_PUSEL_PULL_UP);

    SET_SPI1_CLK_PH8();
    SET_SPI1_I2SMCLK_PH10();
    SET_SPI1_MISO_PE1();
    SET_SPI1_MOSI_PE0();
    SET_SPI1_SS_PH9();

    if (nu_hw_nau88l25_init(&sCodecConfig_SPII2S) != RT_EOK)
        return -1;

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_audio_record);
    UTEST_UNIT_RUN(test_audio_playback);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"spii2s", utest_tc_init, utest_tc_cleanup, 10);

#endif /* NU_PKG_USING_NAU88L25 */
