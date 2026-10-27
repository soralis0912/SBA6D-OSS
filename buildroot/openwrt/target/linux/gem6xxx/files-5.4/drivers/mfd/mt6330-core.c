// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/spmi.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt6330/core.h>
#include <linux/mfd/mt6330/registers.h>

#define MT6330_RTC_BASE		0x0588
#define MT6330_RTC_SIZE		0x3c
#define MT6330_RTC_WRTGR_OFFSET	0x3a

static const struct resource mt6330_keys_resources[] = {
	DEFINE_RES_IRQ(MT6330_IRQ_PWRKEY),
	DEFINE_RES_IRQ(MT6330_IRQ_PWRKEY_R),
};

static const struct resource mt6330_leds_resources[] = {
};


static const struct resource mt6330_auxadc_resources[] = {
};

static const struct resource mt6330_rtc_resources[] = {
	{
		.start = MT6330_RTC_BASE,
		.end   = MT6330_RTC_BASE + MT6330_RTC_SIZE,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MT6330_IRQ_RTC,
		.end   = MT6330_IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MT6330_RTC_WRTGR_OFFSET,
		.end   = MT6330_RTC_WRTGR_OFFSET,
		.flags = IORESOURCE_REG,
	},
};

static const struct resource mt6330_regulators_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VCORE_OC, "VCORE"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VMD11_OC, "VMD11"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VMD12_OC, "VMD12"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VSRAM_MD_OC, "VSRAM_MD"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VRFDIG_OC, "VRFDIG"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VS1_OC, "VS1"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VS2_OC, "VS2"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VS3_OC, "VS3"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VMC_OC, "VMC"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VXO22_OC, "VXO22"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VRF18_OC, "VRF18"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VRF13_OC, "VRF13"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VEFUSE_OC, "VEFUSE"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VA12_1_OC, "VA12_1"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VA12_2_OC, "VA12_2"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VCN18_OC, "VCN18"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VMDDR_OC, "VMDDR"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VMDDQ_OC, "VMDDQ"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VMDD2_OC, "VMDD2"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VRF09_OC, "VRF09"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VAUX18_OC, "VAUX18"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VIO18_1_OC, "VIO18_1"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VIO18_2_OC, "VIO18_2"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VSRAM_PROC_OC, "VSRAM_PROC"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VSRAM_CORE_OC, "VSRAM_CORE"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VSRAM_RFDIG_OC, "VSRAM_RFDIG"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VBBCK_OC, "VBBCK"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VEMC_OC, "VEMC"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VSIM1_OC, "VSIM1"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VSIM2_OC, "VSIM2"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VUSB_OC, "VUSB"),
	DEFINE_RES_IRQ_NAMED(MT6330_IRQ_VRFCK_OC, "VRFCK"),
};

static const struct mfd_cell mt6330_devs[] = {
	{
		.name = "mt635x-auxadc",
		.of_compatible = "mediatek,mt6330-auxadc",
		.num_resources = ARRAY_SIZE(mt6330_auxadc_resources),
		.resources = mt6330_auxadc_resources,
	}, {
		.name = "mt6359-efuse",
		.of_compatible = "mediatek,mt6330-efuse",
	}, {
		.name = "mt-pmic",
		.of_compatible = "mediatek,mt63xx-debug",
	}, {
		.name = "mt6330-regulator",
		.of_compatible = "mediatek,mt6330-regulator",
		.num_resources = ARRAY_SIZE(mt6330_regulators_resources),
		.resources = mt6330_regulators_resources,
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6330_keys_resources),
		.resources = mt6330_keys_resources,
		.of_compatible = "mediatek,mt6330-keys"
	}, {
		.name = "mtk-pmic-leds",
		.num_resources = ARRAY_SIZE(mt6330_leds_resources),
		.resources = mt6330_leds_resources,
		.of_compatible = "mediatek,mt6330_leds"
	}, {
		.name = "mtk-clock-buffer",
		.of_compatible = "mediatek,clock_buffer",
	}, {
		.name = "mt6330-rtc",
		.num_resources = ARRAY_SIZE(mt6330_rtc_resources),
		.resources = mt6330_rtc_resources,
		.of_compatible = "mediatek,mt6330-rtc",
	}, {
		.name = "mt63xx-ot-debug",
		.of_compatible = "mediatek,mt63xx-ot-debug",
	},
};

struct chip_data {
	u32 cid_addr_l;
	u32 cid_addr_h;
	u32 cid_shift;
};

static const struct chip_data mt6330_core = {
	.cid_addr_l = MT6330_SWCID0,
	.cid_addr_h = MT6330_SWCID1,
	.cid_shift = 0,
};

static const struct regmap_config spmi_regmap_config = {
	.reg_bits	= 16,
	.val_bits	= 8,
	.max_register	= 0x22ff,
	.fast_io	= true,
	.use_single_read  = false,
	.use_single_write  = false,
};

static int mt6330_probe(struct spmi_device *sdev)
{
	struct mt6330_chip *pmic;
	struct regmap *regmap;
	struct device_node *np = sdev->dev.of_node;
	int ret;
	unsigned int id = 0;
	const struct chip_data *pmic_core;


	pmic = devm_kzalloc(&sdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	pmic->sdev = sdev;
	pmic->dev = &sdev->dev;
	spmi_device_set_drvdata(sdev, pmic);

	dev_info(&sdev->dev, "mt6330 driver probe +++\n");

	/*
	 * mt6330 MFD is child device of soc pmic spmi.
	 * Regmap is set from its parent.
	 */
	regmap = devm_regmap_init_spmi_ext(sdev, &spmi_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);
	pmic->regmap = regmap;

	pmic_core = of_device_get_match_data(&sdev->dev);
	if (!pmic_core)
		return -ENODEV;

	ret = regmap_read(pmic->regmap, MT6330_SWCID1, &id);
	if (ret) {
		dev_err(&sdev->dev, "Failed to read chip id: %d\n", ret);
		return ret;
	}

	pmic->chip_id = (id & 0xff);
	pmic->irq = of_irq_get(np, 0);
	if (pmic->irq < 0) {
		dev_err(&sdev->dev, "Failed to get irq(%d)\n", pmic->irq);
	}

	ret = mt6330_irq_init(pmic);
	if (ret)
		dev_err(&sdev->dev, "irq_init failed(%d)\n", pmic->irq);

	ret = devm_mfd_add_devices(&sdev->dev, -1, mt6330_devs,
				   ARRAY_SIZE(mt6330_devs), NULL,
				   0, pmic->irq_domain);
	if (ret) {
		irq_domain_remove(pmic->irq_domain);
		dev_err(&sdev->dev, "failed to add child devices: %d\n", ret);
	}

	device_init_wakeup(&sdev->dev, true);

	dev_info(&sdev->dev, "mt6330 driver probe done\n");

	return ret;
}

static const struct of_device_id mt6330_of_match[] = {
	{
		.compatible = "mediatek,mt6330",
		.data = &mt6330_core,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mt6330_of_match);

static struct spmi_driver mt6330_driver = {
	.driver = {
		.name = "mt6330",
		.of_match_table = of_match_ptr(mt6330_of_match),
	},
	.probe = mt6330_probe,
};

module_spmi_driver(mt6330_driver);

MODULE_AUTHOR("Benjamin Chao, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6330 PMIC");
MODULE_LICENSE("GPL");
