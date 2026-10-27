// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/seq_file.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "proc.h"
#include "safexcel.h"

#define PROCREG_DIR				"safexcel"
#define PROCREG_DISABLE_EIP97	"disable_EIP97"
#define PROCREG_ENABLE_LOG		"enable_Log"
#define PROCREG_REF_CNT			"ref_cnt"
#define PROCREG_VCORE			"vcore"

static struct proc_dir_entry *eip97_proc_dir;
static struct proc_dir_entry *proc_disable_eip97;
static struct proc_dir_entry *proc_enable_log;
static struct proc_dir_entry *proc_ref_cnt;
static struct proc_dir_entry *proc_eip97_vcore;

static debug_proc_update_func g_callback;
static void *g_priv;

int dbg_disable_eip97 = DEFAULT_DISABLE_EIP97;
EXPORT_SYMBOL(dbg_disable_eip97);

int dbg_enable_log = DEFAULT_ENABLE_LOG;
EXPORT_SYMBOL(dbg_enable_log);

int dbg_eip97_vcore_max = DEFAULT_VCORE_MAX;
EXPORT_SYMBOL(dbg_eip97_vcore_max);

int dbg_eip97_vcore_min = DEFAULT_VCORE_MIN_MT6890;
EXPORT_SYMBOL(dbg_eip97_vcore_min);

int enable_log_read(struct seq_file *seq, void *v)
{
	seq_puts(seq, dbg_enable_log ? "true\n" : "false\n");
	return 0;
}

ssize_t enable_log_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
	pr_info("enable_log_write in!!\n");

	if (count > 0) {
		char c;
		int val, changed;

		if (get_user(c, buffer))
			return -EFAULT;

		val = (c != '0');
		changed = val != dbg_enable_log;
		dbg_enable_log = val;

		if (changed && g_callback)
			g_callback(PROC_UPDATE_ENABLE_LOG, g_priv);

		pr_info("enable_log_write: c=%c, dbg_enable_log = %d\n", c, dbg_enable_log);
	}

	return count;
}

static int enable_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, enable_log_read, NULL);
}


static const struct file_operations enable_log_fops = {
	.owner = THIS_MODULE,
	.open = enable_log_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = enable_log_write,
	.release = single_release
};


int disable_eip97_read(struct seq_file *seq, void *v)
{
	seq_puts(seq, dbg_disable_eip97 ? "true\n" : "false\n");
	return 0;
}

ssize_t disable_eip97_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
	pr_info("disable_eip97_write in!!\n");

	if (count > 0) {
		char c;
		int val, changed;

		if (get_user(c, buffer))
			return -EFAULT;

		val = (c != '0');
		changed = val != dbg_disable_eip97;
		dbg_disable_eip97 = val;

		if (changed && g_callback)
			g_callback(PROC_UPDATE_DISABLE_EIP97, g_priv);

		pr_info("disable_eip97_write: c=%c, dbg_disable_eip97 = %d\n", c, dbg_disable_eip97);
	}

	return count;
}

static int disable_eip97_open(struct inode *inode, struct file *file)
{
	return single_open(file, disable_eip97_read, NULL);
}


static const struct file_operations disable_eip97_fops = {
	.owner = THIS_MODULE,
	.open = disable_eip97_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = disable_eip97_write,
	.release = single_release
};


static int ref_cnt_read(struct seq_file *seq, void *v)
{
	struct safexcel_crypto_priv *crypto_priv = g_priv;

	seq_printf(seq, "ref_cnt = %d\n", crypto_priv->ref_cnt);
	return 0;
}

static int ref_cnt_open(struct inode *inode, struct file *file)
{
	return single_open(file, ref_cnt_read, NULL);
}

static const struct file_operations ref_cnt_fops = {
	.owner = THIS_MODULE,
	.open = ref_cnt_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};


static int eip97_vcore_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "vcore min = %d, max = INT_MAX\n", dbg_eip97_vcore_min);

	return 0;
}

static ssize_t eip97_vcore_write(struct file *file, const char __user *ptr,
			 size_t len, loff_t *data)
{
	if (len > 0) {
		int val, changed, ret;
		char buf[32];

		if (len > sizeof(buf) - 1)
			len = sizeof(buf) - 1;

		ret = strncpy_from_user(buf, ptr, len);
		if (ret < 0)
			return ret;
		buf[len] = '\0';

		if (kstrtoint(buf, 10, &val))
			return -EINVAL;

		changed = val != dbg_eip97_vcore_min;
		dbg_eip97_vcore_min = val;

		if (changed && g_callback)
			g_callback(PROC_UPDATE_EIP97_VCORE, g_priv);
	}

	return len;
}

static int eip97_vcore_open(struct inode *inode, struct file *file)
{
	return single_open(file, eip97_vcore_read, NULL);
}


static const struct file_operations eip97_vcore_fops = {
	.owner = THIS_MODULE,
	.open = eip97_vcore_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = eip97_vcore_write,
	.release = single_release
};

int safexcel_proc_init(debug_proc_update_func callback, void *priv)
{
	g_callback = callback;
	g_priv = priv;

	if (!eip97_proc_dir)
		eip97_proc_dir = proc_mkdir(PROCREG_DIR, NULL);

	dbg_disable_eip97 = DEFAULT_DISABLE_EIP97;
	proc_disable_eip97 = proc_create(PROCREG_DISABLE_EIP97, 0440,
				      eip97_proc_dir, &disable_eip97_fops);
	if (!proc_disable_eip97)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_DISABLE_EIP97);

	dbg_enable_log = DEFAULT_ENABLE_LOG;
	proc_enable_log = proc_create(PROCREG_ENABLE_LOG, 0440,
				      eip97_proc_dir, &enable_log_fops);
	if (!proc_enable_log)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_ENABLE_LOG);

	proc_ref_cnt = proc_create(PROCREG_REF_CNT, 0440,
				      eip97_proc_dir, &ref_cnt_fops);
	if (!proc_ref_cnt)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_REF_CNT);

	if (EIP97IES_MRVL_MT6990 != ((struct safexcel_crypto_priv *)priv)->version)
	{
		dbg_eip97_vcore_min = DEFAULT_VCORE_MIN_MT6890;
	}
	else
	{
		dbg_eip97_vcore_min = DEFAULT_VCORE_MIN_MT6990;
	}
	proc_eip97_vcore = proc_create(PROCREG_VCORE, 0440,
				      eip97_proc_dir, &eip97_vcore_fops);
	if (!proc_eip97_vcore)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_VCORE);

	return 0;
}

void safexcel_proc_exit(void)
{
	pr_info("proc exit\n");

	if (!eip97_proc_dir)
		return;

	if (proc_disable_eip97)
		remove_proc_entry(PROCREG_DISABLE_EIP97, eip97_proc_dir);

	if (proc_enable_log)
		remove_proc_entry(PROCREG_ENABLE_LOG, eip97_proc_dir);

	if (proc_ref_cnt)
		remove_proc_entry(PROCREG_REF_CNT, eip97_proc_dir);

	if (proc_eip97_vcore)
		remove_proc_entry(PROCREG_VCORE, eip97_proc_dir);

	remove_proc_entry(PROCREG_DIR, NULL);
}

