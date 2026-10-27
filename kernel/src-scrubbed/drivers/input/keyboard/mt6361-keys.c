// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: Gene Chen <gene_chen@richtek.com>
 */

#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/pm_wakeup.h>

#define MT6361_TOP_INFO1		0x00
#define MT6361_TOP_STAT1		0xC0
#define PMIC_RESETB_SP_CTRL		0x202
#define PMIC_RESETB_LP_CTRL		0x203
#define PWRKEY_SP_CTRL			0x204
#define PWRKEY_LP_CTRL			0x205
#define MT6361_WDTRSTB_CTRL		0x20C

#define PMIC_RESETB_SP_TSEL		GENMASK(7, 6)
#define PMIC_RESETB_SP_MODE		GENMASK(5, 4)
#define PMIC_RESETB_LP_TSEL		GENMASK(6, 4)
#define PMIC_RESETB_LP_MODE		GENMASK(2, 0)
#define PMIC_PWRKEY_SP_TSEL		GENMASK(7, 6)
#define PMIC_PWRKEY_SP_MODE		GENMASK(5, 4)
#define PMIC_PWRKEY_LP_TSEL		GENMASK(6, 4)
#define PMIC_PWRKEY_LP_MODE		GENMASK(2, 0)

#define MT6361_MSK_CHIP_REV		GENMASK(3, 0)
#define MT6361_STAT_PWRKEY_MASK		BIT(4)
#define RSV_RESETB_LP_FLAG_MASK		BIT(0)

#define MT6361_CHIP_REV_E2		0x02

struct mt6361_keys {
	struct device *dev;
	struct regmap *rmap;
	struct input_dev *input;
	struct wakeup_source *ws;
	struct work_struct pwrkey_poll_work;
	int chip_rev;
};

struct mt6361_keys_platform_data {
	u32 resetb_sp_tsel;
	u32 resetb_sp_mode;
	u32 resetb_lp_tsel;
	u32 resetb_lp_mode;
	u32 pwrkey_sp_tsel;
	u32 pwrkey_sp_mode;
	u32 pwrkey_lp_tsel;
	u32 pwrkey_lp_mode;
};

static const struct mt6361_keys_platform_data mt6361_keys_pdata_def = {
	.resetb_sp_tsel = 0,
	.resetb_sp_mode = 0,
	.resetb_lp_tsel = 2,
	.resetb_lp_mode = 1,
	.pwrkey_sp_tsel = 0,
	.pwrkey_sp_mode = 0,
	.pwrkey_lp_tsel = 2,
	.pwrkey_lp_mode = 1,
};

struct mt6361_keys_dtprop {
	const char *name;
	u32 addr;
	u32 mask;
	size_t offset;
};

#define MT6361_KEYS_DTPROP(_name, _addr, _mask) \
{ \
	.name = #_name, \
	.addr = _addr, \
	.mask = _mask, \
	.offset = offsetof(struct mt6361_keys_platform_data, _name), \
}

static const struct mt6361_keys_dtprop mt6361_keys_dtprops[] = {
	MT6361_KEYS_DTPROP(resetb_sp_tsel, PMIC_RESETB_SP_CTRL, PMIC_RESETB_SP_TSEL),
	MT6361_KEYS_DTPROP(resetb_sp_mode, PMIC_RESETB_SP_CTRL, PMIC_RESETB_SP_MODE),
	MT6361_KEYS_DTPROP(resetb_lp_tsel, PMIC_RESETB_LP_CTRL, PMIC_RESETB_LP_TSEL),
	MT6361_KEYS_DTPROP(resetb_lp_mode, PMIC_RESETB_LP_CTRL, PMIC_RESETB_LP_MODE),
	MT6361_KEYS_DTPROP(pwrkey_sp_tsel, PWRKEY_SP_CTRL, PMIC_PWRKEY_SP_TSEL),
	MT6361_KEYS_DTPROP(pwrkey_sp_mode, PWRKEY_SP_CTRL, PMIC_PWRKEY_SP_MODE),
	MT6361_KEYS_DTPROP(pwrkey_lp_tsel, PWRKEY_LP_CTRL, PMIC_PWRKEY_LP_TSEL),
	MT6361_KEYS_DTPROP(pwrkey_lp_mode, PWRKEY_LP_CTRL, PMIC_PWRKEY_LP_MODE),
};

static void mt6361_pwrkey_poll_work(struct work_struct *work)
{
	struct mt6361_keys *keys = container_of(work, struct mt6361_keys,
						pwrkey_poll_work);
	int ret, val;

	while (true) {
		ret = regmap_read(keys->rmap, MT6361_TOP_STAT1, &val);
		if (ret < 0) {
			dev_err(keys->dev, "failed to read pwrkey stat\n");
			return;
		}
		if (!(val & MT6361_STAT_PWRKEY_MASK))
			dev_info(keys->dev, "pwrkey not release\n");
		else
			break;
		msleep(100); //TODO: test
	}

	dev_info(keys->dev, "pwrkey release\n");
	input_report_key(keys->input, KEY_POWER, false);
	input_sync(keys->input);
	__pm_relax(keys->ws);
}

static irqreturn_t mt6361_pwrkey_handler(int irq, void *data)
{
	struct mt6361_keys *keys = data;
	int ret, val;
	bool pressed;

	ret = regmap_read(keys->rmap, MT6361_TOP_STAT1, &val);
	if (ret < 0)
		return ret;
	pressed = (val & MT6361_STAT_PWRKEY_MASK) ? false : true;

	dev_info(keys->dev, "press = %d\n", pressed);
	if (pressed) {
		__pm_stay_awake(keys->ws);
		input_report_key(keys->input, KEY_POWER, true);
		input_sync(keys->input);

		if (keys->chip_rev < MT6361_CHIP_REV_E2)
			schedule_work(&keys->pwrkey_poll_work);
	} else {
		dev_info(keys->dev, "press pwrkey not active or released\n");
		input_report_key(keys->input, KEY_POWER, false);
		input_sync(keys->input);
		__pm_relax(keys->ws);
	}

	return IRQ_HANDLED;
}

#define MT6361_KEYS_IRQ(_name) \
{ \
	.name = #_name, \
	.hdlr = mt6361_##_name##_handler, \
}

static irqreturn_t mt6361_pwrkey_lp_handler(int irq, void *data)
{
	struct mt6361_keys *keys = data;

	dev_info(keys->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_pwrkey_sp_handler(int irq, void *data)
{
	struct mt6361_keys *keys = data;

	dev_info(keys->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_pmic_resetb_lp_handler(int irq, void *data)
{
	struct mt6361_keys *keys = data;

	dev_info(keys->dev, "%s\n", __func__);
	input_report_key(keys->input, KEY_1, true);
	input_sync(keys->input);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_pmic_resetb_sp_handler(int irq, void *data)
{
	struct mt6361_keys *keys = data;

	dev_info(keys->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static int mt6361_keys_init_irq(struct mt6361_keys *keys)
{
	int i, ret;
	const struct {
		char *name;
		irq_handler_t hdlr;
	} mt6361_keys_irqs[] = {
		MT6361_KEYS_IRQ(pwrkey),
		MT6361_KEYS_IRQ(pwrkey_lp),
		MT6361_KEYS_IRQ(pwrkey_sp),
		MT6361_KEYS_IRQ(pmic_resetb_lp),
		MT6361_KEYS_IRQ(pmic_resetb_sp),
	};

	for (i = 0; i < ARRAY_SIZE(mt6361_keys_irqs); i++) {
		ret = platform_get_irq_byname(to_platform_device(keys->dev),
					      mt6361_keys_irqs[i].name);
		if (ret < 0) {
			dev_err(keys->dev, "failed to get irq %s\n",
				mt6361_keys_irqs[i].name);
			return ret;
		}
		ret = devm_request_threaded_irq(keys->dev, ret, NULL,
						mt6361_keys_irqs[i].hdlr,
						IRQF_ONESHOT,
						mt6361_keys_irqs[i].name, keys);
		if (ret) {
			dev_err(keys->dev, "failed to request irq %s\n",
				mt6361_keys_irqs[i].name);
			return ret;
		}
	}
	return 0;
}

static int mt6361_keys_init_setting(struct mt6361_keys *keys)
{
	struct mt6361_keys_platform_data *pdata;
	const struct mt6361_keys_dtprop *dp;
	int i, ret, shift;
	void *val;

	pdata = devm_kzalloc(keys->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	memcpy(pdata, &mt6361_keys_pdata_def, sizeof(*pdata));

	for (i = 0; i < ARRAY_SIZE(mt6361_keys_dtprops); i++) {
		dp = &mt6361_keys_dtprops[i];
		val = (void *)pdata + dp->offset;
		shift = ffs(dp->mask) - 1;
		ret = device_property_read_u32(keys->dev, dp->name, val);
		if (ret < 0)
			dev_info(keys->dev,
				 "property %s not found\n", dp->name);

		ret = regmap_update_bits(keys->rmap, dp->addr, dp->mask,
					 *(u32 *)val << shift);
		if (ret) {
			dev_err(keys->dev, "failed to write dtprop %s\n",
				dp->name);
			return ret;
		}
	}
	keys->dev->platform_data = pdata;
	return ret;
}

#define MAX_NAME_SIZE	100
static int mt6361_keys_probe(struct platform_device *pdev)
{
	struct mt6361_keys *keys;
	struct input_dev *input;
	char name[100] = "\0";
	int ret;
	u32 regval;

	dev_info(&pdev->dev, "%s\n", __func__);
	keys = devm_kzalloc(&pdev->dev, sizeof(*keys), GFP_KERNEL);
	if (!keys)
		return -ENOMEM;

	keys->dev = &pdev->dev;
	platform_set_drvdata(pdev, keys);

	keys->rmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!keys->rmap) {
		dev_err(&pdev->dev, "failed to get regmap\n");
		return -ENODEV;
	}

	ret = mt6361_keys_init_setting(keys);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to set inital setting\n");
		return ret;
	}

	input = devm_input_allocate_device(keys->dev);
	if (!input) {
		dev_err(&pdev->dev, "failed to allocate input\n");
		return -ENOMEM;
	}

	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x6361;
	input->id.version = 0x0001;

	input_set_capability(input, EV_KEY, KEY_POWER);
	keys->input = input;

	ret = input_register_device(input);
	if (ret) {
		dev_err(&pdev->dev, "failed to register input device\n");
		return ret;
	}

	ret = scnprintf(name, MAX_NAME_SIZE, "%s-pwrkey-ws", dev_name(keys->dev));
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to set pwrkey ws name\n");
		return ret;
	}
	keys->ws = wakeup_source_register(keys->dev, name);
	if (!keys->ws) {
		dev_err(&pdev->dev, "failed to create pwrkey ws\n");
		return ret;
	}

	/* TODO: check long press reset at last time */
	ret = regmap_read(keys->rmap, MT6361_WDTRSTB_CTRL, &regval);
	if (ret < 0)
		return ret;
	if (regval & RSV_RESETB_LP_FLAG_MASK) {
		dev_info(&pdev->dev, "%s: resetb long press trigger\n", __func__);
		input_report_key(keys->input, KEY_1, true);
		input_sync(keys->input);
	}

	ret = regmap_read(keys->rmap, MT6361_TOP_INFO1, &regval);
	if (ret < 0)
		return ret;
	keys->chip_rev = regval & MT6361_MSK_CHIP_REV;

	INIT_WORK(&keys->pwrkey_poll_work, mt6361_pwrkey_poll_work);

	ret = mt6361_keys_init_irq(keys);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to init irq\n");
		return ret;
	}

	dev_info(&pdev->dev, "%s: successfully\n", __func__);
	return 0;
}

static int mt6361_keys_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mt6361_keys_of_match[] = {
	{ .compatible = "mediatek,mt6361-keys", },
	{ },
};
MODULE_DEVICE_TABLE(of, mt6361_keys_of_match);

static struct platform_driver mt6361_keys_driver = {
	.driver = {
		.name = "mt6361-keys",
		.of_match_table = of_match_ptr(mt6361_keys_of_match),
	},
	.probe = mt6361_keys_probe,
	.remove = mt6361_keys_remove,
};
module_platform_driver(mt6361_keys_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Gene Chen <v_genech@mediatek.com>");
MODULE_DESCRIPTION("MT6361 POWERKEY Driver");
