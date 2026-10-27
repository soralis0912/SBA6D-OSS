/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: Kurt Yang <kurt.yang@mediatek.com>
 */
#ifndef MTK_FE_PROC_H
#define MTK_FE_PROC_H

#include <net/mtk/mtk_frame_engine.h>

#define PROCREG_DIR			"mtkfe"
#define PROCREG_DUMP_USERS		"users"
#define PROCREG_DBG_DMA			"dbg_dma"
#define PROCREG_DBG_LEVEL		"dbg_level"
#define PROCREG_DUMP_DMA		"dump_dma"
#define PROCREG_DUMP_EMI		"dump_emi"

struct fe_proc_operation {
	int (*get_usrs_info)(struct list_head **usrs, unsigned int **ref_cnt);
	int (*dbg_cmds)(char *dma, char *argv[]);
};

int fe_proc_init(struct fe_proc_operation *ops);
void fe_proc_exit(void);

#endif

