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
    result = mount_ramfs("/hyperram", pool, hyperram_size);
    if (result != RT_EOK)
        goto exit_filesystem_init;
#endif

exit_filesystem_init:

    return -result;
}
INIT_ENV_EXPORT(filesystem_init);
#endif

#endif
