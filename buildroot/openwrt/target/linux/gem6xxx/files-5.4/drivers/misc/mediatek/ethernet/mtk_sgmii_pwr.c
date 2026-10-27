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

#include "mtk_sgmii_pwr.h"


static const struct of_device_id sgmii_id_table[] = {
	{ .compatible = "mediatek,sgmii-bring-up",},
	{ },
};
MODULE_DEVICE_TABLE(of, sgmii_id_table);


static struct mtk_gmii *mtk_gmii_glb[2][3];



int sgmii_runtime_control(int id, bool enable)
{
	u32 i = 0;
	int ret = 0;

	pr_info("%s in %d %d\n", __func__, id, enable);

	if (id >= 2 || id < 0) {
		pr_info("%s: wrong id %d\n", __func__,id);
		return -1;
	}
	if (mtk_gmii_glb[id][1] == NULL &&
		mtk_gmii_glb[id][2] == NULL) {
		pr_info("%s: mtk_gmii_glb == NULL %d\n",  __func__, id);
		return -1;
	}
	//Note: Skip type= 0 (PHY)
	for (i = 2; i > 0; i-- ) {
		if (mtk_gmii_glb[id][i] == NULL)
			continue;

		if (mtk_gmii_glb[id][i]->enable == enable) {
			pr_info("%s: Warning %d %d the same %d\n", __func__, id, i, enable);
			ret++;
			continue;
		}

		mtk_gmii_glb[id][i]->enable = enable;

		if (mtk_gmii_glb[id][i]->enable) {
			pr_info("%s enable %d %d\n", __func__, id, i);
			//pm_runtime_enable(mtk_gmii_glb[id][i]->dev);
			pm_runtime_get_sync(mtk_gmii_glb[id][i]->dev);
		} else {
			pr_info("%s disable %d %d\n", __func__, id, i);
			pm_runtime_put_sync(mtk_gmii_glb[id][i]->dev);
			//pm_runtime_disable(mtk_gmii_glb[id][i]->dev);
		}
	}
	return ret;
}

static int sgmii_probe(struct platform_device *pdev)
{

	int err;
	u32 id = 0;
	u32 type = 0;
	struct mtk_gmii *gmii;

	if (!pdev->dev.pm_domain)
		return -EPROBE_DEFER;

	err = of_property_read_u32(pdev->dev.of_node, "mediatek,pwr_type", &type);
	if (err) {
		pr_info("%s: can not find mediatek,pwr_type\n",	__func__);
		return err;
	}

	err =  of_property_read_u32(pdev->dev.of_node, "mediatek,pwr_id" , &id);
	if (err) {
		pr_info("%s: can not find mediatek,pwr_id\n", __func__);
		return err;
	}
	pr_info("%s: sgmii_probe type %d id %d\n", __func__, type, id);


	gmii = devm_kzalloc(&pdev->dev, sizeof(*gmii), GFP_KERNEL);
	if (!gmii) {
		pr_info("%s: gmii failed\n", __func__);
		return -ENOMEM;
	}

	gmii->dev = &pdev->dev;
	gmii->id = id;
	gmii->type = type;

	if (gmii->id < 0) {
		pr_info("%s: Error mediatek,pwr_id id %d\n", __func__, id);
		return -1;
	}
	if (gmii->type < 0) {
		pr_info("%s: Error mediatek,pwr_id type %d\n", __func__, type);
		return -1;
	}

	if (type == 2)
		gmii->enable = false;
	else
		gmii->enable = true;

	mtk_gmii_glb[gmii->id][gmii->type] = gmii;

	platform_set_drvdata(pdev, gmii);

	pm_runtime_enable(&pdev->dev);

	/* 10G USXGMII AN mode default disable, 2.5G Force mode default enable*/
	if (type != 2)
		pm_runtime_get_sync(&pdev->dev);


	return 0;
}

static int sgmii_remove(struct platform_device *pdev)
{
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

 	return 0;
}

int sgmii_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct mtk_gmii *gmii;

	if (pdev == NULL) {
		pr_info("%s pdev == NULL\n", __func__);
		return -1;
	}

	gmii = platform_get_drvdata(pdev);
	if (gmii->enable == false) {
		pr_info("%s_noirq [%d][%d] already disable\n", __func__,
			gmii->id, gmii->type);
		return 0;
	}

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	pr_info(" %s noirq [%d][%d] done\n",  __func__,
		gmii->id, gmii->type);

	return 0;
}

int sgmii_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct mtk_gmii *gmii;

	if (pdev == NULL) {
		pr_info("%s pdev == NULL\n", __func__);
		return -1;
	}

	gmii = platform_get_drvdata(pdev);
	if (gmii->enable == false) {
		pr_info("%s noirq [%d][%d] already disable\n",  __func__,
			gmii->id, gmii->type);
		return 0;
	}

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	pr_info(" %s noirq done\n",  __func__);

	pr_info(" %snoirq [%d][%d] done\n",  __func__,
		gmii->id, gmii->type);

	return 0;
}
static const struct dev_pm_ops sgmii_pm_ops = {
	.suspend_noirq = sgmii_pm_suspend,
	.resume_noirq = sgmii_pm_resume,
};

static struct platform_driver sgmii_bring_up = {
	.probe		= sgmii_probe,
	.remove		= sgmii_remove,
	.driver		= {
		.name	= "sgmii_bring_up",
		.pm = &sgmii_pm_ops,
		.owner	= THIS_MODULE,
		.of_match_table = sgmii_id_table,
	},
};
//move to builtin so as to earlier than clk-bring up
builtin_platform_driver(sgmii_bring_up);
