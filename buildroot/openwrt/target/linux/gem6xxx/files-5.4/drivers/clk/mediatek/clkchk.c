// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2020 MediaTek Inc.
// Author: Owen Chen <owen.chen@mediatek.com>

#define pr_fmt(fmt) "[clkchk] " fmt

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>

#include "clkchk.h"

#if defined(CONFIG_PM_DEBUG)
#define CLKCHK_PM_DOMAIN	1
#else
#define CLKCHK_PM_DOMAIN	0
#endif

#define MAX_CLK_NUM	1024
#define MAX_PD_NUM	64
#define PLL_LEN		20
#define INV_MSK		0xFFFFFFFF

void __attribute__((weak)) clkchk_set_cfg(void)
{
}

static const struct clkchk_ops *clkchk_ops;

void set_clkchk_ops(const struct clkchk_ops *ops)
{
	clkchk_ops = ops;
}

const struct regname *get_all_regnames(void)
{
	if (clkchk_ops == NULL || clkchk_ops->get_all_regnames == NULL)
		return NULL;

	return clkchk_ops->get_all_regnames();
}
EXPORT_SYMBOL(get_all_regnames);

static const char *get_provider_name(struct device_node *node, u32 *cells)
{
	const char *name;
	const char *p;
	u32 cc;

	if (of_property_read_u32(node, "#clock-cells", &cc) != 0)
		cc = 0;

	if (cells != NULL)
		*cells = cc;

	if (cc == 0U) {
		if (of_property_read_string(node,
				"clock-output-names", &name) < 0)
			name = node->name;

		return name;
	}

	if (of_property_read_string(node, "compatible", &name) < 0)
		name = node->name;

	p = strchr(name, (int)'-');

	if (p != NULL)
		return p + 1;
	else
		return name;
}

static void setup_provider_clk(struct provider_clk *pvdck)
{
	const char *pvdname = pvdck->provider_name;
	struct pvd_msk *pm;

	if (!pvdname)
		return;

	if (clkchk_ops == NULL || clkchk_ops->get_pvd_pwr_mask == NULL)
		return;

	pm = clkchk_ops->get_pvd_pwr_mask();

	for (; pm->pvdname != NULL; pm++) {
		if (!strcmp(pvdname, pm->pvdname)) {
			pvdck->pwr_mask = pm->pwr_mask;
			pvdck->sta_type = pm->sta_type;
			return;
		}
	}
	pvdck->pwr_mask = INV_MSK;
}

struct provider_clk *get_all_provider_clks(void)
{
	static struct provider_clk provider_clks[MAX_CLK_NUM];
	struct device_node *node = NULL;
	unsigned int n = 0;

	if (provider_clks[0].ck != NULL)
		return provider_clks;

	do {
		const char *node_name;
		u32 cells;

		node = of_find_node_with_property(node, "#clock-cells");

		if (node == NULL)
			break;

		node_name = get_provider_name(node, &cells);

		if (cells != 0U)  {
			unsigned int i;

			for (i = 0; i < 1000; i++) {
				struct of_phandle_args pa;
				struct clk *ck;

				pa.np = node;
				pa.args[0] = i;
				pa.args_count = 1;
				ck = of_clk_get_from_provider(&pa);

				if (PTR_ERR(ck) == -EINVAL)
					break;
				else if (IS_ERR_OR_NULL(ck))
					continue;

				provider_clks[n].ck = ck;
				provider_clks[n].ck_name = __clk_get_name(ck);
				provider_clks[n].idx = i;
				provider_clks[n].provider_name = node_name;
				setup_provider_clk(&provider_clks[n]);

				++n;
			}
		}
	} while (node != NULL && n < MAX_CLK_NUM);

	return provider_clks;
}
EXPORT_SYMBOL(get_all_provider_clks);

static struct clk *__clk_chk_lookup(const char *name)
{
	struct provider_clk *pvdck = get_all_provider_clks();

	for (; pvdck->ck != NULL; pvdck++) {
		if (!strcmp(pvdck->ck_name, name))
			return pvdck->ck;
	}

	return NULL;
}

const char *ccf_state(struct clk_hw *hw)
{
	if (clk_hw_is_enabled(hw))
		return "enabled";

	if (clk_hw_is_prepared(hw))
		return "prepared";

	return "disabled";
}

static bool clk_chk_pwr_is_on(int *pwr_status,
		struct provider_clk *pvdck)
{
	if (!pvdck || !pvdck->pwr_mask)
		return false;

	if (pvdck->sta_type == PWR_STA) {
		if ((pwr_status[PWR_STA] & pvdck->pwr_mask) != pvdck->pwr_mask ||
				(pwr_status[PWR_STA2] & pvdck->pwr_mask) != pvdck->pwr_mask)
			return false;
		else if ((pwr_status[PWR_STA] & pvdck->pwr_mask) == pvdck->pwr_mask &&
				(pwr_status[PWR_STA2] & pvdck->pwr_mask) == pvdck->pwr_mask)
			return true;
	} else if (pvdck->sta_type == OTHER_STA) {
		if ((pwr_status[OTHER_STA] & pvdck->pwr_mask) == pvdck->pwr_mask)
			return true;
	} else if (pvdck->sta_type == XPU_PWR_STA) {
		if ((pwr_status[XPU_PWR_STA] & pvdck->pwr_mask) == pvdck->pwr_mask)
			return true;
	}

	return false;
}

static bool clkchk_pvdck_is_on(struct provider_clk *pvdck)
{
	int *pwr_stat;

	if (!clkchk_ops || !clkchk_ops->get_pwr_stat)
		return false;

	pwr_stat = clkchk_ops->get_pwr_stat();
	if (IS_ERR_OR_NULL(pwr_stat)) {
		pr_info("pwr_stat is error or null\n");
		return false;
	}

	if (pvdck->pwr_mask == INV_MSK)
		return false;

	if (!pvdck->pwr_mask)
		return true;

	return clk_chk_pwr_is_on(pwr_stat, pvdck);

}

static void dump_enabled_clks(struct clk_hw *c_hw)
{
	const char * const *pll_names;
	const char *p_name, *temp;
	struct clk_hw *p_hw;

	if (IS_ERR_OR_NULL(c_hw))
		return;

	if (!clk_hw_is_prepared(c_hw) && !clk_hw_is_enabled(c_hw))
		return;

	if (clkchk_ops == NULL || clkchk_ops->get_off_pll_names == NULL)
		return;

	pll_names = clkchk_ops->get_off_pll_names();
	p_hw = clk_hw_get_parent(c_hw);
	if (IS_ERR_OR_NULL(p_hw))
		return;

	p_name = clk_hw_get_name(c_hw);
	if (IS_ERR_OR_NULL(p_name))
		return;

	while (1) {
		if (IS_ERR_OR_NULL(p_hw))
			return;
		temp = p_hw != NULL ? clk_hw_get_name(p_hw) : "skip";
		if (IS_ERR_OR_NULL(temp) || strcmp(temp, "skip")==0)
			return;
		else if (strcmp(temp, "clk26m")==0)
			break;
		p_name = p_hw != NULL ? clk_hw_get_name(p_hw) : NULL;
		if (IS_ERR_OR_NULL(p_name))
			return;
		p_hw = p_hw != NULL ? clk_hw_get_parent(p_hw) : NULL;
		if (IS_ERR_OR_NULL(p_hw))
			return;
	}

	for (; *pll_names != NULL && p_name != NULL; pll_names++) {
		if (!strncmp(p_name, *pll_names, PLL_LEN)) {
			p_hw = clk_hw_get_parent(c_hw);
			if (IS_ERR_OR_NULL(p_hw))
				continue;
			pr_notice("[%-21s: %8s, %3d, %3d, %10ld, %21s]\n",
					clk_hw_get_name(c_hw),
					ccf_state(c_hw),
					clk_hw_is_prepared(c_hw),
					clk_hw_is_enabled(c_hw),
					clk_hw_get_rate(c_hw),
					p_hw ? clk_hw_get_name(p_hw) : "None");
			break;
		}
	}
}

void subsys_if_on(void)
{
    static const char * const pwr_names[] = {
		[0] = "MD1",
		[1] = "CONN",
		[3] = "IFR",
		[4] = "PERI",
		[5] = "AUDIO",
		[6] = "DIS0",
		[8] = "EIP97",
		[9] = "PEXTP_R_2LX1_MAC",
		[10] = "PEXTP_R_2LX2_MAC",
		[11] = "PEXTP_D_2LX1_MAC",
		[12] = "PEXTP_D_2LX1_PHY_AO",
		[13] = "PEXTP_D_2LX1_PHY_PD",
		[14] = "PEXTP_R_2LX1_PHY",
		[15] = "PEXTP_R_2LX2_PHY",
		[16] = "MSDC",
		[17] = "ETH",
		[18] = "SSUSB_TOP",
		[19] = "SSUSB_PHY",
		[20] = "MEDSYS",
		[21] = "NETSYS",
		[22] = "HSM_TOP",
		[23] = "XFI_PHY_0",
		[24] = "XFI_PHY_1",
		[25] = "XFI_TOP_0",
		[26] = "XFI_TOP_1",
		[27] = "SGMII_TOP_0",
		[28] = "SGMII_TOP_1",
	};
	static const char * const pwr_names_2[] = {
		[0] = "CPUEB",
		[1] = "MFG",
	};
    u32 val = 0,i;
    static void __iomem *scpsys_base, *pwr_sta, *pwr_sta_2nd, *xpu_pwr_sta;
    scpsys_base = ioremap(0x12001000, PAGE_SIZE);
    pwr_sta = scpsys_base + 0xF38;
    pwr_sta_2nd = scpsys_base + 0xF3C;
    xpu_pwr_sta = scpsys_base + 0xF40;
    pr_notice("MTCMOS status register: pwr_sta %x & pwr_sta %x & xpu_pwr_sta %x!!\n",readl(pwr_sta),readl(pwr_sta_2nd),readl(xpu_pwr_sta));
    val = readl(pwr_sta);
    for (i = 0; i < 29; i++) {
        if((val & BIT(i)) != 0U)
            pr_notice("suspend warning: %s is on!!\n",pwr_names[i]);
    }
	val = readl(xpu_pwr_sta);
	for (i = 0; i < 2; i++) {
        if((val & BIT(i)) != 0U)
            pr_notice("suspend warning: %s is on!!\n",pwr_names_2[i]);
    }
}

static bool check_pll_off(void)
{
	const char * const *name;
	int invalid = 0;
pr_info("check pll off start\n");
	if (clkchk_ops == NULL || clkchk_ops->get_off_pll_names == NULL)
		return false;

	name = clkchk_ops->get_off_pll_names();

	for (; *name != NULL; name++) {
		struct clk *c = __clk_chk_lookup(*name);
		struct clk_hw *c_hw = __clk_get_hw(c);

		if (IS_ERR_OR_NULL(c_hw)){
                  	pr_info("%s is error or null", clk_hw_get_name(c_hw));
			continue;
                }

		if (!clk_hw_is_enabled(c_hw)){
                  	pr_info("%s is disable", clk_hw_get_name(c_hw));
			continue;
                }

		pr_notice("suspend warning : %s is on\n",
				clk_hw_get_name(c_hw));

		invalid++;
	}

	if (clkchk_ops == NULL || clkchk_ops->get_notice_pll_names == NULL)
		return false;

	name = clkchk_ops->get_notice_pll_names();

	for (; *name != NULL; name++) {
		struct clk *c = __clk_chk_lookup(*name);
		struct clk_hw *c_hw = __clk_get_hw(c);

		if (!c_hw)
			continue;

		if (!clk_hw_is_enabled(c_hw))
			continue;

		pr_notice("suspend warning[0m: %s is on\n",
				clk_hw_get_name(c_hw));

		invalid++;
	}

	if (invalid)
		return true;

	return false;
}

static bool is_pll_chk_bug_on(void)
{
	if (clkchk_ops == NULL || clkchk_ops->is_pll_chk_bug_on == NULL)
		return false;

#ifdef CONFIG_MTK_ENG_BUILD
	return clkchk_ops->is_pll_chk_bug_on();
#endif
	return false;
}

/*
 * clkchk vf table checking
 */

static int get_vcore_opp(void)
{
	if (clkchk_ops == NULL || clkchk_ops->get_vcore_opp == NULL)
		return VCORE_NULL;

	return clkchk_ops->get_vcore_opp();
}

static void warn_vcore(int opp, const char *clk_name, int rate, int id)
{
	int vf_opp;

	if (!clkchk_ops || !clkchk_ops->get_vf_opp)
		return;

	vf_opp = clkchk_ops->get_vf_opp(id, opp);
	if ((opp >= 0) && (id >= 0) && (vf_opp > 0) &&
			((rate/1000) > vf_opp) && ((rate/1000) != 26000)) {
		pr_notice("%s Choose %d FAIL!!!![MAX(%d/%d): %d]\r\n",
				clk_name, rate/1000, id, opp,
				vf_opp);

		//BUG_ON(1);
	}
}

static int mtk_mux2id(const char **mux_name)
{
	int i = 0;

	if (!clkchk_ops || !clkchk_ops->get_vf_name
			|| !clkchk_ops->get_vf_num)
		return -EINVAL;

	for (i = 0; clkchk_ops->get_vf_num(); i++) {
		if (strcmp(*mux_name, clkchk_ops->get_vf_name(i)) == 0)
			return i;
	}

	return -EINVAL;
}

/* The clocks have a mechanism for synchronizing rate changes. */
static int mtk_clk_rate_change(struct notifier_block *nb,
					  unsigned long flags, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct clk_hw *hw = __clk_get_hw(ndata->clk);
	const char *clk_name = __clk_get_name(hw->clk);
	int vcore_opp = get_vcore_opp();

	if (vcore_opp == VCORE_NULL)
		return -EINVAL;

	if (flags == PRE_RATE_CHANGE && clk_name) {
		warn_vcore(vcore_opp, clk_name,
			ndata->new_rate, mtk_mux2id(&clk_name));
	}

	return NOTIFY_OK;
}

static struct notifier_block mtk_clk_notifier = {
	.notifier_call = mtk_clk_rate_change,
};

static void mtk_clk_check_muxes(void)
{
	struct clk *clk;
	int i;

	if (!clkchk_ops || !clkchk_ops->get_vf_name
			|| !clkchk_ops->get_vf_num)
		return;

	/*tmp solution to wait vcore dvfs porting*/
	if(!clkchk_ops->get_vcore_opp)
		return;
	if(clkchk_ops->get_vcore_opp() == VCORE_NULL)
		return;
	/*tmp solution*/

	for (i = 0; i < clkchk_ops->get_vf_num(); i++) {
		const char *name = clkchk_ops->get_vf_name(i);

		if (!name)
			continue;

		clk = __clk_chk_lookup(name);
		clk_notifier_register(clk, &mtk_clk_notifier);
	}
}

static int clk_chk_probe(struct platform_device *pdev)
{
	clkchk_set_cfg();
	/*remove mux notifier, no use in mt69XX*/
	//mtk_clk_check_muxes();

	return 0;
}

static int clk_chk_dev_pm_suspend(struct device *dev)
{
	struct provider_clk *pvdck = get_all_provider_clks();
	int *pwr_stat;
pr_info("clkchk suspend_noirq enter\n");
	if (!clkchk_ops || !clkchk_ops->get_pwr_stat)
		return 0;

	pwr_stat = clkchk_ops->get_pwr_stat();
	if (IS_ERR_OR_NULL(pwr_stat)){
          	pr_info("pwr_stat is error or null\n");
		//return 0;
        }

	if (check_pll_off()) {
		for (; pvdck->ck != NULL; pvdck++) {
			struct clk_hw *c_hw = __clk_get_hw(pvdck->ck);

			//if (clk_chk_pwr_is_on(pwr_stat, pvdck))
			dump_enabled_clks(c_hw);
		}

		if (is_pll_chk_bug_on())
			BUG_ON(1);

	}

	//subsys_if_on();

	return 0;
}


static void suspend_print(void)
{
	struct provider_clk *pvdck = get_all_provider_clks();

	if (check_pll_off()) {
		for (; pvdck->ck != NULL; pvdck++) {
			struct clk_hw *c_hw = __clk_get_hw(pvdck->ck);
			if (clkchk_pvdck_is_on(pvdck))
				dump_enabled_clks(c_hw);
		}

		if (is_pll_chk_bug_on())
			BUG_ON(1);

		//WARN_ON(1);
	}

	subsys_if_on();
}

static const struct dev_pm_ops clk_chk_dev_pm_ops = {
	.suspend_noirq = clk_chk_dev_pm_suspend,
	.resume_noirq = NULL,
};

static struct platform_driver clk_chk_drv = {
	.probe = clk_chk_probe,
	.driver = {
		.name = "clk-chk",
		.owner = THIS_MODULE,
		//.pm = &clk_chk_dev_pm_ops,
	},
};

static int clkchk_syscore_suspend(void)
{
    pr_notice("Enter CLOCK suspend Call Back !!\n");
	suspend_print();
	return 0;
}
static void clkchk_syscore_resume(void)
{
    pr_notice("Enter CLOCK resume Call Back !!\n");
}
static struct syscore_ops clkchk_syscore_ops = {
	.suspend = clkchk_syscore_suspend,
	.resume = clkchk_syscore_resume,
};

/*
 * init functions
 */

static int __init clkchk_init(void)
{
	static struct platform_device *clk_chk_dev;

	clk_chk_dev = platform_device_register_simple("clk-chk", -1, NULL, 0);
	if (IS_ERR(clk_chk_dev))
		pr_warn("unable to register clk-chk device");

  	register_syscore_ops(&clkchk_syscore_ops);
	return platform_driver_register(&clk_chk_drv);
}

static void __exit clkchk_exit(void)
{
	platform_driver_unregister(&clk_chk_drv);
}

late_initcall_sync(clkchk_init);
module_exit(clkchk_exit);
MODULE_LICENSE("GPL");

