// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/*
 * Author: Kurt Yang <kurt.yang@mediatek.com>
 *
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <net/mtk/mtk_frame_engine.h>
#include <net/mtk/mtk_fe_dma.h>
#include "mtk_fe_proc.h"

enum fe_clks_map {
	FE_CLK_NETSYS_SEL,
#if !defined(CONFIG_MEDIATEK_NETSYS_V2)
	FE_CLK_MEDSYS_SEL,
#endif /* !defined(CONFIG_MEDIATEK_NETSYS_V2) */
	FE_CLK_NETSYS_500M_SEL,
	FE_CLK_NETSYS_MED_MCU_SEL,
	FE_CLK_NETSYS_WED_MCU_SEL,
	FE_CLK_NETSYS_2X_SEL,
	FE_CLK_MAX
};

static const char * const fe_clks_name[] = {
	[FE_CLK_NETSYS_SEL] = "net_sel",
#if !defined(CONFIG_MEDIATEK_NETSYS_V2)
	[FE_CLK_MEDSYS_SEL] = "med_sel",
#endif /* !defined(CONFIG_MEDIATEK_NETSYS_V2) */
	[FE_CLK_NETSYS_500M_SEL] = "net_500_sel",
	[FE_CLK_NETSYS_MED_MCU_SEL] = "med_mcu_sel",
	[FE_CLK_NETSYS_WED_MCU_SEL] = "wed_mcu_sel",
	[FE_CLK_NETSYS_2X_SEL] = "net_2x_sel",
};


/*
 * struct mtk_fe_data -	This is the structure holding all differences
 *				among various plaforms
 *
 * @required_clks: Flags shown the bitmap for required clocks on
 *				the target SoC
 */
struct mtk_fe_data {
	u32		required_clks;
};

/*
 * struct mtk_fe_dev - This is the main data structure for holding the state
 *			of the driver
 * @dev:	The device pointer
 * @base:	The mapped register i/o base
 * @clks:	clock array for all clocks required
 * @soc:	Holding specific data among vaious SoCs
 * @user_token_list: the list head of current mtk_fe_token objects.
 * @ref_cnt: tracks how many users are using the frame engine. inc/dec by
 *			calling acquire_FE/release_FE. The number should be
 *			equal to the number of user_token_list.
 * @lock:	make sure the token_list operation and reference counting are
 *			atomic.
 */
struct mtk_fe_dev {
	struct device *dev;
	void __iomem *base;
	struct clk	*clks[FE_CLK_MAX];

	const struct mtk_fe_data *soc;

	struct list_head user_token_list;
	unsigned int ref_cnt;
	struct mutex lock;
};

struct mtk_fe_dev *g_fedev;


static int fe_get_users(struct list_head **usrs, unsigned int **ref_cnt)
{
	if (!g_fedev)
		return -1;

	*usrs = &g_fedev->user_token_list;
	*ref_cnt = &g_fedev->ref_cnt;
	return 0;
}

static int fe_dbg_dma_engins(char *dma, char *argv[])
{
	return mtk_fe_dma_dbg(dma, argv);
}

static struct fe_proc_operation fe_dbg_ops = {
	.get_usrs_info = fe_get_users,
	.dbg_cmds = fe_dbg_dma_engins,
};

static void fe_clk_disable(struct mtk_fe_dev *fe)
{
	int clk;

	for (clk = FE_CLK_MAX - 1; clk >= 0; clk--) {
		if (fe->soc->required_clks & BIT(clk)) {
			if (fe->clks[clk] != NULL) {
				dev_info(fe->dev, "disable clk:%s\n",
						fe_clks_name[clk]);
				clk_disable_unprepare(fe->clks[clk]);
			}
		}
	}
}

static int fe_clk_enable(struct mtk_fe_dev *fe)
{
	int clk, ret;

	for (clk = 0; clk < FE_CLK_MAX ; clk++) {
		if (fe->soc->required_clks & BIT(clk)) {
			if (fe->clks[clk] != NULL) {
				ret = clk_prepare_enable(fe->clks[clk]);
				if (ret) {
					dev_info(fe->dev, "[Error] clk_prepare_enable %s FAIL\n",
						fe_clks_name[clk]);
					goto err_disable_clks;
				}
			}
			else {
				dev_info(fe->dev, "[Error] enable clk %s FAIL\n",
						fe_clks_name[clk]);
			}
		}
	}

	return 0;

err_disable_clks:
	while (--clk >= 0)
		clk_disable_unprepare(fe->clks[clk]);

	return ret;
}

int acquire_FE(char *user, struct mtk_fe_token **fe)
{
	if (!g_fedev) {
		pr_info("%s: frameEngine is not init yet, cant acquired by %s\n",
			__func__, user);
		return -1;
	}

	mutex_lock(&g_fedev->lock);

	*fe = devm_kzalloc(g_fedev->dev, sizeof(**fe), GFP_KERNEL);
	strncpy((*fe)->user, user, sizeof((*fe)->user)-1);
	(*fe)->dev = g_fedev->dev;
	(*fe)->priv = g_fedev;
	list_add_tail(&(*fe)->list, &g_fedev->user_token_list);
#if 0
	if (!g_fedev->ref_cnt) {
		pm_runtime_enable(g_fedev->dev);
		pm_runtime_get_sync(g_fedev->dev);
		fe_clk_enable(g_fedev);
	}
#endif
	g_fedev->ref_cnt++;
	mutex_unlock(&g_fedev->lock);
	dev_info(g_fedev->dev, "%s by %s, ref_cnt = %d\n", __func__, user, g_fedev->ref_cnt);
	return 0;
}
EXPORT_SYMBOL(acquire_FE);

int release_FE(struct mtk_fe_token *fe)
{
	struct mtk_fe_dev *fedev = (struct mtk_fe_dev *)fe->priv;

	dev_info(fedev->dev, "%s by %s, ref_cnt = %d\n", __func__, fe->user, fedev->ref_cnt);
	mutex_lock(&fedev->lock);
	list_del(&fe->list);
	devm_kfree(fedev->dev, fe);

	fedev->ref_cnt--;
#if 0
	if (!fedev->ref_cnt) {
		fe_clk_disable(fedev);
		pm_runtime_put_sync(fedev->dev);
		pm_runtime_disable(fedev->dev);
	}
#endif
	mutex_unlock(&fedev->lock);
	return 0;
}
EXPORT_SYMBOL(release_FE);

int mtk_fe_pm_clk(struct mtk_fe_token *fe, bool suspend)
{
	struct mtk_fe_dev *fedev = (struct mtk_fe_dev *)fe->priv;

	dev_info(fedev->dev, "%s by %s\n", __func__, fe->user);

	if (suspend)
		fe_clk_disable(fedev);
	else
		fe_clk_enable(fedev);

	return 0;
}

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
static const struct mtk_fe_data mt6990_data = {
        .required_clks = (BIT(FE_CLK_NETSYS_SEL) |
                                        BIT(FE_CLK_NETSYS_500M_SEL) |
                                        BIT(FE_CLK_NETSYS_MED_MCU_SEL) |
                                        BIT(FE_CLK_NETSYS_WED_MCU_SEL) |
                                        BIT(FE_CLK_NETSYS_2X_SEL)),
};
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
static const struct mtk_fe_data mt6890_data = {
	.required_clks = (BIT(FE_CLK_NETSYS_SEL) |
					BIT(FE_CLK_MEDSYS_SEL) |
					BIT(FE_CLK_NETSYS_500M_SEL) |
					BIT(FE_CLK_NETSYS_MED_MCU_SEL) |
					BIT(FE_CLK_NETSYS_WED_MCU_SEL) |
					BIT(FE_CLK_NETSYS_2X_SEL)),
};
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */

static const struct mtk_fe_data fpga_data = {
	.required_clks = 0,
};


const struct of_device_id of_mtk_fe_match[] = {
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
        {
                .compatible = "mediatek,mt6990-netsys",
                .data = (void *)&mt6990_data
        },
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	{
		.compatible = "mediatek,mt6890-netsys",
		.data = (void *)&mt6890_data
	},
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	{},
};

MODULE_DEVICE_TABLE(of, of_mtk_fe_match);

static int mtk_fe_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *base;
	int i, ret;
	struct mtk_fe_data *fe_dev_data =
			(struct mtk_fe_data *)of_device_get_match_data(dev);

	pr_notice("%s\n", __func__);

	g_fedev = devm_kzalloc(dev, sizeof(*g_fedev), GFP_KERNEL);
	if (!g_fedev)
		return -ENOMEM;

	// Currently, child driver (eth) should run before fe driver.
	of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	/* Workaround it first... Long term we should separate different
	 * memory region for netsys/fe and netsys/eth, to avoid
	 * the conflict issue.
	 */
	//base = devm_ioremap_resource(dev, res);
	base = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(base)) {
		dev_err(dev, "failed to get resource\n");
		return PTR_ERR(base);
	}

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
		return ret;
	}

	ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(33));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_mask: %d\n", ret);
		return ret;
	}
#endif

	for (i = 0; i < ARRAY_SIZE(g_fedev->clks); i++) {
		g_fedev->clks[i] = NULL;
		if (fe_dev_data->required_clks & BIT(i)) {
			g_fedev->clks[i] = devm_clk_get(dev, fe_clks_name[i]);
			if (IS_ERR(g_fedev->clks[i])) {
				dev_err(dev, "clock %s not found\n",
						fe_clks_name[i]);
				return -EINVAL;
			}
		}
	}
	g_fedev->soc = fe_dev_data;
	g_fedev->base = base;
	g_fedev->dev = dev;
	mutex_init(&g_fedev->lock);
	INIT_LIST_HEAD(&g_fedev->user_token_list);

	platform_set_drvdata(pdev, g_fedev->base);
	ret = mtk_fe_dma_probe(pdev);
	if (ret)
		return ret;

	ret = fe_proc_init(&fe_dbg_ops);
	if (ret)
		return ret;

	/* enable power and clock */
	pm_runtime_enable(g_fedev->dev);
	pm_runtime_get_sync(g_fedev->dev);
	fe_clk_enable(g_fedev);

	return 0;
}

static int mtk_fe_remove(struct platform_device *pdev)
{
	pr_notice("%s\n", __func__);
	/* disable clock and power */
	fe_clk_disable(g_fedev);
	pm_runtime_put_sync(g_fedev->dev);
	pm_runtime_disable(g_fedev->dev);

	fe_proc_exit();
	return 0;
}

int fe_pm_suspend(struct device *device)
{
	pr_notice("%s\n", __func__);
	/* suspend */
	//mtk_fe_dma_pm_handle(1);
	fe_clk_disable(g_fedev);
	return 0;
}

int fe_pm_resume(struct device *device)
{
        pr_notice("%s\n", __func__);
        /* resume */
        //mtk_fe_dma_pm_handle(0);
	fe_clk_enable(g_fedev);
        return 0;
}


static const struct dev_pm_ops fe_pm_ops = {
	.suspend_noirq = fe_pm_suspend,
	.resume_noirq = fe_pm_resume,
};

static struct platform_driver fe_driver = {
	.probe = mtk_fe_probe,
	.remove = mtk_fe_remove,
	.driver = {
		.name = "mtk_soc_netsys",
		.pm = &fe_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = of_mtk_fe_match,
	},
};

module_platform_driver(fe_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kurt Yang <kurt.yang@mediatek.com.tw>");
MODULE_DESCRIPTION("Frame Engine driver for MediaTek SoC");
