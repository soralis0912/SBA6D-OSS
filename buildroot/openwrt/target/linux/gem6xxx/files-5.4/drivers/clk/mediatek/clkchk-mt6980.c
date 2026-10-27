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

#include <dt-bindings/power/mt6980-power.h>

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
#include <linux/soc/mediatek/devapc_public.h>
#endif

#include "clkchk.h"
#include "clkchk-mt6980.h"

#define BUG_ON_CHK_ENABLE	0
#define CHECK_VCORE_FREQ		0

/*
 * clkchk dump_regs
 */

#define REGBASE_V(_phys, _id_name, _pg) { .phys = _phys,	\
		.name = #_id_name, .pg = _pg}

static struct regbase rb[] = {
	[top] = REGBASE_V(0x10000000, top, PD_NULL),
	[ifrao] = REGBASE_V(0x10001000, ifrao, PD_NULL),
	[infracfg] = REGBASE_V(0x10001000, infracfg, PD_NULL),
	[apmixed] = REGBASE_V(0x10010000, apmixed, PD_NULL),
	[gce] = REGBASE_V(0x10228000, gce, PD_NULL),
	[imp_iic_top_wrap] = REGBASE_V(0x11000000, imp_iic_top_wrap, PD_NULL),
	[perao] = REGBASE_V(0x11036000, perao, PD_NULL),
	[usb_sif] = REGBASE_V(0x11203e00, usb_sif, PD_NULL),
	[afe] = REGBASE_V(0x11210000, afe, PD_NULL),
	[msdc0] = REGBASE_V(0x11230000, msdc0, MT6980_POWER_DOMAIN_MSDC),
	[msdc1] = REGBASE_V(0x11240000, msdc1, MT6980_POWER_DOMAIN_MSDC),
	[usb_t5ff] = REGBASE_V(0x11ec0300, usb_t5ff, PD_NULL),
	[spm] = REGBASE_V(0x12001000, spm, PD_NULL),
	[vlpcfg] = REGBASE_V(0x1200C000, vlpcfg, PD_NULL),
	[vlp_ck] = REGBASE_V(0x12013000, vlp_ck, PD_NULL),
	[sspm_cfg] = REGBASE_V(0x12300000, sspm_cfg, PD_NULL),
	[mfg_top] = REGBASE_V(0x13fbf000, mfg_top, PD_NULL),
	[mdp] = REGBASE_V(0x14000000, mdp, PD_NULL),
	[dbgao] = REGBASE_V(0xd010000, dbgao, PD_NULL),
	[dem] = REGBASE_V(0xd0a0000, dem, PD_NULL),
	{},
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	/* TOPCKGEN register */
	REGNAME(top, 0x0010, CLK_CFG_0),
	REGNAME(top, 0x0020, CLK_CFG_1),
	REGNAME(top, 0x0030, CLK_CFG_2),
	REGNAME(top, 0x0040, CLK_CFG_3),
	REGNAME(top, 0x0050, CLK_CFG_4),
	REGNAME(top, 0x0060, CLK_CFG_5),
	REGNAME(top, 0x0070, CLK_CFG_6),
	REGNAME(top, 0x0080, CLK_CFG_7),
	REGNAME(top, 0x0090, CLK_CFG_8),
	REGNAME(top, 0x00A0, CLK_CFG_9),
	REGNAME(top, 0x00B0, CLK_CFG_10),
	REGNAME(top, 0x00C0, CLK_CFG_11),
	REGNAME(top, 0x00D0, CLK_CFG_12),
	REGNAME(top, 0x00E0, CLK_CFG_13),
	REGNAME(top, 0x00F0, CLK_CFG_14),
	REGNAME(top, 0x0100, CLK_CFG_15),
	REGNAME(top, 0x0320, CLK_AUDDIV_0),
	REGNAME(top, 0x0328, CLK_AUDDIV_2),
	REGNAME(top, 0x0334, CLK_AUDDIV_3),
	/* INFRACFG_AO register */
	REGNAME(ifrao, 0xC8, MODULE_CG_3),
	REGNAME(ifrao, 0xE8, MODULE_CG_4),
	/* INFRACFG_AO_BUS register */
	REGNAME(infracfg, 0x2A4, INFRA_TOPAXI_PROTECTEN_1),
	REGNAME(infracfg, 0x258, INFRA_TOPAXI_PROTECTEN_STA1_1),
	REGNAME(infracfg, 0x710, INFRA_TOPAXI_PROTECTEN_2),
	REGNAME(infracfg, 0x724, INFRA_TOPAXI_PROTECTEN_STA1_2),
	REGNAME(infracfg, 0xB80, INFRA_TOPAXI_PROTECTEN_INFRA_VDNR),
	REGNAME(infracfg, 0xB90, INFRA_TOPAXI_PROTECTEN_INFRA_VDNR_STA1),
	REGNAME(infracfg, 0xBA0, INFRA_TOPAXI_PROTECTEN_INFRA_VDNR_1),
	REGNAME(infracfg, 0xBB0, INFRA_TOPAXI_PROTECTEN_INFRA_VDNR_1_STA1),
	REGNAME(infracfg, 0x29C, INFRA_TOPAXI_PROTECTEN),
	REGNAME(infracfg, 0x228, INFRA_TOPAXI_PROTECTEN_STA1),
	/* APMIXEDSYS register */
	REGNAME(apmixed, 0x2ec, ARMPLL_LL_CON0),
	REGNAME(apmixed, 0x2f0, ARMPLL_LL_CON1),
	REGNAME(apmixed, 0x2f4, ARMPLL_LL_CON2),
	REGNAME(apmixed, 0x2f8, ARMPLL_LL_CON3),
	REGNAME(apmixed, 0x2fc, ARMPLL_LL_CON4),
	REGNAME(apmixed, 0x328, CCIPLL_CON0),
	REGNAME(apmixed, 0x32c, CCIPLL_CON1),
	REGNAME(apmixed, 0x330, CCIPLL_CON2),
	REGNAME(apmixed, 0x334, CCIPLL_CON3),
	REGNAME(apmixed, 0x338, CCIPLL_CON4),
	REGNAME(apmixed, 0x244, MAINPLL_CON0),
	REGNAME(apmixed, 0x248, MAINPLL_CON1),
	REGNAME(apmixed, 0x24c, MAINPLL_CON2),
	REGNAME(apmixed, 0x250, MAINPLL_CON3),
	REGNAME(apmixed, 0x254, MAINPLL_CON4),
	REGNAME(apmixed, 0x258, UNIVPLL_CON0),
	REGNAME(apmixed, 0x25c, UNIVPLL_CON1),
	REGNAME(apmixed, 0x260, UNIVPLL_CON2),
	REGNAME(apmixed, 0x264, UNIVPLL_CON3),
	REGNAME(apmixed, 0x268, UNIVPLL_CON4),
	REGNAME(apmixed, 0x208, MSDCPLL_CON0),
	REGNAME(apmixed, 0x20c, MSDCPLL_CON1),
	REGNAME(apmixed, 0x210, MSDCPLL_CON2),
	REGNAME(apmixed, 0x214, MSDCPLL_CON3),
	REGNAME(apmixed, 0x218, MSDCPLL_CON4),
	REGNAME(apmixed, 0x26c, MMPLL_CON0),
	REGNAME(apmixed, 0x270, MMPLL_CON1),
	REGNAME(apmixed, 0x274, MMPLL_CON2),
	REGNAME(apmixed, 0x278, MMPLL_CON3),
	REGNAME(apmixed, 0x27c, MMPLL_CON4),
	REGNAME(apmixed, 0x294, APLL1_CON0),
	REGNAME(apmixed, 0x298, APLL1_CON1),
	REGNAME(apmixed, 0x29c, APLL1_CON2),
	REGNAME(apmixed, 0x2a0, APLL1_CON3),
	REGNAME(apmixed, 0x2a4, APLL1_CON4),
	REGNAME(apmixed, 0x2a8, APLL1_CON5),
	REGNAME(apmixed, 0x0040, APLL1_TUNER_CON0),
	REGNAME(apmixed, 0x0000, AP_PLL_CON0),
	REGNAME(apmixed, 0x2ac, APLL2_CON0),
	REGNAME(apmixed, 0x2b0, APLL2_CON1),
	REGNAME(apmixed, 0x2b4, APLL2_CON2),
	REGNAME(apmixed, 0x2b8, APLL2_CON3),
	REGNAME(apmixed, 0x2bc, APLL2_CON4),
	REGNAME(apmixed, 0x2c0, APLL2_CON5),
	REGNAME(apmixed, 0x0054, APLL2_TUNER_CON0),
	REGNAME(apmixed, 0x0000, AP_PLL_CON0),
	REGNAME(apmixed, 0x2c4, MPLL_CON0),
	REGNAME(apmixed, 0x2c8, MPLL_CON1),
	REGNAME(apmixed, 0x2cc, MPLL_CON2),
	REGNAME(apmixed, 0x2d0, MPLL_CON3),
	REGNAME(apmixed, 0x2d4, MPLL_CON4),
	REGNAME(apmixed, 0x230, MFGPLL_CON0),
	REGNAME(apmixed, 0x234, MFGPLL_CON1),
	REGNAME(apmixed, 0x238, MFGPLL_CON2),
	REGNAME(apmixed, 0x23c, MFGPLL_CON3),
	REGNAME(apmixed, 0x240, MFGPLL_CON4),
	REGNAME(apmixed, 0x280, NET1PLL_CON0),
	REGNAME(apmixed, 0x284, NET1PLL_CON1),
	REGNAME(apmixed, 0x288, NET1PLL_CON2),
	REGNAME(apmixed, 0x28c, NET1PLL_CON3),
	REGNAME(apmixed, 0x290, NET1PLL_CON4),
	REGNAME(apmixed, 0x2d8, NET2PLL_CON0),
	REGNAME(apmixed, 0x2dc, NET2PLL_CON1),
	REGNAME(apmixed, 0x2e0, NET2PLL_CON2),
	REGNAME(apmixed, 0x2e4, NET2PLL_CON3),
	REGNAME(apmixed, 0x2e8, NET2PLL_CON4),
	REGNAME(apmixed, 0x300, WEDMCUPLL_CON0),
	REGNAME(apmixed, 0x304, WEDMCUPLL_CON1),
	REGNAME(apmixed, 0x308, WEDMCUPLL_CON2),
	REGNAME(apmixed, 0x30c, WEDMCUPLL_CON3),
	REGNAME(apmixed, 0x310, WEDMCUPLL_CON4),
	REGNAME(apmixed, 0x314, MEDMCUPLL_CON0),
	REGNAME(apmixed, 0x318, MEDMCUPLL_CON1),
	REGNAME(apmixed, 0x31c, MEDMCUPLL_CON2),
	REGNAME(apmixed, 0x320, MEDMCUPLL_CON3),
	REGNAME(apmixed, 0x324, MEDMCUPLL_CON4),
	REGNAME(apmixed, 0x33c, SGMIIPLL_CON0),
	REGNAME(apmixed, 0x340, SGMIIPLL_CON1),
	REGNAME(apmixed, 0x344, SGMIIPLL_CON2),
	REGNAME(apmixed, 0x348, SGMIIPLL_CON3),
	REGNAME(apmixed, 0x34c, SGMIIPLL_CON4),
	/* GCE register */
	REGNAME(gce, 0xF0, GCE_CTL_INT0),
	/* IMP_IIC_TOP_WRAP register */
	REGNAME(imp_iic_top_wrap, 0xD44E00, AP_CG_RO_SOU),
	REGNAME(imp_iic_top_wrap, 0xED2E00, AP_CG_RO_WST_NOR),
	/* PERICFG_AO register */
	REGNAME(perao, 0x10, PERI_CG_0),
	REGNAME(perao, 0x14, PERI_CG_1),
	REGNAME(perao, 0x18, PERI_CG_2),
	/* SSUSB_SIFSLV_IPPC register */
	REGNAME(usb_sif, 0x80, SSUSB_DMA_CTRL),
	REGNAME(usb_sif, 0x50, SSUSB_U2_CTRL_0P),
	REGNAME(usb_sif, 0x30, SSUSB_U3_CTRL_0P),
	/* AFE register */
	REGNAME(afe, 0x0, AUDIO_TOP_0),
	REGNAME(afe, 0x4, AUDIO_TOP_1),
	REGNAME(afe, 0x8, AUDIO_TOP_2),
	/* MSDC0 register */
	REGNAME(msdc0, 0x68, MSDC_NEW_RX_CFG),
	REGNAME(msdc0, 0xB4, PATCH_BIT1),
	/* MSDC1 register */
	REGNAME(msdc1, 0x68, MSDC_NEW_RX_CFG),
	/* SSUSB_USB20_PHY_T5FF register */
	REGNAME(usb_t5ff, 0x80, U2PHYBC12C),
	/* SPM register */
	REGNAME(spm, 0xE00, MD1_PWR_CON),
	REGNAME(spm, 0xF38, PWR_STATUS),
	REGNAME(spm, 0xF3C, PWR_STATUS_2ND),
	REGNAME(spm, 0xF30, MD_BUCK_ISO_CON),
	REGNAME(spm, 0xE04, CONN_ON_PWR_CON),
	REGNAME(spm, 0xE0C, IFR_PWR_CON),
	REGNAME(spm, 0xE10, PERI_PWR_CON),
	REGNAME(spm, 0xE14, AUDIO_PWR_CON),
	REGNAME(spm, 0xE18, DIS0_PWR_CON),
	REGNAME(spm, 0xE38, CPUEB_PWR_CON),
	REGNAME(spm, 0xF40, XPU_PWR_STATUS),
	REGNAME(spm, 0xF44, XPU_PWR_STATUS_2ND),
	REGNAME(spm, 0xE3C, MFG0_PWR_CON),
	REGNAME(spm, 0xE40, EIP97_PWR_CON),
	REGNAME(spm, 0xE44, MSDC_PWR_CON),
	REGNAME(spm, 0xE48, ETH_PWR_CON),
	REGNAME(spm, 0xE4C, MEDSYS_PWR_CON),
	REGNAME(spm, 0xE50, NETSYS_PWR_CON),
	REGNAME(spm, 0xE54, SSUSB_TOP_PWR_CON),
	REGNAME(spm, 0xE58, SSUSB_PHY_PWR_CON),
	REGNAME(spm, 0xE5C, HSM_TOP_PWR_CON),
	REGNAME(spm, 0xE60, PEXTP_R_2LX1_MAC_PWR_CON),
	REGNAME(spm, 0xE64, PEXTP_R_2LX1_PHY_PWR_CON),
	REGNAME(spm, 0xE68, PEXTP_R_2LX2_MAC_PWR_CON),
	REGNAME(spm, 0xE6C, PEXTP_R_2LX2_PHY_PWR_CON),
	REGNAME(spm, 0xE70, PEXTP_D_2LX1_MAC_PWR_CON),
	REGNAME(spm, 0xE74, PEXTP_D_2LX1_PHY_AO_PWR_CON),
	REGNAME(spm, 0xE78, PEXTP_D_2LX1_PHY_PD_PWR_CON),
	REGNAME(spm, 0xE7C, XFI_TOP_0_PWR_CON),
	REGNAME(spm, 0xE80, XFI_PHY_0_PWR_CON),
	REGNAME(spm, 0xE84, XFI_TOP_1_PWR_CON),
	REGNAME(spm, 0xE88, XFI_PHY_1_PWR_CON),
	REGNAME(spm, 0xE8C, SGMII_TOP_0_PWR_CON),
	REGNAME(spm, 0xE90, SGMII_TOP_1_PWR_CON),
	/* VLPCFG register */
	REGNAME(vlpcfg, 0x118, PCIE_MAC_AND_MAC),
	REGNAME(vlpcfg, 0x8, TEST_CK_CTRL_SWCG_2),
	/* VLP_CKSYS register */
	REGNAME(vlp_ck, 0x0008, VLP_CLK_CFG_0),
	REGNAME(vlp_ck, 0x0014, VLP_CLK_CFG_1),
	REGNAME(vlp_ck, 0x0020, VLP_CLK_CFG_2),
	REGNAME(vlp_ck, 0x002C, VLP_CLK_CFG_3),
	REGNAME(vlp_ck, 0x0044, VLP_CLK_CFG_5),
	/* SSPM_CFG register */
	REGNAME(sspm_cfg, 0x43000, SSPM_CK_EN),
	REGNAME(sspm_cfg, 0x43010, SSPM_UART_CTRL),
	/* MFG_TOP register */
	REGNAME(mfg_top, 0x0, MFG_CG),
	/* MDPSYS_CONFIG register */
	REGNAME(mdp, 0x100, MMSYS_CG_0),
	REGNAME(mdp, 0x110, MMSYS_CG_1),
	/* DBGAO register */
	REGNAME(dbgao, 0xA070, ATB),
	/* DEM register */
	REGNAME(dem, 0x70, ATB),
	REGNAME(dem, 0x2C, DBGBUSCLK_EN),
	REGNAME(dem, 0x30, DBGSYSCLK_EN),
	{},
};

static const struct regname *get_all_mt6980_regnames(void)
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
	{"topckgen", PWR_STA, 0x00000008},
	{"apmixedsys", PWR_STA, 0x00000008},
	{"vlp_cksys", PWR_STA, 0x00000008},
	{"afe", PWR_STA, 0x00000020},
	{"dbgao", PWR_STA, 0x00000008},
	{"dem", PWR_STA, 0x00000008},
	{"gce", PWR_STA, 0x00000008},
	{"imp_iic_top_wrap", PWR_STA, 0x00000008},
	{"infracfg_ao", PWR_STA, 0x00000008},
	{"mdpsys", PWR_STA, 0x00000040},
	{"mfg_top", XPU_PWR_STA, 0x00000002},
	{"msdc0", PWR_STA, 0x00010000},
	{"msdc1", PWR_STA, 0x00010000},
	{"pericfg_ao", PWR_STA, 0x00000008},
	{"sspm_cfg", PWR_STA, 0x00000008},
	{"ssusb", PWR_STA, 0x00040000},
	{"ssusb_phy", PWR_STA, 0x00040000},
	{"vlpcfg", PWR_STA, 0x00000008},
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
#if CHECK_VCORE_FREQ
static struct mtk_vf vf_table[] = {
	/* Opp0, Opp1, Opp2, Opp3 */
	MTK_VF_TABLE("axi_sel", 156000, 156000, 156000, 156000),
	MTK_VF_TABLE("peri_haxi_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("peri_axi_sel", 156000, 156000, 156000, 156000),
	MTK_VF_TABLE("bus_aximem_sel", 156000, 156000, 156000, 156000),
	MTK_VF_TABLE("mem_sub_sel", 546000, 436800, 273000, 218400),
	MTK_VF_TABLE("dvfsrc_sel", 26000, 26000, 26000, 26000),
	MTK_VF_TABLE("mm_sel", 178286, 178286, 178286, 178286),
	MTK_VF_TABLE("dbi_sel", 182000, 182000, 182000, 182000),
	MTK_VF_TABLE("disp_pwm_sel", 130000, 130000, 130000, 130000),
	MTK_VF_TABLE("mfg_ref_sel", 416000, 416000, 218400, 218400),
	MTK_VF_TABLE("uart_sel", 104000, 104000, 104000, 104000),
	MTK_VF_TABLE("msdc5hclk_sel", 273000, 273000, 273000, 273000),
	MTK_VF_TABLE("msdc50_0_sel", 416000, 416000, 416000, 416000),
	MTK_VF_TABLE("msdc30_1_sel", 208000, 208000, 208000, 208000),
	MTK_VF_TABLE("msdc_macro_sel", 416000, 416000, 416000, 416000),
	MTK_VF_TABLE("audio_sel", 54600, 54600, 54600, 54600),
	MTK_VF_TABLE("aud_intbus_sel", 136500, 136500, 136500, 136500),
	MTK_VF_TABLE("aud_engen1_sel", 22579, 22579, 22579, 22579),
	MTK_VF_TABLE("aud_engen2_sel", 24576, 24576, 24576, 24576),
	MTK_VF_TABLE("aud_1_sel", 180634, 180634, 180634, 180634),
	MTK_VF_TABLE("aud_2_sel", 196608, 196608, 196608, 196608),
	MTK_VF_TABLE("atb_sel", 273000, 273000, 273000, 273000),
	MTK_VF_TABLE("i2c_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("tl_sel", 136500, 136500, 136500, 136500),
	MTK_VF_TABLE("dpmaif_main_sel", 499200, 436800, 364000, 273000),
	MTK_VF_TABLE("mcupm_sel", 182000, 182000, 182000, 182000),
	MTK_VF_TABLE("sflash_sel", 52000, 52000, 52000, 52000),
	MTK_VF_TABLE("spi_sel", 208000, 208000, 208000, 208000),
	MTK_VF_TABLE("spis_sel", 416000, 416000, 416000, 416000),
	MTK_VF_TABLE("nfi1x_sel", 182000, 182000, 182000, 182000),
	MTK_VF_TABLE("spinfi_bclk_sel", 124800, 124800, 124800, 124800),
	MTK_VF_TABLE("gcpu_sel", 364000, 364000, 312000, 273000),
	MTK_VF_TABLE("ecc_sel", 312000, 242667, 242667, 136500),
	MTK_VF_TABLE("hsm_crypto_sel", 312000, 312000, 26000, 26000),
	MTK_VF_TABLE("hsm_arc_sel", 26000, 26000, 182000, 182000),
	MTK_VF_TABLE("eip97_sel", 800000, 546000, 364000, 218400),
	MTK_VF_TABLE("eth_312p5m_sel", 312500, 312500, 312500, 312500),
	MTK_VF_TABLE("snps_eth_250m_sel", 250000, 250000, 250000, 250000),
	MTK_VF_TABLE("eth_62p4m_sel", 62400, 62400, 62400, 62400),
	MTK_VF_TABLE("eth_50m_rmii_sel", 50000, 50000, 50000, 50000),
	MTK_VF_TABLE("netsys_500m_sel", 500000, 500000, 500000, 500000),
	MTK_VF_TABLE("netsys_sel", 356571, 312000, 242667, 182000),
	MTK_VF_TABLE("netsys_2x_sel", 800000, 800000, 728000, 624000),
	MTK_VF_TABLE("netsys_wed_mcu_sel", 760000, 436800, 364000, 182000),
	MTK_VF_TABLE("netsys_med_mcu_sel", 580000, 356571, 273000, 104000),
	MTK_VF_TABLE("sgmii_0_sel", 325000, 325000, 325000, 325000),
	MTK_VF_TABLE("sgmii_sbus_0_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("sgmii_1_sel", 325000, 325000, 325000, 325000),
	MTK_VF_TABLE("sgmii_sbus_1_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("usxgmii_sbus_0_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("usxgmii_sbus_1_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("ap2conn_host_sel", 78000, 78000, 78000, 78000),
	MTK_VF_TABLE("usb_sel", 249600, 249600, 249600, 249600),
	MTK_VF_TABLE("ssusb_xhci_sel", 249600, 249600, 249600, 249600),
	MTK_VF_TABLE("emi_n_sel", 625000, 625000, 625000, 625000),
	MTK_VF_TABLE("hsm_hsah_sel", 364000, 312000, 312000, 136500),
	MTK_VF_TABLE("rsa_sel", 312000, 312000, 26000, 26000),
	MTK_VF_TABLE("msdc_occ_400_sel", 416000, 416000, 416000, 416000),
	MTK_VF_TABLE("msdc_occ_200_sel", 208000, 208000, 208000, 208000),
	MTK_VF_TABLE("dxcc_sel", 273000, 273000, 273000, 273000),
	MTK_VF_TABLE("pcie_250m_sel", 249600, 249600, 249600, 249600),
	MTK_VF_TABLE("dsi_occ_sel", 156000, 156000, 156000, 156000),
	MTK_VF_TABLE("emi_546_sel", 546000, 546000, 546000, 546000),
	MTK_VF_TABLE("emi_624_sel", 624000, 624000, 624000, 624000),
	{},
};
#endif
static const char *get_vf_name(int id)
{
	#if CHECK_VCORE_FREQ
		return vf_table[id].name;
	#else
		return NULL;
	#endif
}

static int get_vf_opp(int id, int opp)
{
	#if CHECK_VCORE_FREQ
		return vf_table[id].freq_table[opp];
	#else
		return NULL;
	#endif
}

static u32 get_vf_num(void)
{
	#if CHECK_VCORE_FREQ
		return ARRAY_SIZE(vf_table) - 1;
	#else
		return NULL;
	#endif
}

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
/*
 * mt6980: for devapc debug
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
	print_subsys_reg(vlp_ck);
	print_subsys_reg(infracfg);
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
	"apll1",
	"apll2",
	"mfgpll",
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
	const struct regname *rn = get_all_mt6980_regnames();

	for (; rn->base != NULL; rn++) {
		if (!strncmp("PWR_STATUS", rn->name, sizeof("PWR_STATUS")))
			pwr_stat[PWR_STA] = clk_readl(ADDR(rn));
		else if (!strncmp("PWR_STATUS_2ND", rn->name, sizeof("PWR_STATUS_2ND")))
			pwr_stat[PWR_STA2] = clk_readl(ADDR(rn));
		else if (!strncmp("XPU_PWR_STATUS", rn->name, sizeof("XPU_PWR_STATUS")))
			pwr_stat[XPU_PWR_STA] = clk_readl(ADDR(rn));
//		else
//			return ERR_PTR(-EINVAL);
	}

	return pwr_stat;
}

/*
 * init functions
 */

static struct clkchk_ops clkchk_mt6980_ops = {
	.get_all_regnames = get_all_mt6980_regnames,
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

	set_clkchk_ops(&clkchk_mt6980_ops);

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
	register_devapc_vio_callback(&devapc_vio_handle);
#endif
}
