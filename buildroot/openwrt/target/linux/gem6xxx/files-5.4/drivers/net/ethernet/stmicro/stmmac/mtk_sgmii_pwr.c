// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>

static const struct of_device_id sgmii_id_table[] = {
	{ .compatible = "mediatek,sgmii-up",},
	{ },
};
MODULE_DEVICE_TABLE(of, sgmii_id_table);

static int sgmii_probe(struct platform_device *pdev)
{
	pr_err("###########sgmii_probe sgmii_bring_up\n");
	if (!pdev->dev.pm_domain)
		return -EPROBE_DEFER;

	pr_err("###########sgmii_probe sgmii_bring_up power on\n");
	pm_runtime_enable(&pdev->dev);

	/* always enabled in lifetime */
	pm_runtime_get_sync(&pdev->dev);


	return 0;
}

static int sgmii_remove(struct platform_device *pdev)
{
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return 0;
}

int sgmii_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	if (pdev == NULL) {
		pr_info("%s pdev == NULL\n", __func__);
		return -1;
	}

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);


	pr_info("sgmii suspend_noirq done\n");

	return 0;
}

int sgmii_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	if (pdev == NULL) {
		pr_info("%s pdev == NULL\n", __func__);
		return -1;
	}
	pm_runtime_enable(&pdev->dev);

	/* always enabled in lifetime */
	pm_runtime_get_sync(&pdev->dev);

	pr_info("sgmii resume_noirq done\n");

	return 0;
}
static const struct dev_pm_ops sgmii_pm_ops = {
	.suspend_noirq = sgmii_suspend,
	.resume_noirq = sgmii_resume,
};

static struct platform_driver sgmii_up = {
	.probe		= sgmii_probe,
	.remove		= sgmii_remove,
	.driver		= {
		.name	= "sgmii-up",
		.pm = &sgmii_pm_ops,
		.owner	= THIS_MODULE,
		.of_match_table = sgmii_id_table,
	},
};
//move to builtin so as to earlier than clk-bring up
builtin_platform_driver(sgmii_up);
