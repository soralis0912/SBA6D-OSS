// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 MediaTek Inc.
/*
 * Author: Kurt Yang <kurt.yang@mediatek.com>
 */
#include <linux/seq_file.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "mtk_edma.h"
#include "mtk_fe_proc.h"
#include <soc/mediatek/emi.h>

static struct proc_dir_entry *fe_proc_dir;
static struct proc_dir_entry *proc_fe_dump_users;
static struct proc_dir_entry *proc_fe_dbg_dma;
static struct proc_dir_entry *proc_fe_dbg_level;
static struct proc_dir_entry *proc_fe_dump_dma;
static struct proc_dir_entry *proc_fe_dump_emi;

extern struct mtk_edma_module *g_edma;

int fe_dbg_level = 0;
int fe_dump_emi = 0;

int str_to_int(char *s)
{
	int res = 0, i;

	for(i = 0; s[i] != '\0'; i++)
	{
		if(s[i] < '0' || s[i] > '9')
			return -1;
		else
			res = res * 10 + (s[i] - '0');
	}

	return res;
}

struct fe_proc_operation *dbg_ops;

static ssize_t fe_dbg_dma_write(struct file *file,
		const char __user *buffer, size_t len, loff_t *data)
{
	char buf[100] = {0};
	char *p_buf;
	char *p_dma = NULL, *p_token[100] = {0};
	char *p_delimiter = " \t\n";
	int i;

	if (len >= sizeof(buf)) {
		pr_crit("input handling fail!\n");
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	p_buf = buf;
	p_dma = strsep(&p_buf, p_delimiter);
	if (!p_dma)
		goto err;

	for (i = 0, p_token[i] = strsep(&p_buf, p_delimiter);
			p_token[i] && strlen(p_token[i]);
			p_token[++i] = strsep(&p_buf, p_delimiter)) {
	}
	dbg_ops->dbg_cmds(p_dma, p_token);

err:
	return len;
}

static int fe_dbg_dma_read(struct seq_file *seq, void *v)
{
	seq_puts(seq, "dbg_dma cmds:\n");
	seq_printf(seq,
		"echo [dma-name] [cmd] [subcmd] [arg0] [arg1] > /proc/%s/%s\n",
		PROCREG_DIR, PROCREG_DBG_DMA);

	seq_puts(seq, "Example:\n");

	seq_printf(seq, "echo edma help > /proc/%s/%s\n",
		PROCREG_DIR, PROCREG_DBG_DMA);
	return 0;
}

static int fe_dbg_dma_open(struct inode *inode, struct file *file)
{
	return single_open(file, fe_dbg_dma_read, NULL);
}

static const struct file_operations fe_dbg_dma_ops = {
	.owner = THIS_MODULE,
	.open = fe_dbg_dma_open,
	.read = seq_read,
	.write  = fe_dbg_dma_write,
	.llseek = seq_lseek,
	.release = single_release
};

static int fe_dump_users_read(struct seq_file *seq, void *v)
{
	struct list_head *usrs;
	unsigned int *ref_cnt;
	struct mtk_fe_token *usr;
	int ret;

	ret = dbg_ops->get_usrs_info(&usrs, &ref_cnt);

	if (ret) {
		seq_printf(seq, "can't dump current users info, ret = %d\n",
				ret);
		return 0;
	}

	seq_printf(seq, "ref_cnt = %d\n", *ref_cnt);
	list_for_each_entry(usr, usrs, list) {
		seq_printf(seq, "user: %s\n", usr->user);
	}
	return 0;
}

static int fe_dump_users_open(struct inode *inode, struct file *file)
{
	return single_open(file, fe_dump_users_read, NULL);
}

static const struct file_operations fe_dump_users_ops = {
	.owner = THIS_MODULE,
	.open = fe_dump_users_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static ssize_t fe_dbg_level_write(struct file *file,
                const char __user *buffer, size_t len, loff_t *data)
{
        char buf[3] = {0};
        int i;

        if (len > sizeof(buf) || len < 1) {
		pr_crit("Input length is invalid!\n\n");
		pr_crit("Example:\n");
		pr_crit("echo [0~10] > /proc/%s/%s\n",
		PROCREG_DIR, PROCREG_DBG_LEVEL);
                return -1;
        }

        if (copy_from_user(buf, buffer, len))
                return -EFAULT;

	buf[len - 1] = '\0';

	if(str_to_int(buf) == -1){
		pr_crit("Input parameter is invalid!\n\n");
		pr_crit("Example:\n");
		pr_crit("echo [0~10] > /proc/%s/%s\n",
		PROCREG_DIR, PROCREG_DBG_LEVEL);
                return -1;
	}
	else
		fe_dbg_level = str_to_int(buf);

        return len;
}


static int fe_dbg_level_read(struct seq_file *seq, void *v)
{
        seq_printf(seq, "%d\n", fe_dbg_level);
        return 0;
}

static int fe_dbg_level_open(struct inode *inode, struct file *file)
{
        return single_open(file, fe_dbg_level_read, NULL);
}

static const struct file_operations fe_dbg_level_ops = {
	.owner = THIS_MODULE,
	.open = fe_dbg_level_open,
	.read = seq_read,
	.write  = fe_dbg_level_write,
	.llseek = seq_lseek,
	.release = single_release
};

static ssize_t fe_dump_emi_write(struct file *file,
                const char __user *buffer, size_t len, loff_t *data)
{
        char buf[7] = {0};
        int i;

        if (len > sizeof(buf) || len < 1) {
		pr_crit("Input length is invalid!\n\n");
		pr_crit("Example:\n");
		pr_crit("echo xxx > /proc/%s/%s\n",
		PROCREG_DIR, PROCREG_DBG_LEVEL);
                return -1;
        }

        if (copy_from_user(buf, buffer, len))
                return -EFAULT;

	buf[len - 1] = '\0';

	if(str_to_int(buf) == -1){
		pr_crit("Input parameter is invalid!\n\n");
		pr_crit("Example:\n");
		pr_crit("echo xxx > /proc/%s/%s\n",
		PROCREG_DIR, PROCREG_DBG_LEVEL);
                return -1;
	}
	else
		fe_dump_emi = str_to_int(buf);

	if (fe_dump_emi == 0) {
		pr_crit("Stop EMI Monitor!\n");
		mtk_emicen_emi_monitor_stop();
	} else {
		pr_crit("Start EMI Monitor every %d ms!\n", fe_dump_emi);
		mtk_emicen_emi_monitor_init(fe_dump_emi);
	}

        return len;
}

static int fe_dump_emi_read(struct seq_file *seq, void *v)
{
        seq_printf(seq, "%d\n", fe_dump_emi);
        return 0;
}

static int fe_dump_emi_open(struct inode *inode, struct file *file)
{
        return single_open(file, fe_dump_emi_read, NULL);
}

static const struct file_operations fe_dump_emi_ops = {
	.owner = THIS_MODULE,
	.open = fe_dump_emi_open,
	.read = seq_read,
	.write  = fe_dump_emi_write,
	.llseek = seq_lseek,
	.release = single_release
};

static int fe_dump_dma_read(struct seq_file *seq, void *v)
{
	pr_crit("dump edma txd and rxd\n");
	u32 i, edma_idx, ring_idx, base_phy_addr, max_cnt;
	u32 *p;
	void __iomem *base_virt_addr;

	seq_printf(seq, "dump edma txd\n");
	for (edma_idx = 0; edma_idx < EDMA_MODULE_NUM; edma_idx++) {
		for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_TX_RING_NUM; ring_idx++) {
			seq_printf(seq, "edma-%d tx ring-%d\n", edma_idx, ring_idx);
			base_phy_addr = mtk_edma_r32(g_edma, ETX_BASE_PTR_CFG(edma_idx, ring_idx));
			base_virt_addr = phys_to_virt(base_phy_addr);

			if (base_virt_addr == NULL) {
				seq_printf(seq, "phys_to_virt fail, base_virt_addr=%px\n", base_virt_addr);
				return 0;
			}

			p = (u32 *)base_virt_addr;

			max_cnt = mtk_edma_r32(g_edma, ETX_MAX_CNT_CFG(edma_idx, ring_idx));

			seq_printf(seq, "ETX_MAX_CNT_CFG = %d\n", max_cnt);

			if (max_cnt == 0) {
				seq_printf(seq, "Max Count = 0, do not print descriptor\n");
				continue;
			}

			for (i = 0; i < max_cnt; i++) {
				seq_printf(seq, "TX(%d)[%px/%08llX] %08X %08X %08X %08X %08X %08X %08X %08X\n",
					i, p, base_phy_addr + ((u64)p - (u64)base_virt_addr),
					*p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7));

				p += 8;
			}
		}
	}

	seq_puts(seq, "dump edma rxd\n");
	for (edma_idx = 0; edma_idx < EDMA_MODULE_NUM; edma_idx++) {
		for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_RX_RING_NUM; ring_idx++) {
			seq_printf(seq, "edma-%d rx ring-%d\n", edma_idx, ring_idx);
			base_phy_addr = mtk_edma_r32(g_edma, ERX_BASE_PTR_CFG(edma_idx, ring_idx));
			base_virt_addr = phys_to_virt(base_phy_addr);

			if (base_virt_addr == NULL) {
				seq_printf(seq, "phys_to_virt fail, base_virt_addr=%px\n", base_virt_addr);
				return 0;
			}

			p = (u32 *)base_virt_addr;

			max_cnt = mtk_edma_r32(g_edma, ERX_MAX_CNT_CFG(edma_idx, ring_idx));

			seq_printf(seq, "ERX_MAX_CNT_CFG = %d\n", max_cnt);

			if (max_cnt == 0) {
				seq_printf(seq, "Max Count = 0, do not print descriptor\n");
				continue;
			}

			for (i = 0; i < max_cnt; i++) {
				seq_printf(seq, "RX(%d)[%px/%08llX] %08X %08X %08X %08X %08X %08X %08X %08X\n",
					i, p, base_phy_addr + ((u64)p - (u64)base_virt_addr),
					*p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7));

				p += 8;
			}
		}
	}

	return 0;
}

static int fe_dump_dma_open(struct inode *inode, struct file *file)
{
	return single_open(file, fe_dump_dma_read, NULL);
}

static const struct file_operations fe_dump_dma_ops = {
	.owner = THIS_MODULE,
	.open = fe_dump_dma_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int fe_proc_init(struct fe_proc_operation *ops)
{
	dbg_ops = ops;

	if (!fe_proc_dir)
		fe_proc_dir = proc_mkdir(PROCREG_DIR, NULL);

	proc_fe_dump_users = proc_create(PROCREG_DUMP_USERS, 0400,
				      fe_proc_dir, &fe_dump_users_ops);
	if (!proc_fe_dump_users)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_DUMP_USERS);

	proc_fe_dbg_dma = proc_create(PROCREG_DBG_DMA, 0400,
					  fe_proc_dir, &fe_dbg_dma_ops);
	if (!proc_fe_dbg_dma)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_DBG_DMA);

	proc_fe_dbg_level = proc_create(PROCREG_DBG_LEVEL, 0400,
					  fe_proc_dir, &fe_dbg_level_ops);
	if (!proc_fe_dbg_level)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_DBG_LEVEL);

	proc_fe_dump_dma = proc_create(PROCREG_DUMP_DMA, 0400,
					  fe_proc_dir, &fe_dump_dma_ops);
	if (!proc_fe_dump_dma)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_DUMP_DMA);

	proc_fe_dump_emi = proc_create(PROCREG_DUMP_EMI, 0400,
					  fe_proc_dir, &fe_dump_emi_ops);
	if (!proc_fe_dump_emi)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_DUMP_EMI);

	return 0;
}

void fe_proc_exit(void)
{
	pr_info("proc exit\n");
	if (proc_fe_dump_users)
		remove_proc_entry(PROCREG_DUMP_USERS, proc_fe_dump_users);

	if (proc_fe_dbg_dma)
		remove_proc_entry(PROCREG_DBG_DMA, proc_fe_dbg_dma);

	if (proc_fe_dbg_level)
		remove_proc_entry(PROCREG_DBG_LEVEL, proc_fe_dbg_level);

	if (proc_fe_dump_dma)
		remove_proc_entry(PROCREG_DUMP_DMA, proc_fe_dump_dma);

	if (fe_proc_dir)
		remove_proc_entry(PROCREG_DIR, NULL);
}
