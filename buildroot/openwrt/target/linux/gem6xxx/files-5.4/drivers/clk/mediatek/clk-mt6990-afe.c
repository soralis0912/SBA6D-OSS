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

static const struct mtk_gate_regs afe0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x0,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs afe1_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x4,
	.sta_ofs = 0x4,
};

static const struct mtk_gate_regs afe2_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x8,
	.sta_ofs = 0x8,
};

#define GATE_AFE0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &afe0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AFE1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &afe1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AFE2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &afe2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate afe_clks[] = {
	/* AFE0 */
	GATE_AFE0(CLK_AFE_AFE, "afe_afe",
			"audio_ck"/* parent */, 2),
	GATE_AFE0(CLK_AFE_22M, "afe_22m",
			"aud_engen1_ck"/* parent */, 8),
	GATE_AFE0(CLK_AFE_24M, "afe_24m",
			"aud_engen2_ck"/* parent */, 9),
	GATE_AFE0(CLK_AFE_APLL2_TUNER, "afe_apll2_tuner",
			"aud_engen2_ck"/* parent */, 18),
	GATE_AFE0(CLK_AFE_APLL_TUNER, "afe_apll_tuner",
			"aud_engen1_ck"/* parent */, 19),
	GATE_AFE0(CLK_AFE_TDM, "afe_tdm_ck",
			"aud_engen1_ck"/* parent */, 20),
	GATE_AFE0(CLK_AFE_ADC, "afe_adc",
			"audio_ck"/* parent */, 24),
	GATE_AFE0(CLK_AFE_DAC, "afe_dac",
			"audio_ck"/* parent */, 25),
	GATE_AFE0(CLK_AFE_DAC_PREDIS, "afe_dac_predis",
			"audio_ck"/* parent */, 26),
	GATE_AFE0(CLK_AFE_TML, "afe_tml",
			"audio_ck"/* parent */, 27),
	/* AFE1 */
	GATE_AFE1(CLK_AFE_I2S0_BCLK, "afe_i2s0_bclk",
			"audio_ck"/* parent */, 4),
	GATE_AFE1(CLK_AFE_I2S1_BCLK, "afe_i2s1_bclk",
			"audio_ck"/* parent */, 5),
	GATE_AFE1(CLK_AFE_I2S2_BCLK, "afe_i2s2_bclk",
			"audio_ck"/* parent */, 6),
	GATE_AFE1(CLK_AFE_I2S4_BCLK, "afe_i2s4_bclk",
			"audio_ck"/* parent */, 7),
	GATE_AFE1(CLK_AFE_I2S5_BCLK, "afe_i2s5_bclk",
			"audio_ck"/* parent */, 8),
	GATE_AFE1(CLK_AFE_I2S6_BCLK, "afe_i2s6_bclk",
			"audio_ck"/* parent */, 9),
	GATE_AFE1(CLK_AFE_GENERAL2_ASRC, "afe_general2_asrc",
			"audio_ck"/* parent */, 14),
	GATE_AFE1(CLK_AFE_ETDM_OUT0_BCLK, "afe_etdm_out0_bclk",
			"audio_ck"/* parent */, 23),
	/* AFE2 */
	GATE_AFE2(CLK_AFE_CONNSYS_I2S_ASRC, "afe_connsys_i2s_asrc",
			"audio_ck"/* parent */, 12),
	GATE_AFE2(CLK_AFE_AFE_SRC_PCM_TX, "afe_afe_src_pcm_tx",
			"audio_ck"/* parent */, 16),
	GATE_AFE2(CLK_AFE_AFE_SRC_PCM_TX2, "afe_afe_src_pcm_tx2",
			"audio_ck"/* parent */, 17),
	GATE_AFE2(CLK_AFE_AFE_SRC_PCM_TX3, "afe_afe_src_pcm_tx3",
			"audio_ck"/* parent */, 18),
	GATE_AFE2(CLK_AFE_AFE_SRC_PCM_RX, "afe_afe_src_pcm_rx",
			"audio_ck"/* parent */, 19),
	GATE_AFE2(CLK_AFE_AFE_SRC_I2SIN, "afe_afe_src_i2sin",
			"audio_ck"/* parent */, 20),
	GATE_AFE2(CLK_AFE_AFE_SRC_I2SOUT, "afe_afe_src_i2sout",
			"audio_ck"/* parent */, 21),
};

//static const struct mtk_clk_desc afe_mcd = {
//	.clks = afe_clks,
//	.num_clks = CLK_AFE_NR_CLK,
//};

static int clk_mt6990_afe_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_AFE_NR_CLK);

	mtk_clk_register_gates(node, afe_clks, ARRAY_SIZE(afe_clks),
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

static const struct of_device_id of_match_clk_mt6990_afe[] = {
	{
		.compatible = "mediatek,mt6990-afe",
//		.data = &afe_mcd,
	},
	{}
};

static struct platform_driver clk_mt6990_afe_drv = {
	.probe = clk_mt6990_afe_probe,
	.driver = {
		.name = "clk-mt6990-afe",
		.of_match_table = of_match_clk_mt6990_afe,
	},
};

static int __init clk_mt6990_afe_init(void)
{
	return platform_driver_register(&clk_mt6990_afe_drv);
}

static void __exit clk_mt6990_afe_exit(void)
{
	platform_driver_unregister(&clk_mt6990_afe_drv);
}

arch_initcall(clk_mt6990_afe_init);
module_exit(clk_mt6990_afe_exit);
MODULE_LICENSE("GPL");
