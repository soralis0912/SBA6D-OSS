// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: Gene Chen <gene_chen@richtek.com>
 */

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <linux/spmi.h>

#include <dt-bindings/mfd/mt6361.h>

#define MT6361_TOP_INFO1	0x00
#define MT6361_IRQ_IND		0x0B
#define MT6361_IRQ_IND_MASK	0x0C
#define MT6361_TOP_FLAG1	0xA0
#define MT6361_PSC_FLAG1	0xA6
#define MT6361_LVSF_FLAG1	0xB9
#define MT6361_TOP_MASK1	0xE0
#define MT6361_SPMI_RCS_STS0	0x108
#define MT6361_WDTRSTB_CTRL	0x20C
#define MT6361_IRQ_NUM_REGS	(MT6361_LVSF_FLAG1 - MT6361_TOP_FLAG1 + 1)

#define MT6361_MSK_VID		GENMASK(7, 4)
#define MT6361_MSK_CHIP_REV	GENMASK(3, 0)

#define FLAG_PMIC_RESETB_LP_MASK	BIT(6)
#define RSV_RESETB_LP_FLAG_MASK		BIT(0)

static bool dbg_log_en;
module_param(dbg_log_en, bool, 0644);
#define mt_dbg(dev, fmt, ...) \
	do { \
		if (dbg_log_en) \
			dev_info(dev, "%s " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

struct mt6361_data {
	struct spmi_device *sdev;
	struct device *dev;
	struct regmap *rmap;
	struct regmap_irq_chip_data *irq_data;
	u8 vid;
	u8 chip_rev;
	int irq;
};

static int mt6361_spmi_read(void *context, const void *reg, size_t reg_size,
			    void *val, size_t val_size)
{
	struct mt6361_data *ddata = context;
	u16 addr = cpu_to_be16(*(u16 *)reg);
	int i, ret;

	ret = spmi_ext_register_readl(ddata->sdev, addr, val, val_size);
	if (dbg_log_en) {
		for (i = 0; i < val_size; i++)
			mt_dbg(ddata->dev, "%s: addr:0x%x, data:0x%x, len:%d\n",
				 __func__, addr, *((u8 *)val + i), (int)val_size);
	}
	return ret;
}

static int mt6361_spmi_write(void *context, const void *data, size_t count)
{
	struct mt6361_data *ddata = context;
	u16 addr = cpu_to_be16(*(u16 *)data);
	u8 *val = (u8 *)data + 2;
	int i, ret = 0, val_size = count - 2;

	ret = spmi_ext_register_writel(ddata->sdev, addr, val, val_size);
	if (dbg_log_en) {
		for (i = 0; i < val_size; i++)
			mt_dbg(ddata->dev, "%s: addr:0x%x, val:0x%x, len:%d(%d)\n",
				 __func__, addr + i, *(val + i), val_size, ret);
	}
	return ret;
}

static struct regmap_bus mt6361_regmap_bus = {
	.read = mt6361_spmi_read,
	.write = mt6361_spmi_write,
	.max_raw_read = 2,
	.max_raw_write = 2,
};

static const struct regmap_config spmi_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = 0xAFF,
};

#define MT6361_VID 0x70
static int mt6361_check_devid(struct mt6361_data *ddata)
{
	int ret;
	u32 val;
	u8 vid;

	ret = regmap_read(ddata->rmap, MT6361_TOP_INFO1, &val);
	if (ret < 0) {
		dev_err(ddata->dev, "failed to get devid(%d)\n", ret);
		return ret;
	}

	vid = val & MT6361_MSK_VID;
	if (vid != MT6361_VID) {
		dev_err(ddata->dev, "vid:0x%02x is incorrect\n", vid);
		return -ENODEV;
	}

	ddata->chip_rev = val & MT6361_MSK_CHIP_REV;
	dev_info(ddata->dev, "chip_rev = 0x%x\n", ddata->chip_rev);
	return 0;
}

static int mt6361_init_setting(struct mt6361_data *ddata)
{
	int ret;
	u32 val = 0;
	bool resetb_lp;

	/* Unmask all irq indicator */
	ret = regmap_write(ddata->rmap, MT6361_IRQ_IND_MASK, 0);
	if (ret < 0)
		return ret;

	/* store resetb lp event to rsv bit */
	pr_info("%s: ++\n", __func__);
	ret = regmap_read(ddata->rmap, MT6361_PSC_FLAG1, &val);
	if (ret < 0)
		return ret;
	resetb_lp = val & FLAG_PMIC_RESETB_LP_MASK ? true : false;
	dev_info(ddata->dev, "%s: resetb long press=%d\n", __func__, resetb_lp);
	return regmap_update_bits(ddata->rmap, MT6361_WDTRSTB_CTRL,
				 RSV_RESETB_LP_FLAG_MASK, resetb_lp ? 0xFF : 0);
}

static const struct regmap_irq spmi_regmap_irqs[] = {
	/* offset 00 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_HW_TRAP, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_RTC_INT, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VDET, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_WDTRSTB_IN, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PWRKEY, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PMIC_RESETB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_EXT_PMIC_PG, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_EN, 8),
	/* offset 01 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SRCLKEN_IN0, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SRCLKEN_IN1, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SRCLKEN_IN2, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SIM_CARD1_DET_N, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SIM_CARD2_DET_N, 8),
	/* offset 02 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LVPT, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VSYS_DDLO, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VSYS_UVLO, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VSYS_OVLO, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VBB_UVLO, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VBB_OVLO, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VIO18_PG, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_VDIG18_PG, 8),
	/* offset 03 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_THERMAL0, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_THERMAL1, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_THERMAL2, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_THERMAL3, 8),
	/* offset 04 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_TOP_WRKEYT_ERR, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SRCLKEN_IN0_GPIO_DI, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SRCLKEN_IN1_GPIO_DI, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SRCLKEN_IN2_GPIO_DI, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_CARD1_DET_N_GPIO_DI, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_CARD2_DET_N_GPIO_DI, 8),
	/* offset 06 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PSC_WRKEYT_ERR, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BWDT, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PWRKEY_LP, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PWRKEY_SP, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PMIC_RESETB_LP, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_PMIC_RESETB_SP, 8),
	/* offset 08 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO01_VCN15_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO02_VRFIO18_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO03_VIO18_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO04_VRF18_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO05_VRF18AIF_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO06_VA15_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO07_VIO075_PGB, 8),
	/* offset 09 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO08_VA12_1_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO09_VA12_2_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO10_VSRAM_PROC_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO11_VRF13_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO12_VRF13AIF_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO13_VRF12_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO14_VMDD2H_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO15_VMDD2L_PGB, 8),
	/* offset 0A */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO16_VRF09_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO17_VRF09AIF_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO18_VSRAM_DIGRF_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO19_VSRAM_MODEM_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO20_VSRAM_MDFE_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO21_VSRAM_CORE_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO22_VMDDR_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO23_VMDDQ_PGB, 8),
	/* offset 0B */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO24_VAUX18_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO25_VSIM1_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO26_VSIM2_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO27_VUSB_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO28_VMC_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO29_VEFUSE_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO30_VPROC15_PGB, 8),
	/* offset 0C */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO01_VCN15_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO02_VRFIO18_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO03_VIO18_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO04_VRF18_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO05_VRF18AIF_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO06_VA15_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO07_VIO075_OC, 8),
	/* offset 0D */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO08_VA12_1_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO09_VA12_2_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO10_VSRAM_PROC_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO11_VRF13_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO12_VRF13AIF_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO13_VRF12_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO14_VMDD2H_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO15_VMDD2L_OC, 8),
	/* offset 0E */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO16_VRF09_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO17_VRF09AIF_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO18_VSRAM_DIGRF_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO19_VSRAM_MODEM_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO20_VSRAM_MDFE_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO21_VSRAM_CORE_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO22_VMDDR_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO23_VMDDQ_OC, 8),
	/* offset 0F */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO24_VAUX18_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO25_VSIM1_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO26_VSIM2_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO27_VUSB_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO28_VMC_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO29_VEFUSE_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO30_VPROC15_OC, 8),
	/* offset 10 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_LDO_WRKEYT_ERR, 8),
	/* offset 12 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK1_VS1_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK2_VS2_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK3_VS3_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK4_VCORE_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK5_VDD2H_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK6_CPU_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK7_VDDQ_PGB, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK8_VRFDIG_PGB, 8),
	/* offset 13 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK1_VS1_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK2_VS2_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK3_VS3_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK4_VCORE_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK5_VDD2H_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK6_CPU_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK7_VDDQ_OC, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK8_VRFDIG_OC, 8),
	/* offset 14 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_BUCK_WRKEYT_ERR, 8),
	/* offset 16 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_ISINK1_SHORT, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_ISINK1_OPEN, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_ISINK2_SHORT, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_ISINK2_OPEN, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_ISINK3_SHORT, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_ISINK3_OPEN, 8),
	/* offset 17 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_AUXADC_THR_MIN, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_AUXADC_THR_MAX, 8),
	/* offset 18 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SPMI_RSC0, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_SPMI_RSC1, 8),
	/* offset 19 */
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_RGS_LVSF_OC_STATUS, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_RGS_LVSF_PGB_STATUS, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_AD_VCCB_PRD, 8),
	REGMAP_IRQ_REG_LINE(MT6361_FLAG_AD_VBBSNS_PRD, 8),
};

enum mt6361_irq_ind {
	MT6361_IRQ_IND_SPMI,
	MT6361_IRQ_IND_LVSF,
	MT6361_IRQ_IND_AUXADC,
	MT6361_IRQ_IND_RGB,
	MT6361_IRQ_IND_BUCK,
	MT6361_IRQ_IND_LDO,
	MT6361_IRQ_IND_PSC,
	MT6361_IRQ_IND_TOP,
	MT6361_IRQ_IND_MAX,
};

static unsigned int spmi_offsets[] = { 24 };
static unsigned int lvsf_offsets[] = { 25 };
static unsigned int auxadc_offsets[] = { 23 };
static unsigned int rgb_offsets[] = { 22 };
static unsigned int buck_offsets[] = { 18, 19, 20 };
static unsigned int ldo_offsets[] = { 8, 9 , 10, 11, 12, 13, 14, 15, 16 };
static unsigned int psc_offsets[] = { 6 };
static unsigned int top_offsets[] = { 0, 1, 2, 3, 4 };

static struct regmap_irq_sub_irq_map mt6361_sub_reg_offsets[] = {
	REGMAP_IRQ_MAIN_REG_OFFSET(spmi_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(lvsf_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(auxadc_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(rgb_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(buck_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(ldo_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(psc_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(top_offsets),
};

static int mt6361_handle_post_irq(void *irq_drv_data)
{
	struct mt6361_data *ddata = irq_drv_data;

	return regmap_write(ddata->rmap, MT6361_SPMI_RCS_STS0, 1);
}

static struct regmap_irq_chip spmi_regmap_irq_chip = {
	.name = "mt6361_spmi",
	.main_status = MT6361_IRQ_IND,
	.num_main_status_bits = MT6361_IRQ_IND_MAX,
	.sub_reg_offsets = mt6361_sub_reg_offsets,
	.num_main_regs = 1,

	.status_base = MT6361_TOP_FLAG1,
	.mask_base = MT6361_TOP_MASK1,
	.ack_base = MT6361_TOP_FLAG1,
	.init_ack_masked = true,

	.num_regs = MT6361_IRQ_NUM_REGS,
	.irqs = spmi_regmap_irqs,
	.num_irqs = ARRAY_SIZE(spmi_regmap_irqs),
	.handle_post_irq = mt6361_handle_post_irq,
};

static ssize_t irq_trigger_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{

	struct mt6361_data *ddata = dev_get_drvdata(dev);
	struct irq_domain *domain;
	int ret;
	unsigned long irqno;

	ret = kstrtoul(buf, 0, &irqno);
	if (ret < 0) {
		dev_warn(dev, "parsing number fail\n");
		return ret;
	}

	dev_info(dev, "irqno = %lu\n", irqno);

	domain = regmap_irq_get_domain(ddata->irq_data);
	if (!domain) {
		dev_notice(dev, "failed to get irq domain\n");
		return -EINVAL;
	}
	handle_nested_irq(irq_find_mapping(domain, irqno));
	return count;
}
static const DEVICE_ATTR_WO(irq_trigger);

static int mt6361_probe(struct spmi_device *sdev)
{
	struct mt6361_data *ddata;
	struct device_node *np = sdev->dev.of_node;
	int ret;

	ddata = devm_kzalloc(&sdev->dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	ddata->sdev = sdev;
	ddata->dev = &sdev->dev;
	spmi_device_set_drvdata(sdev, ddata);

	ddata->rmap = devm_regmap_init(&sdev->dev, &mt6361_regmap_bus, ddata,
				       &spmi_regmap_config);
	if (IS_ERR(ddata->rmap))
		return PTR_ERR(ddata->rmap);

	ret = mt6361_check_devid(ddata);
	if (ret < 0) {
		dev_err(ddata->dev, "failed to check device id\n");
		return ret;
	}

	ret = mt6361_init_setting(ddata);
	if (ret < 0) {
		dev_err(ddata->dev, "failed to set init setting\n");
		return ret;
	}

	ddata->irq = of_irq_get(np, 0);
	if (ddata->irq < 0) {
		dev_err(ddata->dev, "failed to get irq(%d)\n", ddata->irq);
		return ret;
	}

	spmi_regmap_irq_chip.irq_drv_data = ddata;
	ret = devm_regmap_add_irq_chip(ddata->dev, ddata->rmap, ddata->irq,
				       IRQF_ONESHOT, 0, &spmi_regmap_irq_chip,
				       &ddata->irq_data);
	if (ret < 0) {
		dev_err(ddata->dev, "failed to add irq chip\n");
		return ret;
	}

	ret = device_create_file(ddata->dev, &dev_attr_irq_trigger);
	if (ret < 0) {
		dev_err(ddata->dev, "failed to create debugfs\n");
		return ret;
	}

	dev_info(ddata->dev, "%s: successfully\n", __func__);
	return devm_of_platform_populate(ddata->dev);
}

static void mt6361_remove(struct spmi_device *sdev)
{
}

static int mt6361_suspend(struct device *dev)
{
	struct mt6361_data *ddata = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		enable_irq_wake(ddata->irq);

	dev_info(dev, "%s: done\n", __func__);
	return 0;
}

static int mt6361_resume(struct device *dev)
{
	struct mt6361_data *ddata = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(ddata->irq);

	return 0;
}

static const struct dev_pm_ops mt6361_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mt6361_suspend, mt6361_resume)
};

static const struct of_device_id mt6361_of_match[] = {
	{ .compatible = "mediatek,mt6361", },
	{ },
};
MODULE_DEVICE_TABLE(of, mt6361_of_match);

static struct spmi_driver mt6361_driver = {
	.driver = {
		.name = "mt6361",
		.of_match_table = of_match_ptr(mt6361_of_match),
		.pm = &mt6361_pm_ops,
	},
	.probe = mt6361_probe,
	.remove = mt6361_remove,
};
module_spmi_driver(mt6361_driver);

MODULE_AUTHOR("Gene Chen <gene_chen@richtek.com>");
MODULE_DESCRIPTION("MT6361 SPMI PMIC Driver");
MODULE_LICENSE("GPL v2");
