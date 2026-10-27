// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/bug.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <asm-generic/io.h>

#include "devapc-mt6880.h"
#include "devapc-mtk-multi-ao.h"

static struct mtk_device_info mt6880_devices_infra[] = {
	/* sys_idx, ctrl_idx, vio_idx, device, vio_irq */
	/* 0 */
	{0, 0, 0, "MFGSYS", true},
	{0, 1, 1, "MFGSYS", true},
	{0, 2, 2, "MFGSYS", true},
	{0, 3, 3, "MFGSYS", true},
	{0, 4, 4, "MFGSYS", true},
	{0, 5, 5, "MFGSYS", true},
	{0, 6, 6, "MFGSYS", true},
	{0, 7, 7, "MFGSYS", true},
	{0, 8, 8, "MFGSYS", true},
	{0, 9, 9, "NETSYS", true},

	/* 10 */
	{0, 10, 10, "NETSYS", true},
	{0, 11, 11, "NETSYS", true},
	{0, 12, 12, "NETSYS", true},
	{0, 13, 23, "MCUSYS_CFGREG_APB_S", true},
	{0, 14, 24, "MCUSYS_CFGREG_APB_S-1", true},
	{0, 15, 25, "MCUSYS_CFGREG_APB_S-2", true},
	{0, 16, 26, "MCUSYS_CFGREG_APB_S-3", true},
	{0, 17, 27, "MCUSYS_CFGREG_APB_S-4", true},
	{0, 18, 28, "L3C_S", true},
	{0, 19, 29, "L3C_S-1", true},

	/* 20 */
	{0, 20, 77, "HSM_S", true},
	{1, 0, 13, "MEDSYS", true},
	{1, 1, 14, "MEDSYS", true},
	{1, 2, 15, "MEDSYS", true},
	{1, 3, 16, "MEDSYS", true},
	{1, 4, 17, "MEDSYS", true},
	{1, 5, 18, "MEDSYS", true},
	{1, 6, 19, "MEDSYS", true},
	{1, 7, 20, "MEDSYS", true},
	{1, 8, 21, "MEDSYS", true},

	/* 30 */
	{1, 9, 22, "MEDSYS", true},
	{2, 0, 30, "MMSYS", true},
	{2, 1, 31, "MMSYS", true},
	{2, 2, 32, "MMSYS", true},
	{2, 3, 33, "MMSYS", true},
	{2, 4, 34, "MMSYS", true},
	{2, 5, 35, "MMSYS", true},
	{2, 6, 36, "MMSYS", true},
	{2, 7, 37, "MMSYS", true},
	{2, 8, 38, "MMSYS", true},

	/* 40 */
	{2, 9, 39, "MMSYS", true},
	{2, 10, 40, "MMSYS", true},
	{2, 11, 41, "MMSYS", true},
	{2, 12, 42, "MMSYS", true},
	{2, 13, 43, "MMSYS", true},
	{2, 14, 44, "MMSYS", true},
	{2, 15, 45, "MMSYS", true},
	{2, 16, 46, "MMSYS", true},
	{2, 17, 47, "MMSYS", true},
	{2, 18, 48, "MMSYS", true},

	/* 50 */
	{2, 19, 49, "MMSYS", true},
	{2, 20, 50, "MMSYS", true},
	{2, 21, 51, "MMSYS", true},
	{2, 22, 52, "MMSYS", true},
	{2, 23, 53, "MMSYS", true},
	{2, 24, 54, "MMSYS", true},
	{2, 25, 55, "MMSYS", true},
	{2, 26, 56, "MMSYS", true},
	{2, 27, 57, "MMSYS", true},
	{2, 28, 58, "MMSYS", true},

	/* 60 */
	{2, 29, 59, "MMSYS", true},
	{2, 30, 60, "MMSYS", true},
	{2, 31, 61, "MMSYS", true},
	{2, 32, 62, "MMSYS", true},
	{2, 33, 63, "MMSYS", true},
	{2, 34, 64, "MMSYS", true},
	{2, 35, 65, "MMSYS", true},
	{2, 36, 66, "MMSYS", true},
	{2, 37, 67, "MMSYS", true},
	{2, 38, 68, "MMSYS", true},

	/* 70 */
	{2, 39, 69, "MMSYS", true},
	{-1, -1, 70, "OOB_way_en", true},
	{-1, -1, 71, "OOB_way_en", true},
	{-1, -1, 72, "OOB_way_en", true},
	{-1, -1, 73, "OOB_way_en", true},
	{-1, -1, 74, "OOB_way_en", true},
	{-1, -1, 75, "OOB_way_en", true},
	{-1, -1, 76, "OOB_way_en", true},
	{-1, -1, 77, "OOB_way_en", true},
	{-1, -1, 78, "OOB_way_en", true},

	/* 80 */
	{-1, -1, 79, "OOB_way_en", true},
	{-1, -1, 80, "OOB_way_en", true},
	{-1, -1, 81, "OOB_way_en", true},
	{-1, -1, 82, "OOB_way_en", true},
	{-1, -1, 83, "OOB_way_en", true},
	{-1, -1, 84, "OOB_way_en", true},
	{-1, -1, 85, "OOB_way_en", true},
	{-1, -1, 86, "OOB_way_en", true},
	{-1, -1, 87, "OOB_way_en", true},
	{-1, -1, 88, "OOB_way_en", true},

	/* 90 */
	{-1, -1, 89, "OOB_way_en", true},
	{-1, -1, 90, "OOB_way_en", true},
	{-1, -1, 91, "OOB_way_en", true},
	{-1, -1, 92, "OOB_way_en", true},
	{-1, -1, 93, "DECERR", true},
	{-1, -1, 94, "SRAMROM", false},
	{-1, -1, 95, "reserved", false},
	{-1, -1, 96, "reserved", false},
	{-1, -1, 97, "DEVICE_APC_INFRA_AO", false},
	{-1, -1, 98, "DEVICE_APC_INFRA_PDN", false},

};

static struct mtk_device_info mt6880_devices_peri[] = {
	/* sys_idx, ctrl_idx, vio_idx, device, vio_irq */
	/* 0 */
	{0, 0, 0, "SMGII_S0_S", true},
	{0, 1, 1, "SMGII_S1_S", true},
	{0, 2, 2, "SPM_APB_S", true},
	{0, 3, 3, "SPM_APB_S-1", true},
	{0, 4, 4, "SPM_APB_S-2", true},
	{0, 5, 5, "SPM_APB_S-3", true},
	{0, 6, 6, "SPM_APB_S-4", true},
	{0, 7, 7, "APMIXEDSYS_APB_S", true},
	{0, 8, 8, "APMIXEDSYS_APB_S-1", true},
	{0, 9, 9, "TOPCKGEN_APB_S", true},

	/* 10 */
	{0, 10, 10, "INFRACFG_AO_APB_S", true},
	{0, 11, 11, "INFRACFG_AO_MEM_APB_S", true},
	{0, 12, 12, "PERICFG_AO_APB_S", true},
	{0, 13, 13, "GPIO_APB_S", true},
	{0, 14, 15, "TOPRGU_APB_S", true},
	{0, 15, 16, "RESERVED_APB_S", true},
	{0, 16, 17, "DEVICE_APC_INFRA_AO_APB_S", true},
	{0, 17, 18, "BCRM_INFRA_AO_APB_S", true},
	{0, 18, 19, "DEBUG_CTRL_INFRA_AO_APB_S", true},
	{0, 19, 20, "DEVICE_APC_PERI_AO_APB_S", true},

	/* 20 */
	{0, 20, 21, "BCRM_PERI_AO_APB_S", true},
	{0, 21, 22, "DEBUG_CTRL_PERI_AO_APB_S", true},
	{0, 22, 23, "AP_CIRQ_EINT_APB_S", true},
	{0, 23, 25, "PMIC_WRAP_APB_S", true},
	{0, 24, 26, "KP_APB_S", true},
	{0, 25, 27, "TOP_MISC_APB_S", true},
	{0, 26, 28, "DVFSRC_APB_S", true},
	{0, 27, 29, "MBIST_AO_APB_S", true},
	{0, 28, 30, "DPMAIF_AO_APB_S", true},
	{0, 29, 31, "SYS_TIMER_APB_S", true},

	/* 30 */
	{0, 30, 32, "MODEM_TEMP_SHARE_APB_S", true},
	{0, 31, 33, "PMIF1_APB_S", true},
	{0, 32, 34, "TIA_APB_S", true},
	{0, 33, 35, "MHCCIF_APB0_S", true},
	{0, 34, 36, "MHCCIF_APB1_S", true},
	{0, 35, 37, "TOPCKGEN_INFRA_CFG_APB_S", true},
	{0, 36, 38, "DRM_DEBUG_TOP_APB_S", true},
	{0, 37, 39, "PWR_MD32_S", true},
	{0, 38, 40, "PWR_MD32_S-1", true},
	{0, 39, 41, "PWR_MD32_S-2", true},

	/* 40 */
	{0, 40, 42, "PWR_MD32_S-3", true},
	{0, 41, 43, "PWR_MD32_S-4", true},
	{0, 42, 44, "PWR_MD32_S-5", true},
	{0, 43, 45, "PWR_MD32_S-6", true},
	{0, 44, 46, "PWR_MD32_S-7", true},
	{0, 45, 47, "PWR_MD32_S-8", true},
	{0, 46, 48, "PWR_MD32_S-9", true},
	{0, 47, 49, "PWR_MD32_S-10", true},
	{0, 48, 50, "AUDIO_S", true},
	{0, 49, 51, "AUDIO_S-1", true},

	/* 50 */
	{0, 50, 96, "SSUSB_S", true},
	{0, 51, 97, "SSUSB_S-1", true},
	{0, 52, 98, "SSUSB_S-2", true},
	{0, 53, 99, "DEBUGSYS_APB_S", true},
	{0, 54, 100, "DRAMC_MD32_S0_APB_S", true},
	{0, 55, 101, "DRAMC_MD32_S0_APB_S-1", true},
	{0, 56, 102, "DRAMC_MD32_S1_APB_S", true},
	{0, 57, 103, "DRAMC_MD32_S1_APB_S-1", true},
	{0, 58, 104, "DRAMC_CH0_TOP0_APB_S", true},
	{0, 59, 105, "DRAMC_CH0_TOP1_APB_S", true},

	/* 60 */
	{0, 60, 106, "DRAMC_CH0_TOP2_APB_S", true},
	{0, 61, 107, "DRAMC_CH0_TOP3_APB_S", true},
	{0, 62, 108, "DRAMC_CH0_TOP4_APB_S", true},
	{0, 63, 109, "DRAMC_CH0_TOP5_APB_S", true},
	{0, 64, 110, "DRAMC_CH0_TOP6_APB_S", true},
	{0, 65, 111, "DRAMC_CH1_TOP0_APB_S", true},
	{0, 66, 112, "DRAMC_CH1_TOP1_APB_S", true},
	{0, 67, 113, "DRAMC_CH1_TOP2_APB_S", true},
	{0, 68, 114, "DRAMC_CH1_TOP3_APB_S", true},
	{0, 69, 115, "DRAMC_CH1_TOP4_APB_S", true},

	/* 70 */
	{0, 70, 116, "DRAMC_CH1_TOP5_APB_S", true},
	{0, 71, 117, "DRAMC_CH1_TOP6_APB_S", true},
	{0, 72, 118, "DRAMC_CH2_TOP0_APB_S", true},
	{0, 73, 119, "DRAMC_CH2_TOP1_APB_S", true},
	{0, 74, 120, "DRAMC_CH2_TOP2_APB_S", true},
	{0, 75, 121, "DRAMC_CH2_TOP3_APB_S", true},
	{0, 76, 122, "DRAMC_CH2_TOP4_APB_S", true},
	{0, 77, 123, "DRAMC_CH2_TOP5_APB_S", true},
	{0, 78, 124, "DRAMC_CH2_TOP6_APB_S", true},
	{0, 79, 125, "DRAMC_CH3_TOP0_APB_S", true},

	/* 80 */
	{0, 80, 126, "DRAMC_CH3_TOP1_APB_S", true},
	{0, 81, 127, "DRAMC_CH3_TOP2_APB_S", true},
	{0, 82, 128, "DRAMC_CH3_TOP3_APB_S", true},
	{0, 83, 129, "DRAMC_CH3_TOP4_APB_S", true},
	{0, 84, 130, "DRAMC_CH3_TOP5_APB_S", true},
	{0, 85, 131, "DRAMC_CH3_TOP6_APB_S", true},
	{0, 86, 134, "CCIF3_AP_APB_S", true},
	{0, 87, 135, "CCIF3_MD_APB_S", true},
	{0, 88, 136, "INFRA_BUS_TRACE_APB_S", true},
	{0, 89, 137, "SSC_INFRA_APB0_S", true},

	/* 90 */
	{0, 90, 138, "SSC_INFRA_APB1_S", true},
	{0, 91, 139, "SSC_INFRA_APB2_S", true},
	{0, 92, 140, "SSC_INFRA_APB3_S", true},
	{1, 0, 52, "CONNSYS", true},
	{1, 1, 53, "MDSYS", true},
	{1, 2, 54, "MDSYS", true},
	{1, 3, 55, "MDSYS", true},
	{1, 4, 56, "MDSYS", true},
	{1, 5, 57, "MDSYS", true},
	{1, 6, 58, "MDSYS", true},

	/* 100 */
	{1, 7, 59, "MDSYS", true},
	{1, 8, 60, "MDSYS", true},
	{1, 9, 61, "MDSYS", true},
	{1, 10, 62, "MDSYS", true},
	{1, 11, 63, "MDSYS", true},
	{1, 12, 64, "MDSYS", true},
	{1, 13, 65, "MDSYS", true},
	{1, 14, 66, "MDSYS", true},
	{1, 15, 67, "MDSYS", true},
	{1, 16, 68, "MDSYS", true},

	/* 110 */
	{1, 17, 69, "MDSYS", true},
	{1, 18, 70, "MDSYS", true},
	{1, 19, 71, "MDSYS", true},
	{1, 20, 72, "MDSYS", true},
	{1, 21, 73, "MDSYS", true},
	{1, 22, 74, "MDSYS", true},
	{1, 23, 75, "MDSYS", true},
	{1, 24, 76, "MDSYS", true},
	{1, 25, 77, "MDSYS", true},
	{1, 26, 78, "MDSYS", true},

	/* 120 */
	{1, 27, 79, "MDSYS", true},
	{1, 28, 80, "MDSYS", true},
	{1, 29, 81, "MDSYS", true},
	{1, 30, 82, "MDSYS", true},
	{1, 31, 83, "MDSYS", true},
	{1, 32, 84, "MDSYS", true},
	{1, 33, 85, "MDSYS", true},
	{1, 34, 86, "MDSYS", true},
	{1, 35, 87, "MDSYS", true},
	{1, 36, 88, "MDSYS", true},

	/* 130 */
	{1, 37, 89, "MDSYS", true},
	{1, 38, 90, "MDSYS", true},
	{1, 39, 91, "MDSYS", true},
	{1, 40, 92, "MDSYS", true},
	{1, 41, 93, "MDSYS", true},
	{1, 42, 94, "MDSYS", true},
	{1, 43, 95, "MDSYS", true},
	{-1, -1, 141, "OOB_way_en", true},
	{-1, -1, 142, "OOB_way_en", true},
	{-1, -1, 143, "OOB_way_en", true},

	/* 140 */
	{-1, -1, 144, "OOB_way_en", true},
	{-1, -1, 145, "OOB_way_en", true},
	{-1, -1, 146, "OOB_way_en", true},
	{-1, -1, 147, "OOB_way_en", true},
	{-1, -1, 148, "OOB_way_en", true},
	{-1, -1, 149, "OOB_way_en", true},
	{-1, -1, 150, "OOB_way_en", true},
	{-1, -1, 151, "OOB_way_en", true},
	{-1, -1, 152, "OOB_way_en", true},
	{-1, -1, 153, "OOB_way_en", true},

	/* 150 */
	{-1, -1, 154, "OOB_way_en", true},
	{-1, -1, 155, "OOB_way_en", true},
	{-1, -1, 156, "OOB_way_en", true},
	{-1, -1, 157, "OOB_way_en", true},
	{-1, -1, 158, "OOB_way_en", true},
	{-1, -1, 159, "OOB_way_en", true},
	{-1, -1, 160, "OOB_way_en", true},
	{-1, -1, 161, "OOB_way_en", true},
	{-1, -1, 162, "OOB_way_en", true},
	{-1, -1, 163, "OOB_way_en", true},

	/* 160 */
	{-1, -1, 164, "OOB_way_en", true},
	{-1, -1, 165, "OOB_way_en", true},
	{-1, -1, 166, "OOB_way_en", true},
	{-1, -1, 167, "OOB_way_en", true},
	{-1, -1, 168, "OOB_way_en", true},
	{-1, -1, 169, "OOB_way_en", true},
	{-1, -1, 170, "OOB_way_en", true},
	{-1, -1, 171, "OOB_way_en", true},
	{-1, -1, 172, "OOB_way_en", true},
	{-1, -1, 173, "OOB_way_en", true},

	/* 170 */
	{-1, -1, 174, "OOB_way_en", true},
	{-1, -1, 175, "OOB_way_en", true},
	{-1, -1, 176, "OOB_way_en", true},
	{-1, -1, 177, "OOB_way_en", true},
	{-1, -1, 178, "OOB_way_en", true},
	{-1, -1, 179, "OOB_way_en", true},
	{-1, -1, 180, "OOB_way_en", true},
	{-1, -1, 181, "OOB_way_en", true},
	{-1, -1, 182, "OOB_way_en", true},
	{-1, -1, 183, "OOB_way_en", true},

	/* 180 */
	{-1, -1, 184, "OOB_way_en", true},
	{-1, -1, 185, "OOB_way_en", true},
	{-1, -1, 186, "OOB_way_en", true},
	{-1, -1, 187, "OOB_way_en", true},
	{-1, -1, 188, "OOB_way_en", true},
	{-1, -1, 189, "OOB_way_en", true},
	{-1, -1, 190, "OOB_way_en", true},
	{-1, -1, 191, "OOB_way_en", true},
	{-1, -1, 192, "OOB_way_en", true},
	{-1, -1, 193, "OOB_way_en", true},

	/* 190 */
	{-1, -1, 194, "OOB_way_en", true},
	{-1, -1, 195, "OOB_way_en", true},
	{-1, -1, 196, "OOB_way_en", true},
	{-1, -1, 197, "OOB_way_en", true},
	{-1, -1, 198, "OOB_way_en", true},
	{-1, -1, 199, "OOB_way_en", true},
	{-1, -1, 200, "OOB_way_en", true},
	{-1, -1, 201, "OOB_way_en", true},
	{-1, -1, 202, "OOB_way_en", true},
	{-1, -1, 203, "OOB_way_en", true},

	/* 200 */
	{-1, -1, 204, "OOB_way_en", true},
	{-1, -1, 205, "OOB_way_en", true},
	{-1, -1, 206, "OOB_way_en", true},
	{-1, -1, 207, "OOB_way_en", true},
	{-1, -1, 208, "OOB_way_en", true},
	{-1, -1, 209, "OOB_way_en", true},
	{-1, -1, 210, "OOB_way_en", true},
	{-1, -1, 211, "OOB_way_en", true},
	{-1, -1, 212, "OOB_way_en", true},
	{-1, -1, 213, "OOB_way_en", true},

	/* 210 */
	{-1, -1, 214, "OOB_way_en", true},
	{-1, -1, 215, "OOB_way_en", true},
	{-1, -1, 216, "OOB_way_en", true},
	{-1, -1, 217, "OOB_way_en", true},
	{-1, -1, 218, "DECERR", true},
	{-1, -1, 219, "DECERR", true},
	{-1, -1, 220, "DECERR", true},
	{-1, -1, 221, "DECERR", true},
	{-1, -1, 222, "MDP_MALI", true},
	{-1, -1, 223, "MMSYS2_MALI", true},

	/* 220 */
	{-1, -1, 224, "MMSYS_MALI", true},
	{-1, -1, 225, "PMIC_WRAP", false},
	{-1, -1, 226, "PMIF1", false},
	{-1, -1, 227, "DEVICE_APC_PERI_AO", false},
	{-1, -1, 228, "DEVICE_APC_PERI_PDN", false},

};

static struct mtk_device_info mt6880_devices_peri2[] = {
	/* sys_idx, ctrl_idx, vio_idx, device, vio_irq */
	/* 0 */
	{0, 0, 0, "EFUSE_DEBUG_AO_APB_S", true},
	{0, 1, 1, "APXGPT_APB_S", true},
	{0, 2, 2, "SEJ_APB_S", true},
	{0, 3, 3, "AES_TOP0_APB_S", true},
	{0, 4, 4, "SECURITY_AO_APB_S", true},
	{0, 5, 5, "DEVICE_APC_PERI_AO2_APB_S", true},
	{0, 6, 6, "BCRM_PERI_AO2_APB_S", true},
	{0, 7, 7, "DEBUG_CTRL_PERI_AO2_APB_S", true},
	{0, 8, 8, "SPMI_MST_APB_S", true},
	{0, 9, 9, "SPMIP_MST_APB_S", true},

	/* 10 */
	{0, 10, 10, "CLDMA0_AO_AP_APB_S", true},
	{0, 11, 11, "CLDMA0_AO_MD_APB_S", true},
	{0, 12, 12, "CLDMA1_AO_AP_APB_S", true},
	{0, 13, 13, "CLDMA1_AO_MD_APB_S", true},
	{0, 14, 14, "CLDMA2_AO_AP_APB_S", true},
	{0, 15, 15, "CLDMA2_AO_MD_APB_S", true},
	{0, 16, 16, "CLDMA3_AO_AP_APB_S", true},
	{0, 17, 17, "CLDMA3_AO_MD_APB_S", true},
	{0, 18, 18, "DEBUG_CTRL_FMEM_AO_APB_S", true},
	{0, 19, 19, "BCRM_FMEM_AO_APB_S", true},

	/* 20 */
	{0, 20, 20, "DEVICE_APC_FMEM_AO_APB_S", true},
	{0, 21, 21, "PWM_APB_S", true},
	{0, 22, 22, "GCE_APB_S", true},
	{0, 23, 23, "GCE_APB_S-1", true},
	{0, 24, 24, "GCE_APB_S-2", true},
	{0, 25, 25, "GCE_APB_S-3", true},
	{0, 26, 26, "DPMAIF_PDN_APB_S", true},
	{0, 27, 27, "DPMAIF_PDN_APB_S-1", true},
	{0, 28, 28, "DPMAIF_PDN_APB_S-2", true},
	{0, 29, 29, "DPMAIF_PDN_APB_S-3", true},

	/* 30 */
	{0, 30, 30, "BND_EAST_APB0_S", true},
	{0, 31, 31, "BND_EAST_APB1_S", true},
	{0, 32, 32, "BND_EAST_APB2_S", true},
	{0, 33, 33, "BND_EAST_APB3_S", true},
	{0, 34, 34, "BND_EAST_APB4_S", true},
	{0, 35, 35, "BND_EAST_APB5_S", true},
	{0, 36, 36, "BND_EAST_APB6_S", true},
	{0, 37, 37, "BND_EAST_APB7_S", true},
	{0, 38, 38, "BND_EAST_APB8_S", true},
	{0, 39, 39, "BND_EAST_APB9_S", true},

	/* 40 */
	{0, 40, 40, "BND_EAST_APB10_S", true},
	{0, 41, 41, "BND_EAST_APB11_S", true},
	{0, 42, 42, "BND_EAST_APB12_S", true},
	{0, 43, 43, "BND_EAST_APB13_S", true},
	{0, 44, 44, "BND_EAST_APB14_S", true},
	{0, 45, 45, "BND_EAST_APB15_S", true},
	{0, 46, 46, "BND_WEST_APB0_S", true},
	{0, 47, 47, "BND_WEST_APB1_S", true},
	{0, 48, 48, "BND_WEST_APB2_S", true},
	{0, 49, 49, "BND_WEST_APB3_S", true},

	/* 50 */
	{0, 50, 50, "BND_WEST_APB4_S", true},
	{0, 51, 51, "BND_WEST_APB5_S", true},
	{0, 52, 52, "BND_WEST_APB6_S", true},
	{0, 53, 53, "BND_WEST_APB7_S", true},
	{0, 54, 54, "BND_NORTH_APB0_S", true},
	{0, 55, 55, "BND_NORTH_APB1_S", true},
	{0, 56, 56, "BND_NORTH_APB2_S", true},
	{0, 57, 57, "BND_NORTH_APB3_S", true},
	{0, 58, 58, "BND_NORTH_APB4_S", true},
	{0, 59, 59, "BND_NORTH_APB5_S", true},

	/* 60 */
	{0, 60, 60, "BND_NORTH_APB6_S", true},
	{0, 61, 61, "BND_NORTH_APB7_S", true},
	{0, 62, 62, "BND_NORTH_APB8_S", true},
	{0, 63, 63, "BND_NORTH_APB9_S", true},
	{0, 64, 64, "BND_NORTH_APB10_S", true},
	{0, 65, 65, "BND_NORTH_APB11_S", true},
	{0, 66, 66, "BND_NORTH_APB12_S", true},
	{0, 67, 67, "BND_NORTH_APB13_S", true},
	{0, 68, 68, "BND_NORTH_APB14_S", true},
	{0, 69, 69, "BND_NORTH_APB15_S", true},

	/* 70 */
	{0, 70, 70, "BND_SOUTH_APB0_S", true},
	{0, 71, 71, "BND_SOUTH_APB1_S", true},
	{0, 72, 72, "BND_SOUTH_APB2_S", true},
	{0, 73, 73, "BND_SOUTH_APB3_S", true},
	{0, 74, 74, "BND_SOUTH_APB4_S", true},
	{0, 75, 75, "BND_SOUTH_APB5_S", true},
	{0, 76, 76, "BND_SOUTH_APB6_S", true},
	{0, 77, 77, "BND_SOUTH_APB7_S", true},
	{0, 78, 78, "BND_SOUTH_APB8_S", true},
	{0, 79, 79, "BND_SOUTH_APB9_S", true},

	/* 80 */
	{0, 80, 80, "BND_SOUTH_APB10_S", true},
	{0, 81, 81, "BND_SOUTH_APB11_S", true},
	{0, 82, 82, "BND_SOUTH_APB12_S", true},
	{0, 83, 83, "BND_SOUTH_APB13_S", true},
	{0, 84, 84, "BND_SOUTH_APB14_S", true},
	{0, 85, 85, "BND_SOUTH_APB15_S", true},
	{0, 86, 86, "BND_EAST_NORTH_APB0_S", true},
	{0, 87, 87, "BND_EAST_NORTH_APB1_S", true},
	{0, 88, 88, "BND_EAST_NORTH_APB2_S", true},
	{0, 89, 89, "BND_EAST_NORTH_APB3_S", true},

	/* 90 */
	{0, 90, 90, "BND_EAST_NORTH_APB4_S", true},
	{0, 91, 91, "BND_EAST_NORTH_APB5_S", true},
	{0, 92, 92, "BND_EAST_NORTH_APB6_S", true},
	{0, 93, 93, "BND_EAST_NORTH_APB7_S", true},
	{0, 94, 94, "SYS_CIRQ_APB_S", true},
	{0, 95, 95, "DEVICE_APC_INFRA_PDN_APB_S", true},
	{0, 96, 96, "DEBUG_TRACKER_APB_S", true},
	{0, 97, 97, "CCIF0_AP_APB_S", true},
	{0, 98, 98, "CCIF0_MD_APB_S", true},
	{0, 99, 99, "CCIF1_AP_APB_S", true},

	/* 100 */
	{0, 100, 100, "CCIF1_MD_APB_S", true},
	{0, 101, 101, "MBIST_PDN_APB_S", true},
	{0, 102, 102, "INFRACFG_PDN_APB_S", true},
	{0, 103, 103, "CLDMA0_PDN_AP_APB_S", true},
	{0, 104, 104, "CLDMA0_PDN_MD_APB_S", true},
	{0, 105, 105, "CLDMA1_PDN_AP_APB_S", true},
	{0, 106, 106, "CLDMA1_PDN_MD_APB_S", true},
	{0, 107, 107, "CLDMA2_PDN_AP_APB_S", true},
	{0, 108, 108, "CLDMA2_PDN_MD_APB_S", true},
	{0, 109, 109, "CLDMA3_PDN_AP_APB_S", true},

	/* 110 */
	{0, 110, 110, "CLDMA3_PDN_MD_APB_S", true},
	{0, 111, 111, "TRNG_APB_S", true},
	{0, 112, 112, "EIP_APB_S", true},
	{0, 113, 113, "ECC_TOP_APB_S", true},
	{0, 114, 114, "M2C_APB_S", true},
	{0, 115, 115, "GCPU_APB_S", true},
	{0, 116, 116, "GCPU_NS_APB_S", true},
	{0, 117, 117, "GCPU_MMU_APB_S", true},
	{0, 118, 118, "CQ_DMA_APB_S", true},
	{0, 119, 119, "SRAMROM_APB_S", true},

	/* 120 */
	{0, 120, 120, "INFRACFG_MEM_APB_S", true},
	{0, 121, 123, "SYS_CIRQ2_APB_S", true},
	{0, 122, 124, "DEBUG_TRACKER_APB1_S", true},
	{0, 123, 125, "FAKE_ENGINE_0_APB_S", true},
	{0, 124, 126, "FAKE_ENGINE_1_APB_S", true},
	{0, 125, 127, "EMI_APB_S", true},
	{0, 126, 128, "EMI_MPU_APB_S", true},
	{0, 127, 129, "APDMA_APB_S", true},
	{0, 128, 130, "DEBUG_TRACKER_APB2_S", true},
	{0, 129, 131, "BCRM_INFRA_PDN_APB_S", true},

	/* 130 */
	{0, 130, 132, "BCRM_PERI_PDN_APB_S", true},
	{0, 131, 133, "BCRM_PERI_PDN2_APB_S", true},
	{0, 132, 134, "DEVICE_APC_PERI_PDN_APB_S", true},
	{0, 133, 135, "DEVICE_APC_PERI_PDN2_APB_S", true},
	{0, 134, 136, "BCRM_FMEM_PDN_APB_S", true},
	{-1, -1, 137, "OOB_way_en", true},
	{-1, -1, 138, "OOB_way_en", true},
	{-1, -1, 139, "OOB_way_en", true},
	{-1, -1, 140, "OOB_way_en", true},
	{-1, -1, 141, "OOB_way_en", true},

	/* 140 */
	{-1, -1, 142, "OOB_way_en", true},
	{-1, -1, 143, "OOB_way_en", true},
	{-1, -1, 144, "OOB_way_en", true},
	{-1, -1, 145, "OOB_way_en", true},
	{-1, -1, 146, "OOB_way_en", true},
	{-1, -1, 147, "OOB_way_en", true},
	{-1, -1, 148, "OOB_way_en", true},
	{-1, -1, 149, "OOB_way_en", true},
	{-1, -1, 150, "OOB_way_en", true},
	{-1, -1, 151, "OOB_way_en", true},

	/* 150 */
	{-1, -1, 152, "OOB_way_en", true},
	{-1, -1, 153, "OOB_way_en", true},
	{-1, -1, 154, "OOB_way_en", true},
	{-1, -1, 155, "OOB_way_en", true},
	{-1, -1, 156, "OOB_way_en", true},
	{-1, -1, 157, "OOB_way_en", true},
	{-1, -1, 158, "OOB_way_en", true},
	{-1, -1, 159, "OOB_way_en", true},
	{-1, -1, 160, "OOB_way_en", true},
	{-1, -1, 161, "OOB_way_en", true},

	/* 160 */
	{-1, -1, 162, "OOB_way_en", true},
	{-1, -1, 163, "OOB_way_en", true},
	{-1, -1, 164, "OOB_way_en", true},
	{-1, -1, 165, "OOB_way_en", true},
	{-1, -1, 166, "OOB_way_en", true},
	{-1, -1, 167, "OOB_way_en", true},
	{-1, -1, 168, "OOB_way_en", true},
	{-1, -1, 169, "OOB_way_en", true},
	{-1, -1, 170, "OOB_way_en", true},
	{-1, -1, 171, "OOB_way_en", true},

	/* 170 */
	{-1, -1, 172, "OOB_way_en", true},
	{-1, -1, 173, "OOB_way_en", true},
	{-1, -1, 174, "OOB_way_en", true},
	{-1, -1, 175, "OOB_way_en", true},
	{-1, -1, 176, "OOB_way_en", true},
	{-1, -1, 177, "OOB_way_en", true},
	{-1, -1, 178, "OOB_way_en", true},
	{-1, -1, 179, "OOB_way_en", true},
	{-1, -1, 180, "OOB_way_en", true},
	{-1, -1, 181, "OOB_way_en", true},

	/* 180 */
	{-1, -1, 182, "OOB_way_en", true},
	{-1, -1, 183, "OOB_way_en", true},
	{-1, -1, 184, "OOB_way_en", true},
	{-1, -1, 185, "OOB_way_en", true},
	{-1, -1, 186, "OOB_way_en", true},
	{-1, -1, 187, "OOB_way_en", true},
	{-1, -1, 188, "OOB_way_en", true},
	{-1, -1, 189, "OOB_way_en", true},
	{-1, -1, 190, "OOB_way_en", true},
	{-1, -1, 191, "OOB_way_en", true},

	/* 190 */
	{-1, -1, 192, "OOB_way_en", true},
	{-1, -1, 193, "OOB_way_en", true},
	{-1, -1, 194, "OOB_way_en", true},
	{-1, -1, 195, "OOB_way_en", true},
	{-1, -1, 196, "OOB_way_en", true},
	{-1, -1, 197, "OOB_way_en", true},
	{-1, -1, 198, "OOB_way_en", true},
	{-1, -1, 199, "OOB_way_en", true},
	{-1, -1, 200, "OOB_way_en", true},
	{-1, -1, 201, "OOB_way_en", true},

	/* 200 */
	{-1, -1, 202, "OOB_way_en", true},
	{-1, -1, 203, "OOB_way_en", true},
	{-1, -1, 204, "OOB_way_en", true},
	{-1, -1, 205, "OOB_way_en", true},
	{-1, -1, 206, "OOB_way_en", true},
	{-1, -1, 207, "OOB_way_en", true},
	{-1, -1, 208, "OOB_way_en", true},
	{-1, -1, 209, "OOB_way_en", true},
	{-1, -1, 210, "OOB_way_en", true},
	{-1, -1, 211, "OOB_way_en", true},

	/* 210 */
	{-1, -1, 212, "OOB_way_en", true},
	{-1, -1, 213, "OOB_way_en", true},
	{-1, -1, 214, "OOB_way_en", true},
	{-1, -1, 215, "OOB_way_en", true},
	{-1, -1, 216, "OOB_way_en", true},
	{-1, -1, 217, "OOB_way_en", true},
	{-1, -1, 218, "OOB_way_en", true},
	{-1, -1, 219, "OOB_way_en", true},
	{-1, -1, 220, "OOB_way_en", true},
	{-1, -1, 221, "OOB_way_en", true},

	/* 220 */
	{-1, -1, 222, "OOB_way_en", true},
	{-1, -1, 223, "OOB_way_en", true},
	{-1, -1, 224, "OOB_way_en", true},
	{-1, -1, 225, "OOB_way_en", true},
	{-1, -1, 226, "OOB_way_en", true},
	{-1, -1, 227, "OOB_way_en", true},
	{-1, -1, 228, "OOB_way_en", true},
	{-1, -1, 229, "OOB_way_en", true},
	{-1, -1, 230, "OOB_way_en", true},
	{-1, -1, 231, "OOB_way_en", true},

	/* 230 */
	{-1, -1, 232, "OOB_way_en", true},
	{-1, -1, 233, "OOB_way_en", true},
	{-1, -1, 234, "OOB_way_en", true},
	{-1, -1, 235, "OOB_way_en", true},
	{-1, -1, 236, "OOB_way_en", true},
	{-1, -1, 237, "OOB_way_en", true},
	{-1, -1, 238, "OOB_way_en", true},
	{-1, -1, 239, "OOB_way_en", true},
	{-1, -1, 240, "OOB_way_en", true},
	{-1, -1, 241, "OOB_way_en", true},

	/* 240 */
	{-1, -1, 242, "OOB_way_en", true},
	{-1, -1, 243, "OOB_way_en", true},
	{-1, -1, 244, "OOB_way_en", true},
	{-1, -1, 245, "OOB_way_en", true},
	{-1, -1, 246, "OOB_way_en", true},
	{-1, -1, 247, "OOB_way_en", true},
	{-1, -1, 248, "OOB_way_en", true},
	{-1, -1, 249, "OOB_way_en", true},
	{-1, -1, 250, "OOB_way_en", true},
	{-1, -1, 251, "OOB_way_en", true},

	/* 250 */
	{-1, -1, 252, "OOB_way_en", true},
	{-1, -1, 253, "OOB_way_en", true},
	{-1, -1, 254, "OOB_way_en", true},
	{-1, -1, 255, "OOB_way_en", true},
	{-1, -1, 256, "OOB_way_en", true},
	{-1, -1, 257, "OOB_way_en", true},
	{-1, -1, 258, "OOB_way_en", true},
	{-1, -1, 259, "OOB_way_en", true},
	{-1, -1, 260, "OOB_way_en", true},
	{-1, -1, 261, "OOB_way_en", true},

	/* 260 */
	{-1, -1, 262, "OOB_way_en", true},
	{-1, -1, 263, "OOB_way_en", true},
	{-1, -1, 264, "OOB_way_en", true},
	{-1, -1, 265, "OOB_way_en", true},
	{-1, -1, 266, "OOB_way_en", true},
	{-1, -1, 267, "OOB_way_en", true},
	{-1, -1, 268, "OOB_way_en", true},
	{-1, -1, 269, "OOB_way_en", true},
	{-1, -1, 270, "OOB_way_en", true},
	{-1, -1, 271, "OOB_way_en", true},

	/* 270 */
	{-1, -1, 272, "OOB_way_en", true},
	{-1, -1, 273, "OOB_way_en", true},
	{-1, -1, 274, "DECERR", true},
	{-1, -1, 275, "DECERR", true},
	{-1, -1, 276, "DECERR", true},
	{-1, -1, 277, "DECERR", true},
	{-1, -1, 278, "DECERR", true},
	{-1, -1, 279, "DECERR", true},
	{-1, -1, 280, "DECERR", true},
	{-1, -1, 281, "DECERR", true},

	/* 280 */
	{-1, -1, 282, "CQ_DMA", false},
	{-1, -1, 283, "EMI", false},
	{-1, -1, 284, "EMI_MPU", false},
	{-1, -1, 285, "GCE", false},
	{-1, -1, 286, "AP_DMA", false},
	{-1, -1, 287, "DEVICE_APC_PERI_AO2", false},
	{-1, -1, 288, "DEVICE_APC_PERI_PDN2", false},

};

static struct mtk_device_info mt6880_devices_peri_par[] = {
	/* sys_idx, ctrl_idx, vio_idx, device, vio_irq */
	/* 0 */
	{0, 0, 0, "NOR_AXI_S", true},
	{1, 0, 1, "PCIE0_AHB_S", true},
	{1, 1, 2, "PCIE1_AHB_S", true},
	{1, 2, 3, "PCIE2_AHB_S", true},
	{1, 3, 4, "PCIE3_AHB_S", true},
	{1, 4, 5, "MSDC0_S", true},
	{1, 5, 6, "MSDC1_S", true},
	{1, 6, 8, "AUXADC_APB_S", true},
	{1, 7, 9, "UART0_APB_S", true},
	{1, 8, 10, "UART1_APB_S", true},

	/* 10 */
	{1, 9, 11, "UART2_APB_S", true},
	{1, 10, 12, "UART3_APB_S", true},
	{1, 11, 13, "IIC_P2P_REMAP_APB4_S", true},
	{1, 12, 14, "SPI0_APB_S", true},
	{1, 13, 15, "PTP_THERM_CTRL_APB_S", true},
	{1, 14, 16, "IIC_SLAVE_APB_S", true},
	{1, 15, 17, "DISP_PWM_APB_S", true},
	{1, 16, 18, "SMPU_APB_S", true},
	{1, 17, 19, "SNPS_MAC_APB_S", true},
	{1, 18, 20, "SPI1_APB_S", true},

	/* 20 */
	{1, 19, 21, "SPI2_APB_S", true},
	{1, 20, 22, "SPI3_APB_S", true},
	{1, 21, 23, "SPIS_APB_S", true},
	{1, 22, 24, "IIC_P2P_REMAP_APB0_S", true},
	{1, 23, 25, "IIC_P2P_REMAP_APB1_S", true},
	{1, 24, 26, "IIC_P2P_REMAP_APB2_S", true},
	{1, 25, 27, "IIC_P2P_REMAP_APB3_S", true},
	{1, 26, 28, "BCRM_PERI_PAR_PDN_APB_S", true},
	{1, 27, 29, "DEVICE_APC_PERI_PAR_PDN_APB_S", true},
	{1, 28, 30, "PTP_THERM_CTRL2_APB_S", true},

	/* 30 */
	{1, 29, 31, "NOR_APB_S", true},
	{1, 30, 32, "NFI_APB_S", true},
	{1, 31, 33, "NFIECC_APB_S", true},
	{1, 32, 34, "PCIE0_AXI0_S", true},
	{1, 33, 35, "PCIE0_AXI1_S", true},
	{1, 34, 36, "PCIE0_AXI2_S", true},
	{1, 35, 38, "PCIE1_AXI1_S", true},
	{1, 36, 39, "PCIE2_AXI1_S", true},
	{1, 37, 40, "PCIE3_AXI1_S", true},
	{1, 38, 42, "PCIE1_AXI2_S", true},

	/* 40 */
	{1, 39, 43, "DEVICE_APC_PERI_PAR_AO_APB_S", true},
	{1, 40, 44, "DEBUG_CTRL_PERI_PAR_AO_APB_S", true},
	{1, 41, 45, "BCRM_PERI_PAR_AO_APB_S", true},
	{-1, -1, 46, "OOB_way_en", true},
	{-1, -1, 47, "OOB_way_en", true},
	{-1, -1, 48, "OOB_way_en", true},
	{-1, -1, 49, "OOB_way_en", true},
	{-1, -1, 50, "OOB_way_en", true},
	{-1, -1, 51, "OOB_way_en", true},
	{-1, -1, 52, "OOB_way_en", true},

	/* 50 */
	{-1, -1, 53, "OOB_way_en", true},
	{-1, -1, 54, "OOB_way_en", true},
	{-1, -1, 55, "OOB_way_en", true},
	{-1, -1, 56, "OOB_way_en", true},
	{-1, -1, 57, "OOB_way_en", true},
	{-1, -1, 58, "OOB_way_en", true},
	{-1, -1, 59, "OOB_way_en", true},
	{-1, -1, 60, "OOB_way_en", true},
	{-1, -1, 61, "OOB_way_en", true},
	{-1, -1, 62, "OOB_way_en", true},

	/* 60 */
	{-1, -1, 63, "OOB_way_en", true},
	{-1, -1, 64, "OOB_way_en", true},
	{-1, -1, 65, "OOB_way_en", true},
	{-1, -1, 66, "OOB_way_en", true},
	{-1, -1, 67, "OOB_way_en", true},
	{-1, -1, 68, "OOB_way_en", true},
	{-1, -1, 69, "OOB_way_en", true},
	{-1, -1, 70, "OOB_way_en", true},
	{-1, -1, 71, "OOB_way_en", true},
	{-1, -1, 72, "OOB_way_en", true},

	/* 70 */
	{-1, -1, 73, "OOB_way_en", true},
	{-1, -1, 74, "OOB_way_en", true},
	{-1, -1, 75, "OOB_way_en", true},
	{-1, -1, 76, "OOB_way_en", true},
	{-1, -1, 77, "OOB_way_en", true},
	{-1, -1, 78, "OOB_way_en", true},
	{-1, -1, 79, "OOB_way_en", true},
	{-1, -1, 80, "OOB_way_en", true},
	{-1, -1, 81, "OOB_way_en", true},
	{-1, -1, 82, "OOB_way_en", true},

	/* 80 */
	{-1, -1, 83, "OOB_way_en", true},
	{-1, -1, 84, "OOB_way_en", true},
	{-1, -1, 85, "OOB_way_en", true},
	{-1, -1, 86, "OOB_way_en", true},
	{-1, -1, 87, "OOB_way_en", true},
	{-1, -1, 88, "OOB_way_en", true},
	{-1, -1, 89, "OOB_way_en", true},
	{-1, -1, 90, "OOB_way_en", true},
	{-1, -1, 91, "OOB_way_en", true},
	{-1, -1, 92, "OOB_way_en", true},

	/* 90 */
	{-1, -1, 93, "DECERR", true},
	{-1, -1, 94, "DECERR", true},
	{-1, -1, 95, "DECERR", true},
	{-1, -1, 96, "DECERR", true},
	{-1, -1, 97, "DECERR", true},
	{-1, -1, 98, "reserved", false},
	{-1, -1, 99, "IMP_IIC_WRAP", false},
	{-1, -1, 100, "PCIE_M0_SMPU", false},
	{-1, -1, 101, "DEVICE_APC_PERI_PAR_AO", false},
	{-1, -1, 102, "DEVICE_APC_PERI_PAR_PDN", false},

};

static struct mtk_device_num mt6880_devices_num[] = {
	{SLAVE_TYPE_INFRA, VIO_SLAVE_NUM_INFRA},
	{SLAVE_TYPE_PERI, VIO_SLAVE_NUM_PERI},
	{SLAVE_TYPE_PERI2, VIO_SLAVE_NUM_PERI2},
	{SLAVE_TYPE_PERI_PAR, VIO_SLAVE_NUM_PERI_PAR},
};

static struct INFRAAXI_ID_INFO peri_mi_id_to_master[] = {
	{"APDMA_EXT",         0x0,    0x3},
	{"SPM",               0x6,    0x1f},
	{"PWM",               0xa,    0x1f},
	{"AUDIO",             0xe,    0x1f},
	{"SPM",               0x3,    0x1f},
};

static struct INFRAAXI_ID_INFO infra_mi_id_to_master[] = {
	{"GPSSYS",            0x0,    0x1007},
	{"CQDMA",             0x1,    0x1e3f},
	{"DEBUGTOP",          0x9,    0x1fbf},
	{"SSUSB",             0x11,   0x7ff},
	{"SSUSB2",            0x211,  0x7ff},
	{"NOR",               0x411,  0x7ff},
	{"MSDC1",             0xe11,  0x1fff},
	{"SPI0",              0x1e11, 0x1fff},
	{"SPI2",              0x51,   0x1fff},
	{"SPI3",              0x251,  0x1fff},
	{"SPI Slave",         0x451,  0x1fff},
	{"THERM",             0x91,   0x1fff},
	{"THERM 2",           0x291,  0x1fff},
	{"NFI",               0x491,  0x1fff},
	{"SPI1",              0x691,  0x1fff},
	{"MSDC0",             0xd1,   0x19ff},
	{"SNAP MAC",          0x111,  0x1ff},
	{"GCE",               0x19,   0x1f3f},
	{"CPUEB",             0x21,   0x103f},
	{"APDMA_EXT",         0x29,   0x1c7f},
	{"SPM",               0xe9,   0x1fff},
	{"PWM",               0x169,  0x1fff},
	{"AUDIO",             0x1e9,  0x1fff},
	{"DPMAIF",            0x2,    0x1f87},
	{"SSPM",              0x3,    0x1fe7},
	{"CPUEB",             0x4,    0x1e07},
	{"NETSYS_INFRA",      0x25,   0x107f},
	{"NETSYS_INFRA",      0x35,   0x107f},
	{"NETSYS_INFRA",      0x45,   0x107f},
	{"MEDSYS_INFRA",      0xd,    0x1c0f},
	{"EIP",               0x6,    0x1fff},
	{"EIP",               0x6,    0x1fdf},
	{"GCPU",              0xe,    0x1c1f},
	{"HSM_INFRA",         0x36,   0x1fff},
	{"HSM_INFRA",         0x56,   0x1fff},
	{"HSM_INFRA",         0xd6,   0x1fff},
	{"HSM_INFRA",         0x76,   0x1fff},
	{"CLDMA0",            0x1e,   0x1c7f},
	{"CLDMA1",            0x3e,   0x1c7f},
	{"CLDMA2",            0x5e,   0x1c7f},
	{"CLDMA3",            0x7e,   0x1c7f},
	{"PCIE0",             0x7,    0x1f},
	{"PCIE1",             0xf,    0x1f},
	{"PCIE2",             0x17,   0x1f},
	{"PCIE3",             0x1f,   0x1f},
};

static const char *infra_mi_trans(u32 bus_id)
{
	int master_count = ARRAY_SIZE(infra_mi_id_to_master);
	const char *master = "UNKNOWN_MASTER_FROM_INFRA";
	int i;

	if (bus_id & 0x1)
		return "APMCU";

	bus_id = bus_id >> 0x1;

	for (i = 0; i < master_count; i++) {
		if ((bus_id & infra_mi_id_to_master[i].mask) ==
				infra_mi_id_to_master[i].bus_id)
			master = infra_mi_id_to_master[i].master;
	}

	return master;
}

static const char *peri_mi_trans(u32 bus_id)
{
	int master_count = ARRAY_SIZE(peri_mi_id_to_master);
	const char *master = "UNKNOWN_MASTER_FROM_PERI";
	int i;

	if ((bus_id & 0x3) == 0x0)
		return infra_mi_trans(bus_id >> 2);
	else if ((bus_id & 0x3) == 0x1)
		return "MD";
	else if ((bus_id & 0x3) == 0x3)
		return "APDMA_INT";

	bus_id = bus_id >> 2;

	for (i = 0 ; i < master_count; i++) {
		if ((bus_id & peri_mi_id_to_master[i].mask) ==
				peri_mi_id_to_master[i].bus_id)
			master = peri_mi_id_to_master[i].master;
	}

	return master;
}

static const char *mt6880_bus_id_to_master(uint32_t bus_id, uint32_t vio_addr,
		int slave_type, int shift_sta_bit, int domain)
{
	const char *err_master = "UNKNOWN_MASTER";
	uint8_t h_1byte;

	pr_debug(PFX "[DEVAPC] %s:0x%x, %s:0x%x, %s:0x%x, %s:%d\n",
		"bus_id", bus_id, "vio_addr", vio_addr,
		"slave_type", slave_type,
		"shift_sta_bit", shift_sta_bit);

	if (bus_id == 0x0 && vio_addr == 0x0)
		return NULL;

	h_1byte = (vio_addr >> 24) & 0xFF;

	if (vio_addr >= MD_START_ADDR && vio_addr <= MD_END_ADDR) {
		pr_info(PFX "[DEVAPC] bus_id might be wrong\n");

		if (domain == 0x1)
			return "MD";
		else if (domain == 0x2)
			return "CONNSYS";
		else if (domain == 0x3)
			return "SSPM";
	}

	if (slave_type == SLAVE_TYPE_INFRA) {
		if (shift_sta_bit == 4) {
			if (!(bus_id & 0x1))
				return "EMI_L2C";

			return infra_mi_trans(bus_id >> 1);

		} else if (shift_sta_bit == 5) {
			if (bus_id & 0x1)
				return "GCE";

			return infra_mi_trans(bus_id >> 1);
		}

		return infra_mi_trans(bus_id);

	} else if (slave_type == SLAVE_TYPE_PERI) {
		if (shift_sta_bit == 5 || shift_sta_bit == 9) {
			if (!(bus_id & 0x1))
				return "MD";

			return peri_mi_trans(bus_id >> 1);
		}

		return peri_mi_trans(bus_id);

	} else if (slave_type == SLAVE_TYPE_PERI2) {
		return peri_mi_trans(bus_id);

	} else if (slave_type == SLAVE_TYPE_PERI_PAR) {
		if (shift_sta_bit == 0 || shift_sta_bit == 1 ||
		    shift_sta_bit == 2 || shift_sta_bit == 8) {
			return peri_mi_trans(bus_id);

		} else if (shift_sta_bit == 3) {
			return "DPMAIF";

		} else if (shift_sta_bit == 4) {
			if (bus_id & 0x1)
				return peri_mi_trans(bus_id >> 1);
			else if ((bus_id & 0x7) == 0)
				return "CLDMA0";
			else if ((bus_id & 0x7) == 2)
				return "CLDMA1";
			else if ((bus_id & 0x7) == 4)
				return "CLDMA3";

		} else if (shift_sta_bit == 5) {
			if (bus_id & 0x1)
				return "CLDMA2";
			else
				return "MD";

		} else if (shift_sta_bit == 6) {
			return infra_mi_trans(bus_id);

		} else if (shift_sta_bit == 7) {
			return "MD";
		}

		return err_master;

	}

	return err_master;
}

/* violation index corresponds to subsys */
const char *index_to_subsys(int slave_type, uint32_t vio_index,
		uint32_t vio_addr)
{
	int i;

	/* Filter by violation address */
	if ((vio_addr & 0xFF000000) == CONN_START_ADDR)
		return "CONNSYS";

	/* Filter by violation index */
	if (slave_type == SLAVE_TYPE_INFRA &&
			vio_index < VIO_SLAVE_NUM_INFRA) {
		for (i = 0; i < VIO_SLAVE_NUM_INFRA; i++) {
			if (vio_index == mt6880_devices_infra[i].vio_index)
				return mt6880_devices_infra[i].device;
		}

	} else if (slave_type == SLAVE_TYPE_PERI &&
			vio_index < VIO_SLAVE_NUM_PERI)
		for (i = 0; i < VIO_SLAVE_NUM_PERI; i++) {
			if (vio_index == mt6880_devices_peri[i].vio_index)
				return mt6880_devices_peri[i].device;
		}

	else if (slave_type == SLAVE_TYPE_PERI2 &&
			vio_index < VIO_SLAVE_NUM_PERI2)
		for (i = 0; i < VIO_SLAVE_NUM_PERI2; i++) {
			if (vio_index == mt6880_devices_peri2[i].vio_index)
				return mt6880_devices_peri2[i].device;
		}

	else if (slave_type == SLAVE_TYPE_PERI_PAR &&
			vio_index < VIO_SLAVE_NUM_PERI_PAR)
		for (i = 0; i < VIO_SLAVE_NUM_PERI_PAR; i++) {
			if (vio_index == mt6880_devices_peri_par[i].vio_index)
				return mt6880_devices_peri_par[i].device;
		}

	return "OUT_OF_BOUND";
}

static void mm2nd_vio_handler(void __iomem *infracfg,
			      struct mtk_devapc_vio_info *vio_info,
			      bool mdp_vio, bool disp2_vio, bool mmsys_vio)
{
	uint32_t vio_sta, vio_dbg, rw;
	uint32_t vio_sta_num;
	uint32_t vio0_offset;
	char mm_str[64] = {0};
	void __iomem *reg;
	int i;

	if (!infracfg) {
		pr_err(PFX "%s, param check failed, infracfg ptr is NULL\n",
				__func__);
		return;
	}

	if (mmsys_vio) {
		vio_sta_num = INFRACFG_MM_VIO_STA_NUM;
		vio0_offset = INFRACFG_MM_SEC_VIO0_OFFSET;

		strncpy(mm_str, "INFRACFG_MM_SEC_VIO", sizeof(mm_str));

	} else {
		pr_err(PFX "%s: param check failed, %s:%s, %s:%s, %s:%s\n",
				__func__,
				"mdp_vio", mdp_vio ? "true" : "false",
				"disp2_vio", disp2_vio ? "true" : "false",
				"mmsys_vio", mmsys_vio ? "true" : "false");
		return;
	}

	/* Get mm2nd violation status */
	for (i = 0; i < vio_sta_num; i++) {
		reg = infracfg + vio0_offset + i * 4;
		vio_sta = readl(reg);
		if (vio_sta)
			pr_info(PFX "MM 2nd violation: %s%d:0x%x\n",
					mm_str, i, vio_sta);
	}

	/* Get mm2nd violation address */
	reg = infracfg + vio0_offset + i * 4;
	vio_info->vio_addr = readl(reg);

	/* Get mm2nd violation information */
	reg = infracfg + vio0_offset + (i + 1) * 4;
	vio_dbg = readl(reg);

	vio_info->domain_id = (vio_dbg & INFRACFG_MM2ND_VIO_DOMAIN_MASK) >>
		INFRACFG_MM2ND_VIO_DOMAIN_SHIFT;

	vio_info->master_id = (vio_dbg & INFRACFG_MM2ND_VIO_ID_MASK) >>
		INFRACFG_MM2ND_VIO_ID_SHIFT;

	rw = (vio_dbg & INFRACFG_MM2ND_VIO_RW_MASK) >>
		INFRACFG_MM2ND_VIO_RW_SHIFT;
	vio_info->read = (rw == 0);
	vio_info->write = (rw == 1);
}

static uint32_t mt6880_shift_group_get(int slave_type, uint32_t vio_idx)
{
	if (slave_type == SLAVE_TYPE_INFRA) {
		if ((vio_idx >= 0 && vio_idx <= 8) || vio_idx == 79)
			return 0;
		else if ((vio_idx >= 9 && vio_idx <= 12) || vio_idx == 80)
			return 1;
		else if ((vio_idx >= 13 && vio_idx <= 22) || vio_idx == 81)
			return 2;
		else if ((vio_idx >= 23 && vio_idx <= 27) || vio_idx == 82)
			return 3;
		else if ((vio_idx >= 28 && vio_idx <= 29) || vio_idx == 83)
			return 4;
		else if (vio_idx >= 30 && vio_idx <= 69)
			return 5;
		else if ((vio_idx >= 70 && vio_idx <= 78) ||
			 (vio_idx >= 84 && vio_idx <= 93))
			return 4;

		pr_err(PFX "%s:%d Wrong vio_idx:0x%x\n",
				__func__, __LINE__, vio_idx);

	} else if (slave_type == SLAVE_TYPE_PERI) {
		if (vio_idx == 0)
			return 0;
		else if (vio_idx == 1)
			return 1;
		else if (vio_idx >= 2 && vio_idx <= 6)
			return 2;
		else if (vio_idx >= 7 && vio_idx <= 8)
			return 3;
		else if ((vio_idx >= 9 && vio_idx <= 38) ||
			 (vio_idx >= 141 && vio_idx <= 171) ||
			 vio_idx == 218)
			return 4;
		else if ((vio_idx >= 39 && vio_idx <= 49) || vio_idx == 172)
			return 5;
		else if ((vio_idx >= 50 && vio_idx <= 51) || vio_idx == 173)
			return 6;
		else if (vio_idx == 52 || vio_idx == 174)
			return 7;
		else if ((vio_idx >= 53 && vio_idx <= 95) || vio_idx == 175)
			return 8;
		else if ((vio_idx >= 96 && vio_idx <= 98) || vio_idx == 176)
			return 9;
		else if (vio_idx == 99 || (vio_idx >= 177 && vio_idx <= 178) ||
			 vio_idx == 219)
			return 10;
		else if (vio_idx >= 100 && vio_idx <= 101)
			return 11;
		else if (vio_idx >= 102 && vio_idx <= 103)
			return 12;
		else if ((vio_idx >= 104 && vio_idx <= 133) ||
			 (vio_idx >= 179 && vio_idx <= 209) || vio_idx == 220)
			return 13;
		else if ((vio_idx >= 134 && vio_idx <= 140) ||
			 (vio_idx >= 210 && vio_idx <= 217) || vio_idx == 221)
			return 14;

		pr_err(PFX "%s:%d Wrong vio_idx:0x%x\n",
				__func__, __LINE__, vio_idx);

	} else if (slave_type == SLAVE_TYPE_PERI2) {
		if ((vio_idx >= 0 && vio_idx <= 2) ||
		    (vio_idx >= 137 && vio_idx <= 159) ||
		    vio_idx == 274)
			return 0;
		else if (vio_idx >= 22 && vio_idx <= 25)
			return 1;
		else if (vio_idx >= 26 && vio_idx <= 29)
			return 2;
		else if ((vio_idx >= 30 && vio_idx <= 45) ||
			 (vio_idx >= 160 && vio_idx <= 176) ||
			 vio_idx == 275)
			return 3;
		else if ((vio_idx >= 46 && vio_idx <= 53) ||
			 (vio_idx >= 177 && vio_idx <= 185) ||
			 vio_idx == 276)
			return 4;
		else if ((vio_idx >= 54 && vio_idx <= 69) ||
			 (vio_idx >= 186 && vio_idx <= 202) ||
			 vio_idx == 277)
			return 5;
		else if ((vio_idx >= 70 && vio_idx <= 85) ||
			 (vio_idx >= 203 && vio_idx <= 219) ||
			 vio_idx == 278)
			return 6;
		else if ((vio_idx >= 86 && vio_idx <= 93) ||
			 (vio_idx >= 220 && vio_idx <= 228) ||
			 vio_idx == 279)
			return 7;
		else if ((vio_idx >= 94 && vio_idx <= 124) ||
			 (vio_idx >= 229 && vio_idx <= 260) ||
			 vio_idx == 280)
			return 8;
		else if ((vio_idx >= 125 && vio_idx <= 136) ||
			 (vio_idx >= 261 && vio_idx <= 273) ||
			 vio_idx == 281)
			return 9;

		pr_err(PFX "%s:%d Wrong vio_idx:0x%x\n",
				__func__, __LINE__, vio_idx);

	} else if (slave_type == SLAVE_TYPE_PERI_PAR) {
		if (vio_idx == 0)
			return 0;
		else if ((vio_idx >= 1 && vio_idx <= 7) ||
			 (vio_idx >= 46 && vio_idx <= 52) ||
			 vio_idx == 93)
			return 1;
		else if ((vio_idx >= 8 && vio_idx <= 33) ||
			 (vio_idx >= 53 && vio_idx <= 79) ||
			 vio_idx == 94)
			return 2;
		else if (vio_idx == 34 || vio_idx == 80)
			return 3;
		else if (vio_idx == 35 || vio_idx == 81)
			return 4;
		else if (vio_idx == 36 || vio_idx == 82)
			return 5;
		else if ((vio_idx >= 37 && vio_idx <= 40) ||
			 (vio_idx >= 83 && vio_idx <= 86) ||
			 vio_idx == 95)
			return 6;
		else if ((vio_idx >= 41 && vio_idx <= 42) ||
			 (vio_idx >= 87 && vio_idx <= 88) ||
			 vio_idx == 96)
			return 7;
		else if ((vio_idx >= 43 && vio_idx <= 45) ||
			 (vio_idx >= 89 && vio_idx <= 92) ||
			 vio_idx == 97)
			return 8;

		pr_err(PFX "%s:%d Wrong vio_idx:0x%x\n",
				__func__, __LINE__, vio_idx);

	}

	pr_err(PFX "%s:%d Wrong slave_type:0x%x\n",
			__func__, __LINE__, slave_type);

	return 31;
}

void devapc_catch_illegal_range(phys_addr_t phys_addr, size_t size)
{
	/*
	 * Catch BROM addr mapped
	 */
	if (phys_addr >= 0x0 && phys_addr < SRAM_START_ADDR) {
		pr_err(PFX "%s: %s %s:(%pa), %s:(0x%lx)\n",
				"catch BROM address mapped!",
				__func__, "phys_addr", &phys_addr,
				"size", size);
		BUG_ON(1);
	}
}
EXPORT_SYMBOL(devapc_catch_illegal_range);

static struct mtk_devapc_dbg_status mt6880_devapc_dbg_stat = {
	.enable_ut = PLAT_DBG_UT_DEFAULT,
	.enable_KE = PLAT_DBG_KE_DEFAULT,
	.enable_AEE = PLAT_DBG_AEE_DEFAULT,
	.enable_WARN = PLAT_DBG_WARN_DEFAULT,
	.enable_dapc = PLAT_DBG_DAPC_DEFAULT,
};

static const char * const slave_type_to_str[] = {
	"SLAVE_TYPE_INFRA",
	"SLAVE_TYPE_PERI",
	"SLAVE_TYPE_PERI2",
	"SLAVE_TYPE_PERI_PAR",
	"WRONG_SLAVE_TYPE",
};

static int mtk_vio_mask_sta_num[] = {
	VIO_MASK_STA_NUM_INFRA,
	VIO_MASK_STA_NUM_PERI,
	VIO_MASK_STA_NUM_PERI2,
	VIO_MASK_STA_NUM_PERI_PAR,
};

static struct mtk_devapc_vio_info mt6880_devapc_vio_info = {
	.vio_mask_sta_num = mtk_vio_mask_sta_num,
	.sramrom_vio_idx = SRAMROM_VIO_INDEX,
	.mmsys_vio_idx = MMSYS_VIO_INDEX,
	.sramrom_slv_type = SRAMROM_SLAVE_TYPE,
	.mm2nd_slv_type = MM2ND_SLAVE_TYPE,
};

static const struct mtk_infra_vio_dbg_desc mt6880_vio_dbgs = {
	.vio_dbg_mstid = INFRA_VIO_DBG_MSTID,
	.vio_dbg_mstid_start_bit = INFRA_VIO_DBG_MSTID_START_BIT,
	.vio_dbg_dmnid = INFRA_VIO_DBG_DMNID,
	.vio_dbg_dmnid_start_bit = INFRA_VIO_DBG_DMNID_START_BIT,
	.vio_dbg_w_vio = INFRA_VIO_DBG_W_VIO,
	.vio_dbg_w_vio_start_bit = INFRA_VIO_DBG_W_VIO_START_BIT,
	.vio_dbg_r_vio = INFRA_VIO_DBG_R_VIO,
	.vio_dbg_r_vio_start_bit = INFRA_VIO_DBG_R_VIO_START_BIT,
	.vio_addr_high = INFRA_VIO_ADDR_HIGH,
	.vio_addr_high_start_bit = INFRA_VIO_ADDR_HIGH_START_BIT,
};

static const struct mtk_sramrom_sec_vio_desc mt6880_sramrom_sec_vios = {
	.vio_id_mask = SRAMROM_SEC_VIO_ID_MASK,
	.vio_id_shift = SRAMROM_SEC_VIO_ID_SHIFT,
	.vio_domain_mask = SRAMROM_SEC_VIO_DOMAIN_MASK,
	.vio_domain_shift = SRAMROM_SEC_VIO_DOMAIN_SHIFT,
	.vio_rw_mask = SRAMROM_SEC_VIO_RW_MASK,
	.vio_rw_shift = SRAMROM_SEC_VIO_RW_SHIFT,
};

static const uint32_t mt6880_devapc_pds[] = {
	PD_VIO_MASK_OFFSET,
	PD_VIO_STA_OFFSET,
	PD_VIO_DBG0_OFFSET,
	PD_VIO_DBG1_OFFSET,
	PD_VIO_DBG2_OFFSET,
	PD_APC_CON_OFFSET,
	PD_SHIFT_STA_OFFSET,
	PD_SHIFT_SEL_OFFSET,
	PD_SHIFT_CON_OFFSET,
};

static struct mtk_devapc_soc mt6880_data = {
	.dbg_stat = &mt6880_devapc_dbg_stat,
	.slave_type_arr = slave_type_to_str,
	.slave_type_num = SLAVE_TYPE_NUM,
	.device_info[SLAVE_TYPE_INFRA] = mt6880_devices_infra,
	.device_info[SLAVE_TYPE_PERI] = mt6880_devices_peri,
	.device_info[SLAVE_TYPE_PERI2] = mt6880_devices_peri2,
	.device_info[SLAVE_TYPE_PERI_PAR] = mt6880_devices_peri_par,
	.ndevices = mt6880_devices_num,
	.vio_info = &mt6880_devapc_vio_info,
	.vio_dbgs = &mt6880_vio_dbgs,
	.sramrom_sec_vios = &mt6880_sramrom_sec_vios,
	.devapc_pds = mt6880_devapc_pds,
	.subsys_get = &index_to_subsys,
	.master_get = &mt6880_bus_id_to_master,
	.mm2nd_vio_handler = &mm2nd_vio_handler,
	.shift_group_get = mt6880_shift_group_get,
};

static const struct of_device_id mt6880_devapc_dt_match[] = {
	{ .compatible = "mediatek,mt6880-devapc" },
	{},
};

static int mt6880_devapc_probe(struct platform_device *pdev)
{
	return mtk_devapc_probe(pdev, &mt6880_data);
}

static int mt6880_devapc_remove(struct platform_device *dev)
{
	return mtk_devapc_remove(dev);
}

static struct platform_driver mt6880_devapc_driver = {
	.probe = mt6880_devapc_probe,
	.remove = mt6880_devapc_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = mt6880_devapc_dt_match,
	},
};

module_platform_driver(mt6880_devapc_driver);

MODULE_DESCRIPTION("Mediatek MT6880 Device APC Driver");
MODULE_AUTHOR("Neal Liu <neal.liu@mediatek.com>");
MODULE_LICENSE("GPL");
