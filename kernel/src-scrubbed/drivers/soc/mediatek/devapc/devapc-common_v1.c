// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/sched/debug.h>
#include <linux/uaccess.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/soc/mediatek/devapc_public.h>
#include "devapc-common_v1.h"

/*******************************************************************************
* Data Structure
*
*******************************************************************************/
static struct mtk_devapc_context {
	/* HW reg mapped addr */
	void __iomem *devapc_pd_base[SLAVE_TYPE_NUM_MAX];
	void __iomem *devapc_type2_vio_base[TYPE2_VIO_NUM];
	/* Clock and Interrupt */
	struct clk *devapc_infra_clk;
	uint32_t devapc_irq;
	/* SOC information */
	struct mtk_devapc_soc *soc;
	/* Driver context */
	struct mtk_devapc_vio_handle vio_handle;
	struct mtk_devapc_vio_info vio_info;
	bool clock_gate;
} mtk_devapc_ctx[1];

static const uint32_t mtk_devapc_pds[] = {
	PD_VIO_MASK_OFFSET,
	PD_VIO_STA_OFFSET,
	PD_VIO_DBG0_OFFSET,
	PD_VIO_DBG1_OFFSET,
	PD_VIO_DBG2_OFFSET,
	PD_VIO_DBG3_OFFSET,
	PD_APC_CON_OFFSET,
	PD_SHIFT_STA_OFFSET,
	PD_SHIFT_SEL_OFFSET,
	PD_SHIFT_CON_OFFSET,
};

static const struct mtk_devapc_pd_dbg devapc_pd_dbg = {
	.vio_dbg_dmnid = VIO_DBG_DMNID,
	.vio_dbg_dmnid_start_bit = VIO_DBG_DMNID_START_BIT,
	.vio_dbg_w_vio = VIO_DBG_W_VIO,
	.vio_dbg_w_vio_start_bit = VIO_DBG_W_VIO_START_BIT,
	.vio_dbg_r_vio = VIO_DBG_R_VIO,
	.vio_dbg_r_vio_start_bit = VIO_DBG_R_VIO_START_BIT,
};

static DEFINE_SPINLOCK(devapc_lock);
static LIST_HEAD(viocb_list);

/*******************************************************************************
* Basic Function
*
*******************************************************************************/
static const char * const perm_to_str[] = {
	"NO_PROTECTION",
	"SECURE_RW_ONLY",
	"SECURE_RW_NS_R_ONLY",
	"FORBIDDEN",
	"NO_PERM_CTRL"
};

static const char *perm_to_string(uint8_t perm)
{
	if (perm < 4)
		return perm_to_str[perm];
	else
		return perm_to_str[4];
}

static const char *slave_type_to_string(uint32_t slave_type)
{
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	const struct mtk_devapc_pd_info *devapc_pd_info = 
		mtk_devapc_ctx->soc->devapc_pd_info;

	if (slave_type < slave_num)
		return devapc_pd_info[slave_type].name;
	else
		return devapc_pd_info[slave_num].name;
}

static void __iomem *devapc_get_pd_reg(int slave_type,
				       enum DEVAPC_PD_REG_TYPE pd_reg_type,
				       uint32_t index)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	void __iomem *reg;

	if (unlikely(devapc_pd_info == NULL)) {
		pr_err(PFX "%s:%d NULL pointer\n", __func__, __LINE__);
		return NULL;
	}

    if ( ( 0 <= slave_type )
        && (slave_type < slave_num)
        && (index < devapc_pd_info[slave_type].mask_sta_num)
        && (pd_reg_type < PD_REG_TYPE_NUM)
        && ( 0 <=pd_reg_type )) {

		reg = mtk_devapc_ctx->devapc_pd_base[slave_type] +
			mtk_devapc_pds[pd_reg_type];

		if (pd_reg_type == VIO_MASK || pd_reg_type == VIO_STA)
			reg += 0x4 * index;

	} else {
		pr_err(PFX "%s:0x%x or %s:0x%x or %s:0x%x is out of boundary\n",
				"slave_type", slave_type,
				"pd_reg_type", pd_reg_type,
				"index", index);
		return NULL;
	}

	return reg;
}


/*******************************************************************************
* DEVAPC (PowerDown) Handle Violation Function
*
*******************************************************************************/
// Dump the VIO_SHIFT_STA, VIO_MASK, VIO_STA for each slave type.
// devapc_vio_dump is for debugging purpose.
static void devapc_vio_dump(void)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	void __iomem *pd_vio_shift_sta_reg;
	int slave_type, i;

	for (slave_type = 0; slave_type < slave_num; slave_type++) {

		pd_vio_shift_sta_reg = devapc_get_pd_reg(slave_type,
				VIO_SHIFT_STA, 0);

		pr_info(PFX "[%s] %s: 0x%x\n",
				slave_type_to_string(slave_type),
				"VIO_SHIFT_STA",
				readl(pd_vio_shift_sta_reg)
		       );

		for (i = 0; i < devapc_pd_info[slave_type].mask_sta_num; i++) {
			pr_info(PFX "%s: %s_%d: 0x%x, %s_%d: 0x%x\n",
				slave_type_to_string(slave_type),
				"VIO_MASK", i,
				readl(devapc_get_pd_reg(slave_type,
						VIO_MASK, i)),
				"VIO_STA", i,
				readl(devapc_get_pd_reg(slave_type,
						VIO_STA, i))
				);
		}
	}
}

// Check if the specific device has triggered the violation
static int32_t devapc_vio_check(int slave_type, uint32_t module)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	uint32_t apc_register_index;
	uint32_t apc_set_index;
	void __iomem *reg;

	apc_register_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_set_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if ((slave_type < slave_num) &&
		(apc_register_index < devapc_pd_info[slave_type].mask_sta_num)) {

		reg = devapc_get_pd_reg(slave_type, VIO_STA,
				apc_register_index);

	} else {
		pr_err(PFX "%s: %s, %s:0x%x, %s:0x%x\n",
				__func__, "out of boundary",
				"slave_type", slave_type,
				"module_index", module);
		return -EOVERFLOW;
	}

	if (readl(reg) & (0x1 << apc_set_index))
		return VIOLATION_TRIGGERED;
	else
		return 0;
}

// Clear Violation Status for specific device
// Befor Clear violation status, you should mask the IRQ first.
static int32_t devapc_vio_clear(int slave_type, uint32_t module)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	uint32_t apc_register_index;
	uint32_t apc_set_index;
	void __iomem *reg;

	apc_register_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_set_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if ((slave_type < slave_num) &&
		(apc_register_index < devapc_pd_info[slave_type].mask_sta_num)) {

		reg = devapc_get_pd_reg(slave_type, VIO_STA,
				apc_register_index);
		writel(0x1 << apc_set_index, reg);

	} else {
		pr_err(PFX "%s: %s, %s:0x%x, %s:0x%x\n",
				__func__, "out of boundary",
				"slave_type", slave_type,
				"module_index", module);
		return -EOVERFLOW;
	}

	return 0;
}

static void devapc_vio_mask(int slave_type, uint32_t module, bool mask)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	uint32_t apc_register_index;
	uint32_t apc_set_index;
	void __iomem *reg;

	apc_register_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_set_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if ((slave_type < slave_num) &&
		(apc_register_index < devapc_pd_info[slave_type].mask_sta_num)) {

		reg = devapc_get_pd_reg(slave_type, VIO_MASK,
				apc_register_index);

		if (mask)
			writel(readl(reg) | (1 << apc_set_index), reg);
		else
			writel(readl(reg) & (~(1 << apc_set_index)), reg);

	} else
		pr_err(PFX "%s: %s, %s:0x%x, %s:0x%x, %s:%s\n",
				__func__, "out of boundary",
				"slave_type", slave_type,
				"module_index", module,
				"mask", mask ? "true" : "false");
}

static int devapc_vio_get_shift(int slave_type)
{
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	uint32_t vio_shift_sta;
	int i;
	int shift_bit = -1;

	if (slave_type >= slave_num) {
		pr_err(PFX "%s: param check failed, %s:0x%x\n",
				__func__, "slave_type", slave_type);
		return -1;
	}

	vio_shift_sta = readl(devapc_get_pd_reg(slave_type, VIO_SHIFT_STA, 0));

	for (i = 0; i < MOD_NO_IN_1_DEVAPC * 2; i++) {
		if (vio_shift_sta & (0x1 << i)) {
			shift_bit = i;
			break;
		}
	}
	vio_info->shift_sta_bit = shift_bit;

	return shift_bit;
}

static uint32_t devapc_vio_sync(int slave_type, uint32_t shift_bit)
{
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	void __iomem *pd_vio_shift_sta_reg;
	void __iomem *pd_vio_shift_sel_reg;
	void __iomem *pd_vio_shift_con_reg;
	uint32_t shift_count;
	uint32_t sync_done;

	if (slave_type >= slave_num ||
			shift_bit >= (MOD_NO_IN_1_DEVAPC * 2)) {
		pr_err(PFX "%s:param check failed, %s:0x%x, %s:0x%x\n", __func__, 
				"slave_type", slave_type,
				"shift_bit", shift_bit);
		return 0;
	}

	pd_vio_shift_sta_reg = devapc_get_pd_reg(slave_type, VIO_SHIFT_STA, 0);
	pd_vio_shift_sel_reg = devapc_get_pd_reg(slave_type, VIO_SHIFT_SEL, 0);
	pd_vio_shift_con_reg = devapc_get_pd_reg(slave_type, VIO_SHIFT_CON, 0);

	pr_debug(PFX "%s:0x%x %s:0x%x\n",
			"slave_type", slave_type,
			"VIO_SHIFT_STA", readl(pd_vio_shift_sta_reg));
	
	writel(0x1 << shift_bit, pd_vio_shift_sel_reg);
	writel(0x1, pd_vio_shift_con_reg);

	for (shift_count = 0; (shift_count < 100) &&
			((readl(pd_vio_shift_con_reg) & 0x3) != 0x3);
			++shift_count)
		;

	if ((readl(pd_vio_shift_con_reg) & 0x3) == 0x3)
		sync_done = 1;
	else {
		sync_done = 0;
		pr_info(PFX "sync failed, shift_bit:0x%x\n", shift_bit);
	}

	/* Disable shift mechanism */
	writel(0x0, pd_vio_shift_con_reg);
	writel(0x0, pd_vio_shift_sel_reg);
	writel(0x1 << shift_bit, pd_vio_shift_sta_reg);

	pr_debug(PFX "(Post) %s:0x%x, %s:0x%x, %s:0x%x\n",
			"VIO_SHIFT_STA",
			readl(pd_vio_shift_sta_reg),
			"VIO_SHIFT_SEL",
			readl(pd_vio_shift_sel_reg),
			"VIO_SHIFT_CON",
			readl(pd_vio_shift_con_reg));

	return sync_done;
}

static void devapc_vio_extract(int slave_type)
{
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	void __iomem *vio_dbg0_reg, *vio_dbg1_reg, *vio_dbg2_reg, *vio_dbg3_reg;
	uint32_t dbg0;

	if (slave_type >= slave_num) {
		pr_err(PFX "%s: param check failed, %s:0x%x\n",
				__func__, "slave_type", slave_type);
		return;
	}

	vio_dbg0_reg = devapc_get_pd_reg(slave_type, VIO_DBG0, 0);
	vio_dbg1_reg = devapc_get_pd_reg(slave_type, VIO_DBG1, 0);
	vio_dbg2_reg = devapc_get_pd_reg(slave_type, VIO_DBG2, 0);
	vio_dbg3_reg = devapc_get_pd_reg(slave_type, VIO_DBG3, 0);

	/* Extract violation information */
	dbg0 = readl(vio_dbg0_reg);
	vio_info->master_id = readl(vio_dbg1_reg);
	vio_info->vio_addr = readl(vio_dbg2_reg);
	vio_info->vio_addr_high = readl(vio_dbg3_reg);

	vio_info->domain_id = (dbg0 & devapc_pd_dbg.vio_dbg_dmnid)
		>> devapc_pd_dbg.vio_dbg_dmnid_start_bit;
	vio_info->write = ((dbg0 & devapc_pd_dbg.vio_dbg_w_vio)
			>> devapc_pd_dbg.vio_dbg_w_vio_start_bit) == 1;
	vio_info->read = ((dbg0 & devapc_pd_dbg.vio_dbg_r_vio)
			>> devapc_pd_dbg.vio_dbg_r_vio_start_bit) == 1;

}

static void devapc_print_vio_info(void)
{
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;

	/* Print violation information */
	if (vio_info->write)
		pr_info(PFX "Write Violation\n");
	else if (vio_info->read)
		pr_info(PFX "Read Violation\n");
	else
		pr_err(PFX "R/W Violation are not raised\n");

	pr_info(PFX "%s%x, %s%x, %s%x, %s%x\n",
			"Vio Addr:0x", vio_info->vio_addr,
			"High:0x", vio_info->vio_addr_high,
			"Bus ID:0x", vio_info->master_id,
			"Dom ID:0x", vio_info->domain_id);

	pr_info(PFX "%s - %s%s, %s%x\n",
			"Violation",
			"Current Process:", current->comm,
			"PID:", current->pid);
}

/*******************************************************************************
* SMC call functions 
*
*******************************************************************************/
// Check whether violating the permission setting.
static uint8_t devapc_get_permission(int slave_type, int module_index, int domain)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	const struct mtk_devapc_slave_info **devapc_slave_info = mtk_devapc_ctx->soc->devapc_slave_info;
	int sys_index, ctrl_index, vio_index;
	uint32_t ret, apc_set_index;
	struct arm_smccc_res res;


	if (slave_type >= slave_num ||
			module_index >= devapc_pd_info[slave_type].slave_num) {
		pr_err(PFX "%s: param check failed, %s:0x%x, %s:0x%x\n",
				__func__,
				"slave_type", slave_type,
				"module_index", module_index);
		return 0xFF;
	}

	sys_index = devapc_slave_info[slave_type][module_index].sys_index;
	ctrl_index = devapc_slave_info[slave_type][module_index].ctrl_index;
	vio_index = devapc_slave_info[slave_type][module_index].vio_index;

	if (sys_index == -1 || ctrl_index == -1) {
		pr_err(PFX "%s: cannot get sys_index & ctrl_index\n",
				__func__);
		return 0xFF;
	} else if (sys_index == -2) {
		pr_info(PFX "%s: check ATF logs for type2 permssion\n",
				__func__);
	}

	arm_smccc_smc(MTK_SIP_KERNEL_DAPC_PERM_GET, slave_type, sys_index,
			domain, ctrl_index, vio_index, 0, 0, &res);
	ret = res.a0;

	if (ret == DEAD) {
		pr_err(PFX "%s: permission get failed, ret:0x%x\n",
				__func__, ret);
		return 0xFF;
	}

	return (ret & 0x3);
}

//Handle the SRAMROM violation
static void sramrom_vio_handler(void)
{
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	struct mtk_devapc_type2_vio_offset *vio_offset =
		mtk_devapc_ctx->soc->devapc_type2_vio[SRAMROM_TYPE2_VIO]->vio_offset;
	struct arm_smccc_res res;
	uint32_t sramrom_vio_type;
	uint32_t sramrom_vio_rw;

	arm_smccc_smc(MTK_SIP_KERNEL_CLR_SRAMROM_VIO,
			0, 0, 0, 0, 0, 0, 0, &res);

	sramrom_vio_type = res.a0 & 0x1;
	sramrom_vio_rw = res.a0 >> 31;
	vio_info->domain_id = res.a1;
	vio_info->master_id = res.a2;
	vio_info->vio_addr = res.a3;

	if (sramrom_vio_type == SRAM_VIOLATION)
		pr_info(PFX "%s, SRAM violation is triggered\n", __func__);
	else if (sramrom_vio_type == ROM_VIOLATION)
		pr_info(PFX "%s, ROM violation is triggered\n", __func__);
	else {
		pr_info(PFX "SRAMROM violation is not triggered\n");
		return;
	}

	if (sramrom_vio_rw)
		vio_info->write = 1;
	else
		vio_info->read = 1;

	pr_info(PFX "%s: %s:0x%x, %s:0x%x, %s:%s, %s:0x%x\n",
		__func__, "master_id", vio_info->master_id,
		"domain_id", vio_info->domain_id,
		"rw", sramrom_vio_rw ? "Write" : "Read",
		"vio_addr", vio_info->vio_addr);
}

//Handle the MMSYS violation
static void mmsys_vio_handler(void)
{
	int type2 = MMSYS_TYPE2_VIO;
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	struct mtk_devapc_type2_vio_offset *vio_offset =
		mtk_devapc_ctx->soc->devapc_type2_vio[type2]->vio_offset;
	void __iomem * reg_base = mtk_devapc_ctx->devapc_type2_vio_base[type2];
	uint32_t reg_offset = mtk_devapc_ctx->soc->devapc_type2_vio[type2]->vio_reg->reg_offset;
	uint32_t vio_sta_num = mtk_devapc_ctx->soc->devapc_type2_vio[type2]->vio_reg->vio_sta_num;
	int i;
	void __iomem *reg;
	uint32_t vio_sta, vio_dbg, rw;

	if (!vio_info || !vio_offset) {
		pr_err(PFX "%s, param check failed, vio_offset is NULL\n",
				__func__);
		return;
	}

	pr_info(PFX "%s, MMSYS violation is triggered\n", __func__);

	//get violation status
	for (i = 0; i < vio_sta_num; i++) {
		reg = reg_base + reg_offset + i * 4;
		vio_sta = readl(reg);
		if (vio_sta)
			pr_info(PFX "Type2 violation: vio_sta(%d) : 0x%x\n", i, vio_sta);
	}

	//get violation address
	reg = reg_base + reg_offset + i * 4;
	vio_info->vio_addr = readl(reg);

	//get violation information
	reg = reg_base + reg_offset + i * 4;
	vio_dbg = readl(reg);

	vio_info->master_id = (vio_dbg & vio_offset->vio_id_mask)
		>> vio_offset->vio_id_shift;
	vio_info->domain_id = (vio_dbg & vio_offset->vio_domain_mask)
		>> vio_offset->vio_domain_shift;
	rw = (vio_dbg & vio_offset->vio_rw_mask)
		>> vio_offset->vio_rw_shift;
	if (rw)
		vio_info->write = 1;
	else
		vio_info->read = 1;

	pr_info(PFX "%s: %s:0x%x, %s:0x%x, %s:%s, %s:0x%x\n",
		__func__, "master_id", vio_info->master_id,
		"domain_id", vio_info->domain_id,
		"rw", rw ? "Write" : "Read",
		"vio_addr", vio_info->vio_addr);
}

/*******************************************************************************
* DEVAPC Handle the violation 
*
*******************************************************************************/
//Check Type 1 violation
static bool devapc_check_type1_vio(int slave_type, int *vio_idx, int *index)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	const struct mtk_devapc_slave_info **devapc_slave_info = mtk_devapc_ctx->soc->devapc_slave_info;

	int shift_bit;
	int i;

	if (unlikely(vio_idx == NULL)) {
		pr_err(PFX "%s:%d NULL pointer\n", __func__, __LINE__);
		return NULL;
	}

	for (i = 0; i < devapc_pd_info[slave_type].slave_num; i++) {
		if (devapc_vio_check(slave_type, devapc_slave_info[slave_type][i].vio_index) !=
				VIOLATION_TRIGGERED)
			continue;

		shift_bit = devapc_vio_get_shift(slave_type);

		if (shift_bit < 0)
			continue;

		if (!devapc_vio_sync(slave_type, shift_bit))
			continue;

		devapc_vio_extract(slave_type);
		devapc_print_vio_info();

		*vio_idx = devapc_slave_info[slave_type][i].vio_index;
		*index = i;

		return true;
	}

	pr_debug(PFX "%s: no violation for %s:0x%x\n", __func__ ,
		"slave_type", slave_type);

	return false;
}

//Check the Type 2 violation
static bool devapc_check_type2_vio(int slave_type, int *vio_idx, int *index)
{

	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	const struct mtk_devapc_slave_info **devapc_slave_info = mtk_devapc_ctx->soc->devapc_slave_info;
	uint32_t type2_slave_type, type2_vio_idx;
	int i;

	/* check SRAMROM */
	if ((mtk_devapc_ctx->soc->devapc_type2_vio[SRAMROM_TYPE2_VIO] != NULL)) {
		type2_slave_type = mtk_devapc_ctx->soc->devapc_type2_vio[SRAMROM_TYPE2_VIO]->slave_type;
		type2_vio_idx = mtk_devapc_ctx->soc->devapc_type2_vio[SRAMROM_TYPE2_VIO]->vio_idx;
		if (slave_type == type2_slave_type &&
				devapc_vio_check(slave_type, type2_vio_idx)) {

			pr_info(PFX "SRAMROM violation is triggered\n");
			
			sramrom_vio_handler();

			*vio_idx = type2_vio_idx;
			//find index
			for (i = 0; i < devapc_pd_info[slave_type].mask_sta_num; i++) {
				if (devapc_slave_info[slave_type][i].vio_index == *vio_idx)
					*index = i;
			}

			return true;
		}
	}

	/* check mmsys */
	if ((mtk_devapc_ctx->soc->devapc_type2_vio[MMSYS_TYPE2_VIO] != NULL)) {
		type2_slave_type = mtk_devapc_ctx->soc->devapc_type2_vio[MMSYS_TYPE2_VIO]->slave_type;
		type2_vio_idx = mtk_devapc_ctx->soc->devapc_type2_vio[MMSYS_TYPE2_VIO]->vio_idx;
		if (slave_type == type2_slave_type &&
				devapc_vio_check(slave_type, type2_vio_idx)) {

			pr_info(PFX "MMSYS violation is triggered\n");

			mmsys_vio_handler();

			*vio_idx = type2_vio_idx;
			//find index
			for (i = 0; i < devapc_pd_info[slave_type].mask_sta_num; i++) {
				if (devapc_slave_info[slave_type][i].vio_index == *vio_idx)
					*index = i;
			}

			return true;
		}
	}

	pr_debug(PFX "%s: no violation for %s:0x%x\n", __func__,
			"slave_type", slave_type);

	return false;
}

static void devapc_extra_handler(void)
{
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	struct mtk_devapc_vio_handle *vio_handle = &mtk_devapc_ctx->vio_handle;
	struct devapc_vio_callbacks *viocb;
	uint32_t ret_cb = 0;

	pr_info(PFX "%s:%d\n", "vio_trigger_times",
			mtk_devapc_ctx->vio_info.vio_trigger_times++);

#if (DEVAPC_COMMON_CALLBACK)
	//handle callback function
	list_for_each_entry(viocb, &viocb_list, list) {
		if (viocb->debug_dump)
			viocb->debug_dump();

		if (viocb->debug_dump_adv)
			ret_cb = viocb->debug_dump_adv(vio_info->vio_addr);
	}
#endif

	/* Trigger System Error Handling */
	if (vio_handle->enable_KE ) {
		pr_info(PFX "Device APC Violation Issue");
		BUG_ON(1);

	} else if (vio_handle->enable_WARN) {
		WARN(1, "Device APC Violation Issue");
	}
}

//There is only one violation when triggering interrupt
static irqreturn_t devapc_violation_irq(int irq_number, void *dev_id)
{
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	const struct mtk_devapc_slave_info **devapc_slave_info = mtk_devapc_ctx->soc->devapc_slave_info;
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	int slave_type, vio_idx, index;
	unsigned long flags;
	uint8_t perm;
	bool normal;

	spin_lock_irqsave(&devapc_lock, flags);

	normal = false;
	vio_idx = index = -1;

	pr_info(PFX "%s +++\n", __func__);

	/* There are multiple DEVAPC_PD */
	for (slave_type = 0; slave_type < slave_num; slave_type++) {

		if (!devapc_check_type1_vio(slave_type, &vio_idx, &index))
			if (!devapc_check_type2_vio(slave_type, &vio_idx,
						&index))
				continue;

		/* Ensure that violation info are written before
		 * further operations
		 */
		smp_mb();
		normal = true;

		devapc_vio_mask(slave_type, vio_idx, true);

		if (devapc_vio_clear(slave_type, vio_idx))
			pr_warn(PFX "%s, %s:0x%x, %s:0x%x\n",
					"devapc_vio_clear failed",
					"slave_type", slave_type,
					"vio_index", vio_idx);

        //For Coverity
        if( index < 0 ) {
            pr_info(PFX "WARNING: Abnormal index=%d\n", index);
            continue;
        }

		//For Type 1 only
		if( (devapc_slave_info[slave_type][index].sys_index != -1) &&
			devapc_slave_info[slave_type][index].ctrl_index != -1) {
			perm = devapc_get_permission(slave_type, index, vio_info->domain_id);
			//power/clock is not enabled
			//Not permission violation, should be skip to handle it.
			if (perm == 0 || perm > 3) {
				devapc_vio_mask(slave_type, vio_idx, false);
				pr_info(PFX "Reason: power/clock is not enabled\n");
				continue;
			}
			//permission denied
			pr_info(PFX "Permission setting: %s\n", perm_to_string(perm));
			pr_info(PFX "Reason: permission denied\n");
		}else{
			pr_info(PFX "Reason: Other type violation, %s, vio_idx=%d\n", 
				slave_type_to_string(slave_type), vio_idx);
		}

		devapc_extra_handler();

		devapc_vio_mask(slave_type, vio_idx, false);
	}

	pr_info(PFX "%s ---\n", __func__);
	if (normal) {
		spin_unlock_irqrestore(&devapc_lock, flags);
		return IRQ_HANDLED;
	}

	/* It's an abnormal status */
	pr_info(PFX "WARNING: Abnormal Status\n");
	devapc_vio_dump();

	spin_unlock_irqrestore(&devapc_lock, flags);
	return IRQ_HANDLED;
}

/*******************************************************************************
* DEVAPC External Function
*
*******************************************************************************/

void register_devapc_vio_callback(struct devapc_vio_callbacks *viocb)
{
	INIT_LIST_HEAD(&viocb->list);
	list_add_tail(&viocb->list, &viocb_list);
}
EXPORT_SYMBOL(register_devapc_vio_callback);

void unregister_devapc_vio_callback(struct devapc_vio_callbacks *viocb)
{
	struct devapc_vio_callbacks *target_viocb;
	
	list_for_each_entry(target_viocb, &viocb_list, list) {
		if (viocb->id == target_viocb->id) {
			break;
		}
	}
	list_del(&target_viocb->list);
}
EXPORT_SYMBOL(unregister_devapc_vio_callback);

/*******************************************************************************
* DEVAPC Test 
*
*******************************************************************************/
static void devapc_test_cb(void)
{
	pr_info(PFX "%s success !\n", __func__);
}

static enum devapc_cb_status devapc_test_adv_cb(uint32_t vio_addr)
{
	pr_info(PFX "%s success !\n", __func__);
	pr_info(PFX "vio_addr: 0x%x\n", vio_addr);

	return DEVAPC_NOT_KE;
}

static struct devapc_vio_callbacks devapc_test_handle = {
	.id = DEVAPC_SUBSYS_TEST,
	.debug_dump = devapc_test_cb,
	.debug_dump_adv = devapc_test_adv_cb,
};

static void devapc_trig(unsigned long reg, int rw)
{
	void __iomem *target_reg = ioremap(reg, 4096);
	pr_info(PFX "%s, %s  REG:0x%x\n", __func__, rw==1?"Write":"Read", target_reg);
	if(rw==1) {
		writel(0x1, target_reg);
	}else{
		readl(target_reg);
	}
}

ssize_t mtk_devapc_dbg_read(struct file *file, char __user *buffer,
	size_t count, loff_t *ppos)
{
	struct mtk_devapc_vio_info *vio_info = &mtk_devapc_ctx->vio_info;
	char msg_buf[1024] = {0};
	char *p = msg_buf;
	int len;

	if (unlikely(vio_info == NULL)) {
		pr_err(PFX "%s:%d NULL pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	devapc_dbg_read(p, msg_buf, "DEVAPC last violation:\n");
	if (vio_info->write)
		devapc_dbg_read(p, msg_buf, "\tWrite Violation\n");
	else if (vio_info->read)
		devapc_dbg_read(p, msg_buf, "\tRead Violation\n");
	else
		devapc_dbg_read(p, msg_buf, "\tR/W Violation are not raised\n");

	devapc_dbg_read(p, msg_buf, "\t%s%x, %s%x, %s%x, %s%x\n",
			"Vio Addr:0x", vio_info->vio_addr,
			"High:0x", vio_info->vio_addr_high,
			"Bus ID:0x", vio_info->master_id,
			"Dom ID:0x", vio_info->domain_id);
	devapc_dbg_read(p, msg_buf, "violation count = %d\n",
			vio_info->vio_trigger_times);
	devapc_dbg_read(p, msg_buf, "\n");

	len = p - msg_buf;

	return simple_read_from_buffer(buffer, count, ppos, msg_buf, len);
}

ssize_t mtk_devapc_dbg_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *data)
{
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;
	struct mtk_devapc_vio_handle *vio_handle = &mtk_devapc_ctx->vio_handle;
	long param, sys_index, domain, ctrl_index;
	uint32_t slave_type, apc_set_idx, ret;
	char *parm_str, *cmd_str, *pinput;
	struct arm_smccc_res res;
	char input[32] = {0};
	int err = -EINVAL;
	int len;

	if (unlikely(vio_handle == NULL)) {
		pr_err(PFX "%s:%d NULL pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	len = (count < (sizeof(input) - 1)) ? count : (sizeof(input) - 1);
	if (copy_from_user(input, buffer, len)) {
		pr_err(PFX "copy from user failed!\n");
		return -EFAULT;
	}

    //For coverity
    if( len < 0) {
        pr_err(PFX "len(%d) should >= 0\n", len);
        return -EFAULT;
    }

	input[len] = '\0';
	pinput = input;

	cmd_str = strsep(&pinput, " ");

	if (!cmd_str)
		return -EINVAL;

	parm_str = strsep(&pinput, " ");

	if (!parm_str)
		return -EINVAL;


	if (!strncmp(cmd_str, "register_cb", sizeof("register_cb"))) {
		if( kstrtol(parm_str, 10, &param))
			return err;

		pr_info(PFX "register_cb : %s\n", param ? "Register" : "Unregister");
		if(0 == param) {
			unregister_devapc_vio_callback(&devapc_test_handle);
		}else{
			register_devapc_vio_callback(&devapc_test_handle);
		}
		return count;

	} else if (!strncmp(cmd_str, "devapc_trig", sizeof("devapc_trig"))) {
		long rw=0;
		if( kstrtol(parm_str, 16, &param))
			return err;

		parm_str = strsep(&pinput, " ");
		if (parm_str)
			err = kstrtol(parm_str, 10, &rw);

		pr_info(PFX "devapc_trig : address=0x%x\n", param);
		devapc_trig(param, rw);
		return count;

	} else if (!strncmp(cmd_str, "enable_ke", sizeof("enable_ke"))) {
		if( kstrtol(parm_str, 10, &param))
			return err;

		vio_handle->enable_KE = (param != 0);
		pr_info(PFX "vio_handle->enable_KE = %s\n",
				vio_handle->enable_KE ?
				"enable" : "disable");
		return count;

	} else if (!strncmp(cmd_str, "enable_warn", sizeof("enable_warn"))) {
		if( kstrtol(parm_str, 10, &param))
			return err;

		vio_handle->enable_WARN = (param != 0);
		pr_info(PFX "vio_handle->enable_warn = %s\n",
				vio_handle->enable_WARN ?
				"enable" : "disable");
		return count;

	} else if (!strncmp(cmd_str, "dump_reg", sizeof("dump_reg"))) {
		devapc_vio_dump();
	} else if (!strncmp(cmd_str, "dump_apc", sizeof("dump_apc"))) {
		if( kstrtol(parm_str, 10, &param))
			return err;

		/* slave_type is already parse before */
		slave_type = (uint32_t)param;

		if (slave_type >= slave_num) {
			pr_err(PFX "Wrong slave type:0x%x\n", slave_type);
			return -EFAULT;
		}

		sys_index = 0xFFFFFFFF;
		ctrl_index = 0xFFFFFFFF;
		domain = DOMAIN_OTHERS;

		/* Parse sys_index */
		parm_str = strsep(&pinput, " ");
		if (parm_str)
			err = kstrtol(parm_str, 10, &sys_index);

		/* Parse ctrl_index */
		parm_str = strsep(&pinput, " ");
		if (parm_str != NULL)
			err = kstrtol(parm_str, 10, &ctrl_index);

		/* Parse domain id */
		parm_str = strsep(&pinput, " ");
		if (parm_str)
			err = kstrtol(parm_str, 10, &domain);

		pr_info(PFX "%s:0x%x, %s:0x%lx, %s:0x%lx, %s:0x%lx\n",
				"slave_type", slave_type,
				"sys_index", sys_index,
				"ctrl_index", ctrl_index,
				"domain_id", domain);

		arm_smccc_smc(MTK_SIP_KERNEL_DAPC_PERM_GET, slave_type,
				sys_index, domain, ctrl_index, 0, 0, 0, &res);
		ret = res.a0;

		if (ret == DEAD) {
			pr_err(PFX "%s, SMC call failed, ret: 0x%x\n",
					__func__, ret);
			return -EINVAL;
		}

		pr_info(PFX "Permission is %s\n",
			perm_to_string((ret & 0x3)));
		return count;
	}else
		return -EINVAL;

	return count;
}

static const struct file_operations devapc_dbg_fops = {
	.owner = THIS_MODULE,
	.write = mtk_devapc_dbg_write,
	.read = mtk_devapc_dbg_read,
};

/*******************************************************************************
* DEVAPC start
*
*******************************************************************************/
static void start_devapc(void)
{
	const struct mtk_devapc_pd_info *devapc_pd_info = mtk_devapc_ctx->soc->devapc_pd_info;
	const struct mtk_devapc_slave_info **devapc_slave_info = mtk_devapc_ctx->soc->devapc_slave_info;
	uint32_t slave_num = mtk_devapc_ctx->soc->slave_num;

	void __iomem *pd_vio_shift_sta_reg;
	void __iomem *pd_apc_con_reg;
	uint32_t vio_shift_sta;
	int slave_type, i, vio_idx;
	int index;

	pr_info(PFX "%s +++\n", __func__);
	
	//check each slave type
	for (slave_type = 0; slave_type < slave_num; slave_type++) {

		pd_apc_con_reg = devapc_get_pd_reg(slave_type, APC_CON, 0);
		pd_vio_shift_sta_reg = devapc_get_pd_reg(
				slave_type, VIO_SHIFT_STA, 0);

		if (unlikely(pd_apc_con_reg == NULL ||
			     pd_vio_shift_sta_reg == NULL)) {
			pr_err(PFX "%s:%d NULL pointer\n", __func__, __LINE__);
			return;
		}

		/* Clear DEVAPC violation status */
		writel(BIT(31), pd_apc_con_reg);

		/* Clear violation shift status */
		vio_shift_sta = readl(pd_vio_shift_sta_reg);

		if (vio_shift_sta) {
			writel(vio_shift_sta, pd_vio_shift_sta_reg);
			pr_debug(PFX "clear %s:0x%x %s:0x%x to 0x%x\n",
					"slave_type", slave_type,
					"VIO_SHIFT_STA", vio_shift_sta,
					readl(pd_vio_shift_sta_reg));
		}

		//set mask
		for (i = 0; i < devapc_pd_info[slave_type].slave_num; i++) {
			if (!devapc_slave_info[slave_type][i].enable_vio_irq)
				continue;
			
			vio_idx = devapc_slave_info[slave_type][i].vio_index;			
			if (devapc_vio_check(slave_type, vio_idx) == VIOLATION_TRIGGERED) {
				devapc_vio_clear(slave_type, vio_idx);
			}
			devapc_vio_mask(slave_type, vio_idx, false);
		}
	}

	//devapc_vio_dump();
	pr_info(PFX "%s ---\n", __func__);
}

int mtk_devapc_probe(struct platform_device *pdev,
		struct mtk_devapc_soc *soc)
{
	struct device_node *node = pdev->dev.of_node;
	uint32_t slave_type_num;
	unsigned long reg;
	int slave_type;
	int ret;

	pr_info(PFX "%s +++\n", __func__);

	if (IS_ERR(node)) {
		pr_err(PFX "cannot find device node\n");
		return -ENODEV;
	}

	mtk_devapc_ctx->soc = soc;
	slave_type_num = mtk_devapc_ctx->soc->slave_num;

	for (slave_type = 0; slave_type < slave_type_num; slave_type++) {
		reg = mtk_devapc_ctx->soc->devapc_pd_info[slave_type].base_reg;
		mtk_devapc_ctx->devapc_pd_base[slave_type] = ioremap(reg, 4096); //size=0x1000
		if (unlikely(mtk_devapc_ctx->devapc_pd_base[slave_type] == NULL)) {
			pr_err(PFX "parse devapc_pd_base:0x%x failed\n", slave_type);
			return -EINVAL;
		}
		pr_debug(PFX "slave_type:%d devapc_pd_base:0x%x\n",
			slave_type, reg);
	}

	//get MMSYS register base ioremap
	if(mtk_devapc_ctx->soc->devapc_type2_vio[MMSYS_TYPE2_VIO] != NULL) {
		reg = mtk_devapc_ctx->soc->devapc_type2_vio[MMSYS_TYPE2_VIO]->vio_reg->reg_base;
		mtk_devapc_ctx->devapc_type2_vio_base[MMSYS_TYPE2_VIO] = ioremap(reg, 4096);
		if (unlikely(mtk_devapc_ctx->devapc_type2_vio_base[MMSYS_TYPE2_VIO] == NULL)) {
			pr_err(PFX "parse mmsys_type2_base failed\n");
			return -EINVAL;
		}
		pr_debug(PFX "mmsys_type2_base:0x%x\n", reg);
	}

	//interrupt
	mtk_devapc_ctx->devapc_irq = irq_of_parse_and_map(node, 0);
	if (!mtk_devapc_ctx->devapc_irq) {
		pr_err(PFX "parse and map the interrupt failed\n");
		return -EINVAL;
	}
	pr_info(PFX " IRQ:%d\n", mtk_devapc_ctx->devapc_irq);

	ret = devm_request_irq(&pdev->dev, mtk_devapc_ctx->devapc_irq,
			(irq_handler_t)devapc_violation_irq,
			IRQF_TRIGGER_NONE, "devapc", NULL);
	if (ret) {
		pr_err(PFX "request devapc irq failed, ret:%d\n", ret);
		return ret;
	}

	// CCF (Common Clock Framework)
	mtk_devapc_ctx->clock_gate = false;
	mtk_devapc_ctx->devapc_infra_clk = devm_clk_get(&pdev->dev,
			"devapc-infra-clock");

	if (!IS_ERR(mtk_devapc_ctx->devapc_infra_clk)) {
		pr_info(PFX "Get devapc clock from CCF\n");
		mtk_devapc_ctx->clock_gate = true;
	}

	if (mtk_devapc_ctx->clock_gate && 
			clk_prepare_enable(mtk_devapc_ctx->devapc_infra_clk)) {
		pr_err(PFX " Cannot enable devapc clock\n");
		return -EINVAL;
	}

	proc_create("devapc_dbg", 0664, NULL, &devapc_dbg_fops);

	start_devapc();

	//set default violation handle
	pr_info(PFX "Set default violation handle : (%d, %d)\n",
		mtk_devapc_ctx->soc->devapc_vio_handle.enable_KE,
		mtk_devapc_ctx->soc->devapc_vio_handle.enable_WARN);

	mtk_devapc_ctx->vio_handle.enable_KE =
		mtk_devapc_ctx->soc->devapc_vio_handle.enable_KE;
	mtk_devapc_ctx->vio_handle.enable_WARN =
		mtk_devapc_ctx->soc->devapc_vio_handle.enable_WARN;

	pr_info(PFX "%s ---\n", __func__);
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_devapc_probe);

int mtk_devapc_remove(struct platform_device *dev)
{
	if (mtk_devapc_ctx->clock_gate)
		clk_disable_unprepare(mtk_devapc_ctx->devapc_infra_clk);

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_devapc_remove);

MODULE_DESCRIPTION("Mediatek Device APC Driver");
MODULE_AUTHOR("SR Huang <sr.huang@mediatek.com>");
MODULE_LICENSE("GPL");
