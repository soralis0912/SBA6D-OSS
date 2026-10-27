// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

//#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/seq_file.h>

#include "dbg.h"
#include "msdc_type.h"

#if MTK_MMC_DEBUG
struct msdc_host *mtk_msdc_host[] = {NULL, NULL};
/*#define MTK_MSDC_ERROR_TUNE_DEBUG*/

#define dbg_max_cnt (4000)
#define sd_dbg_max_cnt (1000)

#define MSDC_AEE_BUFFER_SIZE (300 * 1024)
struct dbg_run_host_log {
	unsigned long long time_sec;
	unsigned long long time_usec;
	int type;
	int cmd;
	int arg;
	int cpu;
	unsigned long active_reqs;
	int skip;
};

struct dbg_task_log {
	u32 address;
	unsigned long long size;
};
struct dbg_dma_cmd_log {
	unsigned long long time;
	int cmd;
	int arg;
};

static struct dbg_run_host_log dbg_run_host_log_dat[dbg_max_cnt];
static struct dbg_run_host_log dbg_run_sd_log_dat[dbg_max_cnt];
static struct dbg_dma_cmd_log dbg_dma_cmd_log_dat;
static struct dbg_task_log dbg_task_log_dat[32];
char msdc_aee_buffer[MSDC_AEE_BUFFER_SIZE];
static int dbg_host_cnt, dbg_sd_cnt;

static unsigned int print_cpu_test = UINT_MAX;

/*
 * type 0: cmd; type 1: rsp; type 3: dma end
 * when type 3: arg 0: no data crc error; arg 1: data crc error
 * @cpu, current CPU ID
 * @reserved, userd for softirq dump "data_active_reqs"
 */
inline void __dbg_add_host_log(struct mmc_host *mmc, int type,
			int cmd, int arg, int cpu, unsigned long reserved)
{
	unsigned long long t, tn;
	unsigned long long nanosec_rem;
	unsigned long flags;
	static int last_cmd, last_arg, skip;
	int l_skip = 0;
	struct msdc_host *host = mmc_priv(mmc);
	static int tag = -1;

	/* only log msdc0 */
	if (!host || host->id != 0)
		return;

	t = cpu_clock(print_cpu_test);
	spin_lock_irqsave(&host->log_lock, flags);
	switch (type) {
	case 0: /* normal - cmd */
		tn = t;
		nanosec_rem = do_div(t, 1000000000)/1000;
		if (cmd == 44) {
			tag = (arg >> 16) & 0x1f;
			dbg_task_log_dat[tag].size = arg & 0xffff;
		} else if (cmd == 45) {
			dbg_task_log_dat[tag].address = arg;
		} else if (cmd == 46 || cmd == 47) {
			dbg_dma_cmd_log_dat.time = tn;
			dbg_dma_cmd_log_dat.cmd = cmd;
			dbg_dma_cmd_log_dat.arg = arg;
		}

		dbg_run_host_log_dat[dbg_host_cnt].time_sec = t;
		dbg_run_host_log_dat[dbg_host_cnt].time_usec = nanosec_rem;
		dbg_run_host_log_dat[dbg_host_cnt].type = type;
		dbg_run_host_log_dat[dbg_host_cnt].cmd = cmd;
		dbg_run_host_log_dat[dbg_host_cnt].arg = arg;
		dbg_run_host_log_dat[dbg_host_cnt].skip = l_skip;
		dbg_host_cnt++;
		if (dbg_host_cnt >= dbg_max_cnt)
			dbg_host_cnt = 0;
		break;
	case 1: /* normal -rsp */
	case 5: /* cqhci - data */
	case 60: /* cqhci - dcmd */
	case 61: /* cqhci - dcmd resp */
		nanosec_rem = do_div(t, 1000000000)/1000;
		/*skip log if last cmd rsp are the same*/
		if (last_cmd == cmd &&
			last_arg == arg && cmd == 13) {
			skip++;
			if (dbg_host_cnt == 0)
				dbg_host_cnt = dbg_max_cnt;
			/*remove type = 0, command*/
			dbg_host_cnt--;
			break;
		}
		last_cmd = cmd;
		last_arg = arg;
		l_skip = skip;
		skip = 0;

		dbg_run_host_log_dat[dbg_host_cnt].time_sec = t;
		dbg_run_host_log_dat[dbg_host_cnt].time_usec = nanosec_rem;
		dbg_run_host_log_dat[dbg_host_cnt].type = type;
		dbg_run_host_log_dat[dbg_host_cnt].cmd = cmd;
		dbg_run_host_log_dat[dbg_host_cnt].arg = arg;
		dbg_run_host_log_dat[dbg_host_cnt].skip = l_skip;
		dbg_host_cnt++;
		if (dbg_host_cnt >= dbg_max_cnt)
			dbg_host_cnt = 0;
		break;
	/* add softirq record */
	case MAGIC_CQHCI_DBG_TYPE_SIRQ:
		tn = t;
		nanosec_rem = do_div(t, 1000000000)/1000;

		dbg_run_host_log_dat[dbg_host_cnt].time_sec = t;
		dbg_run_host_log_dat[dbg_host_cnt].time_usec = nanosec_rem;
		dbg_run_host_log_dat[dbg_host_cnt].type = type;
		dbg_run_host_log_dat[dbg_host_cnt].cmd = cmd;
		dbg_run_host_log_dat[dbg_host_cnt].arg = arg;
		dbg_run_host_log_dat[dbg_host_cnt].skip = l_skip;
		dbg_run_host_log_dat[dbg_host_cnt].cpu = cpu;
		/*For Kernel-4.19 not record requests bitmap, always 0*/
		dbg_run_host_log_dat[dbg_host_cnt].active_reqs = reserved;

		dbg_host_cnt++;
		if (dbg_host_cnt >= dbg_max_cnt)
			dbg_host_cnt = 0;
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&host->log_lock, flags);

}
inline void __dbg_add_sd_log(struct mmc_host *mmc, int type,
			int cmd, int arg)
{
	unsigned long long t, tn;
	unsigned long long nanosec_rem;
	static int last_cmd, last_arg, skip;
	int l_skip = 0;
	struct msdc_host *host = mmc_priv(mmc);

	/* only log msdc1 */
	if (!host || host->id == 0)
		return;

	t = cpu_clock(print_cpu_test);

	switch (type) {
	case 0: /* normal - cmd */
		tn = t;
		nanosec_rem = do_div(t, 1000000000)/1000;

		dbg_run_sd_log_dat[dbg_sd_cnt].time_sec = t;
		dbg_run_sd_log_dat[dbg_sd_cnt].time_usec = nanosec_rem;
		dbg_run_sd_log_dat[dbg_sd_cnt].type = type;
		dbg_run_sd_log_dat[dbg_sd_cnt].cmd = cmd;
		dbg_run_sd_log_dat[dbg_sd_cnt].arg = arg;
		dbg_run_sd_log_dat[dbg_sd_cnt].skip = l_skip;
		dbg_sd_cnt++;
		if (dbg_sd_cnt >= sd_dbg_max_cnt)
			dbg_sd_cnt = 0;
		break;
	case 1: /* normal -rsp */
		nanosec_rem = do_div(t, 1000000000)/1000;
		/*skip log if last cmd rsp are the same*/
		if (last_cmd == cmd &&
			last_arg == arg && cmd == 13) {
			skip++;
			if (dbg_sd_cnt == 0)
				dbg_sd_cnt = sd_dbg_max_cnt;
			/*remove type = 0, command*/
			dbg_sd_cnt--;
			break;
		}
		last_cmd = cmd;
		last_arg = arg;
		l_skip = skip;
		skip = 0;

		dbg_run_sd_log_dat[dbg_sd_cnt].time_sec = t;
		dbg_run_sd_log_dat[dbg_sd_cnt].time_usec = nanosec_rem;
		dbg_run_sd_log_dat[dbg_sd_cnt].type = type;
		dbg_run_sd_log_dat[dbg_sd_cnt].cmd = cmd;
		dbg_run_sd_log_dat[dbg_sd_cnt].arg = arg;
		dbg_run_sd_log_dat[dbg_sd_cnt].skip = l_skip;
		dbg_sd_cnt++;
		if (dbg_sd_cnt >= sd_dbg_max_cnt)
			dbg_sd_cnt = 0;
		break;
	default:
		break;
	}
}
/* all cases which except softirq of IO */
void dbg_add_host_log(struct mmc_host *mmc, int type,
		int cmd, int arg)
{
	__dbg_add_host_log(mmc, type, cmd, arg, -1, 0);
}

void dbg_add_sd_log(struct mmc_host *mmc, int type,
		int cmd, int arg)
{
	__dbg_add_sd_log(mmc, type, cmd, arg);
}

void dbg_add_sirq_log(struct mmc_host *mmc, int type,
		int cmd, int arg, int cpu, unsigned long active_reqs)
{
	__dbg_add_host_log(mmc, type, cmd, arg, cpu, active_reqs);
}

void mmc_cmd_dump(char **buff, unsigned long *size, struct seq_file *m,
	struct mmc_host *mmc, u32 latest_cnt)
{
	int i, j;
	int tag = -1;
	int is_read, is_rel, is_fprg;
	unsigned long long time_sec, time_usec;
	int type, cmd, arg, skip, cnt, cpu;
	unsigned long active_reqs;
	struct msdc_host *host;
	u32 dump_cnt;

	if (!mmc || !mmc->card)
		return;
	/* only dump msdc0 */
	host = mmc_priv(mmc);
	if (!host || host->id != 0)
		return;

	dump_cnt = min_t(u32, latest_cnt, dbg_max_cnt);

	i = dbg_host_cnt - 1;
	if (i < 0)
		i = dbg_max_cnt - 1;

	for (j = 0; j < dump_cnt; j++) {
		time_sec = dbg_run_host_log_dat[i].time_sec;
		time_usec = dbg_run_host_log_dat[i].time_usec;
		type = dbg_run_host_log_dat[i].type;
		cmd = dbg_run_host_log_dat[i].cmd;
		arg = dbg_run_host_log_dat[i].arg;
		skip = dbg_run_host_log_dat[i].skip;
		if (dbg_run_host_log_dat[i].type == 70) {
			cpu = dbg_run_host_log_dat[i].cpu;
			active_reqs = dbg_run_host_log_dat[i].active_reqs;
		} else {
			cpu = -1;
			active_reqs = 0;
		}
		if (cmd == 44 && !type) {
			cnt = arg & 0xffff;
			tag = (arg >> 16) & 0x1f;
			is_read = (arg >> 30) & 0x1;
			is_rel = (arg >> 31) & 0x1;
			is_fprg = (arg >> 24) & 0x1;
			SPREAD_PRINTF(buff, size, m,
		"%03d [%5llu.%06llu]%2d %3d %08x id=%02d %s cnt=%d %d %d\n",
				j, time_sec, time_usec,
				type, cmd, arg, tag,
				is_read ? "R" : "W",
				cnt, is_rel, is_fprg);
		} else if ((cmd == 46 || cmd == 47) && !type) {
			tag = (arg >> 16) & 0x1f;
			SPREAD_PRINTF(buff, size, m,
				"%03d [%5llu.%06llu]%2d %3d %08x id=%02d\n",
				j, time_sec, time_usec,
				type, cmd, arg, tag);
		} else
			SPREAD_PRINTF(buff, size, m,
			"%03d [%5llu.%06llu]%2d %3d %08x (%d) (0x%08lx) (%d)\n",
				j, time_sec, time_usec,
				type, cmd, arg, skip, active_reqs, cpu);
		i--;
		if (i < 0)
			i = dbg_max_cnt - 1;
	}

	SPREAD_PRINTF(buff, size, m,
		"claimed(%d), claim_cnt(%d), claimer pid(%d), comm %s\n",
		mmc->claimed, mmc->claim_cnt,
		mmc->claimer && mmc->claimer->task ?
			mmc->claimer->task->pid : 0,
		mmc->claimer && mmc->claimer->task ?
			mmc->claimer->task->comm : "NULL");
}

void msdc_dump_host_state(char **buff, unsigned long *size,
	struct seq_file *m, struct msdc_host *host)
{
	/* add log description*/
	SPREAD_PRINTF(buff, size, m,
		"column 1   : log number(Reverse order);\n");
	SPREAD_PRINTF(buff, size, m,
		"column 2   : kernel time\n");
	SPREAD_PRINTF(buff, size, m,
		"column 3   : type(0-cmd, 1-resp, 5-cqhci cmd, 60-cqhci dcmd doorbell,");
	SPREAD_PRINTF(buff, size, m,
		"61-cqhci dcmd complete(irq in), 70-cqhci softirq in);\n");
	SPREAD_PRINTF(buff, size, m,
		"column 4&5 : cmd index&arg(1XX-task XX's task descriptor low 32bit, ");
	SPREAD_PRINTF(buff, size, m,
		"2XX-task XX's task descriptor high 32bit, ");
	SPREAD_PRINTF(buff, size, m,
		"5XX-task XX's task completion(irq in), ");
	SPREAD_PRINTF(buff, size, m,
		"others index-command index(non 70 type) or cmd/data error(70 type)) ");
	SPREAD_PRINTF(buff, size, m,
		"others arg-command arg(non 70 type) or cmdq_req->tag(70 type));\n");
	SPREAD_PRINTF(buff, size, m,
		"column 6   : repeat count(The role of problem analysis is low);\n");
	SPREAD_PRINTF(buff, size, m,
		"column 7   : record data_active_reqs;\n");
	SPREAD_PRINTF(buff, size, m,
		"column 8   : only record softirq's running CPU id(only for 70 type);\n");
}

void get_msdc_aee_buffer(unsigned long *vaddr, unsigned long *size)
{
	struct msdc_host *host = mtk_msdc_host[0];
	unsigned long free_size = MSDC_AEE_BUFFER_SIZE;
	char *buff;

	if (host == NULL) {
		pr_info("====== Null msdc, dump skipped ======\n");
		return;
	}

	buff = msdc_aee_buffer;
	msdc_dump_host_state(&buff, &free_size, NULL, host);
	mmc_cmd_dump(&buff, &free_size, NULL, host->mmc, dbg_max_cnt);
	/* retrun start location */
	*vaddr = (unsigned long)msdc_aee_buffer;
	*size = MSDC_AEE_BUFFER_SIZE - free_size;
}
EXPORT_SYMBOL(get_msdc_aee_buffer);
#else
void dbg_add_host_log(struct mmc_host *mmc, int type, int cmd, int arg)
{
	//pr_info("config MTK_MMC_DEBUG is not set: %s!\n",__func__);
}
void dbg_add_sd_log(struct mmc_host *mmc, int type,
		int cmd, int arg)
{
	//pr_info("config MTK_MMC_DEBUG is not set: %s!\n",__func__);
}

void dbg_add_sirq_log(struct mmc_host *mmc, int type,
	int cmd, int arg, int cpu, unsigned long active_reqs)
{
	//pr_info("config MTK_MMC_DEBUG is not set: %s!\n",__func__);
}

void mmc_cmd_dump(char **buff, unsigned long *size, struct seq_file *m,
	struct mmc_host *mmc, u32 latest_cnt)
{
	//pr_info("config MTK_MMC_DEBUG is not set: %s!\n",__func__);
}
void msdc_dump_host_state(char **buff, unsigned long *size,
		struct seq_file *m, struct msdc_host *host)
{
	//pr_info("config MTK_MMC_DEBUG is not set: %s!\n",__func__);
}
static void msdc_proc_dump(struct seq_file *m, u32 id)
{
	//pr_info("config MTK_MMC_DEBUG is not set : %s!\n",__func__);
}
void get_msdc_aee_buffer(unsigned long *vaddr, unsigned long *size)
{
	//pr_info("config MTK_MMC_DEBUG is not set : %s!\n",__func__);
}
#endif
