/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef _CLK_MT6880_FMETER_H
#define _CLK_MT6880_FMETER_H

/* generate from clock_table.xlsx from TOPCKGEN DE */

/* CKGEN Part */
#define FM_AXI_CK				1
#define FM_SPM_CK				2
#define FM_BUS_CK				3
#define FM_MM_CK				4
#define FM_MFG_REF_CK				5
#define FM_FUART_CK				6
#define FM_MSDC50_0_H_CK			7
#define FM_MSDC50_0_CK				8
#define FM_MSDC30_1_CK				9
#define FM_AUDIO_CK				10
#define FM_AUD_INTBUS_CK			11
#define FM_AUD_ENGEN1_CK			12
#define FM_AUD_ENGEN2_CK			13
#define FM_AUD1_CK				14
#define FM_AUD2_CK				15
#define FM_FPWRAP_ULPOSC_CK			16
#define FM_ATB_CK				17
#define FM_PWRMCU_CK				18
#define FM_DBI_CK				19
#define FM_FDISP_PWM_CK				20
#define FM_FUSB_CK				21
#define FM_FSSUSB_XHCI_CK			22
#define FM_I2C_CK				23
#define FM_TL_CK				24
#define FM_DPMAIF_MAIN_CK			25
#define FM_PWM_CK				26
#define FM_SPMI_M_MST_CK			27
#define FM_SPMI_P_MST_CK			28
#define FM_DVFSRC_CK				29
#define FM_MCUPM_CK				30
#define FM_SFLASH_CK				31
#define FM_GCPU_CK				32
#define FM_SPI_CK				33
#define FM_SPIS_CK				34
#define FM_ECC_CK				35
#define FM_NFI1X_CK				36
#define FM_SPINFI_BCLK_CK			37
#define FM_NETSYS_CK				38
#define FM_MEDSYS_CK				39
#define FM_HSM_CRYPTO_CK			40
#define FM_HSM_ARC_CK				41
#define FM_EIP97_CK				42
#define FM_SNPS_ETH_312P5M_CK			43
#define FM_SNPS_ETH_250M_CK			44
#define FM_SNPS_PTP_CK				45
#define FM_SNPS_ETH_50M_RMII_CK			46
#define FM_NETSYS_500M_CK			47
#define FM_NETSYS_MED_MCU_CK			48
#define FM_NETSYS_WED_MCU_CK			49
#define FM_NETSYS_2X_CK				50
#define FM_SGMII_CK				51
#define FM_SGMII_SBUS_CK			52
/* ABIST Part */
#define FM_APLL1_CK				2
#define FM_APLL2_CK				3
#define FM_APPLLGP_MON_FM_CK			4
#define FM_ARMPLL_LL_CK				10
#define FM_CCIPLL_CK				11
#define FM_NET1PLL_CK				12
#define FM_NET2PLL_CK				13
#define FM_WEDMCUPLL_CK				14
#define FM_MEDMCUPLL_CK				15
#define FM_SGMIIPLL_CK				16
#define FM_SNPSETHPLL_CK			17
#define FM_DSI0_LNTC_DSICLK			20
#define FM_DSI0_MPPLL_TST_CK			21
#define FM_MDPLL1_FS26M_DRF_GUIDE		22
#define FM_MFG_CK				23
#define FM_MAINPLL_CK				24
#define FM_MDPLL1_FS26M_GUIDE			25
#define FM_MFGPLL_CK				26
#define FM_MMPLL_CK				27
#define FM_MMPLL_D3_CK				28
#define FM_MPLL_CK				29
#define FM_MSDCPLL_CK				30
#define FM_RCLRPLL_DIV4_CK			31
#define FM_RPHYPLL_DIV4_CK			33
#define FM_ULPOSC_CK				37
#define FM_UNIVPLL_CK				38
#define FMEM_AFT_CH0				43
#define FMEM_AFT_CH1				44
#define FM_TRNG_FREQ_DEBUG_OUT0			45
#define FM_TRNG_FREQ_DEBUG_OUT1			46
#define FMEM_BFE_CH0				47
#define FMEM_BFE_CH1				48
#define FM_466M_FMEM_INFRASYS			49
#define FM_MCUSYS_ARM_OUT_ALL			50
#define FM_RTC32K_I_VAO				57
/* ABIST2 Part */
//#define FM_MCUPM_CK				13
//#define FM_SFLASH_CK				14
#define FM_UNIPLL_SES_CK			15
//#define FM_ULPOSC_CK				16
#define FM_ULPOSC_CORE_CK			17
#define FM_SRCK_CK				18
#define FM_MAINPLL_H728M_CK			19
#define FM_MAINPLL_H546M_CK			20
#define FM_MAINPLL_H436P8M_CK			21
#define FM_MAINPLL_H364M_CK			22
#define FM_MAINPLL_H312M_CK			23
#define FM_UNIVPLL_1248M_CK			24
#define FM_UNIVPLL_832M_CK			25
#define FM_UNIVPLL_624M_CK			26
#define FM_UNIVPLL_499M_CK			27
#define FM_UNIVPLL_416M_CK			28
#define FM_UNIVPLL_356P6M_CK			29
//#define FM_MMPLL_D3_CK				30
#define FM_MMPLL_D4_CK				31
#define FM_MMPLL_D5_CK				32
#define FM_MMPLL_D6_CK				33
#define FM_MMPLL_D7_CK				34
#define FM_MMPLL_D9_CK				35
//#define FM_NET1PLL_CK				36
//#define FM_NET2PLL_CK				37
//#define FM_WEDMCUPLL_CK				38
//#define FM_MEDMCUPLL_CK				39
//#define FM_SGMIIPLL_CK				40

#endif /* _CLK_MT6880_FMETER_H */
