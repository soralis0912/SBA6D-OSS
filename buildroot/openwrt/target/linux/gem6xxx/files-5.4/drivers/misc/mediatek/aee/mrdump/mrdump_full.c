// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 */

#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/kexec.h>
#include <linux/module.h>
#include <linux/processor.h>

#include <mt-plat/mboot_params.h>
#include <mrdump.h>
#include "mrdump_private.h"

#ifndef CONFIG_KEXEC_CORE
/* note_buf_t defined in linux/crash_core.h included by linux/kexec.h */
static note_buf_t __percpu *crash_notes;
#endif

static unsigned long mrdump_output_lbaooo;

#ifndef CONFIG_KEXEC_CORE
static u32 *mrdump_append_elf_note(u32 *buf, char *name, unsigned int type,
				   void *data, size_t data_len)
{
	struct elf_note note;

	note.n_namesz = strlen(name) + 1;
	note.n_descsz = data_len;
	note.n_type = type;
	memcpy(buf, &note, sizeof(note));
	buf += (sizeof(note) + 3) / 4;
	memcpy(buf, name, note.n_namesz);
	buf += (note.n_namesz + 3) / 4;
	memcpy(buf, data, note.n_descsz);
	buf += (note.n_descsz + 3) / 4;

	return buf;
}

static void mrdump_final_note(u32 *buf)
{
	struct elf_note note;

	note.n_namesz = 0;
	note.n_descsz = 0;
	note.n_type = 0;
	memcpy(buf, &note, sizeof(note));
}

static void crash_save_cpu(struct pt_regs *regs, int cpu)
{
	struct elf_prstatus prstatus;
	u32 *buf;

	if ((cpu < 0) || (cpu >= nr_cpu_ids))
		return;
	if (!crash_notes)
		return;
	buf = (u32 *)per_cpu_ptr(crash_notes, cpu);
	if (!buf)
		return;
	memset(&prstatus, 0, sizeof(prstatus));
	prstatus.pr_pid = current->pid;
	elf_core_copy_kernel_regs((elf_gregset_t *)&prstatus.pr_reg, regs);
	buf = mrdump_append_elf_note(buf, CRASH_CORE_NOTE_NAME, NT_PRSTATUS,
				     &prstatus, sizeof(prstatus));
	mrdump_final_note(buf);
}
#endif

#if IS_ENABLED(CONFIG_SYSFS)
static ssize_t mrdump_version_show(struct module_attribute *attr,
		struct module_kobject *kobj, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s", MRDUMP_GO_DUMP);
}

static struct module_attribute mrdump_version_attribute =
	__ATTR(version, 0400, mrdump_version_show, NULL);

static struct attribute *attrs[] = {
	&mrdump_version_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int __init mrdump_sysfs_init(void)
{
	struct kobject *kobj;
	struct kset *p_module_kset = aee_get_module_kset();

	if (!p_module_kset) {
		pr_notice("MT-RAMDUMP: Cannot find module_kset");
		return -EINVAL;
	}

	kobj = kset_find_obj(p_module_kset, KBUILD_MODNAME);
	if (kobj) {
		if (sysfs_create_group(kobj, &attr_group)) {
			pr_notice("MT-RAMDUMP: sysfs create sysfs failed\n");
			return -ENOMEM;
		}
	} else {
		pr_notice("MT-RAMDUMP: Cannot find module %s object\n",
				KBUILD_MODNAME);
		return -EINVAL;
	}

	pr_info("%s: done.\n", __func__);
	return 0;
}
#ifndef MODULE
module_init(mrdump_sysfs_init);
#endif
#endif

int __init mrdump_full_init(void)
{
	mrdump_cblock->enabled = MRDUMP_ENABLE_COOKIE;
#ifdef MODULE
#if IS_ENABLED(CONFIG_SYSFS)
	mrdump_sysfs_init();
#endif
#endif
	pr_info("%s: MT-RAMDUMP enabled done\n", __func__);
	return 0;
}

static int param_set_mrdump_lbaooo(const char *val,
		const struct kernel_param *kp)
{
	int retval = 0;

	if (mrdump_cblock) {
		retval = param_set_ulong(val, kp);
		if (!retval)
			mrdump_cblock->output_fs_lbaooo = mrdump_output_lbaooo;
	}

	return retval;
}

/* sys/modules/mrdump/parameter/lbaooo */
struct kernel_param_ops param_ops_mrdump_lbaooo = {
	.set = param_set_mrdump_lbaooo,
	.get = param_get_ulong,
};

param_check_ulong(lbaooo, &mrdump_output_lbaooo);
/* 0644: S_IRUGO | S_IWUSR */
module_param_cb(lbaooo, &param_ops_mrdump_lbaooo, &mrdump_output_lbaooo,
		0644);
__MODULE_PARM_TYPE(lbaooo, "ulong");

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek MRDUMP module");
MODULE_AUTHOR("MediaTek Inc.");
