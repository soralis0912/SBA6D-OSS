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
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/ledtrig-mediatek.h>

#define MT6361_REG_ISINK_TOP_CKPDN_CON0	(0x133)

#define MT6361_REG_ISINK1_CON0		(0x140)
#define MT6361_REG_ISINK1_CON1		(0x141)
#define MT6361_REG_ISINK1_CON2		(0x142)
#define MT6361_REG_ISINK1_CON3		(0x143)
#define MT6361_REG_ISINK1_CON4		(0x144)
#define MT6361_REG_ISINK1_CON5		(0x145)
#define MT6361_REG_ISINK2_CON0		(0x146)
#define MT6361_REG_ISINK2_CON1		(0x147)
#define MT6361_REG_ISINK2_CON2		(0x148)
#define MT6361_REG_ISINK2_CON3		(0x149)
#define MT6361_REG_ISINK2_CON4		(0x14A)
#define MT6361_REG_ISINK2_CON5		(0x14B)
#define MT6361_REG_ISINK3_CON0		(0x14C)
#define MT6361_REG_ISINK3_CON1		(0x14D)
#define MT6361_REG_ISINK3_CON2		(0x14E)
#define MT6361_REG_ISINK3_CON3		(0x14F)
#define MT6361_REG_ISINK3_CON4		(0x150)
#define MT6361_REG_ISINK3_CON5		(0x151)
#define MT6361_REG_ISINK_SFSTR0		(0x156)
#define MT6361_REG_ISINK_SFSTR1		(0x157)
#define MT6361_REG_ISINK1_MODE_CTRL0	(0x158)
#define MT6361_REG_ISINK2_MODE_CTRL0	(0x159)
#define MT6361_REG_ISINK3_MODE_CTRL0	(0x15A)
#define MT6361_REG_DRIVER_ANA_CON2	(0x15C)
#define MT6361_REG_DRIVER_ISINK1_CON0	(0x171)
#define MT6361_REG_DRIVER_ISINK2_CON0	(0x172)
#define MT6361_REG_DRIVER_ISINK3_CON0	(0x173)
#define MT6361_BRIGHT_SEL_MASK		GENMASK(7, 5)
#define MT6361_BRIGHT_EN_MASK		GENMASK(7, 5)
#define MT6361_REG_ISINK_EN_CTRL0	(0x174)
#define MT6361_REG_ISINK_EN_CTRL1	(0x175)
#define MT6361_ISINK_STEPUA		2000
#define MT6361_ISINK_MAXUA		12000
#define STATE_OFF			0
#define STATE_KEEP			1
#define STATE_ON			2

enum {
	MT6361_ISINK1,
	MT6361_ISINK2,
	MT6361_ISINK3,
	MT6361_ISINK_MAX,
};

#define MT6361_VIRTUAL_MULTICOLOR	(MT6361_ISINK_MAX + 1)
#define MULTICOLOR_NUM_CHANNELS		3

static int mt6361_set_pwm_dim_duty(struct mt_led_info *info, unsigned int sel);
static int mt6361_get_pwm_dim_duty(struct mt_led_info *info);

struct mt6361_trig_info {
	u16 bright_reg;
	u8 bright_mask;
	u8 bright_sft;
	u16 mode_reg;
	u8 mode_mask;
	u8 mode_sft;
	u16 enable_reg;
	u16 enable_mask;
	u16 enable_sft;
	u16 sfstr_reg;
	u8 sfstr_mask;
	u8 sfstr_sft;
	u16 sfstr_en_reg;
	u8 sfstr_en_mask;
	u8 sfstr_en_sft;
	u16 duty_reg;
	u8 duty_mask;
	u8 duty_sft;
	u16 freq_reg;
	u8 freq_mask;
	u8 freq_sft;
	u16 tr1_reg;
	u8 tr1_mask;
	u8 tr1_sft;
	u16 tr2_reg;
	u8 tr2_mask;
	u8 tr2_sft;
	u16 tf1_reg;
	u8 tf1_mask;
	u8 tf1_sft;
	u16 tf2_reg;
	u8 tf2_mask;
	u8 tf2_sft;
	u16 ton_reg;
	u8 ton_mask;
	u8 ton_sft;
	u16 toff_reg;
	u8 toff_mask;
	u8 toff_sft;
};

struct mt6361_led {
	struct mt_led_info linfo; /* must be the firset member */
	struct mt6361_isink_data *idata;
	struct mt6361_trig_info trig_info;
	u32 led_no;
	u32 default_state;
	int on;
	int max_curr_sel;
};

struct mt6361_isink_data {
	struct regmap *rmap;
	struct device *dev;
	unsigned int leds_active;
	struct mutex lock;
	int mc_on;
	struct mt6361_led *leds;
	int led_on_cnt;
};

static int mt6361_init_common_properties(struct mt6361_led *led,
					 struct led_init_data *init_data)
{
	const char *const states[] = { "off", "keep", "on" };
	const char *str;
	int ret = 0;
	u32 val = 0;

	if (!fwnode_property_read_string(init_data->fwnode, "default-state",
					 &str)) {
		ret = match_string(states, ARRAY_SIZE(states), str);
		if (ret < 0)
			ret = STATE_OFF;
		led->default_state = ret;
	}

	if (!fwnode_property_read_u32(init_data->fwnode, "soft-start-en", &val)) {
		if (val) {
			ret = regmap_update_bits(led->idata->rmap,
						 led->trig_info.sfstr_en_reg,
						 led->trig_info.sfstr_en_mask,
						 led->trig_info.sfstr_en_mask);
		}
	}
	return ret;
}

static int mt6361_isink_enable(struct mt6361_led *led, int on)
{
	int ret = 0;
	u8 idx = led->led_no;
	u8 en_ctrl0 = BIT(6-idx) | BIT(2-idx);
	u8 en_ctrl1 = BIT(6-idx);
	u8 ck_pdn = BIT(idx+1);
	u8 ana_con2 = BIT(idx);

	if (led->idata->led_on_cnt == 0 && on) {
		/* enable RG_DRV_128K_CK_PDN , set 0 = enable*/
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_TOP_CKPDN_CON0,
					 BIT(4), 0);
		if (ret)
			goto out;
	}


	if (on) {
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_DRIVER_ANA_CON2, ana_con2, 0);
		if (ret)
			goto out;
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_TOP_CKPDN_CON0,
					 ck_pdn, 0);
		if (ret)
			goto out;

		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_EN_CTRL0, en_ctrl0,
					 en_ctrl0);
		if (ret)
			goto out;
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_EN_CTRL1, en_ctrl1,
					 en_ctrl1);
		if (ret)
			goto out;
	} else {
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_EN_CTRL1, en_ctrl1, 0);
		if (ret)
			goto out;
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_EN_CTRL0, en_ctrl0, 0);
		if (ret)
			goto out;

		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_TOP_CKPDN_CON0,
					 ck_pdn, ck_pdn);
		if (ret)
			goto out;
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_DRIVER_ANA_CON2, ana_con2,
					 ana_con2);
		if (ret)
			goto out;
	}

	led->on = on;
	led->idata->led_on_cnt += on ? (1) : (-1);
	if (!led->idata->led_on_cnt) {
		/* disable RG_DRV_128K_CK_PDN */
		ret = regmap_update_bits(led->idata->rmap,
					 MT6361_REG_ISINK_TOP_CKPDN_CON0,
					 BIT(4), BIT(4));
		if (ret)
			goto out;
	}
out:
	return ret;
}

static int mt6361_isink_cc_brightness_set(struct led_classdev *led_cdev,
					 unsigned int brightness)
{
	struct mt6361_led *led = (struct mt6361_led *)led_cdev;
	int ret = 0;

	dev_dbg(led->idata->dev, "%s isink%d, current = %d\n", __func__,
		led->led_no, brightness);
	mutex_lock(&led->idata->lock);
	ret = regmap_update_bits(led->idata->rmap, led->trig_info.bright_reg,
				 led->trig_info.bright_mask,
				 (brightness ? brightness - 1 : 0) << led->trig_info.bright_sft);
	mutex_unlock(&led->idata->lock);
	return ret;
}

static int __maybe_unused mt6361_isink_cc_brightness_get(struct led_classdev *led_cdev)
{
	struct mt6361_led *led = (struct mt6361_led *)led_cdev;
	int ret;
	u32 rdata = 0;

	mutex_lock(&led->idata->lock);
	ret = regmap_read(led->idata->rmap, led->trig_info.bright_reg, &rdata);
	mutex_unlock(&led->idata->lock);
	if (ret < 0)
		return LED_OFF;
	rdata =	(rdata & led->trig_info.bright_mask) >> led->trig_info.bright_sft;
	dev_dbg(led->idata->dev, "%s isink%d, current = %d\n", __func__,
		led->led_no, rdata);
	return rdata + 1;
}

enum led_brightness mt6361_isink_pwm_brightness_get(struct led_classdev *led_cdev)
{
	struct mt6361_led *led = (struct mt6361_led *)led_cdev;

	return mt6361_get_pwm_dim_duty(&led->linfo);
}

static int mt6361_isink_pwm_brightness_set(struct led_classdev *led_cdev,
				       enum led_brightness brightness)
{
	struct mt6361_led *led = (struct mt6361_led *)led_cdev;
	int ret;

	dev_dbg(led->idata->dev, "%s isink%d, brightness = %d\n", __func__,
		 led->led_no, brightness);

	ret = mt6361_set_pwm_dim_duty(&led->linfo, brightness);
	if (ret < 0) {
		dev_err(led->idata->dev, "%s: failed to set brightness\n",
			__func__);
		return ret;
	}
	return mt6361_isink_enable(led, brightness ? true : false);
}

static struct mt6361_trig_info mt6361_trig_info[MT6361_ISINK_MAX];

#if IS_ENABLED(CONFIG_LEDS_CLASS_MULTICOLOR)
static int mt6361_mc_enable(struct mt6361_isink_data *idata, u32 mask, u32 enable)
{
	int ret = 0;

	ret = regmap_update_bits(idata->rmap, MT6361_REG_ISINK_EN_CTRL0, mask,
				 enable);
	if (ret == 0)
		idata->mc_on = enable;
	return ret;
}

static int mt6361_mc_brightness_set(struct led_classdev *led_cdev,
				    enum led_brightness brightness)
{
	struct led_classdev_mc *mccdev = lcdev_to_mccdev(led_cdev);
	struct mt6361_led *led = (struct mt6361_led *)mccdev;
	struct mt6361_isink_data *idata = led->idata;
	struct mt6361_trig_info *trig_info = mt6361_trig_info;
	u32 real_bright = 0, enable_mask = 0, enable = 0;
	int on = (brightness > 0) ? 1 : 0;
	int i = 0, ret = 0;

	mutex_lock(&idata->lock);
	led_mc_calc_color_components(mccdev, brightness);
	for (i = 0; i < mccdev->num_colors; i++) {
		struct mc_subled *subled = mccdev->subled_info + i;

		real_bright = min(led_cdev->max_brightness, subled->brightness);
		ret = regmap_update_bits(idata->rmap,
					 trig_info[i].bright_reg,
					 trig_info[i].bright_mask,
					 real_bright << trig_info[i].bright_sft);
		if (ret)
			goto unlock;
		enable_mask |= trig_info[i].enable_mask;
		if (real_bright)
			enable |= trig_info[i].enable_mask;
	}
	if (on != idata->mc_on)
		ret = mt6361_mc_enable(idata, enable_mask, enable);
unlock:
	mutex_unlock(&idata->lock);
	return ret;
}
#endif /* multicolor */

static u32 clamp_align(u32 val, u32 min, u32 max, u32 step)
{
	u32 retval;

	retval = clamp_val(val, min, max);
	if (step > 1)
		retval = rounddown(retval - min, step) + min;
	return retval;
}

#define MT6361_BRIGHT_REG_ISINK1	MT6361_REG_DRIVER_ISINK1_CON0
#define MT6361_BRIGHT_MASK_ISINK1	GENMASK(7, 5)
#define MT6361_BRIGHT_SFT_ISINK1	(5)
#define MT6361_ENABLE_REG_ISINK1	MT6361_REG_ISINK_EN_CTRL0
#define MT6361_ENABLE_MASK_ISINK1	(BIT(6) | BIT(2) | BIT(14))
#define MT6361_ENABLE_SFT_ISINK1	(6)
#define MT6361_MODE_REG_ISINK1		MT6361_REG_ISINK1_MODE_CTRL0
#define MT6361_MODE_MASK_ISINK1		GENMASK(7, 6)
#define MT6361_MODE_SFT_ISINK1		(6)
#define MT6361_SFSTR_REG_ISINK1		MT6361_REG_ISINK_SFSTR0
#define MT6361_SFSTR_MASK_ISINK1	GENMASK(7, 6)
#define MT6361_SFSTR_SFT_ISINK1		(6)
#define MT6361_SFSTR_EN_REG_ISINK1	MT6361_REG_ISINK_SFSTR0
#define MT6361_SFSTR_EN_MASK_ISINK1	BIT(5)
#define MT6361_SFSTR_EN_SFT_ISINK1	(5)
#define MT6361_DUTY_REG_ISINK1		MT6361_REG_ISINK1_CON2
#define MT6361_DUTY_MASK_ISINK1		GENMASK(7, 0)
#define MT6361_DUTY_SFT_ISINK1		(0)
#define MT6361_FREQ_REG_ISINK1		MT6361_REG_ISINK1_CON0
#define MT6361_FREQ_MASK_ISINK1		GENMASK(7, 0)
#define MT6361_FREQ_SFT_ISINK1		(0)
#define MT6361_TR1_REG_ISINK1		MT6361_REG_ISINK1_CON3
#define MT6361_TR1_MASK_ISINK1		GENMASK(7, 4)
#define MT6361_TR1_SFT_ISINK1		(4)
#define MT6361_TR2_REG_ISINK1		MT6361_REG_ISINK1_CON3
#define MT6361_TR2_MASK_ISINK1		GENMASK(3, 0)
#define MT6361_TR2_SFT_ISINK1		(0)
#define MT6361_TF1_REG_ISINK1		MT6361_REG_ISINK1_CON4
#define MT6361_TF1_MASK_ISINK1		GENMASK(7, 4)
#define MT6361_TF1_SFT_ISINK1		(4)
#define MT6361_TF2_REG_ISINK1		MT6361_REG_ISINK1_CON4
#define MT6361_TF2_MASK_ISINK1		GENMASK(3, 0)
#define MT6361_TF2_SFT_ISINK1		(0)
#define MT6361_TON_REG_ISINK1		MT6361_REG_ISINK1_CON5
#define MT6361_TON_MASK_ISINK1		GENMASK(7, 4)
#define MT6361_TON_SFT_ISINK1		(4)
#define MT6361_TOFF_REG_ISINK1		MT6361_REG_ISINK1_CON5
#define MT6361_TOFF_MASK_ISINK1		GENMASK(3, 0)
#define MT6361_TOFF_SFT_ISINK1		(0)

#define MT6361_BRIGHT_REG_ISINK2	MT6361_REG_DRIVER_ISINK2_CON0
#define MT6361_BRIGHT_MASK_ISINK2	GENMASK(7, 5)
#define MT6361_BRIGHT_SFT_ISINK2	(5)
#define MT6361_ENABLE_REG_ISINK2	MT6361_REG_ISINK_EN_CTRL0
#define MT6361_ENABLE_MASK_ISINK2	(BIT(5) | BIT(1) | BIT(13))
#define MT6361_ENABLE_SFT_ISINK2	(5)
#define MT6361_MODE_REG_ISINK2		MT6361_REG_ISINK2_MODE_CTRL0
#define MT6361_MODE_MASK_ISINK2		GENMASK(7, 6)
#define MT6361_MODE_SFT_ISINK2		(6)
#define MT6361_SFSTR_REG_ISINK2		MT6361_REG_ISINK_SFSTR0
#define MT6361_SFSTR_MASK_ISINK2	GENMASK(4, 3)
#define MT6361_SFSTR_SFT_ISINK2		(3)
#define MT6361_SFSTR_EN_REG_ISINK2	MT6361_REG_ISINK_SFSTR0
#define MT6361_SFSTR_EN_MASK_ISINK2	BIT(1)
#define MT6361_SFSTR_EN_SFT_ISINK2	(1)
#define MT6361_DUTY_REG_ISINK2		MT6361_REG_ISINK2_CON2
#define MT6361_DUTY_MASK_ISINK2		GENMASK(7, 0)
#define MT6361_DUTY_SFT_ISINK2		(0)
#define MT6361_FREQ_REG_ISINK2		MT6361_REG_ISINK2_CON0
#define MT6361_FREQ_MASK_ISINK2		GENMASK(7, 0)
#define MT6361_FREQ_SFT_ISINK2		(0)
#define MT6361_TR1_REG_ISINK2		MT6361_REG_ISINK2_CON3
#define MT6361_TR1_MASK_ISINK2		GENMASK(7, 4)
#define MT6361_TR1_SFT_ISINK2		(4)
#define MT6361_TR2_REG_ISINK2		MT6361_REG_ISINK2_CON3
#define MT6361_TR2_MASK_ISINK2		GENMASK(3, 0)
#define MT6361_TR2_SFT_ISINK2		(0)
#define MT6361_TF1_REG_ISINK2		MT6361_REG_ISINK2_CON4
#define MT6361_TF1_MASK_ISINK2		GENMASK(7, 4)
#define MT6361_TF1_SFT_ISINK2		(4)
#define MT6361_TF2_REG_ISINK2		MT6361_REG_ISINK2_CON4
#define MT6361_TF2_MASK_ISINK2		GENMASK(3, 0)
#define MT6361_TF2_SFT_ISINK2		(0)
#define MT6361_TON_REG_ISINK2		MT6361_REG_ISINK2_CON5
#define MT6361_TON_MASK_ISINK2		GENMASK(7, 4)
#define MT6361_TON_SFT_ISINK2		(4)
#define MT6361_TOFF_REG_ISINK2		MT6361_REG_ISINK2_CON5
#define MT6361_TOFF_MASK_ISINK2		GENMASK(3, 0)
#define MT6361_TOFF_SFT_ISINK2		(0)

#define MT6361_BRIGHT_REG_ISINK3	MT6361_REG_DRIVER_ISINK3_CON0
#define MT6361_BRIGHT_MASK_ISINK3	GENMASK(7, 5)
#define MT6361_BRIGHT_SFT_ISINK3	(5)
#define MT6361_ENABLE_REG_ISINK3	MT6361_REG_ISINK_EN_CTRL0
#define MT6361_ENABLE_MASK_ISINK3	(BIT(4) | BIT(0) | BIT(12))
#define MT6361_ENABLE_SFT_ISINK3	(4)
#define MT6361_MODE_REG_ISINK3		MT6361_REG_ISINK3_MODE_CTRL0
#define MT6361_MODE_MASK_ISINK3		GENMASK(7, 6)
#define MT6361_MODE_SFT_ISINK3		(6)
#define MT6361_SFSTR_REG_ISINK3		MT6361_REG_ISINK_SFSTR1
#define MT6361_SFSTR_MASK_ISINK3	GENMASK(7, 6)
#define MT6361_SFSTR_SFT_ISINK3		(6)
#define MT6361_SFSTR_EN_REG_ISINK3	MT6361_REG_ISINK_SFSTR1
#define MT6361_SFSTR_EN_MASK_ISINK3	BIT(5)
#define MT6361_SFSTR_EN_SFT_ISINK3	(5)
#define MT6361_DUTY_REG_ISINK3		MT6361_REG_ISINK3_CON2
#define MT6361_DUTY_MASK_ISINK3		GENMASK(7, 0)
#define MT6361_DUTY_SFT_ISINK3		(0)
#define MT6361_FREQ_REG_ISINK3		MT6361_REG_ISINK3_CON0
#define MT6361_FREQ_MASK_ISINK3		GENMASK(7, 0)
#define MT6361_FREQ_SFT_ISINK3		(0)
#define MT6361_TR1_REG_ISINK3		MT6361_REG_ISINK3_CON3
#define MT6361_TR1_MASK_ISINK3		GENMASK(7, 4)
#define MT6361_TR1_SFT_ISINK3		(4)
#define MT6361_TR2_REG_ISINK3		MT6361_REG_ISINK3_CON3
#define MT6361_TR2_MASK_ISINK3		GENMASK(3, 0)
#define MT6361_TR2_SFT_ISINK3		(0)
#define MT6361_TF1_REG_ISINK3		MT6361_REG_ISINK3_CON4
#define MT6361_TF1_MASK_ISINK3		GENMASK(7, 4)
#define MT6361_TF1_SFT_ISINK3		(4)
#define MT6361_TF2_REG_ISINK3		MT6361_REG_ISINK3_CON4
#define MT6361_TF2_MASK_ISINK3		GENMASK(3, 0)
#define MT6361_TF2_SFT_ISINK3		(0)
#define MT6361_TON_REG_ISINK3		MT6361_REG_ISINK3_CON5
#define MT6361_TON_MASK_ISINK3		GENMASK(7, 4)
#define MT6361_TON_SFT_ISINK3		(4)
#define MT6361_TOFF_REG_ISINK3		MT6361_REG_ISINK3_CON5
#define MT6361_TOFF_MASK_ISINK3		GENMASK(3, 0)
#define MT6361_TOFF_SFT_ISINK3		(0)

#define MT6361_TRIG_REG_ISINK(_id)				\
{								\
	.bright_reg = MT6361_BRIGHT_REG_ISINK##_id,		\
	.bright_mask = MT6361_BRIGHT_MASK_ISINK##_id,		\
	.bright_sft = MT6361_BRIGHT_SFT_ISINK##_id,		\
	.mode_reg = MT6361_MODE_REG_ISINK##_id,		\
	.mode_mask = MT6361_MODE_MASK_ISINK##_id,		\
	.mode_sft = MT6361_MODE_SFT_ISINK##_id,		\
	.enable_reg = MT6361_ENABLE_REG_ISINK##_id,		\
	.enable_mask = MT6361_ENABLE_MASK_ISINK##_id,		\
	.enable_sft = MT6361_ENABLE_SFT_ISINK##_id,		\
	.sfstr_reg = MT6361_SFSTR_REG_ISINK##_id,		\
	.sfstr_mask = MT6361_SFSTR_MASK_ISINK##_id,		\
	.sfstr_sft = MT6361_SFSTR_SFT_ISINK##_id,		\
	.sfstr_en_reg = MT6361_SFSTR_EN_REG_ISINK##_id,		\
	.sfstr_en_mask = MT6361_SFSTR_EN_MASK_ISINK##_id,		\
	.sfstr_en_sft = MT6361_SFSTR_EN_SFT_ISINK##_id,		\
	.duty_reg = MT6361_DUTY_REG_ISINK##_id,		\
	.duty_mask = MT6361_DUTY_MASK_ISINK##_id,		\
	.duty_sft = MT6361_DUTY_SFT_ISINK##_id,		\
	.freq_reg = MT6361_FREQ_REG_ISINK##_id,		\
	.freq_mask = MT6361_FREQ_MASK_ISINK##_id,		\
	.freq_sft = MT6361_FREQ_SFT_ISINK##_id,		\
	.tr1_reg = MT6361_TR1_REG_ISINK##_id,		\
	.tr1_mask = MT6361_TR1_MASK_ISINK##_id,		\
	.tr1_sft = MT6361_TR1_SFT_ISINK##_id,		\
	.tr2_reg = MT6361_TR2_REG_ISINK##_id,		\
	.tr2_mask = MT6361_TR2_MASK_ISINK##_id,		\
	.tr2_sft = MT6361_TR2_SFT_ISINK##_id,		\
	.tf1_reg = MT6361_TF1_REG_ISINK##_id,		\
	.tf1_mask = MT6361_TF1_MASK_ISINK##_id,		\
	.tf1_sft = MT6361_TF1_SFT_ISINK##_id,		\
	.tf2_reg = MT6361_TF2_REG_ISINK##_id,		\
	.tf2_mask = MT6361_TF2_MASK_ISINK##_id,		\
	.tf2_sft = MT6361_TF2_SFT_ISINK##_id,		\
	.ton_reg = MT6361_TON_REG_ISINK##_id,		\
	.ton_mask = MT6361_TON_MASK_ISINK##_id,		\
	.ton_sft = MT6361_TON_SFT_ISINK##_id,		\
	.toff_reg = MT6361_TOFF_REG_ISINK##_id,		\
	.toff_mask = MT6361_TOFF_MASK_ISINK##_id,		\
	.toff_sft = MT6361_TOFF_SFT_ISINK##_id,		\
}

static struct mt6361_trig_info mt6361_trig_info[MT6361_ISINK_MAX] = {
	MT6361_TRIG_REG_ISINK(1),
	MT6361_TRIG_REG_ISINK(2),
	MT6361_TRIG_REG_ISINK(3),
};

static int mt6361_change_mode(struct led_classdev *led_dev, int mode)
{
	struct mt6361_led *led = (struct mt6361_led *)led_dev;

	if (mode < 0 || mode >= MT_LED_MODE_MAX)
		return -EINVAL;

	dev_info(led_dev->dev, "%s mode = %d\n", __func__, mode);

	return regmap_update_bits(led->idata->rmap, led->trig_info.mode_reg,
				  led->trig_info.mode_mask,
				  mode << led->trig_info.mode_sft);
}

static int mt6361_get_current_step(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.sfstr_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.sfstr_mask) >>
		led->trig_info.sfstr_sft;
}

static int mt6361_set_current_step(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.sfstr_reg,
				  led->trig_info.sfstr_mask,
				  sel << led->trig_info.sfstr_sft);
}

static int mt6361_list_pwm_duty(struct mt_led_info *info, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%s\n", "0~255");
	return ret < 0 ? ret : 0;
}

static int mt6361_list_pwm_freq(struct mt_led_info *info, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%s\n", "0~65535 (500 HZ ~ 0.076HZ)");
	return ret < 0 ? ret : 0;
}

static int mt6361_get_pwm_dim_duty(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.duty_reg, &value);
	if (ret < 0)
		return ret;
	pr_info("%s: duty = %d\n", __func__, value);
	return value;
}

static int mt6361_set_pwm_dim_duty(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	pr_info("%s: duty = %d\n", __func__, sel);
	return regmap_write(led->idata->rmap, led->trig_info.duty_reg, sel);
}

static int mt6361_get_pwm_dim_freq(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_raw_read(led->idata->rmap, led->trig_info.freq_reg, &value, 2);
	if (ret < 0)
		return ret;
	return value;
}

static int mt6361_set_pwm_dim_freq(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_raw_write(led->idata->rmap, led->trig_info.freq_reg, &sel, 2);
}

static int mt6361_get_breath_tr1(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.tr1_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.tr1_mask) >> led->trig_info.tr1_sft;
}

static int mt6361_set_breath_tr1(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.tr1_reg,
				  led->trig_info.tr1_mask,
				  sel << led->trig_info.tr1_sft);
}

static int mt6361_get_breath_tr2(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.tr2_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.tr2_mask) >> led->trig_info.tr2_sft;
}

static int mt6361_set_breath_tr2(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.tr2_reg,
				  led->trig_info.tr2_mask,
				  sel << led->trig_info.tr2_sft);
}

static int mt6361_get_breath_tf1(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.tf1_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.tf1_mask) >> led->trig_info.tf1_sft;
}

static int mt6361_set_breath_tf1(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.tf1_reg,
				  led->trig_info.tf1_mask,
				  sel << led->trig_info.tf1_sft);
}

static int mt6361_get_breath_tf2(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.tf2_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.tf2_mask) >> led->trig_info.tf2_sft;
}

static int mt6361_set_breath_tf2(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.tf2_reg,
				  led->trig_info.tf2_mask,
				  sel << led->trig_info.tf2_sft);
}

static int mt6361_get_breath_ton(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.ton_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.ton_mask) >> led->trig_info.ton_sft;
}

static int mt6361_set_breath_ton(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.ton_reg,
				  led->trig_info.ton_mask,
				  sel << led->trig_info.ton_sft);
}

static int mt6361_get_breath_toff(struct mt_led_info *info)
{
	struct mt6361_led *led = (struct mt6361_led *)info;
	int ret = 0;
	unsigned int value = 0;

	ret = regmap_read(led->idata->rmap, led->trig_info.toff_reg, &value);
	if (ret < 0)
		return ret;
	return (value & led->trig_info.toff_mask) >> led->trig_info.toff_sft;
}

static int mt6361_set_breath_toff(struct mt_led_info *info, unsigned int sel)
{
	struct mt6361_led *led = (struct mt6361_led *)info;

	return regmap_update_bits(led->idata->rmap, led->trig_info.toff_reg,
				  led->trig_info.toff_mask,
				  sel << led->trig_info.toff_sft);
}
static irqreturn_t mt6361_isink1_short_handler(int irq, void *data)
{
	struct mt6361_isink_data *idata = data;

	dev_dbg(idata->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_isink1_open_handler(int irq, void *data)
{
	struct mt6361_isink_data *idata = data;

	dev_dbg(idata->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_isink2_short_handler(int irq, void *data)
{
	struct mt6361_isink_data *idata = data;

	dev_dbg(idata->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_isink2_open_handler(int irq, void *data)
{
	struct mt6361_isink_data *idata = data;

	dev_dbg(idata->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t mt6361_isink3_short_handler(int irq, void *data)
{
	struct mt6361_isink_data *idata = data;

	dev_dbg(idata->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}
static irqreturn_t mt6361_isink3_open_handler(int irq, void *data)
{
	struct mt6361_isink_data *idata = data;

	dev_dbg(idata->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static struct mt_led_trig_ops mt6361_isink_trig_ops = {
	.change_mode = mt6361_change_mode,
	.get_current_step = mt6361_get_current_step,
	.set_current_step = mt6361_set_current_step,
	.get_pwm_dim_duty = mt6361_get_pwm_dim_duty,
	.set_pwm_dim_duty = mt6361_set_pwm_dim_duty,
	.get_pwm_dim_freq = mt6361_get_pwm_dim_freq,
	.set_pwm_dim_freq = mt6361_set_pwm_dim_freq,
	.get_breath_tr1 = mt6361_get_breath_tr1,
	.get_breath_tr2 = mt6361_get_breath_tr2,
	.get_breath_tf1 = mt6361_get_breath_tf1,
	.get_breath_tf2 = mt6361_get_breath_tf2,
	.get_breath_ton = mt6361_get_breath_ton,
	.get_breath_toff = mt6361_get_breath_toff,
	.set_breath_tr1 = mt6361_set_breath_tr1,
	.set_breath_tr2 = mt6361_set_breath_tr2,
	.set_breath_tf1 = mt6361_set_breath_tf1,
	.set_breath_tf2 = mt6361_set_breath_tf2,
	.set_breath_ton = mt6361_set_breath_ton,
	.set_breath_toff = mt6361_set_breath_toff,
	.list_pwm_duty = mt6361_list_pwm_duty,
	.list_pwm_freq = mt6361_list_pwm_freq,
};

static int mt6361_init_isink_properties(struct mt6361_led *led,
					struct led_init_data *init_data)
{
	struct led_classdev *lcdev;
	struct mt6361_isink_data *idata = led->idata;
	int max_ua = MT6361_ISINK_MAXUA, step_ua = MT6361_ISINK_STEPUA;
	int max_brightness;

	u32 val = 0;
#if IS_ENABLED(CONFIG_LEDS_CLASS_MULTICOLOR)
	struct fwnode_handle *child;
	int num_color = 0;
#endif
	int ret = 0;

	if (led->led_no == MT6361_VIRTUAL_MULTICOLOR) {
#if IS_ENABLED(CONFIG_LEDS_CLASS_MULTICOLOR)
		struct mc_subled *sub_led;

		sub_led = devm_kzalloc(idata->dev,
				       sizeof(*sub_led) * MULTICOLOR_NUM_CHANNELS,
				       GFP_KERNEL);
		fwnode_for_each_child_node(init_data->fwnode, child) {
			u32 reg = 0, color = 0;

			ret = fwnode_property_read_u32(child, "reg", &reg);
			if (ret || reg > MT6361_ISINK3 ||
				idata->leds_active & BIT(reg))
				return -EINVAL;
			ret = fwnode_property_read_u32(child, "color", &color);
			if (ret) {
				dev_err(idata->dev,
					"led %d, no color specified\n",
					led->led_no);
				return ret;
			}
			idata->leds_active |= BIT(reg);
			sub_led[num_color].color_index = color;
			sub_led[num_color].channel = reg;
			num_color++;
		}
		if (num_color < 2) {
			dev_err(idata->dev,
				"Multicolor must include 2 or more led channel\n");
			return -EINVAL;
		}
		led->linfo.mc.num_colors = num_color;
		led->linfo.mc.subled_info = sub_led;
		lcdev = &led->linfo.mc.led_cdev;
		lcdev->brightness_set_blocking = mt6361_mc_brightness_set;
#else
		return -EOPNOTSUPP;
#endif /* multicolor */
	} else {
		lcdev = &led->linfo.isink;
		lcdev->brightness_set_blocking = mt6361_isink_pwm_brightness_set;
		lcdev->brightness_get = mt6361_isink_pwm_brightness_get;
	}
	ret = fwnode_property_read_u32(init_data->fwnode, "led-max-microamp",
				       &val);
	if (ret) {
		dev_warn(idata->dev,
			 "Not specified led-max-microamp, config to the min\n");
		val = step_ua;
	} else
		val = clamp_align(val, 0, max_ua, step_ua);

	led->max_curr_sel = val / step_ua; /* 0 map to disable */
	dev_info(idata->dev, "max_curr_sel = %d\n", led->max_curr_sel);

	fwnode_property_read_string(init_data->fwnode, "linux,default-trigger",
				    &lcdev->default_trigger);
	return 0;
}

static int mt6361_isink_init_default_state(struct mt6361_led *led)
{
	struct mt6361_isink_data *idata = led->idata;
	unsigned int regval = 0;
	int ret = 0;
	bool enable = false;

	ret = regmap_bulk_read(idata->rmap, led->trig_info.enable_reg, &regval, 2);
	if (ret)
		return ret;
	dev_info(idata->dev, "%s enable_reg val = 0x%04x\n", __func__, regval);

	ret = mt6361_isink_cc_brightness_set(&led->linfo.isink, led->max_curr_sel);
	if (ret < 0) {
		dev_err(idata->dev, "failed to set max curr\n", __func__);
		return ret;
	}

	switch (led->default_state) {
	case STATE_ON:
		led->linfo.isink.brightness = LED_FULL;
		enable = true;
		break;
	case STATE_KEEP:
		ret = mt6361_get_pwm_dim_duty(&led->linfo);
		if (ret < 0)
			return ret;
		led->linfo.isink.brightness = ret;
		break;
	default:
		led->linfo.isink.brightness = LED_OFF;
		break;
	}

	if (led->default_state != STATE_KEEP)
		ret = mt6361_isink_pwm_brightness_set(&led->linfo.isink,
						  led->linfo.isink.brightness);
	return ret;
}
static int mt6361_led_register(struct device *parent, struct mt6361_led *led,
			       struct led_init_data *init_data)
{
	int ret = 0;

	dev_info(parent, "%s, led_no = %d\n", __func__, led->led_no);
	switch (led->led_no) {
	case MT6361_VIRTUAL_MULTICOLOR:
#if IS_ENABLED(CONFIG_LEDS_CLASS_MULTICOLOR)
		ret = mt6361_mc_brightness_set(&led->linfo.mc.led_cdev, LED_OFF);
		if (ret < 0) {
			dev_err(parent,
				"Failed to init nulticolor brightness\n");
			return ret;
		}
		ret = devm_led_classdev_multicolor_register_ext(parent,
								&led->linfo.mc,
								init_data);
		if (ret) {
			dev_err(parent, "Couldn't register multicolor\n");
			return ret;
		}
#endif /* multicolor */
		break;
	default:
		ret = mt6361_isink_init_default_state(led);
		if (ret) {
			dev_err(parent, "Failed to init %d isink state\n",
				led->led_no);
		}
		ret = devm_led_classdev_register_ext(parent, &led->linfo.isink,
						     init_data);
		if (ret) {
			dev_err(parent, "Couldn't register isink %d\n",
				led->led_no);
			return ret;
		}
		break;
	}
	return 0;
}

int mt6361_isink_parse_dt(struct device *dev, struct mt6361_isink_data *idata)
{
	struct fwnode_handle *child;
	int i = 0, ret = 0;

	pr_info("%s\n", __func__);
	device_for_each_child_node(dev, child) {
		struct led_init_data init_data = { .fwnode = child, };
		u32 reg;

		ret = fwnode_property_read_u32(child, "reg", &reg);
		if (ret) {
			dev_err(dev, "parsing reg failed\n");
			return ret;
		}
		if (reg > MT6361_ISINK_MAX) {
			dev_err(dev, "out of isink index\n");
			return -EINVAL;
		}
		if (idata->leds_active & BIT(reg)) {
			dev_err(dev, "duplicate led\n");
			return -EINVAL;
		}
		idata->leds_active |= BIT(reg);
		idata->leds[i].led_no = reg;
		idata->leds[i].idata = idata;
		if (reg < MT6361_ISINK_MAX)
			idata->leds[i].trig_info = mt6361_trig_info[reg];
		idata->leds[i].linfo.ops = &mt6361_isink_trig_ops;
		idata->leds[i].linfo.magic_code = MT_LED_SUPPORT_ALL;
		ret = mt6361_init_common_properties(&idata->leds[i], &init_data);
		if (ret) {
			dev_err(dev, "init common properties failed\n");
			return ret;
		}
		if (reg == MT6361_VIRTUAL_MULTICOLOR ||
			reg < MT6361_ISINK_MAX) {
			ret = mt6361_init_isink_properties(&idata->leds[i],
							   &init_data);
			if (ret) {
				dev_err(dev, "init isink properties failed\n");
				return -EINVAL;
			}
		}
		ret = mt6361_led_register(dev, &idata->leds[i], &init_data);
		if (ret) {
			dev_err(dev, "led register failed\n");
			return -EINVAL;
		}
		i++;
	}
	return 0;
}

#define MT6361_ISINK_IRQ(_name)			\
{						\
	.name = #_name,				\
	.hdlr = mt6361_##_name##_handler,	\
}

static int mt6361_isink_irq_register(struct mt6361_isink_data *idata)
{
	int i = 0, ret = 0;
	const struct {
		char *name;
		irq_handler_t hdlr;
	} mt6361_isink_irq[] = {
		MT6361_ISINK_IRQ(isink1_short),
		MT6361_ISINK_IRQ(isink1_open),
		MT6361_ISINK_IRQ(isink2_short),
		MT6361_ISINK_IRQ(isink2_open),
		MT6361_ISINK_IRQ(isink3_short),
		MT6361_ISINK_IRQ(isink3_open),
	};

	for (i = 0; i < ARRAY_SIZE(mt6361_isink_irq); i++) {
		ret = platform_get_irq_byname(to_platform_device(idata->dev),
					      mt6361_isink_irq[i].name);
		if (ret < 0) {
			dev_info(idata->dev, "no irq %s\n",
				 mt6361_isink_irq[i].name);
			continue;
		}
		ret = devm_request_threaded_irq(idata->dev, ret, NULL,
						mt6361_isink_irq[i].hdlr,
						IRQF_ONESHOT,
						mt6361_isink_irq[i].name, idata);
		if (ret < 0) {
			dev_err(idata->dev, "failed to register irq %s\n",
				mt6361_isink_irq[i].name);
			return ret;
		}
	}
	return 0;
}

static int mt6361_isink_probe(struct platform_device *pdev)
{
	struct mt6361_isink_data *isink_data;
	int ret = 0;
	int count = 0;

	pr_info("%s\n", __func__);
	count = device_get_child_node_count(&pdev->dev);
	if (!count || count > MT6361_ISINK_MAX) {
		dev_err(&pdev->dev,
			"No child node or node count over max led number %d\n",
			count);
		return -EINVAL;
	}
	pr_info("%s count = %d\n", __func__, count);
	isink_data = devm_kzalloc(&pdev->dev, sizeof(*isink_data), GFP_KERNEL);
	isink_data->leds = devm_kzalloc(&pdev->dev,
					sizeof(struct mt6361_led)*count,
					GFP_KERNEL);
	isink_data->dev = &pdev->dev;
	mutex_init(&isink_data->lock);
	isink_data->rmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!isink_data->rmap) {
		dev_err(&pdev->dev, "get regmap failed\n");
		return -ENODEV;
	}
	ret = mt6361_isink_parse_dt(&pdev->dev, isink_data);
	if (ret < 0) {
		dev_err(&pdev->dev, "parse dt failed\n");
		return -ENODEV;
	}
	platform_set_drvdata(pdev, isink_data);
	ret = mt6361_isink_irq_register(isink_data);
	if (ret < 0) {
		dev_err(&pdev->dev, "register irq failed\n");
		return ret;
	}
	return 0;
}

static const struct of_device_id __maybe_unused mt6361_isink_of_match[] = {
	{	.compatible = "mediatek,mt6361-isink", },
	{}
};
MODULE_DEVICE_TABLE(of, mt6361_isink_of_match);

static struct platform_driver mt6361_isink_driver = {
	.probe = mt6361_isink_probe,
	.driver = {
		.name = "mt6361-isink",
		.of_match_table = mt6361_isink_of_match,
	},
};

module_platform_driver(mt6361_isink_driver);
MODULE_AUTHOR("Sakya Chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("MT6361 ISINK Driver");
MODULE_LICENSE("GPL v2");
