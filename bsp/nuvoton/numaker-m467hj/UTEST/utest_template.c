/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

static void template_dump_test_setting(void)
{
    rt_kprintf("\n[TEMPLATE utest] description\n");
    rt_kprintf("  purpose               : Provide a reference UTEST skeleton for new cases.\n");
    rt_kprintf("  test cases            : test_case1, test_case2\n");
    rt_kprintf("  hardware dependency   : none (template placeholder)\n\n");
}

static void test_case1(void)
{
    uassert_true(1);
}

static void test_case2(void)
{
    uassert_true(0);
}

static rt_err_t utest_tc_init(void)
{
    template_dump_test_setting();
    // Backup original multiple function pin setting in here if necessary.

    // Set multiple function pin setting for the test cases in here if necessary.

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    // Restore multiple function pin setting to original setting in here if necessary.

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_case1);
    UTEST_UNIT_RUN(test_case2);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"template",
                utest_tc_init, utest_tc_cleanup, 1);
