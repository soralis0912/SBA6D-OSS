// SPDX-License-Identifier: GPL-2.0
/*
 * mediatek-mt6880-cpufreq.c - MT6880 CPUFreq Driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Wei-Chia Su <Wei-Chia.Su@mediatek.com>
 */
#include <asm-generic/delay.h>
#include <clk-mtk.h>
#include <mt6880-clk.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/cpu.h>
#include <linux/cpu_cooling.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/energy_model.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/uaccess.h>
enum cpu_level {
	CPU_LEVEL_0,
	NUM_CPU_LEVEL
};
enum opp_idx_type {
	CUR_OPP_IDX = 0,
	TARGET_OPP_IDX = 1,
	NR_OPP_IDX,
};
enum mt_cpu_dvfs_id {
        MT_CPU_DVFS_LL,
        MT_CPU_DVFS_CCI,
        NR_MT_CPU_DVFS,
};
#define FHCTL (0)
#define mt_reg_sync_writel(v, a) \
        do {    \
                __raw_writel((v), (void __force __iomem *)((a)));   \
                mb();  \
        } while (0)
#define _BIT_(_bit_)	(unsigned)(1 << (_bit_))
#define _BITS_(_bits_, _val_)	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define _BITMASK_(_bits_)	(((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)	(((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))
#define IOMEM(a) ((void __force __iomem *)((a)))
#define cpufreq_read(addr) __raw_readl(IOMEM(addr))
#define cpufreq_write(addr, val) mt_reg_sync_writel((val), ((void *)addr))
#define cpufreq_write_mask(addr, mask, val) cpufreq_write(addr, (cpufreq_read(addr) & ~(_BITMASK_(mask))) | _BITS_(mask, val))
#define PLL_SETTLE_TIME	20
#define POS_SETTLE_TIME	1
#define APMIXED_NODE "mediatek,apmixed"
#define MCUCFG_NODE "mediatek,mcucfg"
#define cpu_dvfs_is(p, id) (p == &cpu_dvfs[id])
#define for_each_cpu_dvfs(i, p) for (i = 0, p = cpu_dvfs; i < NR_MT_CPU_DVFS; i++, p = &cpu_dvfs[i])
#define for_each_cpu_dvfs_only(i, p) \
        for (i = 0, p = cpu_dvfs; (i < NR_MT_CPU_DVFS) && (i != MT_CPU_DVFS_CCI); i++, p = &cpu_dvfs[i])
#define FP(pos, clk) { \
        .pos_div = pos, \
        .clk_div = clk, \
}
#define CPU_PLL_NAME "cpu-pll"
#define CCI_PLL_NAME "cci-pll"
#define CPU_PLLDIV_NAME "cpu-plldiv"
#define CCI_PLLDIV_NAME "cci-pll"
struct mt_cpu_freq_info {
	/*const*/ unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;
};
struct mt_cpu_dvfs {
	const char *name;
	const enum mt_cpu_dvfs_id id;
	unsigned int *armpll_addr;
	unsigned int *ckdiv_addr;
	struct mt_cpu_freq_method *freq_tbl;
};
struct cpufreq_frequency_table *cci_freq_table;
struct clk *cci_clk;
struct mtk_cpu_dvfs_info {
	struct cpumask cpus;
	struct device *cpu_dev;
	struct regulator *proc_reg;
	struct clk *cpu_clk;
	struct list_head list_head;
};
static LIST_HEAD(dvfs_info_list);
static struct mt_cpu_dvfs cpu_dvfs[] = {
	[MT_CPU_DVFS_LL] = {
		.name = __stringify(MT_CPU_DVFS_LL),
		.id = MT_CPU_DVFS_LL,
	},
	[MT_CPU_DVFS_CCI] = {
		.name = __stringify(MT_CPU_DVFS_CCI),
		.id = MT_CPU_DVFS_CCI,
	},
};
static unsigned long apmixed_base;
static unsigned long mcucfg_base;
static struct mt_cpu_dvfs *id_to_cpu_dvfs(enum mt_cpu_dvfs_id id)
{
	return (id < NR_MT_CPU_DVFS) ? &cpu_dvfs[id] : NULL;
}
struct mt_cpu_freq_method {
	const unsigned int pos_div;
	const unsigned int clk_div;
};
struct opp_idx_tbl {
	struct mt_cpu_dvfs *p;
	struct mt_cpu_freq_method *slot;
};
static struct opp_idx_tbl opp_tbl_m[NR_OPP_IDX];
static struct mt_cpu_freq_method opp_tbl_method_LL_FY[] = {
	FP(4,   1),
	FP(4,   1),
	FP(4,   1),
	FP(2,   1),
	FP(2,   1),
};
static struct mt_cpu_freq_method opp_tbl_method_CCI_FY[] = {
	FP(4,   1),
	FP(4,   1),
	FP(4,   1),
	FP(4,   1),
	FP(2,   1),
};
struct opp_tbl_info {
	struct mt_cpu_freq_info *const opp_tbl;
	const int size;
};
struct opp_tbl_m_info {
	struct mt_cpu_freq_method *const opp_tbl_m;
};
static struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
	},
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_FY },
	},
};
static unsigned int _cpu_dds_calc(unsigned int khz)
{
	unsigned int dds;
	dds = ((khz / 1000) << 14) / 26;
	return dds;
}
#if !FHCTL
static void adjust_armpll_dds(struct mt_cpu_dvfs *p, unsigned int vco, unsigned int pos_div)
{
	unsigned int dds;
	unsigned int val;
	dds = _GET_BITS_VAL_(21:0, vco);
	val = cpufreq_read(p->armpll_addr) & ~(_BITMASK_(21:0));
	val |= dds;
	cpufreq_write(p->armpll_addr, val | _BIT_(31) /* CHG */);
	udelay(PLL_SETTLE_TIME);
}
#endif
static void adjust_posdiv(struct mt_cpu_dvfs *p, unsigned int pos_div)
{
	unsigned int sel;
	sel = (pos_div == 1 ? 0 :
	       pos_div == 2 ? 1 :
	       pos_div == 4 ? 2 : 0);
	cpufreq_write_mask(p->armpll_addr, 26:24, sel);
	udelay(POS_SETTLE_TIME);
}
static void adjust_clkdiv(struct mt_cpu_dvfs *p, unsigned int clk_div)
{
	unsigned int sel;
	sel = (clk_div == 1 ? 8 :
	       clk_div == 2 ? 10 :
	       clk_div == 4 ? 11 : 8);
	cpufreq_write_mask(p->ckdiv_addr, 21:17, sel);
}
static void set_cur_freq(struct mt_cpu_dvfs *p, unsigned int target_khz, int idx)
{
	unsigned int sel, cur_posdiv, cur_clkdiv, dds;
	sel = _GET_BITS_VAL_(26:24, cpufreq_read(p->armpll_addr));
	cur_posdiv = (sel == 0 ? 1 :
		      sel == 1 ? 2 :
		      sel == 2 ? 4 : 1);
	sel = _GET_BITS_VAL_(21:17, cpufreq_read(p->ckdiv_addr));
	cur_clkdiv = (sel == 8 ? 1 :
		      sel == 10 ? 2 :
		      sel == 11 ? 4 : 1);
	opp_tbl_m[TARGET_OPP_IDX].p = p;
	opp_tbl_m[TARGET_OPP_IDX].slot = &p->freq_tbl[idx];
	/* post_div 1 -> 2 */
	if (cur_posdiv < opp_tbl_m[TARGET_OPP_IDX].slot->pos_div) {
		adjust_posdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->pos_div);

    }
	/* armpll_div 1 -> 2 */
	if (cur_clkdiv < opp_tbl_m[TARGET_OPP_IDX].slot->clk_div) {
		adjust_clkdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->clk_div);

    }
	dds = _cpu_dds_calc(target_khz *
	      opp_tbl_m[TARGET_OPP_IDX].slot->pos_div * opp_tbl_m[TARGET_OPP_IDX].slot->clk_div);
#if !FHCTL
	adjust_armpll_dds(p, dds, opp_tbl_m[TARGET_OPP_IDX].slot->pos_div);
#else
	if (cpu_dvfs_is(p, MT_CPU_DVFS_CCI))
		mtk_fh_set_rate(CLK_TOP_CCIPLL_CK_VRPOC_CCI, dds,-1);
	else if (cpu_dvfs_is(p, MT_CPU_DVFS_LL))
		mtk_fh_set_rate(CLK_TOP_ARMPLL_LL_CK_VRPOC, dds,-1);
#endif
	/* armpll_div 2 -> 1 */
	if (cur_clkdiv > opp_tbl_m[TARGET_OPP_IDX].slot->clk_div)
		adjust_clkdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->clk_div);
	/* post_div 2 -> 1 */
	if (cur_posdiv > opp_tbl_m[TARGET_OPP_IDX].slot->pos_div)
		adjust_posdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->pos_div);
}
static struct mtk_cpu_dvfs_info *mtk_cpu_dvfs_info_lookup(int cpu)
{
	struct mtk_cpu_dvfs_info *info;
	list_for_each_entry(info, &dvfs_info_list, list_head) {
		if (cpumask_test_cpu(cpu, &info->cpus))
			return info;
	}
	return NULL;
}
static int mtk_cpufreq_set_voltage(struct mtk_cpu_dvfs_info *info, int vproc)
{
	return regulator_set_voltage(info->proc_reg, vproc, INT_MAX);
}
#if 0
static unsigned int pll_to_clk(unsigned int pll_f, unsigned int ckdiv1)
{
	unsigned int freq = pll_f;
	switch (ckdiv1) {
	case 8:
		break;
	case 9:
	
	freq = freq * 3 / 4;
		break;
	case 10:
		freq = freq * 2 / 4;
		break;
	case 11:
		freq = freq * 1 / 4;
		break;
	case 16:
		break;
	case 17:
		freq = freq * 4 / 5;
		break;
	case 18:
		freq = freq * 3 / 5;
		break;
	case 19:
		freq = freq * 2 / 5;
		break;
	case 20:
		freq = freq * 1 / 5;
		break;
	case 24:
		break;
	case 25:
		freq = freq * 5 / 6;
		break;
	case 26:
		freq = freq * 4 / 6;
		break;
	case 27:
		freq = freq * 3 / 6;
		break;
	case 28:
		freq = freq * 2 / 6;
		break;
	case 29:
		freq = freq * 1 / 6;
		break;
	default:
		break;
	}
	return freq;
}
static unsigned int _cpu_freq_calc(unsigned int con1, unsigned int ckdiv1)
{
	unsigned int freq;
	unsigned int posdiv;
	posdiv = _GET_BITS_VAL_(26:24, con1);
	con1 &= _BITMASK_(21:0);
	freq = ((con1 * 26) >> 14) * 1000;
	switch (posdiv) {
	case 0:
		break;
	case 1:
		freq = freq / 2;
		break;
	case 2:
		freq = freq / 4;
		break;
	case 3:
		freq = freq / 8;
		break;
	default:
		freq = freq / 16;
		break;
	};
	return pll_to_clk(freq, ckdiv1);
}
static unsigned int get_cur_phy_freq(struct mt_cpu_dvfs *p)
{
	unsigned int con1;
	unsigned int ckdiv1;
	unsigned int cur_khz;
	con1 = cpufreq_read(p->armpll_addr);
	ckdiv1 = cpufreq_read(p->ckdiv_addr);
	ckdiv1 = _GET_BITS_VAL_(21:17, ckdiv1);
	cur_khz = _cpu_freq_calc(con1, ckdiv1);
	return cur_khz;
}
#endif
static int mtk_cpufreq_set_target(struct cpufreq_policy *policy,
				  unsigned int index)
{
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	struct mtk_cpu_dvfs_info *info = policy->driver_data;
	struct device *cpu_dev = info->cpu_dev;
	struct dev_pm_opp *opp, *old_opp;
	long freq_hz, old_freq_hz, freq_hz_cci;
	int vproc, old_vproc, target_vproc, ret;
	freq_hz_cci = cci_freq_table[index].frequency * 1000;
	freq_hz = freq_table[index].frequency * 1000;
	old_freq_hz = policy->cur * 1000;
	old_opp = dev_pm_opp_find_freq_ceil(cpu_dev, &old_freq_hz);
	if (IS_ERR(old_opp))
		return PTR_ERR(old_opp);
	old_vproc = dev_pm_opp_get_voltage(old_opp);
	dev_pm_opp_put(old_opp);
	opp = dev_pm_opp_find_freq_ceil(cpu_dev, &freq_hz);

	if (IS_ERR(opp))
		return PTR_ERR(opp);
	vproc = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);
	target_vproc = vproc;
	if (old_vproc < target_vproc) {
		ret = mtk_cpufreq_set_voltage(info, target_vproc);
		if (ret) {
			mtk_cpufreq_set_voltage(info, old_vproc);
			return ret;
		}
	}
	set_cur_freq(id_to_cpu_dvfs(MT_CPU_DVFS_CCI), freq_hz_cci/1000, index);
	set_cur_freq(id_to_cpu_dvfs(MT_CPU_DVFS_LL), freq_hz/1000, index);
	if (vproc < old_vproc) {
		ret = mtk_cpufreq_set_voltage(info, vproc);
		if (ret)
			return ret;
	}
	return 0;
}

static int mtk_cpu_dvfs_info_init(struct mtk_cpu_dvfs_info *info, int cpu)
{
	struct device *cpu_dev;
	struct regulator *proc_reg = ERR_PTR(-ENODEV);
	struct clk *cpu_clk = ERR_PTR(-ENODEV);
	struct platform_device *npdev = NULL;
	struct device_node *node;
	int ret;
	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev)
		return -ENODEV;
	cpu_clk = clk_get(cpu_dev, "cpu");
	if (IS_ERR(cpu_clk))
		return PTR_ERR(cpu_clk);
	proc_reg = regulator_get_optional(cpu_dev, "proc");
	if (IS_ERR(proc_reg)) {
		ret = PTR_ERR(proc_reg);
		goto out_free_resources;
	}
	node = of_find_node_by_name(NULL, "cci");
	if (!node) {
		ret = -ENODEV;
		goto out_free_resources;
	}
	npdev = of_device_alloc(node, NULL, NULL);
	if (!npdev) {
		ret = -ENODEV;
		goto out_free_resources;
	}
	if (of_device_is_compatible(node, "mediatek,mt6880-cci"))
		npdev->dev.of_node = node;
	else {
		ret = -ENODEV;
		goto out_free_resources;
	}
	ret = dev_pm_opp_of_add_table(&npdev->dev);
	if (ret) {
		ret = -ENODEV;
		goto out_free_resources;
	}
        ret = dev_pm_opp_init_cpufreq_table(&npdev->dev, &cci_freq_table);
	if (ret) {
		ret = -ENODEV;
		goto out_free_resources;
	}
	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, &info->cpus);
	if (ret) {
		ret = -ENODEV;
		goto out_free_cpufreq_table;
	}
	ret = dev_pm_opp_of_cpumask_add_table(&info->cpus);
	if (ret) {
		ret = -ENODEV;
		goto out_free_cpufreq_table;
	}
	info->cpu_dev = cpu_dev;
	info->proc_reg = proc_reg;
	info->cpu_clk = cpu_clk;
	if (npdev) {
		of_platform_device_destroy(&npdev->dev, NULL);
		of_dev_put(npdev);
	}
	return 0;
out_free_cpufreq_table:
	dev_pm_opp_free_cpufreq_table(cpu_dev, &cci_freq_table);
out_free_resources:
	if (npdev) {
		of_platform_device_destroy(&npdev->dev, NULL);
		of_dev_put(npdev);
	}
	if (!IS_ERR(proc_reg))
		regulator_put(proc_reg);
	if (!IS_ERR(cpu_clk))
		clk_put(cpu_clk);
	return ret;
}
static void mtk_cpu_dvfs_info_release(struct mtk_cpu_dvfs_info *info)
{
	if (!IS_ERR(info->proc_reg))
		regulator_put(info->proc_reg);
	if (!IS_ERR(info->cpu_clk))
		clk_put(info->cpu_clk);
	dev_pm_opp_of_cpumask_remove_table(&info->cpus);
}
static int mtk_cpufreq_init(struct cpufreq_policy *policy)
{
	struct mtk_cpu_dvfs_info *info;
	struct cpufreq_frequency_table *freq_table;
	//struct em_data_callback em_cb = EM_DATA_CB(of_dev_pm_opp_get_cpu_power);
	//struct em_data_callback em_cb = EM_DATA_CB(_get_cpu_power);
	int ret, count = 0;
	info = mtk_cpu_dvfs_info_lookup(policy->cpu);
	if (!info)
		return -EINVAL;
	count = dev_pm_opp_get_opp_count(info->cpu_dev);
	if (count <= 0)
		return -EINVAL;
	ret = dev_pm_opp_init_cpufreq_table(info->cpu_dev, &freq_table);
        if (ret)
                return -EINVAL;
	cpumask_copy(policy->cpus, &info->cpus);
	//em_register_perf_domain(policy->cpus, count, &em_cb);
    dev_pm_opp_of_register_em(policy->cpus);
	policy->freq_table = freq_table;
	policy->driver_data = info;
	policy->clk = info->cpu_clk;
	return 0;
}

static int mtk_cpufreq_exit(struct cpufreq_policy *policy)
{
	struct mtk_cpu_dvfs_info *info = policy->driver_data;
	dev_pm_opp_free_cpufreq_table(info->cpu_dev, &policy->freq_table);
	return 0;
}
static struct cpufreq_driver mtk_cpufreq_driver = {
	.flags = CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK |
		 CPUFREQ_HAVE_GOVERNOR_PER_POLICY | CPUFREQ_IS_COOLING_DEV,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = mtk_cpufreq_set_target,
	.get = cpufreq_generic_get,
	.init = mtk_cpufreq_init,
	.exit = mtk_cpufreq_exit,
	.name = "mtk-cpufreq",
	.attr = cpufreq_generic_attr,
};
static int mtk_cpufreq_probe(struct platform_device *pdev)
{
	struct mtk_cpu_dvfs_info *info, *tmp;
	struct mt_cpu_dvfs *p;
	int cpu, ret, j;
	struct opp_tbl_m_info *opp_tbl_info;
	struct device_node *node;
	unsigned int pll_shift, plldiv_shift;
	node = of_find_compatible_node(NULL, NULL, APMIXED_NODE);
	if (!node)
		return -ENODEV;
	apmixed_base = (unsigned long)(of_iomap(node, 0));
	if (!apmixed_base)
		return -ENODEV;
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node)
		return -ENODEV;
	mcucfg_base = (unsigned long)(of_iomap(node, 0));
	if (!mcucfg_base)
		return -ENODEV;
	node = of_find_node_by_name(NULL, "cpufreq");
	if (!node) {
		pr_notice("failed to find node @ %s\n", __func__);
		return -ENODEV;
	}
	for_each_cpu_dvfs(j, p) {
		if (cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
			of_property_read_u32_index(node,
					CPU_PLL_NAME, 0, &pll_shift);
			of_property_read_u32_index(node,
					CPU_PLLDIV_NAME, 0, &plldiv_shift);
		} else {	/* CCI */
			of_property_read_u32_index(node,
					CCI_PLL_NAME, 0, &pll_shift);
			of_property_read_u32_index(node,
					CCI_PLLDIV_NAME, 0, &plldiv_shift);
		}
		p->armpll_addr = (unsigned int *)(apmixed_base + pll_shift);
		p->ckdiv_addr = (unsigned int *)(mcucfg_base + plldiv_shift);
		opp_tbl_info = &opp_tbls_m[j][0];
		p->freq_tbl = opp_tbl_info->opp_tbl_m;
	}
	for_each_possible_cpu(cpu) {
		info = mtk_cpu_dvfs_info_lookup(cpu);
		if (info)
			continue;
		info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
		if (!info) {
			ret = -ENOMEM;
			goto release_dvfs_info_list;
		}
		ret = mtk_cpu_dvfs_info_init(info, cpu);
		if (ret)
			goto release_dvfs_info_list;
		list_add(&info->list_head, &dvfs_info_list);
	}
	ret = cpufreq_register_driver(&mtk_cpufreq_driver);
	if (ret)
		goto release_dvfs_info_list;
	return 0;
release_dvfs_info_list:
	list_for_each_entry_safe(info, tmp, &dvfs_info_list, list_head) {
		mtk_cpu_dvfs_info_release(info);
		list_del(&info->list_head);
	}
	return ret;
}
static const struct of_device_id mtk_cpufreq_machines[] __initconst = {
	{ .compatible = "mediatek-cpufreq-kernel", },
	{ }
};
static struct platform_driver mtk_cpufreq_platdrv = {
	.driver = {
		.name	= "mtk-cpufreq",
		.of_match_table = mtk_cpufreq_machines,
	},
	.probe		= mtk_cpufreq_probe,
};
module_platform_driver(mtk_cpufreq_platdrv);
MODULE_DESCRIPTION("MediaTek CPUFreq driver");
MODULE_AUTHOR("Wei-Chia Su <wei-chia.su@mediatek.com>");
MODULE_LICENSE("GPL v2");
