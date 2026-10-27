// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/audit.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <linux/freezer.h>
#include <linux/tty.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/signal.h>
#include "mtk_selinux_warning.h"

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif

#define PRINT_BUF_LEN   100
#define MOD		"SELINUX"
#define SCONTEXT_FILTER
#define AV_FILTER
#define MTK_SELINUX_FORCE_WARNING

static const char *aee_filter_list[AEE_FILTER_NUM] = {
};

#define AEE_AV_FILTER_NUM 5
static const char *aee_av_filter_list[AEE_AV_FILTER_NUM] = {
};

#define SKIP_PATTERN_NUM 5
static const char *skip_pattern[SKIP_PATTERN_NUM] = {
	"comm=\"find\" name=\"/\" dev=\"ubifs\" ino=1 scontext=system_u:system_r:init_t tcontext=system_u:object_r:protect_f_file_t",
	"comm=\"find\" name=\"/\" dev=\"ubifs\" ino=1 scontext=system_u:system_r:init_t tcontext=system_u:object_r:protect_s_file_t",
	"comm=\"find\" path=\"/mnt/vendor/mdota\" dev=\"ubifs\" ino=1 scontext=system_u:system_r:init_t tcontext=system_u:object_r:mdota_file_t",
	"comm=\"find\" path=\"/mnt/vendor/mdota2\" dev=\"ubifs\" ino=1 scontext=system_u:system_r:init_t tcontext=system_u:object_r:mdota_file_t",
};


static int mtk_check_filter(char *scontext);
static int mtk_get_scontext(char *data, char *buf);
static char *mtk_get_process(char *in);

/*
 * call aee warning handling when selinux warning triggered
 */
static void mtk_selinux_warn_handle(char *data)
{
	char scontext[AEE_FILTER_LEN] = { '\0' };
	char *pname = scontext;

	if (!mtk_get_scontext(data, scontext))
		return;

	pname = mtk_get_process(scontext);
	if (pname == 0)
		return;

	pr_info("[%s] %s (scontext=%s, process=%s)\n",
		MOD, data, scontext, pname);

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
	{
		char printbuf[PRINT_BUF_LEN] = { '\0' };

		snprintf(printbuf, PRINT_BUF_LEN - 1,
			"[%s][WARNING]\nCR_DISPATCH_PROCESSNAME:%s\n",
			MOD, pname);

		aee_kernel_warning_api(__FILE__, __LINE__,
			DB_OPT_DEFAULT | DB_OPT_NATIVE_BACKTRACE,
			printbuf, data);
	}
#endif
}

static int mtk_check_filter(char *scontext)
{
	int i = 0;

	/* check whether scontext in filter list */
	for (i = 0; i < AEE_FILTER_NUM && aee_filter_list[i] != NULL; i++) {
		if (strcmp(scontext, aee_filter_list[i]) == 0)
			return i;
	}

	return -1;
}

static bool mtk_check_skip_pattern(char *data)
{
	int i = 0;

	/* check whether the log contains specific pattern */
	for (i = 0; i < SKIP_PATTERN_NUM && skip_pattern[i] != NULL; i++) {
		if (strstr(data, skip_pattern[i]) != NULL)
			return true;
	}

	return false;
}

#define AV_LEN 30
static void mtk_check_av(char *data)
{
	char *start = NULL;
	char *end = NULL;
	char av_buf[AV_LEN] = { '\0' };
	char scontext[AEE_FILTER_LEN] = { '\0' };
	char printbuf[PRINT_BUF_LEN] = { '\0' };
	char *pname = scontext;
	char *iter;
	int i;

	if (!mtk_get_scontext(data, scontext))
		return;

	pname = mtk_get_process(scontext);
	if (pname == 0)
		return;

	start = strstr(data, "denied  { ");
	end = strstr(data, "}");
	if (start == NULL || end == NULL || end < start)
		return;

	start = start+10;

	iter = start;
	while (iter < end) {
		if (*iter == ' ' && iter-start > 0 && iter-start < AV_LEN) {
			strncpy(av_buf, start, iter-start);
			for (i = 0;
				i < AEE_AV_FILTER_NUM &&
				aee_av_filter_list[i] != NULL;
				++i) {

				if (strcmp(av_buf,
					aee_av_filter_list[i]) == 0) {

					if (mtk_check_skip_pattern(data))
						return;

					memset(printbuf, '\0', PRINT_BUF_LEN);

					snprintf(printbuf, PRINT_BUF_LEN-1,
						"[%s][WARNING]\nCR_DISPATCH_PROCESSNAME:%s\n",
						MOD, pname);
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
					aee_kernel_warning_api(
						__FILE__, __LINE__,
						DB_OPT_DEFAULT | DB_OPT_NATIVE_BACKTRACE,
						printbuf, data);
#endif
				}

			}

			start = iter+1;
		}
		iter++;
	}
}

static int mtk_get_scontext(char *data, char *buf)
{
	char *t1;
	char *t2;
	int diff = 0;

	t1 = strstr(data, "scontext=");

	if (t1 == NULL)
		return 0;

	t1 += 9;
	t2 = strchr(t1, ' ');

	if (t2 == NULL)
		return 0;

	diff = t2 - t1;
	if (diff >= AEE_FILTER_LEN)
		return 0;

	strncpy(buf, t1, diff);
	return 1;
}


static char *mtk_get_process(char *in)
{
	char *out = in;
	char *tmp;
	int i;

	/* omit two ':' */
	for (i = 0; i < 2; i++) {
		out = strchr(out, ':');
		if (out == NULL)
			return 0;
		out = out + 1;
	}

    /* if mls is enabled */
	tmp = strchr(out, ':');

	if (tmp != NULL)
		*tmp = '\0';

	return out;
}

void mtk_audit_hook(char *data)
{
#ifdef SCONTEXT_FILTER
	char scontext[AEE_FILTER_LEN] = { '\0' };
	char *pname = scontext;
	int ret = 0;

	/* get scontext from avc warning */
	ret = mtk_get_scontext(data, scontext);
	if (!ret)
		return;

	/* check scontext is in warning list */
	ret = mtk_check_filter(scontext);
	if (ret >= 0) {
		pr_debug("[%s], In AEE Warning List scontext: %s\n",
			MOD, scontext);

		if (!IS_ENABLED(CONFIG_MTK_AEE_FEATURE))
			return;

		pname = mtk_get_process(scontext);

		if (pname != 0) {
			char printbuf[PRINT_BUF_LEN] = { '\0' };


			snprintf(printbuf, PRINT_BUF_LEN-1,
				"[%s][WARNING]\nCR_DISPATCH_PROCESSNAME:%s\n",
				MOD, pname);

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
			aee_kernel_warning_api(__FILE__, __LINE__,
				DB_OPT_DEFAULT | DB_OPT_NATIVE_BACKTRACE,
				printbuf, data);
#endif
		}
	}
#endif
#ifdef AV_FILTER
	mtk_check_av(data);
#endif
#ifdef MTK_SELINUX_FORCE_WARNING
	if (!mtk_check_skip_pattern(data))
		mtk_selinux_warn_handle(data);
#endif
}
static int __init selinux_init(void)
{
	mtk_audit_hook_set(mtk_audit_hook);
	pr_info("[SELinux] MTK SELinux init done\n");

	return 0;
}

static void __exit selinux_exit(void)
{
	pr_info("[SELinux] MTK SELinux func exit\n");
}

module_init(selinux_init);
module_exit(selinux_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek SELINUX Driver");
MODULE_AUTHOR("Kuan-Hsin Lee <kuan-hsin.lee@mediatek.com>");
