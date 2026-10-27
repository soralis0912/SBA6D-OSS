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

static const struct mtk_gate_regs mdp0_cg_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs mdp1_cg_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

static const struct mtk_gate_regs mdp2_cg_regs = {
	.set_ofs = 0x124,
	.clr_ofs = 0x128,
	.sta_ofs = 0x120,
};

#define GATE_MDP0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mdp0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_MDP1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mdp1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_MDP2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mdp2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mdp_clks[] = {
	/* MDP0 */
	GATE_MDP0(CLK_MDP_MUTEX0, "mdp_mutex0",
			"mm_ck"/* parent */, 0),
	GATE_MDP0(CLK_MDP_APB_BUS, "mdp_apb_bus",
			"mm_ck"/* parent */, 1),
	GATE_MDP0(CLK_MDP_RSZ0, "mdp_rsz0",
			"mm_ck"/* parent */, 2),
	GATE_MDP0(CLK_MDP_DISP_GAMMA0, "mdp_disp_gamma0",
			"mm_ck"/* parent */, 3),
	GATE_MDP0(CLK_MDP_WROT0, "mdp_wrot0",
			"mm_ck"/* parent */, 4),
	GATE_MDP0(CLK_MDP_DISP_COLOR0, "mdp_disp_color0",
			"mm_ck"/* parent */, 5),
	GATE_MDP0(CLK_MDP_DISP_CCORR0, "mdp_disp_ccorr0",
			"mm_ck"/* parent */, 6),
	GATE_MDP0(CLK_MDP_DISP_AAL0, "mdp_disp_aal0",
			"mm_ck"/* parent */, 7),
	GATE_MDP0(CLK_MDP_DISP_RDMA0, "mdp_disp_rdma0",
			"mm_ck"/* parent */, 8),
	GATE_MDP0(CLK_MDP_RDMA0, "mdp_rdma0",
			"mm_ck"/* parent */, 9),
	GATE_MDP0(CLK_MDP_FAKE_ENG0, "mdp_fake_eng0",
			"mm_ck"/* parent */, 10),
	GATE_MDP0(CLK_MDP_DISP_DITHER0, "mdp_disp_dither0",
			"mm_ck"/* parent */, 11),
	GATE_MDP0(CLK_MDP_DISP_WDMA0, "mdp_disp_wdma0",
			"mm_ck"/* parent */, 12),
	GATE_MDP0(CLK_MDP_TDSHP0, "mdp_tdshp0",
			"mm_ck"/* parent */, 13),
	GATE_MDP0(CLK_MDP_DISP_OVL0, "mdp_disp_ovl0",
			"mm_ck"/* parent */, 14),
	GATE_MDP0(CLK_MDP_DISP_DBPI0, "mdp_disp_dbpi0",
			"mm_ck"/* parent */, 15),
	GATE_MDP0(CLK_MDP_DISP_DSI0, "mdp_CLK0",
			"mm_ck"/* parent */, 16),
	GATE_MDP0(CLK_MDP_SMI_COMMON, "mdp_smi_common",
			"mm_ck"/* parent */, 17),
	/* MDP1 */
	GATE_MDP1(CLK_MDP_DISP_DSI0_1, "mdp_disp_dsi0_1",
			"dsi_occ_ck"/* parent */, 16),
	/* MDP1 */
	GATE_MDP2(CLK_MDP_DISP_DBPI0_1, "mdp_disp_dbpi0_1",
			"dbi_ck"/* parent */, 0),
};

//static const struct mtk_clk_desc mdp_mcd = {
//	.clks = mdp_clks,
//	.num_clks = CLK_MDP_NR_CLK,
//};

static const struct of_device_id of_match_clk_mt6990_mmsys[] = {
	{
		.compatible = "mediatek,mt6990-mdpsys",
//		.data = &mdp_mcd,
	}, {
		/* sentinel */
	}
};

static int clk_mt6990_mmsys_grp_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_MDP_NR_CLK);

	mtk_clk_register_gates(node, mdp_clks, ARRAY_SIZE(mdp_clks),
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

static struct platform_driver clk_mt6990_mmsys_drv = {
	.probe = clk_mt6990_mmsys_grp_probe,
	.driver = {
		.name = "clk-mt6990-mmsys",
		.of_match_table = of_match_clk_mt6990_mmsys,
	},
};

static int __init clk_mt6990_mmsys_init(void)
{
	return platform_driver_register(&clk_mt6990_mmsys_drv);
}

static void __exit clk_mt6990_mmsys_exit(void)
{
	platform_driver_unregister(&clk_mt6990_mmsys_drv);
}

arch_initcall(clk_mt6990_mmsys_init);
module_exit(clk_mt6990_mmsys_exit);
MODULE_LICENSE("GPL");
