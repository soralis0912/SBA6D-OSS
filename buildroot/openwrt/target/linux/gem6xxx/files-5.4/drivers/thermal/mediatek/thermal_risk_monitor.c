// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/thermal.h>
#include <linux/pm_wakeup.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include "thermal_risk_monitor.h"
#include "mtk_thermal_ipi.h"

/*==================================================
 * Global variable
 *==================================================
 */
static struct trm_data g_trm_data;
static struct task_struct *thermal_awake_task;

/*==================================================
 * Platform data
 *==================================================
 */
static char mt6880_id_to_tz_map[NUM_THERMAL_SENSOR][THERMAL_NAME_LENGTH] = {
	"soc_max",
	"cpu_little2",
	"",
	"",
	"gpu1",
	"md_4g",
	"soc_dram_ntc",
	"ltepa_ntc",
	"nrpa_ntc",
	"rf_ntc",
};

/*==================================================
 * Local function
 *==================================================
 */
static int trm_update_policy(enum thermal_sensor id, int threshold, int hysteresis)
{
	struct trm_sensor_data *sen_data;

	mutex_lock(&g_trm_data.sen_list_lock);
	list_for_each_entry(sen_data, &g_trm_data.sen_list, node) {
		if (sen_data->id == id) {
			sen_data->threshold = threshold;
			sen_data->hysteresis = hysteresis;
			mutex_unlock(&g_trm_data.sen_list_lock);
			return 0;
		}
	}
	mutex_unlock(&g_trm_data.sen_list_lock);

	return -1;
}

static int trm_add_policy(enum thermal_sensor id, int threshold, int hysteresis)
{
	struct trm_sensor_data *sen_data;

	sen_data = devm_kzalloc(g_trm_data.dev, sizeof(*sen_data), GFP_KERNEL);
	if (!sen_data) {
		dev_err(g_trm_data.dev,
			"Failed to allocate memory, sensor id %u\n", id);
		return -ENOMEM;
	}

	sen_data->id = id;
	sen_data->threshold = threshold;
	sen_data->hysteresis = hysteresis;

	mutex_lock(&g_trm_data.sen_list_lock);
	list_add(&sen_data->node, &g_trm_data.sen_list);
	mutex_unlock(&g_trm_data.sen_list_lock);

	return 0;
}

static ssize_t trm_policy_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret, threshold, hysteresis;
	char cmd[THERMAL_NAME_LENGTH];
	unsigned int i, id, len;
	struct thermal_ipi_data thermal_data;

	if (sscanf(buf, "%19s %d %d", cmd, &threshold, &hysteresis) == 3) {
		len = strlen(cmd);
		id = NUM_THERMAL_SENSOR;

		for (i = 0; i < NUM_THERMAL_SENSOR; i++) {
			if (!strncmp(cmd, g_trm_data.id_to_tz_map +
				i * THERMAL_NAME_LENGTH, len)) {
				id = i;
				break;
			}
		}

		if (id == NUM_THERMAL_SENSOR) {
			dev_info(g_trm_data.dev, "Not support this tz %s\n",
				cmd);
			return -EINVAL;
		}

		ret = trm_update_policy(id, threshold, hysteresis);
		if (ret == -1) {
			ret = trm_add_policy(id, threshold, hysteresis);
			if (ret)
				return ret;
		}

		thermal_data.u.data.arg[0] = id;
		thermal_data.u.data.arg[1] = threshold;
		thermal_data.u.data.arg[2] = hysteresis;

		while (thermal_to_sspm(TRM_IPI_INIT_GRP1, &thermal_data) !=
			IPI_SUCCESS)
			udelay(100);
	} else if (sscanf(buf, "%19s", cmd) == 1) {
		if (strncmp(cmd, "disabled", 8))
			return -EINVAL;

		thermal_data.u.data.arg[0] = 0;
		thermal_data.u.data.arg[1] = 0;
		thermal_data.u.data.arg[2] = 0;

		while (thermal_to_sspm(TRM_IPI_DISABLE_CMD, &thermal_data) !=
			IPI_SUCCESS)
			udelay(100);
	} else {
		return -EINVAL;
	}

	return count;
}

TRM_ATTR_WO(trm_policy);

static struct attribute *trm_attrs[] = {
	&trm_policy_attr.attr,
	NULL
};

static struct attribute_group trm_attr_group = {
	.name	= "trm",
	.attrs	= trm_attrs,
};

static char * get_tz_name_by_id(enum thermal_sensor id)
{
	if (id >= NUM_THERMAL_SENSOR)
		return NULL;

	return g_trm_data.id_to_tz_map + id * THERMAL_NAME_LENGTH;
}

static int get_temp(enum thermal_sensor id, int *temp)
{
	struct thermal_zone_device *tzd;

	tzd = thermal_zone_get_zone_by_name(get_tz_name_by_id(id));
	if (IS_ERR(tzd))
		return PTR_ERR(tzd);

	return thermal_zone_get_temp(tzd, temp);
}

static int check_thermal_risk(void)
{
	struct trm_sensor_data *sen_data;
	int ret, temp = 0;

	mutex_lock(&g_trm_data.sen_list_lock);
	list_for_each_entry(sen_data, &g_trm_data.sen_list, node) {
		ret = get_temp(sen_data->id, &temp);

		if (ret) {
			dev_info(g_trm_data.dev,
				"Error: Failed to get temp of tz %s, %d\n",
				get_tz_name_by_id(sen_data->id), ret);
			continue;
		}

		if (temp >= (sen_data->threshold - sen_data->hysteresis)) {
			mutex_unlock(&g_trm_data.sen_list_lock);
			return 1;
		}
	}
	mutex_unlock(&g_trm_data.sen_list_lock);

	return 0;
}

static int keep_ap_awake_thread(void *data)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();

	while (!kthread_should_stop()) {
		pm_stay_awake(g_trm_data.dev);
		while (1) {
			if (!check_thermal_risk())
				break;

			msleep(1000);
		}

		pm_relax(g_trm_data.dev);
		dev_info(g_trm_data.dev, "The thermal risk averted\n");
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static int trm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	mutex_init(&g_trm_data.sen_list_lock);
	INIT_LIST_HEAD(&g_trm_data.sen_list);

	g_trm_data.id_to_tz_map = (char *) of_device_get_match_data(dev);

	if (!g_trm_data.id_to_tz_map) {
		dev_err(dev, "Error: Failed to get trm platform data\n");
		return -ENODATA;
	}

	g_trm_data.dev = &pdev->dev;
	platform_set_drvdata(pdev, &g_trm_data);

	ret = sysfs_create_group(kernel_kobj, &trm_attr_group);
	if (ret) {
		dev_err(dev, "Failed to create TRM sysfs, ret=%d!\n", ret);
		return ret;
	}

	g_trm_data.dev->power.wakeup = wakeup_source_register(NULL, "trm");

	thermal_awake_task = kthread_run(keep_ap_awake_thread, NULL, "sspm_to_thermal");
	if (IS_ERR(thermal_awake_task)) {
		ret = PTR_ERR(thermal_awake_task);
		thermal_awake_task = NULL;
		dev_err(dev, "Fail to create a thread for thermal awake task, %d\n",
			ret);

		return ret;
	}

	return 0;
}

static int trm_remove(struct platform_device *pdev)
{
	sysfs_remove_group(kernel_kobj, &trm_attr_group);
	wakeup_source_unregister(g_trm_data.dev->power.wakeup);

	return 0;
}
/*==================================================
 * Extern function
 *==================================================
 */
void keep_ap_wake(void)
{
	if (thermal_awake_task != NULL) {
		dev_info(g_trm_data.dev, "Wake AP up for a thermal risk\n");
		wake_up_process(thermal_awake_task);
	}
}
/*==================================================
 * Support chips
 *==================================================
 */
static const struct of_device_id trm_of_match[] = {
	{
		.compatible = "mediatek,mt6880-trm",
		.data = (void *)mt6880_id_to_tz_map,
	},
	{
	},
};
MODULE_DEVICE_TABLE(of, trm_of_match);
/*==================================================*/
static struct platform_driver mtk_trm = {
	.probe = trm_probe,
	.remove = trm_remove,
	.driver = {
		.name = "mtk-trm",
		.of_match_table = trm_of_match,
	},
};

module_platform_driver(mtk_trm);
MODULE_AUTHOR("Yu-Chia Chang <ethan.chang@mediatek.com>");
MODULE_DESCRIPTION("Mediatek thermal risk monitor driver");
MODULE_LICENSE("GPL v2");
