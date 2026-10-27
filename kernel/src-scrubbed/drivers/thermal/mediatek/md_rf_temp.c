// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/wait.h>
#include <mtk_ccci_common.h>

#define WAIT_MD_FEEDBACK_TIMEOUT_MS	(20)
#define RF_TEMP_SIGN_BIT		(7)

enum md_rf_thread_state {
	MD_RF_NOT_STARTED = 0,
	MD_RF_WAITING_RESPONSE,
	MD_RF_RECEIVED_RESPONSE,
};

struct md_rf_info {
	struct device *dev;
	enum md_rf_thread_state state;
	int cur_temp;
	wait_queue_head_t md_rf_wq;
};

/* Use global variable because we need to use it in md_rf_handler */
static struct md_rf_info *rf_info;

static int md_rf_get_temp(void *no_used, int *temp)
{
	int ret;

	rf_info->state = MD_RF_WAITING_RESPONSE;
	ret = exec_ccci_kern_func_by_md_id(MD_SYS1,
			MD_RF_MAX_TEMPERATURE_SUB6, NULL, 0);
	if (ret) {
		if (ret == -CCCI_ERR_MD_NOT_READY) {
			dev_dbg(rf_info->dev,
				"MD is not ready yet, retry next time!\n");
			return -EAGAIN;
		} else {
			dev_err(rf_info->dev,
				"send msg to MD fail, ret=%d\n", ret);
			return -EINVAL;
		}
	}

	dev_dbg(rf_info->dev, "send request done, waiting ack event\n");
	ret = wait_event_timeout(rf_info->md_rf_wq,
		rf_info->state == MD_RF_RECEIVED_RESPONSE,
		msecs_to_jiffies(WAIT_MD_FEEDBACK_TIMEOUT_MS));
	if (!ret) {
		dev_dbg(rf_info->dev,
			"wait event timeout, state=%d, ret=%d\n",
			rf_info->state, ret);
		*temp = THERMAL_TEMP_INVALID;
	} else {
		*temp = rf_info->cur_temp;
	}

	dev_dbg(rf_info->dev, "RF temp = %d\n", *temp);

	return 0;
}

static const struct thermal_zone_of_device_ops md_rf_ops = {
	.get_temp = md_rf_get_temp,
};

static int md_msg_handler(int md_id, int data)
{
	unsigned int rat, temp;

	rat = (data >> 8) & 0xFF;
	temp = data & 0xFF;

	rf_info->cur_temp =
		sign_extend32(temp, RF_TEMP_SIGN_BIT) * 1000;
	rf_info->state = MD_RF_RECEIVED_RESPONSE;

	dev_dbg(rf_info->dev, "rat=%d, temp=%d\n", rat, temp);

	wake_up(&rf_info->md_rf_wq);

	return 0;
}

static int md_rf_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *tz_dev;
	int ret;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "Only DT based supported\n");
		return -ENODEV;
	}

	rf_info = devm_kzalloc(&pdev->dev, sizeof(*rf_info), GFP_KERNEL);
	if (!rf_info)
		return -ENOMEM;

	rf_info->dev = &pdev->dev;
	rf_info->state = MD_RF_NOT_STARTED;
	init_waitqueue_head(&rf_info->md_rf_wq);
	ret = register_ccci_sys_call_back(MD_SYS1, MD_RF_MAX_TEMPERATURE_SUB6,
				md_msg_handler);
	if (ret) {
		dev_err(&pdev->dev, "register CB to CCCI failed, ret = %d\n",
			ret);
		return -EINVAL;
	}

	tz_dev = devm_thermal_zone_of_sensor_register(
			&pdev->dev, 0, NULL, &md_rf_ops);
	if (IS_ERR(tz_dev)) {
		dev_err(&pdev->dev, "MD RF TZ sensor register fail!\n");
		return PTR_ERR(tz_dev);
	}

	return 0;
}

static const struct of_device_id md_rf_of_match[] = {
	{ .compatible = "mediatek,md-rf", },
	{},
};
MODULE_DEVICE_TABLE(of, md_rf_of_match);

static struct platform_driver md_rf_driver = {
	.probe = md_rf_probe,
	.driver = {
		.name = "mtk-md-rf",
		.of_match_table = md_rf_of_match,
	},
};

module_platform_driver(md_rf_driver);

MODULE_AUTHOR("Shun-Yao Yang <brian-sy.yang@mediatek.com>");
MODULE_DESCRIPTION("Mediatek modem RF thermal driver");
MODULE_LICENSE("GPL v2");
