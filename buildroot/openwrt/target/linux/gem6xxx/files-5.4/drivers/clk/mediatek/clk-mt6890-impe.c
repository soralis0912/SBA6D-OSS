// SPDX-License-Identifier: GPL-2.0

/*

 * Copyright (c) 2019 MediaTek Inc.

 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt6890-clk.h>

#define MT_CLKMGR_MODULE_INIT	0

#define MT_CCF_BRINGUP		1

#define INV_OFS			-1



static const struct mtk_gate_regs impe_cg_regs = {
	.set_ofs = 0xe00,
	.clr_ofs = 0xe00,
	.sta_ofs = 0xe00,
};

#define GATE_IMPE(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &impe_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate impe_clks[] = {
	GATE_IMPE(CLK_IMPE_AP_CLOCK_I2C0_RO, "impe_i2c0_ro",
			"i2c_ck"/* parent */, 0),
	GATE_IMPE(CLK_IMPE_AP_CLOCK_I2C1_RO, "impe_i2c1_ro",
			"i2c_ck"/* parent */, 1),
	GATE_IMPE(CLK_IMPE_AP_CLOCK_I2C2_RO, "impe_i2c2_ro",
			"i2c_ck"/* parent */, 2),
	GATE_IMPE(CLK_IMPE_AP_CLOCK_I2C3_RO, "impe_i2c3_ro",
			"i2c_ck"/* parent */, 3),
	GATE_IMPE(CLK_IMPE_AP_CLOCK_I2C4_RO, "impe_i2c4_ro",
			"i2c_ck"/* parent */, 4),
	GATE_IMPE(CLK_IMPE_AP_CLOCK_I2C5_RO, "impe_i2c5_ro",
			"i2c_ck"/* parent */, 5),
};

static int clk_mt6890_impe_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_IMPE_NR_CLK);

	mtk_clk_register_gates(node, impe_clks, ARRAY_SIZE(impe_clks),
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

static const struct of_device_id of_match_clk_mt6890_impe[] = {
	{ .compatible = "mediatek,mt6890-imp_iic_wrap_e", },
	{}
};

#if MT_CLKMGR_MODULE_INIT

static struct platform_driver clk_mt6890_impe_drv = {
	.probe = clk_mt6890_impe_probe,
	.driver = {
		.name = "clk-mt6890-impe",
		.of_match_table = of_match_clk_mt6890_impe,
	},
};

builtin_platform_driver(clk_mt6890_impe_drv);

#else

static struct platform_driver clk_mt6890_impe_drv = {
	.probe = clk_mt6890_impe_probe,
	.driver = {
		.name = "clk-mt6890-impe",
		.of_match_table = of_match_clk_mt6890_impe,
	},
};
static int __init clk_mt6890_impe_platform_init(void)
{
	return platform_driver_register(&clk_mt6890_impe_drv);
}
arch_initcall(clk_mt6890_impe_platform_init);

#endif	/* MT_CLKMGR_MODULE_INIT */
