// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_GPUFREQ_H__
#define __MTK_GPUFREQ_H__

// #if defined(CONFIG_MACH_MT6885)
// #include "mt6885/mtk_gpufreq_plat.h"

// #elif defined(CONFIG_MACH_MT6873)
// #include "mt6873/mtk_gpufreq_plat.h"

// #elif defined(CONFIG_MACH_MT6853)
// #include "mt6853/mtk_gpufreq_plat.h"
// #endif


#include <linux/module.h>
#include <linux/clk.h>

#define BUCK_ON 1
#define BUCK_OFF 0
#define BUCK_ENFORCE_OFF 4

#define DRV_Reg32(addr) readl(addr)
#define DRV_WriteReg32(addr, val) writel(val, addr)
#define DRV_SetReg32(addr, val) DRV_WriteReg32(addr, DRV_Reg32(addr) | (val))
#define DRV_ClrReg32(addr, val) DRV_WriteReg32(addr, DRV_Reg32(addr) & ~(val))

struct mt_gpufreq_power_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_power;
};

typedef void (*gpufreq_power_limit_notify)(unsigned int); /* legacy */

/****************************
 * MTK GPUFREQ API
 ****************************/
extern unsigned int mt_gpufreq_get_cur_freq_index(void);
extern unsigned int mt_gpufreq_get_cur_freq(void);
extern unsigned int mt_gpufreq_get_cur_volt(void);
extern unsigned int mt_gpufreq_get_cur_vsram(void);
extern unsigned int mt_gpufreq_get_dvfs_table_num(void);
extern unsigned int mt_gpufreq_target(unsigned int idx);
extern unsigned int mt_gpufreq_voltage_enable_set(unsigned int enable);
extern unsigned int
mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size);
extern unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx);
extern unsigned int mt_gpufreq_get_volt_by_idx(unsigned int idx);
extern unsigned int mt_gpufreq_get_ori_opp_idx(unsigned int idx);
extern unsigned int
mt_gpufreq_get_dyn_power(unsigned int freq_khz, unsigned int volt);
extern unsigned int mt_gpufreq_get_pow_by_idx(unsigned int idx);
extern void mt_gpufreq_thermal_protect(unsigned int limited_power);
extern void mt_gpufreq_restore_default_volt(void);
extern void mt_gpufreq_enable_by_ptpod(void);
extern void mt_gpufreq_disable_by_ptpod(void);
extern unsigned int mt_gpufreq_get_max_power(void);
extern unsigned int mt_gpufreq_get_min_power(void);
extern unsigned int mt_gpufreq_get_thermal_limit_index(void);
extern unsigned int mt_gpufreq_get_thermal_limit_freq(void);
extern void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power);
extern unsigned int mt_gpufreq_get_leakage_mw(void);
extern int mt_gpufreq_get_cur_ceiling_idx(void);
extern int mt_gpufreq_get_opp_idx_by_freq(unsigned int gpufreq_hz);
extern void mt_gpufreq_enable_CG(void);
extern void mt_gpufreq_disable_CG(void);
extern void mt_gpufreq_enable_MTCMOS(bool bEnableHWAPM);
extern void mt_gpufreq_disable_MTCMOS(bool bEnableHWAPM);
extern void mt_gpufreq_set_loading(unsigned int gpu_loading); /* legacy */
extern struct mt_gpufreq_power_table_info *mtk_gpufreq_get_powtab(int *num_ret);
extern unsigned int mt_gpufreq_get_seg_max_opp_index(void);
extern void mt_gpufreq_external_cg_control(void);
extern void mt_gpufreq_check_bus_idle(void);


#endif /* __MTK_GPUFREQ_H__ */

