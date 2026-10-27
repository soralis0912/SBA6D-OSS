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

static const struct mtk_gate_regs sspm_cfg0_cg_regs = {
	.set_ofs = 0x43000,
	.clr_ofs = 0x43000,
	.sta_ofs = 0x43000,
};

static const struct mtk_gate_regs sspm_cfg1_cg_regs = {
	.set_ofs = 0x43010,
	.clr_ofs = 0x43010,
	.sta_ofs = 0x43010,
};

#define GATE_SSPM_CFG0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &sspm_cfg0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_SSPM_CFG1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &sspm_cfg1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate sspm_cfg_clks[] = {
	/* SSPM_CFG0 */
	GATE_SSPM_CFG0(CLK_SSPM_CFG_TMR_EN, "sspm_cfg_tmr_en",
			"hf_fsspm_ck"/* parent */, 1),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_SLPP_M_EN, "sspm_cfg_slpp_m_en",
			"hf_fsspm_ck"/* parent */, 2),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_HCLK_EN, "sspm_cfg_h_en",
			"hf_fsspm_ck"/* parent */, 3),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_DMA_EN, "sspm_cfg_dma_en",
			"hf_fsspm_ck"/* parent */, 4),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_UART_EN, "sspm_cfg_uart_en",
			"hf_fsspm_ck"/* parent */, 5),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_TWAM_EN, "sspm_cfg_twam_en",
			"hf_fsspm_ck"/* parent */, 6),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_RSV1, "sspm_cfg_rsv1",
			"hf_fsspm_ck"/* parent */, 7),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_PMICWP2P_EN, "sspm_cfg_pmicwp2p_en",
			"hf_fsspm_ck"/* parent */, 8),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_MD32PCLK_EN, "sspm_cfg_md32pclk_en",
			"hf_fsspm_ck"/* parent */, 9),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_SPMP2P_EN, "sspm_cfg_spmp2p_en",
			"hf_fsspm_ck"/* parent */, 13),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_APBBR_EN, "sspm_cfg_apbbr_en",
			"hf_fsspm_ck"/* parent */, 14),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_AXIBR_EN, "sspm_cfg_axibr_en",
			"hf_fsspm_ck"/* parent */, 15),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_SLPP_I_EN, "sspm_cfg_slpp_i_en",
			"hf_fsspm_ck"/* parent */, 16),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_SLPP_H_EN, "sspm_cfg_slpp_h_en",
			"hf_fsspm_ck"/* parent */, 17),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_ICACHE_EN, "sspm_cfg_icache_en",
			"hf_fsspm_ck"/* parent */, 18),
	GATE_SSPM_CFG0(CLK_SSPM_CFG_LAT_MON_EN, "sspm_cfg_lat_mon_en",
			"hf_fsspm_ck"/* parent */, 20),
	/* SSPM_CFG1 */
	GATE_SSPM_CFG1(CLK_SSPM_CFG_UART_BCLK, "sspm_cfg_uart_bclk",
			"hf_fsspm_ck"/* parent */, 0),
};

//static const struct mtk_clk_desc sspm_cfg_mcd = {
//	.clks = sspm_cfg_clks,
//	.num_clks = CLK_SSPM_CFG_NR_CLK,
//};

static int clk_mt6990_sspm_cfg_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_SSPM_CFG_NR_CLK);

	mtk_clk_register_gates(node, sspm_cfg_clks, ARRAY_SIZE(sspm_cfg_clks),
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

static const struct of_device_id of_match_clk_mt6990_sspm_cfg[] = {
	{
		.compatible = "mediatek,mt6990-sspm_cfg",
//		.data = &sspm_cfg_mcd,
	},
	{}
};

static struct platform_driver clk_mt6990_sspm_cfg_drv = {
	.probe = clk_mt6990_sspm_cfg_probe,
	.driver = {
		.name = "clk-mt6990-sspm_cfg",
		.of_match_table = of_match_clk_mt6990_sspm_cfg,
	},
};

static int __init clk_mt6990_sspm_cfg_init(void)
{
	return platform_driver_register(&clk_mt6990_sspm_cfg_drv);
}

static void __exit clk_mt6990_sspm_cfg_exit(void)
{
	platform_driver_unregister(&clk_mt6990_sspm_cfg_drv);
}

arch_initcall(clk_mt6990_sspm_cfg_init);
module_exit(clk_mt6990_sspm_cfg_exit);
MODULE_LICENSE("GPL");
