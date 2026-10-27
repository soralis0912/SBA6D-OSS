/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides macros for MT6361 device bindings.
 *
 * Copyright (c) 2021 Mediatek Inc.
 * Author: Sakya Chang <jeff_chang@richtek.com>
 */

#ifndef _MT_LED_INFO
#define _MT_LED_INFO
#include <linux/leds.h>
#if IS_ENABLED(CONFIG_LEDS_CLASS_MULTICOLOR)
#include <linux/led-class-multicolor.h>
#endif /* multicolor */

#define MT_LED_SUPPORTED		(5526789 << 8)
#define MT_LED_SUPPORT_CC_MODE		(1 << 0)
#define MT_LED_SUPPORT_PWM_MODE		(1 << 1)
#define MT_LED_SUPPORT_BREATH_MODE	(1 << 2)
#define MT_LED_SUPPORT_ALL	(MT_LED_SUPPORTED|MT_LED_SUPPORT_CC_MODE| \
				MT_LED_SUPPORT_PWM_MODE|MT_LED_SUPPORT_BREATH_MODE)
enum {
	CC_MODE_ATTR_SFSTR,
};

enum {
	PWM_MODE_ATTR_DIM_DUTY,
	PWM_MODE_ATTR_DIM_FREQ,
	PWM_MODE_ATTR_LIST_DUTY,
	PWM_MODE_ATTR_LIST_FREQ,
};

enum {
	BREATH_MODE_ATTR_TR1,
	BREATH_MODE_ATTR_TR2,
	BREATH_MODE_ATTR_TF1,
	BREATH_MODE_ATTR_TF2,
	BREATH_MODE_ATTR_TON,
	BREATH_MODE_ATTR_TOFF,
	BREATH_MODE_ATTR_LIST_TIME,
};

enum {
	MT_LED_PWM_MODE,
	MT_LED_BREATH_MODE,
	MT_LED_CC_MODE,
	MT_LED_MODE_MAX,
};

struct mt_led_info;
struct mt_led_trig_ops {
	int (*change_mode)(struct led_classdev *led, int mode);
	int (*get_current_step)(struct mt_led_info *info);
	int (*set_current_step)(struct mt_led_info *info, unsigned int sel);
	int (*get_pwm_dim_duty)(struct mt_led_info *info);
	int (*set_pwm_dim_duty)(struct mt_led_info *info, unsigned int sel);
	int (*get_pwm_dim_freq)(struct mt_led_info *info);
	int (*set_pwm_dim_freq)(struct mt_led_info *info, unsigned int sel);
	int (*get_breath_tr1)(struct mt_led_info *info);
	int (*get_breath_tr2)(struct mt_led_info *info);
	int (*get_breath_tf1)(struct mt_led_info *info);
	int (*get_breath_tf2)(struct mt_led_info *info);
	int (*get_breath_ton)(struct mt_led_info *info);
	int (*get_breath_toff)(struct mt_led_info *info);
	int (*set_breath_tr1)(struct mt_led_info *info, unsigned int sel);
	int (*set_breath_tr2)(struct mt_led_info *info, unsigned int sel);
	int (*set_breath_tf1)(struct mt_led_info *info, unsigned int sel);
	int (*set_breath_tf2)(struct mt_led_info *info, unsigned int sel);
	int (*set_breath_ton)(struct mt_led_info *info, unsigned int sel);
	int (*set_breath_toff)(struct mt_led_info *info, unsigned int sel);
	int (*list_pwm_duty)(struct mt_led_info *info, char *buf);
	int (*list_pwm_freq)(struct mt_led_info *info, char *buf);
};

struct mt_led_info {
	union {
		struct led_classdev isink;
#if IS_ENABLED(CONFIG_LEDS_CLASS_MULTICOLOR)
		struct led_classdev_mc mc;
#endif /* multi color */
	};
	struct mt_led_trig_ops *ops;
	unsigned int magic_code;
};
#endif /* _MT_LED_INFO */
