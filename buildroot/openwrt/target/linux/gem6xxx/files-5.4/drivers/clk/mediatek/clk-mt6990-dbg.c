// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.
// Author: Pei-hsuan Cheng <pei-hsuan.cheng@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt6990-clk.h>

#define MT_CCF_BRINGUP		0

/* Regular Number Definition */
#define INV_OFS			-1
#define INV_BIT			-1

static const struct mtk_gate_regs dbgao_cg_regs = {
	.set_ofs = 0xA070,
	.clr_ofs = 0xA070,
	.sta_ofs = 0xA070,
};

#define GATE_DBGAO(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dbgao_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate dbgao_clks[] = {
	GATE_DBGAO(CLK_DBGAO_ATB_EN, "dbgao_atb_en",
			"atb_ck"/* parent */, 0),
};

//static const struct mtk_clk_desc dbgao_mcd = {
//	.clks = dbgao_clks,
//	.num_clks = CLK_DBGAO_NR_CLK,
//};

static const struct mtk_gate_regs dem0_cg_regs = {
	.set_ofs = 0x2C,
	.clr_ofs = 0x2C,
	.sta_ofs = 0x2C,
};

static const struct mtk_gate_regs dem1_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

static const struct mtk_gate_regs dem2_cg_regs = {
	.set_ofs = 0x70,
	.clr_ofs = 0x70,
	.sta_ofs = 0x70,
};

#define GATE_DEM0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dem0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_DEM1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dem1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_DEM2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dem2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate dem_clks[] = {
	/* DEM0 */
	GATE_DEM0(CLK_DEM_BUSCLK_EN, "dem_busclk_en",
			"axi_ck"/* parent */, 0),
	/* DEM1 */
	GATE_DEM1(CLK_DEM_SYSCLK_EN, "dem_sysclk_en",
			"axi_ck"/* parent */, 0),
	/* DEM2 */
	GATE_DEM2(CLK_DEM_ATB_EN, "dem_atb_en",
			"axi_ck"/* parent */, 0),
};

//static const struct mtk_clk_desc dem_mcd = {
//	.clks = dem_clks,
//	.num_clks = CLK_DEM_NR_CLK,
//};

static int clk_mt6990_dem_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_DEM_NR_CLK);

	mtk_clk_register_gates(node, dem_clks, ARRAY_SIZE(dem_clks),
			clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

//	r = mtk_clk_simple_probe(pdev);
	if (r)
		dev_err(&pdev->dev,
			"could not register clock provider: %s: %d\n",
			pdev->name, r);

#if MT_CCF_BRINGUP
	pr_notice("%s: %s init end\n", __func__, pdev->name);
#endif

	return r;
}

static int clk_mt6990_dbgao_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_DBGAO_NR_CLK);

	mtk_clk_register_gates(node, dbgao_clks, ARRAY_SIZE(dbgao_clks),
			clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

//	r = mtk_clk_simple_probe(pdev);
	if (r)
		dev_err(&pdev->dev,
			"could not register clock provider: %s: %d\n",
			pdev->name, r);

#if MT_CCF_BRINGUP
	pr_notice("%s: %s init end\n", __func__, pdev->name);
#endif

	return r;
}

static int clk_mt6990_dbg_grp_probe(struct platform_device *pdev)
{
	int (*clk_probe)(struct platform_device *pd);
	int r;

	clk_probe = of_device_get_match_data(&pdev->dev);
	if (!clk_probe)
		return -EINVAL;

	r = clk_probe(pdev);
	if (r)
		dev_err(&pdev->dev,
			"could not register clock provider: %s: %d\n",
			pdev->name, r);

	return r;
}

static const struct of_device_id of_match_clk_mt6990_dbg[] = {
	{
		.compatible = "mediatek,mt6990-dbgao",
		.data = clk_mt6990_dbgao_probe,
	}, {
		.compatible = "mediatek,mt6990-dem",
		.data = clk_mt6990_dem_probe,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt6990_dbg_drv = {
	.probe = clk_mt6990_dbg_grp_probe,
	.driver = {
		.name = "clk-mt6990-dbg",
		.of_match_table = of_match_clk_mt6990_dbg,
	},
};

static int __init clk_mt6990_dbg_init(void)
{
	return platform_driver_register(&clk_mt6990_dbg_drv);
}

static void __exit clk_mt6990_dbg_exit(void)
{
	platform_driver_unregister(&clk_mt6990_dbg_drv);
}

arch_initcall(clk_mt6990_dbg_init);
module_exit(clk_mt6990_dbg_exit);
MODULE_LICENSE("GPL");
