/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if defined(BSP_USING_CANFD)

#include "rtdevice.h"

#include "NuMicro.h"

#define NU_CANFD_DEVNAME "canfd0"

static void canfd_dump_test_setting(void)
{
    rt_kprintf("\n[CANFD utest] description\n");
    rt_kprintf("  purpose               : Verify CAN FD config APIs and loopback transmission.\n");
    rt_kprintf("  can device            : %s\n", NU_CANFD_DEVNAME);
    rt_kprintf("  payload length        : %d\n", 8);
#if defined(RT_CAN_USING_HDR)
    rt_kprintf("  hdr filter mode       : enabled\n\n");
#else
    rt_kprintf("  hdr filter mode       : disabled\n\n");
#endif
}

static struct rt_can_msg tx_msg = {0};
static struct rt_can_msg rx_msg = {0};

static rt_device_t can0_dev;

struct rt_can_status can_status;

struct can_configure sCanConfig_New ;
struct can_configure sCanConfig_Org ;

static void can_config_test(rt_device_t can_dev);

static struct rt_semaphore rx_sem;

static rt_err_t can_rx_done(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

#if defined(RT_CAN_USING_HDR)
static rt_int32_t rx_filter_hdr = 0;
static rt_err_t can_rx_filter_done(rt_device_t dev, void *args, rt_int32_t hdr, rt_size_t size)
{
    rx_filter_hdr = hdr;
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

static void can_hdr_tx_rx_test(rt_uint32_t id, rt_uint8_t id_type, rt_uint8_t rtr, rt_uint8_t len, rt_uint8_t data_offset)
{
    rt_size_t  size;
    rt_err_t res;

    tx_msg.id =  id;              /* ID  */
    tx_msg.ide = id_type;         /* ID type */
    tx_msg.rtr = rtr;            /*  Data frame type */
    tx_msg.len = len;            /*  Data Len 8  */
    tx_msg.data[0] = 0x00 + data_offset;
    tx_msg.data[1] = 0x10 + data_offset;
    tx_msg.data[2] = 0x20 + data_offset;
    tx_msg.data[3] = 0x30 + data_offset;
    tx_msg.data[4] = 0x40 + data_offset;
    tx_msg.data[5] = 0x50 + data_offset;
    tx_msg.data[6] = 0x60 + data_offset;
    tx_msg.data[7] = 0x70 + data_offset;

    /*CANFD0 Send Message*/
    size = rt_device_write(can0_dev, 0, &tx_msg, sizeof(tx_msg));
    /*Confirmed message transmission*/
    uassert_int_not_equal(size, 0);

    rt_kprintf("tx- id: %08x, ide: %08x, len: %08x\n", tx_msg.id, tx_msg.ide, tx_msg.len);

    /*Wait CANFD0 receive match message*/
    res = rt_sem_take(&rx_sem, 500);

    if (res == RT_EOK)
    {
        /*Read Match message*/
        rx_msg.hdr_index = rx_filter_hdr;
        rt_device_read(can0_dev, 0, &rx_msg, sizeof(rx_msg));

        /* Check that the message is correct */
        rt_kprintf("rx- id: %08x, ide: %08x, len: %08x\n", rx_msg.id, rx_msg.ide, rx_msg.len);

        uassert_int_equal(tx_msg.id, rx_msg.id);
        uassert_int_equal(tx_msg.ide, rx_msg.ide);
        uassert_int_equal(tx_msg.len, rx_msg.len);
        uassert_buf_equal(&tx_msg.data, &rx_msg.data, sizeof(&tx_msg.data));
    }

    /*Clear Rx message buffer*/
    rt_memset((void *)&rx_msg, 0, sizeof(rx_msg));

    return;
}

static void testcase_can_loopback_hdr_mode(void)
{
    rt_err_t ret;

    ret = rt_device_open(can0_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    /* Set CANFD0 as Normal mode */
    ret = rt_device_control(can0_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LOOPBACK);
    uassert_int_equal(ret, RT_EOK);

    rt_device_set_rx_indicate(can0_dev, can_rx_done);

    struct rt_can_filter_item items[4] =
    {
        RT_CAN_FILTER_ITEM_INIT(0x7FF, 0, 0, 0, 0x1FC3FFFF, can_rx_filter_done, RT_NULL), /* Standard ID,match ID:0x7F0~0x7FF */
        RT_CAN_FILTER_ITEM_INIT(0x7FFFF, 1, 0, 0, 0x1FFFFF00, can_rx_filter_done, RT_NULL), /* Extended ID,match ID:0x7FF00~0x7FFFF */
        RT_CAN_FILTER_STD_INIT(0x133, can_rx_filter_done, RT_NULL),                       /* Standard ID,0x133       */
        RT_CAN_FILTER_EXT_INIT(0x12345678, can_rx_filter_done, RT_NULL),                  /* Extended ID,0x12345678       */
    };

    items[0].hdr_bank = 0;
    items[1].hdr_bank = 1;
    items[2].hdr_bank = 2;
    items[3].hdr_bank = 3;

    struct rt_can_filter_config cfg = {4, 1, items}; /* 4 Filter item*/
    /* Set CAN 0 Filter Message*/
    ret = rt_device_control(can0_dev, RT_CAN_CMD_SET_FILTER, &cfg);
    RT_ASSERT(ret == RT_EOK);

    /* ID 0x7F1, Standard ID, Data frame, Data Len 8,offset 0 */
    can_hdr_tx_rx_test(0x7F1, RT_CAN_STDID, RT_CAN_DTR, 8, 0); //Will be filtered.
    /* ID1 0x7F9, Standard ID, Data frame, Data Len 8,offset 0 */
    can_hdr_tx_rx_test(0x7F9, RT_CAN_STDID, RT_CAN_DTR, 8, 0); //Will be filtered.
    /* ID 0x133, Standard ID, Data frame, Data Len 8,offset 2 */
    can_hdr_tx_rx_test(0x133, RT_CAN_STDID, RT_CAN_DTR, 8, 2); //Will pass

    /* ID 0x7FFF0, Extended ID, Data frame, Data Len 8,Data offset 1 */
    can_hdr_tx_rx_test(0x7FFF0, RT_CAN_EXTID, RT_CAN_DTR, 8, 1); //Will be filtered.
    /* ID1 0x7FF12, Extended ID, Data frame, Data Len 8,offset 1 */
    can_hdr_tx_rx_test(0x7FF12, RT_CAN_EXTID, RT_CAN_DTR, 8, 1); //Will be filtered.
    /* ID 0x12345678, Extended ID, Data frame, Data Len 8,offset 2 */
    can_hdr_tx_rx_test(0x12345678, RT_CAN_EXTID, RT_CAN_DTR, 8, 3); //Will pass.

    /*Close CANFD0 bus*/
    ret = rt_device_close(can0_dev);
    uassert_int_equal(ret, RT_EOK);

    return;
}
#else
static void can_loopback_tx_rx_test(rt_uint32_t id, rt_uint8_t id_type, rt_uint8_t rtr, rt_uint8_t len, rt_uint8_t data_offset)
{
    rt_size_t  size;

    tx_msg.id =  id;              /* ID  */
    tx_msg.ide = id_type;         /* ID type */
    tx_msg.rtr = rtr;             /* Data frame type */
    tx_msg.len = len;             /* Data Len 8  */
    tx_msg.data[0] = 0x00 + data_offset;
    tx_msg.data[1] = 0x10 + data_offset;
    tx_msg.data[2] = 0x20 + data_offset;
    tx_msg.data[3] = 0x30 + data_offset;
    tx_msg.data[4] = 0x40 + data_offset;
    tx_msg.data[5] = 0x50 + data_offset;
    tx_msg.data[6] = 0x60 + data_offset;
    tx_msg.data[7] = 0x70 + data_offset;


    /*CANFD0 Send Message*/
    size = rt_device_write(can0_dev, 0, &tx_msg, sizeof(tx_msg));
    /*Check message is Send*/
    uassert_int_not_equal(size, 0);

    /* Receive the Loopback message(Send Message) */
    size = rt_device_read(can0_dev, 0, &rx_msg, sizeof(rx_msg));
    /*Check message is received*/
    uassert_int_not_equal(size, 0);

    /* Check that the message is correct */
    uassert_int_equal(tx_msg.id, rx_msg.id);
    uassert_int_equal(tx_msg.ide, rx_msg.ide);
    uassert_int_equal(tx_msg.len, tx_msg.len);
    uassert_buf_equal(&tx_msg.data, &rx_msg.data, tx_msg.len);

    rt_memset((void *)&tx_msg, 0, sizeof(tx_msg));
    rt_memset((void *)&rx_msg, 0, sizeof(rx_msg));

    return;
}

static void testcase_can_loopback_mode(void)
{
    rt_err_t ret;

    ret = rt_device_open(can0_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    /* Set CANFD0 as Normal mode */
    ret = rt_device_control(can0_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LOOPBACK);
    uassert_int_equal(ret, RT_EOK);

    rt_device_set_rx_indicate(can0_dev, can_rx_done);

    /* ID 0x123, Standard ID, Data frame, Data Len 8,offset 0 */
    can_loopback_tx_rx_test(0x123, RT_CAN_STDID, RT_CAN_DTR, 8, 0);

    /* ID 0x7FF, Standard ID, Data frame, Data Len 8,offset 0 */
    can_loopback_tx_rx_test(0x7FF, RT_CAN_STDID, RT_CAN_DTR, 8, 1);

    /* ID 0x7FFF0, Extended ID, Data frame, Data Len 8,offset 0 */
    can_loopback_tx_rx_test(0x7FFF0, RT_CAN_EXTID, RT_CAN_DTR, 8, 0);

    /* ID 0x12345678, Extended ID, Data frame, Data Len 8,offset 1 */
    can_loopback_tx_rx_test(0x12345678, RT_CAN_EXTID, RT_CAN_DTR, 8, 1);

    /* Close */
    ret = rt_device_close(can0_dev);
    uassert_int_equal(ret, RT_EOK);

    return;
}

#endif

static void can_config_test(rt_device_t can_dev)
{

    rt_err_t ret;
    rt_uint32_t mode;
    rt_uint32_t baud_rate;

    ret = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    uassert_int_equal(ret, RT_EOK);

    /* Enable CAN bus Error status Interrupt */
    ret = rt_device_control(can_dev, RT_DEVICE_CTRL_SET_INT, (void *) RT_DEVICE_CAN_INT_ERR);
    uassert_int_equal(ret, RT_EOK);

    /* Disable CAN RX Interrupt  */
    ret = rt_device_control(can_dev, RT_DEVICE_CTRL_CLR_INT, (void *)(RT_DEVICE_FLAG_INT_RX));
    uassert_int_equal(ret, RT_EOK);

    /* Disable CAN TX Interrupt  */
    ret = rt_device_control(can_dev, RT_DEVICE_CTRL_CLR_INT, (void *)(RT_DEVICE_FLAG_INT_TX));
    uassert_int_equal(ret, RT_EOK);

    /* Disable CAN Error status Interrupt  */
    ret = rt_device_control(can_dev, RT_DEVICE_CTRL_CLR_INT, (void *)(RT_DEVICE_CAN_INT_ERR));
    uassert_int_equal(ret, RT_EOK);

    /* Set CAN as Lisen mode */
    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LISTEN);
    uassert_int_equal(ret, RT_EOK);

    /* Set CAN as Loopback mode */
    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LOOPBACK);
    uassert_int_equal(ret, RT_EOK);

    /* Set CAN as Loopback+Lisen mode */
    //ret = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LOOPBACKANLISTEN);
    //uassert_int_equal(ret, RT_EOK);

    /* Set CAN as Normal mode */
    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
    uassert_int_equal(ret, RT_EOK);

    mode = 4;
    /*CAN not support operation mode */
    ret = rt_device_control(can0_dev, RT_CAN_CMD_SET_MODE, (void *)mode);
    uassert_int_equal(ret, -(RT_ERROR));

    /* Set CAN Baud Rate as 500kbps*/
    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN500kBaud);
    uassert_int_equal(ret, RT_EOK);

    baud_rate = 10;
    /* Set CAN Baud Rate as 10bps*/
    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)baud_rate);
    uassert_int_equal(ret, -(RT_ERROR));

    /* Set CAN Baud Rate as 1Mbps*/
    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN500kBaud);
    uassert_int_equal(ret, RT_EOK);


    /* Get CAN bus Status*/
    ret = rt_device_control(can_dev, RT_CAN_CMD_GET_STATUS, &can_status);
    uassert_int_equal(ret, RT_EOK);

    /* Close */
    ret = rt_device_close(can_dev);
    uassert_int_equal(ret, RT_EOK);

    return;
}

static void testcase_can_config(void)
{
    can0_dev = rt_device_find(NU_CANFD_DEVNAME);
    can_config_test(can0_dev);

    return;
}

static rt_err_t utest_tc_init(void)
{
    canfd_dump_test_setting();
    // Backup original multiple function pin setting in here if necessary.

    // Set multiple function pin setting for the test cases in here if necessary.

    return rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
}

static rt_err_t utest_tc_cleanup(void)
{
    rt_sem_detach(&rx_sem);

    // Restore multiple function pin setting

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(testcase_can_config);
#if defined(RT_CAN_USING_HDR)
    UTEST_UNIT_RUN(testcase_can_loopback_hdr_mode);
#else
    UTEST_UNIT_RUN(testcase_can_loopback_mode);
#endif
}
UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"canfd", utest_tc_init, utest_tc_cleanup, 2);



static void can_rx_thread(void *parameter)
{
    int i;
    struct rt_can_msg rxmsg = {0};
    rt_device_set_rx_indicate(can0_dev, can_rx_done);

#if defined(RT_CAN_USING_HDR)
    rt_err_t res;
    struct rt_can_filter_item items[5] =
    {
        RT_CAN_FILTER_ITEM_INIT(0x100, 0, 0, 1, 0x700, RT_NULL, RT_NULL),
        RT_CAN_FILTER_ITEM_INIT(0x300, 0, 0, 1, 0x700, RT_NULL, RT_NULL),
        RT_CAN_FILTER_ITEM_INIT(0x211, 0, 0, 1, 0x7ff, RT_NULL, RT_NULL),
        RT_CAN_FILTER_STD_INIT(0x486, RT_NULL, RT_NULL),
        {0x555, 0, 0, 1, 0x7ff, 7,}
    };
    struct rt_can_filter_config cfg = {5, 1, items};
    res = rt_device_control(can0_dev, RT_CAN_CMD_SET_FILTER, &cfg);
    RT_ASSERT(res == RT_EOK);
#endif

    while (1)
    {
        rxmsg.hdr_index = -1;
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        rt_device_read(can0_dev, 0, &rxmsg, sizeof(rxmsg));
        rt_kprintf("ID:0x%x", rxmsg.id);
        for (i = 0; i < rxmsg.len; i++)
        {
            rt_kprintf("%2x", rxmsg.data[i]);
        }

        rt_kprintf("\n");
    }
}

int can_sample(int argc, char *argv[])
{
    rt_err_t res;
    rt_thread_t thread;
    char can_name[RT_NAME_MAX];

    if (argc == 2)
    {
        rt_strncpy(can_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(can_name, NU_CANFD_DEVNAME, RT_NAME_MAX);
    }
    can0_dev = rt_device_find(can_name);
    if (!can0_dev)
    {
        rt_kprintf("find %s failed!\n", can_name);
        return RT_ERROR;
    }

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    res = rt_device_open(can0_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    RT_ASSERT(res == RT_EOK);

    res = rt_device_control(can0_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN500kBaud);
    RT_ASSERT(res == RT_EOK);

//      res = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
    res = rt_device_control(can0_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LOOPBACK);

    RT_ASSERT(res == RT_EOK);

    thread = rt_thread_create("can_rx", can_rx_thread, RT_NULL, 2048, 25, 10);
    RT_ASSERT(thread);

    rt_thread_startup(thread);
    rt_kprintf("create can_rx thread ok!\n");

    return res;
}
MSH_CMD_EXPORT(can_sample, can device sample);

void can_sendmsg(void)
{
    struct rt_can_msg msg = {0};
    int id_arr[] = { 0x100, 0x200, 0x300, 0x211 };
    rt_size_t  size;
    int i;

    for (i = 0; i < sizeof(id_arr) / sizeof(int); i++)
    {
        msg.id = id_arr[i];
        msg.ide = RT_CAN_STDID;
        msg.rtr = RT_CAN_DTR;
        msg.len = 8;
        msg.data[0] = 0x00;
        msg.data[1] = 0x11;
        msg.data[2] = 0x22;
        msg.data[3] = 0x33;
        msg.data[4] = 0x44;
        msg.data[5] = 0x55;
        msg.data[6] = 0x66;
        msg.data[7] = 0x77;

        size = rt_device_write(can0_dev, 0, &msg, sizeof(msg));
        if (size == 0)
        {
            rt_kprintf("can dev write data failed!\n");
        }
        rt_kprintf("sent %x.\n", id_arr[i]);
    }
}
MSH_CMD_EXPORT(can_sendmsg, can device send);

#endif //#if defined(BSP_USING_CANFD)
