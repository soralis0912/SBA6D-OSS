// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.
// Author: Pei-hsuan Cheng <pei-hsuan.cheng@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt6990-clk.h>

#define MT_CCF_BRINGUP		0

/* Regular Number Definition */
#define INV_OFS			-1
#define INV_BIT			-1

static const struct mtk_gate_regs mfg_top_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_MFG_TOP(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mfg_top_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mfg_top_clks[] = {
	GATE_MFG_TOP(CLK_MFG_TOP_BG3D, "mfg_bg3d",
			"mfg_ck"/* parent */, 0),
};

//static const struct mtk_clk_desc mfg_top_mcd = {
//	.clks = mfg_top_clks,
//	.num_clks = CLK_MFG_TOP_NR_CLK,
//};

static int clk_mt6990_mfg_top_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_MFG_TOP_NR_CLK);

	mtk_clk_register_gates(node, mfg_top_clks, ARRAY_SIZE(mfg_top_clks),
			clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

//	r = mtk_clk_simple_probe(pdev);
	if (r)
		dev_err(&pdev->dev,
			"could not register clock provider: %s: %d\n",
			pdev->name, r);

#if MT_CCF_BRINGUP
	pr_notice("%s init end\n", __func__);
#endif

	return r;
}

static const struct of_device_id of_match_clk_mt6990_mfg_top[] = {
	{
		.compatible = "mediatek,mt6990-mfg_top",
//		.data = &mfg_top_mcd,
	},
	{}
};

static struct platform_driver clk_mt6990_mfg_top_drv = {
	.probe = clk_mt6990_mfg_top_probe,
	.driver = {
		.name = "clk-mt6990-mfg_top",
		.of_match_table = of_match_clk_mt6990_mfg_top,
	},
};

static int __init clk_mt6990_mfg_top_init(void)
{
	return platform_driver_register(&clk_mt6990_mfg_top_drv);
}

static void __exit clk_mt6990_mfg_top_exit(void)
{
	platform_driver_unregister(&clk_mt6990_mfg_top_drv);
}

arch_initcall(clk_mt6990_mfg_top_init);
module_exit(clk_mt6990_mfg_top_exit);
MODULE_LICENSE("GPL");
