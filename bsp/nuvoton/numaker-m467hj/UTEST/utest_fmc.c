/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(RT_USING_FAL) && defined(BSP_USING_FMC) && defined(RT_USING_FINSH)

#include "fal.h"
#include "msh.h"

#ifndef UTEST_FMC_BENCH_SIZE
#define UTEST_FMC_BENCH_SIZE      4096
#endif

static const struct fal_partition *g_part_ldrom = RT_NULL;
static const struct fal_partition *g_part_aprom = RT_NULL;

static void fmc_run_msh(const char *cmd)
{
    int result;

    rt_kprintf("[FMC utest] exec: %s\n", cmd);
    result = msh_exec((char *)cmd, rt_strlen(cmd));
    rt_kprintf("[FMC utest] ret : %d\n", result);
}

static void fmc_run_bench(const char *partition_name)
{
    char cmd[32];

    rt_snprintf(cmd, sizeof(cmd), "fal probe %s", partition_name);
    fmc_run_msh(cmd);

    rt_snprintf(cmd, sizeof(cmd), "fal bench %d yes", UTEST_FMC_BENCH_SIZE);
    fmc_run_msh(cmd);
}

static void fmc_dump_test_setting(void)
{
    rt_kprintf("\n[FMC utest] test configuration\n");
    rt_kprintf("  purpose               : Verify FAL partitions and run the LDROM benchmark.\n");
    rt_kprintf("  fal support           : enabled\n");
    rt_kprintf("  fmc support           : enabled\n");
    rt_kprintf("  finsh support         : enabled\n");
    rt_kprintf("  ldrom partition       : %s\n", g_part_ldrom ? "ready" : "missing");
    rt_kprintf("  aprom partition       : %s\n", g_part_aprom ? "ready" : "missing");
    rt_kprintf("  benchmark size        : %d\n", UTEST_FMC_BENCH_SIZE);
    rt_kprintf("  aprom shell test      : skipped\n\n");
}

static void test_fmc_partition_exist(void)
{
    uassert_not_null(g_part_ldrom);
    uassert_not_null(g_part_aprom);
}

static void test_fmc_ldrom_bench(void)
{
    uassert_not_null(g_part_ldrom);
    fmc_run_bench("LDROM");
}

static void test_fmc_aprom_partition_exist(void)
{
    uassert_not_null(g_part_aprom);

    rt_kprintf("[FMC utest] skip shell probe on APROM to avoid self-flash access while running from APROM.\n");
}

static rt_err_t utest_tc_init(void)
{
    extern int fal_init(void);
    extern int fal_init_check(void);

    if (!fal_init_check())
        fal_init();

    g_part_ldrom = fal_partition_find("LDROM");
    g_part_aprom = fal_partition_find("APROM");

    fmc_dump_test_setting();

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_fmc_partition_exist);
    UTEST_UNIT_RUN(test_fmc_ldrom_bench);
    UTEST_UNIT_RUN(test_fmc_aprom_partition_exist);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"fmc",
                utest_tc_init, utest_tc_cleanup, 120);

#endif /* defined(RT_USING_FAL) && defined(BSP_USING_FMC) && defined(RT_USING_FINSH) */
