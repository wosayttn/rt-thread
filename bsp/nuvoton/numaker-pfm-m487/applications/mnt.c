/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>

#undef LOG_TAG
#define LOG_TAG "mnt"
#define DBG_TAG LOG_TAG
#include "drv_log.h"

#if defined(RT_USING_DFS)
#include <dfs_fs.h>
#include <dfs_file.h>
#include <dfs_romfs.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#if defined(RT_USING_FAL)
    #include <fal.h>
#endif


#ifdef RT_USING_DFS_MNTTABLE
const struct dfs_mount_tbl mount_table[] =
{
    /* The SD card is mounted dynamically by the automount thread below,
       so it is intentionally not listed here. */
    {0},
};
#endif

#if defined(RT_USING_DFS_ROMFS)
static const rt_uint8_t _romfs_readme_txt[] =
    "RT-Thread ROMFS root.\r\n"
    "Writable paths are mounted separately on /sd0 and /sf.\r\n";

static const struct romfs_dirent _romfs_root_dirent[] =
{
    {ROMFS_DIRENT_DIR, "sd0", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "udisk", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "sf", RT_NULL, 0},
    {ROMFS_DIRENT_FILE, "readme.txt", _romfs_readme_txt, sizeof(_romfs_readme_txt) - 1},
};

const struct romfs_dirent romfs_root =
{
    ROMFS_DIRENT_DIR,
    "/",
    (const rt_uint8_t *)_romfs_root_dirent,
    sizeof(_romfs_root_dirent) / sizeof(_romfs_root_dirent[0])
};
#endif

#if defined(BSP_USING_SDH)

#define SD_CARD_DEVICE_NAME    "sd0"
#define SD_CARD_MOUNT_POINT    "/sd0"
#define SD_CARD_CHECK_INTERVAL 1000      /* Card detect poll interval, in ms */

/* Automount thread: mount the SD card on insertion and release it on removal.
   The block framework unregisters the "sd0" device and unmounts it when the
   card is pulled out, so we only need to react to the device presence edges. */
static void sd_automount_thread_entry(void *parameter)
{
    rt_bool_t prev_present = RT_FALSE;

    while (1)
    {
        rt_bool_t present = (rt_device_find(SD_CARD_DEVICE_NAME) != RT_NULL);

        if (present && !prev_present)
        {
            /* Card just inserted */
            if (dfs_mount(SD_CARD_DEVICE_NAME, SD_CARD_MOUNT_POINT, "elm", 0, RT_NULL) == RT_EOK)
            {
                LOG_I("mount %s on %s: ok", SD_CARD_DEVICE_NAME, SD_CARD_MOUNT_POINT);
            }
            else
            {
                LOG_W("mount %s on %s failed.", SD_CARD_DEVICE_NAME, SD_CARD_MOUNT_POINT);
                LOG_W("Try to execute 'mkfs -t elm %s' first, then re-insert the card.", SD_CARD_DEVICE_NAME);
            }
        }
        else if (!present && prev_present)
        {
            /* Card just removed. It is unmounted automatically by the block
               framework; call dfs_unmount() defensively to release the mount point. */
            dfs_unmount(SD_CARD_MOUNT_POINT);
            LOG_I("card removed, %s unmounted", SD_CARD_MOUNT_POINT);
        }

        prev_present = present;
        rt_thread_mdelay(SD_CARD_CHECK_INTERVAL);
    }
}

static rt_err_t sd_automount_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("sd_mount", sd_automount_thread_entry, RT_NULL,
                           1024, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid == RT_NULL)
    {
        LOG_E("failed to create sd_mount thread.");
        return -RT_ERROR;
    }

    rt_thread_startup(tid);
    return RT_EOK;
}
#endif

/* Initialize the filesystem */
int filesystem_init(void)
{
    rt_err_t result = RT_EOK;

#if defined(RT_USING_FAL)
    extern int fal_init_check(void);
    if (!fal_init_check())
        fal_init();
#endif

#if defined(RT_USING_DFS_ROMFS)
    if (dfs_mount(RT_NULL, "/", "rom", 0, (const void *)&romfs_root) != 0)
    {
        LOG_E("failed to mount romfs on \"/\"");
        result = -RT_ERROR;
        goto exit_filesystem_init;
    }

    LOG_I("romfs mounted on \"/\".");

    struct rt_device *psSPIFlash = fal_blk_device_create("sf");

    if (!psSPIFlash)
    {
        LOG_E("Failed to create block device for sf.");
        goto exit_filesystem_init;
    }
    else if (dfs_mount(psSPIFlash->parent.name, "/sf", "elm", 0, 0) != 0)
    {
        LOG_E("Failed to mount elm on /sf.");
        LOG_E("Try to execute 'mkfs -t elm %s' first, then reboot.", "sf");
        goto exit_filesystem_init;
    }
    LOG_I("mount %s with elmfat type: ok", "sf");
#endif

#if defined(BSP_USING_SDH)
    /* Start SD card automount (hot-plug mount/unmount on /sd0) */
    sd_automount_init();
#endif

exit_filesystem_init:

    return -result;
}
INIT_ENV_EXPORT(filesystem_init);
#endif