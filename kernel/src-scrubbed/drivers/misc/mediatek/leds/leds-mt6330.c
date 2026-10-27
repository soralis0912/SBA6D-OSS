// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/mfd/mt6330/registers.h>
#include <linux/mfd/mt6330/core.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include <linux/ledtrig-mediatek.h>

#ifndef UNUSED
#define UNUSED(x) do { (void)(x); } while (0)
#endif

/*
 * Register field for mt6330_TOP_CKPDN0 to enable
 * 128K clock common for LED device.
 */
#define RG_DRV_ISINK_CK_PDN             		MT6330_XPP_TOP_CKPDN_CON0
#define RG_DRV_128K_CK_PDN_MASK         	BIT(4)
#define RG_DRV_128K_CK_PDN_SHIFT        	(4)
#define RG_DRV_ISINK_CK_PDN_MASK(i)     	BIT(i)
#define RG_DRV_ISINK_CK_PDN_VAL(i)      	BIT(i)

//ISINK channel enable
#define ISINK_CH_EN                   		MT6330_ISINK_EN_CTRL1
#define ISINK_CH_EN_MASK(i)           	BIT(7-i)
#define ISINK_CH_EN_SHIFT(i)          	(7-i)
#define ISINK_CHOP1_EN               		 MT6330_ISINK_EN_CTRL0
#define ISINK_CHOP1_EN_MASK(i)        	BIT(3-i)
#define ISINK_CHOP1_EN_SHIFT(i)       	(3-i)

//ISINK Step
#define ISINK_CH_STEP(i)             		(MT6330_DRIVER_DL_ISINK0_CON0 + (i))
#define ISINK_CH_STEP_SHIFT			(5)
#define ISINK_CH_STEP_MASK  		(0x7 << ISINK_CH_STEP_SHIFT)
#define ISINK_CH_STEP_MAX        		(0x7)

//ISINK mode
#define ISINK_MODE(i)                 		(MT6330_ISINK3_MODE_CTRL0 + (i%3))
#define ISINK_MODE_SHIFT             	 	(6)
#define ISINK_MODE_MASK            		(0x3 << ISINK_MODE_SHIFT)
#define ISINK_MODE_PWM			(0x00)
#define ISINK_MODE_BREATH			(0x01)
#define ISINK_MODE_REGISTER		(0x11)

//PWM mode : frequency
#define ISINK_DIM_FSEL_L(i)          		(MT6330_ISINK3_CON0 + 0x6 * (i%3))
#define ISINK_DIM_FSEL_L_SHIFT          	(0)
#define ISINK_DIM_FSEL_L_MASK         	(0xFF<<ISINK_DIM_FSEL_L_SHIFT)
#define ISINK_DIM_FSEL_L_MAX		(0xFF)

#define ISINK_DIM_FSEL_H(i)           		(MT6330_ISINK3_CON1 + 0x6 * (i%3))
#define ISINK_DIM_FSEL_H_SHIFT          	(0)
#define ISINK_DIM_FSEL_H_MASK         	(0xFF<<ISINK_DIM_FSEL_H_SHIFT)
#define ISINK_DIM_FSEL_H_MAX		(0xFF)

//PWM mode : duty
#define ISINK_DIM_DUTY(i)             		(MT6330_ISINK3_CON2 + 0x6 * (i%3))
#define ISINK_DIM_DUTY_SHIFT          	(0)
#define ISINK_DIM_DUTY_MASK          	(0xFF<<ISINK_DIM_DUTY_SHIFT)
#define ISINK_DIM_DUTY_MAX		(0xFF)

//Breath mode :
#define ISINK_BREATH_TR_SEL(i)        	(MT6330_ISINK3_CON3 + 0x6 * (i%3))
#define ISINK_BREATH_TR1_SEL_SHIFT	(4)
#define ISINK_BREATH_TR1_SEL_MASK  	(0xF0)
#define ISINK_BREATH_TR2_SEL_SHIFT	(0)
#define ISINK_BREATH_TR2_SEL_MASK  	(0x0F)
#define ISINK_BREATH_TF_SEL(i)        	(MT6330_ISINK3_CON4 + 0x6 * (i%3))
#define ISINK_BREATH_TF1_SEL_SHIFT   	(4)
#define ISINK_BREATH_TF1_SEL_MASK   (0xF0)
#define ISINK_BREATH_TF2_SEL_SHIFT   	(0)
#define ISINK_BREATH_TF2_SEL_MASK   (0x0F)
#define ISINK_BREATH_TONOFF_SEL(i)        	(MT6330_ISINK3_CON5 + 0x6 * (i%3))
#define ISINK_BREATH_TON_SEL_SHIFT  (4)
#define ISINK_BREATH_TON_SEL_MASK 	(0xF0)
#define ISINK_BREATH_TOFF_SEL_SHIFT (0)
#define ISINK_BREATH_TOFF_SEL_MASK	(0x0F)

#define ISINK_SFSTR(i)                (MT6330_ISINK_SFSTR0 + (1-i%2))
#define ISINK_SFSTR_TC_MASK(i)        (0x3 << (3 + 3 * (i/2)))
#define ISINK_SFSTR_TC_SHIFT(i)       (3 + 3 * (i/2))
#define ISINK_SFSTR_EN_MASK(i)        BIT(1 + 4 * (i/2))
#define ISINK_SFSTR_EN_SHIFT(i)        (1 + 4 * (i/2))

#define ISINK_PHASE_DLY_TC(i)         (MT6330_ISINK3_PHASE_DLY_CON0 + (i%3))
#define ISINK_PHASE_DLY_TC_MASK       (0x3 << 6)
#define ISINK_PHASE_DLY_EN(i)         (MT6330_ISINK3_PHASE_DLY_CON0 + (i%3))
#define ISINK_PHASE_DLY_EN_MASK       BIT(5)
#define EN1_GPIO_SEL(i)               (MT6330_DRIVER_DL_ISINK1_CON0 + (i))
#define EN1_GPIO_SEL_MASK             BIT(0)
#define BIAS1_GPIO_SEL_MASK           BIT(1)
#define STEP1_GPIO_SEL_MASK           BIT(2)
#define CHOP1_GPIO_SEL_MASK           BIT(3)
#define ISINK_CHOP_CLK_SW_SEL(i)      (MT6330_ISINK3_PHASE_DLY_CON0 + (i%3))
#define ISINK_CHOP_CLK_SW_SEL_MASK    BIT(0)
#define ISINK_CHOP_CLK_SW_MASK        BIT(1)

#define mt6330_MAX_PERIOD		10000
#define mt6330_MAX_BRIGHTNESS		255
#define mt6330_UNIT_DUTY		2000
#define mt6330_CAL_HW_DUTY(o, p)	DIV_ROUND_CLOSEST((o) * 1000000ul,\
					(p) * mt6330_UNIT_DUTY)
//#define LED_TEST
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME " %s(%d) :" fmt, __func__, __LINE__

enum {
	MT6330_LED0 = 0,
	MT6330_LED1,
	MT6330_LED2,
	MT6330_LED3,
	MT6330_MAX,
};

enum {
	MT6360_LEDMODE_PWM = 0,
	MT6360_LEDMODE_BREATH,
	MT6360_LEDMODE_CC,
	MT6360_LEDMODE_MAX,
};

struct mt6330_reg_info {
	int index;
	unsigned int currsel_reg;
	unsigned int currsel_mask;
	unsigned int enable_mask;
	unsigned int mode_reg;
	unsigned int mode_mask;
	unsigned int pwmduty_reg;
	unsigned int pwmduty_mask;
	unsigned int pwmfreq_reg;
	unsigned int pwmfreq_mask;
	unsigned int breath_regbase;
};

struct mt6330_leds;

/**
 * struct mt6330_led - state container for the LED device
 * @id:			the identifier in mt6330 LED device
 * @parent:		the pointer to mt6330 LED controller
 * @cdev:		LED class device for this LED device
 * @current_brightness: current state of the LED device
 */
struct mt6330_led {
	struct mt_led_info l_info; /* most be the first member */
	int			id;
	struct mt6330_leds	*parent;
	enum led_brightness	current_brightness;
};

/**
 * struct mt6330_leds -	state container for holding LED controller
 *			of the driver
 * @dev:		the device pointer
 * @hw:			the underlying hardware providing shared
 *			bus for the register operations
 * @lock:		the lock among process context
 * @led:		the array that contains the state of individual
 *			LED device
 */
struct mt6330_leds {
	struct device		*dev;
	struct regmap *regmap;
	/* protect among process context */
	struct mutex		lock;
	struct mt6330_led	*led[MT6330_MAX];
};

static int mt6330_led_set_clock(struct led_classdev *cdev, int en)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value;
	int ret = 0;

	dev_info(led->parent->dev, "%s: PMIC LED(%d) Set clock %s.\n", 
		__func__, led->id, en?"True":
"False");

	//XPP clock control
	value = RG_DRV_ISINK_CK_PDN_MASK(led->id);
	if(en){
		ret = regmap_update_bits(regmap, RG_DRV_ISINK_CK_PDN,
		RG_DRV_ISINK_CK_PDN_MASK(led->id),
		~value);
	}else{
		ret = regmap_update_bits(regmap, RG_DRV_ISINK_CK_PDN,
		RG_DRV_ISINK_CK_PDN_MASK(led->id),
		value);
	}

	return ret;
}

static int mt6330_led_set_ISINK(struct led_classdev *cdev, int en)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value;
	int ret = 0;

	dev_dbg(led->parent->dev, "%s: PMIC LED(%d) Set ISINK %s.\n", 
		__func__, led->id, en?"True":
"False");

	//PMIC ISINK enable
	value = ISINK_CHOP1_EN_MASK(led->id);
	if(!en)
		value=~value;
	ret = regmap_update_bits(regmap, ISINK_CHOP1_EN,
		ISINK_CHOP1_EN_MASK(led->id), value);
	if (ret < 0)
		return ret;

	value = ISINK_CH_EN_MASK(led->id);
	if(!en)
		value=~value;
	ret = regmap_update_bits(regmap, ISINK_CH_EN,
		ISINK_CH_EN_MASK(led->id), value);
	if (ret < 0)
		return ret;

	return ret;
}

static int mt6330_led_get_ISINK(struct led_classdev *cdev, int *CTL0, int *CTL1)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret = 0;

	ret = regmap_read(regmap, ISINK_CHOP1_EN, CTL0);
	if (ret < 0)
		return ret;

	ret = regmap_read(regmap, ISINK_CH_EN, CTL1);
	if (ret < 0)
		return ret;

	return ret;
}


/*
 * Trigger : CC mode / breath mode / PWM mode
 */
 //Trigger mode change
static int mt6330_led_set_current_step(struct mt_led_info *info, unsigned int step);
static int mt6330_led_set_pwm_dim_freq(struct mt_led_info *info, unsigned int freq);
static int mt6330_led_hw_brightness(struct led_classdev *cdev, enum led_brightness brightness);

static int mt6330_led_change_mode(struct led_classdev *cdev, int mode)
{

	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret = 0;

	int mode_val = 0;

	if (mode >= MT_LED_MODE_MAX)
		return -EINVAL;

	mutex_lock(&leds->lock);

	dev_info(led->parent->dev, "%s mode = %d\n", __func__, mode);
	switch (mode) {
	case MT_LED_CC_MODE:
		mode_val = 2;
		break;
	case MT_LED_PWM_MODE:
		mode_val = 0;
		break;
	case MT_LED_BREATH_MODE:
		mode_val = 1;
		break;
	}

	//PMIC ISINK disable
	mt6330_led_set_ISINK(cdev, false);

	//PMIC mode: PWM mode(0x00) / Breath mode(0x01) / CC mode(0x10)
	mode_val = mode_val << ISINK_MODE_SHIFT;
	ret = regmap_update_bits(regmap,  ISINK_MODE(led->id),
		ISINK_MODE_MASK, mode_val);

	//PMIC ISINK enable
	mt6330_led_set_ISINK(cdev, true);

	//default mode
	if(MT_LED_PWM_MODE==mode){
		mt6330_led_set_current_step(l_info, ISINK_CH_STEP_MAX);
		mt6330_led_set_pwm_dim_freq(l_info, 0x0);
		mt6330_led_hw_brightness(cdev, LED_FULL);
	}
	mutex_unlock(&leds->lock);

	return ret;
}

/*
 * Trigger : CC mode
 */
static int mt6330_led_get_current_step(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_CH_STEP(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_CH_STEP(led->id));
		return -1;
	}

	return value;
}

static int mt6330_led_set_current_step(struct mt_led_info *info, unsigned int step)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value;
	int ret;

	if (step > 8){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, step);
		return -EINVAL;
	}

	//PMIC step
	value=step << ISINK_CH_STEP_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_CH_STEP(led->id),
		ISINK_CH_STEP_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_CH_STEP(led->id));
		return -1;
	}

	return value;
}

/*
 * Trigger : PWM mode
 */
static int mt6330_led_list_pwm_duty(struct mt_led_info *info, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%s\n", "0~255");
	return 0;
}


static int mt6330_led_get_pwm_dim_duty(struct mt_led_info *info)

{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_DIM_DUTY(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_DIM_DUTY(led->id));
		return -1;
	}

	return value;
}

static int mt6330_led_set_pwm_dim_duty(struct mt_led_info *info, unsigned int duty)

{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (duty > 255){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, duty);
		return -EINVAL;
	}

	//PWM Duty
	value=duty << ISINK_DIM_DUTY_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_DIM_DUTY(led->id),
		ISINK_DIM_DUTY_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_DIM_DUTY(led->id));
		return -1;
	}

	return ret;
}

static int mt6330_led_list_pwm_freq(struct mt_led_info *info, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%s\n", "0~65535 (500 HZ ~ 0.076HZ)");
	return 0;
}

static int mt6330_led_get_pwm_dim_freq(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value;
	unsigned int temp=0;
	int ret;

	ret = regmap_read(regmap, ISINK_DIM_FSEL_H(led->id), &temp);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_DIM_FSEL_H(led->id));
		return -1;
	}
	value = temp;

	ret = regmap_read(regmap, ISINK_DIM_FSEL_L(led->id), &temp);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_DIM_FSEL_L(led->id));
		return -1;
	}
	value = (value<<8)+temp;

	return value;
}

static int mt6330_led_set_pwm_dim_freq(struct mt_led_info *info, unsigned int freq)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value_H, value_L;

	if (freq > 0xFFFF){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, freq);
		return -EINVAL;
	}

	//PWM Frequency
	value_L=freq & ISINK_DIM_FSEL_L_MASK ;
	ret = regmap_update_bits(regmap, ISINK_DIM_FSEL_L(led->id),
		ISINK_DIM_FSEL_L_MASK, value_L);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_DIM_FSEL_L(led->id));
		return -1;
	}

	value_H=(freq>>8) & ISINK_DIM_FSEL_H_MASK ;
	ret = regmap_update_bits(regmap, ISINK_DIM_FSEL_H(led->id),
		ISINK_DIM_FSEL_H_MASK, value_H);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_DIM_FSEL_H(led->id));
		return -1;
	}

	return ret;
}

/*
 * Trigger : breath mode
 */

static int mt6330_led_get_breath_tr1(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_BREATH_TR_SEL(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_BREATH_TR_SEL(led->id));
		return -1;
	}

	value = value >> ISINK_BREATH_TR1_SEL_SHIFT;

	return value;
}

static int mt6330_led_get_breath_tr2(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_BREATH_TR_SEL(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_BREATH_TR_SEL(led->id));
		return -1;
	}

	value = value&ISINK_BREATH_TR2_SEL_MASK;

	return value;
}

static int mt6330_led_get_breath_tf1(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_BREATH_TF_SEL(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_BREATH_TF_SEL(led->id));
		return -1;
	}

	value = value >> ISINK_BREATH_TF1_SEL_SHIFT;

	return value;
}

static int mt6330_led_get_breath_tf2(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_BREATH_TF_SEL(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_BREATH_TF_SEL(led->id));
		return -1;
	}

	value = value&ISINK_BREATH_TF2_SEL_MASK;

	return value;
}

static int mt6330_led_get_breath_ton(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_BREATH_TONOFF_SEL(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_BREATH_TONOFF_SEL(led->id));
		return -1;
	}

	value = value >> ISINK_BREATH_TON_SEL_SHIFT;

	return value;
}

static int mt6330_led_get_breath_toff(struct mt_led_info *info)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int value=0;
	int ret;

	ret = regmap_read(regmap, ISINK_BREATH_TONOFF_SEL(led->id), &value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Read ERROR.\n", __func__, ISINK_BREATH_TONOFF_SEL(led->id));
		return -1;
	}

	value = value&ISINK_BREATH_TOFF_SEL_MASK;

	return value;
}

static int mt6330_led_set_breath_tr1(struct mt_led_info *info, unsigned int time)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (time > 15){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, time);
		return -EINVAL;
	}

	//mt6330_led_set_ISINK(info->cdev, false);
	//Breath Time
	value=time << ISINK_BREATH_TR1_SEL_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_BREATH_TR_SEL(led->id),
		ISINK_BREATH_TR1_SEL_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_BREATH_TR_SEL(led->id));
		return -1;
	}
	//mt6330_led_set_ISINK(info->cdev, true);

	return ret;
}

static int mt6330_led_set_breath_tr2(struct mt_led_info *info, unsigned int time)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (time > 15){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, time);
		return -EINVAL;
	}

	//mt6330_led_set_ISINK(info->cdev, false);
	//Breath Time
	value=time << ISINK_BREATH_TR2_SEL_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_BREATH_TR_SEL(led->id),
		ISINK_BREATH_TR2_SEL_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_BREATH_TR_SEL(led->id));
		return -1;
	}
	//mt6330_led_set_ISINK(info->cdev, true);

	return ret;
}

static int mt6330_led_set_breath_tf1(struct mt_led_info *info, unsigned int time)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (time > 15){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, time);
		return -EINVAL;
	}

	//Breath Time
	value=time << ISINK_BREATH_TR1_SEL_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_BREATH_TF_SEL(led->id),
		ISINK_BREATH_TF1_SEL_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_BREATH_TF_SEL(led->id));
		return -1;
	}

	return ret;
}

static int mt6330_led_set_breath_tf2(struct mt_led_info *info, unsigned int time)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (time > 15){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, time);
		return -EINVAL;
	}

	//Breath Time
	value=time << ISINK_BREATH_TF2_SEL_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_BREATH_TF_SEL(led->id),
		ISINK_BREATH_TR2_SEL_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_BREATH_TF_SEL(led->id));
		return -1;
	}

	return ret;
}

static int mt6330_led_set_breath_ton(struct mt_led_info *info, unsigned int time)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (time > 15){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, time);
		return -EINVAL;
	}

	//Breath Time
	value=time << ISINK_BREATH_TON_SEL_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_BREATH_TONOFF_SEL(led->id),
		ISINK_BREATH_TON_SEL_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_BREATH_TONOFF_SEL(led->id));
		return -1;
	}

	return ret;
}

static int mt6330_led_set_breath_toff(struct mt_led_info *info, unsigned int time)
{
	struct mt6330_led *led = container_of(info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	if (time > 15){
		dev_err(led->parent->dev, "%s: Input %d is out of range.\n", __func__, time);
		return -EINVAL;
	}

	//Breath Time
	value=time << ISINK_BREATH_TOFF_SEL_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_BREATH_TONOFF_SEL(led->id),
		ISINK_BREATH_TOFF_SEL_MASK, value);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: Reg(0x%x) Write ERROR.\n", __func__, ISINK_BREATH_TONOFF_SEL(led->id));
		return -1;
	}

	return ret;
}

struct mt_led_trig_ops mt6330_led_ops = {
	.change_mode = mt6330_led_change_mode,
	.get_current_step = mt6330_led_get_current_step,
	.set_current_step = mt6330_led_set_current_step,
	.get_pwm_dim_duty = mt6330_led_get_pwm_dim_duty,
	.set_pwm_dim_duty = mt6330_led_set_pwm_dim_duty,
	.get_pwm_dim_freq = mt6330_led_get_pwm_dim_freq,
	.set_pwm_dim_freq = mt6330_led_set_pwm_dim_freq,
	.get_breath_tr1 = mt6330_led_get_breath_tr1,
	.get_breath_tr2 = mt6330_led_get_breath_tr2,
	.get_breath_tf1 = mt6330_led_get_breath_tf1,
	.get_breath_tf2 = mt6330_led_get_breath_tf2,
	.get_breath_ton = mt6330_led_get_breath_ton,
	.get_breath_toff = mt6330_led_get_breath_toff,
	.set_breath_tr1 = mt6330_led_set_breath_tr1,
	.set_breath_tr2 = mt6330_led_set_breath_tr2,
	.set_breath_tf1 = mt6330_led_set_breath_tf1,
	.set_breath_tf2 = mt6330_led_set_breath_tf2,
	.set_breath_ton = mt6330_led_set_breath_ton,
	.set_breath_toff = mt6330_led_set_breath_toff,
	.list_pwm_duty = mt6330_led_list_pwm_duty,
	.list_pwm_freq = mt6330_led_list_pwm_freq,
};

/*
 * Setup current output for the corresponding
 * brightness level.
 */
static int mt6330_led_hw_brightness(struct led_classdev *cdev,
				    enum led_brightness brightness)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	int ret;
	unsigned int value;

	//PMIC Duty
	value=brightness << ISINK_DIM_DUTY_SHIFT;
	ret = regmap_update_bits(regmap, ISINK_DIM_DUTY(led->id),
		ISINK_DIM_DUTY_MASK, value);
	if (ret < 0)
		return ret;

	return 0;
}

static int mt6330_led_hw_off(struct led_classdev *cdev)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);

	dev_info(led->parent->dev, "%s: PMIC LED(%d) disable.\n",
		__func__, led->id);

	mt6330_led_set_current_step(l_info, ISINK_CH_STEP_MAX);
	mt6330_led_set_pwm_dim_freq(l_info, 0x0);
	mt6330_led_hw_brightness(cdev, LED_OFF);

	//PMIC ISINK disable
	return mt6330_led_set_ISINK(cdev, false);
}

static enum led_brightness
mt6330_get_led_hw_brightness(struct led_classdev *cdev)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	struct regmap *regmap = leds->regmap;
	unsigned int status_CTL0=0;
	unsigned int status_CTL1=0;
	int value=0;
	int ret=0;

	mt6330_led_get_ISINK(cdev, &status_CTL0, &status_CTL1);

	if (!(status_CTL0 & ISINK_CHOP1_EN_MASK(led->id)))
		return 0;

	if (!(status_CTL1 & ISINK_CH_EN_MASK(led->id)))
		return 0;

	ret = regmap_read(regmap, ISINK_DIM_DUTY(led->id), &value);

	if (ret < 0)
		return ret;

	return  value;
}

static int mt6330_led_hw_on(struct led_classdev *cdev,
			    enum led_brightness brightness)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	int ret = 0;

	dev_info(led->parent->dev, "%s: PMIC LED(%d) enable.\n",
		__func__, led->id);

	//PMIC ISINK enable
	mt6330_led_set_ISINK(cdev, true);

	ret = mt6330_led_hw_brightness(cdev, brightness);
	if (ret < 0)
		return ret;

	return 0;
}

static void mt6330_led_set_brightness(struct led_classdev *cdev,
				     enum led_brightness brightness)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	struct mt6330_leds *leds = led->parent;
	int ret;
	mutex_lock(&leds->lock);

	if (!led->current_brightness && brightness) {
		ret = mt6330_led_hw_on(cdev, brightness);
		if (ret < 0)
			goto out;
	} else if (brightness) {
		ret = mt6330_led_hw_brightness(cdev, brightness);
		if (ret < 0)
			goto out;
	} else {
		ret = mt6330_led_hw_off(cdev);
		if (ret < 0)
			goto out;
	}

	led->current_brightness = brightness;
out:
	mutex_unlock(&leds->lock);
}

static int mt6330_led_set_blink(struct led_classdev *cdev,
				unsigned long *delay_on,
				unsigned long *delay_off)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	unsigned long period;
	int duty;
	int freq;
	int precision=1000;
	int ret=0;

	//We do not care about delay on
	dev_info(led->parent->dev, "%s: delay_on = %lu, delay_off=%lu\n",
		__func__, *delay_on, *delay_off);

	/*
	 * Units are in ms, if over the hardware able
	 * to support, fallback into software blink
	 */
	if((*delay_on) < 0 || (*delay_off) < 0){
		dev_err(led->parent->dev, "%s: delay_on (%lu) or delay_off (%lu) is invalid value.\n",
			__func__, *delay_on, *delay_off);
		return -EINVAL;
	}

	/*
	 * LED subsystem requires a default user
	 * friendly blink pattern for the LED so using
	 * 1Hz duty cycle 50% here if without specific
	 * value delay_on and delay off being assigned.
	 */
	if(!(*delay_on) && !(*delay_off)){
		*delay_on=500;
		*delay_off=500;
	}

	period = (*delay_on) + (*delay_off);
	if (period > mt6330_MAX_PERIOD){
		dev_err(led->parent->dev, "%s: delay_on + delay_off = %lu is invalid value.\n", __func__, period);
		return -EINVAL;
	}
	dev_info(led->parent->dev, "%s: period = %lu\n", __func__, period);

	//change mode to PWM
	ret = mt6330_led_change_mode(cdev, MT_LED_PWM_MODE);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: mt6330_led_change_mode (%d) ERROR.\n", __func__, MT_LED_PWM_MODE);
		goto error;
	}

	//duty is the ratio between 1~256
	duty = precision*256*(*delay_on) / period;
	duty /= precision;
	duty = duty-1; //0~255
	dev_info(led->parent->dev, "%s: Duty = %d\n", __func__, duty);
	ret = mt6330_led_hw_brightness(cdev, duty);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: mt6330_led_hw_brightness (%d) ERROR.\n", __func__, duty);
		goto error;
	}

	//freq=(period/2) -1 , unit of period is ms
	freq = ((period/2)-1>0) ? ((period/2)-1) : 0;
	dev_info(led->parent->dev, "%s: Frequency = %d\n", __func__, freq);
	ret = mt6330_led_set_pwm_dim_freq(l_info, freq);
	if (ret < 0){
		dev_err(led->parent->dev, "%s: mt6330_led_set_pwm_dim_freq (%d) ERROR.\n", __func__, freq);
		goto error;
	}
	return 0;

error:
	//disable LED
	mt6330_led_set_ISINK(cdev, false);
	return -EIO;
}

static int mt6330_led_set_dt_default(struct led_classdev *cdev,
				     struct device_node *np)
{
	struct mt_led_info *l_info = (struct mt_led_info *)cdev;
	struct mt6330_led *led = container_of(l_info, struct mt6330_led, l_info);
	const char *state;
	int ret = 0;

	dev_info(led->parent->dev, "mt6330 parse led start\n");

	led->l_info.isink.name = of_get_property(np, "label", NULL) ? : np->name;
	led->l_info.isink.default_trigger = of_get_property(np,
		"linux,default-trigger",
		NULL);
	state = of_get_property(np, "default-state", NULL);
	if (state) {
		if (!strcmp(state, "keep")) {
			ret = mt6330_get_led_hw_brightness(cdev);
			if (ret < 0)
				return ret;
			led->current_brightness = ret;
			ret = 0;
		} else if (!strcmp(state, "on")) {
			mt6330_led_set_brightness(cdev, cdev->max_brightness);
		} else  {
			mt6330_led_set_brightness(cdev, LED_OFF);
		}
	}
	pr_info("mt6330 parse led[%d]: %s, %s, %s\n",
		led->id, led->l_info.isink.name, state, led->l_info.isink.default_trigger);

	return ret;
}

static int mt6330_led_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	struct mt6330_leds *leds;
	struct mt6330_led *led;
	int ret;
	u32 reg;

	pr_info("%s probe begain id = %d\n", __func__, pdev->id);
	dev_info(&pdev->dev, "mt6330 led probe\n");

	leds = devm_kzalloc(dev, sizeof(*leds), GFP_KERNEL);
	if (!leds)
		return -ENOMEM;

	platform_set_drvdata(pdev, leds);
	leds->dev = dev;

	/*
	 * leds->hw points to the underlying bus for the register
	 * controlled.
	 */
	leds->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!leds->regmap) {
		dev_err(&pdev->dev, "failed to allocate regmap\n");
		return -ENODEV;
	}
	mutex_init(&leds->lock);

	// Top clock 128K on
	ret = regmap_update_bits(leds->regmap, MT6330_RG_PMU128K_CK_PDN_ADDR,
		(MT6330_RG_PMU128K_CK_PDN_MASK<<MT6330_RG_PMU128K_CK_PDN_SHIFT),
		~(MT6330_RG_PMU128K_CK_PDN_MASK<<MT6330_RG_PMU128K_CK_PDN_SHIFT));
	if (ret < 0)
		return ret;

	for_each_available_child_of_node(np, child) {
		ret = of_property_read_u32(child, "reg", &reg);
		if (ret) {
			dev_err(dev, "Failed to read led 'reg' property\n");
			goto put_child_node;
		}

		if (reg >= MT6330_MAX || leds->led[reg]) {
			dev_err(dev, "Invalid led reg %u\n", reg);
			ret = -EINVAL;
			goto put_child_node;
		}

		led = devm_kzalloc(dev, sizeof(*led), GFP_KERNEL);
		if (!led) {
			ret = -ENOMEM;
			goto put_child_node;
		}

		leds->led[reg] = led;
		leds->led[reg]->id = reg;
		leds->led[reg]->l_info.isink.max_brightness = mt6330_MAX_BRIGHTNESS;
		//leds->led[reg]->l_info.cdev.brightness_set_blocking =
					//mt6330_led_set_brightness;
		leds->led[reg]->l_info.isink.brightness_set =
					mt6330_led_set_brightness;
		leds->led[reg]->l_info.isink.blink_set = mt6330_led_set_blink;
		leds->led[reg]->l_info.isink.brightness_get =
					mt6330_get_led_hw_brightness;
		leds->led[reg]->l_info.magic_code = MT_LED_SUPPORT_ALL;
		leds->led[reg]->l_info.ops = &mt6330_led_ops;
		leds->led[reg]->parent = leds;

		ret = mt6330_led_set_dt_default(&leds->led[reg]->l_info.isink, child);
		if (ret < 0) {
			dev_err(leds->dev,
				"Failed to parse LED[%d] node from devicetree\n", reg);
			goto put_child_node;
		}

		ret = devm_led_classdev_register(dev, &leds->led[reg]->l_info.isink);
		if (ret) {
			dev_err(&pdev->dev, "Failed to register LED: %d\n",
				ret);
			goto put_child_node;
		}
		leds->led[reg]->l_info.isink.dev->of_node = child;

		//clock ON
		mt6330_led_set_clock(&leds->led[reg]->l_info.isink, true);

		//default PWM mode
		mt6330_led_change_mode(&leds->led[reg]->l_info.isink, MT_LED_PWM_MODE);

		//default PWM step
		mt6330_led_set_current_step(&leds->led[reg]->l_info, ISINK_CH_STEP_MAX);

		//ISINK OFF
		mt6330_led_hw_off(&leds->led[reg]->l_info.isink);
	}
	pr_info("mt6330 led end!");

	return 0;

put_child_node:
	of_node_put(child);
	return ret;
}

static int mt6330_led_remove(struct platform_device *pdev)
{
	struct mt6330_leds *leds = platform_get_drvdata(pdev);
	int i;

	/* Turn the LEDs off on driver removal. */
	for (i = 0 ; leds->led[i] ; i++){
		//ISINK disable
		mt6330_led_hw_off(&leds->led[i]->l_info.isink);

		//clock OFF
		mt6330_led_set_clock(&leds->led[i]->l_info.isink, false);
	}


	mutex_destroy(&leds->lock);

	return 0;
}

static const struct of_device_id mt6330_led_dt_match[] = {
	{ .compatible = "mediatek,mt6330_leds" },
	{},
};
MODULE_DEVICE_TABLE(of, mt6330_led_dt_match);

static struct platform_driver mt6330_led_driver = {
	.probe		= mt6330_led_probe,
	.remove		= mt6330_led_remove,
	.driver		= {
		.name	= "leds-mt6330",
		.of_match_table = mt6330_led_dt_match,
	},
};

static int __init mt6330_leds_init(void)
{
	int ret;

	pr_info("Leds init");
	ret = platform_driver_register(&mt6330_led_driver);

	if (ret) {
		pr_info("driver register error: %d", ret);
		return ret;
	}

	return ret;
}

static void __exit mt6330_leds_exit(void)
{
	platform_driver_unregister(&mt6330_led_driver);
}

module_init(mt6330_leds_init);
module_exit(mt6330_leds_exit);

//module_platform_driver(mt6330_led_driver);

MODULE_DESCRIPTION("LED driver for Mediatek mt6330 PMIC");
MODULE_AUTHOR("Mediatek Corporation");
MODULE_LICENSE("GPL");

