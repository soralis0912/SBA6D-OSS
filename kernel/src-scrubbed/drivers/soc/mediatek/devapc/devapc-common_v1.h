/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#ifndef __DEVAPC_COMMON_V1_H__
#define __DEVAPC_COMMON_V1_H__

#include <linux/platform_device.h>
#include <linux/types.h>

/******************************************************************************
 * Configuration
 ******************************************************************************/
#define DEVAPC_COMMON_CALLBACK (0)

/******************************************************************************
 * VARIABLE DEFINATION
 ******************************************************************************/
#define MOD_NO_IN_1_DEVAPC	16
#define VIOLATION_TRIGGERED	1
#define DEAD			0xdeadbeaf
#define PFX			"[DEVAPC]: "
#define SLAVE_TYPE_NUM_MAX	10

#define devapc_dbg_read(p, s, fmt, args...) \
	(p += scnprintf(p, sizeof(s) - strlen(s), fmt, ##args))

#define UNUSED(x)		(void)(x)

/*******************************************************************************
* Definition : Violation Information
*
*******************************************************************************/
/* For DEVAPC VIO */
#define VIO_DBG_MSTID					0xFFFFFFFF
#define VIO_DBG_MSTID_START_BIT		0
#define VIO_DBG_DMNID				0x0000003F
#define VIO_DBG_DMNID_START_BIT		0
#define VIO_DBG_W_VIO					0x00000040
#define VIO_DBG_W_VIO_START_BIT		6
#define VIO_DBG_R_VIO					0x00000080
#define VIO_DBG_R_VIO_START_BIT		7
#define VIO_ADDR_HIGH					0x00000F00
#define VIO_ADDR_HIGH_START_BIT		8


/*******************************************************************************
* Data Structure
*
*******************************************************************************/
enum DEVAPC_DOM_ID {
	DOMAIN_0 = 0,
	DOMAIN_1,
	DOMAIN_2,
	DOMAIN_3,
	DOMAIN_4,
	DOMAIN_5,
	DOMAIN_6,
	DOMAIN_7,
	DOMAIN_8,
	DOMAIN_9,
	DOMAIN_10,
	DOMAIN_11,
	DOMAIN_12,
	DOMAIN_13,
	DOMAIN_14,
	DOMAIN_15,
	DOMAIN_OTHERS,
};

enum SRAMROM_VIO {
	ROM_VIOLATION = 0,
	SRAM_VIOLATION,
};

enum DEVAPC_TYPE2_VIO {
	SRAMROM_TYPE2_VIO = 0,
	MMSYS_TYPE2_VIO,
	TYPE2_VIO_NUM,
};

/*******************************************************************************
* Data Structure for PowerDown Register
*
*******************************************************************************/
enum DEVAPC_PD_REG_TYPE {
	VIO_MASK = 0,
	VIO_STA,
	VIO_DBG0,
	VIO_DBG1,
	VIO_DBG2,
	VIO_DBG3,
	APC_CON,
	VIO_SHIFT_STA,
	VIO_SHIFT_SEL,
	VIO_SHIFT_CON,
	PD_REG_TYPE_NUM,
};

enum DEVAPC_PD_OFFSET {
	PD_VIO_MASK_OFFSET = 0x0,
	PD_VIO_STA_OFFSET = 0x400,
	PD_VIO_DBG0_OFFSET = 0x900,
	PD_VIO_DBG1_OFFSET = 0x904,
	PD_VIO_DBG2_OFFSET = 0x908,
	PD_VIO_DBG3_OFFSET = 0x90C,
	PD_APC_CON_OFFSET = 0xF00,
	PD_SHIFT_STA_OFFSET = 0xF20,
	PD_SHIFT_SEL_OFFSET = 0xF30,
	PD_SHIFT_CON_OFFSET = 0xF10,
};

struct mtk_devapc_pd_dbg {
	uint32_t vio_dbg_dmnid;
	uint8_t vio_dbg_dmnid_start_bit;
	uint32_t vio_dbg_w_vio;
	uint8_t vio_dbg_w_vio_start_bit;
	uint32_t vio_dbg_r_vio;
	uint8_t vio_dbg_r_vio_start_bit;
};

/*******************************************************************************
* Data Structure for context
*
*******************************************************************************/
struct mtk_devapc_vio_info {
	bool read;
	bool write;
	uint32_t vio_addr;
	uint32_t vio_addr_high;
	uint32_t master_id;
	uint32_t domain_id;
	int shift_sta_bit;
	int vio_trigger_times;
};

struct mtk_devapc_vio_handle {
	bool enable_KE;
	bool enable_WARN;
};

/*******************************************************************************
* External Data Structure
*
*******************************************************************************/
struct mtk_devapc_pd_info {
	unsigned long base_reg;
	char * name;
	int mask_sta_num;
	int slave_num;
};

struct mtk_devapc_slave_info {
	int sys_index;
	int ctrl_index;
	int vio_index;
	const char *name;
	bool enable_vio_irq;
};

//Type 2 violation information
struct mtk_devapc_type2_vio_reg {
	const uint32_t reg_base;
	const uint32_t reg_offset;
	int vio_sta_num;
};

struct mtk_devapc_type2_vio_offset {
	uint32_t vio_id_mask;
	uint8_t vio_id_shift;
	uint32_t vio_domain_mask;
	uint8_t vio_domain_shift;
	uint32_t vio_rw_mask;
	uint8_t vio_rw_shift;
};

struct mtk_devapc_type2_vio {
	int slave_type;
	int vio_idx;
	struct mtk_devapc_type2_vio_offset *vio_offset;
	struct mtk_devapc_type2_vio_reg *vio_reg;
};

struct mtk_devapc_soc {
	int slave_num;
	const struct mtk_devapc_vio_handle devapc_vio_handle;
	const struct mtk_devapc_pd_info *devapc_pd_info;
	const struct mtk_devapc_slave_info *devapc_slave_info[SLAVE_TYPE_NUM_MAX];
	const struct mtk_devapc_type2_vio *devapc_type2_vio[TYPE2_VIO_NUM];
};

extern int mtk_devapc_probe(struct platform_device *pdev,
		struct mtk_devapc_soc *soc);
extern int mtk_devapc_remove(struct platform_device *dev);

#endif /* __DEVAPC_COMMON_V1_H__ */

