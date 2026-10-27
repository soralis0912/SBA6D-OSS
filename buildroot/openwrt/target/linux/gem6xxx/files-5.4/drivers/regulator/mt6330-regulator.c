// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/mfd/mt6330/registers.h>
#include <linux/mfd/mt6330/core.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/mt6330-regulator.h>
#include <linux/regulator/of_regulator.h>

#define MT6330_BUCK_MODE_AUTO		0
#define MT6330_BUCK_MODE_FORCE_PWM	1
#define MT6330_BUCK_MODE_NORMAL		0
#define MT6330_BUCK_MODE_LP		2

/*
 * MT6330 regulators' information
 *
 * @desc: standard fields of regulator description.
 * @da_reg: for query status of regulators.
 * @qi: Mask for query enable signal status of regulators.
 * @modeset_reg: for operating AUTO/PWM mode register.
 * @modeset_mask: MASK for operating modeset register.
 * @modeset_shift: SHIFT for operating modeset register.
 */
struct mt6330_regulator_info {
	struct regulator_desc desc;
	u32 da_vsel_reg;
	u8 da_vsel_mask;
	u8 da_vsel_shift;
	u32 da_reg;
	u8 qi;
	u32 modeset_reg;
	u8 modeset_mask;
	u8 modeset_shift;
	u32 lp_mode_reg;
	u8 lp_mode_mask;
	u8 lp_mode_shift;
};

/*
 * MTK regulators' init data
 *
 * @id: chip id
 * @size: num of regulators
 * @regulator_info: regulator info.
 */
struct mt_regulator_init_data {
	u32 id;
	u32 size;
	struct mt6330_regulator_info *regulator_info;
};

#define MT_BUCK(match, _name, min, max, step,				\
		min_sel, volt_ranges, _enable_reg,			\
		_da_reg,						\
		_da_vsel_reg, _da_vsel_mask, _da_vsel_shift,		\
		_vsel_reg, _vsel_mask,					\
		_lp_mode_reg, _lp_mode_shift,			\
		_modeset_reg, _modeset_shift)				\
[MT6330_ID_##_name] = {							\
	.desc = {							\
		.name = #_name,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6330_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6330_ID_##_name,				\
		.owner = THIS_MODULE,					\
		.uV_step = (step),					\
		.linear_min_sel = (min_sel),				\
		.n_voltages = ((max) - (min)) / (step) + 1,		\
		.min_uV = (min),					\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
		.vsel_reg = _vsel_reg,					\
		.vsel_mask = _vsel_mask,				\
		.enable_reg = _enable_reg,				\
		.enable_mask = BIT(0),					\
		.of_map_mode = mt6330_map_mode,				\
	},								\
	.da_vsel_reg = _da_vsel_reg,					\
	.da_vsel_mask = _da_vsel_mask,					\
	.da_vsel_shift = _da_vsel_shift,				\
	.da_reg = _da_reg,						\
	.qi = BIT(0),							\
	.lp_mode_reg = _lp_mode_reg,					\
	.lp_mode_mask = BIT(_lp_mode_shift),				\
	.lp_mode_shift = _lp_mode_shift,				\
	.modeset_reg = _modeset_reg,					\
	.modeset_mask = BIT(_modeset_shift),				\
	.modeset_shift = _modeset_shift					\
}

#define MT_LDO_REGULAR(match, _name, min, max, step,			\
		       min_sel, volt_ranges, _enable_reg,		\
		       _da_reg,						\
		       _da_vsel_reg, _da_vsel_mask, _da_vsel_shift,	\
		       _vsel_reg, _vsel_mask)				\
[MT6330_ID_##_name] = {							\
	.desc = {							\
		.name = #_name,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6330_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6330_ID_##_name,				\
		.owner = THIS_MODULE,					\
		.uV_step = (step),					\
		.linear_min_sel = (min_sel),				\
		.n_voltages = ((max) - (min)) / (step) + 1,		\
		.min_uV = (min),					\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
		.vsel_reg = _vsel_reg,					\
		.vsel_mask = _vsel_mask,				\
		.enable_reg = _enable_reg,				\
		.enable_mask = BIT(0),					\
	},								\
	.da_vsel_reg = _da_vsel_reg,					\
	.da_vsel_mask = _da_vsel_mask,					\
	.da_vsel_shift = _da_vsel_shift,				\
	.da_reg = _da_reg,						\
	.qi = BIT(0),							\
}

#define MT_LDO_NON_REGULAR(match, _name,				\
			   _volt_table, _enable_reg, _enable_mask,	\
			   _da_reg,					\
			   _vsel_reg, _vsel_mask)			\
[MT6330_ID_##_name] = {							\
	.desc = {							\
		.name = #_name,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6330_volt_table_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6330_ID_##_name,				\
		.owner = THIS_MODULE,					\
		.n_voltages = ARRAY_SIZE(_volt_table),			\
		.volt_table = _volt_table,				\
		.vsel_reg = _vsel_reg,					\
		.vsel_mask = _vsel_mask,				\
		.enable_reg = _enable_reg,				\
		.enable_mask = BIT(_enable_mask),			\
	},								\
	.da_reg = _da_reg,						\
	.qi = BIT(0),							\
}

#define MT_REG_FIXED(match, _name, _enable_reg,				\
		     _da_reg,						\
		     _fixed_volt)					\
[MT6330_ID_##_name] = {							\
	.desc = {							\
		.name = #_name,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6330_volt_fixed_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6330_ID_##_name,				\
		.owner = THIS_MODULE,					\
		.n_voltages = 1,					\
		.enable_reg = _enable_reg,				\
		.enable_mask = BIT(0),					\
		.fixed_uV = (_fixed_volt),				\
	},								\
	.da_reg = _da_reg,						\
	.qi = BIT(0),							\
}

//vs1
static const struct regulator_linear_range mt_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(800000, 0, 0x70, 12500),
};
//vs2
static const struct regulator_linear_range mt_volt_range2[] = {
	REGULATOR_LINEAR_RANGE(800000, 0, 0x40, 12500),
};
//vs3/vcore/vmd11/vmd12/vsram_md/vrfdig
static const struct regulator_linear_range mt_volt_range3[] = {
	REGULATOR_LINEAR_RANGE(400000, 0, 0x7f, 6250),
};
//vsram_proc/vsram_core/vsram_rfdig
static const struct regulator_linear_range mt_volt_range4[] = {
	REGULATOR_LINEAR_RANGE(500000, 0, 0x7f, 6250),
};

static const u32 vrfck_voltages[] = {
	1240000,
	1600000,
};

static const u32 vsim1_voltages[] = {
	0,
	0,
	0,
	1700000,
	1800000,
	0,
	0,
	0,
	0,
	0,
	2900000,
	3000000,
};

static const u32 vio18_2_voltages[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	1600000,
	1700000,
	1800000,
};

static const u32 vmdd2_voltages[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	1000000,
	1050000,
	1100000,
	1150000,
};

static const u32 vrf13_voltages[] = {
	0,
	0,
	1100000,
	1200000,
	1300000,
};

static const u32 vxo22_voltages[] = {
	1800000,
	0,
	0,
	0,
	2200000,
};

static const u32 vefuse_voltages[] = {
	0,
	0,
	0,
	1700000,
	1800000,
	2000000,
};

static const u32 vio18_1_voltages[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	1600000,
	1700000,
	1800000,
};

static const u32 va12_2_voltages[] = {
	0,
	0,
	1100000,
	1200000,
	1300000,
};

static const u32 vemc_voltages[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	2800000,
	2900000,
	3000000,
	0,
	3300000,
};

static const u32 vmddr_voltages[] = {
	0,
	0,
	0,
	700000,
	750000,
	800000,
};

static const u32 vrf18_voltages[] = {
	0,
	0,
	0,
	0,
	0,
	1700000,
	1800000,
};

static const u32 vmddq_voltages[] = {
	550000,
	600000,
	650000,
};

static const u32 vmc_voltages[] = {
	0,
	0,
	0,
	0,
	1800000,
	0,
	0,
	0,
	0,
	0,
	2900000,
	3000000,
	0,
	3300000,
};

static const u32 vsim2_voltages[] = {
	0,
	0,
	0,
	1700000,
	1800000,
	0,
	0,
	0,
	0,
	0,
	2900000,
	3000000,
};

static const u32 vusb_voltages[] = {
	3000000,
};

static const u32 vcn18_voltages[] = {
	1800000,
};

static const u32 vaux18_voltages[] = {
	1800000,
};

static const u32 vrf09_voltages[] = {
	900000,
};

static const u32 vbbck_voltages[] = {
	1200000,
};

static const u32 va12_1_voltages[] = {
	1200000,
};

static int mt6330_regulator_disable(struct regulator_dev *rdev)
{
	int ret = 0;

	if (rdev->use_count == 0) {
		dev_notice(&rdev->dev, "%s:%s should not be disable.(use_count=0)\n"
			   , __func__, rdev->desc->name);
		ret = -EIO;
	} else
		ret = regulator_disable_regmap(rdev);

	return ret;
}


static inline unsigned int mt6330_map_mode(unsigned int mode)
{
	switch (mode) {
	case MT6330_BUCK_MODE_NORMAL:
		return REGULATOR_MODE_NORMAL;
	case MT6330_BUCK_MODE_FORCE_PWM:
		return REGULATOR_MODE_FAST;
	case MT6330_BUCK_MODE_LP:
		return REGULATOR_MODE_IDLE;
	default:
		return REGULATOR_MODE_INVALID;
	}
}

static int mt6330_regulator_get_voltage_sel(struct regulator_dev *rdev)
{
	struct mt6330_regulator_info *info = rdev_get_drvdata(rdev);
	int ret = 0, regval = 0;

	ret = regmap_read(rdev->regmap, info->da_vsel_reg, &regval);
	if (ret != 0) {
		dev_notice(&rdev->dev,
			"Failed to get mt6330 regulator voltage: %d\n", ret);
		return ret;
	}

	ret = (regval >> info->da_vsel_shift) & info->da_vsel_mask;

	return ret;
}

static unsigned int mt6330_regulator_get_mode(struct regulator_dev *rdev)
{
	struct mt6330_regulator_info *info = rdev_get_drvdata(rdev);
	int ret = 0, regval = 0;

	ret = regmap_read(rdev->regmap, info->modeset_reg, &regval);
	if (ret != 0) {
		dev_notice(&rdev->dev,
			"Failed to get mt6330 buck mode: %d\n", ret);
		return ret;
	}

	if ((regval & info->modeset_mask) >> info->modeset_shift ==
		MT6330_BUCK_MODE_FORCE_PWM)
		return REGULATOR_MODE_FAST;


	ret = regmap_read(rdev->regmap, info->lp_mode_reg, &regval);
	if (ret != 0) {
		dev_notice(&rdev->dev,
			"Failed to get mt6330 buck lp mode: %d\n", ret);
		return ret;
	}

	if (regval & info->lp_mode_mask)
		return REGULATOR_MODE_IDLE;
	else
		return REGULATOR_MODE_NORMAL;
}

static int mt6330_regulator_set_mode(struct regulator_dev *rdev,
				     unsigned int mode)
{
	struct mt6330_regulator_info *info = rdev_get_drvdata(rdev);
	int ret = 0, val = 0;
	int curr_mode;

	curr_mode = mt6330_regulator_get_mode(rdev);
	switch (mode) {
	case REGULATOR_MODE_FAST:
		if (curr_mode == REGULATOR_MODE_IDLE) {
			WARN_ON(1);
			dev_notice(&rdev->dev,
				   "BUCK %s is LP mode, can't FPWM\n",
				   rdev->desc->name);
			return -EIO;
		}
		val = MT6330_BUCK_MODE_FORCE_PWM;
		val <<= info->modeset_shift;
		ret = regmap_update_bits(rdev->regmap,
					 info->modeset_reg,
					 info->modeset_mask,
					 val);
		break;
	case REGULATOR_MODE_NORMAL:
		if (curr_mode == REGULATOR_MODE_FAST) {
			val = MT6330_BUCK_MODE_AUTO;
			val <<= info->modeset_shift;
			ret = regmap_update_bits(rdev->regmap,
						 info->modeset_reg,
						 info->modeset_mask,
						 val);
		} else if (curr_mode == REGULATOR_MODE_IDLE) {
			val = MT6330_BUCK_MODE_NORMAL;
			val <<= info->lp_mode_shift;
			ret = regmap_update_bits(rdev->regmap,
						 info->lp_mode_reg,
						 info->lp_mode_mask,
						 val);
			udelay(100);
		}
		break;
	case REGULATOR_MODE_IDLE:
		if (curr_mode == REGULATOR_MODE_FAST) {
			WARN_ON(1);
			dev_notice(&rdev->dev,
				   "BUCK %s is FPWM mode, can't enter LP\n",
				   rdev->desc->name);
			return -EIO;
		}
		val = MT6330_BUCK_MODE_LP >> 1;
		val <<= info->lp_mode_shift;
		ret = regmap_update_bits(rdev->regmap,
					 info->lp_mode_reg,
					 info->lp_mode_mask,
					 val);
		break;
	default:
		ret = -EINVAL;
		goto err_mode;
	}

err_mode:
	if (ret != 0) {
		dev_notice(&rdev->dev,
			"Failed to set mt6330 buck mode: %d\n", ret);
		return ret;
	}

	return 0;
}

static int mt6330_get_status(struct regulator_dev *rdev)
{
	int ret = 0;
	u32 regval = 0;
	struct mt6330_regulator_info *info = rdev_get_drvdata(rdev);

	ret = regmap_read(rdev->regmap, info->da_reg, &regval);
	if (ret != 0) {
		dev_notice(&rdev->dev, "Failed to get enable reg: %d\n", ret);
		return ret;
	}

	return (regval & info->qi) ? REGULATOR_STATUS_ON : REGULATOR_STATUS_OFF;
}

static const struct regulator_ops mt6330_volt_range_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.map_voltage = regulator_map_voltage_linear_range,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = mt6330_regulator_get_voltage_sel,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.enable = regulator_enable_regmap,
	.disable = mt6330_regulator_disable,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt6330_get_status,
	.set_mode = mt6330_regulator_set_mode,
	.get_mode = mt6330_regulator_get_mode,
};

static const struct regulator_ops mt6330_volt_table_ops = {
	.list_voltage = regulator_list_voltage_table,
	.map_voltage = regulator_map_voltage_iterate,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.enable = regulator_enable_regmap,
	.disable = mt6330_regulator_disable,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt6330_get_status,
};

static const struct regulator_ops mt6330_volt_fixed_ops = {
	.enable = regulator_enable_regmap,
	.disable = mt6330_regulator_disable,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt6330_get_status,
};

/* The array is indexed by id(MT6330_ID_XXX) */
static struct mt6330_regulator_info mt6330_regulators[] = {
	MT_BUCK("buck_vs1", VS1, 800000, 2200000, 12500,
		0, mt_volt_range1, MT6330_RG_BUCK_VS1_EN_ADDR,
		MT6330_DA_VS1_EN_ADDR,
		MT6330_DA_VS1_VOSEL_ADDR,
		MT6330_DA_VS1_VOSEL_MASK,
		MT6330_DA_VS1_VOSEL_SHIFT,
		MT6330_RG_BUCK_VS1_VOSEL_ADDR,
		MT6330_RG_BUCK_VS1_VOSEL_MASK <<
		MT6330_RG_BUCK_VS1_VOSEL_SHIFT,
		MT6330_RG_BUCK_VS1_LP_ADDR, MT6330_RG_BUCK_VS1_LP_SHIFT,
		MT6330_RG_VS1_FCCM_ADDR, MT6330_RG_VS1_FCCM_SHIFT),
	MT_BUCK("buck_vmd12", VMD12, 400000, 1193750, 6250,
		0, mt_volt_range3, MT6330_RG_BUCK_VMD12_EN_ADDR,
		MT6330_DA_VMD12_EN_ADDR,
		MT6330_DA_VMD12_VOSEL_ADDR,
		MT6330_DA_VMD12_VOSEL_MASK,
		MT6330_DA_VMD12_VOSEL_SHIFT,
		MT6330_RG_BUCK_VMD12_VOSEL_ADDR,
		MT6330_RG_BUCK_VMD12_VOSEL_MASK <<
		MT6330_RG_BUCK_VMD12_VOSEL_SHIFT,
		MT6330_RG_BUCK_VMD12_LP_ADDR, MT6330_RG_BUCK_VMD12_LP_SHIFT,
		MT6330_RG_VMD12_FCCM_ADDR, MT6330_RG_VMD12_FCCM_SHIFT),
	MT_BUCK("buck_vrfdig", VRFDIG, 400000, 1193750, 6250,
		0, mt_volt_range3, MT6330_RG_BUCK_VRFDIG_EN_ADDR,
		MT6330_DA_VRFDIG_EN_ADDR,
		MT6330_DA_VRFDIG_VOSEL_ADDR,
		MT6330_DA_VRFDIG_VOSEL_MASK,
		MT6330_DA_VRFDIG_VOSEL_SHIFT,
		MT6330_RG_BUCK_VRFDIG_VOSEL_ADDR,
		MT6330_RG_BUCK_VRFDIG_VOSEL_MASK <<
		MT6330_RG_BUCK_VRFDIG_VOSEL_SHIFT,
		MT6330_RG_BUCK_VRFDIG_LP_ADDR, MT6330_RG_BUCK_VRFDIG_LP_SHIFT,
		MT6330_RG_VRFDIG_FCCM_ADDR, MT6330_RG_VRFDIG_FCCM_SHIFT),
	MT_BUCK("buck_vcore", VCORE, 400000, 1193750, 6250,
		0, mt_volt_range3, MT6330_RG_BUCK_VCORE_EN_ADDR,
		MT6330_DA_VCORE_EN_ADDR,
		MT6330_DA_VCORE_VOSEL_ADDR,
		MT6330_DA_VCORE_VOSEL_MASK,
		MT6330_DA_VCORE_VOSEL_SHIFT,
		MT6330_RG_BUCK_VCORE_VOSEL_ADDR,
		MT6330_RG_BUCK_VCORE_VOSEL_MASK <<
		MT6330_RG_BUCK_VCORE_VOSEL_SHIFT,
		MT6330_RG_BUCK_VCORE_LP_ADDR, MT6330_RG_BUCK_VCORE_LP_SHIFT,
		MT6330_RG_VCORE_FCCM_ADDR, MT6330_RG_VCORE_FCCM_SHIFT),
	MT_BUCK("buck_vs2", VS2, 800000, 1600000, 12500,
		0, mt_volt_range2, MT6330_RG_BUCK_VS2_EN_ADDR,
		MT6330_DA_VS2_EN_ADDR,
		MT6330_DA_VS2_VOSEL_ADDR,
		MT6330_DA_VS2_VOSEL_MASK,
		MT6330_DA_VS2_VOSEL_SHIFT,
		MT6330_RG_BUCK_VS2_VOSEL_ADDR,
		MT6330_RG_BUCK_VS2_VOSEL_MASK <<
		MT6330_RG_BUCK_VS2_VOSEL_SHIFT,
		MT6330_RG_BUCK_VS2_LP_ADDR, MT6330_RG_BUCK_VS2_LP_SHIFT,
		MT6330_RG_VS2_FCCM_ADDR, MT6330_RG_VS2_FCCM_SHIFT),
	MT_BUCK("buck_vmd11", VMD11, 400000, 1193750, 6250,
		0, mt_volt_range3, MT6330_RG_BUCK_VMD11_EN_ADDR,
		MT6330_DA_VMD11_EN_ADDR,
		MT6330_DA_VMD11_VOSEL_ADDR,
		MT6330_DA_VMD11_VOSEL_MASK,
		MT6330_DA_VMD11_VOSEL_SHIFT,
		MT6330_RG_BUCK_VMD11_VOSEL_ADDR,
		MT6330_RG_BUCK_VMD11_VOSEL_MASK <<
		MT6330_RG_BUCK_VMD11_VOSEL_SHIFT,
		MT6330_RG_BUCK_VMD11_LP_ADDR, MT6330_RG_BUCK_VMD11_LP_SHIFT,
		MT6330_RG_VMD11_FCCM_ADDR, MT6330_RG_VMD11_FCCM_SHIFT),
	MT_BUCK("buck_vsram_md", VSRAM_MD, 400000, 1193750, 6250,
		0, mt_volt_range3, MT6330_RG_BUCK_VSRAM_MD_EN_ADDR,
		MT6330_DA_VSRAM_MD_EN_ADDR,
		MT6330_DA_VSRAM_MD_VOSEL_ADDR,
		MT6330_DA_VSRAM_MD_VOSEL_MASK,
		MT6330_DA_VSRAM_MD_VOSEL_SHIFT,
		MT6330_RG_BUCK_VSRAM_MD_VOSEL_ADDR,
		MT6330_RG_BUCK_VSRAM_MD_VOSEL_MASK <<
		MT6330_RG_BUCK_VSRAM_MD_VOSEL_SHIFT,
		MT6330_RG_BUCK_VSRAM_MD_LP_ADDR, MT6330_RG_BUCK_VSRAM_MD_LP_SHIFT,
		MT6330_RG_VSRAM_MD_FCCM_ADDR, MT6330_RG_VSRAM_MD_FCCM_SHIFT),
	MT_BUCK("buck_vs3", VS3, 400000, 1193750, 6250,
		0, mt_volt_range3, MT6330_RG_BUCK_VS3_EN_ADDR,
		MT6330_DA_VS3_EN_ADDR,
		MT6330_DA_VS3_VOSEL_ADDR,
		MT6330_DA_VS3_VOSEL_MASK,
		MT6330_DA_VS3_VOSEL_SHIFT,
		MT6330_RG_BUCK_VS3_VOSEL_ADDR,
		MT6330_RG_BUCK_VS3_VOSEL_MASK <<
		MT6330_RG_BUCK_VS3_VOSEL_SHIFT,
		MT6330_RG_BUCK_VS3_LP_ADDR, MT6330_RG_BUCK_VS3_LP_SHIFT,
		MT6330_RG_VS3_FCCM_ADDR, MT6330_RG_VS3_FCCM_SHIFT),
	MT_REG_FIXED("ldo_vusb", VUSB, MT6330_RG_LDO_VUSB_EN_ADDR,
		MT6330_DA_VUSB_B_EN_ADDR, 3000000),
	MT_REG_FIXED("ldo_vcn18", VCN18, MT6330_RG_LDO_VCN18_EN_ADDR,
		MT6330_DA_VCN18_B_EN_ADDR, 1800000),
	MT_REG_FIXED("ldo_vaux18", VAUX18, MT6330_RG_LDO_VAUX18_EN_ADDR,
		MT6330_DA_VAUX18_B_EN_ADDR, 1800000),
	MT_REG_FIXED("ldo_vrf09", VRF09, MT6330_RG_LDO_VRF09_EN_ADDR,
		MT6330_DA_VRF09_B_EN_ADDR, 900000),
	MT_REG_FIXED("ldo_vbbck", VBBCK, MT6330_RG_LDO_VBBCK_EN_ADDR,
		MT6330_DA_VBBCK_B_EN_ADDR, 1200000),
	MT_REG_FIXED("ldo_va12_1", VA12_1, MT6330_RG_LDO_VA12_1_EN_ADDR,
		MT6330_DA_VA12_1_B_EN_ADDR, 1200000),
	MT_LDO_NON_REGULAR("ldo_vrfck", VRFCK,
		vrfck_voltages,
		MT6330_RG_LDO_VRFCK_EN_ADDR,
		MT6330_RG_LDO_VRFCK_EN_SHIFT,
		MT6330_DA_VRFCK_B_EN_ADDR,
		MT6330_RG_VRFCK_VOSEL_ADDR,
		MT6330_RG_VRFCK_VOSEL_MASK <<
		MT6330_RG_VRFCK_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vsim1", VSIM1,
		vsim1_voltages,
		MT6330_RG_LDO_VSIM1_EN_ADDR,
		MT6330_RG_LDO_VSIM1_EN_SHIFT,
		MT6330_DA_VSIM1_B_EN_ADDR,
		MT6330_RG_VSIM1_VOSEL_ADDR,
		MT6330_RG_VSIM1_VOSEL_MASK <<
		MT6330_RG_VSIM1_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vio18_2", VIO18_2,
		vio18_2_voltages,
		MT6330_RG_LDO_VIO18_2_EN_ADDR,
		MT6330_RG_LDO_VIO18_2_EN_SHIFT,
		MT6330_DA_VIO18_2_B_EN_ADDR,
		MT6330_RG_VIO18_2_VOSEL_ADDR,
		MT6330_RG_VIO18_2_VOSEL_MASK <<
		MT6330_RG_VIO18_2_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vmdd2", VMDD2,
		vmdd2_voltages,
		MT6330_RG_LDO_VMDD2_EN_ADDR,
		MT6330_RG_LDO_VMDD2_EN_SHIFT,
		MT6330_DA_VMDD2_B_EN_ADDR,
		MT6330_RG_VMDD2_VOSEL_ADDR,
		MT6330_RG_VMDD2_VOSEL_MASK <<
		MT6330_RG_VMDD2_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vrf13", VRF13,
		vrf13_voltages,
		MT6330_RG_LDO_VRF13_EN_ADDR,
		MT6330_RG_LDO_VRF13_EN_SHIFT,
		MT6330_DA_VRF13_B_EN_ADDR,
		MT6330_RG_VRF13_VOSEL_ADDR,
		MT6330_RG_VRF13_VOSEL_MASK <<
		MT6330_RG_VRF13_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vxo22", VXO22,
		vxo22_voltages,
		MT6330_RG_LDO_VXO22_EN_ADDR,
		MT6330_RG_LDO_VXO22_EN_SHIFT,
		MT6330_DA_VXO22_B_EN_ADDR,
		MT6330_RG_VXO22_VOSEL_ADDR,
		MT6330_RG_VXO22_VOSEL_MASK <<
		MT6330_RG_VXO22_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vefuse", VEFUSE,
		vefuse_voltages,
		MT6330_RG_LDO_VEFUSE_EN_ADDR,
		MT6330_RG_LDO_VEFUSE_EN_SHIFT,
		MT6330_DA_VEFUSE_B_EN_ADDR,
		MT6330_RG_VEFUSE_VOSEL_ADDR,
		MT6330_RG_VEFUSE_VOSEL_MASK <<
		MT6330_RG_VEFUSE_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vio18_1", VIO18_1,
		vio18_1_voltages,
		MT6330_RG_LDO_VIO18_1_EN_ADDR,
		MT6330_RG_LDO_VIO18_1_EN_SHIFT,
		MT6330_DA_VIO18_1_B_EN_ADDR,
		MT6330_RG_VIO18_1_VOSEL_ADDR,
		MT6330_RG_VIO18_1_VOSEL_MASK <<
		MT6330_RG_VIO18_1_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_va12_2", VA12_2,
		va12_2_voltages,
		MT6330_RG_LDO_VA12_2_EN_ADDR,
		MT6330_RG_LDO_VA12_2_EN_SHIFT,
		MT6330_DA_VA12_2_B_EN_ADDR,
		MT6330_RG_VA12_2_VOSEL_ADDR,
		MT6330_RG_VA12_2_VOSEL_MASK <<
		MT6330_RG_VA12_2_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vemc", VEMC,
		vemc_voltages,
		MT6330_RG_LDO_VEMC_EN_ADDR,
		MT6330_RG_LDO_VEMC_EN_SHIFT,
		MT6330_DA_VEMC_B_EN_ADDR,
		MT6330_RG_VEMC_VOSEL_ADDR,
		MT6330_RG_VEMC_VOSEL_MASK <<
		MT6330_RG_VEMC_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vmddr", VMDDR,
		vmddr_voltages,
		MT6330_RG_LDO_VMDDR_EN_ADDR,
		MT6330_RG_LDO_VMDDR_EN_SHIFT,
		MT6330_DA_VMDDR_B_EN_ADDR,
		MT6330_RG_VMDDR_VOSEL_ADDR,
		MT6330_RG_VMDDR_VOSEL_MASK <<
		MT6330_RG_VMDDR_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vrf18", VRF18,
		vrf18_voltages,
		MT6330_RG_LDO_VRF18_EN_ADDR,
		MT6330_RG_LDO_VRF18_EN_SHIFT,
		MT6330_DA_VRF18_B_EN_ADDR,
		MT6330_RG_VRF18_VOSEL_ADDR,
		MT6330_RG_VRF18_VOSEL_MASK <<
		MT6330_RG_VRF18_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vmddq", VMDDQ,
		vmddq_voltages,
		MT6330_RG_LDO_VMDDQ_EN_ADDR,
		MT6330_RG_LDO_VMDDQ_EN_SHIFT,
		MT6330_DA_VMDDQ_B_EN_ADDR,
		MT6330_RG_VMDDQ_VOSEL_ADDR,
		MT6330_RG_VMDDQ_VOSEL_MASK <<
		MT6330_RG_VMDDQ_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vmc", VMC,
		vmc_voltages,
		MT6330_RG_LDO_VMC_EN_ADDR,
		MT6330_RG_LDO_VMC_EN_SHIFT,
		MT6330_DA_VMC_B_EN_ADDR,
		MT6330_RG_VMC_VOSEL_ADDR,
		MT6330_RG_VMC_VOSEL_MASK <<
		MT6330_RG_VMC_VOSEL_SHIFT),
	MT_LDO_NON_REGULAR("ldo_vsim2", VSIM2,
		vsim2_voltages,
		MT6330_RG_LDO_VSIM2_EN_ADDR,
		MT6330_RG_LDO_VSIM2_EN_SHIFT,
		MT6330_DA_VSIM2_B_EN_ADDR,
		MT6330_RG_VSIM2_VOSEL_ADDR,
		MT6330_RG_VSIM2_VOSEL_MASK <<
		MT6330_RG_VSIM2_VOSEL_SHIFT),
	MT_LDO_REGULAR("ldo_vsram_core", VSRAM_CORE, 500000, 1293750, 6250,
		0, mt_volt_range4, MT6330_RG_LDO_VSRAM_CORE_EN_ADDR,
		MT6330_DA_VSRAM_CORE_B_EN_ADDR,
		MT6330_DA_VSRAM_CORE_VOSEL_ADDR,
		MT6330_DA_VSRAM_CORE_VOSEL_MASK,
		MT6330_DA_VSRAM_CORE_VOSEL_SHIFT,
		MT6330_RG_LDO_VSRAM_CORE_VOSEL_ADDR,
		MT6330_RG_LDO_VSRAM_CORE_VOSEL_MASK <<
		MT6330_RG_LDO_VSRAM_CORE_VOSEL_SHIFT),
	MT_LDO_REGULAR("ldo_vsram_proc", VSRAM_PROC, 500000, 1293750, 6250,
		0, mt_volt_range4, MT6330_RG_LDO_VSRAM_PROC_EN_ADDR,
		MT6330_DA_VSRAM_PROC_B_EN_ADDR,
		MT6330_DA_VSRAM_PROC_VOSEL_ADDR,
		MT6330_DA_VSRAM_PROC_VOSEL_MASK,
		MT6330_DA_VSRAM_PROC_VOSEL_SHIFT,
		MT6330_RG_LDO_VSRAM_PROC_VOSEL_ADDR,
		MT6330_RG_LDO_VSRAM_PROC_VOSEL_MASK <<
		MT6330_RG_LDO_VSRAM_PROC_VOSEL_SHIFT),
	MT_LDO_REGULAR("ldo_vsram_rfdig", VSRAM_RFDIG, 500000, 1293750, 6250,
		0, mt_volt_range4, MT6330_RG_LDO_VSRAM_RFDIG_EN_ADDR,
		MT6330_DA_VSRAM_RFDIG_B_EN_ADDR,
		MT6330_DA_VSRAM_RFDIG_VOSEL_ADDR,
		MT6330_DA_VSRAM_RFDIG_VOSEL_MASK,
		MT6330_DA_VSRAM_RFDIG_VOSEL_SHIFT,
		MT6330_RG_LDO_VSRAM_RFDIG_VOSEL_ADDR,
		MT6330_RG_LDO_VSRAM_RFDIG_VOSEL_MASK <<
		MT6330_RG_LDO_VSRAM_RFDIG_VOSEL_SHIFT),
};

static const struct mt_regulator_init_data mt6330_regulator_init_data = {
	.id = MT6330_SWCID0,
	.size = MT6330_MAX_REGULATOR,
	.regulator_info = &mt6330_regulators[0],
};

static const struct platform_device_id mt6330_platform_ids[] = {
	{"mt6330-regulator", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, mt6330_platform_ids);

static const struct of_device_id mt6330_of_match[] = {
	{
		.compatible = "mediatek,mt6330-regulator",
		.data = &mt6330_regulator_init_data,
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, mt6330_of_match);

static int mt6330_regulator_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct mt6330_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct mt6330_regulator_info *mt_regulators;
	struct mt_regulator_init_data *regulator_init_data;
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	int i;
	u32 reg_value_l = 0, reg_value_h = 0;

	of_id = of_match_device(mt6330_of_match, &pdev->dev);
	if (!of_id || !of_id->data)
		return -ENODEV;
	regulator_init_data = (struct mt_regulator_init_data *)of_id->data;
	mt_regulators = regulator_init_data->regulator_info;

	/* Read PMIC chip revision to update constraints and voltage table */
	if (regmap_read(chip->regmap, MT6330_SWCID0, &reg_value_l) < 0) {
		dev_notice(&pdev->dev, "Failed to read Chip ID\n");
		return -EIO;
	}
	if (regmap_read(chip->regmap, MT6330_SWCID1, &reg_value_h) < 0) {
		dev_notice(&pdev->dev, "Failed to read Chip ID\n");
		return -EIO;
	}
	dev_info(&pdev->dev, "Chip ID = 0x%x%x\n", reg_value_h, reg_value_l);

	for (i = 0; i < regulator_init_data->size; i++) {
		config.dev = &pdev->dev;
		config.driver_data = (mt_regulators + i);
		config.regmap = chip->regmap;
		rdev = devm_regulator_register(&pdev->dev,
				&(mt_regulators + i)->desc, &config);
		if (IS_ERR(rdev)) {
			dev_notice(&pdev->dev, "failed to register %s\n",
				(mt_regulators + i)->desc.name);
			return PTR_ERR(rdev);
		}
	}

	return 0;
}

static struct platform_driver mt6330_regulator_driver = {
	.driver = {
		.name = "mt6330-regulator",
		.of_match_table = of_match_ptr(mt6330_of_match),
	},
	.probe = mt6330_regulator_probe,
	.id_table = mt6330_platform_ids,
};

module_platform_driver(mt6330_regulator_driver);

MODULE_AUTHOR("Benjamin Chao <benjamin.chao@mediatek.com>");
MODULE_DESCRIPTION("Regulator Driver for MediaTek MT6330 PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mt6330-regulator");
