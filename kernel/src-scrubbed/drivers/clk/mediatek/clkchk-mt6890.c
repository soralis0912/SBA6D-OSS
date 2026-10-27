// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2020 MediaTek Inc.
// Author: David Cheng <pei-hsuan.cheng@mediatek.com>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/seq_file.h>

#include <dt-bindings/power/mt6890-power.h>

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
#include <linux/soc/mediatek/devapc_public.h>
#endif

#include "clkchk.h"
#include "clkchk-mt6890.h"

#define BUG_ON_CHK_ENABLE	0
#define CHECK_VCORE_FREQ		1

/*
 * clkchk dump_regs
 */

#define REGBASE_V(_phys, _id_name, _pg) { .phys = _phys,	\
		.name = #_id_name, .pg = _pg}

static struct regbase rb[] = {
	[top] = REGBASE_V(0x10000000, top, PD_NULL),
	[ifrao] = REGBASE_V(0x10001000, ifrao, PD_NULL),
	[infracfg_ao_bus] = REGBASE_V(0x10001000, infracfg_ao_bus, PD_NULL),
	[spm] = REGBASE_V(0x10006000, spm, PD_NULL),
	[apmixed] = REGBASE_V(0x1000C000, apmixed, PD_NULL),
	[audsys] = REGBASE_V(0x11210000, audsys, MT6890_POWER_DOMAIN_AUDIO),
	[dbgsys_dem] = REGBASE_V(0x0d0a0000, dbgsys_dem, PD_NULL),
	[gce] = REGBASE_V(0x10228000, gce, PD_NULL),
	[impe] = REGBASE_V(0x11cb1000, impe, PD_NULL),
	[mfg] = REGBASE_V(0x13fbf000, mfg, MT6890_POWER_DOMAIN_MFG0),
	[mm] = REGBASE_V(0x14000000, mm, MT6890_POWER_DOMAIN_DIS),
	{},
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	/* TOPCKGEN register */
	REGNAME(top,  0x0010, CLK_CFG_0),
	REGNAME(top,  0x0020, CLK_CFG_1),
	REGNAME(top,  0x0030, CLK_CFG_2),
	REGNAME(top,  0x0040, CLK_CFG_3),
	REGNAME(top,  0x0050, CLK_CFG_4),
	REGNAME(top,  0x0060, CLK_CFG_5),
	REGNAME(top,  0x0070, CLK_CFG_6),
	REGNAME(top,  0x0080, CLK_CFG_7),
	REGNAME(top,  0x0090, CLK_CFG_8),
	REGNAME(top,  0x00A0, CLK_CFG_9),
	REGNAME(top,  0x00B0, CLK_CFG_10),
	REGNAME(top,  0x00C0, CLK_CFG_11),
	REGNAME(top,  0x00D0, CLK_CFG_12),
	REGNAME(top,  0x0320, CLK_AUDDIV_0),
	/* INFRACFG_AO register */
	REGNAME(ifrao,  0x90, MODULE_SW_CG_0),
	REGNAME(ifrao,  0x94, MODULE_SW_CG_1),
	REGNAME(ifrao,  0xac, MODULE_SW_CG_2),
	REGNAME(ifrao,  0xc8, MODULE_SW_CG_3),
	REGNAME(ifrao,  0xe8, MODULE_SW_CG_4),
	/* INFRACFG_AO_BUS register */
	REGNAME(infracfg_ao_bus,  0x0220, INFRA_TOPAXI_PROTECTEN),
	REGNAME(infracfg_ao_bus,  0x0224, INFRA_TOPAXI_PROTECTEN_STA0),
	REGNAME(infracfg_ao_bus,  0x0228, INFRA_TOPAXI_PROTECTEN_STA1),
	REGNAME(infracfg_ao_bus,  0x0B80, INFRA_TOPAXI_PROTECTEN_VDNR),
	REGNAME(infracfg_ao_bus,  0x0B8C, INFRA_TOPAXI_PROTECTEN_VDNR_STA0),
	REGNAME(infracfg_ao_bus,  0x0B90, INFRA_TOPAXI_PROTECTEN_VDNR_STA1),
	REGNAME(infracfg_ao_bus,  0x0250, INFRA_TOPAXI_PROTECTEN_1),
	REGNAME(infracfg_ao_bus,  0x0254, INFRA_TOPAXI_PROTECTEN_STA0_1),
	REGNAME(infracfg_ao_bus,  0x0258, INFRA_TOPAXI_PROTECTEN_STA1_1),
	REGNAME(infracfg_ao_bus,  0x0710, INFRA_TOPAXI_PROTECTEN_2),
	REGNAME(infracfg_ao_bus,  0x0720, INFRA_TOPAXI_PROTECTEN_STA0_2),
	REGNAME(infracfg_ao_bus,  0x0724, INFRA_TOPAXI_PROTECTEN_STA1_2),
	REGNAME(infracfg_ao_bus,  0x0BA0, INFRA_TOPAXI_PROTECTEN_VDNR_1),
	REGNAME(infracfg_ao_bus,  0x0BAC, INFRA_TOPAXI_PROTECTEN_VDNR_STA0_1),
	REGNAME(infracfg_ao_bus,  0x0BB0, INFRA_TOPAXI_PROTECTEN_VDNR_STA1_1),
	REGNAME(infracfg_ao_bus,  0x02C0, INFRA_TOPAXI_PROTECTEN_MCU),
	REGNAME(infracfg_ao_bus,  0x02E0, INFRA_TOPAXI_PROTECTEN_MCU_STA0),
	REGNAME(infracfg_ao_bus,  0x02E4, INFRA_TOPAXI_PROTECTEN_MCU_STA1),
	REGNAME(infracfg_ao_bus,  0x0DC8, INFRA_TOPAXI_PROTECTEN_MM_2),
	REGNAME(infracfg_ao_bus,  0x0DD4, INFRA_TOPAXI_PROTECTEN_MM_STA0_2),
	REGNAME(infracfg_ao_bus,  0x0DD8, INFRA_TOPAXI_PROTECTEN_MM_STA1_2),
	REGNAME(infracfg_ao_bus,  0x02D0, INFRA_TOPAXI_PROTECTEN_MM),
	REGNAME(infracfg_ao_bus,  0x02E8, INFRA_TOPAXI_PROTECTEN_MM_STA0),
	REGNAME(infracfg_ao_bus,  0x02EC, INFRA_TOPAXI_PROTECTEN_MM_STA1),
	REGNAME(infracfg_ao_bus,  0x0BB4, INFRA_TOPAXI_PROTECTEN_SUB_VDNR),
	REGNAME(infracfg_ao_bus,  0x0BC0, INFRA_TOPAXI_PROTECTEN_SUB_VDNR_STA0),
	REGNAME(infracfg_ao_bus,  0x0BC4, INFRA_TOPAXI_PROTECTEN_SUB_VDNR_STA1),
	/* SPM register */
	REGNAME(spm,  0x16C, PWR_STATUS),
	REGNAME(spm,  0x170, PWR_STATUS_2ND),
	REGNAME(spm,  0x178, OTHER_PWR_STATUS),
	REGNAME(spm,  0x300, MD1_PWR_CON),
	REGNAME(spm,  0x304, CONN_PWR_CON),
	REGNAME(spm,  0x308, MFG0_PWR_CON),
	REGNAME(spm,  0x30C, PEXTP_D_2LX1_PHY_PWR_CON),
	REGNAME(spm,  0x310, PEXTP_R_2LX1_PHY_PWR_CON),
	REGNAME(spm,  0x314, PEXTP_R_1LX2_0P_PHY_PWR_CON),
	REGNAME(spm,  0x318, PEXTP_R_1LX2_1P_PHY_PWR_CON),
	REGNAME(spm,  0x31C, SSUSB_PHY_PWR_CON),
	REGNAME(spm,  0x328, EIP97_PWR_CON),
	REGNAME(spm,  0x330, PEXTP_D_2LX1_PWR_CON),
	REGNAME(spm,  0x334, PEXTP_R_2LX1_PWR_CON),
	REGNAME(spm,  0x338, PEXTP_R_1LX2_PWR_CON),
	REGNAME(spm,  0x33C, ETH_PWR_CON),
	REGNAME(spm,  0x340, SSUSB_PWR_CON),
	REGNAME(spm,  0x344, HSMTOP_PWR_CON),
	REGNAME(spm,  0x34C, NETSYS_PWR_CON),
	REGNAME(spm,  0x350, DIS_PWR_CON),
	REGNAME(spm,  0x354, AUDIO_PWR_CON),
	REGNAME(spm,  0x358, SGMII_0_PHY_PWR_CON),
	REGNAME(spm,  0x35C, SGMII_1_PHY_PWR_CON),
	REGNAME(spm,  0x360, SGMII_0_TOP_PWR_CON),
	REGNAME(spm,  0x364, SGMII_1_TOP_PWR_CON),
	REGNAME(spm,  0x3A4, MSDC_PWR_CON),
	REGNAME(spm,  0x3C0, MCUPM_PWR_CON),
	/* APMIXEDSYS register */
	REGNAME(apmixed,  0x204, ARMPLL_LL_CON0),
	REGNAME(apmixed,  0x208, ARMPLL_LL_CON1),
	REGNAME(apmixed,  0x20c, ARMPLL_LL_CON2),
	REGNAME(apmixed,  0x210, ARMPLL_LL_CON3),
	REGNAME(apmixed,  0x214, ARMPLL_LL_CON4),
	REGNAME(apmixed,  0x218, CCIPLL_CON0),
	REGNAME(apmixed,  0x21c, CCIPLL_CON1),
	REGNAME(apmixed,  0x220, CCIPLL_CON2),
	REGNAME(apmixed,  0x224, CCIPLL_CON3),
	REGNAME(apmixed,  0x228, CCIPLL_CON4),
	REGNAME(apmixed,  0x604, MPLL_CON0),
	REGNAME(apmixed,  0x608, MPLL_CON1),
	REGNAME(apmixed,  0x60c, MPLL_CON2),
	REGNAME(apmixed,  0x610, MPLL_CON3),
	REGNAME(apmixed,  0x614, MPLL_CON4),
	REGNAME(apmixed,  0x404, MAINPLL_CON0),
	REGNAME(apmixed,  0x408, MAINPLL_CON1),
	REGNAME(apmixed,  0x40c, MAINPLL_CON2),
	REGNAME(apmixed,  0x410, MAINPLL_CON3),
	REGNAME(apmixed,  0x414, MAINPLL_CON4),
	REGNAME(apmixed,  0x418, UNIVPLL_CON0),
	REGNAME(apmixed,  0x41c, UNIVPLL_CON1),
	REGNAME(apmixed,  0x420, UNIVPLL_CON2),
	REGNAME(apmixed,  0x424, UNIVPLL_CON3),
	REGNAME(apmixed,  0x428, UNIVPLL_CON4),
	REGNAME(apmixed,  0x22c, MSDCPLL_CON0),
	REGNAME(apmixed,  0x230, MSDCPLL_CON1),
	REGNAME(apmixed,  0x234, MSDCPLL_CON2),
	REGNAME(apmixed,  0x238, MSDCPLL_CON3),
	REGNAME(apmixed,  0x23c, MSDCPLL_CON4),
	REGNAME(apmixed,  0x42c, MMPLL_CON0),
	REGNAME(apmixed,  0x430, MMPLL_CON1),
	REGNAME(apmixed,  0x434, MMPLL_CON2),
	REGNAME(apmixed,  0x438, MMPLL_CON3),
	REGNAME(apmixed,  0x43c, MMPLL_CON4),
	REGNAME(apmixed,  0x618, MFGPLL_CON0),
	REGNAME(apmixed,  0x61c, MFGPLL_CON1),
	REGNAME(apmixed,  0x620, MFGPLL_CON2),
	REGNAME(apmixed,  0x624, MFGPLL_CON3),
	REGNAME(apmixed,  0x628, MFGPLL_CON4),
	REGNAME(apmixed,  0x454, APLL1_CON0),
	REGNAME(apmixed,  0x458, APLL1_CON1),
	REGNAME(apmixed,  0x45c, APLL1_CON2),
	REGNAME(apmixed,  0x460, APLL1_CON3),
	REGNAME(apmixed,  0x464, APLL1_CON4),
	REGNAME(apmixed,  0x468, APLL1_CON5),
	REGNAME(apmixed,  0x46c, APLL2_CON0),
	REGNAME(apmixed,  0x470, APLL2_CON1),
	REGNAME(apmixed,  0x474, APLL2_CON2),
	REGNAME(apmixed,  0x478, APLL2_CON3),
	REGNAME(apmixed,  0x47c, APLL2_CON4),
	REGNAME(apmixed,  0x480, APLL2_CON5),
	REGNAME(apmixed, 0x804 , NET1PLL_CON0),
	REGNAME(apmixed, 0x808 , NET1PLL_CON1),
	REGNAME(apmixed, 0x80c , NET1PLL_CON2),
	REGNAME(apmixed, 0x810 , NET1PLL_CON3),
	REGNAME(apmixed, 0x814 , NET1PLL_CON4),
	REGNAME(apmixed, 0x818 , NET2PLL_CON0),
	REGNAME(apmixed, 0x81c , NET2PLL_CON1),
	REGNAME(apmixed, 0x820 , NET2PLL_CON2),
	REGNAME(apmixed, 0x824 , NET2PLL_CON3),
	REGNAME(apmixed, 0x828 , NET2PLL_CON4),
	REGNAME(apmixed, 0x82c , WEDMCUPLL_CON0),
	REGNAME(apmixed, 0x830 , WEDMCUPLL_CON1),
	REGNAME(apmixed, 0x834 , WEDMCUPLL_CON2),
	REGNAME(apmixed, 0x838 , WEDMCUPLL_CON3),
	REGNAME(apmixed, 0x83c , WEDMCUPLL_CON4),
	REGNAME(apmixed, 0x840 , MEDMCUPLL_CON0),
	REGNAME(apmixed, 0x844 , MEDMCUPLL_CON1),
	REGNAME(apmixed, 0x848 , MEDMCUPLL_CON2),
	REGNAME(apmixed, 0x84c , MEDMCUPLL_CON3),
	REGNAME(apmixed, 0x850 , MEDMCUPLL_CON4),
	REGNAME(apmixed, 0x240 , SGMIIPLL_CON0),
	REGNAME(apmixed, 0x244 , SGMIIPLL_CON1),
	REGNAME(apmixed, 0x248 , SGMIIPLL_CON2),
	REGNAME(apmixed, 0x24c , SGMIIPLL_CON3),
	REGNAME(apmixed, 0x250 , SGMIIPLL_CON4),
	/* AUDIO register */
	REGNAME(audsys,  0x0, AUDIO_TOP_0),
	REGNAME(audsys,  0x4, AUDIO_TOP_1),
	REGNAME(audsys,  0x8, AUDIO_TOP_2),
	/* dbgsys_dem register */
	REGNAME(dbgsys_dem,  0x70, ATB),
	REGNAME(dbgsys_dem,  0x2c, DBGBUSCLK_EN),
	REGNAME(dbgsys_dem,  0x30, DBGSYSCLK_EN),
	/* gce register */
	REGNAME(gce,  0xf0, GCE_CTL_INT0),
	/* IMP_IIC_WRAP_E register */
	REGNAME(impe,  0xe00, AP_CLOCK_CG_EST),
	/* MFGCFG register */
	REGNAME(mfg,  0x0, MFG_CG),
	/* MMSYS_CONFIG register */
	REGNAME(mm,  0x100, MMSYS_CG_CON0),
	REGNAME(mm,  0x110, MMSYS_CG_CON1),
	REGNAME(mm,  0x120, MMSYS_CG_CON2),
	{},
};

static const struct regname *get_all_mt6890_regnames(void)
{
	return rn;
}

static void init_regbase(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rb) - 1; i++) {
		if (!rb[i].phys)
			continue;

		rb[i].virt = ioremap_nocache(rb[i].phys, 0x1000);
	}
}

/*
 * clkchk pwr_state
 */
static struct pvd_msk pvd_pwr_mask[] = {
	{"infracfg_ao", PWR_STA, 0x00000200},
	{"topckgen", PWR_STA, 0x00000200},
	{"apmixedsys", PWR_STA, 0x00000200},
	{"mfgsys", PWR_STA, 0x00000004},
	{"mmsys_config", PWR_STA, 0x00100000},
	{"audsys", PWR_STA, 0x00200000},
	{},
};

static struct pvd_msk *get_pvd_pwr_mask(void)
{
	return pvd_pwr_mask;
}

/*
 * clkchk vf table
 */

#define MTK_VF_TABLE(_n, _freq0, _freq1, _freq2, _freq3) {		\
		.name = _n,		\
		.freq_table = {_freq0, _freq1, _freq2, _freq3},	\
	}

struct mtk_vf {
	const char *name;
	int freq_table[4];
};
/*
 * Opp0 : 0p725v
 * Opp1 : 0p65v
 * Opp2 : 0p60v
 * Opp3 : 0p55v
 */
static struct mtk_vf vf_table[] = {
	/* Opp0, Opp1, Opp2, Opp3 */
	MTK_VF_TABLE("axi_sel", 156000, 156000, 156000, 136500),
	MTK_VF_TABLE("spm_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("bus_aximem_sel", 156000, 156000, 156000, 156000),
	MTK_VF_TABLE("mm_sel", 178285.7, 178285.7, 178285.7, 178285.7),
	MTK_VF_TABLE("mfg_ref_sel", 416000, 416000, 218400, 218400),
	MTK_VF_TABLE("uart_sel", 52000, 52000, 52000, 52000),
	MTK_VF_TABLE("msdc50_0_h_sel", 273000, 273000, 273000, 273000),
	MTK_VF_TABLE("msdc50_0_sel", 416000, 416000, 416000, 416000),
	MTK_VF_TABLE("msdc30_1_sel", 208000, 208000, 208000, 208000),
	MTK_VF_TABLE("audio_sel", 54600, 54600, 54600, 54600),
	MTK_VF_TABLE("aud_intbus_sel", 136500, 136500, 136500, 136500),
	MTK_VF_TABLE("aud_engen1_sel", 22579.2, 22579.2, 22579.2, 22579.2),
	MTK_VF_TABLE("aud_engen2_sel", 24576, 24576, 24576, 24576),
	MTK_VF_TABLE("aud_1_sel", 180633.6, 180633.6, 180633.6, 180633.6),
	MTK_VF_TABLE("aud_2_sel", 196608, 196608, 196608, 196608),
	MTK_VF_TABLE("pwrap_ulposc_sel", 65000, 65000, 65000, 65000),
	MTK_VF_TABLE("atb_sel", 273000, 273000, 273000, 273000),
	MTK_VF_TABLE("pwrmcu_sel", 364000, 312000, 312000, 273000),
	MTK_VF_TABLE("dbi_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("disp_pwm_sel", 130000, 130000, 130000, 130000),
	MTK_VF_TABLE("usb_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("ssusb_xhci_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("i2c_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("tl_sel", 136500, 136500, 136500, 136500),
	MTK_VF_TABLE("dpmaif_main_sel", 364000, 364000, 364000, 273000),
	MTK_VF_TABLE("pwm_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("spmi_m_mst_sel", 32500, 32500, 32500, 32500),
	MTK_VF_TABLE("spmi_p_mst_sel", 32500, 32500, 32500, 32500),
	MTK_VF_TABLE("dvfsrc_sel", 26000, 26000, 26000, 26000),
	MTK_VF_TABLE("mcupm_sel", 182000, 182000, 182000, 182000),
	MTK_VF_TABLE("sflash_sel", 62400, 62400, 62400, 62400),
	MTK_VF_TABLE("gcpu_sel", 416000, 364000, 312000, 273000),
	MTK_VF_TABLE("spi_sel", 208000, 208000, 208000, 178285.7),
	MTK_VF_TABLE("spis_sel", 416000, 416000, 416000, 312000),
	MTK_VF_TABLE("ecc_sel", 312000, 242666.7, 242666.7, 136500),
	MTK_VF_TABLE("nfi1x_sel", 182000, 182000, 182000, 182000),
	MTK_VF_TABLE("spinfi_bclk_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("netsys_sel", 356571.4, 242666.7, 156000, 78000),
	MTK_VF_TABLE("medsys_sel", 356571.4, 242666.7, 156000, 78000),
	MTK_VF_TABLE("hsm_crypto_sel", 312000, 312000, 26000, 26000),
	MTK_VF_TABLE("hsm_arc_sel", 182000, 182000, 26000, 26000),
	MTK_VF_TABLE("eip97_sel", 800000, 546000, 364000, 218400),
	MTK_VF_TABLE("snps_eth_312p5m_sel", 312500, 312500, 312500, 312500),
	MTK_VF_TABLE("snps_eth_250m_sel", 250000, 250000, 250000, 250000),
	MTK_VF_TABLE("snps_ptp_sel", 62400, 62400, 62400, 62400),
	MTK_VF_TABLE("snps_rmii_sel", 50000, 50000, 50000, 50000),
	MTK_VF_TABLE("netsys_500m_sel", 500000, 500000, 500000, 500000),
	MTK_VF_TABLE("netsys_med_mcu_sel", 580000, 356571.4, 273000, 104000),
	MTK_VF_TABLE("netsys_wed_mcu_sel", 760000, 436000, 364000, 182000),
	MTK_VF_TABLE("netsys_2x_sel", 800000, 546000, 273000, 124800),
	MTK_VF_TABLE("sgmii_sel", 325000, 325000, 325000, 325000),
	MTK_VF_TABLE("sgmii_sbus_sel", 78000, 78000, 78000, 78000),
	{},
};

static const char *get_vf_name(int id)
{
	return vf_table[id].name;
}

static int get_vf_opp(int id, int opp)
{
	return vf_table[id].freq_table[opp];
}

static u32 get_vf_num(void)
{
	return ARRAY_SIZE(vf_table) - 1;
}

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
/*
 * MT6890: for devapc debug
 */
static bool is_valid_reg(void __iomem *addr)
{
#ifdef CONFIG_64BIT
	return ((u64)addr & 0xf0000000) != 0UL ||
			(((u64)addr >> 32U) & 0xf0000000) != 0UL;
#else
	return ((u32)addr & 0xf0000000) != 0U;
#endif
}

void print_subsys_reg(enum chk_sys_id id)
{
	struct regbase *rb_dump;
	const struct regname *rns = &rn[0];
	int i;

	if (id >= chk_sys_num) {
		pr_info("wrong id:%d\n", id);
		return;
	}

	rb_dump = &rb[id];

	for (i = 0; i < ARRAY_SIZE(rn) - 1; i++, rns++) {
		if (!is_valid_reg(ADDR(rns)))
			return;

		/* filter out the subsys that we don't want */
		if (rns->base != rb_dump)
			continue;

		pr_info("%-18s: [0x%08x] = 0x%08x\n",
			rns->name, PHYSADDR(rns), clk_readl(ADDR(rns)));
	}
}

static void devapc_dump_regs(void)
{
#if BUG_ON_CHK_ENABLE
	print_subsys_reg(spm);
	print_subsys_reg(top);
	print_subsys_reg(ifrao);
	print_subsys_reg(infracfg_ao_bus);
	print_subsys_reg(apmixed);
#else
	pr_info("skip devapc dump\n");
#endif
}

static struct devapc_vio_callbacks devapc_vio_handle = {
	.id = DEVAPC_SUBSYS_CLKMGR,
	.debug_dump = devapc_dump_regs,
};
#endif

/*
 * clkchk dump_clks
 */
static const char * const off_pll_names[] = {
	"univpll",
	"msdcpll",
	"mmpll",
	"mfgpll",
	"apll1",
	"apll2",
	"net1pll",
	"net2pll",
	"wedmcupll",
	"medmcupll",
	"sgmiipll",
	NULL
};

static const char * const notice_pll_names[] = {
	NULL
};

static const char * const *get_off_pll_names(void)
{
	return off_pll_names;
}

static const char * const *get_notice_pll_names(void)
{
	return notice_pll_names;
}

static bool is_pll_chk_bug_on(void)
{
#if BUG_ON_CHK_ENABLE
	return true;
#endif
	return false;
}

static int get_vcore_opp(void)
{
#if IS_ENABLED(CONFIG_MTK_DVFSRC_HELPER)
	return get_sw_req_vcore_opp();
#else
	return VCORE_NULL;
#endif
}

static int pwr_stat[STA_NUM];

static int *get_pwr_status(void)
{
	const struct regname *rn = get_all_mt6890_regnames();

	for (; rn->base != NULL; rn++) {
		if (!strncmp("PWR_STATUS", rn->name, sizeof("PWR_STATUS")))
			pwr_stat[PWR_STA] = clk_readl(ADDR(rn));
		else if (!strncmp("PWR_STATUS_2ND", rn->name, sizeof("PWR_STATUS_2ND")))
			pwr_stat[PWR_STA2] = clk_readl(ADDR(rn));
		else if (!strncmp("OTHER_PWR_STATUS", rn->name, sizeof("OTHER_PWR_STATUS")))
			pwr_stat[OTHER_STA] = clk_readl(ADDR(rn));
//		else
//			return ERR_PTR(-EINVAL);
	}

	return pwr_stat;
}

/*
 * init functions
 */

static struct clkchk_ops clkchk_mt6890_ops = {
	.get_all_regnames = get_all_mt6890_regnames,
	.get_pvd_pwr_mask = get_pvd_pwr_mask,
	.get_off_pll_names = get_off_pll_names,
	.get_notice_pll_names = get_notice_pll_names,
	.is_pll_chk_bug_on = is_pll_chk_bug_on,
	.get_vf_name = get_vf_name,
	.get_vf_opp = get_vf_opp,
	.get_vf_num = get_vf_num,
	.get_vcore_opp = get_vcore_opp,
	.get_pwr_stat = get_pwr_status,
};

void clkchk_set_cfg(void)
{
	init_regbase();

	set_clkchk_ops(&clkchk_mt6890_ops);

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
	register_devapc_vio_callback(&devapc_vio_handle);
#endif
}
