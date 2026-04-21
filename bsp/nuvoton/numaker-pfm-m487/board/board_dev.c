/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtdevice.h>
#include "drv_sys.h"

#if defined(BOARD_USING_QSPI_FLASH)
#include "qspinor.h"

static int rt_hw_spiflash_init(void)
{
    LOG_I("Initializing QSPI Flash ...");
    /* Here, we use Dual I/O to drive the SPI flash by default. */
    /* If you want to use Quad I/O, you can modify to 4 from 2 and crossover D2/D3 pin of SPI flash. */
	
    if (rt_hw_qspi_device_attach("qspi0", "qspi01", PIN_NONE, 2, SpiFlash_EnterQspiMode, SpiFlash_ExitQspiMode) != RT_EOK)
        return -1;
    LOG_I("QSPI Flash initialized successfully.");

#if defined(RT_USING_SFUD)
    LOG_I("Probing QSPI Flash ...");
    if (rt_sfud_flash_probe(FAL_USING_NOR_FLASH_DEV_NAME, "qspi01") == RT_NULL)
    {
        return -(RT_ERROR);
    }
    LOG_I("QSPI Flash probed successfully.");
#endif

    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_spiflash_init);
#endif /* BOARD_USING_QSPI_FLASH */

#if defined(BOARD_USING_NAU88L25) && defined(NU_PKG_USING_NAU88L25)
#include "acodec_nau88l25.h"
S_NU_NAU88L25_CONFIG sCodecConfig =
{
    .i2c_bus_name = "i2c2",

    .i2s_bus_name = "sound0",

    .pin_phonejack_en = NU_GET_PININDEX(NU_PE, 13),

    .pin_phonejack_det = 0,
};

int rt_hw_nau88l25_port(void)
{
    if (nu_hw_nau88l25_init(&sCodecConfig) != RT_EOK)
        return -1;

    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_nau88l25_port);
#endif /* BOARD_USING_NAU88L25 */
