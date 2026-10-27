// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2021 MediaTek Inc.
//
// Author: Gene Chen <gene_chen@richtek.com>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include <dt-bindings/regulator/mediatek,mt6361-regulator.h>

#define MT6361_TOP_INFO1		0x00
#define MT6361_BUCK_TOP_KEY_PROT	0x4CB

#define MT6361_MSK_CHIP_REV	GENMASK(3, 0)

#define MT6361_MASK_LP			BIT(0)
#define MT6361_MASK_ULP			BIT(1)
#define MT6361_MASK_FCCM		BIT(2)

#define MT6361_VSEL_SIZE	2

enum {
	MT6361_REGULATOR_LDO1 = 0,
	MT6361_REGULATOR_LDO2,
	MT6361_REGULATOR_LDO3,
	MT6361_REGULATOR_LDO4,
	MT6361_REGULATOR_LDO5,
	MT6361_REGULATOR_LDO6,
	MT6361_REGULATOR_LDO7,
	MT6361_REGULATOR_LDO8,
	MT6361_REGULATOR_LDO9,
	MT6361_REGULATOR_LDO10,
	MT6361_REGULATOR_LDO11,
	MT6361_REGULATOR_LDO12,
	MT6361_REGULATOR_LDO13,
	MT6361_REGULATOR_LDO14,
	MT6361_REGULATOR_LDO14_E2,
	MT6361_REGULATOR_LDO15,
	MT6361_REGULATOR_LDO15_E2,
	MT6361_REGULATOR_LDO16,
	MT6361_REGULATOR_LDO16_E2,
	MT6361_REGULATOR_LDO17,
	MT6361_REGULATOR_LDO17_E2,
	MT6361_REGULATOR_LDO18,
	MT6361_REGULATOR_LDO19,
	MT6361_REGULATOR_LDO21,
	MT6361_REGULATOR_LDO22,
	MT6361_REGULATOR_LDO23,
	MT6361_REGULATOR_LDO23_E2,
	MT6361_REGULATOR_LDO24,
	MT6361_REGULATOR_LDO25,
	MT6361_REGULATOR_LDO26,
	MT6361_REGULATOR_LDO27,
	MT6361_REGULATOR_LDO28,
	MT6361_REGULATOR_LDO29,
	MT6361_REGULATOR_LDO30,
	MT6361_REGULATOR_BUCK1,
	MT6361_REGULATOR_BUCK2,
	MT6361_REGULATOR_BUCK3,
	MT6361_REGULATOR_BUCK4,
	MT6361_REGULATOR_BUCK5,
	MT6361_REGULATOR_BUCK6,
	MT6361_REGULATOR_BUCK7,
	MT6361_REGULATOR_BUCK8,
	MT6361_REGULATOR_MAX,
};

struct mt6361_regulator_data {
	struct device *dev;
	struct regmap *rmap;
	u8 chip_rev;
};

struct mt6361_regulator_desc {
	struct regulator_desc desc;
	unsigned int mode_reg;
	unsigned int mode_mask;
	unsigned int ulp_reg;
	unsigned int ulp_mask;
	unsigned int state_reg;
	unsigned int state_mask;
	unsigned int fccm_reg;
	unsigned int fccm_mask;
};

static irqreturn_t mt6361_pgb_event_handler(int irq, void *data)
{
	struct regulator_dev *rdev = data;

	regulator_notifier_call_chain(rdev, REGULATOR_EVENT_FAIL, NULL);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_oc_event_handler(int irq, void *data)
{
	struct regulator_dev *rdev = data;

	regulator_notifier_call_chain(rdev, REGULATOR_EVENT_OVER_CURRENT, NULL);
	return IRQ_HANDLED;
}

/* 2 bytes voltage selector */
static const struct regulator_linear_range ldo_vout_ranges1[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(700000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(710000, 0x0101, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(800000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(810000, 0x0201, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(900000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(910000, 0x0301, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(1000000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(1010000, 0x0401, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(1100000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(1110000, 0x0501, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(1200000, 0x050a, 0x0600, 0),
	REGULATOR_LINEAR_RANGE(1210000, 0x0601, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(1300000, 0x060a, 0x0700, 0),
	REGULATOR_LINEAR_RANGE(1310000, 0x0701, 0x0709, 10000),
	REGULATOR_LINEAR_RANGE(1400000, 0x070a, 0x0800, 0),
	REGULATOR_LINEAR_RANGE(1410000, 0x0801, 0x0809, 10000),
	REGULATOR_LINEAR_RANGE(1500000, 0x080a, 0x0900, 0),
	REGULATOR_LINEAR_RANGE(1510000, 0x0901, 0x0909, 10000),
	REGULATOR_LINEAR_RANGE(1600000, 0x090a, 0x0a00, 0),
	REGULATOR_LINEAR_RANGE(1610000, 0x0a01, 0x0a09, 10000),
	REGULATOR_LINEAR_RANGE(1700000, 0x0a0a, 0x0b00, 0),
	REGULATOR_LINEAR_RANGE(1710000, 0x0b01, 0x0b09, 10000),
	REGULATOR_LINEAR_RANGE(1800000, 0x0b0a, 0x0c00, 0),
	REGULATOR_LINEAR_RANGE(1810000, 0x0c01, 0x0c09, 10000),
	REGULATOR_LINEAR_RANGE(1900000, 0x0c0a, 0x0d00, 0),
	REGULATOR_LINEAR_RANGE(1910000, 0x0d01, 0x0d09, 10000),
	REGULATOR_LINEAR_RANGE(2000000, 0x0d0a, 0x0e00, 0),
	REGULATOR_LINEAR_RANGE(2010000, 0x0e01, 0x0e09, 10000),
	REGULATOR_LINEAR_RANGE(2100000, 0x0e0a, 0x0f00, 0),
	REGULATOR_LINEAR_RANGE(2110000, 0x0f01, 0x0f09, 10000),
	REGULATOR_LINEAR_RANGE(2200000, 0x0f0a, 0x0f0f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges2[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(700000, 0x000a, 0x000f, 0),
	REGULATOR_LINEAR_RANGE(650000, 0x0100, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(750000, 0x010a, 0x010f, 0),
	REGULATOR_LINEAR_RANGE(700000, 0x0200, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(800000, 0x020a, 0x020f, 0),
	REGULATOR_LINEAR_RANGE(750000, 0x0300, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(850000, 0x030a, 0x030f, 0),
	REGULATOR_LINEAR_RANGE(800000, 0x0400, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(900000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(910000, 0x0501, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(1000000, 0x050a, 0x050f, 0),
	REGULATOR_LINEAR_RANGE(950000, 0x0600, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(1050000, 0x060a, 0x060f, 0),
	REGULATOR_LINEAR_RANGE(1000000, 0x0700, 0x0709, 10000),
	REGULATOR_LINEAR_RANGE(1100000, 0x070a, 0x070f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges3[] = {
	REGULATOR_LINEAR_RANGE(900000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(1000000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(1010000, 0x0101, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(1100000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(1110000, 0x0201, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(1200000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(1210000, 0x0301, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(1300000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(1310000, 0x0401, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(1400000, 0x040a, 0x040f, 0),
	REGULATOR_LINEAR_RANGE(1700000, 0x0500, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(1800000, 0x050a, 0x0600, 0),
	REGULATOR_LINEAR_RANGE(1810000, 0x0601, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(1900000, 0x060a, 0x060f, 0),
	REGULATOR_LINEAR_RANGE(1910000, 0x070a, 0x070f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges4[] = {
	REGULATOR_LINEAR_RANGE(550000, 0x0000, 0x0009, 5000),
	REGULATOR_LINEAR_RANGE(600000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(605000, 0x0101, 0x0109, 5000),
	REGULATOR_LINEAR_RANGE(650000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(655000, 0x0201, 0x0209, 5000),
	REGULATOR_LINEAR_RANGE(700000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(705000, 0x0301, 0x0309, 5000),
	REGULATOR_LINEAR_RANGE(750000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(755000, 0x0401, 0x0409, 5000),
	REGULATOR_LINEAR_RANGE(800000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(805000, 0x0501, 0x0509, 5000),
	REGULATOR_LINEAR_RANGE(850000, 0x050a, 0x050f, 0),
	REGULATOR_LINEAR_RANGE(900000, 0x0600, 0x0609, 5000),
	REGULATOR_LINEAR_RANGE(950000, 0x060a, 0x0700, 0),
	REGULATOR_LINEAR_RANGE(955000, 0x0701, 0x0709, 5000),
	REGULATOR_LINEAR_RANGE(1000000, 0x070a, 0x0800, 0),
	REGULATOR_LINEAR_RANGE(1005000, 0x0801, 0x0809, 5000),
	REGULATOR_LINEAR_RANGE(1050000, 0x080a, 0x0900, 0),
	REGULATOR_LINEAR_RANGE(1055000, 0x0901, 0x0909, 5000),
	REGULATOR_LINEAR_RANGE(1100000, 0x090a, 0x0a00, 0),
	REGULATOR_LINEAR_RANGE(1105000, 0x0a01, 0x0a09, 5000),
	REGULATOR_LINEAR_RANGE(1150000, 0x0a0a, 0x0b00, 0),
	REGULATOR_LINEAR_RANGE(1155000, 0x0b01, 0x0b09, 5000),
	REGULATOR_LINEAR_RANGE(1200000, 0x0b0a, 0x0b0f, 0),
	REGULATOR_LINEAR_RANGE(1700000, 0x0c00, 0x0c09, 5000),
	REGULATOR_LINEAR_RANGE(1750000, 0x0c0a, 0x0d00, 0),
	REGULATOR_LINEAR_RANGE(1755000, 0x0d01, 0x0d09, 5000),
	REGULATOR_LINEAR_RANGE(1800000, 0x0d0a, 0x0e00, 0),
	REGULATOR_LINEAR_RANGE(1805000, 0x0e01, 0x0e09, 5000),
	REGULATOR_LINEAR_RANGE(1850000, 0x0e0a, 0x0f00, 0),
	REGULATOR_LINEAR_RANGE(1855000, 0x0f01, 0x0f09, 5000),
	REGULATOR_LINEAR_RANGE(1900000, 0x0f0a, 0x0f0f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges5[] = {
	REGULATOR_LINEAR_RANGE(1800000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(1900000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(1910000, 0x0101, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(2000000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(2010000, 0x0201, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(2100000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(2110000, 0x0301, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(2200000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(2210000, 0x0401, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(2300000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(2310000, 0x0501, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(2400000, 0x050a, 0x0600, 0),
	REGULATOR_LINEAR_RANGE(2410000, 0x0601, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(2500000, 0x060a, 0x0700, 0),
	REGULATOR_LINEAR_RANGE(2510000, 0x0701, 0x0709, 10000),
	REGULATOR_LINEAR_RANGE(2600000, 0x070a, 0x0800, 0),
	REGULATOR_LINEAR_RANGE(2610000, 0x0801, 0x0809, 10000),
	REGULATOR_LINEAR_RANGE(2700000, 0x080a, 0x0900, 0),
	REGULATOR_LINEAR_RANGE(2710000, 0x0901, 0x0909, 10000),
	REGULATOR_LINEAR_RANGE(2800000, 0x090a, 0x0a00, 0),
	REGULATOR_LINEAR_RANGE(2810000, 0x0a01, 0x0a09, 10000),
	REGULATOR_LINEAR_RANGE(2900000, 0x0a0a, 0x0b00, 0),
	REGULATOR_LINEAR_RANGE(2910000, 0x0b01, 0x0b09, 10000),
	REGULATOR_LINEAR_RANGE(3000000, 0x0b0a, 0x0c00, 0),
	REGULATOR_LINEAR_RANGE(3010000, 0x0c01, 0x0c09, 10000),
	REGULATOR_LINEAR_RANGE(3100000, 0x0c0a, 0x0d00, 0),
	REGULATOR_LINEAR_RANGE(3110000, 0x0d01, 0x0d09, 10000),
	REGULATOR_LINEAR_RANGE(3200000, 0x0d0a, 0x0e00, 0),
	REGULATOR_LINEAR_RANGE(3210000, 0x0e01, 0x0e09, 10000),
	REGULATOR_LINEAR_RANGE(3300000, 0x0e0a, 0x0f00, 0),
	REGULATOR_LINEAR_RANGE(3310000, 0x0f01, 0x0f09, 10000),
	REGULATOR_LINEAR_RANGE(3400000, 0x0f0a, 0x0f0f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges6[] = {
	REGULATOR_LINEAR_RANGE(1200000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(1300000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(1310000, 0x0101, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(1400000, 0x010a, 0x010f, 0),
	REGULATOR_LINEAR_RANGE(1500000, 0x0200, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(1600000, 0x020a, 0x020f, 0),
	REGULATOR_LINEAR_RANGE(1700000, 0x0300, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(1800000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(1810000, 0x0401, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(1900000, 0x040a, 0x040f, 0),
	REGULATOR_LINEAR_RANGE(2000000, 0x0500, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(2100000, 0x050a, 0x0600, 0),
	REGULATOR_LINEAR_RANGE(2110000, 0x0601, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(2200000, 0x060a, 0x0700, 0),
	REGULATOR_LINEAR_RANGE(2210000, 0x0701, 0x0709, 10000),
	REGULATOR_LINEAR_RANGE(2300000, 0x070a, 0x070f, 0),
	REGULATOR_LINEAR_RANGE(2700000, 0x0800, 0x0809, 10000),
	REGULATOR_LINEAR_RANGE(2800000, 0x080a, 0x0900, 0),
	REGULATOR_LINEAR_RANGE(2810000, 0x0901, 0x0909, 10000),
	REGULATOR_LINEAR_RANGE(2900000, 0x090a, 0x0a00, 0),
	REGULATOR_LINEAR_RANGE(2910000, 0x0a01, 0x0a09, 10000),
	REGULATOR_LINEAR_RANGE(3000000, 0x0a0a, 0x0b00, 0),
	REGULATOR_LINEAR_RANGE(3010000, 0x0b01, 0x0b09, 10000),
	REGULATOR_LINEAR_RANGE(3100000, 0x0b0a, 0x0c00, 0),
	REGULATOR_LINEAR_RANGE(3110000, 0x0c01, 0x0c09, 10000),
	REGULATOR_LINEAR_RANGE(3200000, 0x0c0a, 0x0c0f, 0),
	REGULATOR_LINEAR_RANGE(3300000, 0x0d00, 0x0d09, 10000),
	REGULATOR_LINEAR_RANGE(3400000, 0x0d0a, 0x0e00, 0),
	REGULATOR_LINEAR_RANGE(3410000, 0x0e01, 0x0e09, 10000),
	REGULATOR_LINEAR_RANGE(3500000, 0x0e0a, 0x0f00, 0),
	REGULATOR_LINEAR_RANGE(3510000, 0x0f01, 0x0f09, 10000),
	REGULATOR_LINEAR_RANGE(3600000, 0x0f0a, 0x0f0f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges7[] = {
	REGULATOR_LINEAR_RANGE(1500000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(1600000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(1610000, 0x0101, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(1700000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(1710000, 0x0201, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(1800000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(1810000, 0x0301, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(1900000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(1910000, 0x0401, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(2000000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(2010000, 0x0501, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(2100000, 0x050a, 0x0600, 0),
	REGULATOR_LINEAR_RANGE(2110000, 0x0601, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(2200000, 0x060a, 0x0700, 0),
	REGULATOR_LINEAR_RANGE(2210000, 0x0701, 0x0709, 10000),
	REGULATOR_LINEAR_RANGE(2300000, 0x070a, 0x0800, 0),
	REGULATOR_LINEAR_RANGE(2310000, 0x0801, 0x0809, 10000),
	REGULATOR_LINEAR_RANGE(2400000, 0x080a, 0x0900, 0),
	REGULATOR_LINEAR_RANGE(2410000, 0x0901, 0x0909, 10000),
	REGULATOR_LINEAR_RANGE(2500000, 0x090a, 0x0a00, 0),
	REGULATOR_LINEAR_RANGE(2510000, 0x0a01, 0x0a09, 10000),
	REGULATOR_LINEAR_RANGE(2600000, 0x0a0a, 0x0b00, 0),
	REGULATOR_LINEAR_RANGE(2610000, 0x0b01, 0x0b09, 10000),
	REGULATOR_LINEAR_RANGE(2700000, 0x0b0a, 0x0c00, 0),
	REGULATOR_LINEAR_RANGE(2710000, 0x0c01, 0x0c09, 10000),
	REGULATOR_LINEAR_RANGE(2800000, 0x0c0a, 0x0d00, 0),
	REGULATOR_LINEAR_RANGE(2810000, 0x0d01, 0x0d09, 10000),
	REGULATOR_LINEAR_RANGE(2900000, 0x0d0a, 0x0e00, 0),
	REGULATOR_LINEAR_RANGE(2910000, 0x0e01, 0x0e09, 10000),
	REGULATOR_LINEAR_RANGE(3000000, 0x0e0a, 0x0f00, 0),
	REGULATOR_LINEAR_RANGE(3010000, 0x0f01, 0x0f09, 10000),
	REGULATOR_LINEAR_RANGE(3100000, 0x0f0a, 0x0f0f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges8[] = {
	REGULATOR_LINEAR_RANGE(400000, 0, 0x7f, 6250),
};

static const struct regulator_linear_range ldo_vout_ranges9[] = {
	REGULATOR_LINEAR_RANGE(266000, 0, 0x7f, 4160),
};

static const struct regulator_linear_range ldo_vout_ranges10[] = {
	REGULATOR_LINEAR_RANGE(250000, 0, 0x7f, 6250),
};

static const struct regulator_linear_range ldo_vout_ranges11[] = {
	REGULATOR_LINEAR_RANGE(550000, 0x0000, 0x0009, 5000),
	REGULATOR_LINEAR_RANGE(600000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(605000, 0x0101, 0x0109, 5000),
	REGULATOR_LINEAR_RANGE(650000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(655000, 0x0201, 0x0209, 5000),
	REGULATOR_LINEAR_RANGE(700000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(705000, 0x0301, 0x0309, 5000),
	REGULATOR_LINEAR_RANGE(750000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(755000, 0x0401, 0x0409, 5000),
	REGULATOR_LINEAR_RANGE(800000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(805000, 0x0501, 0x0509, 5000),
	REGULATOR_LINEAR_RANGE(850000, 0x050a, 0x0600, 0),
	REGULATOR_LINEAR_RANGE(855000, 0x0601, 0x0609, 5000),
	REGULATOR_LINEAR_RANGE(900000, 0x060a, 0x0700, 0),
	REGULATOR_LINEAR_RANGE(905000, 0x0701, 0x0709, 5000),
	REGULATOR_LINEAR_RANGE(950000, 0x070a, 0x0800, 0),
	REGULATOR_LINEAR_RANGE(955000, 0x0801, 0x0809, 5000),
	REGULATOR_LINEAR_RANGE(1000000, 0x080a, 0x0900, 0),
	REGULATOR_LINEAR_RANGE(1005000, 0x0901, 0x0909, 5000),
	REGULATOR_LINEAR_RANGE(1050000, 0x090a, 0x0a00, 0),
	REGULATOR_LINEAR_RANGE(1055000, 0x0a01, 0x0a09, 5000),
	REGULATOR_LINEAR_RANGE(1100000, 0x0a0a, 0x0b00, 0),
	REGULATOR_LINEAR_RANGE(1105000, 0x0b01, 0x0b09, 5000),
	REGULATOR_LINEAR_RANGE(1150000, 0x0b0a, 0x0c00, 0),
	REGULATOR_LINEAR_RANGE(1155000, 0x0c01, 0x0c09, 5000),
	REGULATOR_LINEAR_RANGE(1200000, 0x0c0a, 0x0c0f, 0),
	REGULATOR_LINEAR_RANGE(1700000, 0x0d00, 0x0d09, 5000),
	REGULATOR_LINEAR_RANGE(1750000, 0x0d0a, 0x0e00, 0),
	REGULATOR_LINEAR_RANGE(1755000, 0x0e01, 0x0e09, 5000),
	REGULATOR_LINEAR_RANGE(1800000, 0x0e0a, 0x0f00, 0),
	REGULATOR_LINEAR_RANGE(1805000, 0x0f01, 0x0f09, 5000),
	REGULATOR_LINEAR_RANGE(1850000, 0x0f0a, 0x0f0f, 0),
};

static const struct regulator_linear_range ldo_vout_ranges12[] = {
	REGULATOR_LINEAR_RANGE(800000, 0x0000, 0x0009, 10000),
	REGULATOR_LINEAR_RANGE(900000, 0x000a, 0x0100, 0),
	REGULATOR_LINEAR_RANGE(910000, 0x0101, 0x0109, 10000),
	REGULATOR_LINEAR_RANGE(1000000, 0x010a, 0x0200, 0),
	REGULATOR_LINEAR_RANGE(1010000, 0x0201, 0x0209, 10000),
	REGULATOR_LINEAR_RANGE(1100000, 0x020a, 0x0300, 0),
	REGULATOR_LINEAR_RANGE(1110000, 0x0301, 0x0309, 10000),
	REGULATOR_LINEAR_RANGE(1200000, 0x030a, 0x0400, 0),
	REGULATOR_LINEAR_RANGE(1210000, 0x0401, 0x0409, 10000),
	REGULATOR_LINEAR_RANGE(1300000, 0x040a, 0x0500, 0),
	REGULATOR_LINEAR_RANGE(1310000, 0x0501, 0x0509, 10000),
	REGULATOR_LINEAR_RANGE(1400000, 0x050a, 0x050f, 0),
	REGULATOR_LINEAR_RANGE(1700000, 0x0600, 0x0609, 10000),
	REGULATOR_LINEAR_RANGE(1800000, 0x060a, 0x060f, 0),
};

static const struct regulator_linear_range buck_vout_ranges1[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xff, 12500),
};

static const struct regulator_linear_range buck_vout_ranges2[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xff, 6250),
};

static int mt6361_regulator_set_voltage_sel(struct regulator_dev *rdev,
					    unsigned sel)
{
	const struct regulator_desc *desc = rdev->desc;
	struct regmap *regmap = rdev_get_regmap(rdev);

	sel <<= ffs(desc->vsel_mask) - 1;

	return regmap_bulk_write(regmap, desc->vsel_reg, &sel, MT6361_VSEL_SIZE);
}

static int mt6361_regulator_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *desc = rdev->desc;
	struct regmap *regmap = rdev_get_regmap(rdev);
	unsigned int val;
	int ret;

	ret = regmap_bulk_read(regmap, desc->vsel_reg, &val, MT6361_VSEL_SIZE);
	if (ret)
		return ret;

	val &= desc->vsel_mask;
	val >>= ffs(desc->vsel_mask) - 1;
	return val;
}

static unsigned int mt6361_regulator_get_mode(struct regulator_dev *rdev)
{
	const struct mt6361_regulator_desc *rdesc =
				(struct mt6361_regulator_desc *)rdev->desc;
	struct regmap *regmap = rdev_get_regmap(rdev);
	int shift = ffs(rdesc->mode_mask) - 1, ulp_shift = ffs(rdesc->ulp_mask) - 1;
	unsigned int val, fccm_val, ulp_val;
	int ret;

	ret = regmap_read(regmap, rdesc->mode_reg, &val);
	if (ret)
		return ret;
	val &= rdesc->mode_mask;
	val >>= shift;

	switch (val) {
	case MT6361_OPMODE_NORMAL:
		if (!rdesc->fccm_reg)
			return REGULATOR_MODE_NORMAL;

		ret = regmap_read(regmap, rdesc->fccm_reg, &fccm_val);
		if (ret)
			return ret;
		if (fccm_val & rdesc->fccm_mask)
			return REGULATOR_MODE_FAST;
		else
			return REGULATOR_MODE_NORMAL;
	case MT6361_OPMODE_LP:
		if (!rdesc->ulp_reg)
			return REGULATOR_MODE_IDLE;

		ret = regmap_read(regmap, rdesc->ulp_reg, &ulp_val);
		if (ret)
			return ret;
		ulp_val &= rdesc->ulp_mask;
		ulp_val >>= ulp_shift;

		if (ulp_val)
			return REGULATOR_MODE_STANDBY;
		else
			return REGULATOR_MODE_IDLE;
	default:
		return -EINVAL;
	}
}

static int mt6361_buckkey(struct regmap *regmap, bool en)
{
	u8 buckkey[2] = {0x43, 0x55};

	if (en)
		return regmap_raw_write(regmap, MT6361_BUCK_TOP_KEY_PROT, buckkey, 2);

	buckkey[0] = buckkey[1] = 0;
	return regmap_raw_write(regmap, MT6361_BUCK_TOP_KEY_PROT, buckkey, 2);
}

static int mt6361_regulator_set_mode(struct regulator_dev *rdev,
				     unsigned int mode)
{
	const struct mt6361_regulator_desc *rdesc =
				(struct mt6361_regulator_desc *)rdev->desc;
	struct regmap *regmap = rdev_get_regmap(rdev);
	int shift = ffs(rdesc->mode_mask) - 1, ulp_shift = ffs(rdesc->ulp_mask) - 1;
	int fccm_shift = ffs(rdesc->fccm_mask) - 1;
	unsigned int val, lp, ulp, fccm;
	int ret, curr_mode;

	curr_mode = mt6361_regulator_get_mode(rdev);
	switch (mode) {
	case REGULATOR_MODE_NORMAL:
		val = MT6361_OPMODE_NORMAL;
		break;
	case REGULATOR_MODE_STANDBY:
		if (curr_mode == REGULATOR_MODE_FAST) {
			WARN_ON(1);
			dev_notice(&rdev->dev,
				   "BUCK %s is FPWM mode, can't enter ULP\n",
				   rdev->desc->name);
			return -EIO;
		}
		val = MT6361_OPMODE_ULP;
		break;
	case REGULATOR_MODE_IDLE:
		if (curr_mode == REGULATOR_MODE_FAST) {
			WARN_ON(1);
			dev_notice(&rdev->dev,
				   "BUCK %s is FPWM mode, can't enter LP\n",
				   rdev->desc->name);
			return -EIO;
		}
		val = MT6361_OPMODE_LP;
		break;
	case REGULATOR_MODE_FAST:
		if (curr_mode == REGULATOR_MODE_IDLE ||
		    curr_mode == REGULATOR_MODE_STANDBY) {
			WARN_ON(1);
			dev_notice(&rdev->dev,
				   "BUCK %s is LP mode, can't FPWM\n",
				   rdev->desc->name);
			return -EIO;
		}
		val = MT6361_OPMODE_FCCM;
		break;
	default:
		return -EINVAL;
	}

	lp = val & MT6361_MASK_LP ? 1 : 0;
	ulp = val & MT6361_MASK_ULP ? 1 : 0;
	fccm = val & MT6361_MASK_FCCM ? 1 : 0;

	ret = regmap_update_bits(regmap, rdesc->mode_reg, rdesc->mode_mask,
				 lp << shift);
	if (ret) {
		dev_err(&rdev->dev, "failed to set mode(%d)\n", ret);
		return ret;
	}

	if (rdesc->ulp_reg) {
		ret = regmap_update_bits(regmap, rdesc->ulp_reg, rdesc->ulp_mask,
					 ulp << ulp_shift);
		if (ret) {
			dev_err(&rdev->dev, "failed to set ulp mode(%d)\n", ret);
			return ret;
		}
	}

	if (rdesc->fccm_reg) {
		ret = mt6361_buckkey(regmap, true);
		if (ret) {
			dev_err(&rdev->dev, "failed to en buckkey(%d)\n", ret);
			return ret;
		}
		ret = regmap_update_bits(regmap, rdesc->fccm_reg, rdesc->fccm_mask,
					 fccm << fccm_shift);
		if (ret)
			dev_err(&rdev->dev, "failed to set fccm mode(%d)\n", ret);
		mt6361_buckkey(regmap, false);
		return ret;
	}

	return 0;
}

static unsigned int mt6361_regulator_of_map_mode(unsigned int hw_mode)
{
	unsigned int trans_mode = 0;

	switch (hw_mode) {
	case MT6361_OPMODE_NORMAL:
		trans_mode = REGULATOR_MODE_NORMAL;
		break;
	case MT6361_OPMODE_LP:
		trans_mode = REGULATOR_MODE_IDLE;
		break;
	case MT6361_OPMODE_ULP:
		trans_mode = REGULATOR_MODE_STANDBY;
		break;
	case MT6361_OPMODE_FCCM:
		trans_mode = REGULATOR_MODE_FAST;
		break;
	}
	return trans_mode;
}

static int mt6361_regulator_get_status(struct regulator_dev *rdev)
{
	const struct mt6361_regulator_desc *rdesc = (struct mt6361_regulator_desc *)rdev->desc;
	struct regmap *regmap = rdev_get_regmap(rdev);
	unsigned int val;
	int ret;

	ret = regmap_read(regmap, rdesc->state_reg, &val);
	if (ret)
		return ret;

	if (val & rdesc->state_mask)
		return REGULATOR_STATUS_ON;

	return REGULATOR_STATUS_OFF;
}

static const struct regulator_ops mt6361_volt_tbl_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.set_voltage_sel = mt6361_regulator_set_voltage_sel,
	.get_voltage_sel = mt6361_regulator_get_voltage_sel,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.set_mode = mt6361_regulator_set_mode,
	.get_mode = mt6361_regulator_get_mode,
	.get_status = mt6361_regulator_get_status,
};

static const struct regulator_ops mt6361_volt_tbl_ops2 = {
	.list_voltage = regulator_list_voltage_linear_range,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.set_mode = mt6361_regulator_set_mode,
	.get_mode = mt6361_regulator_get_mode,
	.get_status = mt6361_regulator_get_status,
};

#define MT6361_REGULATOR_DESC(_name, ereg, emask, vreg, vmask,		\
			      mreg, mmask, streg, stmask, ureg, umask,	\
			      freg, fmask, vranges, vcnts, offon_delay,	\
			      _ops)					\
{									\
	.desc = {							\
		.name = #_name,						\
		.id = MT6361_REGULATOR_##_name,				\
		.of_match = of_match_ptr(#_name),			\
		.of_map_mode = mt6361_regulator_of_map_mode,		\
		.owner = THIS_MODULE,					\
		.ops = &_ops,						\
		.type = REGULATOR_VOLTAGE,				\
		.vsel_reg = vreg,					\
		.vsel_mask = vmask,					\
		.enable_reg = ereg,					\
		.enable_mask = emask,					\
		.linear_ranges = vranges,				\
		.n_linear_ranges = ARRAY_SIZE(vranges),			\
		.n_voltages = vcnts,					\
		.off_on_delay = offon_delay,				\
	},								\
	.mode_reg = mreg,						\
	.mode_mask = mmask,						\
	.ulp_reg = ureg,						\
	.ulp_mask = umask,						\
	.fccm_reg = freg,						\
	.fccm_mask = fmask,						\
	.state_reg = streg,						\
	.state_mask = stmask,						\
}

static struct mt6361_regulator_desc mt6361_regulator_descs[] = {
	MT6361_REGULATOR_DESC(LDO1, 0x500, 0x01, 0x50E, 0x0F0F, 0x500, 0x02,
			      0x504, 0x01, 0x501, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO2, 0x520, 0x01, 0x52E, 0x0F0F, 0x520, 0x02,
			      0x524, 0x01, 0x521, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO3, 0x540, 0x01, 0x54E, 0x0F0F, 0x540, 0x02,
			      0x544, 0x01, 0x541, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO4, 0x560, 0x01, 0x56E, 0x0F0F, 0x560, 0x02,
			      0x564, 0x01, 0x561, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO5, 0x580, 0x01, 0x58E, 0x0F0F, 0x580, 0x02,
			      0x584, 0x01, 0x581, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO6, 0x5A0, 0x01, 0x5AE, 0x0F0F, 0x5A0, 0x02,
			      0x5A4, 0x01, 0x5A1, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO7, 0x5C0, 0x01, 0x5CE, 0x0F07, 0x5C0, 0x02,
			      0x5C4, 0x01, 0x5C1, 0x04, 0, 0,
			      ldo_vout_ranges2, 0x0710, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO8, 0x600, 0x01, 0x60E, 0x0F0F, 0x600, 0x02,
			      0x604, 0x01, 0x601, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO9, 0x620, 0x01, 0x62E, 0x0F0F, 0x620, 0x02,
			      0x624, 0x01, 0x621, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO10, 0x223, 0x10, 0x238, 0x7F, 0x229, 0x10,
			      0x644, 0x01, 0x229, 0x10, 0, 0,
			      ldo_vout_ranges8, 0x80, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(LDO11, 0x660, 0x01, 0x66E, 0x0F0F, 0x660, 0x02,
			      0x664, 0x01, 0x661, 0x04, 0, 0,
			      ldo_vout_ranges3, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO12, 0x680, 0x01, 0x68E, 0x0F0F, 0x680, 0x02,
			      0x684, 0x01, 0x681, 0x04, 0, 0,
			      ldo_vout_ranges3, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO13, 0x6A0, 0x01, 0x6AE, 0x0F0F, 0x6A0, 0x02,
			      0x6A4, 0x01, 0x6A1, 0x04, 0, 0,
			      ldo_vout_ranges1, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO14, 0x6C0, 0x01, 0x6CE, 0x0F0F, 0x6C0, 0x02,
			      0x6C4, 0x01, 0x6C1, 0x04, 0, 0,
			      ldo_vout_ranges4, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO15, 0x6E0, 0x01, 0x6EE, 0x0F0F, 0x6E0, 0x02,
			      0x6E4, 0x01, 0x6E1, 0x04, 0, 0,
			      ldo_vout_ranges4, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO16, 0x700, 0x01, 0x70E, 0x0F0F, 0x700, 0x02,
			      0x704, 0x01, 0x701, 0x04, 0, 0,
			      ldo_vout_ranges3, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO17, 0x720, 0x01, 0x72E, 0x0F0F, 0x720, 0x02,
			      0x724, 0x01, 0x721, 0x04, 0, 0,
			      ldo_vout_ranges3, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO18, 0x223, 0x08, 0x239, 0x7F, 0x229, 0x08,
			      0x744, 0x01, 0, 0, 0, 0,
			      ldo_vout_ranges8, 0x80, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(LDO19, 0x223, 0x04, 0x23A, 0x7F, 0x229, 0x04,
			      0x764, 0x01, 0, 0, 0, 0,
			      ldo_vout_ranges8, 0x80, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(LDO21, 0x223, 0x01, 0x23C, 0x7F, 0x229, 0x01,
			      0x7A4, 0x01, 0, 0, 0, 0,
			      ldo_vout_ranges8, 0x80, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(LDO22, 0x223, 0x02, 0x23B, 0x7F, 0x229, 0x02,
			      0x7C4, 0x01, 0, 0, 0, 0,
			      ldo_vout_ranges8, 0x80, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(LDO23, 0x223, 0x20, 0x7FE, 0x7F, 0x229, 0x20,
			      0x7E4, 0x01, 0, 0, 0, 0,
			      ldo_vout_ranges9, 0x80, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(LDO24, 0x800, 0x01, 0x80E, 0x0F0F, 0x800, 0x02,
			      0x804, 0x01, 0x801, 0x04, 0, 0,
			      ldo_vout_ranges5, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO25, 0x820, 0x01, 0x82E, 0x0F0F, 0x820, 0x02,
			      0x824, 0x01, 0x821, 0x04, 0, 0,
			      ldo_vout_ranges6, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO26, 0x840, 0x01, 0x84E, 0x0F0F, 0x840, 0x02,
			      0x844, 0x01, 0x841, 0x04, 0, 0,
			      ldo_vout_ranges6, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO27, 0x860, 0x01, 0x86E, 0x0F0F, 0x860, 0x02,
			      0x864, 0x01, 0x861, 0x04, 0, 0,
			      ldo_vout_ranges6, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO28, 0x880, 0x01, 0x88E, 0x0F0F, 0x880, 0x02,
			      0x884, 0x01, 0x881, 0x04, 0, 0,
			      ldo_vout_ranges6, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO29, 0x8A0, 0x01, 0x8AE, 0x0F0F, 0x8A0, 0x02,
			      0x8A4, 0x01, 0x8A1, 0x04, 0, 0,
			      ldo_vout_ranges6, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(LDO30, 0x8C0, 0x01, 0x8CE, 0x0F0F, 0x8C0, 0x02,
			      0x8C4, 0x01, 0x8C1, 0x04, 0, 0,
			      ldo_vout_ranges7, 0x0F10, 0, mt6361_volt_tbl_ops),
	MT6361_REGULATOR_DESC(BUCK1, 0x220, 0x01, 0x237, 0xFF, 0x226, 0x01,
			      0x311, 0x01, 0, 0, 0x32C, 0x01,
			      buck_vout_ranges1, 0x100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK2, 0x220, 0x02, 0x236, 0xFF, 0x226, 0x02,
			      0x351, 0x01, 0, 0, 0x36C, 0x01,
			      buck_vout_ranges1, 0x100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK3, 0x220, 0x04, 0x235, 0xFF, 0x226, 0x04,
			      0x391, 0x01, 0, 0, 0x3AC, 0x01,
			      buck_vout_ranges2, 0x100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK4, 0x220, 0x08, 0x234, 0xFF, 0x226, 0x08,
			      0x3D1, 0x01, 0, 0, 0x3EC, 0x01,
			      buck_vout_ranges2, 0x100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK5, 0x220, 0x10, 0x233, 0xFF, 0x226, 0x10,
			      0x411, 0x01, 0, 0, 0x421, 0x01,
			      buck_vout_ranges2, 0x100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK6, 0x220, 0x20, 0x232, 0xFF, 0x226, 0x20,
			      0x441, 0x01, 0, 0, 0x451, 0x01,
			      buck_vout_ranges2, 0x0100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK7, 0x220, 0x40, 0x231, 0xFF, 0x226, 0x40,
			      0x471, 0x01, 0, 0, 0x481, 0x01,
			      buck_vout_ranges2, 0x100, 0, mt6361_volt_tbl_ops2),
	MT6361_REGULATOR_DESC(BUCK8, 0x220, 0x80, 0x230, 0xFF, 0x226, 0x80,
			      0x4A1, 0x01, 0, 0, 0x4B1, 0x01,
			      buck_vout_ranges2, 0x100, 0, mt6361_volt_tbl_ops2),
};

#define MAX_NAME_SIZE	20
static int mt6361_regulator_irq_register(struct platform_device *pdev,
					 struct regulator_dev *rdev,
					 const struct mt6361_regulator_desc *rdesc)
{
	struct device_node *np = pdev->dev.of_node, *child;
	const struct regulator_desc *desc = rdev->desc;
	int irq, ret;
	char *irq_name;

	child = of_get_child_by_name(np, desc->name);
	if (!child)
		return -ENODEV;

	irq = of_irq_get_byname(child, "pgb");
	if (irq < 0)
		return irq;

	irq_name = devm_kzalloc(&pdev->dev, sizeof(char) * MAX_NAME_SIZE,
				GFP_KERNEL);
	if (!irq_name)
		return -ENOMEM;

	ret = scnprintf(irq_name, MAX_NAME_SIZE, "%s_PGB\n", desc->name);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to set %s pgb irq name\n", desc->name);
		return ret;
	}

	ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
					&mt6361_pgb_event_handler, 0,
					irq_name, rdev);
	if (ret) {
		dev_err(&pdev->dev, "Fail to request %s irq\n", irq_name);
		return ret;
	}

	irq = of_irq_get_byname(child, "oc");
	if (irq < 0)
		return irq;

	irq_name = devm_kzalloc(&pdev->dev, sizeof(char) * MAX_NAME_SIZE,
				GFP_KERNEL);
	if (!irq_name)
		return -ENOMEM;

	ret = scnprintf(irq_name, MAX_NAME_SIZE, "%s_OC\n", desc->name);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to set %s oc irq name\n", desc->name);
		return ret;
	}

	ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
					&mt6361_oc_event_handler, 0,
					irq_name, rdev);
	if (ret) {
		dev_err(&pdev->dev, "Fail to request %s irq\n", irq_name);
		return ret;
	}
	return 0;
}

/* VMDD2H */
static struct mt6361_regulator_desc mt6361_ldo14_e2_rdesc =
	MT6361_REGULATOR_DESC(LDO14_E2, 0x6C0, 0x01, 0x6CE, 0x0F0F, 0x6C0, 0x02,
			      0x6C4, 0x01, 0x6C1, 0x04, 0, 0,
			      ldo_vout_ranges11, 0x0F10, 0, mt6361_volt_tbl_ops);
/* VMDD2L */
static struct mt6361_regulator_desc mt6361_ldo15_e2_rdesc =
	MT6361_REGULATOR_DESC(LDO15_E2, 0x6E0, 0x01, 0x6EE, 0x0F0F, 0x6E0, 0x02,
			      0x6E4, 0x01, 0x6E1, 0x04, 0, 0,
			      ldo_vout_ranges11, 0x0F10, 0, mt6361_volt_tbl_ops);
/* VRF09 */
static struct mt6361_regulator_desc mt6361_ldo16_e2_rdesc =
MT6361_REGULATOR_DESC(LDO16_E2, 0x700, 0x01, 0x70E, 0x0F0F, 0x700, 0x02,
		      0x704, 0x01, 0x701, 0x04, 0, 0,
		      ldo_vout_ranges12, 0x0F10, 0, mt6361_volt_tbl_ops);
/* VRF09AIF */
static struct mt6361_regulator_desc mt6361_ldo17_e2_rdesc =
MT6361_REGULATOR_DESC(LDO17_E2, 0x720, 0x01, 0x72E, 0x0F0F, 0x720, 0x02,
		      0x724, 0x01, 0x721, 0x04, 0, 0,
		      ldo_vout_ranges12, 0x0F10, 0, mt6361_volt_tbl_ops);
/* VMDDQ */
static struct mt6361_regulator_desc mt6361_ldo23_e2_rdesc =
	MT6361_REGULATOR_DESC(LDO23_E2, 0x223, 0x20, 0x7FE, 0x7F, 0x229, 0x20,
			      0x7E4, 0x01, 0, 0, 0, 0,
			      ldo_vout_ranges10, 0x80, 0, mt6361_volt_tbl_ops2);

#define MT6361_CHIP_REV_E1 0x01
#define MT6361_CHIP_REV_E2 0x02
static int mt6361_check_voltage_table(struct mt6361_regulator_data *rdata,
				      struct mt6361_regulator_desc *rdesc)
{
	if (rdata->chip_rev < MT6361_CHIP_REV_E2)
		return 0;

	switch (rdesc->desc.id) {
	case MT6361_REGULATOR_LDO14: /* VMDD2H */
		dev_info(rdata->dev, "%s: replace ldo14 e2 rdesc\n", __func__);
		*rdesc = mt6361_ldo14_e2_rdesc;
		break;
	case MT6361_REGULATOR_LDO15: /* VMDD2L */
		dev_info(rdata->dev, "%s: replace ldo15 e2 rdesc\n", __func__);
		*rdesc = mt6361_ldo15_e2_rdesc;
		break;
	case MT6361_REGULATOR_LDO16: /* VRF09 */
		dev_info(rdata->dev, "%s: replace ldo16 e2 rdesc\n", __func__);
		*rdesc = mt6361_ldo16_e2_rdesc;
		break;
	case MT6361_REGULATOR_LDO17: /* VRF09AIF */
		dev_info(rdata->dev, "%s: replace ldo17 e2 rdesc\n", __func__);
		*rdesc = mt6361_ldo17_e2_rdesc;
		break;
	case MT6361_REGULATOR_LDO23: /* VMDDQ */
		dev_info(rdata->dev, "%s: replace ldo23 e2 rdesc\n", __func__);
		*rdesc = mt6361_ldo23_e2_rdesc;
		break;
	}
	return 0;
}

static int mt6361_regulator_probe(struct platform_device *pdev)
{
	struct mt6361_regulator_data *rdata;
	struct regulator_config config = {};
	int i, ret;
	u32 regval = 0;

	dev_info(&pdev->dev, "%s\n", __func__);
	rdata = devm_kzalloc(&pdev->dev, sizeof(struct mt6361_regulator_data),
			     GFP_KERNEL);
	if (!rdata)
		return -ENOMEM;

	rdata->dev = &pdev->dev;
	platform_set_drvdata(pdev, rdata);

	rdata->rmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!rdata->rmap) {
		dev_err(&pdev->dev, "failed to get regmap\n");
		return -ENODEV;
	}

	ret = regmap_read(rdata->rmap, MT6361_TOP_INFO1, &regval);
	if (ret < 0)
		return ret;
	rdata->chip_rev = regval &= MT6361_MSK_CHIP_REV;

	config.dev = &pdev->dev;
	config.driver_data = rdata;
	config.regmap = rdata->rmap;

	for (i = 0; i < ARRAY_SIZE(mt6361_regulator_descs); i++) {
		struct mt6361_regulator_desc *rdesc = mt6361_regulator_descs + i;
		struct regulator_dev *rdev;

		ret = mt6361_check_voltage_table(rdata, rdesc);

		rdev = devm_regulator_register(&pdev->dev, &rdesc->desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s regulator(%d)\n",
				rdesc->desc.name, PTR_ERR(rdev));
			continue;
		}

		ret = mt6361_regulator_irq_register(pdev, rdev, rdesc);
		if (ret) {
			dev_err(&pdev->dev, "failed to register %s regulator irqs\n",
				rdesc->desc.name);
			return ret;
		}
	}

	dev_info(&pdev->dev, "%s: successfully\n", __func__);
	return 0;
}

static const struct of_device_id mt6361_regulator_of_match[] = {
	{ .compatible = "mediatek,mt6361-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, mt6361_regulator_of_match);

static struct platform_driver mt6361_regulator_driver = {
	.driver = {
		.name = "mt6361-regulator",
		.of_match_table = of_match_ptr(mt6361_regulator_of_match),
	},
	.probe = mt6361_regulator_probe,
};
module_platform_driver(mt6361_regulator_driver);

MODULE_AUTHOR("Gene Chen <gene_chen@richtek.com>");
MODULE_DESCRIPTION("mt6361 Regulator Driver");
MODULE_LICENSE("GPL v2");
