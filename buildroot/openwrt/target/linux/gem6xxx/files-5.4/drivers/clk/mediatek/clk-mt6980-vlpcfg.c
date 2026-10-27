// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.
// Author: Pei-hsuan Cheng <pei-hsuan.cheng@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt6980-clk.h>

#define MT_CCF_BRINGUP		0

/* Regular Number Definition */
#define INV_OFS			-1
#define INV_BIT			-1

static const struct mtk_gate_regs vlpcfg0_cg_regs = {
	.set_ofs = 0x118,
	.clr_ofs = 0x118,
	.sta_ofs = 0x118,
};

static const struct mtk_gate_regs vlpcfg1_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x8,
	.sta_ofs = 0x8,
};

#define GATE_VLPCFG0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vlpcfg0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_VLPCFG0_I(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vlpcfg0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_VLPCFG1_I(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vlpcfg1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate vlpcfg_clks[] = {
	/* VLPCFG0 */
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_TL_32K, "vlpcfg_pcie_tl_32k",
			"rtc_ck"/* parent */, 0),
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_TL_26M, "vlpcfg_pcie_tl_26m",
			"f26m_ck"/* parent */, 1),
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_TL_96M, "vlpcfg_pcie_tl_96m",
			"vlp_tl_vlp_ck"/* parent */, 2),
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_TL_ULPOSC, "vlpcfg_PCIE_ULPOSC",
			"vlp_pcie_26m_ck"/* parent */, 3),
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_REF_CK_26M, "vlpcfg_pcie_ref_26m",
			"f26m_ck"/* parent */, 5),
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_AXI250, "vlpcfg_pcie_axi250",
			"pcie_250m_ck"/* parent */, 6),
	GATE_VLPCFG0(CLK_VLPCFG_PCIE_SRAM_26M, "vlpcfg_pcie_sram_26m",
			"f26m_ck"/* parent */, 7),
	/* VLPCFG1 */
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_1, "vlpcfg_pwm_cgen_1",
			"vlp_pwm_vlp_ck"/* parent */, 0),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_2, "vlpcfg_pwm_cgen_2",
			"vlp_pwm_vlp_ck"/* parent */, 1),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_3, "vlpcfg_pwm_cgen_3",
			"vlp_pwm_vlp_ck"/* parent */, 2),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_4, "vlpcfg_pwm_cgen_4",
			"vlp_pwm_vlp_ck"/* parent */, 3),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_5, "vlpcfg_pwm_cgen_5",
			"vlp_pwm_vlp_ck"/* parent */, 4),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_6, "vlpcfg_pwm_cgen_6",
			"vlp_pwm_vlp_ck"/* parent */, 5),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_7, "vlpcfg_pwm_cgen_7",
			"vlp_pwm_vlp_ck"/* parent */, 6),
	GATE_VLPCFG1_I(CLK_VLPCFG_PWM_CK_CGEN_8, "vlpcfg_pwm_cgen_8",
			"vlp_pwm_vlp_ck"/* parent */, 7),
};

//static const struct mtk_clk_desc vlpcfg_mcd = {
//	.clks = vlpcfg_clks,
//	.num_clks = CLK_VLPCFG_NR_CLK,
//};

static int clk_mt6980_vlpcfg_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_VLPCFG_NR_CLK);

	mtk_clk_register_gates(node, vlpcfg_clks, ARRAY_SIZE(vlpcfg_clks),
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

static const struct of_device_id of_match_clk_mt6980_vlpcfg[] = {
	{
		.compatible = "mediatek,mt6980-vlpcfg",
//		.data = &vlpcfg_mcd,
	},
	{}
};

static struct platform_driver clk_mt6980_vlpcfg_drv = {
	.probe = clk_mt6980_vlpcfg_probe,
	.driver = {
		.name = "clk-mt6980-vlpcfg",
		.of_match_table = of_match_clk_mt6980_vlpcfg,
	},
};

static int __init clk_mt6980_vlpcfg_init(void)
{
	return platform_driver_register(&clk_mt6980_vlpcfg_drv);
}

static void __exit clk_mt6980_vlpcfg_exit(void)
{
	platform_driver_unregister(&clk_mt6980_vlpcfg_drv);
}

arch_initcall(clk_mt6980_vlpcfg_init);
module_exit(clk_mt6980_vlpcfg_exit);
MODULE_LICENSE("GPL");
