/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  MTK SPI bus driver definitions
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Leilk Liu <leilk.liu@mediatek.com>
 */

#ifndef ____LINUX_PLATFORM_DATA_SPI_MTK_H
#define ____LINUX_PLATFORM_DATA_SPI_MTK_H

#define _MTK_SPI_TEST_MODE_  0

/* Board specific platform_data */
struct mtk_chip_config {
	u32 sample_sel;

	u32 cs_setuptime;
	u32 cs_holdtime;
	u32 cs_idletime;
	u32 tick_delay;

#if _MTK_SPI_TEST_MODE_
 u32 tx_mlsb;
 u32 rx_mlsb;
 u32 deassert_mode;
 u32 rx_little_endian;
 u32 tx_little_endian;
 u32 pad;
 u32 probe;
 u32 test;
#endif
};
#endif
