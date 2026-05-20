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
#include "dfs_ramfs.h"
#include "dfs_romfs.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#if defined(RT_USING_FAL)
    #include <fal.h>
#endif

#if defined(RT_USING_DFS_RAMFS)
    #define MOUNT_POINT_HYPERRAM      "/hyperram"
#endif

#if defined(BOARD_USING_STORAGE_SPIFLASH)
    #define PARTITION_NAME_FILESYSTEM "sf"
    #define MOUNT_POINT_SPIFLASH0 "/sf"
#endif

#ifdef RT_USING_DFS_MNTTABLE
const struct dfs_mount_tbl mount_table[] =
{
    { "sd0", "/sd0", "elm", 0, RT_NULL },
    {0},
};
#endif

#if defined(RT_USING_DFS_ROMFS)
static const rt_uint8_t _romfs_readme_txt[] =
    "RT-Thread ROMFS root.\r\n"
    "Writable paths are mounted separately on /sd0 and /hyperram.\r\n";

static const struct romfs_dirent _romfs_root_dirent[] =
{
    {ROMFS_DIRENT_DIR, "sd0", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "udisk", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "hyperram", RT_NULL, 0},
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


#if defined(RT_USING_DFS_RAMFS) && defined(BOARD_USING_EXTERNAL_HYPERRAM)
static rt_err_t mount_ramfs(const char *mount_point, rt_uint8_t *pool, rt_size_t size)
{
    struct dfs_ramfs *ramfs;

    ramfs = dfs_ramfs_create(pool, size);
    if (ramfs == RT_NULL)
    {
        LOG_E("failed to create ramfs for %s", mount_point);
        return -RT_ENOMEM;
    }

    if (dfs_mount(RT_NULL, mount_point, "ram", 0, (const void *)ramfs) != 0)
    {
        LOG_E("failed to mount ramfs on %s", mount_point);
        return -RT_ERROR;
    }

    LOG_I("ramfs mounted on \"%s\".", mount_point);

    return RT_EOK;
}

/* Initialize the filesystem */
int filesystem_init(void)
{
    rt_err_t result = RT_EOK;
    rt_uint8_t *pool = (rt_uint8_t *)0x80000000; // HBI address of HyperRAM
    rt_size_t hyperram_size = BOARD_USING_HYPERRAM_SIZE;

#if defined(RT_USING_DFS_ROMFS)
    if (dfs_mount(RT_NULL, "/", "rom", 0, (const void *)&romfs_root) != 0)
    {
        LOG_E("failed to mount romfs on \"/\"");
        result = -RT_ERROR;
        goto exit_filesystem_init;
    }

    LOG_I("romfs mounted on \"/\".");
#endif

#if defined(RT_USING_DFS_RAMFS) && defined(BOARD_USING_EXTERNAL_HYPERRAM)
    result = mount_ramfs(MOUNT_POINT_HYPERRAM, pool, hyperram_size);
    if (result != RT_EOK)
        goto exit_filesystem_init;
#endif

exit_filesystem_init:

    return -result;
}
INIT_ENV_EXPORT(filesystem_init);
#endif

#if defined(BOARD_USING_STORAGE_SPIFLASH)
int mnt_init_spiflash0(void)
{
#if defined(RT_USING_FAL)
    extern int fal_init_check(void);
    if (!fal_init_check())
        fal_init();
#endif
    struct rt_device *psNorFlash = fal_blk_device_create(PARTITION_NAME_FILESYSTEM);
    if (!psNorFlash)
    {
        rt_kprintf("Failed to create block device for %s.\n", PARTITION_NAME_FILESYSTEM);
        goto exit_mnt_init_spiflash0;
    }
    else if (dfs_mount(psNorFlash->parent.name, MOUNT_POINT_SPIFLASH0, "elm", 0, 0) != 0)
    {
        rt_kprintf("Failed to mount elm on %s.\n", MOUNT_POINT_SPIFLASH0);
        rt_kprintf("Try to execute 'mkfs -t elm %s' first, then reboot.\n", PARTITION_NAME_FILESYSTEM);
        goto exit_mnt_init_spiflash0;
    }
    rt_kprintf("mount %s with elmfat type: ok\n", PARTITION_NAME_FILESYSTEM);

exit_mnt_init_spiflash0:

    return 0;
}
INIT_APP_EXPORT(mnt_init_spiflash0);

#endif

#endif
