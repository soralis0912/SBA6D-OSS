// SPDX-License-Identifier: GPL-2.0

/*

 * Copyright (c) 2019 MediaTek Inc.

 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt6880-clk.h>

#define MT_CLKMGR_MODULE_INIT	0

#define MT_CCF_BRINGUP		1

#define INV_OFS			-1



static const struct mtk_gate_regs dbgsys_dem0_cg_regs = {
	.set_ofs = 0x2c,
	.clr_ofs = 0x2c,
	.sta_ofs = 0x2c,
};

static const struct mtk_gate_regs dbgsys_dem1_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

static const struct mtk_gate_regs dbgsys_dem2_cg_regs = {
	.set_ofs = 0x70,
	.clr_ofs = 0x70,
	.sta_ofs = 0x70,
};

#define GATE_DBGSYS_DEM0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dbgsys_dem0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_DBGSYS_DEM1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dbgsys_dem1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_DBGSYS_DEM2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &dbgsys_dem2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate dbgsys_dem_clks[] = {
	/* DBGSYS_DEM0 */
	GATE_DBGSYS_DEM0(CLK_DBGSYS_DEM_BUSCLK_EN, "dbgsys_dem_busclk_en",
			"axi_ck"/* parent */, 0),
	/* DBGSYS_DEM1 */
	GATE_DBGSYS_DEM1(CLK_DBGSYS_DEM_SYSCLK_EN, "dbgsys_dem_sysclk_en",
			"axi_ck"/* parent */, 0),
	/* DBGSYS_DEM2 */
	GATE_DBGSYS_DEM2(CLK_DBGSYS_DEM_ATB_EN, "dbgsys_dem_atb_en",
			"axi_ck"/* parent */, 0),
};

static int clk_mt6880_dbgsys_dem_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_DBGSYS_DEM_NR_CLK);

	mtk_clk_register_gates(node, dbgsys_dem_clks, ARRAY_SIZE(dbgsys_dem_clks),
			clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

#if MT_CCF_BRINGUP
	pr_notice("%s init end\n", __func__);
#endif

	return r;
}

static const struct of_device_id of_match_clk_mt6880_dbgsys_dem[] = {
	{ .compatible = "mediatek,mt6880-dbgsys_dem", },
	{}
};

#if MT_CLKMGR_MODULE_INIT

static struct platform_driver clk_mt6880_dbgsys_dem_drv = {
	.probe = clk_mt6880_dbgsys_dem_probe,
	.driver = {
		.name = "clk-mt6880-dbgsys_dem",
		.of_match_table = of_match_clk_mt6880_dbgsys_dem,
	},
};

builtin_platform_driver(clk_mt6880_dbgsys_dem_drv);

#else

static struct platform_driver clk_mt6880_dbgsys_dem_drv = {
	.probe = clk_mt6880_dbgsys_dem_probe,
	.driver = {
		.name = "clk-mt6880-dbgsys_dem",
		.of_match_table = of_match_clk_mt6880_dbgsys_dem,
	},
};
static int __init clk_mt6880_dbgsys_dem_platform_init(void)
{
	return platform_driver_register(&clk_mt6880_dbgsys_dem_drv);
}
arch_initcall(clk_mt6880_dbgsys_dem_platform_init);

#endif	/* MT_CLKMGR_MODULE_INIT */
