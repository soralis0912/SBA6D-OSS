// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/soc/mediatek/mtk_dvfsrc.h>

#include "dvfsrc-met.h"
#include "dvfsrc-common.h"

static inline u32 dvfsrc_met_read(struct mtk_dvfsrc_met *dvfs, u32 offset)
{
	return readl(dvfs->regs + offset);
}

enum met_src_index {
	MD2SPM = 0,
	DDR__SW_REQ1_SPM,
	DDR__QOS_BW,
	DDR__EMI_TOTAL,
	DDR__HRT_BW,
	DDR__MD_LATENCY,
	DDR__MD_DDR,
	DDR__NETSYS,
	VCORE__SW_REQ3_PMQOS,
	VCORE__SW_REQ7_DPAMIF,
	VCORE__NETSYS,
	VCORE__PCIE_0,
	VCORE__PCIE_1,
	VCORE__PCIE_2,
	VCORE__PCIE_3,
	PMQOS_TATOL,
	PMQOS_BW0,
	PMQOS_BW1,
	PMQOS_BW2,
	PMQOS_BW3,
	PMQOS_BW4,
	PMQOS_PEAK_BW,
	HRT_MD_BW,
	MD_SCENARIO,
	MD_EMI_LATENCY,
	SRC_MAX,
};

/* met profile table */
static unsigned int met_vcorefs_src[SRC_MAX];

static char *met_src_name[SRC_MAX] = {
	"MD2SPM",
	"DDR__SW_REQ1_SPM",
	"DDR__QOS_BW",
	"DDR__EMI_TOTAL",
	"DDR__HRT_BW",
	"DDR__MD_LATENCY",
	"DDR__MD_DDR",
	"DDR__NETSYS",
	"VCORE__SW_REQ3_PMQOS",
	"VCORE__SW_REQ7_DPAMIF",
	"VCORE__NETSYS",
	"VCORE__PCIE_0",
	"VCORE__PCIE_1",
	"VCORE__PCIE_2",
	"VCORE__PCIE_3",
	"PMQOS_TATOL",
	"PMQOS_BW0",
	"PMQOS_BW1",
	"PMQOS_BW2",
	"PMQOS_BW3",
	"PMQOS_BW4",
	"PMQOS_PEAK_BW",
	"HRT_MD_BW",
	"MD_SCENARIO",
	"MD_EMI_LATENCY",
};

#define DVFSRC_SW_REQ1	(0x4)
#define DVFSRC_SW_REQ2	(0x8)
#define DVFSRC_SW_REQ3	(0xC)
#define DVFSRC_SW_REQ4	(0x10)
#define DVFSRC_SW_REQ5	(0x14)
#define DVFSRC_SW_REQ6	(0x18)
#define DVFSRC_SW_REQ7	(0x1C)
#define DVFSRC_SW_REQ8	(0x20)
#define DVFSRC_ISP_HRT  (0x290)

#define DVFSRC_VCORE_REQUEST (0x6C)
#define DVFSRC_DEBUG_STA_0 (0x700)
#define DVFSRC_DEBUG_STA_2 (0x708)
#define DVFSRC_DEBUG_STA_5 (0x714)
#define DVFSRC_DEBUG_STA_6 (0x718)

#define DVFSRC_PCIE_VCORE_REQ (0xE0)
#define DVFSRC_PCIE_VCORE_REQ_2 (0xEC)
#define DVFSRC_SW_BW_0	(0x260)
#define DVFSRC_SW_BW_1	(0x264)
#define DVFSRC_SW_BW_2	(0x268)
#define DVFSRC_SW_BW_3	(0x26C)
#define DVFSRC_SW_BW_4	(0x270)
#define DVFSRC_SW_BW_5	(0x274)
#define DVFSRC_SW_BW_6	(0x278)

#define DVFSRC_CURRENT_LEVEL	(0xD44)

/* DVFSRC_SW_REQX */
#define DDR_SW_AP_SHIFT		12
#define DDR_SW_AP_MASK		0x7
#define VCORE_SW_AP_SHIFT	4
#define VCORE_SW_AP_MASK	0x7

/* DVFSRC_VCORE_REQUEST 0x70 */
#define VCORE_SCP_GEAR_SHIFT	12
#define VCORE_SCP_GEAR_MASK		0x7

/* met profile function */
static int dvfsrc_get_src_req_num(void)
{
	return SRC_MAX;
}

static char **dvfsrc_get_src_req_name(void)
{
	return met_src_name;
}

static u32 dvfsrc_get_current_level(struct mtk_dvfsrc_met *dvfsrc)
{
	return dvfsrc_met_read(dvfsrc, DVFSRC_CURRENT_LEVEL);
}

static u32 dvfsrc_ddr_qos(struct mtk_dvfsrc_met *dvfs)
{
	u32 qos_total_bw = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_0) +
			   dvfsrc_met_read(dvfs, DVFSRC_SW_BW_1) +
			   dvfsrc_met_read(dvfs, DVFSRC_SW_BW_2) +
			   dvfsrc_met_read(dvfs, DVFSRC_SW_BW_3) +
			   dvfsrc_met_read(dvfs, DVFSRC_SW_BW_4);
	u32 qos_peak_bw = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_6);

	qos_total_bw = max_t(u32, qos_total_bw, qos_peak_bw);

	if (qos_total_bw < 0x19)
		return 0;
	else if (qos_total_bw < 0x26)
		return 1;
	else if (qos_total_bw < 0x33)
		return 2;
	else if (qos_total_bw < 0x3B)
		return 3;
	else if (qos_total_bw < 0x4C)
		return 4;
	else if (qos_total_bw < 0x66)
		return 5;
	else
		return 6;
}

static int dvfsrc_emi_mon_gear(struct mtk_dvfsrc_met *dvfs)
{
	unsigned int total_bw_status;
	int i;

	total_bw_status = dvfsrc_met_read(dvfs, DVFSRC_DEBUG_STA_2) & 0x3F;
	for (i = 5; i >= 0 ; i--) {
		if ((total_bw_status >> i) > 0)
			return i + 1;
	}

	return 0;
}

static void vcorefs_get_src_ddr_req(struct mtk_dvfsrc_met *dvfs)
{
	unsigned int val;

	val = dvfsrc_met_read(dvfs, DVFSRC_SW_REQ1);
	met_vcorefs_src[DDR__SW_REQ1_SPM] =
		(val >> DDR_SW_AP_SHIFT) & DDR_SW_AP_MASK;

	met_vcorefs_src[DDR__QOS_BW] =
		dvfsrc_ddr_qos(dvfs);

	met_vcorefs_src[DDR__EMI_TOTAL] =
		dvfsrc_emi_mon_gear(dvfs);

	met_vcorefs_src[DDR__HRT_BW] =
		mtk_dvfsrc_query_debug_info(DVFSRC_HRT_BW_DDR_REQ);

	met_vcorefs_src[DDR__MD_LATENCY] =
		mtk_dvfsrc_query_debug_info(DVFSRC_MD_IMP_DDR_REQ);

	met_vcorefs_src[DDR__MD_DDR] =
		mtk_dvfsrc_query_debug_info(DVFSRC_MD_SCEN_DDR_REQ);

	val = dvfsrc_met_read(dvfs, DVFSRC_DEBUG_STA_5);
	met_vcorefs_src[DDR__NETSYS] = (val >> 23) & 0x7;
}

static void vcorefs_get_src_vcore_req(struct mtk_dvfsrc_met *dvfs)
{
	u32 val;
	u32 sta;

	val = dvfsrc_met_read(dvfs, DVFSRC_SW_REQ3);
	met_vcorefs_src[VCORE__SW_REQ3_PMQOS] =
		(val >> VCORE_SW_AP_SHIFT) & VCORE_SW_AP_MASK;

	val = dvfsrc_met_read(dvfs, DVFSRC_SW_REQ7);
	met_vcorefs_src[VCORE__SW_REQ7_DPAMIF] =
		(val >> VCORE_SW_AP_SHIFT) & VCORE_SW_AP_MASK;

	sta = dvfsrc_met_read(dvfs, DVFSRC_DEBUG_STA_6);
	if ((sta >> 10) & 0x1) {
		val = dvfsrc_met_read(dvfs, DVFSRC_PCIE_VCORE_REQ);
		met_vcorefs_src[VCORE__PCIE_0] = val & 0x7;
	} else
		met_vcorefs_src[VCORE__PCIE_0] = 0;

	sta = dvfsrc_met_read(dvfs, DVFSRC_DEBUG_STA_5);
	val = dvfsrc_met_read(dvfs, DVFSRC_PCIE_VCORE_REQ_2);
	if ((sta >> 26) & 0x1) {
		met_vcorefs_src[VCORE__PCIE_1] = val & 0x7;
	} else
		met_vcorefs_src[VCORE__PCIE_1] = 0;
	if ((sta >> 28) & 0x1) {
		met_vcorefs_src[VCORE__PCIE_2] = (val >> 4) & 0x7;
	} else
		met_vcorefs_src[VCORE__PCIE_2] = 0;
	if ((sta >> 30) & 0x1) {
		met_vcorefs_src[VCORE__PCIE_3] = (val >> 8) & 0x7;
	} else
		met_vcorefs_src[VCORE__PCIE_3] = 0;
	
	met_vcorefs_src[VCORE__NETSYS] = (sta >> 18) & 0x7;
}

static void vcorefs_get_src_misc_info(struct mtk_dvfsrc_met *dvfs)
{
	u32 qos_bw0, qos_bw1, qos_bw2, qos_bw3, qos_bw4;
	u32 sta0, sta2;

	qos_bw0 = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_0);
	qos_bw1 = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_1);
	qos_bw2 = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_2);
	qos_bw3 = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_3);
	qos_bw4 = dvfsrc_met_read(dvfs, DVFSRC_SW_BW_4);
	sta0 = dvfsrc_met_read(dvfs, DVFSRC_DEBUG_STA_0);
	sta2 = dvfsrc_met_read(dvfs, DVFSRC_DEBUG_STA_2);

	met_vcorefs_src[MD2SPM] =
		sta0 & 0xFFFF;

	met_vcorefs_src[PMQOS_PEAK_BW] =
		dvfsrc_met_read(dvfs, DVFSRC_SW_BW_6);

	met_vcorefs_src[PMQOS_TATOL] =
		qos_bw0 + qos_bw1 + qos_bw2 + qos_bw3 + qos_bw4;

	met_vcorefs_src[PMQOS_BW0] = qos_bw0;
	met_vcorefs_src[PMQOS_BW1] = qos_bw1;
	met_vcorefs_src[PMQOS_BW2] = qos_bw2;
	met_vcorefs_src[PMQOS_BW3] = qos_bw3;
	met_vcorefs_src[PMQOS_BW4] = qos_bw4;

	met_vcorefs_src[MD_SCENARIO] = sta0 & 0xFFFF;

	met_vcorefs_src[MD_EMI_LATENCY] = (sta2 >> 12) & 0x3;

	met_vcorefs_src[HRT_MD_BW] = mtk_dvfsrc_query_debug_info(DVFSRC_MD_HRT_BW);
}


static unsigned int *dvfsrc_get_src_req(struct mtk_dvfsrc_met *dvfs)
{
	vcorefs_get_src_ddr_req(dvfs);
	vcorefs_get_src_vcore_req(dvfs);
	vcorefs_get_src_misc_info(dvfs);

	return met_vcorefs_src;
}

static int dvfsrc_get_ddr_ratio(struct mtk_dvfsrc_met *dvfs)
{
	int dram_opp;

	dram_opp = mtk_dvfsrc_query_opp_info(MTK_DVFSRC_CURR_DRAM_OPP);

	if (dram_opp < 6)
		return 8;
	else
		return 4;
}

const struct dvfsrc_met_config mt6880_met_config = {
	.dvfsrc_get_src_req_num = dvfsrc_get_src_req_num,
	.dvfsrc_get_src_req_name = dvfsrc_get_src_req_name,
	.dvfsrc_get_src_req = dvfsrc_get_src_req,
	.dvfsrc_get_ddr_ratio = dvfsrc_get_ddr_ratio,
	.get_current_level = dvfsrc_get_current_level,
};

