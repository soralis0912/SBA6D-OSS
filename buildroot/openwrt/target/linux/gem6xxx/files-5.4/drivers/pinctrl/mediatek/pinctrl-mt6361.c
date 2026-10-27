// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: ChiYuan Huang <cy_huang@richtek.com>
 */

#include <dt-bindings/pinctrl/pinctrl-mt6361.h>
#include <linux/bitops.h>
#include <linux/gpio/driver.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include "../pinctrl-utils.h"
#include "../core.h"

#define MT6361_LVSF_DINA_REG		0x185
#define MT6361_LVSF_DINB_REG		0x186
#define MT6361_LVSF_HIZ_REG		0x187
#define MT6361_LVSF_MODE_REG		0x19B
#define MT6361_LVSF_POWER_REG		0x19C
#define MT6361_LVSF_DOUTA_REG		0x1A9
#define MT6361_LVSF_DOUTB_REG		0x1AA

#define MT6361_LVSF_RPU_MASK		GENMASK(7, 4)
#define MT6361_LVSF_RPD_MASK		GENMASK(1, 0)
#define MT6361_LVSF_DATA_MASK(_no)	BIT((_no) % 6)
#define MT6361_LVSF123_HIZ_MASK		(GENMASK(2, 0) | GENMASK(10, 8))
#define MT6361_LVSF4_HIZ_MASK		(BIT(3) | BIT(11))
#define MT6361_LVSF5_HIZ_MASK		(BIT(4) | BIT(12))
#define MT6361_LVSF6_HIZ_MASK		(BIT(5) | BIT(13))
#define MT6361_LVSF123_POWER_MASK	BIT(0)
#define MT6361_LVSF4_POWER_MASK		BIT(1)
#define MT6361_LVSF5_POWER_MASK		BIT(2)
#define MT6361_LVSF6_POWER_MASK		BIT(3)
#define MT6361_LVSF123_MODE_MASK	GENMASK(2, 0)
#define MT6361_LVSF4_MODE_MASK		BIT(3)
#define MT6361_LVSF5_MODE_MASK		BIT(4)
#define MT6361_LVSF6_MODE_MASK		BIT(5)

#define MT6361_PART_PINS		6
#define MT6361_NUM_PINS			12

struct mt6361_pctrl_pin {
	unsigned int pull_reg;
	unsigned int in_reg;
	unsigned int out_reg;
	unsigned int q1_reg;
	unsigned int q3_reg;
	unsigned int qmos_mask;
};

struct mt6361_pctrl_grp {
	const char *name;
	const unsigned int *pins;
	size_t npins;
	unsigned int hiz_mask;
	unsigned int power_mask;
	unsigned int mode_mask;
};

struct mt6361_pctrl_func {
	const char *name;
	const char * const *groups;
	size_t ngroups;
};

struct mt6361_priv {
	struct device *dev;
	struct regmap *regmap;
	struct pinctrl_desc pctrl_desc;
	struct gpio_chip gc;
};

static const struct pinctrl_pin_desc mt6361_pins[] = {
	PINCTRL_PIN(MT6361_PIN_LVSFA1, "A1"),
	PINCTRL_PIN(MT6361_PIN_LVSFA2, "A2"),
	PINCTRL_PIN(MT6361_PIN_LVSFA3, "A3"),
	PINCTRL_PIN(MT6361_PIN_LVSFA4, "A4"),
	PINCTRL_PIN(MT6361_PIN_LVSFA5, "A5"),
	PINCTRL_PIN(MT6361_PIN_LVSFA6, "A6"),
	PINCTRL_PIN(MT6361_PIN_LVSFB1, "B1"),
	PINCTRL_PIN(MT6361_PIN_LVSFB2, "B2"),
	PINCTRL_PIN(MT6361_PIN_LVSFB3, "B3"),
	PINCTRL_PIN(MT6361_PIN_LVSFB4, "B4"),
	PINCTRL_PIN(MT6361_PIN_LVSFB5, "B5"),
	PINCTRL_PIN(MT6361_PIN_LVSFB6, "B6")
};

#define MT6361_PINCTRL_PIN(_part, _pullrg, _q1rg, _q3rg, _qmask) \
{ \
	.in_reg = MT6361_LVSF_DIN##_part##_REG, \
	.out_reg = MT6361_LVSF_DOUT##_part##_REG, \
	.pull_reg = _pullrg, \
	.q1_reg = _q1rg, \
	.q3_reg = _q3rg, \
	.qmos_mask = _qmask, \
}

static const struct mt6361_pctrl_pin mt6361_pctrl_pins[] = {
	MT6361_PINCTRL_PIN(A, 0x18f, 0x1a2, 0x19e, 0x03),
	MT6361_PINCTRL_PIN(A, 0x18e, 0x1a2, 0x19e, 0x0c),
	MT6361_PINCTRL_PIN(A, 0x18d, 0x1a1, 0x19d, 0x03),
	MT6361_PINCTRL_PIN(A, 0x18c, 0x1a1, 0x19d, 0x0c),
	MT6361_PINCTRL_PIN(A, 0x18b, 0x1a1, 0x19d, 0x30),
	MT6361_PINCTRL_PIN(A, 0x18a, 0x1a1, 0x19d, 0xc0),
	MT6361_PINCTRL_PIN(B, 0x197, 0x1a8, 0x1a4, 0x03),
	MT6361_PINCTRL_PIN(B, 0x196, 0x1a8, 0x1a4, 0x0c),
	MT6361_PINCTRL_PIN(B, 0x195, 0x1a7, 0x1a3, 0x03),
	MT6361_PINCTRL_PIN(B, 0x194, 0x1a7, 0x1a3, 0x0c),
	MT6361_PINCTRL_PIN(B, 0x193, 0x1a7, 0x1a3, 0x30),
	MT6361_PINCTRL_PIN(B, 0x192, 0x1a7, 0x1a3, 0xc0)
};

static const unsigned int lvsf123_pins[] = { 0, 1, 2, 6, 7, 8 };
static const unsigned int lvsf4_pins[] = { 3, 9 };
static const unsigned int lvsf5_pins[] = { 4, 10 };
static const unsigned int lvsf6_pins[] = { 5, 11 };

#define MT6361_PINCTRL_GRP(_name, _no) \
{ \
	.name = #_name "_grp", \
	.pins = _name##_pins, \
	.npins = ARRAY_SIZE(_name##_pins), \
	.power_mask = MT6361_LVSF##_no##_POWER_MASK, \
	.hiz_mask = MT6361_LVSF##_no##_HIZ_MASK, \
	.mode_mask = MT6361_LVSF##_no##_MODE_MASK, \
}

static const struct mt6361_pctrl_grp mt6361_pctrl_grps[] = {
	MT6361_PINCTRL_GRP(lvsf123, 123),
	MT6361_PINCTRL_GRP(lvsf4, 4),
	MT6361_PINCTRL_GRP(lvsf5, 5),
	MT6361_PINCTRL_GRP(lvsf6, 6)
};

static const char * const lvsf_grps[] = {
	"lvsf123_grp", "lvsf4_grp", "lvsf5_grp", "lvsf6_grp"
};

#define MT6361_PCTRL_FUNC(_name) \
{ \
	.name = #_name, \
	.groups = lvsf_grps, \
	.ngroups = ARRAY_SIZE(lvsf_grps), \
}

static const struct mt6361_pctrl_func mt6361_pctrl_funcs[] = {
	MT6361_PCTRL_FUNC(lvsf),
	MT6361_PCTRL_FUNC(gpio)
};

static int mt6361_read16(struct mt6361_priv *priv, unsigned int reg, u16 *val)
{
	__le16 le16_val;
	int ret;

	ret = regmap_raw_read(priv->regmap, reg, &le16_val, sizeof(le16_val));
	if (ret)
		return ret;

	*val = le16_to_cpu(le16_val);
	return 0;
}

static int mt6361_write16(struct mt6361_priv *priv, unsigned int reg, u16 val)
{
	__le16 le16_val;

	le16_val = cpu_to_le16(val);
	return regmap_raw_write(priv->regmap, reg, &le16_val, sizeof(le16_val));
}

static int mt6361_pmux_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(mt6361_pctrl_funcs);
}

static const char * mt6361_pmux_get_function_name(struct pinctrl_dev *pctldev,
						  unsigned selector)
{
	return mt6361_pctrl_funcs[selector].name;
}

static int mt6361_pmux_get_function_groups(struct pinctrl_dev *pctldev,
					   unsigned selector,
					   const char * const **groups,
					   unsigned *num_groups)
{
	*groups = mt6361_pctrl_funcs[selector].groups;
	*num_groups = mt6361_pctrl_funcs[selector].ngroups;
	return 0;
}

static int mt6361_pmux_set_mux(struct pinctrl_dev *pctldev,
			       unsigned int function,
			       unsigned int group)
{
	struct mt6361_priv *priv = pinctrl_dev_get_drvdata(pctldev);
	const struct mt6361_pctrl_grp *grpcfg =  mt6361_pctrl_grps + group;
	const struct mt6361_pctrl_pin *pin;
	unsigned int mode_val, regval;
	u16 hiz_val;
	int i, ret;

	mode_val = function ? grpcfg->mode_mask : 0;

	ret = regmap_read(priv->regmap, MT6361_LVSF_MODE_REG, &regval);
	if (ret)
		return ret;

	if ((regval & grpcfg->mode_mask) == mode_val) {
		dev_dbg(priv->dev, "No change\n");
		return 0;
	}

	ret = mt6361_read16(priv, MT6361_LVSF_HIZ_REG, &hiz_val);
	if (ret)
		return ret;

	hiz_val |= grpcfg->hiz_mask;
	ret = mt6361_write16(priv, MT6361_LVSF_HIZ_REG, hiz_val);
	if (ret)
		return ret;

	/*
	 * HW limitation:
	 * Config RPU/RPD/Q1/Q3 all to zero
	 */
	for (i = 0; i < grpcfg->npins; i++) {
		pin = mt6361_pctrl_pins + grpcfg->pins[i];

		ret = regmap_write(priv->regmap, pin->pull_reg, 0);
		ret |= regmap_update_bits(priv->regmap, pin->q1_reg,
					  pin->qmos_mask, 0);
		ret |= regmap_update_bits(priv->regmap, pin->q3_reg,
					  pin->qmos_mask, 0);
		if (ret)
			return ret;
	}

	ret = regmap_update_bits(priv->regmap, MT6361_LVSF_MODE_REG,
				 grpcfg->mode_mask, mode_val);
	if (ret)
		return ret;

	hiz_val &= ~grpcfg->hiz_mask;
	return mt6361_write16(priv, MT6361_LVSF_HIZ_REG, hiz_val);
}

static int mt6361_pmux_gpio_request_enable(struct pinctrl_dev *pctldev,
					   struct pinctrl_gpio_range *range,
					   unsigned pin)
{
	const struct mt6361_pctrl_grp *grp;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(mt6361_pctrl_grps); i++) {
		grp = mt6361_pctrl_grps + i;

		for (j = 0; j < grp->npins; j++) {
			if (pin != grp->pins[j])
				continue;
			return mt6361_pmux_set_mux(pctldev, 1, i);
		}
	}

	return -EINVAL;
}

static int mt6361_pmux_gpio_set_direction(struct pinctrl_dev *pctldev,
					  struct pinctrl_gpio_range *range,
					  unsigned pin,
					  bool input)
{
	struct mt6361_priv *priv = pinctrl_dev_get_drvdata(pctldev);
	const struct mt6361_pctrl_pin *pin_cfg = mt6361_pctrl_pins + pin;
	unsigned int qmask = pin_cfg->qmos_mask;
	int ret;

	ret = regmap_update_bits(priv->regmap, pin_cfg->q1_reg, qmask, 0);
	ret |= regmap_update_bits(priv->regmap, pin_cfg->q3_reg, qmask, 0);
	return ret;
}

static int mt6361_pctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(mt6361_pctrl_grps);
}

static const char * mt6361_pctrl_get_group_name(struct pinctrl_dev *pctldev,
						unsigned selector)
{
	return mt6361_pctrl_grps[selector].name;
}

static int mt6361_pctrl_get_group_pins(struct pinctrl_dev *pctldev,
				       unsigned selector,const unsigned **pins,
				       unsigned *num_pins)
{
	*pins = mt6361_pctrl_grps[selector].pins;
	*num_pins = mt6361_pctrl_grps[selector].npins;
	return 0;
}

static int mt6361_pin_config_get(struct pinctrl_dev *pctldev, unsigned pin,
				 unsigned long *config)
{
	struct mt6361_priv *priv = pinctrl_dev_get_drvdata(pctldev);
	const struct mt6361_pctrl_pin *pin_cfg = mt6361_pctrl_pins + pin;
	unsigned int param = pinconf_to_config_param(*config);
	unsigned int arg = 0, pullval, outval, q1val, q3val;
	int ret;

	if (pin >= MT6361_NUM_PINS)
		return -ENOTSUPP;

	ret = regmap_read(priv->regmap, pin_cfg->pull_reg, &pullval);
	ret |= regmap_read(priv->regmap, pin_cfg->out_reg, &outval);
	ret |= regmap_read(priv->regmap, pin_cfg->q1_reg, &q1val);
	ret |= regmap_read(priv->regmap, pin_cfg->q3_reg, &q3val);
	if (ret)
		return ret;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		if ((pullval & MT6361_LVSF_RPU_MASK) ||
		    (pullval & MT6361_LVSF_RPD_MASK))
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		if (!(pullval & MT6361_LVSF_RPU_MASK))
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		if (!(pullval & MT6361_LVSF_RPD_MASK))
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		if (q1val & pin_cfg->qmos_mask ||
		    q3val & pin_cfg->qmos_mask)
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_OUTPUT:
		if (outval & MT6361_LVSF_DATA_MASK(pin))
			arg = 1;
		break;
	case PIN_CONFIG_OUTPUT_ENABLE:
		if (!(q1val & pin_cfg->qmos_mask) &&
		    !(q3val & pin_cfg->qmos_mask))
			return -EINVAL;
		arg = 1;
		break;
	default:
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);
	return 0;
}

static int mt6361_pin_config_set(struct pinctrl_dev *pctldev, unsigned pin,
				 unsigned long *configs, unsigned num_configs)
{
	struct mt6361_priv *priv = pinctrl_dev_get_drvdata(pctldev);
	const struct mt6361_pctrl_pin *pin_cfg = mt6361_pctrl_pins + pin;
	unsigned int pullval, outval, q1val = 0, q3val = 0;
	int i, ret;

	if (pin >= MT6361_NUM_PINS)
		return -ENOTSUPP;

	ret = regmap_read(priv->regmap, pin_cfg->pull_reg, &pullval);
	ret |= regmap_read(priv->regmap, pin_cfg->out_reg, &outval);
	ret |= regmap_update_bits(priv->regmap, pin_cfg->q1_reg,
				  pin_cfg->qmos_mask, 0);
	ret |= regmap_update_bits(priv->regmap, pin_cfg->q3_reg,
				  pin_cfg->qmos_mask, 0);
	if (ret)
		return ret;

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);
		unsigned int arg = pinconf_to_config_argument(configs[i]);
		unsigned int tmp;

		switch (param) {
		case PIN_CONFIG_BIAS_DISABLE:
			pullval = 0;
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			pullval &= ~MT6361_LVSF_RPU_MASK;
			tmp = arg << (ffs(MT6361_LVSF_RPU_MASK) - 1);
			pullval |= tmp & MT6361_LVSF_RPU_MASK;
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			pullval &= ~MT6361_LVSF_RPD_MASK;
			tmp = arg << (ffs(MT6361_LVSF_RPD_MASK) - 1);
			pullval |= tmp & MT6361_LVSF_RPD_MASK;
			break;
		case PIN_CONFIG_INPUT_ENABLE:
		case PIN_CONFIG_OUTPUT_ENABLE:
			break;
		case PIN_CONFIG_OUTPUT:
			if (arg) {
				q3val |= pin_cfg->qmos_mask;
				outval |= MT6361_LVSF_DATA_MASK(pin);
			} else {
				q1val |= pin_cfg->qmos_mask;
				outval &= ~MT6361_LVSF_DATA_MASK(pin);
			}
			break;
		default:
			dev_dbg(priv->dev,
				"Unsupported configuration parameter '%u'\n",
				param);
		}
	}

	ret = regmap_write(priv->regmap, pin_cfg->pull_reg, pullval);
	ret |= regmap_write(priv->regmap, pin_cfg->out_reg, outval);
	ret |= regmap_write(priv->regmap, pin_cfg->q1_reg, q1val);
	ret |= regmap_write(priv->regmap, pin_cfg->q3_reg, q3val);
	return ret;
}

static int mt6361_pin_config_group_set(struct pinctrl_dev *pctldev,
				       unsigned selector,
				       unsigned long *configs,
				       unsigned num_configs)
{
	struct mt6361_priv *priv = pinctrl_dev_get_drvdata(pctldev);
	const struct mt6361_pctrl_grp *grp = mt6361_pctrl_grps + selector;
	unsigned int pwrval, regval;
	u16 hiz_val;
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);
		unsigned int arg = pinconf_to_config_argument(configs[i]);

		if (param != PIN_CONFIG_POWER_SOURCE)
			continue;

		/* 0: VCCA 1: VCCB */
		pwrval = arg ? grp->power_mask : 0;

		ret = regmap_read(priv->regmap, MT6361_LVSF_POWER_REG, &regval);
		if (ret)
			return ret;

		if ((regval & grp->power_mask) == pwrval)
			break;

		ret = mt6361_read16(priv, MT6361_LVSF_HIZ_REG, &hiz_val);
		if (ret)
			return ret;

		hiz_val |= grp->hiz_mask;
		ret = mt6361_write16(priv, MT6361_LVSF_HIZ_REG, hiz_val);
		if (ret)
			return ret;

		ret = regmap_update_bits(priv->regmap, MT6361_LVSF_POWER_REG,
					 grp->power_mask, pwrval);
		if (ret)
			return ret;

		hiz_val &= ~grp->hiz_mask;
		ret = mt6361_write16(priv, MT6361_LVSF_HIZ_REG, hiz_val);
		if (ret)
			return ret;
	}

	return 0;
}

static int mt6361_pin_config_group_get(struct pinctrl_dev *pctldev,
				       unsigned selector,
				       unsigned long *config)
{
	struct mt6361_priv *priv = pinctrl_dev_get_drvdata(pctldev);
	const struct mt6361_pctrl_grp *grp = mt6361_pctrl_grps + selector;
	unsigned int param = pinconf_to_config_param(*config);
	unsigned int arg = 0, regval;
	int ret;

	if (param != PIN_CONFIG_POWER_SOURCE)
		return -ENOTSUPP;

	ret = regmap_read(priv->regmap, MT6361_LVSF_POWER_REG, &regval);
	if (ret)
		return ret;

	if (regval & grp->power_mask)
		arg = 1;

	*config = pinconf_to_config_packed(param, arg);
	return 0;
}

static const struct pinmux_ops mt6361_pinmux_ops = {
	.get_functions_count = mt6361_pmux_get_functions_count,
	.get_function_name = mt6361_pmux_get_function_name,
	.get_function_groups = mt6361_pmux_get_function_groups,
	.set_mux = mt6361_pmux_set_mux,
	.gpio_request_enable = mt6361_pmux_gpio_request_enable,
	.gpio_set_direction = mt6361_pmux_gpio_set_direction,
	.strict = true,
};

static const struct pinctrl_ops mt6361_pctrl_ops = {
	.get_groups_count = mt6361_pctrl_get_groups_count,
	.get_group_name = mt6361_pctrl_get_group_name,
	.get_group_pins = mt6361_pctrl_get_group_pins,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_all,
	.dt_free_map = pinctrl_utils_free_map,
};

static const struct pinconf_ops mt6361_pconf_ops = {
	.pin_config_get = mt6361_pin_config_get,
	.pin_config_set = mt6361_pin_config_set,
	.pin_config_group_get = mt6361_pin_config_group_get,
	.pin_config_group_set = mt6361_pin_config_group_set,
	.is_generic = true,
};

static int mt6361_gpio_get_value(struct gpio_chip *gc, unsigned int offs)
{
	struct mt6361_priv *priv = gpiochip_get_data(gc);
	const struct mt6361_pctrl_pin *pin_cfg = mt6361_pctrl_pins + offs;
	unsigned int val;
	int ret;

	ret = regmap_read(priv->regmap, pin_cfg->in_reg, &val);
	if (ret)
		return ret;

	return !!(val & MT6361_LVSF_DATA_MASK(offs));
}

static void mt6361_gpio_set_value(struct gpio_chip *gc, unsigned int offs,
				 int val)
{
	struct mt6361_priv *priv = gpiochip_get_data(gc);
	const struct mt6361_pctrl_pin *pin_cfg = mt6361_pctrl_pins + offs;
	unsigned int dout = val ? 0 : MT6361_LVSF_DATA_MASK(offs);

	/* The order must be followed */
	if (val) {
		regmap_update_bits(priv->regmap, pin_cfg->q1_reg,
				   pin_cfg->qmos_mask, 0);
		regmap_update_bits(priv->regmap, pin_cfg->q3_reg,
				   pin_cfg->qmos_mask, pin_cfg->qmos_mask);
	} else {
		regmap_update_bits(priv->regmap, pin_cfg->q3_reg,
				   pin_cfg->qmos_mask, 0);
		regmap_update_bits(priv->regmap, pin_cfg->q1_reg,
				   pin_cfg->qmos_mask, pin_cfg->qmos_mask);
	}

	regmap_update_bits(priv->regmap, pin_cfg->out_reg,
			   MT6361_LVSF_DATA_MASK(offs), dout);
}

static int mt6361_gpio_get_direction(struct gpio_chip *gc, unsigned int offs)
{
	struct mt6361_priv *priv = gpiochip_get_data(gc);
	const struct mt6361_pctrl_pin *pin_cfg = mt6361_pctrl_pins + offs;
	unsigned int q1_val, q3_val;
	int ret;

	ret = regmap_read(priv->regmap, pin_cfg->q1_reg, &q1_val);
	ret |= regmap_read(priv->regmap, pin_cfg->q3_reg, &q3_val);
	if (ret)
		return ret;

	if (!(q1_val & pin_cfg->qmos_mask) && !(q3_val & pin_cfg->qmos_mask))
		return 1;

	return 0;
}

static int mt6361_gpio_direction_input(struct gpio_chip *gc, unsigned int offs)
{
	return pinctrl_gpio_direction_input(gc->base + offs);
}

static int mt6361_gpio_direction_output(struct gpio_chip *gc, unsigned int offs,
					int val)
{
	int ret;

	ret = pinctrl_gpio_direction_output(gc->base + offs);
	if (ret)
		return ret;

	mt6361_gpio_set_value(gc, offs, val);
	return 0;

}

static int mt6361_pinctrl_probe(struct platform_device *pdev)
{
	struct mt6361_priv *priv;
	struct pinctrl_dev *pctrl_dev;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;

	priv->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!priv->regmap) {
		dev_err(&pdev->dev, "Failed to get regmap\n");
		return -ENODEV;
	}

	priv->pctrl_desc.name = dev_name(&pdev->dev);
	priv->pctrl_desc.owner = THIS_MODULE,
	priv->pctrl_desc.pins = mt6361_pins,
	priv->pctrl_desc.npins = ARRAY_SIZE(mt6361_pins),
	priv->pctrl_desc.pctlops = &mt6361_pctrl_ops,
	priv->pctrl_desc.confops = &mt6361_pconf_ops,
	priv->pctrl_desc.pmxops = &mt6361_pinmux_ops,

	pctrl_dev = devm_pinctrl_register(&pdev->dev, &priv->pctrl_desc, priv);
	if (IS_ERR(pctrl_dev)) {
		ret = PTR_ERR(pctrl_dev);
		dev_err(&pdev->dev, "Failed to register pinctrl (%d)\n", ret);
		return ret;
	}

	priv->gc.label = dev_name(&pdev->dev);
	priv->gc.parent = &pdev->dev;
	priv->gc.owner = THIS_MODULE;
	priv->gc.base = -1;
	priv->gc.ngpio = MT6361_NUM_PINS;
	priv->gc.request = gpiochip_generic_request;
	priv->gc.free = gpiochip_generic_free;
	priv->gc.set_config = gpiochip_generic_config;
	priv->gc.get_direction = mt6361_gpio_get_direction;
	priv->gc.direction_input = mt6361_gpio_direction_input;
	priv->gc.direction_output = mt6361_gpio_direction_output;
	priv->gc.get = mt6361_gpio_get_value;
	priv->gc.set = mt6361_gpio_set_value;

	return devm_gpiochip_add_data(&pdev->dev, &priv->gc, priv);
}

static const struct of_device_id __maybe_unused mt6361_pinctrl_dev_tables[] = {
	{ .compatible = "mediatek,mt6361-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, mt6361_pinctrl_dev_tables);

static struct platform_driver mt6361_pinctrl_driver = {
	.driver = {
		.name = "mt6361-pinctrl",
		.of_match_table = mt6361_pinctrl_dev_tables,
	},
	.probe = mt6361_pinctrl_probe,
};
module_platform_driver(mt6361_pinctrl_driver);

MODULE_AUTHOR("ChiYuan Huang <cy_huang@richtek.com>");
MODULE_DESCRIPTION("Mediatek MT6361 pinctrl Driver");
MODULE_LICENSE("GPL v2");
