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

#include <dt-bindings/power/mt6990-power.h>

#ifdef CONFIG_MTK_DEVAPC
#include <linux/soc/mediatek/devapc_public.h>
#endif

#include "clkdbg.h"
#include "clkchk.h"
#include "clkchk-mt6990.h"
#include "clk-mt6990-fmeter.h"

/*
 * clkdbg dump_state
 */

static const char * const clk_names[] = {
	/* topckgen */
	"axi_sel",
	"peri_haxi_sel",
	"peri_axi_sel",
	"bus_aximem_sel",
	"mem_sub_sel",
	"dvfsrc_sel",
	"mm_sel",
	"dbi_sel",
	"disp_pwm_sel",
	"mfg_ref_sel",
	"mfg_sel",
	"uart_sel",
	"msdc5hclk_sel",
	"msdc50_0_sel",
	"msdc30_1_sel",
	"msdc_macro_sel",
	"audio_sel",
	"aud_intbus_sel",
	"aud_engen1_sel",
	"aud_engen2_sel",
	"aud_1_sel",
	"aud_2_sel",
	"atb_sel",
	"i2c_sel",
	"tl_sel",
	"dpmaif_main_sel",
	"mcupm_sel",
	"sflash_sel",
	"spi_sel",
	"spis_sel",
	"nfi1x_sel",
	"spinfi_bclk_sel",
	"gcpu_sel",
	"ecc_sel",
	"hsm_crypto_sel",
	"hsm_arc_sel",
	"eip97_sel",
	"eth_312p5m_sel",
	"snps_eth_250m_sel",
	"eth_62p4m_sel",
	"eth_50m_rmii_sel",
	"netsys_500m_sel",
	"netsys_sel",
	"netsys_2x_sel",
	"netsys_wed_mcu_sel",
	"netsys_med_mcu_sel",
	"sgmii_0_sel",
	"sgmii_sbus_0_sel",
	"sgmii_1_sel",
	"sgmii_sbus_1_sel",
	"usxgmii_sbus_0_sel",
	"usxgmii_sbus_1_sel",
	"ap2conn_host_sel",
	"usb_sel",
	"ssusb_xhci_sel",
	"emi_n_sel",
	"hsm_hsah_sel",
	"rsa_sel",
	"msdc_occ_400_sel",
	"msdc_occ_200_sel",
	"dxcc_sel",
	"pcie_250m_sel",
	"dsi_occ_sel",
	"emi_546_sel",
	"emi_624_sel",
	"apll_i2s0_mck_sel",
	"apll_i2s1_mck_sel",
	"apll_i2s2_mck_sel",
	"apll_i2s4_mck_sel",
	"apll_i2s_tdmout_sel",
	"apll_i2s5_mck_sel",
	"apll_i2s6_mck_sel",

	/* topckgen */
	"apll12_div0",
	"apll12_div1",
	"apll12_div2",
	"apll12_div4",
	"apll12_div5",
	"apll12_div6",

	/* infracfg_ao */
	"ifrao_bist2fpc",
	"ifrao_dpmaif",
	"ifrao_eip97",
	"ifrao_cldma0",
	"ifrao_cldma1",
	"ifrao_cldma2",
	"ifrao_cldma3",
	"ifrao_cldma4",
	"ifrao_cldma5",
	"ifrao_gce",
	"ifrao_cq_dma",
	"ifrao_sramrom",
	"ifrao_ccif0",
	"ifrao_ccif1",
	"ifrao_ccif3",
	"ifrao_dpmaif_26m",

	/* apmixedsys */
	"armpll_ll",
	"ccipll",
	"mainpll",
	"univpll",
	"msdcpll",
	"mmpll",
	"apll1",
	"apll2",
	"mpll",
	"mfgpll",
	"net1pll",
	"net2pll",
	"wedmcupll",
	"medmcupll",
	"sgmiipll",

	/* gce */
	"gce_0",

	/* imp_iic_top_wrap */
	"imp_iic_wrap_i2c0",
	"imp_iic_wrap_i2c1",
	"imp_iic_wrap_i2c4",
	"imp_iic_wrap_i2c5",
	"imp_iic_wrap_i2c2",
	"imp_iic_wrap_i2c3",

	/* pericfg_ao */
	"perao_uart0",
	"perao_uart1",
	"perao_uart2",
	"perao_uart3",
	"perao_uarthub_pclk",
	"perao_uarthub",
	"perao_spi0_bclk",
	"perao_spi1_bclk",
	"perao_spi2_bclk",
	"perao_spi3_bclk",
	"perao_spis_bclk",
	"perao_spi0_hclk",
	"perao_spi1_hclk",
	"perao_spi2_hclk",
	"perao_spi3_hclk",
	"perao_spis_hclk",
	"perao_i2c_slave",
	"perao_disp_pwm_bclk",
	"perao_flash_clk",
	"perao_flashif_bclk",
	"perao_flash_axi_clk",
	"perao_flash_dram_clk",
	"perao_msdc1_src",
	"perao_msdc1_hclk",
	"perao_msdc50_scan",
	"perao_msdc50_hclk",
	"perao_msdc0_slave",
	"perao_msdc0_master",
	"perao_ap_dma_hclk",
	"perao_nfi_bclk",
	"perao_nfi_cfclk",
	"perao_nfi_hclk",
	"perao_therm_bclk",
	"perao_therm_sys",
	"perao_ptp_therm_bclk",
	"perao_ptp_therm_sys",
	"perao_usb_frmcnt",
	"perao_usb_sys",
	"perao_usb_xchi",
	"perao_usb_dma",
	"perao_usb_mcu",
	"perao_usb_26m",
	"perao_dwc_rmii",
	"perao_dwc_250",
	"perao_dwc_312p5",
	"perao_dwc_ptp_ref",
	"perao_dwc_ptp_aclk",
	"perao_pcie_2lx1_tl",
	"perao_pcie_2lx1_ref",
	"perao_pcie_2lx1_axi",
	"perao_pcie_2lx1_ahb",
	"perao_pcie_2lx1_26m",
	"perao_pcie_2lx2_tl",
	"perao_pcie_2lx2_ref",
	"perao_pcie_2lx2_axi",
	"perao_pcie_2lx2_ahb",
	"perao_pcie_2lx2_26m",
	"perao_audio_qaxi",
	"perao_audio_26m",
	"perao_med_main",
	"perao_med_26m",
	"perao_dwc_csr",
	"perao_mdap_bus",
	"perao_mdap_26m",
	"perao_p2p_east_tx",
	"perao_p2p_west_tx",
	"perao_p2p_south_tx",
	"perao_p2p_north_tx",
	"perao_p2p_wn_tx",
	"perao_peri2pcie0__tx",

	/* ssusb_sifslv_ippc */
	"usb_sif_usb_dma",
	"usb_sif_usb_u2_p",
	"usb_sif_usb_u3_p",

	/* afe */
	"afe_afe",
	"afe_22m",
	"afe_24m",
	"afe_apll2_tuner",
	"afe_apll_tuner",
	"afe_tdm_ck",
	"afe_adc",
	"afe_dac",
	"afe_dac_predis",
	"afe_tml",
	"afe_i2s0_bclk",
	"afe_i2s1_bclk",
	"afe_i2s2_bclk",
	"afe_i2s4_bclk",
	"afe_i2s5_bclk",
	"afe_i2s6_bclk",
	"afe_general2_asrc",
	"afe_etdm_out0_bclk",
	"afe_connsys_i2s_asrc",
	"afe_afe_src_pcm_tx",
	"afe_afe_src_pcm_tx2",
	"afe_afe_src_pcm_tx3",
	"afe_afe_src_pcm_rx",
	"afe_afe_src_i2sin",
	"afe_afe_src_i2sout",

	/* msdc0 */
	"msdc0_msdc_rx",
	"msdc0_axi_wrap_cken",

	/* msdc1 */
	"msdc1_msdc_rx",

	/* ssusb_usb20_phy_t5ff */
	"usb_t5ff_chgdt_en",

	/* vlpcfg */
	"vlpcfg_pcie_tl_32k",
	"vlpcfg_pcie_tl_26m",
	"vlpcfg_pcie_tl_96m",
	"vlpcfg_PCIE_ULPOSC",
	"vlpcfg_pcie_ref_26m",
	"vlpcfg_pcie_axi250",
	"vlpcfg_pcie_sram_26m",
	"vlpcfg_pwm_cgen_1",
	"vlpcfg_pwm_cgen_2",
	"vlpcfg_pwm_cgen_3",
	"vlpcfg_pwm_cgen_4",
	"vlpcfg_pwm_cgen_5",
	"vlpcfg_pwm_cgen_6",
	"vlpcfg_pwm_cgen_7",
	"vlpcfg_pwm_cgen_8",

	/* vlp_cksys */
	"vlp_scp_sel",
	"vlp_pwrap_ulposc_sel",
	"vlp_gpt_bclk_sel",
	"vlp_dxcc_sel",
	"vlp_spmi_p_sel",
	"vlp_spmi_m_sel",
	"vlp_dvfsrc_sel",
	"vlp_pwm_vlp_sel",
	"vlp_axi_vlp_sel",
	"vlp_dbgao_26m_sel",
	"vlp_systimer_26m_sel",
	"vlp_pwrmcu_sel",
	"vlp_sspm_f26m_sel",
	"vlp_apeint_66m_sel",
	"vlp_srck_sel",
	"vlp_sramrc_sel",
	"vlp_tl_vlp_sel",
	"vlp_hsm_crypto_sel",
	"vlp_hsm_arc_sel",

	/* sspm_cfg */
	"sspm_cfg_tmr_en",
	"sspm_cfg_slpp_m_en",
	"sspm_cfg_h_en",
	"sspm_cfg_dma_en",
	"sspm_cfg_uart_en",
	"sspm_cfg_twam_en",
	"sspm_cfg_rsv1",
	"sspm_cfg_pmicwp2p_en",
	"sspm_cfg_md32pclk_en",
	"sspm_cfg_spmp2p_en",
	"sspm_cfg_apbbr_en",
	"sspm_cfg_axibr_en",
	"sspm_cfg_slpp_i_en",
	"sspm_cfg_slpp_h_en",
	"sspm_cfg_icache_en",
	"sspm_cfg_lat_mon_en",
	"sspm_cfg_uart_bclk",

	/* mfg_top */
	"mfg_bg3d",

	/* mdpsys_config */
	"mdp_mutex0",
	"mdp_apb_bus",
	"mdp_rsz0",
	"mdp_disp_gamma0",
	"mdp_wrot0",
	"mdp_disp_color0",
	"mdp_disp_ccorr0",
	"mdp_disp_aal0",
	"mdp_disp_rdma0",
	"mdp_rdma0",
	"mdp_fake_eng0",
	"mdp_disp_dither0",
	"mdp_disp_wdma0",
	"mdp_tdshp0",
	"mdp_disp_ovl0",
	"mdp_disp_dbpi0",
	"mdp_CLK0",
	"mdp_smi_common",
	"mdp_CLK0_1",
	"mdp_disp_dbpi0_1",

	/* dbgao */
	"dbgao_atb_en",

	/* dem */
	"dem_atb_en",
	"dem_busclk_en",
	"dem_sysclk_en",
};

static const char * const *get_all_clk_names(void)
{
	return clk_names;
}

/*
 * fmeter
 */

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */

#define FMCLK3(_t, _i, _n, _o, _g) { .type = _t, \
		.id = _i, .name = _n, .ofs = _o, .grp = _g}
#define FMCLK2(_t, _i, _n, _o, _p) { .type = _t, \
		.id = _i, .name = _n, .ofs = _o, .pdn = _p}
#define FMCLK(_t, _i, _n) { .type = _t, .id = _i, .name = _n}
#define REGBASE_V(_phys, _id_name, _pg) { .phys = _phys,	\
		.name = #_id_name, .pg = _pg}

#define VLP_FM_WAIT_TIME	40 /* ~= 38.64ns * 1023 */
#define _CKGEN(x)		( ioremap_nocache(0x10000000, 0x1000) + (x))
#define _VLPCK(x)		( ioremap_nocache(0x12013000, 0x1000) + (x))
#define CLK26CALI_0		_CKGEN(0x220)
#define CLK26CALI_1		_CKGEN(0x224)
#define CLK_MISC_CFG_0		_CKGEN(0x240)
#define CLK_DBG_CFG		_CKGEN(0x28C)
#define VLP_FQMTR_CON0		_VLPCK(0x230)
#define VLP_FQMTR_CON1		_VLPCK(0x234)

static const struct fmeter_clk fclks[] = {
	/* CKGEN Part */
	FMCLK2(CKGEN, FM_AXI_CK, "fm_axi_ck", 0x0010, 7),
	FMCLK2(CKGEN, FMP_HAXI_CK, "fmp_haxi_ck", 0x0010, 15),
	FMCLK2(CKGEN, FMP_AXI_CK, "fmp_axi_ck", 0x0010, 23),
	FMCLK2(CKGEN, FM_B, "fm_b", 0x0010, 31),
	FMCLK2(CKGEN, FM_MEM_SUB_CK, "fm_mem_sub_ck", 0x0020, 7),
	FMCLK2(CKGEN, FM_DVFSRC_CK, "fm_dvfsrc_ck", 0x0020, 15),
	FMCLK2(CKGEN, FM_MM_CK, "fm_mm_ck", 0x0020, 23),
	FMCLK2(CKGEN, FM_DBI_PRE, "fm_dbi_pre", 0x0020, 31),
	FMCLK2(CKGEN, FM_DISP_PWM_CK, "fm_disp_pwm_ck", 0x0030, 7),
	FMCLK2(CKGEN, FM_MFG_REF_CK, "fm_mfg_ref_ck", 0x0030, 15),
	FMCLK2(CKGEN, FM_UART_CK, "fm_uart_ck", 0x0030, 23),
	FMCLK2(CKGEN, FM_MSDC5HCLK_CK, "fm_msdc5hclk_ck", 0x0030, 31),
	FMCLK2(CKGEN, FM_MSDC50_0_CK, "fm_msdc50_0_ck", 0x0040, 7),
	FMCLK2(CKGEN, FM_MSDC30_1_CK, "fm_msdc30_1_ck", 0x0040, 15),
	FMCLK2(CKGEN, FM_MSDC_MACRO_CK, "fm_msdc_macro_ck", 0x0040, 23),
	FMCLK2(CKGEN, FM_AUDIO_CK, "fm_audio_ck", 0x0040, 31),
	FMCLK2(CKGEN, FM_AUD_INTBUS_CK, "fm_aud_intbus_ck", 0x0050, 7),
	FMCLK2(CKGEN, FM_AUD_ENGEN1_CK, "fm_aud_engen1_ck", 0x0050, 15),
	FMCLK2(CKGEN, FM_AUD_ENGEN2_CK, "fm_aud_engen2_ck", 0x0050, 23),
	FMCLK2(CKGEN, FM_AUD_1_CK, "fm_aud_1_ck", 0x0050, 31),
	FMCLK2(CKGEN, FM_AUD_2_CK, "fm_aud_2_ck", 0x0060, 7),
	FMCLK2(CKGEN, FM_ATB_CK, "fm_atb_ck", 0x0060, 15),
	FMCLK2(CKGEN, FM_I2C_CK, "fm_i2c_ck", 0x0060, 23),
	FMCLK2(CKGEN, FM_TL_CK, "fm_tl_ck", 0x0060, 31),
	FMCLK2(CKGEN, FM_DPMAIF_MAIN_CK, "fm_dpmaif_main_ck", 0x0070, 7),
	FMCLK2(CKGEN, FM_MCUPM_CK, "fm_mcupm_ck", 0x0070, 15),
	FMCLK2(CKGEN, FM_SFLASH_CK, "fm_sflash_ck", 0x0070, 23),
	FMCLK2(CKGEN, FM_SPI_CK, "fm_spi_ck", 0x0070, 31),
	FMCLK2(CKGEN, FM_SPIS_CK, "fm_spis_ck", 0x0080, 7),
	FMCLK2(CKGEN, FM_NFI1X_CK, "fm_nfi1x_ck", 0x0080, 15),
	FMCLK2(CKGEN, FM_SPINFI_BCLK_CK, "fm_spinfi_bclk_ck", 0x0080, 23),
	FMCLK2(CKGEN, FM_GCPU_CK, "fm_gcpu_ck", 0x0080, 31),
	FMCLK2(CKGEN, FM_ECC_CK, "fm_ecc_ck", 0x0090, 7),
	FMCLK2(CKGEN, FM_HSM_CRYPTO_CK, "fm_hsm_crypto_ck", 0x0090, 15),
	FMCLK2(CKGEN, FM_HSM_ARC_CK, "fm_hsm_arc_ck", 0x0090, 23),
	FMCLK2(CKGEN, FM_EIP97_CK, "fm_eip97_ck", 0x0090, 31),
	FMCLK2(CKGEN, FM_SNPS_ETH_312P5M_CK, "fm_snps_eth_312p5m_ck", 0x00A0, 7),
	FMCLK2(CKGEN, FM_SNPS_ETH_250M_CK, "fm_snps_eth_250m_ck", 0x00A0, 15),
	FMCLK2(CKGEN, FM_ETH_62P4M_CK, "fm_eth_62p4m_ck", 0x00A0, 23),
	FMCLK2(CKGEN, FM_SNPS_ETH_50M_RMII_CK, "fm_snps_eth_50m_rmii_ck", 0x00A0, 31),
	FMCLK2(CKGEN, FM_NETSYS_500M_CK, "fm_netsys_500m_ck", 0x00B0, 7),
	FMCLK2(CKGEN, FM_NETSYS_CK, "fm_netsys_ck", 0x00B0, 15),
	FMCLK2(CKGEN, FM_NETSYS_2X_CK, "fm_netsys_2x_ck", 0x00B0, 23),
	FMCLK2(CKGEN, FM_NETSYS_WED_MCU_CK, "fm_netsys_wed_mcu_ck", 0x00B0, 31),
	FMCLK2(CKGEN, FM_NETSYS_MED_MCU_CK, "fm_netsys_med_mcu_ck", 0x00C0, 7),
	FMCLK2(CKGEN, FM_SGMII_0_CK, "fm_sgmii_0_ck", 0x00C0, 15),
	FMCLK2(CKGEN, FM_SGMII_SBUS_0_CK, "fm_sgmii_sbus_0_ck", 0x00C0, 23),
	FMCLK2(CKGEN, FM_SGMII_1_CK, "fm_sgmii_1_ck", 0x00C0, 31),
	FMCLK2(CKGEN, FM_SGMII_SBUS_1_CK, "fm_sgmii_sbus_1_ck", 0x00D0, 7),
	FMCLK2(CKGEN, FM_USXGMII_SBUS_0_CK, "fm_usxgmii_sbus_0_ck", 0x00D0, 15),
	FMCLK2(CKGEN, FM_USXGMII_SBUS_1_CK, "fm_usxgmii_sbus_1_ck", 0x00D0, 23),
	FMCLK2(CKGEN, FM_AP2CONN_HOST_CK, "fm_ap2conn_host_ck", 0x00D0, 31),
	FMCLK2(CKGEN, FM_USB_CK, "fm_usb_ck", 0x00E0, 7),
	FMCLK2(CKGEN, FM_USB_XHCI_CK, "fm_usb_xhci_ck", 0x00E0, 15),
	FMCLK2(CKGEN, FM_EMI_N_CK, "fm_emi_n_ck", 0x00E0, 23),
	FMCLK2(CKGEN, FM_HSM_HSAH_CK, "fm_hsm_hsah_ck", 0x00E0, 31),
	FMCLK2(CKGEN, FM_RSA_CK, "fm_rsa_ck", 0x00F0, 7),
	FMCLK2(CKGEN, FM_MSDC_OCC_400_CK, "fm_msdc_occ_400_ck", 0x00F0, 15),
	FMCLK2(CKGEN, FM_MSDC_OCC_200_CK, "fm_msdc_occ_200_ck", 0x00F0, 23),
	FMCLK2(CKGEN, FM_DXCC_CK, "fm_dxcc_ck", 0x00F0, 31),
	FMCLK2(CKGEN, FM_PCIE_250M_CK, "fm_pcie_250m_ck", 0x0100, 7),
	FMCLK2(CKGEN, FM_DSI_OCC_CK, "fm_dsi_occ_ck", 0x0100, 15),
	FMCLK2(CKGEN, FM_EMI_INTERFACE_546_CK, "fm_emi_interface_546_ck", 0x0100, 23),
	/* ABIST Part */
	FMCLK(ABIST, FM_MSDC22_IN_CK, "fm_msdc22_in_ck"),
	FMCLK(ABIST, FM_MSDC21_IN_CK, "fm_msdc21_in_ck"),
	FMCLK(ABIST, FM_MSDC12_IN_CK, "fm_msdc12_in_ck"),
	FMCLK(ABIST, FM_MSDC11_IN_CK, "fm_msdc11_in_ck"),
	FMCLK(ABIST, FM_MSDC02_IN_CK, "fm_msdc02_in_ck"),
	FMCLK(ABIST, FM_MSDC01_IN_CK, "fm_msdc01_in_ck"),
	FMCLK(ABIST, FM_LVTS_CKMON_LM, "fm_lvts_ckmon_lm"),
	FMCLK(ABIST, FM_LVTS_CKMON_L9, "fm_lvts_ckmon_l9"),
	FMCLK(ABIST, FM_LVTS_CKMON_L2, "fm_lvts_ckmon_l2"),
	FMCLK(ABIST, FM_LVTS_CKMON_L1, "fm_lvts_ckmon_l1"),
	FMCLK(ABIST, FM_RCLRPLL_DIV4_CHC, "fm_rclrpll_div4_chc"),
	FMCLK(ABIST, FM_RPHYPLL_DIV4_CHC, "fm_rphypll_div4_chc"),
	FMCLK(ABIST, FM_RCLRPLL_DIV4_CHA, "fm_rclrpll_div4_cha"),
	FMCLK(ABIST, FM_RPHYPLL_DIV4_CHA, "fm_rphypll_div4_cha"),
	FMCLK(ABIST, FM_DSI0_LNTC_DSICLK, "fm_dsi0_lntc_dsiclk"),
	FMCLK(ABIST, FM_DSI0_MPPLL_TST_CK, "fm_dsi0_mppll_tst_ck"),
	FMCLK(ABIST, FM_CLKSQ1_FS26M_SOC_GUIDE, "fm_clksq1_fs26m_soc_guide"),
	FMCLK(ABIST, FM_ULPOSC2_MON_V_VCORE_CK, "fm_ulposc2_mon_v_vcore_ck"),
	FMCLK(ABIST, FM_ULPOSC_MON_VCROE_CK, "fm_ulposc_mon_vcroe_ck"),
	FMCLK(ABIST, FM_WBG_DIG_BPLL_CK, "fm_wbg_dig_bpll_ck"),
	FMCLK(ABIST, FM_466M_FMEM_INFRASYS, "fm_466m_fmem_infrasys"),
	FMCLK(ABIST, FM_MCUSYS_ARM_OUT_ALL, "fm_mcusys_arm_out_all"),
	FMCLK(ABIST, FM_F32K_VCORE_CK, "fm_f32k_vcore_ck"),
	FMCLK(ABIST, FM_MMPLL_D3_CK, "fm_mmpll_d3_ck"),
	FMCLK(ABIST, FM_UNIVPLL_DIV3_CK, "fm_univpll_div3_ck"),
	FMCLK(ABIST, FM_MAINPLL_DIV3_CK, "fm_mainpll_div3_ck"),
	FMCLK(ABIST, FM_MPLL_CK, "fm_mpll_ck"),
	FMCLK(ABIST, FM_SGMIIPLL_CK, "fm_sgmiipll_ck"),
	FMCLK(ABIST, FM_MEDMCUPLL_CK, "fm_medmcupll_ck"),
	FMCLK(ABIST, FM_WEDMCUPLL_CK, "fm_wedmcupll_ck"),
	FMCLK(ABIST, FM_NET2PLL_CK, "fm_net2pll_ck"),
	FMCLK(ABIST, FM_APLL2_CK, "fm_apll2_ck"),
	FMCLK(ABIST, FM_APLL1_CK, "fm_apll1_ck"),
	FMCLK(ABIST, FM_NET1PLL_CK, "fm_net1pll_ck"),
	FMCLK(ABIST, FM_MMPLL_CK, "fm_mmpll_ck"),
	FMCLK(ABIST, FM_UNIVPLL_CK, "fm_univpll_ck"),
	FMCLK(ABIST, FM_MAINPLL_CK, "fm_mainpll_ck"),
	FMCLK(ABIST, FM_MFGPLL_CK, "fm_mfgpll_ck"),
	FMCLK(ABIST, FM_EMIPLL_CK, "fm_emipll_ck"),
	FMCLK(ABIST, FM_MSDCPLL_CK, "fm_msdcpll_ck"),
	FMCLK(ABIST, FM_CCIPLL_CK, "fm_ccipll_ck"),
	FMCLK(ABIST, FM_ARMPLL_LL_CK, "fm_armpll_ll_ck"),
	FMCLK(ABIST, FM_MPLL_CKDIV_CK, "fm_mpll_ckdiv_ck"),
	FMCLK(ABIST, FM_SGMIIPLL_CKDIV_CK, "fm_sgmiipll_ckdiv_ck"),
	FMCLK(ABIST, FM_MEDMCUPLL_CKDIV_CK, "fm_medmcupll_ckdiv_ck"),
	FMCLK(ABIST, FM_WEDMCUPLL_CKDIV_CK, "fm_wedmcupll_ckdiv_ck"),
	FMCLK(ABIST, FM_NET2PLL_CKDIV_CK, "fm_net2pll_ckdiv_ck"),
	FMCLK(ABIST, FM_APLL2_CKDIV_CK, "fm_apll2_ckdiv_ck"),
	FMCLK(ABIST, FM_APLL1_CKDIV_CK, "fm_apll1_ckdiv_ck"),
	FMCLK(ABIST, FM_NET1PLL_CKDIV_CK, "fm_net1pll_ckdiv_ck"),
	FMCLK(ABIST, FM_UNIVPLL_192M_CK, "fm_univpll_192m_ck"),
	FMCLK(ABIST, FM_MAINPLL_CKDIV_CK, "fm_mainpll_ckdiv_ck"),
	FMCLK(ABIST, FM_MFGPLL_CKDIV_CK, "fm_mfgpll_ckdiv_ck"),
	FMCLK(ABIST, FM_EMIPLL_CKDIV_CK, "fm_emipll_ckdiv_ck"),
	FMCLK(ABIST, FM_MSDCPLL_CKDIV_CK, "fm_msdcpll_ckdiv_ck"),
	FMCLK(ABIST, FM_CCIPLL_CKDIV_CK, "fm_ccipll_ckdiv_ck"),
	FMCLK(ABIST, FM_ARMPLL_LL_CKDIV_CK, "fm_armpll_ll_ckdiv_ck"),
	FMCLK(ABIST, FM_PLLGP_MON_FM_CK, "fm_pllgp_mon_fm_ck"),
	FMCLK(ABIST, FM_CLKMON0_CK, "fm_clkmon0_ck"),
	FMCLK(ABIST, FM_CLKMON1_CK, "fm_clkmon1_ck"),
	FMCLK(ABIST, FM_CLKMON2_CK, "fm_clkmon2_ck"),
	FMCLK(ABIST, FM_CLKMON3_CK, "fm_clkmon3_ck"),
	/* ABIST_2 Part */
	FMCLK(ABIST_2, FM_AUD_I2S0_M_CK, "fm_aud_i2s0_m_ck"),
	FMCLK(ABIST_2, FM_AUD_I2S1_M_CK, "fm_aud_i2s1_m_ck"),
	FMCLK(ABIST_2, FM_AUD_I2S2_M_CK, "fm_aud_i2s2_m_ck"),
	FMCLK(ABIST_2, FM_AUD_I2S4_M_CK, "fm_aud_i2s4_m_ck"),
	FMCLK(ABIST_2, FM_AUD_TDMOUT_M_CK, "fm_aud_tdmout_m_ck"),
	FMCLK(ABIST_2, FM_AUD_TDMOUT_B_CK, "fm_aud_tdmout_b_ck"),
	FMCLK(ABIST_2, FM_AUD_I2S5_M_CK, "fm_aud_i2s5_m_ck"),
	FMCLK(ABIST_2, FM_AUD_I2S6_M_CK, "fm_aud_i2s6_m_ck"),
	FMCLK(ABIST_2, FM_SFLASH_CK_2, "fm_sflash_ck_2"),
	FMCLK(ABIST_2, FM_UNIPLL_SES_CK, "fm_unipll_ses_ck"),
	FMCLK(ABIST_2, FM_SRCK_CK, "fm_srck_ck"),
	FMCLK(ABIST_2, FM_MAIN_H728M_CK, "fm_main_h728m_ck"),
	FMCLK(ABIST_2, FM_MAIN_H546M_CK, "fm_main_h546m_ck"),
	FMCLK(ABIST_2, FM_MAIN_H436P8M_CK, "fm_main_h436p8m_ck"),
	FMCLK(ABIST_2, FM_MAIN_H364M_CK, "fm_main_h364m_ck"),
	FMCLK(ABIST_2, FM_MAIN_H312M_CK, "fm_main_h312m_ck"),
	FMCLK(ABIST_2, FM_UNIVPLL_1248M_CK, "fm_univpll_1248m_ck"),
	FMCLK(ABIST_2, FM_UNIV_832M_CK, "fm_univ_832m_ck"),
	FMCLK(ABIST_2, FM_UNIV_624M_CK, "fm_univ_624m_ck"),
	FMCLK(ABIST_2, FM_UNIV_499M_CK, "fm_univ_499m_ck"),
	FMCLK(ABIST_2, FM_UNIV_416M_CK, "fm_univ_416m_ck"),
	FMCLK(ABIST_2, FM_UNIV_356P6M_CK, "fm_univ_356p6m_ck"),
	FMCLK(ABIST_2, FM_MMPLL_D3_CK_2, "fm_mmpll_d3_ck_2"),
	FMCLK(ABIST_2, FM_MMPLL_D4_CK, "fm_mmpll_d4_ck"),
	FMCLK(ABIST_2, FM_MMPLL_D5_CK, "fm_mmpll_d5_ck"),
	FMCLK(ABIST_2, FM_MMPLL_D6_CK, "fm_mmpll_d6_ck"),
	FMCLK(ABIST_2, FM_MMPLL_D7_CK, "fm_mmpll_d7_ck"),
	FMCLK(ABIST_2, FM_MMPLL_D9_CK, "fm_mmpll_d9_ck"),
	FMCLK(ABIST_2, FM_DBI_CK, "fm_dbi_ck"),
	FMCLK(ABIST_2, FM_EINT_E_MCLK, "fm_eint_e_mclk"),
	FMCLK(ABIST_2, FM_EINT_S_MCLK, "fm_eint_s_mclk"),
	FMCLK(ABIST_2, FM_EINT_N_MCLK, "fm_eint_n_mclk"),
	FMCLK(ABIST_2, FM_EINT_W_MCLK, "fm_eint_w_mclk"),
	FMCLK(ABIST_2, FM_AP2CONN_OSC_CK, "fm_ap2conn_osc_ck"),
	FMCLK(ABIST_2, FM_USB_FMCNT_CK, "fm_usb_fmcnt_ck"),
	FMCLK(ABIST_2, FMP_250M_CK, "fmp_250m_ck"),
	FMCLK(ABIST_2, FM_UARTHUB_FBCLK_104M_CK, "fm_uarthub_fbclk_104m_ck"),
	FMCLK(ABIST_2, FM_SGMII_REF_XTAL_0_CK, "fm_sgmii_ref_xtal_0_ck"),
	FMCLK(ABIST_2, FM_SGMII_REF_XTAL_1_CK, "fm_sgmii_ref_xtal_1_ck"),
	FMCLK(ABIST_2, FM_XFI_PHY0_CK, "fm_xfi_phy0_ck"),
	FMCLK(ABIST_2, FM_XFI_PHY1_CK, "fm_xfi_phy1_ck"),
	FMCLK(ABIST_2, FM_XFI_PLL_REF_XTAL_CK, "fm_xfi_pll_ref_xtal_ck"),
	FMCLK(ABIST_2, FM_INFRA_PCCIF_SLOW_CK, "fm_infra_pccif_slow_ck"),
	FMCLK(ABIST_2, FM_PMSRCK_CK, "fm_pmsrck_ck"),
	FMCLK(ABIST_2, FM_EMI_INTERFACE_624_CK, "fm_emi_interface_624_ck"),
	/* VLPCK Part */
	FMCLK(VLPCK, FM_SCP_CK, "fm_scp_ck"),
	FMCLK(VLPCK, FM_SPM_CK, "fm_spm_ck"),
	FMCLK(VLPCK, FM_PWRAP_ULPOSC_CK, "fm_pwrap_ulposc_ck"),
	FMCLK(VLPCK, FM_F66M_APXGPT_BCLK_CK, "fm_f66m_apxgpt_bclk_ck"),
	FMCLK(VLPCK, FM_DXCC_CK_2, "fm_dxcc_ck_2"),
	FMCLK(VLPCK, FM_SPMI_P_CK, "fm_spmi_p_ck"),
	FMCLK(VLPCK, FM_SPMI_M_CK, "fm_spmi_m_ck"),
	FMCLK(VLPCK, FM_DVFSRC_CK_2, "fm_dvfsrc_ck_2"),
	FMCLK(VLPCK, FM_PWM_VLP_CK, "fm_pwm_vlp_ck"),
	FMCLK(VLPCK, FM_AXI_VLP_CK, "fm_axi_vlp_ck"),
	FMCLK(VLPCK, FM_DBGAO_26M_CK, "fm_dbgao_26m_ck"),
	FMCLK(VLPCK, FM_SYSTIMER_26M_CK, "fm_systimer_26m_ck"),
	FMCLK(VLPCK, FM_PWRMCU_CK, "fm_pwrmcu_ck"),
	FMCLK(VLPCK, FM_SSPM_F26M_CK, "fm_sspm_f26m_ck"),
	FMCLK(VLPCK, FM_APEINT_66M_CK, "fm_apeint_66m_ck"),
	FMCLK(VLPCK, FM_SRCK_CK_2, "fm_srck_ck_2"),
	FMCLK(VLPCK, FM_SRAMRC_CK, "fm_sramrc_ck"),
	FMCLK(VLPCK, FM_SEJ_26M_CK, "fm_sej_26m_ck"),
	FMCLK(VLPCK, FM_MD_BUCK_26M_CK, "fm_md_buck_26m_ck"),
	FMCLK(VLPCK, FM_SSPM_ULPOSC_CK, "fm_sspm_ulposc_ck"),
	FMCLK(VLPCK, FM_DBGAO_66M_CK, "fm_dbgao_66m_ck"),
	FMCLK(VLPCK, FM_RTC_CK, "fm_rtc_ck"),
	FMCLK(VLPCK, FM_ULPOSC_CORE_CK, "fm_ulposc_core_ck"),
	FMCLK(VLPCK, FM_ULPOSC_CK, "fm_ulposc_ck"),
	FMCLK(VLPCK, FM_SCP_SPI_CK, "fm_scp_spi_ck"),
	FMCLK(VLPCK, FM_SCP_IIC_CK, "fm_scp_iic_ck"),
	FMCLK(VLPCK, FM_OSC_SYNC_CK, "fm_osc_sync_ck"),
	FMCLK(VLPCK, FM_OSC_SYNC_2, "fm_osc_sync_2"),
	FMCLK(VLPCK, FM_TL_VLP_CK, "fm_tl_vlp_ck"),
	FMCLK(VLPCK, FM_HSM_ARC_CK_2, "fm_hsm_arc_ck_2"),
	FMCLK(VLPCK, FM_HSM_CRYPTO_CK_2, "fm_hsm_crypto_ck_2"),
	{},
};

//same as Pichon
unsigned int mt_get_abist_freq(unsigned int ID)
{
    int output = 0, i = 0, flag = 0;
    unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1, clk_div, post_div;

    // CLK26CALI_0[15]: rst 1 -> 0
    clk_writel(CLK26CALI_0, (clk_readl(CLK26CALI_0) & 0xFFFF7FFF));
    // CLK26CALI_0[15]: rst 0 -> 1
    clk_writel(CLK26CALI_0, (clk_readl(CLK26CALI_0) | 0x00008000));

    // sel abist_cksw and enable freq meter sel abist
    clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
    clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFF80FFFC)|(ID << 16));

    // select divider, div 4
    clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
    clk_writel(CLK_MISC_CFG_0,
        (clk_misc_cfg_0 & 0x00FFFFFF) | (3 << 24));

    clk26cali_1 = clk_readl(CLK26CALI_1);
    clk_writel(CLK26CALI_0, 0x9000);
    clk_writel(CLK26CALI_0, 0x9010);

    /* wait frequency meter finish */
    while (clk_readl(CLK26CALI_0) & 0x10)
    {
        udelay(10);
        i++;
        if(i > 100) {
            flag = 1;
            break;
        }
    }

    if(flag)
        output = 0;
    else {
        temp = clk_readl(CLK26CALI_1) & 0xFFFF;

        output = ((temp * 26000) ) / 1024; // Khz
    }

    clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
    clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
    clk_writel(CLK26CALI_0, 0x8000);
    clk_writel(CLK26CALI_1, clk26cali_1);

//    pal_log_info("abist meter[%d] = %d Khz)\n", ID, output * 4);
    return output * 4 /** clk_div / post_div*/;
}

unsigned int mt_get_abist2_freq(unsigned int ID)
{
    int output = 0, i = 0, flag = 0;
    unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1, clk_div, post_div;

    // CLK26CALI_0[15]: rst 1 -> 0
    clk_writel(CLK26CALI_0, (clk_readl(CLK26CALI_0) & 0xFFFF7FFF));
    // CLK26CALI_0[15]: rst 0 -> 1
    clk_writel(CLK26CALI_0, (clk_readl(CLK26CALI_0) | 0x00008000));

    //sel abist2_cksw and enable freq meter sel abist2
    clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
    clk_writel(CLK_DBG_CFG,
            (clk_dbg_cfg & 0xE0FFFFFC)|(ID << 24) | (0x2));

    // select divider, div 4
    clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
    clk_writel(CLK_MISC_CFG_0,
            (clk_misc_cfg_0 & 0x00FFFFFF) | (3 << 24));

    clk26cali_1 = clk_readl(CLK26CALI_1);
    clk_writel(CLK26CALI_0, 0x9000);
    clk_writel(CLK26CALI_0, 0x9010);

    /* wait frequency meter finish */
    while (clk_readl(CLK26CALI_0) & 0x10)
    {
        udelay(10);
        i++;
        if(i > 100) {
            flag = 1;
            break;
        }
    }

    if(flag)
        output = 0;
    else {
        temp = clk_readl(CLK26CALI_1) & 0xFFFF;

        output = ((temp * 26000) ) / 1024; // Khz
    }

    clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
    clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
    clk_writel(CLK26CALI_0, 0x8000);
    clk_writel(CLK26CALI_1, clk26cali_1 & 0xFFFF0000);

//    pal_log_info("abist2 meter[%d] = %d Khz\n", ID, output * 4 );
    return output * 4 /* *clk_div / post_div*/;
}

unsigned int mt_get_ckgen_freq(unsigned int ID)
{
    int output = 0, i = 0, flag = 0;
    unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1, clk_div;

    // CLK26CALI_0[15]: rst 1 -> 0
    clk_writel(CLK26CALI_0, (clk_readl(CLK26CALI_0) & 0xFFFF7FFF));
    // CLK26CALI_0[15]: rst 0 -> 1
    clk_writel(CLK26CALI_0, (clk_readl(CLK26CALI_0) | 0x00008000));

    //sel ckgen_cksw[22] and enable freq meter sel ckgen[21:16], 01:hd_faxi_ck
    clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
    clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFFF80FC)|(ID << 8)|(0x1));

    // select divider?dvt set zero
    clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
    clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));

    clk26cali_1 = clk_readl(CLK26CALI_1);
    clk_writel(CLK26CALI_0, 0x9000);
    clk_writel(CLK26CALI_0, 0x9010);

    /* wait frequency meter finish */
    while (clk_readl(CLK26CALI_0) & 0x10)
    {
        udelay(10);
        i++;
        if(i > 100) {
            flag = 1;
            break;
        }
    }

    if(flag)
        output = 0;
    else {
        temp = clk_readl(CLK26CALI_1) & 0xFFFF;

        output = ((temp * 26000) ) / 1024; // Khz
    }

//    pal_log_info("ckgen meter[%d] = %d Khz(0x%x)\n", ID, output, clk_readl(CLK_DBG_CFG));

    clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
    clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
    clk_writel(CLK26CALI_0, 0x8000);
    clk_writel(CLK26CALI_1, clk26cali_1);

    return output;
}

unsigned int mt_get_vlpck_freq(unsigned int ID)
{
    int output = 0, i = 0;
    unsigned int temp, con0, con1;

    // CLK26CALI_0[15]: rst 1 -> 0
    clk_writel(VLP_FQMTR_CON0, 0);
    // CLK26CALI_0[15]: rst 0 -> 1
    clk_writel(VLP_FQMTR_CON0, (clk_readl(VLP_FQMTR_CON0) | 0x00008000));

    con0 = clk_readl(VLP_FQMTR_CON0);
    // vlp freq meter sel ckgen[20:16]
    // other subsys freq meter sel ckgen[2:0]
    clk_writel(VLP_FQMTR_CON0, (con0 & 0xFFE0FFFF)|(ID << 16));

    con1 = clk_readl(VLP_FQMTR_CON1);
    clk_writel(VLP_FQMTR_CON1, (con1 & 0x0000FFFF) | (0x1FF << 16));

    // select divider?dvt set zero
    con0 = clk_readl(VLP_FQMTR_CON0);
    clk_writel(VLP_FQMTR_CON0, (con0 & 0x00FFFFFF));

    clk_writel(VLP_FQMTR_CON0, (clk_readl(VLP_FQMTR_CON0) | 0x1000));
    clk_writel(VLP_FQMTR_CON0, (clk_readl(VLP_FQMTR_CON0) | 0x10));

    /* wait frequency meter finish */
    udelay(VLP_FM_WAIT_TIME);

    temp = clk_readl(VLP_FQMTR_CON1) & 0xFFFF;

    output = ((temp * 26000) ) / 512; // Khz

    clk_writel(VLP_FQMTR_CON0, 0x8000);

    //pr_info("vlp_cksys meter[%d] = %d Khz\n", ID, output);
    return output;
}

 static u32 fmeter_freq_op(const struct fmeter_clk *fclk)
{
	if (fclk->type == ABIST)
		return mt_get_abist_freq(fclk->id);
	else if (fclk->type == ABIST_2)
		return mt_get_abist2_freq(fclk->id);
	else if (fclk->type == CKGEN)
		return mt_get_ckgen_freq(fclk->id);
	else if (fclk->type == VLPCK)
		return mt_get_vlpck_freq(fclk->id);
	return 0;
}

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	return fclks;
}





/*
 * clkdbg pwr_status
 */
static u32 pwr_ofs[STA_NUM] = {
	[PWR_STA] = 0xF38,
	[PWR_STA2] = 0xF3C,
};

u32 *get_spm_pwr_status(void)
{
	static void __iomem *scpsys_base, *pwr_addr[STA_NUM];
	static u32 pwr_sta[STA_NUM];
	int i;

	for (i = 0; i < STA_NUM; i++) {
		if (!scpsys_base)
			scpsys_base = ioremap(0x12001000, PAGE_SIZE);

		if (pwr_ofs[i] && !pwr_sta[i]) {
			pwr_addr[i] = scpsys_base + pwr_ofs[i];
			pwr_sta[i] = clk_readl(pwr_addr[i]);
		}
	}

	return pwr_sta;
}

/*
 * init functions
 */

static struct clkdbg_ops clkdbg_mt6990_ops = {
	.get_all_fmeter_clks = get_all_fmeter_clks,
	.prepare_fmeter = NULL,
	.unprepare_fmeter = NULL,
	.fmeter_freq = fmeter_freq_op,
	.get_all_clk_names = get_all_clk_names,
	.get_spm_pwr_status = get_spm_pwr_status,
};

void clkdbg_set_cfg(void)
{
	set_clkdbg_ops(&clkdbg_mt6990_ops);
}

