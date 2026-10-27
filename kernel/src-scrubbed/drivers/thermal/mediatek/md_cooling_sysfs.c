// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <md_cooling.h>

#define MD_COOLING_ATTR_RO(_name) \
static struct kobj_attribute _name##_attr = __ATTR_RO(_name)
#define MD_COOLING_ATTR_WO(_name) \
static struct kobj_attribute _name##_attr = __ATTR_WO(_name)

static ssize_t status_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct md_cooling_device *md_cdev;
	int len = 0, i, j;
	unsigned int pa_num = get_pa_num();

	len += snprintf(buf + len, PAGE_SIZE - len, "PA num = %d\n", pa_num);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"MD status = %d\n", get_md_status());

	for (i = 0; i < NR_MD_COOLING_TYPE; i++) {
		for (j = 0; j < pa_num; j++) {
			md_cdev = get_md_cdev((enum md_cooling_type)i, j);
			if (!md_cdev)
				continue;

			len += snprintf(buf + len, PAGE_SIZE - len,
				"\n[%s, %d]\n", md_cdev->name, md_cdev->pa_id);
			len += snprintf(buf + len, PAGE_SIZE - len,
				"target/max = %ld/%ld\n",
				md_cdev->target_level, md_cdev->max_level);
			if (md_cdev->type == MD_COOLING_TYPE_TX_PWR)
				len += snprintf(buf + len, PAGE_SIZE - len,
					"throttle tx_pwr = %d/%d/%d\n",
					md_cdev->throttle_tx_power[0],
					md_cdev->throttle_tx_power[1],
					md_cdev->throttle_tx_power[2]);
		}
	}

	return len;
}

static ssize_t duty_ctrl_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0, msg, no_ims, active, suspend;

	if ((sscanf(buf, "%d %d %d", &no_ims, &active, &suspend) != 3))
		return -EINVAL;

	if (get_md_status() != MD_LV_THROTTLE_DISABLED)
		return -EBUSY;

	if (no_ims)
		/* set active/suspend=1/255 for no IMS case */
		msg = duty_ctrl_to_tmc_msg(1, 255, 0);
	else if (active >= 1 && active <= 255
		&& suspend >= 1 && suspend <= 255)
		msg = duty_ctrl_to_tmc_msg(active, suspend, 1);
	else
		msg = TMC_THROTTLE_DISABLE_MSG;
	ret = send_throttle_msg(msg);
	if (ret)
		return -EBUSY;

	return count;
}

static ssize_t ca_ctrl_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0, ca_ctrl;

	if ((kstrtouint(buf, 10, &ca_ctrl)))
		return -EINVAL;

	if (get_md_status() != MD_LV_THROTTLE_DISABLED)
		return -EBUSY;

	ret = send_throttle_msg(ca_ctrl_to_tmc_msg(ca_ctrl));
	if (ret)
		return -EBUSY;

	return count;
}

static ssize_t pa_ctrl_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0, pa_ctrl;

	if ((kstrtouint(buf, 10, &pa_ctrl)))
		return -EINVAL;

	if (get_md_status() != MD_LV_THROTTLE_DISABLED)
		return -EBUSY;

	ret = send_throttle_msg(pa_ctrl_to_tmc_msg(pa_ctrl));
	if (ret)
		return -EBUSY;

	return count;
}

static ssize_t update_tx_pwr_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int id, tx_pwr_lv1, tx_pwr_lv2, tx_pwr_lv3;
	unsigned int tx_pwr[MAX_NUM_TX_PWR_LV];

	if ((sscanf(buf, "%d %d %d %d", &id, &tx_pwr_lv1,
		&tx_pwr_lv2, &tx_pwr_lv3) != (MAX_NUM_TX_PWR_LV + 1)))
		return -EINVAL;

	if (id >= get_pa_num()) {
		pr_err("Invalid PA id(%d)\n", id);
		return -EINVAL;
	}

	tx_pwr[0] = tx_pwr_lv1;
	tx_pwr[1] = tx_pwr_lv2;
	tx_pwr[2] = tx_pwr_lv3;

	update_throttle_power(id, tx_pwr);

	return count;
}

MD_COOLING_ATTR_RO(status);
MD_COOLING_ATTR_WO(duty_ctrl);
MD_COOLING_ATTR_WO(ca_ctrl);
MD_COOLING_ATTR_WO(pa_ctrl);
MD_COOLING_ATTR_WO(update_tx_pwr);

static struct attribute *md_cooling_attrs[] = {
	&status_attr.attr,
	&duty_ctrl_attr.attr,
	&ca_ctrl_attr.attr,
	&pa_ctrl_attr.attr,
	&update_tx_pwr_attr.attr,
	NULL
};

static struct attribute_group md_cooling_attr_group = {
	.name	= "md_cooling",
	.attrs	= md_cooling_attrs,
};

static int __init md_cooling_sysfs_init(void)
{
	int ret;

	ret = sysfs_create_group(kernel_kobj, &md_cooling_attr_group);
	if (ret)
		pr_err("failed to create md cooling sysfs, ret=%d!\n", ret);

	return ret;
}

static void __exit md_cooling_sysfs_exit(void)
{
	sysfs_remove_group(kernel_kobj, &md_cooling_attr_group);
}
module_init(md_cooling_sysfs_init);
module_exit(md_cooling_sysfs_exit);

MODULE_AUTHOR("Shun-Yao Yang <brian-sy.yang@mediatek.com>");
MODULE_DESCRIPTION("Mediatek modem cooling sysfs driver");
MODULE_LICENSE("GPL v2");
