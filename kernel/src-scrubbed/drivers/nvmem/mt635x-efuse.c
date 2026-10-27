// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/mfd/mt6359p/registers.h>
#include <linux/mfd/mt6330/registers.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

/* PMIC EFUSE registers definition */
#define RG_OTP_PA		0
#define RG_OTP_PDIN		1
#define RG_OTP_PTM		2
#define RG_OTP_PWE		3
#define RG_OTP_PPROG		4
#define RG_OTP_PWE_SRC		5
#define RG_OTP_PROG_PKEY	6
#define RG_OTP_RD_PKEY		7
#define RG_OTP_RD_TRIG		8
#define RG_RD_RDY_BYPASS	9
#define RG_SKIP_OTP_OUT		10
#define RG_OTP_RD_SW		11
#define RG_OTP_DOUT_SW		12
#define RG_OTP_RD_BUSY		13
#define RG_OTP_PA_SW		14
#define RG_OTP_OSC_CK_EN_SW	15

/* Mask definition for EFUSE control engine clock register */
#define RG_EFUSE_CK_PDN_HWEN_MASK	BIT(2)
#define RG_EFUSE_CK_PDN_MASK		BIT(4)
#define RG_OTP_RD_BUSY_MASK		BIT(0)
#define RG_OTP_OSC_CK_EN_SW_MASK	BIT(0)
#define RG_OTP_OSC_CK_EN_SW_SEL_MASK	BIT(1)

/* Register SET/CLR offset */
#define SET_OFFSET	0x2
#define CLR_OFFSET	0x4

/* EFUSE Register width (bytes) definitions */
#define EFUSE_REG_WIDTH		2

/* Timeout (us) of polling the status */
#define EFUSE_POLL_TIMEOUT	30000
#define EFUSE_POLL_DELAY_US	50
#define EFUSE_READ_DELAY_US	80

/* efuse attribute for chip dependent charasterictic*/
#define EFUSE_ATTR_BASIC	0x0
#define EFUSE_ATTR_OSC_CK	0x1
#define EFUSE_ATTR_SPMI_8BIT	0x2

struct efuse_chip_data {
	unsigned int reg_num;
	unsigned int base;
	unsigned int ck_pdn;
	unsigned int ck_pdn_hwen;
	const unsigned int *regtbl;
	unsigned int attr;
};

struct mt635x_efuse {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock;
	unsigned int base;
	const struct efuse_chip_data *data;
	int trig_sta;
};

static const unsigned int mt6330_efuse_regs_tbl[] = {
	0x0,
	0x1,
	0x2,
	0x3,
	0x4,
	0x5,
	0x6,
	0x8,
	0xa,
	0xb,
	0xc,
	0xd,
	0xe,
	0x10,
	0x11,
	-1
};

static const unsigned int mt6359p_efuse_regs_tbl[] = {
	0x0,
	0x2,
	0x4,
	0x6,
	0x8,
	0xA,
	0xC,
	0xE,
	0x10,
	0x12,
	0x14,
	0x16,
	0x18,
	0x1A,
	0x1C
};

static int mt635x_efuse_poll_busy(struct mt635x_efuse *efuse)
{
	int ret;
	unsigned int val = 0;

	udelay(EFUSE_POLL_DELAY_US);
	ret = regmap_read_poll_timeout(efuse->regmap,
				       efuse->data->base + efuse->data->regtbl[RG_OTP_RD_BUSY],
				       val,
				       !(val & RG_OTP_RD_BUSY_MASK),
				       EFUSE_POLL_DELAY_US,
				       EFUSE_POLL_TIMEOUT);
	if (ret) {
		dev_err(efuse->dev, "timeout to update the efuse status\n");
		return ret;
	}
	return 0;
}

static int mt635x_efuse_read(void *context, unsigned int offset,
			     void *_val, size_t bytes)
{
	struct mt635x_efuse *efuse = context;
	unsigned int base = efuse->data->base;
	unsigned int buf = 0;
	unsigned int offset_end = offset + bytes;
	unsigned short *val = _val;
	unsigned int clroffset = CLR_OFFSET, setoffset = SET_OFFSET;
	int ret;

	if (efuse->data->attr & EFUSE_ATTR_SPMI_8BIT) {
		clroffset >>= 1;
		setoffset >>= 1;
	}
	mutex_lock(&efuse->lock);
	/* Enable the efuse ctrl engine clock */
	ret = regmap_write(efuse->regmap,
			   efuse->data->ck_pdn_hwen + clroffset,
			   RG_EFUSE_CK_PDN_HWEN_MASK);
	if (ret)
		goto unlock_efuse;
	ret = regmap_write(efuse->regmap,
			   efuse->data->ck_pdn + clroffset,
			   RG_EFUSE_CK_PDN_MASK);
	if (ret)
		goto disable_efuse;

	if (efuse->data->attr & EFUSE_ATTR_OSC_CK) {
		regmap_update_bits(efuse->regmap, base + efuse->data->regtbl[RG_OTP_OSC_CK_EN_SW],
			   RG_OTP_OSC_CK_EN_SW_SEL_MASK, RG_OTP_OSC_CK_EN_SW_SEL_MASK);
		regmap_update_bits(efuse->regmap, base + efuse->data->regtbl[RG_OTP_OSC_CK_EN_SW],
			   RG_OTP_OSC_CK_EN_SW_MASK, RG_OTP_OSC_CK_EN_SW_MASK);
	}

	/* Set SW trigger read */
	ret = regmap_write(efuse->regmap, base + efuse->data->regtbl[RG_OTP_RD_SW], 1);
	if (ret)
		goto disable_efuse;
	for (; offset < offset_end; offset += EFUSE_REG_WIDTH) {
		/* Set the row to be read, one row is 2 bytes data */
		ret = regmap_write(efuse->regmap, base + efuse->data->regtbl[RG_OTP_PA], offset);
		if (ret)
			goto disable_efuse;
		/* Start trigger read */
		efuse->trig_sta = efuse->trig_sta ? 0 : 1;
		ret = regmap_write(efuse->regmap, base + efuse->data->regtbl[RG_OTP_RD_TRIG],
				   efuse->trig_sta);
		if (ret) {
			efuse->trig_sta = efuse->trig_sta ? 0 : 1;
			goto disable_efuse;
		}

		/*
		 * Polling the busy status to make sure the reading process
		 * is completed, that means the data can be read out now.
		 */
		ret = mt635x_efuse_poll_busy(efuse);
		if (ret)
			goto disable_efuse;

		/* Read data from efuse memory, must delay before read */
		udelay(EFUSE_READ_DELAY_US);
		if (efuse->data->attr & EFUSE_ATTR_SPMI_8BIT)
			ret = regmap_bulk_read(efuse->regmap,
				  base + efuse->data->regtbl[RG_OTP_DOUT_SW], &buf, 2);
		else
			ret = regmap_read(efuse->regmap, base + efuse->data->regtbl[RG_OTP_DOUT_SW],
				  &buf);
		if (ret)
			goto disable_efuse;
		dev_dbg(efuse->dev, "EFUSE[%d]=0x%x\n", offset, buf);
		*val++ = (unsigned short)buf;
	}
disable_efuse:
	/* Disable SW trigger read */
	regmap_write(efuse->regmap, base + efuse->data->regtbl[RG_OTP_RD_SW], 0);
	/* Disable the efuse ctrl engine clock */
	regmap_write(efuse->regmap, efuse->data->ck_pdn_hwen + setoffset,
		     RG_EFUSE_CK_PDN_HWEN_MASK);
	regmap_write(efuse->regmap, efuse->data->ck_pdn + setoffset,
		     RG_EFUSE_CK_PDN_MASK);
	if (efuse->data->attr & EFUSE_ATTR_OSC_CK) {
		regmap_update_bits(efuse->regmap, base + efuse->data->regtbl[RG_OTP_OSC_CK_EN_SW],
			   RG_OTP_OSC_CK_EN_SW_SEL_MASK, 0);
		regmap_update_bits(efuse->regmap, base + efuse->data->regtbl[RG_OTP_OSC_CK_EN_SW],
			   RG_OTP_OSC_CK_EN_SW_MASK, 0);
	}
unlock_efuse:
	mutex_unlock(&efuse->lock);

	return ret;
}

static int mt635x_efuse_probe(struct platform_device *pdev)
{
	struct nvmem_config econfig = { };
	struct nvmem_device *nvmem;
	struct mt635x_efuse *efuse;
	struct mt6397_chip *chip;
	int ret;

	chip = dev_get_drvdata(pdev->dev.parent);
	if (!chip)
		return -ENODEV;
	efuse = devm_kzalloc(&pdev->dev, sizeof(*efuse), GFP_KERNEL);
	if (!efuse)
		return -ENOMEM;

	efuse->regmap = chip->regmap;
	if (!efuse->regmap) {
		dev_err(&pdev->dev, "failed to get efuse regmap\n");
		return -ENODEV;
	}

	efuse->data = of_device_get_match_data(&pdev->dev);
	if (!efuse->data) {
		dev_err(&pdev->dev, "failed to get efuse data\n");
		return -ENODEV;
	}
	ret = regmap_read(efuse->regmap, efuse->data->base + efuse->data->regtbl[RG_OTP_RD_TRIG],
			  &efuse->trig_sta);
	if (ret)
		return ret;

	mutex_init(&efuse->lock);
	efuse->dev = &pdev->dev;
	platform_set_drvdata(pdev, efuse);

	econfig.stride = 2;
	econfig.word_size = EFUSE_REG_WIDTH;
	econfig.read_only = true;
	econfig.name = "mt635x-efuse";
	econfig.size = efuse->data->reg_num * EFUSE_REG_WIDTH;
	econfig.reg_read = mt635x_efuse_read;
	econfig.priv = efuse;
	econfig.dev = &pdev->dev;
	nvmem = devm_nvmem_register(&pdev->dev, &econfig);
	if (IS_ERR(nvmem)) {
		dev_err(&pdev->dev, "failed to register %s nvmem config\n",
			econfig.name);
		mutex_destroy(&efuse->lock);
		return PTR_ERR(nvmem);
	}

	return 0;
}

static const struct efuse_chip_data mt6330_efuse_data = {
	.reg_num = 128,
	.base = MT6330_OTP_CON0,
	.ck_pdn = MT6330_TOP_CKPDN_CON1,
	.ck_pdn_hwen = MT6330_TOP_CKHWEN_CON0,
	.regtbl = mt6330_efuse_regs_tbl,
	.attr = (EFUSE_ATTR_BASIC | EFUSE_ATTR_OSC_CK | EFUSE_ATTR_SPMI_8BIT)
};

static const struct efuse_chip_data mt6359p_efuse_data = {
	.reg_num = 128,
	.base = MT6359P_OTP_CON0,
	.ck_pdn = MT6359P_TOP_CKPDN_CON0,
	.ck_pdn_hwen = MT6359P_TOP_CKHWEN_CON0,
	.regtbl = mt6359p_efuse_regs_tbl,
	.attr = EFUSE_ATTR_BASIC
};

static const struct of_device_id mt635x_efuse_of_match[] = {
	{
		.compatible = "mediatek,mt6330-efuse",
		.data = &mt6330_efuse_data
	}, {
		.compatible = "mediatek,mt6359p-efuse",
		.data = &mt6359p_efuse_data
	}, {
		/* sentinel */
	}
};

static struct platform_driver mt635x_efuse_driver = {
	.probe = mt635x_efuse_probe,
	.driver = {
		.name = "mt635x-efuse",
		.of_match_table = mt635x_efuse_of_match,
	},
};
module_platform_driver(mt635x_efuse_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jeter Chen <Jeter.Chen@mediatek.com>");
MODULE_DESCRIPTION("MediaTek PMIC eFuse Driver for MT6359 PMIC");
