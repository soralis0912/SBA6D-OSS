// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: Sakya Chang <jeff_chang@richtek.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/nvmem-provider.h>

#define MT6361_REG_AUXADC_DIG_3_ELR16	(0x0AC7)
#define MT6361_REG_AUXADC_DIG_3_ELR27	(0x0AD2)
#define MT6361_REG_AUXADC_EFUSE_O_VTS_L		(0x0AC8)

struct mt6361_efuse_data {
	struct nvmem_device *nvmem;
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock;
};

/*main entry point */
static int mt6361_efuse_read(void *context, unsigned int offset,
			     void *val, size_t bytes)
{
	struct mt6361_efuse_data *edata = context;
	u8 *data = (u8 *)val;
	unsigned int rdata = 0;
	int ret = 0, i = 0;
	u8 degc_cali = 0, adc_cali_en = 0, o_slope_sign = 0, o_slope = 0;
	u8 vts[8] = {0};

	dev_info(edata->dev, "%s\n", __func__);
	mutex_lock(&edata->lock);

	ret = regmap_read(edata->regmap, MT6361_REG_AUXADC_DIG_3_ELR16, &rdata);
	if (ret)
		goto unlock;
	degc_cali = rdata & GENMASK(5, 0);

	adc_cali_en = (rdata & BIT(6)) >> 6;

	ret = regmap_read(edata->regmap, MT6361_REG_AUXADC_DIG_3_ELR27, &rdata);
	if (ret)
		goto unlock;
	o_slope = rdata & GENMASK(4, 0);
	o_slope_sign =  (rdata & BIT(7)) >> 7;

	ret = regmap_raw_read(edata->regmap, MT6361_REG_AUXADC_EFUSE_O_VTS_L, vts, 8);
	if (ret)
		goto unlock;
/*
 * For fit user application
 * degc_cali should be data[1] Bit [5:0]
 * adc_cali_en should be data[1] Bit [6]
 * o_slope_sign should be data[2] Bit 7
 * o_slope should be data[2] Bit [6:1]
 * o_vts[7:0]@iio_chan_id0 = data[3][7:0]
 * o_vts[12:8]@iio_chan_id0 = data[4][4:0]
 * o_vts[0:7]@iio_chan_id1 = data[5][7:0]
 * o_vts[12:8]@iio_chan_id1 = data[6][4:0]
 * o_vts[0:7]@iio_chan_id2 = data[7][7:0]
 * o_vts[12:8]@iio_chan_id2 = data[8][4:0]
 * o_vts[0:7]@iio_chan_id3 = data[9][7:0]
 * o_vts[12:8]@iio_chan_id3 = data[10][4:0]
 */
	/* reset data */
	memset(data, 0, bytes);

	/* assign data */
	data[1] |= degc_cali;
	data[1] |= (adc_cali_en << 6);
	data[2] |= (o_slope_sign << 7);
	data[2] |= (o_slope << 1);
	data[3] |= vts[0];
	data[4] |= (vts[1] & GENMASK(4, 0));
	data[5] |= vts[2];
	data[6] |= (vts[3] & GENMASK(4, 0));
	data[7] |= vts[4];
	data[8] |= (vts[5] & GENMASK(4, 0));
	data[9] |= vts[6];
	data[10] |= (vts[7] & GENMASK(4, 0));

	for (i = 0; i < 12; i++)
		dev_info(edata->dev, "efuse data = 0x%x\n", data[i]);
unlock:
	mutex_unlock(&edata->lock);

	return ret;
}

static struct nvmem_config mt6361_efuse_nvmem_config = {
	.name = "mt6361-efuse",
	.size = 12,
	.word_size = 1,
	.stride = 1,
	.owner = THIS_MODULE,
	.reg_read = mt6361_efuse_read,
	.read_only = true,
};

static int mt6361_efuse_probe(struct platform_device *pdev)
{
	struct mt6361_efuse_data *edata;
	struct nvmem_config cfg;

	edata = devm_kzalloc(&pdev->dev, sizeof(*edata), GFP_KERNEL);
	if (!edata)
		return -ENOMEM;
	edata->dev = &pdev->dev;
	edata->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!edata->regmap) {
		dev_err(edata->dev, "get regmap failed\n");
		return 0;
	}

	mutex_init(&edata->lock);
	cfg = mt6361_efuse_nvmem_config;
	cfg.dev = &pdev->dev;
	cfg.priv = edata;
	edata->nvmem = devm_nvmem_register(edata->dev, &cfg);

	return PTR_ERR_OR_ZERO(edata->nvmem);
}

static const struct of_device_id mt6361_efuse_match[] = {
	{ .compatible = "mediatek,mt6361-efuse" },
	{ },
};
MODULE_DEVICE_TABLE(of, mt6361_efuse_match);

static struct platform_driver mt6361_efuse_driver = {
	.probe = mt6361_efuse_probe,
	.driver = {
		.name = "mt6361-efuse",
		.of_match_table = mt6361_efuse_match,
	},
};

module_platform_driver(mt6361_efuse_driver);
MODULE_AUTHOR("Sakya Chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("MT6361 Efuse Driver");
MODULE_LICENSE("GPL v2");
