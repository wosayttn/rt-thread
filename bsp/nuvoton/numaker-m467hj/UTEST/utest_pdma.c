/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_PDMA)

#include "drv_pdma.h"

#define DMA_WORKER_MAX  4
#define DMA_MEM_RETRY 8
#define TEST_LEN  8192
#define THREAD_STACK_SIZE 1536
#define THREAD_PRIORITY 20
#define THREAD_TIMESLICE 10
#define DEF_TEST_CHAIN_ELEMENT 2

static rt_mutex_t g_worker_dma_print_lock = RT_NULL;
static nu_pdma_desc_t pdma_descs[DEF_TEST_CHAIN_ELEMENT]  __attribute__((aligned(64))) = {0};

static void pdma_dump_test_setting(void)
{
    rt_kprintf("\n[PDMA utest] test configuration\n");
    rt_kprintf("  purpose               : Verify PDMA memcpy, scatter-gather and descriptor setup.\n");
    rt_kprintf("  worker threads        : %d\n", DMA_WORKER_MAX);
    rt_kprintf("  memcpy retry count    : %d\n", DMA_MEM_RETRY);
    rt_kprintf("  transfer bytes        : %d\n", TEST_LEN);
    rt_kprintf("  sg chain elements     : %d\n", DEF_TEST_CHAIN_ELEMENT);
    rt_kprintf("  thread stack size     : %d\n", THREAD_STACK_SIZE);
    rt_kprintf("  thread priority       : %d\n", THREAD_PRIORITY);
    rt_kprintf("  thread timeslice      : %d\n", THREAD_TIMESLICE);
    rt_kprintf("  descriptor alignment  : %d\n", 64);
#ifdef NU_PDMA_MEMFUN_ACTOR_MAX
    rt_kprintf("  pdma memfun actors    : %d\n", NU_PDMA_MEMFUN_ACTOR_MAX);
#endif
    rt_kprintf("  event mask (done)     : 0x%08x\n", NU_PDMA_EVENT_TRANSFER_DONE);
    rt_kprintf("  event mask (abort)    : 0x%08x\n", NU_PDMA_EVENT_ABORT);
    rt_kprintf("  event mask (alignment): 0x%08x\n\n", NU_PDMA_EVENT_ALIGNMENT);
}

static void *utest_malloc(int size)
{
    return rt_malloc_align(size, 64);
}

static void utest_free(void *p)
{
    rt_free_align(p);
}

static void worker_dma_memcpy(void *pdata)
{
    int retry = DMA_MEM_RETRY;

    char *szSrc = (char *) utest_malloc(TEST_LEN);
    char *szDst = (char *) utest_malloc(TEST_LEN);

    RT_ASSERT(szSrc);
    RT_ASSERT(szDst);

    while (retry--)
    {
        void *ret;
        rt_memset(szSrc, 0x12, TEST_LEN);
        rt_memset(szDst, 0x34, TEST_LEN);

        ret = nu_pdma_memcpy(szDst, szSrc, TEST_LEN);
        rt_mutex_take(g_worker_dma_print_lock, RT_WAITING_FOREVER);
        uassert_not_null(ret);
        uassert_buf_equal(szSrc, szDst, TEST_LEN);
        rt_mutex_release(g_worker_dma_print_lock);
    }

    utest_free(szSrc);
    utest_free(szDst);
}

/*
 * Stress PDMA memory copy by running multiple worker threads in parallel.
 */
static void dma_memfun(void)
{
    int idx = 0;
    rt_thread_t thread_worker[DMA_WORKER_MAX];
    rt_thread_t working;

    g_worker_dma_print_lock = rt_mutex_create("pdma_plock", RT_IPC_FLAG_PRIO);

    // Init thread
    for (idx = 0; idx < DMA_WORKER_MAX; idx++)
    {
        thread_worker[idx] = rt_thread_create("pdma_worker",
                                              worker_dma_memcpy, NULL,
                                              THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE);
    }

    // Run all
    for (idx = 0; idx < DMA_WORKER_MAX; idx++)
    {
        if (thread_worker[idx] != RT_NULL) rt_thread_startup(thread_worker[idx]);
    }

    // Join
    while ((working = rt_thread_find("pdma_worker")) != RT_NULL)
        rt_thread_mdelay(100);
    uassert_null(working);

    rt_mutex_delete(g_worker_dma_print_lock);
}

static int pdma_sg_counter = 0;

static void pdma_sg_cb(void *pvUserData, uint32_t u32EventFilter)
{
    rt_sem_t psem = (rt_sem_t)pvUserData;

    if (u32EventFilter & NU_PDMA_EVENT_TRANSFER_DONE)
    {
        pdma_sg_counter++;
    }
    else
    {
        rt_kprintf("u32EventFilter=%08x\n", u32EventFilter);
        return ;
    }

    /* If got DEF_TEST_CHAIN_ELEMENT times TD, notify channel owner. */
    if (DEF_TEST_CHAIN_ELEMENT == pdma_sg_counter)
        rt_sem_release(psem);
}

/*
 * Dump descriptor content for scatter-gather debugging.
 */
static void dma_dump_desc(nu_pdma_desc_t psPdmaDesc)
{
    rt_kprintf("psPdmaDesc=%08x\n", (uint32_t)psPdmaDesc);/*!< [0x0000] Descriptor Table Control Register of PDMA Channel n.             */
    rt_kprintf("\tCTL=%08x\n", psPdmaDesc->CTL);/*!< [0x0000] Descriptor Table Control Register of PDMA Channel n.             */
    rt_kprintf("\tSA=%08x\n", psPdmaDesc->SA);/*!< [0x0000] Descriptor Table Control Register of PDMA Channel n.             */
    rt_kprintf("\tDA=%08x\n", psPdmaDesc->DA);/*!< [0x0000] Descriptor Table Control Register of PDMA Channel n.             */
    rt_kprintf("\tNEXT=%08x\n", psPdmaDesc->NEXT);/*!< [0x0000] Descriptor Table Control Register of PDMA Channel n.             */
}

/*
 * Verify scatter-gather memory transfer and callback completion.
 */
static void dma_scatter_gather(void)
{
    rt_err_t result;
    int iChanID = -1;

    rt_sem_t psem = RT_NULL;
    int i = 0 ;

    char *szSrc = (char *) utest_malloc(DEF_TEST_CHAIN_ELEMENT * TEST_LEN);
    char *szDst = (char *) utest_malloc(DEF_TEST_CHAIN_ELEMENT * TEST_LEN);

    RT_ASSERT(szSrc);
    RT_ASSERT(szDst);

    /* Finish signal */
    psem = rt_sem_create("sg_sem", 0, RT_IPC_FLAG_FIFO);
    uassert_not_null(psem);

    /* Allocate channel */
    iChanID = nu_pdma_channel_allocate(PDMA_MEM);
    uassert_true(iChanID >= 0);
    if (iChanID < 0)   /* Failed to allocating. */
        return;

    /* Register CB */
    {
        struct nu_pdma_chn_cb sChnCB;

        /* Register ISR callback function */
        sChnCB.m_eCBType = eCBType_Event;
        sChnCB.m_pfnCBHandler = pdma_sg_cb;
        sChnCB.m_pvUserData = (void *)psem;

        nu_pdma_filtering_set(iChanID, NU_PDMA_EVENT_ABORT | NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_ALIGNMENT);
        nu_pdma_callback_register(iChanID, &sChnCB);
    }
    nu_pdma_sgtbls_allocate(iChanID, &pdma_descs[0], DEF_TEST_CHAIN_ELEMENT);

    /* Prepare data and tables for sg, SET it as a chain. */
    for (i = 0; i < DEF_TEST_CHAIN_ELEMENT; i++)
    {
        nu_pdma_desc_t next;
        rt_memset(&szSrc[i * TEST_LEN], i * 2, TEST_LEN);

        if (i == (DEF_TEST_CHAIN_ELEMENT - 1)) // last one
        {
            next = NULL;
        }
        else // Link to next
        {
            next = pdma_descs[(i + 1) % DEF_TEST_CHAIN_ELEMENT];
        }

        result = nu_pdma_desc_setup(iChanID,
                                    pdma_descs[i],
                                    8,
                                    (uint32_t)&szSrc[i * TEST_LEN],
                                    (uint32_t)&szDst[i * TEST_LEN],
                                    TEST_LEN,
                                    next,
                                    0);
        dma_dump_desc(pdma_descs[i]);
        uassert_int_equal(result, RT_EOK);
    }

    /* Clean count before transferring */
    pdma_sg_counter = 0;

    /* Assign head descriptor & go if it is m2m */
    result = nu_pdma_sg_transfer(iChanID, pdma_descs[0], 0);
    uassert_int_equal(result, RT_EOK);

    /* Wait copying done. */
    rt_sem_take(psem, 2000);

    rt_kprintf("pdma_sg_counter=%d\n", pdma_sg_counter);

    /* Verify buffer content */
    uassert_buf_equal(szSrc, szDst, TEST_LEN * DEF_TEST_CHAIN_ELEMENT);

    /* Free sg tables */
    nu_pdma_sgtbls_free(iChanID, &pdma_descs[0], DEF_TEST_CHAIN_ELEMENT);

    result = nu_pdma_channel_free(iChanID);
    uassert_int_equal(result, RT_EOK);

    result = rt_sem_delete(psem);
    uassert_int_equal(result, RT_EOK);

    utest_free(szSrc);
    utest_free(szDst);
}

/*
 * Verify scatter-gather descriptor table allocation and release.
 */
static void dma_sg_tbles_management(void)
{
    int i;

    /* Allocate sg tables */
    uassert_int_equal(nu_pdma_sgtbls_allocate(0, &pdma_descs[0], DEF_TEST_CHAIN_ELEMENT), RT_EOK);

    for (i = 0; i < DEF_TEST_CHAIN_ELEMENT; i++)
    {
        /* Check every tables is valid */
        uassert_not_null(pdma_descs[i]);
    }
    nu_pdma_sgtbls_free(0, &pdma_descs[0], DEF_TEST_CHAIN_ELEMENT);
}

static rt_err_t utest_tc_init(void)
{
    pdma_dump_test_setting();

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    /* Registered test suite */
    UTEST_UNIT_RUN(dma_memfun);
    UTEST_UNIT_RUN(dma_scatter_gather);
    UTEST_UNIT_RUN(dma_sg_tbles_management);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"pdma",
                utest_tc_init, utest_tc_cleanup, 30);

#endif /* BSP_USING_PDMA */
