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

static const struct mtk_gate_regs gce_cg_regs = {
	.set_ofs = 0xF0,
	.clr_ofs = 0xF0,
	.sta_ofs = 0xF0,
};

#define GATE_GCE(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &gce_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate gce_clks[] = {
	GATE_GCE(CLK_GCE_0, "gce_0",
			"axi_ck"/* parent */, 16),
};

//static const struct mtk_clk_desc gce_mcd = {
//	.clks = gce_clks,
//	.num_clks = CLK_GCE_NR_CLK,
//};

static int clk_mt6990_gce_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_GCE_NR_CLK);

	mtk_clk_register_gates(node, gce_clks, ARRAY_SIZE(gce_clks),
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

static const struct of_device_id of_match_clk_mt6990_gce[] = {
	{
		.compatible = "mediatek,mt6990-gce",
//		.data = &gce_mcd,
	},
	{}
};

static struct platform_driver clk_mt6990_gce_drv = {
	.probe = clk_mt6990_gce_probe,
	.driver = {
		.name = "clk-mt6990-gce",
		.of_match_table = of_match_clk_mt6990_gce,
	},
};

static int __init clk_mt6990_gce_init(void)
{
	return platform_driver_register(&clk_mt6990_gce_drv);
}

static void __exit clk_mt6990_gce_exit(void)
{
	platform_driver_unregister(&clk_mt6990_gce_drv);
}

arch_initcall(clk_mt6990_gce_init);
module_exit(clk_mt6990_gce_exit);
MODULE_LICENSE("GPL");
