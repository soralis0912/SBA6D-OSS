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

static const struct mtk_gate_regs audsys0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x0,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs audsys1_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x4,
	.sta_ofs = 0x4,
};

static const struct mtk_gate_regs audsys2_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x8,
	.sta_ofs = 0x8,
};

#define GATE_AUDSYS0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audsys0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AUDSYS1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audsys1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AUDSYS2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audsys2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate audsys_clks[] = {
	/* AUDSYS0 */
	GATE_AUDSYS0(CLK_AUDSYS_AFE, "aud_afe",
			"audio_ck"/* parent */, 2),
	GATE_AUDSYS0(CLK_AUDSYS_22M, "aud_22m",
			"aud_engen1_ck"/* parent */, 8),
	GATE_AUDSYS0(CLK_AUDSYS_24M, "aud_24m",
			"aud_engen2_ck"/* parent */, 9),
	GATE_AUDSYS0(CLK_AUDSYS_APLL2_TUNER, "aud_apll2_tuner",
			"aud_engen2_ck"/* parent */, 18),
	GATE_AUDSYS0(CLK_AUDSYS_APLL_TUNER, "aud_apll_tuner",
			"aud_engen1_ck"/* parent */, 19),
	GATE_AUDSYS0(CLK_AUDSYS_TDM, "aud_tdm_ck",
			"aud_engen1_ck"/* parent */, 20),
	GATE_AUDSYS0(CLK_AUDSYS_ADC, "aud_adc",
			"audio_ck"/* parent */, 24),
	GATE_AUDSYS0(CLK_AUDSYS_DAC, "aud_dac",
			"audio_ck"/* parent */, 25),
	GATE_AUDSYS0(CLK_AUDSYS_DAC_PREDIS, "aud_dac_predis",
			"audio_ck"/* parent */, 26),
	GATE_AUDSYS0(CLK_AUDSYS_TML, "aud_tml",
			"audio_ck"/* parent */, 27),
	/* AUDSYS1 */
	GATE_AUDSYS1(CLK_AUDSYS_I2S0_BCLK, "aud_i2s0_bclk",
			"audio_ck"/* parent */, 4),
	GATE_AUDSYS1(CLK_AUDSYS_I2S1_BCLK, "aud_i2s1_bclk",
			"audio_ck"/* parent */, 5),
	GATE_AUDSYS1(CLK_AUDSYS_I2S2_BCLK, "aud_i2s2_bclk",
			"audio_ck"/* parent */, 6),
	GATE_AUDSYS1(CLK_AUDSYS_I2S4_BCLK, "aud_i2s4_bclk",
			"audio_ck"/* parent */, 7),
	GATE_AUDSYS1(CLK_AUDSYS_I2S5_BCLK, "aud_i2s5_bclk",
			"audio_ck"/* parent */, 8),
	GATE_AUDSYS1(CLK_AUDSYS_I2S6_BCLK, "aud_i2s6_bclk",
			"audio_ck"/* parent */, 9),
	GATE_AUDSYS1(CLK_AUDSYS_GENERAL1_ASRC, "aud_general1_asrc",
			"audio_ck"/* parent */, 13),
	GATE_AUDSYS1(CLK_AUDSYS_GENERAL2_ASRC, "aud_general2_asrc",
			"audio_ck"/* parent */, 14),
	GATE_AUDSYS1(CLK_AUDSYS_ADDA6_ADC, "aud_adda6_adc",
			"audio_ck"/* parent */, 20),
	/* AUDSYS2 */
	GATE_AUDSYS2(CLK_AUDSYS_CONNSYS_I2S_ASRC, "aud_connsys_i2s_asrc",
			"audio_ck"/* parent */, 12),
	GATE_AUDSYS2(CLK_AUDSYS_AFE_SRC_PCM_TX, "aud_afe_src_pcm_tx",
			"audio_ck"/* parent */, 16),
	GATE_AUDSYS2(CLK_AUDSYS_AFE_SRC_PCM_TX2, "aud_afe_src_pcm_tx2",
			"audio_ck"/* parent */, 17),
	GATE_AUDSYS2(CLK_AUDSYS_AFE_SRC_PCM_TX3, "aud_afe_src_pcm_tx3",
			"audio_ck"/* parent */, 18),
	GATE_AUDSYS2(CLK_AUDSYS_AFE_SRC_PCM_RX, "aud_afe_src_pcm_rx",
			"audio_ck"/* parent */, 19),
	GATE_AUDSYS2(CLK_AUDSYS_AFE_SRC_I2SIN, "aud_afe_src_i2sin",
			"audio_ck"/* parent */, 20),
	GATE_AUDSYS2(CLK_AUDSYS_AFE_SRC_I2SOUT, "aud_afe_src_i2sout",
			"audio_ck"/* parent */, 21),
};

static int clk_mt6880_audsys_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_AUDSYS_NR_CLK);

	mtk_clk_register_gates(node, audsys_clks, ARRAY_SIZE(audsys_clks),
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

static const struct of_device_id of_match_clk_mt6880_audsys[] = {
	{ .compatible = "mediatek,mt6880-audsys", },
	{}
};

#if MT_CLKMGR_MODULE_INIT

static struct platform_driver clk_mt6880_audsys_drv = {
	.probe = clk_mt6880_audsys_probe,
	.driver = {
		.name = "clk-mt6880-audsys",
		.of_match_table = of_match_clk_mt6880_audsys,
	},
};

builtin_platform_driver(clk_mt6880_audsys_drv);

#else

static struct platform_driver clk_mt6880_audsys_drv = {
	.probe = clk_mt6880_audsys_probe,
	.driver = {
		.name = "clk-mt6880-audsys",
		.of_match_table = of_match_clk_mt6880_audsys,
	},
};
static int __init clk_mt6880_audsys_platform_init(void)
{
	return platform_driver_register(&clk_mt6880_audsys_drv);
}
arch_initcall(clk_mt6880_audsys_platform_init);

#endif	/* MT_CLKMGR_MODULE_INIT */
