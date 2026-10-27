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

#ifdef CONFIG_MTK_DEVAPC
#include <linux/soc/mediatek/devapc_public.h>
#endif

#include "clkdbg.h"
#include "clkchk.h"

/*
 * clkdbg dump_state
 */

static const char * const clk_names[] = {
    /* topckgen */
    "axi_sel",
    "spm_sel",
    "bus_aximem_sel",
    "mm_sel",
    "mfg_ref_sel",
    "mfg_sel",
    "uart_sel",
    "msdc50_0_h_sel",
    "msdc50_0_sel",
    "msdc30_1_sel",
    "audio_sel",
    "aud_intbus_sel",
    "aud_engen1_sel",
    "aud_engen2_sel",
    "aud_1_sel",
    "aud_2_sel",
    "pwrap_ulposc_sel",
    "atb_sel",
    "pwrmcu_sel",
    "dbi_sel",
    "disp_pwm_sel",
    "usb_sel",
    "ssusb_xhci_sel",
    "i2c_sel",
    "tl_sel",
    "dpmaif_main_sel",
    "pwm_sel",
    "spmi_m_mst_sel",
    "spmi_p_mst_sel",
    "dvfsrc_sel",
    "mcupm_sel",
    "sflash_sel",
    "gcpu_sel",
    "spi_sel",
    "spis_sel",
    "ecc_sel",
    "nfi1x_sel",
    "spinfi_bclk_sel",
    "netsys_sel",
    "medsys_sel",
    "hsm_crypto_sel",
    "hsm_arc_sel",
    "eip97_sel",
    "snps_eth_312p5m_sel",
    "snps_eth_250m_sel",
    "snps_ptp_sel",
    "snps_rmii_sel",
    "netsys_500m_sel",
    "netsys_med_mcu_sel",
    "netsys_wed_mcu_sel",
    "netsys_2x_sel",
    "sgmii_sel",
    "sgmii_sbus_sel",
    "apll_i2s0_mck_sel",
    "apll_i2s1_mck_sel",
    "apll_i2s2_mck_sel",
    "apll_i2s4_mck_sel",
    "apll_tdmout_mck_sel",
    "apll_i2s5_mck_sel",
    "apll_i2s6_mck_sel",

    /* topckgen */
    "apll12_div0",
    "apll12_div1",
    "apll12_div2",
    "apll12_div4",
    "apll12_div_tdmout_m",
    "apll12_div_tdmout_b",
    "apll12_div5",
    "apll12_div6",

    /* dbgsys_dem */
    "dbgsys_dem_atb_en",
    "dbgsys_dem_busclk_en",
    "dbgsys_dem_sysclk_en",

    /* infracfg_ao */
    "ifrao_pmic_tmr_set",
    "ifrao_pmic_ap_set",
    "ifrao_pmic_md_set",
    "ifrao_pmic_conn_set",
    "ifrao_sej",
    "ifrao_apxgpt",
    "ifrao_mcupm",
    "ifrao_gce",
    "ifrao_gce2",
    "ifrao_therm",
    "ifrao_i2c0",
    "ifrao_i2c1",
    "ifrao_i2c2",
    "ifrao_i2c3",
    "ifrao_pwm_hclk",
    "ifrao_pwm1",
    "ifrao_pwm2",
    "ifrao_pwm3",
    "ifrao_pwm4",
    "ifrao_pwm5",
    "ifrao_pwm",
    "ifrao_uart0",
    "ifrao_uart1",
    "ifrao_uart2",
    "ifrao_uart3",
    "ifrao_gce_26m_set",
    "ifrao_spi0",
    "ifrao_msdc0",
    "ifrao_msdc1",
    "ifrao_msdc0_clk",
    "ifrao_gcpu",
    "ifrao_trng",
    "ifrao_auxadc",
    "ifrao_cpum",
    "ifrao_ccif1_ap",
    "ifrao_ccif1_md",
    "ifrao_auxadc_md",
    "ifrao_pcie_tl_26m",
    "ifrao_msdc1_clk",
    "ifrao_pcie_tl_96m",
    "ifrao_dapc",
    "ifrao_ccif_ap",
    "ifrao_debugsys",
    "ifrao_audio",
    "ifrao_ccif_md",
    "ifrao_devmpu_bclk",
    "ifrao_dramc26",
    "ifrao_ssusb",
    "ifrao_disp_pwm",
    "ifrao_cldmabclk",
    "ifrao_audio26m",
    "ifrao_mdtemp",
    "ifrao_spi1",
    "ifrao_i2c4",
    "ifrao_spi2",
    "ifrao_spi3",
    "ifrao_unipro_tick",
    "ifrao_ufs_bclk",
    "ifrao_md32_bclk",
    "ifrao_unipro_mbist",
    "ifrao_pwm6",
    "ifrao_pwm7",
    "ifrao_i2c_slave",
    "ifrao_i2c1a",
    "ifrao_i2c1_imm",
    "ifrao_i2c2a",
    "ifrao_i2c2_imm",
    "ifrao_ssusb_xhci",
    "ifrao_msdc0sf",
    "ifrao_msdc1sf",
    "ifrao_msdc2sf",
    "ifrao_sspm_26m",
    "ifrao_sspm_32k",
    "ifrao_i2c6",
    "ifrao_ap_msdc0",
    "ifrao_md_msdc0",
    "ifrao_ccif5_ap",
    "ifrao_ccif5_md",
    "ifrao_pcie_h_133m",
    "ifrao_spis_h_66m",
    "ifrao_pcie_peri_26m",
    "ifrao_ccif2_ap",
    "ifrao_ccif2_md",
    "ifrao_sej_f13m",
    "ifrao_aes",
    "ifrao_i2c7",
    "ifrao_i2c8",
    "ifrao_fbist2fpc",
    "ifrao_dpmaif_main",
    "ifrao_pcie_tl_32k",
    "ifrao_ccif4_ap",
    "ifrao_ccif4_md",
    "ifrao_133m_mclk_ck",
    "ifrao_66m_mclk_ck",
    "ifrao_infra_133m",
    "ifrao_infra_66m",
    "ifrao_peru_bus_133m",
    "ifrao_peru_bus_66m",
    "ifrao_133m_cldma_top",
    "ifrao_ecc_top",
    "ifrao_66m_gcpu",
    "ifrao_133m_dwc_ether",
    "ifrao_133m_flashif",
    "ifrao_133m_pcie_p0",
    "ifrao_133m_pcie_p1",
    "ifrao_133m_pcie_p2",
    "ifrao_133m_pcie_p3",
    "ifrao_mmw_dpmaif_ck",
    "ifrao_nfi",
    "ifrao_fpinfi_bclk_ck",
    "ifrao_66m_nfi_h_ck",
    "ifrao_fspis_ck",
    "ifrao_26m_p1",
    "ifrao_26m_p2",
    "ifrao_26m_p3",
    "ifrao_flash_26m",
    "ifrao_sflash_ck",

    /* apmixedsys */
    "armpll_ll",
    "ccipll",
    "mpll",
    "mainpll",
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

    /* gce */
    "gce_0",

    /* audiosys */
    "aud_afe",
    "aud_22m",
    "aud_24m",
    "aud_apll2_tuner",
    "aud_apll_tuner",
    "aud_tdm_ck",
    "aud_adc",
    "aud_dac",
    "aud_dac_predis",
    "aud_tml",
    "aud_i2s0_bclk",
    "aud_i2s1_bclk",
    "aud_i2s2_bclk",
    "aud_i2s4_bclk",
    "aud_i2s5_bclk",
    "aud_i2s6_bclk",
    "aud_general1_asrc",
    "aud_general2_asrc",
    "aud_adda6_adc",
    "aud_connsys_i2s_asrc",
    "aud_afe_src_pcm_tx",
    "aud_afe_src_pcm_tx2",
    "aud_afe_src_pcm_tx3",
    "aud_afe_src_pcm_rx",
    "aud_afe_src_i2sin",
    "aud_afe_src_i2sout",

    /* imp_iic_wrap_e */
    "impe_i2c0_ro",
    "impe_i2c1_ro",
    "impe_i2c2_ro",
    "impe_i2c3_ro",
    "impe_i2c4_ro",
    "impe_i2c5_ro",

    /* mfgsys */
    "mfgcfg_bg3d",

    /* mmsys */
    "mmsys_mutex0",
    "mmsys_apb_bus",
    "mm_mdp_rsz0",
    "mm_disp_gamma0",
    "mm_mdp_wrot0",
    "mm_disp_color0",
    "mm_disp_ccorr0",
    "mm_disp_aal0",
    "mm_disp_rdma0",
    "mm_mdp_rdma0",
    "mmsys_fake_eng0",
    "mm_disp_dither0",
    "mm_disp_wdma0",
    "mm_mdp_tdshp0",
    "mm_disp_ovl0",
    "mm_dbpi0",
    "mm_disp_dsi0",
    "mmsys_smi_common",
    "disp_axi",
    "dsi",
    "dbi",
};

static const char * const *get_all_clk_names(void)
{
	return clk_names;
}

/*
 * clkdbg pwr_status
 */
static u32 pwr_ofs[STA_NUM] = {
	[PWR_STA] = 0x16C,
	[PWR_STA2] = 0x170,
	[OTHER_STA] = 0x178,
};

u32 *get_spm_pwr_status(void)
{
	static void __iomem *scpsys_base, *pwr_addr[STA_NUM];
	static u32 pwr_sta[STA_NUM];
	int i;

	for (i = 0; i < STA_NUM; i++) {
		if (!scpsys_base)
			scpsys_base = ioremap(0x10006000, PAGE_SIZE);

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

static struct clkdbg_ops clkdbg_mt6890_ops = {
	.get_all_fmeter_clks = NULL,
	.prepare_fmeter = NULL,
	.unprepare_fmeter = NULL,
	.fmeter_freq = NULL,
	.get_all_clk_names = get_all_clk_names,
	.get_spm_pwr_status = get_spm_pwr_status,
};

void clkdbg_set_cfg(void)
{
	set_clkdbg_ops(&clkdbg_mt6890_ops);
}

