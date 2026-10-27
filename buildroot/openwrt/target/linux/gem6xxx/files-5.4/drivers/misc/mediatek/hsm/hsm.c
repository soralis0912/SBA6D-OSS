// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>

static void hsm_clock_on(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_HSM_CLOCK_ON,
		0, 0, 0, 0, 0, 0, 0, &res);
}

static void hsm_clock_off(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_HSM_CLOCK_OFF,
		0, 0, 0, 0, 0, 0, 0, &res);
}

static void hsm_reset(void)
{
}

static ssize_t hsm_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	pr_info("%s with buf:%s\n", __func__, buf);
	return count;
}

static ssize_t hsm_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "MTK HSM\n");
}

static DEVICE_ATTR_RW(hsm);

static int hsm_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = device_create_file(&pdev->dev, &dev_attr_hsm);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
	hsm_clock_on();
	pr_info("%s with ret:%d\n", __func__, ret);
	return ret;
}

static int hsm_remove(struct platform_device *pdev)
{
	int ret = 0;

	device_remove_file(&pdev->dev, &dev_attr_hsm);
	hsm_clock_off();
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return ret;
}

static const struct of_device_id mtk_hsm_match[] = {
	{ .compatible = "mediatek,hsm-common", },
};
MODULE_DEVICE_TABLE(of, mtk_hsm_match);

static int hsm_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	hsm_clock_off();
	return 0;
}

static int hsm_resume(struct platform_device *pdev)
{
	hsm_reset();
	hsm_clock_on();
	return 0;
}

static struct platform_driver hsm_common_driver = {
	.probe = hsm_probe,
	.remove = hsm_remove,
	.driver = {
		.name = "hsm-common",
		.of_match_table = of_match_ptr(mtk_hsm_match),
	},
	.suspend = hsm_suspend,
	.resume  = hsm_resume,
};

/* platform device */
static struct platform_device hsm_common_device = {
	.name = "hsm-common",
};

static int __init hsm_init(void)
{
	int err;

	err = platform_device_register(&hsm_common_device);
	if (err)
		return err;

	err = platform_driver_register(&hsm_common_driver);
	if (err)
		platform_device_unregister(&hsm_common_device);

	return err;
}

static void __exit hsm_exit(void)
{
	platform_driver_unregister(&hsm_common_driver);
	platform_device_unregister(&hsm_common_device);
}
module_init(hsm_init);
module_exit(hsm_exit);

