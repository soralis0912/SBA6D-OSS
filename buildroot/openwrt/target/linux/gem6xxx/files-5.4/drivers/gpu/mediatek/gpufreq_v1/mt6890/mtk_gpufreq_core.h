/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017 MediaTek Inc.
 */


#ifndef _MT_GPUFREQ_CORE_H_
#define _MT_GPUFREQ_CORE_H_

/*************************************************
 * MT6890 segment_1 :
 *************************************************/
#define SEG1_GPU_DVFS_FREQ0	(780000)/* KHz */
#define SEG1_GPU_DVFS_FREQ1	(570000)/* KHz */
#define SEG1_GPU_DVFS_FREQ2	(360000)/* KHz */
#define SEG1_GPU_DVFS_FREQ3	(300000)/* KHz */

#define SEG1_GPU_DVFS_VOLT0	(75000)/* mV x 100 */
#define SEG1_GPU_DVFS_VOLT1	(65000)/* mV x 100 */
#define SEG1_GPU_DVFS_VOLT2	(60000)/* mV x 100 */
#define SEG1_GPU_DVFS_VOLT3	(55000)/* mV x 100 */

/* VSRAM has fixed to 0.75V*/

/**************************************************
 * PMIC Setting
 **************************************************/
#define VGPU_MAX_VOLT	(SEG1_GPU_DVFS_VOLT0)
#define VCORE_OPP_0	750000
#define VCORE_OPP_1	650000
#define VCORE_OPP_2	600000
#define VCORE_OPP_3	550000
#define VCORE_OPP_UNREQ	550000
/**************************************************
 * Clock Setting
 **************************************************/
#define POST_DIV_4_MAX_FREQ	(950000)
#define POST_DIV_4_MIN_FREQ	(375000)
#define POST_DIV_8_MAX_FREQ	(475000)
#define POST_DIV_8_MIN_FREQ	(187500)
#define POST_DIV_MASK	(0x07000000)
#define POST_DIV_SHIFT	(24)
#define TO_MHz_HEAD	(100)
#define TO_MHz_TAIL	(10)
#define ROUNDING_VALUE	(5)
#define DDS_SHIFT	(14)
#define GPUPLL_FIN	(26)
#define GPUPLL_CON2	(g_apmixed_base + 0x620)
/**************************************************
 * Operation Definition
 **************************************************/
#define GPUOP(khz, volt, idx)	\
	{	\
		.gpufreq_khz = khz,	\
		.gpufreq_volt = volt,	\
		.gpufreq_idx = idx,	\
	}
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
/**************************************************
 * Enumerations
 **************************************************/
enum g_post_divider_power_enum  {
	POST_DIV2 = 1,
	POST_DIV4,
	POST_DIV8,
	POST_DIV16,
	POST_DIV_INIT,
};
enum g_clock_source_enum  {
	CLOCK_MAIN = 0,
	CLOCK_SUB,
};
enum g_limited_idx_enum {
	IDX_THERMAL_PROTECT_LIMITED = 0,
	IDX_LOW_BATT_LIMITED,
	IDX_BATT_PERCENT_LIMITED,
	IDX_BATT_OC_LIMITED,
	IDX_PBM_LIMITED,
	NUMBER_OF_LIMITED_IDX,
};

/**************************************************
 * Log Setting
 **************************************************/
#define GPUFERQ_TAG "[GPU/DVFS]"
#define gpufreq_perr(fmt, args...)\
	pr_debug(GPUFERQ_TAG"[ERROR]"fmt, ##args)
#define gpufreq_pwarn(fmt, args...)\
	pr_debug(GPUFERQ_TAG"[WARNING]"fmt, ##args)
#define gpufreq_pr_info(fmt, args...)\
	pr_info(GPUFERQ_TAG"[INFO]"fmt, ##args)
#define gpufreq_pr_debug(fmt, args...)\
	pr_debug(GPUFERQ_TAG"[DEBUG]"fmt, ##args)

#define	GPUFREQ_UNREFERENCED(param) ((void)(param))
#define	MT_GPUFREQ_SRAM_DEBUG
/**************************************************
 * Reference Power Setting
 **************************************************/
#define GPU_ACT_REF_POWER	(1285) /* mW  */
#define GPU_ACT_REF_FREQ	(900000) /* KHz */
#define GPU_ACT_REF_VOLT	(90000) /* mV x 100 */
#define GPU_DVFS_PTPOD_DISABLE_VOLT	(80000) /* mV x 100 */
/**************************************************
 * Structures
 **************************************************/
struct g_opp_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_idx;
};
struct g_clk_info {
	struct clk *clk_mux;/* main clock for mfg setting*/
	struct clk *clk_main_parent;/* sub clock for mfg trans mux setting*/
	struct clk *clk_sub_parent; /* sub clock for mfg trans parent setting*/
	struct clk *cg_bg3d; /* clock gating for mfg */
	struct device *mtcmos_mfg; /* for mtcmos pm_runtime */
};
struct g_pmic_info {
	struct regulator *reg_vcore;
};
#endif /* _MT_GPUFREQ_CORE_H_ */
/**************************************************
 * Proc Node Definition
 **************************************************/
#ifdef CONFIG_PROC_FS
#define PROC_FOPS_RW(name)	\
static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)\
{	\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));\
}	\
	static const struct file_operations mt_ ## name ## _proc_fops = {\
		.owner = THIS_MODULE,	\
		.open = mt_ ## name ## _proc_open,	\
		.read = seq_read,	\
		.llseek = seq_lseek,	\
		.release = single_release,	\
		.write = mt_ ## name ## _proc_write,	\
	}
#define PROC_FOPS_RO(name)	\
static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)\
{	\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));\
}	\
	static const struct file_operations mt_ ## name ## _proc_fops =	\
	{	\
		.owner = THIS_MODULE,	\
		.open = mt_ ## name ## _proc_open,	\
		.read = seq_read,	\
		.llseek = seq_lseek,	\
		.release = single_release,	\
	}
#define PROC_ENTRY(name) \
	{__stringify(name), &mt_ ## name ## _proc_fops}
#endif