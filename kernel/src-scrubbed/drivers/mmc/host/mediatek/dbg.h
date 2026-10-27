/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MT_MSDC_DEUBG__
#define __MT_MSDC_DEUBG__

#include "mtk_sd.h"
//#include "msdc_type.h"

#define MTK_MMC_DEBUG (1)
/*
 * snprintf may return a value of size or "more" to indicate
 * that the output was truncated, thus be careful of "more"
 * case.
 */
#define SPREAD_PRINTF(buff, size, evt, fmt, args...) \
do { \
	if (buff && size && *(size)) { \
		unsigned long var = snprintf(*(buff), *(size), fmt, ##args); \
		if (var > 0) { \
			if (var > *(size)) \
				var = *(size); \
			*(size) -= var; \
			*(buff) += var; \
		} \
	} \
	if (evt) \
		seq_printf(evt, fmt, ##args); \
	if (!buff && !evt) { \
		pr_info(fmt, ##args); \
	} \
} while (0)

#define MAGIC_CQHCI_DBG_TYPE 5
#define MAGIC_CQHCI_DBG_NUM_L 100
#define MAGIC_CQHCI_DBG_NUM_U 200
#define MAGIC_CQHCI_DBG_NUM_RI 500

#define MAGIC_CQHCI_DBG_TYPE_DCMD 60
/* softirq type */
#define MAGIC_CQHCI_DBG_TYPE_SIRQ 70

extern struct msdc_host *mtk_msdc_host[];
void dbg_add_host_log(struct mmc_host *mmc, int type, int cmd, int arg);
void dbg_add_sd_log(struct mmc_host *mmc, int type, int cmd, int arg);
void dbg_add_sirq_log(struct mmc_host *mmc, int type,
		int cmd, int arg, int cpu, unsigned long active_reqs);
void mmc_cmd_dump(char **buff, unsigned long *size, struct seq_file *m,
		struct mmc_host *mmc, u32 latest_cnt);
void msdc_dump_host_state(char **buff, unsigned long *size,
		struct seq_file *m, struct msdc_host *host);
#endif
