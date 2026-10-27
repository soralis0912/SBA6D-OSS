// SPDX-License-Identifier: GPL-2.0

/*

 * Copyright (c) 2019 MediaTek Inc.

 */


#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

#include "clk-mtk.h"
#include "clk-mux.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt6880-clk.h>

/* bringup config */
#define MT_CCF_BRINGUP		1
#define MT_CCF_MUX_DISABLE	0
#define MT_CCF_PLL_DISABLE	0

/* Regular Number Definition */
#define INV_OFS	-1
#define INV_BIT	-1

/* TOPCK MUX SEL REG */
#define CLK_CFG_UPDATE		0x04
#define CLK_CFG_UPDATE1		0x08
#define CLK_CFG_0		0x0010
#define CLK_CFG_0_SET		0x0014
#define CLK_CFG_0_CLR		0x0018
#define CLK_CFG_1		0x0020
#define CLK_CFG_1_SET		0x0024
#define CLK_CFG_1_CLR		0x0028
#define CLK_CFG_2		0x0030
#define CLK_CFG_2_SET		0x0034
#define CLK_CFG_2_CLR		0x0038
#define CLK_CFG_3		0x0040
#define CLK_CFG_3_SET		0x0044
#define CLK_CFG_3_CLR		0x0048
#define CLK_CFG_4		0x0050
#define CLK_CFG_4_SET		0x0054
#define CLK_CFG_4_CLR		0x0058
#define CLK_CFG_5		0x0060
#define CLK_CFG_5_SET		0x0064
#define CLK_CFG_5_CLR		0x0068
#define CLK_CFG_6		0x0070
#define CLK_CFG_6_SET		0x0074
#define CLK_CFG_6_CLR		0x0078
#define CLK_CFG_7		0x0080
#define CLK_CFG_7_SET		0x0084
#define CLK_CFG_7_CLR		0x0088
#define CLK_CFG_8		0x0090
#define CLK_CFG_8_SET		0x0094
#define CLK_CFG_8_CLR		0x0098
#define CLK_CFG_9		0x00A0
#define CLK_CFG_9_SET		0x00A4
#define CLK_CFG_9_CLR		0x00A8
#define CLK_CFG_10		0x00B0
#define CLK_CFG_10_SET		0x00B4
#define CLK_CFG_10_CLR		0x00B8
#define CLK_CFG_11		0x00C0
#define CLK_CFG_11_SET		0x00C4
#define CLK_CFG_11_CLR		0x00C8
#define CLK_CFG_12		0x00D0
#define CLK_CFG_12_SET		0x00D4
#define CLK_CFG_12_CLR		0x00D8
#define CLK_AUDDIV_0		0x0320

/* TOPCK MUX SHIFT */
#define TOP_MUX_AXI_SHIFT		0
#define TOP_MUX_SPM_SHIFT		1
#define TOP_MUX_BUS_AXIMEM_SHIFT	2
#define TOP_MUX_MM_SHIFT		3
#define TOP_MUX_MFG_REF_SHIFT		4
#define TOP_MUX_UART_SHIFT		5
#define TOP_MUX_MSDC50_0_HCLK_SHIFT	6
#define TOP_MUX_MSDC50_0_SHIFT		7
#define TOP_MUX_MSDC30_1_SHIFT		8
#define TOP_MUX_AUDIO_SHIFT		9
#define TOP_MUX_AUD_INTBUS_SHIFT	10
#define TOP_MUX_AUD_ENGEN1_SHIFT	11
#define TOP_MUX_AUD_ENGEN2_SHIFT	12
#define TOP_MUX_AUD_1_SHIFT		13
#define TOP_MUX_AUD_2_SHIFT		14
#define TOP_MUX_PWRAP_ULPOSC_SHIFT	15
#define TOP_MUX_ATB_SHIFT		16
#define TOP_MUX_PWRMCU_SHIFT		17
#define TOP_MUX_DBI_SHIFT		18
#define TOP_MUX_DISP_PWM_SHIFT		19
#define TOP_MUX_USB_TOP_SHIFT		20
#define TOP_MUX_SSUSB_XHCI_SHIFT	21
#define TOP_MUX_I2C_SHIFT		22
#define TOP_MUX_TL_SHIFT		23
#define TOP_MUX_DPMAIF_MAIN_SHIFT	24
#define TOP_MUX_PWM_SHIFT		25
#define TOP_MUX_SPMI_M_MST_SHIFT	26
#define TOP_MUX_SPMI_P_MST_SHIFT	27
#define TOP_MUX_DVFSRC_SHIFT		28
#define TOP_MUX_MCUPM_SHIFT		29
#define TOP_MUX_SFLASH_SHIFT		30
#define TOP_MUX_GCPU_SHIFT		0
#define TOP_MUX_SPI_SHIFT		1
#define TOP_MUX_SPIS_SHIFT		2
#define TOP_MUX_ECC_SHIFT		3
#define TOP_MUX_NFI1X_SHIFT		4
#define TOP_MUX_SPINFI_BCLK_SHIFT	5
#define TOP_MUX_NETSYS_SHIFT		6
#define TOP_MUX_MEDSYS_SHIFT		7
#define TOP_MUX_HSM_CRYPTO_SHIFT	8
#define TOP_MUX_HSM_ARC_SHIFT		9
#define TOP_MUX_EIP97_SHIFT		10
#define TOP_MUX_SNPS_ETH_312P5M_SHIFT	11
#define TOP_MUX_SNPS_ETH_250M_SHIFT	12
#define TOP_MUX_SNPS_ETH_62P4M_PTP_SHIFT 13
#define TOP_MUX_SNPS_ETH_50M_RMII_SHIFT	14
#define TOP_MUX_NETSYS_500M_SHIFT	15
#define TOP_MUX_NETSYS_MED_MCU_SHIFT	16
#define TOP_MUX_NETSYS_WED_MCU_SHIFT	17
#define TOP_MUX_NETSYS_2X_SHIFT		18
#define TOP_MUX_SGMII_SHIFT		19
#define TOP_MUX_SGMII_SBUS_SHIFT	20

/* TOPCK DIVIDER REG */
#define CLK_AUDDIV_2		0x0328
#define CLK_AUDDIV_3		0x0334

/* APMIXED PLL REG */
#define ARMPLL_LL_CON0		0x204
#define ARMPLL_LL_CON1		0x208
#define ARMPLL_LL_CON2		0x20c
#define ARMPLL_LL_CON3		0x210
#define ARMPLL_LL_CON4		0x214
#define CCIPLL_CON0		0x218
#define CCIPLL_CON1		0x21c
#define CCIPLL_CON2		0x220
#define CCIPLL_CON3		0x224
#define CCIPLL_CON4		0x228
#define MPLL_CON0		0x604
#define MPLL_CON1		0x608
#define MPLL_CON2		0x60c
#define MPLL_CON3		0x610
#define MPLL_CON4		0x614
#define MAINPLL_CON0		0x404
#define MAINPLL_CON1		0x408
#define MAINPLL_CON2		0x40c
#define MAINPLL_CON3		0x410
#define MAINPLL_CON4		0x414
#define UNIVPLL_CON0		0x418
#define UNIVPLL_CON1		0x41c
#define UNIVPLL_CON2		0x420
#define UNIVPLL_CON3		0x424
#define UNIVPLL_CON4		0x428
#define MSDCPLL_CON0		0x22c
#define MSDCPLL_CON1		0x230
#define MSDCPLL_CON2		0x234
#define MSDCPLL_CON3		0x238
#define MSDCPLL_CON4		0x23c
#define MMPLL_CON0		0x42c
#define MMPLL_CON1		0x430
#define MMPLL_CON2		0x434
#define MMPLL_CON3		0x438
#define MMPLL_CON4		0x43c
#define MFGPLL_CON0		0x618
#define MFGPLL_CON1		0x61c
#define MFGPLL_CON2		0x620
#define MFGPLL_CON3		0x624
#define MFGPLL_CON4		0x628
#define APLL1_CON0		0x454
#define APLL1_CON1		0x458
#define APLL1_CON2		0x45c
#define APLL1_CON3		0x460
#define APLL1_CON4		0x464
#define APLL1_CON5		0x468
#define APLL2_CON0		0x46c
#define APLL2_CON1		0x470
#define APLL2_CON2		0x474
#define APLL2_CON3		0x478
#define APLL2_CON4		0x47c
#define APLL2_CON5		0x480
#define NET1PLL_CON0		0x804
#define NET1PLL_CON1		0x808
#define NET1PLL_CON2		0x80c
#define NET1PLL_CON3		0x810
#define NET1PLL_CON4		0x814
#define NET2PLL_CON0		0x818
#define NET2PLL_CON1		0x81c
#define NET2PLL_CON2		0x820
#define NET2PLL_CON3		0x824
#define NET2PLL_CON4		0x828
#define WEDMCUPLL_CON0		0x82c
#define WEDMCUPLL_CON1		0x830
#define WEDMCUPLL_CON2		0x834
#define WEDMCUPLL_CON3		0x838
#define WEDMCUPLL_CON4		0x83c
#define MEDMCUPLL_CON0		0x840
#define MEDMCUPLL_CON1		0x844
#define MEDMCUPLL_CON2		0x848
#define MEDMCUPLL_CON3		0x84c
#define MEDMCUPLL_CON4		0x850
#define SGMIIPLL_CON0		0x240
#define SGMIIPLL_CON1		0x244
#define SGMIIPLL_CON2		0x248
#define SGMIIPLL_CON3		0x24c
#define SGMIIPLL_CON4		0x250
#define APLL1_TUNER_CON0		0x0054
#define APLL2_TUNER_CON0		0x0058
#define AP_PLL_CON0		0x0

static DEFINE_SPINLOCK(mt6880_clk_lock);

static void __iomem *apmixed_base;

static const struct mtk_fixed_factor top_divs[] = {
	FACTOR(CLK_TOP_ARMPLL_LL_CK_VRPOC, "armpll_ll_vrpoc",
			"armpll_ll", 1, 1),
	FACTOR(CLK_TOP_CCIPLL_CK_VRPOC_CCI, "ccipll_vrpoc_cci",
			"ccipll", 1, 1),
	FACTOR(CLK_TOP_MFGPLL, "mfgpll_ck",
			"mfgpll", 1, 1),
	FACTOR(CLK_TOP_MAINPLL, "mainpll_ck",
			"mainpll", 1, 1),
	FACTOR(CLK_TOP_MAINPLL_D3, "mainpll_d3",
			"mainpll", 1, 3),
	FACTOR(CLK_TOP_MAINPLL_D4, "mainpll_d4",
			"mainpll", 1, 4),
	FACTOR(CLK_TOP_MAINPLL_D4_D2, "mainpll_d4_d2",
			"mainpll", 1, 8),
	FACTOR(CLK_TOP_MAINPLL_D4_D4, "mainpll_d4_d4",
			"mainpll", 1, 16),
	FACTOR(CLK_TOP_MAINPLL_D4_D8, "mainpll_d4_d8",
			"mainpll", 1, 32),
	FACTOR(CLK_TOP_MAINPLL_D4_D16, "mainpll_d4_d16",
			"mainpll", 1, 64),
	FACTOR(CLK_TOP_MAINPLL_D5, "mainpll_d5",
			"mainpll", 1, 5),
	FACTOR(CLK_TOP_MAINPLL_D5_D2, "mainpll_d5_d2",
			"mainpll", 1, 10),
	FACTOR(CLK_TOP_MAINPLL_D5_D4, "mainpll_d5_d4",
			"mainpll", 1, 20),
	FACTOR(CLK_TOP_MAINPLL_D5_D8, "mainpll_d5_d8",
			"mainpll", 1, 40),
	FACTOR(CLK_TOP_MAINPLL_D6, "mainpll_d6",
			"mainpll", 1, 6),
	FACTOR(CLK_TOP_MAINPLL_D6_D2, "mainpll_d6_d2",
			"mainpll", 1, 12),
	FACTOR(CLK_TOP_MAINPLL_D6_D4, "mainpll_d6_d4",
			"mainpll", 1, 24),
	FACTOR(CLK_TOP_MAINPLL_D6_D8, "mainpll_d6_d8",
			"mainpll", 1, 48),
	FACTOR(CLK_TOP_MAINPLL_D7, "mainpll_d7",
			"mainpll", 1, 7),
	FACTOR(CLK_TOP_MAINPLL_D7_D2, "mainpll_d7_d2",
			"mainpll", 1, 14),
	FACTOR(CLK_TOP_MAINPLL_D7_D4, "mainpll_d7_d4",
			"mainpll", 1, 28),
	FACTOR(CLK_TOP_MAINPLL_D7_D8, "mainpll_d7_d8",
			"mainpll", 1, 56),
	FACTOR(CLK_TOP_MAINPLL_D8, "mainpll_d8",
			"mainpll", 1, 8),
	FACTOR(CLK_TOP_MAINPLL_D9, "mainpll_d9",
			"mainpll", 1, 9),
	FACTOR(CLK_TOP_UNIVPLL, "univpll_ck",
			"univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2",
			"univpll", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3",
			"univpll", 1, 3),
	FACTOR(CLK_TOP_UNIVPLL_D4, "univpll_d4",
			"univpll", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_D4_D2, "univpll_d4_d2",
			"univpll", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL_D4_D4, "univpll_d4_d4",
			"univpll", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL_D4_D8, "univpll_d4_d8",
			"univpll", 1, 32),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5",
			"univpll", 1, 5),
	FACTOR(CLK_TOP_UNIVPLL_D5_D2, "univpll_d5_d2",
			"univpll", 1, 10),
	FACTOR(CLK_TOP_UNIVPLL_D5_D4, "univpll_d5_d4",
			"univpll", 1, 20),
	FACTOR(CLK_TOP_UNIVPLL_D5_D8, "univpll_d5_d8",
			"univpll", 1, 40),
	FACTOR(CLK_TOP_UNIVPLL_D5_D16, "univpll_d5_d16",
			"univpll", 1, 80),
	FACTOR(CLK_TOP_UNIVPLL_D6, "univpll_d6",
			"univpll", 1, 6),
	FACTOR(CLK_TOP_UNIVPLL_D6_D2, "univpll_d6_d2",
			"univpll", 1, 12),
	FACTOR(CLK_TOP_UNIVPLL_D6_D4, "univpll_d6_d4",
			"univpll", 1, 24),
	FACTOR(CLK_TOP_UNIVPLL_D6_D8, "univpll_d6_d8",
			"univpll", 1, 48),
	FACTOR(CLK_TOP_UNIVPLL_D6_D16, "univpll_d6_d16",
			"univpll", 1, 96),
	FACTOR(CLK_TOP_UNIVPLL_D7, "univpll_d7",
			"univpll", 1, 7),
	FACTOR(CLK_TOP_UNIVPLL_D7_D2, "univpll_d7_d2",
			"univpll", 1, 14),
	FACTOR(CLK_TOP_UNIVPLL_192M, "univpll_192m_ck",
			"univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_192M_D2, "univpll_192m_d2",
			"univpll", 1, 26),
	FACTOR(CLK_TOP_UNIVPLL_192M_D4, "univpll_192m_d4",
			"univpll", 1, 52),
	FACTOR(CLK_TOP_UNIVPLL_192M_D8, "univpll_192m_d8",
			"univpll", 1, 104),
	FACTOR(CLK_TOP_UNIVPLL_192M_D16, "univpll_192m_d16",
			"univpll", 1, 208),
	FACTOR(CLK_TOP_UNIVPLL_192M_D32, "univpll_192m_d32",
			"univpll", 1, 416),
	FACTOR(CLK_TOP_USB20_192M, "usb20_192m_ck",
			"univpll", 1, 13),
	FACTOR(CLK_TOP_USB20_PLL_D2, "usb20_pll_d2",
			"univpll", 1, 26),
	FACTOR(CLK_TOP_USB20_PLL_D4, "usb20_pll_d4",
			"univpll", 1, 52),
	FACTOR(CLK_TOP_APLL1, "apll1_ck",
			"apll1", 1, 1),
	FACTOR(CLK_TOP_APLL1_D2, "apll1_d2",
			"apll1", 1, 2),
	FACTOR(CLK_TOP_APLL1_D4, "apll1_d4",
			"apll1", 1, 4),
	FACTOR(CLK_TOP_APLL1_D8, "apll1_d8",
			"apll1", 1, 8),
	FACTOR(CLK_TOP_APLL2, "apll2_ck",
			"apll2", 1, 1),
	FACTOR(CLK_TOP_APLL2_D2, "apll2_d2",
			"apll2", 1, 2),
	FACTOR(CLK_TOP_APLL2_D4, "apll2_d4",
			"apll2", 1, 4),
	FACTOR(CLK_TOP_APLL2_D8, "apll2_d8",
			"apll2", 1, 8),
	FACTOR(CLK_TOP_MMPLL, "mmpll_ck",
			"mmpll", 1, 1),
	FACTOR(CLK_TOP_MMPLL_D3, "mmpll_d3",
			"mmpll", 1, 3),
	FACTOR(CLK_TOP_MMPLL_D4, "mmpll_d4",
			"mmpll", 1, 4),
	FACTOR(CLK_TOP_MMPLL_D4_D2, "mmpll_d4_d2",
			"mmpll", 1, 8),
	FACTOR(CLK_TOP_MMPLL_D4_D4, "mmpll_d4_d4",
			"mmpll", 1, 16),
	FACTOR(CLK_TOP_MMPLL_D5, "mmpll_d5",
			"mmpll", 1, 5),
	FACTOR(CLK_TOP_MMPLL_D5_D2, "mmpll_d5_d2",
			"mmpll", 1, 10),
	FACTOR(CLK_TOP_MMPLL_D5_D4, "mmpll_d5_d4",
			"mmpll", 1, 20),
	FACTOR(CLK_TOP_MMPLL_D6, "mmpll_d6",
			"mmpll", 1, 6),
	FACTOR(CLK_TOP_MMPLL_D6_D2, "mmpll_d6_d2",
			"mmpll", 1, 12),
	FACTOR(CLK_TOP_MMPLL_D7, "mmpll_d7",
			"mmpll", 1, 7),
	FACTOR(CLK_TOP_MMPLL_D9, "mmpll_d9",
			"mmpll", 1, 9),
	FACTOR(CLK_TOP_TVDPLL, "tvdpll_ck",
			"net1pll", 1, 1),
	FACTOR(CLK_TOP_TVDPLL_D2, "tvdpll_d2",
			"net1pll", 1, 2),
	FACTOR(CLK_TOP_TVDPLL_D4, "tvdpll_d4",
			"net1pll", 1, 4),
	FACTOR(CLK_TOP_TVDPLL_D8, "tvdpll_d8",
			"net1pll", 1, 8),
	FACTOR(CLK_TOP_TVDPLL_D16, "tvdpll_d16",
			"net1pll", 1, 16),
	FACTOR(CLK_TOP_MSDCPLL, "msdcpll_ck",
			"msdcpll", 1, 1),
	FACTOR(CLK_TOP_MSDCPLL_D2, "msdcpll_d2",
			"msdcpll", 1, 2),
	FACTOR(CLK_TOP_MSDCPLL_D4, "msdcpll_d4",
			"msdcpll", 1, 4),
	FACTOR(CLK_TOP_MSDCPLL_D8, "msdcpll_d8",
			"msdcpll", 1, 8),
	FACTOR(CLK_TOP_MSDCPLL_D16, "msdcpll_d16",
			"msdcpll", 1, 16),
	FACTOR(CLK_TOP_CLKRTC, "clkrtc",
			"clk32k", 1, 1),
	FACTOR(CLK_TOP_TCK_26M_MX8, "tck_26m_mx8_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_TCK_26M_MX9, "tck_26m_mx9_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_TCK_26M_MX10, "tck_26m_mx10_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_TCK_26M_MX11, "tck_26m_mx11_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_TCK_26M_MX12, "tck_26m_mx12_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_CSW_FAXI, "csw_faxi_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_CSW_F26M_CK_D52, "csw_f26m_d52",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_CSW_F26M_CK_D2, "csw_f26m_d2",
			"clk26m", 1, 2),
	FACTOR(CLK_TOP_OSC, "osc_ck",
			"ulposc", 1, 1),
	FACTOR(CLK_TOP_OSC_D2, "osc_d2",
			"ulposc", 1, 2),
	FACTOR(CLK_TOP_OSC_D4, "osc_d4",
			"ulposc", 1, 4),
	FACTOR(CLK_TOP_OSC_D8, "osc_d8",
			"ulposc", 1, 8),
	FACTOR(CLK_TOP_OSC_D16, "osc_d16",
			"ulposc", 1, 16),
	FACTOR(CLK_TOP_OSC_D10, "osc_d10",
			"ulposc", 1, 10),
	FACTOR(CLK_TOP_OSC_D20, "osc_d20",
			"ulposc", 1, 20),
	FACTOR(CLK_TOP_TVDPLL_D5, "tvdpll_d5",
			"net1pll", 1, 5),
	FACTOR(CLK_TOP_TVDPLL_D10, "tvdpll_d10",
			"net1pll", 1, 10),
	FACTOR(CLK_TOP_TVDPLL_D25, "tvdpll_d25",
			"net1pll", 1, 25),
	FACTOR(CLK_TOP_TVDPLL_D50, "tvdpll_d50",
			"net1pll", 1, 50),
	FACTOR(CLK_TOP_NET2PLL, "net2pll_ck",
			"net2pll", 1, 1),
	FACTOR(CLK_TOP_WEDMCUPLL, "wedmcupll_ck",
			"wedmcupll", 1, 1),
	FACTOR(CLK_TOP_MEDMCUPLL, "medmcupll_ck",
			"medmcupll", 1, 1),
	FACTOR(CLK_TOP_SGMIIPLL, "sgmiipll_ck",
			"sgmiipll", 1, 1),
	FACTOR(CLK_TOP_F26M, "f26m_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_FRTC, "frtc_ck",
			"clk32k", 1, 1),
	FACTOR(CLK_TOP_AXI, "axi_ck",
			"axi_sel", 1, 1),
	FACTOR(CLK_TOP_SPM, "spm_ck",
			"spm_sel", 1, 1),
	FACTOR(CLK_TOP_BUS, "bus_ck",
			"bus_aximem_sel", 1, 1),
	FACTOR(CLK_TOP_MM, "mm_ck",
			"mm_sel", 1, 1),
	FACTOR(CLK_TOP_MFG_REF, "mfg_ref_ck",
			"mfg_ref_sel", 1, 1),
	FACTOR(CLK_TOP_MFG, "mfg_ck",
			"mfg_sel", 1, 1),
	FACTOR(CLK_TOP_FUART, "fuart_ck",
			"uart_sel", 1, 1),
	FACTOR(CLK_TOP_MSDC50_0_HCLK, "msdc50_0_h_ck",
			"msdc50_0_h_sel", 1, 1),
	FACTOR(CLK_TOP_MSDC50_0, "msdc50_0_ck",
			"msdc50_0_sel", 1, 1),
	FACTOR(CLK_TOP_MSDC30_1, "msdc30_1_ck",
			"msdc30_1_sel", 1, 1),
	FACTOR(CLK_TOP_AUDIO, "audio_ck",
			"audio_sel", 1, 1),
	FACTOR(CLK_TOP_AUD_INTBUS, "aud_intbus_ck",
			"aud_intbus_sel", 1, 1),
	FACTOR(CLK_TOP_AUD_ENGEN1, "aud_engen1_ck",
			"aud_engen1_sel", 1, 1),
	FACTOR(CLK_TOP_AUD_ENGEN2, "aud_engen2_ck",
			"aud_engen2_sel", 1, 1),
	FACTOR(CLK_TOP_AUD_1, "aud_1_ck",
			"aud_1_sel", 1, 1),
	FACTOR(CLK_TOP_AUD_2, "aud_2_ck",
			"aud_2_sel", 1, 1),
	FACTOR(CLK_TOP_FPWRAP_ULPOSC, "fpwrap_ulposc_ck",
			"pwrap_ulposc_sel", 1, 1),
	FACTOR(CLK_TOP_ATB, "atb_ck",
			"atb_sel", 1, 1),
	FACTOR(CLK_TOP_PWRMCU, "pwrmcu_ck",
			"pwrmcu_sel", 1, 1),
	FACTOR(CLK_TOP_DBI, "dbi_ck",
			"dbi_sel", 1, 1),
	FACTOR(CLK_TOP_FDISP_PWM, "fdisp_pwm_ck",
			"disp_pwm_sel", 1, 1),
	FACTOR(CLK_TOP_FUSB_TOP, "fusb_ck",
			"usb_sel", 1, 1),
	FACTOR(CLK_TOP_FSSUSB_XHCI, "fssusb_xhci_ck",
			"ssusb_xhci_sel", 1, 1),
	FACTOR(CLK_TOP_I2C, "i2c_ck",
			"i2c_sel", 1, 1),
	FACTOR(CLK_TOP_TL, "tl_ck",
			"tl_sel", 1, 1),
	FACTOR(CLK_TOP_DPMAIF_MAIN, "dpmaif_main_ck",
			"dpmaif_main_sel", 1, 1),
	FACTOR(CLK_TOP_PWM, "pwm_ck",
			"pwm_sel", 1, 1),
	FACTOR(CLK_TOP_SPMI_M_MST, "spmi_m_mst_ck",
			"spmi_m_mst_sel", 1, 1),
	FACTOR(CLK_TOP_SPMI_P_MST, "spmi_p_mst_ck",
			"spmi_p_mst_sel", 1, 1),
	FACTOR(CLK_TOP_DVFSRC, "dvfsrc_ck",
			"dvfsrc_sel", 1, 1),
	FACTOR(CLK_TOP_MCUPM, "mcupm_ck",
			"mcupm_sel", 1, 1),
	FACTOR(CLK_TOP_SFLASH, "sflash_ck",
			"sflash_sel", 1, 1),
	FACTOR(CLK_TOP_GCPU, "gcpu_ck",
			"gcpu_sel", 1, 1),
	FACTOR(CLK_TOP_SPI, "spi_ck",
			"spi_sel", 1, 1),
	FACTOR(CLK_TOP_SPIS, "spis_ck",
			"spis_sel", 1, 1),
	FACTOR(CLK_TOP_ECC, "ecc_ck",
			"ecc_sel", 1, 1),
	FACTOR(CLK_TOP_NFI1X, "nfi1x_ck",
			"nfi1x_sel", 1, 1),
	FACTOR(CLK_TOP_SPINFI_BCLK, "spinfi_bclk_ck",
			"spinfi_bclk_sel", 1, 1),
	FACTOR(CLK_TOP_NETSYS, "netsys_ck",
			"netsys_sel", 1, 1),
	FACTOR(CLK_TOP_MEDSYS, "medsys_ck",
			"medsys_sel", 1, 1),
	FACTOR(CLK_TOP_HSM_CRYPTO, "hsm_crypto_ck",
			"hsm_crypto_sel", 1, 1),
	FACTOR(CLK_TOP_HSM_ARC, "hsm_arc_ck",
			"hsm_arc_sel", 1, 1),
	FACTOR(CLK_TOP_EIP97, "eip97_ck",
			"eip97_sel", 1, 1),
	FACTOR(CLK_TOP_SNPS_ETH_312P5M, "snps_eth_312p5m_ck",
			"snps_eth_312p5m_sel", 1, 1),
	FACTOR(CLK_TOP_SNPS_ETH_250M, "snps_eth_250m_ck",
			"snps_eth_250m_sel", 1, 1),
	FACTOR(CLK_TOP_SNPS_ETH_62P4M_PTP, "snps_ptp_ck",
			"snps_ptp_sel", 1, 1),
	FACTOR(CLK_TOP_SNPS_ETH_50M_RMII, "snps_eth_50m_rmii_ck",
			"snps_rmii_sel", 1, 1),
	FACTOR(CLK_TOP_NETSYS_500M, "netsys_500m_ck",
			"netsys_500m_sel", 1, 1),
	FACTOR(CLK_TOP_NETSYS_MED_MCU, "netsys_med_mcu_ck",
			"netsys_med_mcu_sel", 1, 1),
	FACTOR(CLK_TOP_NETSYS_WED_MCU, "netsys_wed_mcu_ck",
			"netsys_wed_mcu_sel", 1, 1),
	FACTOR(CLK_TOP_NETSYS_2X, "netsys_2x_ck",
			"netsys_2x_sel", 1, 1),
	FACTOR(CLK_TOP_SGMII, "sgmii_ck",
			"sgmii_sel", 1, 1),
	FACTOR(CLK_TOP_SGMII_SBUS, "sgmii_sbus_ck",
			"sgmii_sbus_sel", 1, 1),
	FACTOR(CLK_TOP_SYS_26M, "sys_26m_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_F_UFS_MP_SAP_CFG, "ufs_cfg_ck",
			"clk26m", 1, 1),
	FACTOR(CLK_TOP_F_UFS_TICK1US, "f_ufs_tick1us_ck",
			"clk26m", 1, 1),
};

static const char * const axi_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d4",
	"mainpll_d7_d2",
	"mainpll_d4_d2",
	"mainpll_d5_d2",
	"mainpll_d6_d2",
	"osc_d4"
};

static const char * const spm_parents[] = {
	"tck_26m_mx9_ck",
	"osc_d10",
	"mainpll_d7_d4",
	"clkrtc"
};

static const char * const bus_aximem_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d7_d2",
	"mainpll_d4_d2",
	"mainpll_d5_d2",
	"mainpll_d6"
};

static const char * const mm_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d2",
	"univpll_d7_d2",
	"mainpll_d6_d2",
	"univpll_d4_d4"
};

static const char * const mfg_ref_parents[] = {
	"tck_26m_mx9_ck",
	"tck_26m_mx9_ck",
	"univpll_d6",
	"mainpll_d5_d2"
};

static const char * const mfg_parents[] = {
	"mfg_ref_sel",
	"mfgpll_ck"
};

static const char * const uart_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d8"
};

static const char * const msdc50_0_h_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d2",
	"mainpll_d6_d2"
};

static const char * const msdc50_0_parents[] = {
	"tck_26m_mx9_ck",
	"msdcpll_ck",
	"msdcpll_d2",
	"univpll_d4_d4",
	"mainpll_d6_d2",
	"univpll_d4_d2"
};

static const char * const msdc30_1_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d2",
	"mainpll_d6_d2",
	"mainpll_d7_d2",
	"msdcpll_d2"
};

static const char * const audio_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d5_d8",
	"mainpll_d7_d8",
	"mainpll_d4_d16"
};

static const char * const aud_intbus_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d4",
	"mainpll_d7_d4"
};

static const char * const aud_engen1_parents[] = {
	"tck_26m_mx9_ck",
	"apll1_d2",
	"apll1_d4",
	"apll1_d8"
};

static const char * const aud_engen2_parents[] = {
	"tck_26m_mx9_ck",
	"apll2_d2",
	"apll2_d4",
	"apll2_d8"
};

static const char * const aud_1_parents[] = {
	"tck_26m_mx9_ck",
	"apll1_ck"
};

static const char * const aud_2_parents[] = {
	"tck_26m_mx9_ck",
	"apll2_ck"
};

static const char * const pwrap_ulposc_parents[] = {
	"osc_d10",
	"tck_26m_mx9_ck",
	"osc_d4",
	"osc_d8",
	"osc_d16"
};

static const char * const atb_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d2",
	"mainpll_d5_d2"
};

static const char * const pwrmcu_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d5_d2",
	"univpll_d5_d2",
	"mainpll_d4_d2",
	"univpll_d4_d2",
	"mainpll_d6"
};

static const char * const dbi_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d4_d8",
	"univpll_d6_d8"
};

static const char * const disp_pwm_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d4",
	"osc_d2",
	"osc_d4",
	"osc_d16"
};

static const char * const usb_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const ssusb_xhci_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const i2c_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d8",
	"univpll_d5_d4"
};

static const char * const tl_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d4",
	"mainpll_d6_d4"
};

static const char * const dpmaif_main_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d4_d4",
	"mainpll_d6",
	"mainpll_d4_d2",
	"univpll_d4_d2"
};

static const char * const pwm_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d4_d8"
};

static const char * const spmi_m_mst_parents[] = {
	"tck_26m_mx9_ck",
	"csw_f26m_d2",
	"osc_d8",
	"osc_d10",
	"osc_d16",
	"osc_d20",
	"clkrtc"
};

static const char * const spmi_p_mst_parents[] = {
	"tck_26m_mx9_ck",
	"csw_f26m_d2",
	"osc_d8",
	"osc_d10",
	"osc_d16",
	"osc_d20",
	"clkrtc",
	"mainpll_d7_d8",
	"mainpll_d5_d8"
};

static const char * const dvfsrc_parents[] = {
	"tck_26m_mx9_ck",
	"osc_d10"
};

static const char * const mcupm_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d6_d4",
	"mainpll_d6_d2"
};

static const char * const sflash_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d7_d8",
	"univpll_d6_d8",
	"univpll_d5_d8"
};

static const char * const gcpu_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6",
	"mainpll_d6",
	"univpll_d4_d2",
	"mainpll_d4_d2",
	"univpll_d6_d2"
};

static const char * const spi_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d8",
	"univpll_d4_d8",
	"univpll_d6_d4",
	"univpll_d5_d4",
	"univpll_d4_d4",
	"univpll_d7_d2",
	"univpll_d6_d2"
};

static const char * const spis_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d8",
	"univpll_d4_d8",
	"univpll_d6_d4",
	"univpll_d4_d4",
	"univpll_d6_d2",
	"univpll_d4_d2",
	"univpll_d6"
};

static const char * const ecc_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d4",
	"mainpll_d9",
	"univpll_d4_d2"
};

static const char * const nfi1x_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d5_d4",
	"mainpll_d7_d4",
	"mainpll_d6_d4",
	"univpll_d6_d4",
	"mainpll_d4_d4",
	"univpll_d4_d4",
	"mainpll_d6_d2"
};

static const char * const spinfi_bclk_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d8",
	"univpll_d5_d8",
	"mainpll_d4_d8",
	"univpll_d4_d8",
	"mainpll_d6_d4",
	"univpll_d6_d4",
	"univpll_d5_d4"
};

static const char * const netsys_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d4_d8",
	"mainpll_d7_d2",
	"mainpll_d9",
	"univpll_d7"
};

static const char * const medsys_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d4_d8",
	"mainpll_d7_d2",
	"mainpll_d9",
	"univpll_d7"
};

static const char * const hsm_crypto_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d2",
	"mainpll_d6_d2",
	"mainpll_d7"
};

static const char * const hsm_arc_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d4_d8",
	"mainpll_d4_d4",
	"mainpll_d6_d2"
};

static const char * const eip97_parents[] = {
	"tck_26m_mx9_ck",
	"net2pll_ck",
	"mainpll_d3",
	"univpll_d4",
	"mainpll_d4",
	"univpll_d5",
	"mainpll_d6",
	"mainpll_d5_d2"
};

static const char * const snps_eth_312p5m_parents[] = {
	"tck_26m_mx9_ck",
	"tvdpll_d8"
};

static const char * const snps_eth_250m_parents[] = {
	"tck_26m_mx9_ck",
	"tvdpll_d10"
};

static const char * const snps_ptp_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d5_d8"
};

static const char * const snps_rmii_parents[] = {
	"tck_26m_mx9_ck",
	"tvdpll_d50"
};

static const char * const netsys_500m_parents[] = {
	"tck_26m_mx9_ck",
	"tvdpll_d5"
};

static const char * const netsys_med_mcu_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d6_d4",
	"mainpll_d4_d2",
	"univpll_d7",
	"medmcupll_ck"
};

static const char * const netsys_wed_mcu_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d6_d2",
	"mainpll_d6",
	"mainpll_d5",
	"wedmcupll_ck"
};

static const char * const netsys_2x_parents[] = {
	"tck_26m_mx9_ck",
	"univpll_d5_d4",
	"mainpll_d4_d2",
	"mainpll_d4",
	"net2pll_ck"
};

static const char * const sgmii_parents[] = {
	"tck_26m_mx9_ck",
	"sgmiipll_ck"
};

static const char * const sgmii_sbus_parents[] = {
	"tck_26m_mx9_ck",
	"mainpll_d7_d4"
};

static const char * const apll_i2s0_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const char * const apll_i2s1_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const char * const apll_i2s2_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const char * const apll_i2s4_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const char * const apll_tdmout_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const char * const apll_i2s5_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const char * const apll_i2s6_mck_parents[] = {
	"aud_1_sel",
	"aud_2_sel"
};

static const struct mtk_mux top_muxes[] = {
#if MT_CCF_MUX_DISABLE
	/* CLK_CFG_0 */
	MUX_CLR_SET_UPD(CLK_TOP_AXI_SEL/* dts */, "axi_sel",
		axi_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AXI_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPM_SEL/* dts */, "spm_sel",
		spm_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 8/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SPM_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_BUS_AXIMEM_SEL/* dts */, "bus_aximem_sel",
		bus_aximem_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 16/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_BUS_AXIMEM_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MM_SEL/* dts */, "mm_sel",
		mm_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MM_SHIFT/* upd shift */),
	/* CLK_CFG_1 */
	MUX_CLR_SET_UPD(CLK_TOP_MFG_REF_SEL/* dts */, "mfg_ref_sel",
		mfg_ref_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 0/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MFG_REF_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MFG_SEL/* dts */, "mfg_sel",
		mfg_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 2/* lsb */, 1/* width */,
		INV_OFS/* upd ofs */,
		INV_BIT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_UART_SEL/* dts */, "uart_sel",
		uart_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 8/* lsb */, 1/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_UART_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC50_0_HCLK_SEL/* dts */, "msdc50_0_h_sel",
		msdc50_0_h_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 16/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MSDC50_0_HCLK_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC50_0_SEL/* dts */, "msdc50_0_sel",
		msdc50_0_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MSDC50_0_SHIFT/* upd shift */),
	/* CLK_CFG_2 */
	MUX_CLR_SET_UPD(CLK_TOP_MSDC30_1_SEL/* dts */, "msdc30_1_sel",
		msdc30_1_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MSDC30_1_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_AUDIO_SEL/* dts */, "audio_sel",
		audio_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 8/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUDIO_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_AUD_INTBUS_SEL/* dts */, "aud_intbus_sel",
		aud_intbus_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 16/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_INTBUS_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_AUD_ENGEN1_SEL/* dts */, "aud_engen1_sel",
		aud_engen1_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 24/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_ENGEN1_SHIFT/* upd shift */),
	/* CLK_CFG_3 */
	MUX_CLR_SET_UPD(CLK_TOP_AUD_ENGEN2_SEL/* dts */, "aud_engen2_sel",
		aud_engen2_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 0/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_ENGEN2_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_AUD_1_SEL/* dts */, "aud_1_sel",
		aud_1_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 8/* lsb */, 1/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_1_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_AUD_2_SEL/* dts */, "aud_2_sel",
		aud_2_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 16/* lsb */, 1/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_2_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_PWRAP_ULPOSC_SEL/* dts */, "pwrap_ulposc_sel",
		pwrap_ulposc_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_PWRAP_ULPOSC_SHIFT/* upd shift */),
	/* CLK_CFG_4 */
	MUX_CLR_SET_UPD(CLK_TOP_ATB_SEL/* dts */, "atb_sel",
		atb_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 0/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_ATB_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_PWRMCU_SEL/* dts */, "pwrmcu_sel",
		pwrmcu_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 8/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_PWRMCU_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_DBI_SEL/* dts */, "dbi_sel",
		dbi_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 16/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DBI_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_DISP_PWM_SEL/* dts */, "disp_pwm_sel",
		disp_pwm_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DISP_PWM_SHIFT/* upd shift */),
	/* CLK_CFG_5 */
	MUX_CLR_SET_UPD(CLK_TOP_USB_TOP_SEL/* dts */, "usb_sel",
		usb_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 0/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_USB_TOP_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SSUSB_XHCI_SEL/* dts */, "ssusb_xhci_sel",
		ssusb_xhci_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 8/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SSUSB_XHCI_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_I2C_SEL/* dts */, "i2c_sel",
		i2c_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 16/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_I2C_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_TL_SEL/* dts */, "tl_sel",
		tl_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 24/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_TL_SHIFT/* upd shift */),
	/* CLK_CFG_6 */
	MUX_CLR_SET_UPD(CLK_TOP_DPMAIF_MAIN_SEL/* dts */, "dpmaif_main_sel",
		dpmaif_main_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DPMAIF_MAIN_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_PWM_SEL/* dts */, "pwm_sel",
		pwm_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 8/* lsb */, 1/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_PWM_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPMI_M_MST_SEL/* dts */, "spmi_m_mst_sel",
		spmi_m_mst_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 16/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SPMI_M_MST_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPMI_P_MST_SEL/* dts */, "spmi_p_mst_sel",
		spmi_p_mst_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 24/* lsb */, 4/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SPMI_P_MST_SHIFT/* upd shift */),
	/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(CLK_TOP_DVFSRC_SEL/* dts */, "dvfsrc_sel",
		dvfsrc_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 0/* lsb */, 1/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DVFSRC_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MCUPM_SEL/* dts */, "mcupm_sel",
		mcupm_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 8/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MCUPM_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SFLASH_SEL/* dts */, "sflash_sel",
		sflash_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 16/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SFLASH_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_GCPU_SEL/* dts */, "gcpu_sel",
		gcpu_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_GCPU_SHIFT/* upd shift */),
	/* CLK_CFG_8 */
	MUX_CLR_SET_UPD(CLK_TOP_SPI_SEL/* dts */, "spi_sel",
		spi_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SPI_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPIS_SEL/* dts */, "spis_sel",
		spis_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 8/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SPIS_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_ECC_SEL/* dts */, "ecc_sel",
		ecc_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 16/* lsb */, 2/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_ECC_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_NFI1X_SEL/* dts */, "nfi1x_sel",
		nfi1x_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NFI1X_SHIFT/* upd shift */),
	/* CLK_CFG_9 */
	MUX_CLR_SET_UPD(CLK_TOP_SPINFI_BCLK_SEL/* dts */, "spinfi_bclk_sel",
		spinfi_bclk_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SPINFI_BCLK_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_NETSYS_SEL/* dts */, "netsys_sel",
		netsys_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 8/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MEDSYS_SEL/* dts */, "medsys_sel",
		medsys_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 16/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_MEDSYS_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_HSM_CRYPTO_SEL/* dts */, "hsm_crypto_sel",
		hsm_crypto_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 24/* lsb */, 2/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_HSM_CRYPTO_SHIFT/* upd shift */),
	/* CLK_CFG_10 */
	MUX_CLR_SET_UPD(CLK_TOP_HSM_ARC_SEL/* dts */, "hsm_arc_sel",
		hsm_arc_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 0/* lsb */, 2/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_HSM_ARC_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_EIP97_SEL/* dts */, "eip97_sel",
		eip97_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 8/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_EIP97_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SNPS_ETH_312P5M_SEL/* dts */, "snps_eth_312p5m_sel",
		snps_eth_312p5m_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 16/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_312P5M_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SNPS_ETH_250M_SEL/* dts */, "snps_eth_250m_sel",
		snps_eth_250m_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 24/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_250M_SHIFT/* upd shift */),
	/* CLK_CFG_11 */
	MUX_CLR_SET_UPD(CLK_TOP_SNPS_ETH_62P4M_PTP_SEL/* dts */, "snps_ptp_sel",
		snps_ptp_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 0/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_62P4M_PTP_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SNPS_ETH_50M_RMII_SEL/* dts */, "snps_rmii_sel",
		snps_rmii_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 8/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_50M_RMII_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_NETSYS_500M_SEL/* dts */, "netsys_500m_sel",
		netsys_500m_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 16/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_500M_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_NETSYS_MED_MCU_SEL/* dts */, "netsys_med_mcu_sel",
		netsys_med_mcu_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 24/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_MED_MCU_SHIFT/* upd shift */),
	/* CLK_CFG_12 */
	MUX_CLR_SET_UPD(CLK_TOP_NETSYS_WED_MCU_SEL/* dts */, "netsys_wed_mcu_sel",
		netsys_wed_mcu_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_WED_MCU_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_NETSYS_2X_SEL/* dts */, "netsys_2x_sel",
		netsys_2x_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 8/* lsb */, 3/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_2X_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SGMII_SEL/* dts */, "sgmii_sel",
		sgmii_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 16/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SGMII_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SGMII_SBUS_SEL/* dts */, "sgmii_sbus_sel",
		sgmii_sbus_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 24/* lsb */, 1/* width */,
		CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SGMII_SBUS_SHIFT/* upd shift */),
#else
	/* CLK_CFG_0 */
	MUX_CLR_SET_UPD(CLK_TOP_AXI_SEL/* dts */, "axi_sel",
		axi_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 0/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AXI_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPM_SEL/* dts */, "spm_sel",
		spm_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 8/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SPM_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_BUS_AXIMEM_SEL/* dts */, "bus_aximem_sel",
		bus_aximem_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 16/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_BUS_AXIMEM_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MM_SEL/* dts */, "mm_sel",
		mm_parents/* parent */, CLK_CFG_0, CLK_CFG_0_SET,
		CLK_CFG_0_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MM_SHIFT/* upd shift */),
	/* CLK_CFG_1 */
	MUX_CLR_SET_UPD(CLK_TOP_MFG_REF_SEL/* dts */, "mfg_ref_sel",
		mfg_ref_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 0/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MFG_REF_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MFG_SEL/* dts */, "mfg_sel",
		mfg_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 2/* lsb */, 1/* width */,
		INV_OFS/* upd ofs */,
		INV_BIT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_UART_SEL/* dts */, "uart_sel",
		uart_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 8/* lsb */, 1/* width */,
		15/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_UART_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC50_0_HCLK_SEL/* dts */, "msdc50_0_h_sel",
		msdc50_0_h_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 16/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MSDC50_0_HCLK_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MSDC50_0_SEL/* dts */, "msdc50_0_sel",
		msdc50_0_parents/* parent */, CLK_CFG_1, CLK_CFG_1_SET,
		CLK_CFG_1_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MSDC50_0_SHIFT/* upd shift */),
	/* CLK_CFG_2 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MSDC30_1_SEL/* dts */, "msdc30_1_sel",
		msdc30_1_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 0/* lsb */, 3/* width */,
		7/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MSDC30_1_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUDIO_SEL/* dts */, "audio_sel",
		audio_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 8/* lsb */, 2/* width */,
		15/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUDIO_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_INTBUS_SEL/* dts */, "aud_intbus_sel",
		aud_intbus_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 16/* lsb */, 2/* width */,
		23/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_INTBUS_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_ENGEN1_SEL/* dts */, "aud_engen1_sel",
		aud_engen1_parents/* parent */, CLK_CFG_2, CLK_CFG_2_SET,
		CLK_CFG_2_CLR/* set parent */, 24/* lsb */, 2/* width */,
		31/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_ENGEN1_SHIFT/* upd shift */),
	/* CLK_CFG_3 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_ENGEN2_SEL/* dts */, "aud_engen2_sel",
		aud_engen2_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 0/* lsb */, 2/* width */,
		7/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_ENGEN2_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_1_SEL/* dts */, "aud_1_sel",
		aud_1_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 8/* lsb */, 1/* width */,
		15/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_1_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_2_SEL/* dts */, "aud_2_sel",
		aud_2_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 16/* lsb */, 1/* width */,
		23/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_AUD_2_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_PWRAP_ULPOSC_SEL/* dts */, "pwrap_ulposc_sel",
		pwrap_ulposc_parents/* parent */, CLK_CFG_3, CLK_CFG_3_SET,
		CLK_CFG_3_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_PWRAP_ULPOSC_SHIFT/* upd shift */),
	/* CLK_CFG_4 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ATB_SEL/* dts */, "atb_sel",
		atb_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 0/* lsb */, 2/* width */,
		7/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_ATB_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_PWRMCU_SEL/* dts */, "pwrmcu_sel",
		pwrmcu_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 8/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_PWRMCU_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DBI_SEL/* dts */, "dbi_sel",
		dbi_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 16/* lsb */, 3/* width */,
		23/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DBI_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DISP_PWM_SEL/* dts */, "disp_pwm_sel",
		disp_pwm_parents/* parent */, CLK_CFG_4, CLK_CFG_4_SET,
		CLK_CFG_4_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DISP_PWM_SHIFT/* upd shift */),
	/* CLK_CFG_5 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_USB_TOP_SEL/* dts */, "usb_sel",
		usb_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 0/* lsb */, 2/* width */,
		7/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_USB_TOP_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SSUSB_XHCI_SEL/* dts */, "ssusb_xhci_sel",
		ssusb_xhci_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 8/* lsb */, 2/* width */,
		15/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SSUSB_XHCI_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_I2C_SEL/* dts */, "i2c_sel",
		i2c_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 16/* lsb */, 2/* width */,
		23/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_I2C_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_TL_SEL/* dts */, "tl_sel",
		tl_parents/* parent */, CLK_CFG_5, CLK_CFG_5_SET,
		CLK_CFG_5_CLR/* set parent */, 24/* lsb */, 2/* width */,
		31/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_TL_SHIFT/* upd shift */),
	/* CLK_CFG_6 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DPMAIF_MAIN_SEL/* dts */, "dpmaif_main_sel",
		dpmaif_main_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 0/* lsb */, 3/* width */,
		7/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DPMAIF_MAIN_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_PWM_SEL/* dts */, "pwm_sel",
		pwm_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 8/* lsb */, 1/* width */,
		15/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_PWM_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPMI_M_MST_SEL/* dts */, "spmi_m_mst_sel",
		spmi_m_mst_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 16/* lsb */, 3/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SPMI_M_MST_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_SPMI_P_MST_SEL/* dts */, "spmi_p_mst_sel",
		spmi_p_mst_parents/* parent */, CLK_CFG_6, CLK_CFG_6_SET,
		CLK_CFG_6_CLR/* set parent */, 24/* lsb */, 4/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SPMI_P_MST_SHIFT/* upd shift */),
	/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(CLK_TOP_DVFSRC_SEL/* dts */, "dvfsrc_sel",
		dvfsrc_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 0/* lsb */, 1/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_DVFSRC_SHIFT/* upd shift */),
	MUX_CLR_SET_UPD(CLK_TOP_MCUPM_SEL/* dts */, "mcupm_sel",
		mcupm_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 8/* lsb */, 2/* width */,
		CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_MCUPM_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SFLASH_SEL/* dts */, "sflash_sel",
		sflash_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 16/* lsb */, 2/* width */,
		23/* pdn */, CLK_CFG_UPDATE/* upd ofs */,
		TOP_MUX_SFLASH_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_GCPU_SEL/* dts */, "gcpu_sel",
		gcpu_parents/* parent */, CLK_CFG_7, CLK_CFG_7_SET,
		CLK_CFG_7_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_GCPU_SHIFT/* upd shift */),
	/* CLK_CFG_8 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPI_SEL/* dts */, "spi_sel",
		spi_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 0/* lsb */, 3/* width */,
		7/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SPI_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPIS_SEL/* dts */, "spis_sel",
		spis_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 8/* lsb */, 3/* width */,
		15/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SPIS_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ECC_SEL/* dts */, "ecc_sel",
		ecc_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 16/* lsb */, 2/* width */,
		23/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_ECC_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_NFI1X_SEL/* dts */, "nfi1x_sel",
		nfi1x_parents/* parent */, CLK_CFG_8, CLK_CFG_8_SET,
		CLK_CFG_8_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NFI1X_SHIFT/* upd shift */),
	/* CLK_CFG_9 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPINFI_BCLK_SEL/* dts */, "spinfi_bclk_sel",
		spinfi_bclk_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 0/* lsb */, 3/* width */,
		7/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SPINFI_BCLK_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_NETSYS_SEL/* dts */, "netsys_sel",
		netsys_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 8/* lsb */, 3/* width */,
		15/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MEDSYS_SEL/* dts */, "medsys_sel",
		medsys_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 16/* lsb */, 3/* width */,
		23/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_MEDSYS_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_HSM_CRYPTO_SEL/* dts */, "hsm_crypto_sel",
		hsm_crypto_parents/* parent */, CLK_CFG_9, CLK_CFG_9_SET,
		CLK_CFG_9_CLR/* set parent */, 24/* lsb */, 2/* width */,
		31/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_HSM_CRYPTO_SHIFT/* upd shift */),
	/* CLK_CFG_10 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_HSM_ARC_SEL/* dts */, "hsm_arc_sel",
		hsm_arc_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 0/* lsb */, 2/* width */,
		7/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_HSM_ARC_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_EIP97_SEL/* dts */, "eip97_sel",
		eip97_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 8/* lsb */, 3/* width */,
		15/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_EIP97_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_312P5M_SEL/* dts */, "snps_eth_312p5m_sel",
		snps_eth_312p5m_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 16/* lsb */, 1/* width */,
		23/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_312P5M_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_250M_SEL/* dts */, "snps_eth_250m_sel",
		snps_eth_250m_parents/* parent */, CLK_CFG_10, CLK_CFG_10_SET,
		CLK_CFG_10_CLR/* set parent */, 24/* lsb */, 1/* width */,
		31/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_250M_SHIFT/* upd shift */),
	/* CLK_CFG_11 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_62P4M_PTP_SEL/* dts */, "snps_ptp_sel",
		snps_ptp_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 0/* lsb */, 1/* width */,
		7/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_62P4M_PTP_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_50M_RMII_SEL/* dts */, "snps_rmii_sel",
		snps_rmii_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 8/* lsb */, 1/* width */,
		15/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SNPS_ETH_50M_RMII_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_NETSYS_500M_SEL/* dts */, "netsys_500m_sel",
		netsys_500m_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 16/* lsb */, 1/* width */,
		23/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_500M_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_NETSYS_MED_MCU_SEL/* dts */, "netsys_med_mcu_sel",
		netsys_med_mcu_parents/* parent */, CLK_CFG_11, CLK_CFG_11_SET,
		CLK_CFG_11_CLR/* set parent */, 24/* lsb */, 3/* width */,
		31/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_MED_MCU_SHIFT/* upd shift */),
	/* CLK_CFG_12 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_NETSYS_WED_MCU_SEL/* dts */, "netsys_wed_mcu_sel",
		netsys_wed_mcu_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 0/* lsb */, 3/* width */,
		7/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_WED_MCU_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_NETSYS_2X_SEL/* dts */, "netsys_2x_sel",
		netsys_2x_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 8/* lsb */, 3/* width */,
		15/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_NETSYS_2X_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SGMII_SEL/* dts */, "sgmii_sel",
		sgmii_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 16/* lsb */, 1/* width */,
		23/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SGMII_SHIFT/* upd shift */),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SGMII_SBUS_SEL/* dts */, "sgmii_sbus_sel",
		sgmii_sbus_parents/* parent */, CLK_CFG_12, CLK_CFG_12_SET,
		CLK_CFG_12_CLR/* set parent */, 24/* lsb */, 1/* width */,
		31/* pdn */, CLK_CFG_UPDATE1/* upd ofs */,
		TOP_MUX_SGMII_SBUS_SHIFT/* upd shift */),
#endif /* MT_CCF_MUX_DISABLE */
};

static const struct mtk_composite top_composites[] = {
	/* CLK_AUDDIV_0 */
	MUX(CLK_TOP_APLL_I2S0_MCK_SEL/* dts */, "apll_i2s0_mck_sel",
		apll_i2s0_mck_parents/* parent */, 0x0320/* ofs */,
		16/* lsb */, 1/* width */),
	MUX(CLK_TOP_APLL_I2S1_MCK_SEL/* dts */, "apll_i2s1_mck_sel",
		apll_i2s1_mck_parents/* parent */, 0x0320/* ofs */,
		17/* lsb */, 1/* width */),
	MUX(CLK_TOP_APLL_I2S2_MCK_SEL/* dts */, "apll_i2s2_mck_sel",
		apll_i2s2_mck_parents/* parent */, 0x0320/* ofs */,
		18/* lsb */, 1/* width */),
	MUX(CLK_TOP_APLL_I2S4_MCK_SEL/* dts */, "apll_i2s4_mck_sel",
		apll_i2s4_mck_parents/* parent */, 0x0320/* ofs */,
		19/* lsb */, 1/* width */),
	MUX(CLK_TOP_APLL_TDMOUT_MCK_SEL/* dts */, "apll_tdmout_mck_sel",
		apll_tdmout_mck_parents/* parent */, 0x0320/* ofs */,
		20/* lsb */, 1/* width */),
	MUX(CLK_TOP_APLL_I2S5_MCK_SEL/* dts */, "apll_i2s5_mck_sel",
		apll_i2s5_mck_parents/* parent */, 0x0320/* ofs */,
		21/* lsb */, 1/* width */),
	MUX(CLK_TOP_APLL_I2S6_MCK_SEL/* dts */, "apll_i2s6_mck_sel",
		apll_i2s6_mck_parents/* parent */, 0x0320/* ofs */,
		22/* lsb */, 1/* width */),
#if MT_CCF_MUX_DISABLE
	/* CLK_AUDDIV_2 */
	DIV_GATE(CLK_TOP_APLL12_CK_DIV0/* dts */, "apll12_div0"/* ccf */,
		"apll_i2s0_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_2/* ofs */, 0/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV1/* dts */, "apll12_div1"/* ccf */,
		"apll_i2s1_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_2/* ofs */, 8/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV2/* dts */, "apll12_div2"/* ccf */,
		"apll_i2s2_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_2/* ofs */, 16/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV4/* dts */, "apll12_div4"/* ccf */,
		"apll_i2s4_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_2/* ofs */, 24/* lsb */,
		8/* width */),
	/* CLK_AUDDIV_3 */
	DIV_GATE(CLK_TOP_APLL12_CK_DIV_TDMOUT_M/* dts */, "apll12_div_tdmout_m"/* ccf */,
		"apll_tdmout_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_3/* ofs */, 0/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV_TDMOUT_B/* dts */, "apll12_div_tdmout_b"/* ccf */,
		"apll_tdmout_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_3/* ofs */, 8/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV5/* dts */, "apll12_div5"/* ccf */,
		"apll_i2s5_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_3/* ofs */, 16/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV6/* dts */, "apll12_div6"/* ccf */,
		"apll_i2s6_mck_sel"/* parent */, INV_OFS/* pdn ofs */,
		INV_BIT/* pdn bit */, CLK_AUDDIV_3/* ofs */, 24/* lsb */,
		8/* width */),
#else
	/* CLK_AUDDIV_2 */
	DIV_GATE(CLK_TOP_APLL12_CK_DIV0/* dts */, "apll12_div0"/* ccf */,
		"apll_i2s0_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		0/* pdn bit */, CLK_AUDDIV_2/* ofs */, 0/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV1/* dts */, "apll12_div1"/* ccf */,
		"apll_i2s1_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		1/* pdn bit */, CLK_AUDDIV_2/* ofs */, 8/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV2/* dts */, "apll12_div2"/* ccf */,
		"apll_i2s2_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		2/* pdn bit */, CLK_AUDDIV_2/* ofs */, 16/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV4/* dts */, "apll12_div4"/* ccf */,
		"apll_i2s4_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		3/* pdn bit */, CLK_AUDDIV_2/* ofs */, 24/* lsb */,
		8/* width */),
	/* CLK_AUDDIV_3 */
	DIV_GATE(CLK_TOP_APLL12_CK_DIV_TDMOUT_M/* dts */, "apll12_div_tdmout_m"/* ccf */,
		"apll_tdmout_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		4/* pdn bit */, CLK_AUDDIV_3/* ofs */, 0/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV_TDMOUT_B/* dts */, "apll12_div_tdmout_b"/* ccf */,
		"apll_tdmout_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		5/* pdn bit */, CLK_AUDDIV_3/* ofs */, 8/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV5/* dts */, "apll12_div5"/* ccf */,
		"apll_i2s5_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		6/* pdn bit */, CLK_AUDDIV_3/* ofs */, 16/* lsb */,
		8/* width */),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV6/* dts */, "apll12_div6"/* ccf */,
		"apll_i2s6_mck_sel"/* parent */, 0x0320/* pdn ofs */,
		7/* pdn bit */, CLK_AUDDIV_3/* ofs */, 24/* lsb */,
		8/* width */),
#endif /* MT_CCF_MUX_DISABLE */
};











static const struct mtk_gate_regs ifrao0_cg_regs = {
	.set_ofs = 0x70,
	.clr_ofs = 0x70,
	.sta_ofs = 0x70,
};

static const struct mtk_gate_regs ifrao1_cg_regs = {
	.set_ofs = 0x74,
	.clr_ofs = 0x74,
	.sta_ofs = 0x74,
};

static const struct mtk_gate_regs ifrao2_cg_regs = {
	.set_ofs = 0x80,
	.clr_ofs = 0x84,
	.sta_ofs = 0x90,
};

static const struct mtk_gate_regs ifrao3_cg_regs = {
	.set_ofs = 0x88,
	.clr_ofs = 0x8c,
	.sta_ofs = 0x94,
};

static const struct mtk_gate_regs ifrao4_cg_regs = {
	.set_ofs = 0xa4,
	.clr_ofs = 0xa8,
	.sta_ofs = 0xac,
};

static const struct mtk_gate_regs ifrao5_cg_regs = {
	.set_ofs = 0xc0,
	.clr_ofs = 0xc4,
	.sta_ofs = 0xc8,
};

static const struct mtk_gate_regs ifrao6_cg_regs = {
	.set_ofs = 0xe0,
	.clr_ofs = 0xe4,
	.sta_ofs = 0xe8,
};

#define GATE_IFRAO0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_IFRAO1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_IFRAO2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_IFRAO3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao3_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_IFRAO3_I(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao3_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_IFRAO4(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao4_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_IFRAO5(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao5_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_IFRAO6(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao6_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_IFRAO6_I(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ifrao6_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate ifrao_clks[] = {
	/* IFRAO0 */
	/* IFRAO1 */
	/* IFRAO2 */
	GATE_IFRAO2(CLK_IFRAO_PMIC_TMR_SET, "ifrao_pmic_tmr_set",
			"fpwrap_ulposc_ck"/* parent */, 0),
	GATE_IFRAO2(CLK_IFRAO_PMIC_AP_SET, "ifrao_pmic_ap_set",
			"fpwrap_ulposc_ck"/* parent */, 1),
	GATE_IFRAO2(CLK_IFRAO_PMIC_MD_SET, "ifrao_pmic_md_set",
			"fpwrap_ulposc_ck"/* parent */, 2),
	GATE_IFRAO2(CLK_IFRAO_PMIC_CONN_SET, "ifrao_pmic_conn_set",
			"fpwrap_ulposc_ck"/* parent */, 3),
	GATE_IFRAO2(CLK_IFRAO_SEJ, "ifrao_sej",
			"axi_ck"/* parent */, 5),
	GATE_IFRAO2(CLK_IFRAO_MCUPM, "ifrao_mcupm",
			"mcupm_ck"/* parent */, 7),
	GATE_IFRAO2(CLK_IFRAO_GCE, "ifrao_gce",
			"axi_ck"/* parent */, 8),
	GATE_IFRAO2(CLK_IFRAO_GCE2, "ifrao_gce2",
			"axi_ck"/* parent */, 9),
	GATE_IFRAO2(CLK_IFRAO_THERM, "ifrao_therm",
			"axi_ck"/* parent */, 10),
	GATE_IFRAO2(CLK_IFRAO_I2C0, "ifrao_i2c0",
			"i2c_ck"/* parent */, 11),
	GATE_IFRAO2(CLK_IFRAO_I2C1, "ifrao_i2c1",
			"i2c_ck"/* parent */, 12),
	GATE_IFRAO2(CLK_IFRAO_I2C2, "ifrao_i2c2",
			"i2c_ck"/* parent */, 13),
	GATE_IFRAO2(CLK_IFRAO_I2C3, "ifrao_i2c3",
			"i2c_ck"/* parent */, 14),
	GATE_IFRAO2(CLK_IFRAO_PWM_HCLK, "ifrao_pwm_hclk",
			"axi_ck"/* parent */, 15),
	GATE_IFRAO2(CLK_IFRAO_PWM1, "ifrao_pwm1",
			"pwm_ck"/* parent */, 16),
	GATE_IFRAO2(CLK_IFRAO_PWM2, "ifrao_pwm2",
			"pwm_ck"/* parent */, 17),
	GATE_IFRAO2(CLK_IFRAO_PWM3, "ifrao_pwm3",
			"pwm_ck"/* parent */, 18),
	GATE_IFRAO2(CLK_IFRAO_PWM4, "ifrao_pwm4",
			"pwm_ck"/* parent */, 19),
	GATE_IFRAO2(CLK_IFRAO_PWM5, "ifrao_pwm5",
			"pwm_ck"/* parent */, 20),
	GATE_IFRAO2(CLK_IFRAO_PWM, "ifrao_pwm",
			"pwm_ck"/* parent */, 21),
	GATE_IFRAO2(CLK_IFRAO_UART0, "ifrao_uart0",
			"fuart_ck"/* parent */, 22),
	GATE_IFRAO2(CLK_IFRAO_UART1, "ifrao_uart1",
			"fuart_ck"/* parent */, 23),
	GATE_IFRAO2(CLK_IFRAO_UART2, "ifrao_uart2",
			"fuart_ck"/* parent */, 24),
	GATE_IFRAO2(CLK_IFRAO_UART3, "ifrao_uart3",
			"fuart_ck"/* parent */, 25),
	GATE_IFRAO2(CLK_IFRAO_GCE_26M_SET, "ifrao_gce_26m_set",
			"axi_ck"/* parent */, 27),
	/* IFRAO3 */
	GATE_IFRAO3(CLK_IFRAO_SPI0, "ifrao_spi0",
			"spi_ck"/* parent */, 1),
	GATE_IFRAO3(CLK_IFRAO_MSDC0, "ifrao_msdc0",
			"axi_ck"/* parent */, 2),
	GATE_IFRAO3(CLK_IFRAO_MSDC1, "ifrao_msdc1",
			"axi_ck"/* parent */, 4),
	GATE_IFRAO3(CLK_IFRAO_MSDC0_SRC_CLK, "ifrao_msdc0_clk",
			"msdc50_0_ck"/* parent */, 6),
	GATE_IFRAO3(CLK_IFRAO_GCPU, "ifrao_gcpu",
			"axi_ck"/* parent */, 8),
	GATE_IFRAO3(CLK_IFRAO_TRNG, "ifrao_trng",
			"axi_ck"/* parent */, 9),
	GATE_IFRAO3(CLK_IFRAO_AUXADC, "ifrao_auxadc",
			"f26m_ck"/* parent */, 10),
	GATE_IFRAO3(CLK_IFRAO_CPUM, "ifrao_cpum",
			"axi_ck"/* parent */, 11),
	GATE_IFRAO3(CLK_IFRAO_CCIF1_AP, "ifrao_ccif1_ap",
			"axi_ck"/* parent */, 12),
	GATE_IFRAO3(CLK_IFRAO_CCIF1_MD, "ifrao_ccif1_md",
			"axi_ck"/* parent */, 13),
	GATE_IFRAO3(CLK_IFRAO_AUXADC_MD, "ifrao_auxadc_md",
			"f26m_ck"/* parent */, 14),
	GATE_IFRAO3(CLK_IFRAO_PCIE_TL_26M, "ifrao_pcie_tl_26m",
			"axi_ck"/* parent */, 15),
	GATE_IFRAO3(CLK_IFRAO_MSDC1_SRC_CLK, "ifrao_msdc1_clk",
			"msdc30_1_ck"/* parent */, 16),
	GATE_IFRAO3(CLK_IFRAO_PCIE_TL_96M, "ifrao_pcie_tl_96m",
			"tl_ck"/* parent */, 18),
	GATE_IFRAO3(CLK_IFRAO_DEVICE_APC, "ifrao_dapc",
			"axi_ck"/* parent */, 20),
	GATE_IFRAO3(CLK_IFRAO_CCIF_AP, "ifrao_ccif_ap",
			"axi_ck"/* parent */, 23),
	GATE_IFRAO3(CLK_IFRAO_DEBUGSYS, "ifrao_debugsys",
			"axi_ck"/* parent */, 24),
	GATE_IFRAO3(CLK_IFRAO_AUDIO, "ifrao_audio",
			"axi_ck"/* parent */, 25),
	GATE_IFRAO3(CLK_IFRAO_CCIF_MD, "ifrao_ccif_md",
			"axi_ck"/* parent */, 26),
	GATE_IFRAO3(CLK_IFRAO_DEVMPU_BCLK, "ifrao_devmpu_bclk",
			"axi_ck"/* parent */, 30),
	/* IFRAO4 */
	GATE_IFRAO4(CLK_IFRAO_SSUSB, "ifrao_ssusb",
			"fusb_ck"/* parent */, 1),
	GATE_IFRAO4(CLK_IFRAO_DISP_PWM, "ifrao_disp_pwm",
			"axi_ck"/* parent */, 2),
	GATE_IFRAO4(CLK_IFRAO_CLDMA_BCLK, "ifrao_cldmabclk",
			"axi_ck"/* parent */, 3),
	GATE_IFRAO4(CLK_IFRAO_AUDIO_26M_BCLK, "ifrao_audio26m",
			"f26m_ck"/* parent */, 4),
	GATE_IFRAO4(CLK_IFRAO_MODEM_TEMP_SHARE, "ifrao_mdtemp",
			"f26m_ck"/* parent */, 5),
	GATE_IFRAO4(CLK_IFRAO_SPI1, "ifrao_spi1",
			"spi_ck"/* parent */, 6),
	GATE_IFRAO4(CLK_IFRAO_I2C4, "ifrao_i2c4",
			"i2c_ck"/* parent */, 7),
	GATE_IFRAO4(CLK_IFRAO_SPI2, "ifrao_spi2",
			"spi_ck"/* parent */, 9),
	GATE_IFRAO4(CLK_IFRAO_SPI3, "ifrao_spi3",
			"spi_ck"/* parent */, 10),
	GATE_IFRAO4(CLK_IFRAO_UNIPRO_TICK, "ifrao_unipro_tick",
			"f26m_ck"/* parent */, 12),
	GATE_IFRAO4(CLK_IFRAO_UFS_MP_SAP_BCLK, "ifrao_ufs_bclk",
			"f26m_ck"/* parent */, 13),
	GATE_IFRAO4(CLK_IFRAO_MD32_BCLK, "ifrao_md32_bclk",
			"axi_ck"/* parent */, 14),
	GATE_IFRAO4(CLK_IFRAO_UNIPRO_MBIST, "ifrao_unipro_mbist",
			"axi_ck"/* parent */, 16),
	GATE_IFRAO4(CLK_IFRAO_PWM6, "ifrao_pwm6",
			"i2c_ck"/* parent */, 18),
	GATE_IFRAO4(CLK_IFRAO_PWM7, "ifrao_pwm7",
			"i2c_ck"/* parent */, 19),
	GATE_IFRAO4(CLK_IFRAO_I2C_SLAVE, "ifrao_i2c_slave",
			"i2c_ck"/* parent */, 20),
	GATE_IFRAO4(CLK_IFRAO_I2C1_ARBITER, "ifrao_i2c1a",
			"i2c_ck"/* parent */, 21),
	GATE_IFRAO4(CLK_IFRAO_I2C1_IMM, "ifrao_i2c1_imm",
			"i2c_ck"/* parent */, 22),
	GATE_IFRAO4(CLK_IFRAO_I2C2_ARBITER, "ifrao_i2c2a",
			"i2c_ck"/* parent */, 23),
	GATE_IFRAO4(CLK_IFRAO_I2C2_IMM, "ifrao_i2c2_imm",
			"i2c_ck"/* parent */, 24),
	GATE_IFRAO4(CLK_IFRAO_SSUSB_XHCI, "ifrao_ssusb_xhci",
			"fssusb_xhci_ck"/* parent */, 31),
	/* IFRAO5 */
	GATE_IFRAO5(CLK_IFRAO_MSDC0_SELF, "ifrao_msdc0sf",
			"msdc50_0_ck"/* parent */, 0),
	GATE_IFRAO5(CLK_IFRAO_MSDC1_SELF, "ifrao_msdc1sf",
			"msdc50_0_ck"/* parent */, 1),
	GATE_IFRAO5(CLK_IFRAO_MSDC2_SELF, "ifrao_msdc2sf",
			"msdc50_0_ck"/* parent */, 2),
	GATE_IFRAO5(CLK_IFRAO_SSPM_26M_SELF, "ifrao_sspm_26m",
			"f26m_ck"/* parent */, 3),
	GATE_IFRAO5(CLK_IFRAO_SSPM_32K_SELF, "ifrao_sspm_32k",
			"frtc_ck"/* parent */, 4),
	GATE_IFRAO5(CLK_IFRAO_I2C6, "ifrao_i2c6",
			"i2c_ck"/* parent */, 6),
	GATE_IFRAO5(CLK_IFRAO_AP_MSDC0, "ifrao_ap_msdc0",
			"msdc50_0_ck"/* parent */, 7),
	GATE_IFRAO5(CLK_IFRAO_MD_MSDC0, "ifrao_md_msdc0",
			"msdc50_0_ck"/* parent */, 8),
	GATE_IFRAO5(CLK_IFRAO_CCIF5_AP, "ifrao_ccif5_ap",
			"axi_ck"/* parent */, 9),
	GATE_IFRAO5(CLK_IFRAO_CCIF5_MD, "ifrao_ccif5_md",
			"axi_ck"/* parent */, 10),
	GATE_IFRAO5(CLK_IFRAO_PCIE_TOP_HCLK_133M, "ifrao_pcie_h_133m",
			"axi_ck"/* parent */, 11),
	GATE_IFRAO5(CLK_IFRAO_SPIS_HCLK_66M, "ifrao_spis_h_66m",
			"axi_ck"/* parent */, 14),
	GATE_IFRAO5(CLK_IFRAO_PCIE_PERI_26M, "ifrao_pcie_peri_26m",
			"f26m_ck"/* parent */, 15),
	GATE_IFRAO5(CLK_IFRAO_CCIF2_AP, "ifrao_ccif2_ap",
			"axi_ck"/* parent */, 16),
	GATE_IFRAO5(CLK_IFRAO_CCIF2_MD, "ifrao_ccif2_md",
			"axi_ck"/* parent */, 17),
	GATE_IFRAO5(CLK_IFRAO_SEJ_F13M, "ifrao_sej_f13m",
			"f26m_ck"/* parent */, 20),
	GATE_IFRAO5(CLK_IFRAO_AES, "ifrao_aes",
			"axi_ck"/* parent */, 21),
	GATE_IFRAO5(CLK_IFRAO_I2C7, "ifrao_i2c7",
			"i2c_ck"/* parent */, 22),
	GATE_IFRAO5(CLK_IFRAO_I2C8, "ifrao_i2c8",
			"i2c_ck"/* parent */, 23),
	GATE_IFRAO5(CLK_IFRAO_FBIST2FPC, "ifrao_fbist2fpc",
			"msdc50_0_ck"/* parent */, 24),
	GATE_IFRAO5(CLK_IFRAO_DPMAIF_MAIN, "ifrao_dpmaif_main",
			"dpmaif_main_ck"/* parent */, 26),
	GATE_IFRAO5(CLK_IFRAO_PCIE_TL_32K, "ifrao_pcie_tl_32k",
			"frtc_ck"/* parent */, 27),
	GATE_IFRAO5(CLK_IFRAO_CCIF4_AP, "ifrao_ccif4_ap",
			"axi_ck"/* parent */, 28),
	GATE_IFRAO5(CLK_IFRAO_CCIF4_MD, "ifrao_ccif4_md",
			"axi_ck"/* parent */, 29),
	/* IFRAO6 */
	GATE_IFRAO6(CLK_IFRAO_133M_MCLK_CK, "ifrao_133m_mclk_ck",
			"axi_ck"/* parent */, 0),
	GATE_IFRAO6(CLK_IFRAO_66M_MCLK_CK, "ifrao_66m_mclk_ck",
			"axi_ck"/* parent */, 1),
	GATE_IFRAO6(CLK_IFRAO_66M_PERI_BUS_MCLK_CK, "ifrao_66m_peri_mclk",
			"axi_ck"/* parent */, 2),
	GATE_IFRAO6(CLK_IFRAO_INFRA_FREE_DCM_133M, "ifrao_infra_133m",
			"axi_ck"/* parent */, 3),
	GATE_IFRAO6(CLK_IFRAO_INFRA_FREE_DCM_66M, "ifrao_infra_66m",
			"axi_ck"/* parent */, 4),
	GATE_IFRAO6(CLK_IFRAO_PERU_BUS_DCM_133M, "ifrao_peru_bus_133m",
			"axi_ck"/* parent */, 5),
	GATE_IFRAO6(CLK_IFRAO_PERU_BUS_DCM_66M, "ifrao_peru_bus_66m",
			"axi_ck"/* parent */, 6),
	GATE_IFRAO6(CLK_IFRAO_RG_133M_CLDMA_TOP, "ifrao_133m_cldma_top",
			"axi_ck"/* parent */, 7),
	GATE_IFRAO6(CLK_IFRAO_RG_ECC_TOP, "ifrao_ecc_top",
			"axi_ck"/* parent */, 8),
	GATE_IFRAO6(CLK_IFRAO_RG_66M_GCPU, "ifrao_66m_gcpu",
			"axi_ck"/* parent */, 9),
	GATE_IFRAO6(CLK_IFRAO_RG_133M_DWC_ETHER, "ifrao_133m_dwc_ether",
			"axi_ck"/* parent */, 11),
	GATE_IFRAO6(CLK_IFRAO_RG_133M_FLASHIF, "ifrao_133m_flashif",
			"axi_ck"/* parent */, 12),
	GATE_IFRAO6(CLK_IFRAO_RG_133M_PCIE_P0, "ifrao_133m_pcie_p0",
			"axi_ck"/* parent */, 13),
	GATE_IFRAO6_I(CLK_IFRAO_RG_133M_PCIE_P1, "ifrao_133m_pcie_p1",
			"axi_ck"/* parent */, 14),
	GATE_IFRAO6_I(CLK_IFRAO_RG_133M_PCIE_P2, "ifrao_133m_pcie_p2",
			"axi_ck"/* parent */, 15),
	GATE_IFRAO6_I(CLK_IFRAO_RG_133M_PCIE_P3, "ifrao_133m_pcie_p3",
			"axi_ck"/* parent */, 16),
	GATE_IFRAO6(CLK_IFRAO_RG_MMW_DPMAIF_TOP_CK, "ifrao_mmw_dpmaif_ck",
			"axi_ck"/* parent */, 17),
	GATE_IFRAO6(CLK_IFRAO_RG_NFI, "ifrao_nfi",
			"nfi1x_ck"/* parent */, 18),
	GATE_IFRAO6(CLK_IFRAO_RG_FPINFI_BCLK_CK, "ifrao_fpinfi_bclk_ck",
			"spinfi_bclk_ck"/* parent */, 19),
	GATE_IFRAO6(CLK_IFRAO_RG_66M_NFI_HCLK_CK, "ifrao_66m_nfi_h_ck",
			"axi_ck"/* parent */, 20),
	GATE_IFRAO6(CLK_IFRAO_RG_FSPIS_CK, "ifrao_fspis_ck",
			"axi_ck"/* parent */, 21),
	GATE_IFRAO6(CLK_IFRAO_RG_PCIE_PERI_26M_P1, "ifrao_26m_p1",
			"axi_ck"/* parent */, 25),
	GATE_IFRAO6(CLK_IFRAO_RG_PCIE_PERI_26M_P2, "ifrao_26m_p2",
			"axi_ck"/* parent */, 26),
	GATE_IFRAO6(CLK_IFRAO_RG_PCIE_PERI_26M_P3, "ifrao_26m_p3",
			"axi_ck"/* parent */, 27),
	GATE_IFRAO6(CLK_IFRAO_RG_FLASHIF_PERI_26M, "ifrao_flash_26m",
			"axi_ck"/* parent */, 30),
	GATE_IFRAO6(CLK_IFRAO_RG_FLASHIF_SFLASH, "ifrao_sflash_ck",
			"axi_ck"/* parent */, 31),
};



static const struct mtk_gate_regs peri_cg_regs = {
	.set_ofs = 0x20c,
	.clr_ofs = 0x20c,
	.sta_ofs = 0x20c,
};

#define GATE_PERI(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate peri_clks[] = {
};

static const struct mtk_gate_regs apmixed_cg_regs = {
	.set_ofs = 0x14,
	.clr_ofs = 0x14,
	.sta_ofs = 0x14,
};
#define GATE_APMIXED(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &apmixed_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate apmixed_clks[] = {
};


#define MT6880_PLL_FMAX		(3800UL * MHZ)
#define MT6880_PLL_FMIN		(1500UL * MHZ)
#define MT6880_INTEGER_BITS	8

#if MT_CCF_PLL_DISABLE
#define PLL_CFLAGS		PLL_AO
#else
#define PLL_CFLAGS		(0)
#endif

#define PLL(_id, _name, _reg, _en_reg, _en_mask, _pll_en_bit,		\
			_pwr_reg,	 _flags, _rst_bar_mask,		\
			_pd_reg, _pd_shift, _tuner_reg,			\
			_tuner_en_reg, _tuner_en_bit,			\
			_pcw_reg, _pcw_shift, _pcwbits) {		\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.en_reg = _en_reg,					\
		.en_mask = _en_mask,					\
		.pll_en_bit = _pll_en_bit,				\
		.pwr_reg = _pwr_reg,					\
		.flags = (_flags | PLL_CFLAGS),				\
		.rst_bar_mask = _rst_bar_mask,				\
		.fmax = MT6880_PLL_FMAX,				\
		.fmin = MT6880_PLL_FMIN,				\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.tuner_en_reg = _tuner_en_reg,			\
		.tuner_en_bit = _tuner_en_bit,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
		.pcwbits = _pcwbits,					\
		.pcwibits = MT6880_INTEGER_BITS,			\
	}
#define PLL_B(_id, _name, _reg, _en_mask, _pwr_reg, _flags,		\
			_rst_bar_mask, _pd_reg, _pd_shift,		\
			_tuner_reg, _tuner_en_reg, _tuner_en_bit,	\
			_pcw_reg, _pcw_shift, _pcwbits)			\
		PLL(_id, _name, _reg, 0, _en_mask, 0,			\
			_pwr_reg,	 _flags, _rst_bar_mask,		\
			_pd_reg, _pd_shift, _tuner_reg,			\
			_tuner_en_reg, _tuner_en_bit,			\
			_pcw_reg, _pcw_shift, _pcwbits)

static const struct mtk_pll_data plls[] = {
	PLL(CLK_APMIXED_ARMPLL_LL, "armpll_ll", ARMPLL_LL_CON0/*base*/,
		ARMPLL_LL_CON0, 0x0200, 9/*en*/,
		ARMPLL_LL_CON4, PLL_AO, BIT(0)/*rstb*/,
		0x020C, 24/*pd*/,
		0, 0, 0/*tuner*/,
		ARMPLL_LL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_CCIPLL, "ccipll", CCIPLL_CON0/*base*/,
		CCIPLL_CON0, 0x0200, 9/*en*/,
		CCIPLL_CON4, PLL_AO, BIT(0)/*rstb*/,
		0x0220, 24/*pd*/,
		0, 0, 0/*tuner*/,
		CCIPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_MPLL, "mpll", MPLL_CON0/*base*/,
		MPLL_CON0, 0x0200, 9/*en*/,
		MPLL_CON4, PLL_AO, BIT(0)/*rstb*/,
		0x060C, 24/*pd*/,
		0, 0, 0/*tuner*/,
		MPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", MAINPLL_CON0/*base*/,
		MAINPLL_CON0, 0x0200, 9/*en*/,
		MAINPLL_CON4, HAVE_RST_BAR|PLL_AO, BIT(23)/*rstb*/,
		0x040C, 24/*pd*/,
		0, 0, 0/*tuner*/,
		MAINPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_UNIVPLL, "univpll", UNIVPLL_CON0/*base*/,
		UNIVPLL_CON0, 0x0200, 9/*en*/,
		UNIVPLL_CON4, HAVE_RST_BAR, BIT(23)/*rstb*/,
		0x0420, 24/*pd*/,
		0, 0, 0/*tuner*/,
		UNIVPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", MSDCPLL_CON0/*base*/,
		MSDCPLL_CON0, 0x0200, 9/*en*/,
		MSDCPLL_CON4, 0, BIT(0)/*rstb*/,
		0x234, 24/*pd*/,
		0, 0, 0/*tuner*/,
		MSDCPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_MMPLL, "mmpll", MMPLL_CON0/*base*/,
		MMPLL_CON0, 0x0200, 9/*en*/,
		MMPLL_CON4, HAVE_RST_BAR, BIT(23)/*rstb*/,
		0x0434, 24/*pd*/,
		0, 0, 0/*tuner*/,
		MMPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_MFGPLL, "mfgpll", MFGPLL_CON0/*base*/,
		MFGPLL_CON0, 0x0200, 9/*en*/,
		MFGPLL_CON4, 0, BIT(0)/*rstb*/,
		0x0620, 24/*pd*/,
		0, 0, 0/*tuner*/,
		MFGPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_APLL1, "apll1", APLL1_CON0/*base*/,
		APLL1_CON0, 0x0200, 9/*en*/,
		APLL1_CON5, 0, BIT(0)/*rstb*/,
		0x045C, 24/*pd*/,
		APLL1_TUNER_CON0, AP_PLL_CON0, 12/*tuner*/,
		APLL1_CON3, 0, 32/*pcw*/),
	PLL(CLK_APMIXED_APLL2, "apll2", APLL2_CON0/*base*/,
		APLL2_CON0, 0x0200, 9/*en*/,
		APLL2_CON5, 0, BIT(0)/*rstb*/,
		0x0474, 24/*pd*/,
		APLL2_TUNER_CON0, AP_PLL_CON0, 13/*tuner*/,
		APLL2_CON3, 0, 32/*pcw*/),
	PLL(CLK_APMIXED_NET1PLL, "net1pll", NET1PLL_CON0/*base*/,
		NET1PLL_CON0, 0x0200, 9/*en*/,
		NET1PLL_CON4, 0, BIT(0)/*rstb*/,
		0x080C, 24/*pd*/,
		0, 0, 0/*tuner*/,
		NET1PLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_NET2PLL, "net2pll", NET2PLL_CON0/*base*/,
		NET2PLL_CON0, 0x0200, 9/*en*/,
		NET2PLL_CON4, 0, BIT(0)/*rstb*/,
		0x0820, 24/*pd*/,
		0, 0, 0/*tuner*/,
		NET2PLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_WEDMCUPLL, "wedmcupll", WEDMCUPLL_CON0/*base*/,
		WEDMCUPLL_CON0, 0x0200, 9/*en*/,
		WEDMCUPLL_CON4, 0, BIT(0)/*rstb*/,
		0x0834, 24/*pd*/,
		0, 0, 0/*tuner*/,
		WEDMCUPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_MEDMCUPLL, "medmcupll", MEDMCUPLL_CON0/*base*/,
		MEDMCUPLL_CON0, 0x0200, 9/*en*/,
		MEDMCUPLL_CON4, 0, BIT(0)/*rstb*/,
		0x0848, 24/*pd*/,
		0, 0, 0/*tuner*/,
		MEDMCUPLL_CON2, 0, 22/*pcw*/),
	PLL(CLK_APMIXED_SGMIIPLL, "sgmiipll", SGMIIPLL_CON0/*base*/,
		SGMIIPLL_CON0, 0x0200, 9/*en*/,
		SGMIIPLL_CON4, 0, BIT(0)/*rstb*/,
		0x0248, 24/*pd*/,
		0, 0, 0/*tuner*/,
		SGMIIPLL_CON2, 0, 22/*pcw*/),
};

static int clk_mt6880_apmixed_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	void __iomem *base;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		pr_err("%s(): ioremap failed\n", __func__);
		return PTR_ERR(base);
	}

	clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	mtk_clk_register_plls(node, plls, ARRAY_SIZE(plls),
			clk_data);

	mtk_clk_register_gates(node, apmixed_clks, ARRAY_SIZE(apmixed_clks),
			clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r) {
		kfree(clk_data);
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	}

	apmixed_base = base;

#if MT_CCF_BRINGUP
	pr_notice("%s init end\n", __func__);
#endif

	return r;
}

static int clk_mt6880_ifrao_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_IFRAO_NR_CLK);

	mtk_clk_register_gates(node, ifrao_clks, ARRAY_SIZE(ifrao_clks),
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

static int clk_mt6880_peri_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	clk_data = mtk_alloc_clk_data(CLK_PERI_NR_CLK);

	mtk_clk_register_gates(node, peri_clks, ARRAY_SIZE(peri_clks),
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

static struct clk_onecell_data *mt6880_top_clk_data;

static int clk_mt6880_top_probe(struct platform_device *pdev)
{
	int r;
	struct device_node *node = pdev->dev.of_node;

	void __iomem *base;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

#if MT_CCF_BRINGUP
	pr_notice("%s init begin\n", __func__);
#endif

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		pr_err("%s(): ioremap failed\n", __func__);
		return PTR_ERR(base);
	}

	mt6880_top_clk_data = mtk_alloc_clk_data(CLK_TOP_NR_CLK);

	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs),
			mt6880_top_clk_data);

	mtk_clk_register_muxes(top_muxes, ARRAY_SIZE(top_muxes), node,
			&mt6880_clk_lock, mt6880_top_clk_data);

	mtk_clk_register_composites(top_composites, ARRAY_SIZE(top_composites),
			base, &mt6880_clk_lock, mt6880_top_clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get,
			mt6880_top_clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
/*
	mtk_clk_check_muxes(top_muxes, ARRAY_SIZE(top_muxes),
			mt6880_top_clk_data);
*/
#if MT_CCF_BRINGUP
	pr_notice("%s init end\n", __func__);
#endif

	return r;
}

/* for suspend LDVT only 
void pll_force_off(void)
{
	void __iomem *rst_reg, *en_reg, *pwr_reg;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(plls); i++) {
		// do not pwrdn the AO PLLs 
		if ((plls[i].flags & PLL_AO) == PLL_AO)
			continue;

		if ((plls[i].flags & HAVE_RST_BAR) == HAVE_RST_BAR) {
			rst_reg = apmixed_base + plls[i].rst_bar_reg;
			writel(readl(rst_reg) & ~plls[i].rst_bar_mask,
				rst_reg);
		}

		en_reg = apmixed_base + plls[i].en_reg;

		pwr_reg = apmixed_base + plls[i].pwr_reg;

		writel(readl(en_reg) & ~plls[i].en_mask,
				en_reg);
		writel(readl(pwr_reg) | plls[i].iso_mask,
				pwr_reg);
		writel(readl(pwr_reg) & ~plls[i].pwron_mask,
				pwr_reg);
	}
}

static struct generic_pm_domain **get_all_genpd(void)
{
	static struct generic_pm_domain *pds[31];
	static int num_pds;
	const size_t maxpd = ARRAY_SIZE(pds);
	struct device_node *node;
	struct platform_device *pdev;
	int r;
	if (num_pds != 0)
		goto out;
	node = of_find_node_with_property(NULL, "#power-domain-cells");
	if (node == NULL)
		return NULL;
	pdev = platform_device_alloc("traverse", 0);
	for (num_pds = 0; num_pds < maxpd; num_pds++) {
		struct of_phandle_args pa;
		pa.np = node;
		pa.args[0] = num_pds;
		pa.args_count = 1;
		r = of_genpd_add_device(&pa, &pdev->dev);
		if (r == -EINVAL)
			continue;
		else if (r != 0)
			pr_warn("%s(): of_genpd_add_device(%d)\n", __func__, r);
		pds[num_pds] = pd_to_genpd(pdev->dev.pm_domain);
		//r = pm_genpd_remove_device(pds[num_pds], &pdev->dev);
		r = pm_genpd_remove_device(&pdev->dev);
		if (r != 0)
			pr_warn("%s(): pm_genpd_remove_device(%d)\n",
					__func__, r);
		if (IS_ERR(pds[num_pds])) {
			pds[num_pds] = NULL;
			break;
		}
	}
	platform_device_put(pdev);
out:
	return pds;
}

void subsys_force_off(void)
{
	struct generic_pm_domain *genpd;
	int (*gpd_op)(struct generic_pm_domain *);
	int r = 0;
    struct generic_pm_domain **pds = get_all_genpd();
    for (; *pds != NULL; pds++) {
        genpd = *pds;
        if (IS_ERR_OR_NULL(genpd))
            continue;
        if((genpd->flags & GENPD_FLAG_ALWAYS_ON)|(genpd->status == GPD_STATE_POWER_OFF))
            continue;
        gpd_op = genpd->power_off;
        r |= gpd_op(genpd);
    }
}

void pll_if_on(void)
{
    void __iomem *en_reg;
    u32 i;
	for (i = 0; i < ARRAY_SIZE(plls); i++) {

        en_reg = apmixed_base + plls[i].en_reg;

        if (readl(en_reg) & plls[i].en_mask)
            pr_notice("suspend warning : %s is on !!!\n",plls[i].name);

	}
}

void subsys_if_on(void)
{
    static const char * const pwr_names[] = {
		[0] = "MD1",
		[1] = "CONN",
		[2] = "MFG0",
		[3] = "PEXTP_D_2LX1_PHY",
		[4] = "PEXTP_R_2LX1_PHY",
		[5] = "PEXTP_R_1LX2_0P_PHY",
		[6] = "PEXTP_R_1LX2_1P_PHY",
		[7] = "SSUSB_PHY",
		[8] = "SGMII_0_PHY",
		[9] = "IFR",
		[10] = "SGMII_1_PHY",
		[11] = "DPY",
		[12] = "PEXTP_D_2LX1",
		[13] = "PEXTP_R_2LX1",
		[14] = "PEXTP_R_1LX2",
		[15] = "ETH",
		[16] = "SSUSB",
		[17] = "SGMII_0_TOP",
		[18] = "SGMII_1_TOP",
		[19] = "NETSYS",
		[20] = "DIS",
		[21] = "AUDIO",
		[22] = "EIP97",
		[23] = "HSMTOP",
		[24] = "DRAMC_MD32",
		[25] = "(Reserved)",
		[26] = "(Reserved)",
		[27] = "(Reserved)",
		[28] = "DPY2",
		[29] = "MCUPM",
		[30] = "MSDC",
		[31] = "PERI",
	};
    u32 val = 0,i;
    static void __iomem *scpsys_base, *pwr_sta, *pwr_sta_2nd;

    scpsys_base = ioremap(0x10006000, PAGE_SIZE);
    pwr_sta = scpsys_base + 0x16c;
    pwr_sta_2nd = scpsys_base + 0x170;
    val = readl(pwr_sta) & readl(pwr_sta_2nd);

    for (i = 0; i < 32; i++) {
        if((val & BIT(i)) != 0U)
            pr_notice("suspend warning: %s is on!!\n",pwr_names[i]);
    }
}

static int pll_status_cmd(struct seq_file *s, void *v)
{
	seq_printf(s, "Call pll_if_on \n");
    pll_if_on();
	return 0;
}

static int mtcmos_status_cmd(struct seq_file *s, void *v)
{
	seq_printf(s, "Call subsys_if_on \n");
    subsys_if_on();
	return 0;
}

static int pll_off_cmd(struct seq_file *s, void *v)
{
	seq_printf(s, "Call pll_force_off \n");
    pll_force_off();
	return 0;
}

static int mtcmos_off_cmd(struct seq_file *s, void *v)
{
	seq_printf(s, "Call subsys_force_off \n");
    subsys_force_off();
	return 0;
}

static int all_off_cmd(struct seq_file *s, void *v)
{
	seq_printf(s, "Call pll/mtcmos off and status \n");
    pll_force_off();
    subsys_force_off();
    pll_if_on();
    subsys_if_on();
	return 0;
}

static const struct cmd_fn cmds[] = {
    CMDFN("pll_status", pll_status_cmd),
    CMDFN("mtcmos_status", mtcmos_status_cmd),
    CMDFN("pll_off", pll_off_cmd),
    CMDFN("mtcmos_off", mtcmos_off_cmd),
    CMDFN("all_off", all_off_cmd),
    {}
};
*/
static const struct of_device_id of_match_clk_mt6880[] = {
	{
		.compatible = "mediatek,mt6880-apmixedsys",
		.data = clk_mt6880_apmixed_probe,
	}, {
		.compatible = "mediatek,mt6880-infracfg_ao",
		.data = clk_mt6880_ifrao_probe,
	}, {
		.compatible = "mediatek,mt6880-pericfg",
		.data = clk_mt6880_peri_probe,
	}, {
		.compatible = "mediatek,mt6880-topckgen",
		.data = clk_mt6880_top_probe,
	}, {
		/* sentinel */
	}
};

static int clk_mt6880_probe(struct platform_device *pdev)
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

	//set_custom_cmds(cmds);
	return r;
}

static struct platform_driver clk_mt6880_drv = {
	.probe = clk_mt6880_probe,
	.driver = {
		.name = "clk-mt6880",
		.owner = THIS_MODULE,
		.of_match_table = of_match_clk_mt6880,
	},
};

static int __init clk_mt6880_init(void)
{
	return platform_driver_register(&clk_mt6880_drv);
}

arch_initcall_sync(clk_mt6880_init);

