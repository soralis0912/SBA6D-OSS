// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: Sakya Chang <jeff_chang@richtek.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/ledtrig-mediatek.h>

static int mt_led_change_mode(struct led_classdev *led, int mode)
{
	struct mt_led_info *info = (struct mt_led_info *)led;

	if (info->ops->change_mode)
		return info->ops->change_mode(led, mode);
	return -EOPNOTSUPP;
}

static ssize_t mt_led_cc_attr_show(struct device *dev,
				   struct device_attribute *attr, char *buf);
static ssize_t mt_led_cc_attr_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t cnt);

static const struct device_attribute mt_led_cc_mode_attrs[] = {
	__ATTR(soft_start_step, 0644, mt_led_cc_attr_show, mt_led_cc_attr_store),
};

static int mt_led_cc_get_current_step(struct mt_led_info *info)
{
	if (info->ops->get_current_step)
		return info->ops->get_current_step(info);
	return -EOPNOTSUPP;
}

static int mt_led_cc_set_current_step(struct mt_led_info *info,
					 unsigned long value)
{
	if (info->ops->set_current_step)
		return info->ops->set_current_step(info, value);
	return -EOPNOTSUPP;
}

static ssize_t mt_led_cc_attr_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct mt_led_info *info = (struct mt_led_info *)led_cdev;
	int ret = 0, retval = 0;
	ptrdiff_t cmd;

	cmd = attr - mt_led_cc_mode_attrs;
	buf[0] = '\0';
	switch (cmd) {
	case CC_MODE_ATTR_SFSTR:
		retval = mt_led_cc_get_current_step(info);
		break;
	default:
		return -EINVAL;
	}
	if (retval < 0)
		return retval;
	ret = snprintf(buf, PAGE_SIZE, "%d\n", retval);
	if (ret)
		dev_dbg(dev, "snprintf failed (%d)\n", ret);
	return strlen(buf);
}

static ssize_t mt_led_cc_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t cnt)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct mt_led_info *info = (struct mt_led_info *)led_cdev;
	int ret = 0;
	unsigned long value = 0;
	ptrdiff_t cmd;

	cmd = attr - mt_led_cc_mode_attrs;
	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;
	switch (cmd) {
	case CC_MODE_ATTR_SFSTR:
		ret = mt_led_cc_set_current_step(info, value);
		if (ret < 0)
			return ret;
		break;
	default:
		break;
	}
	return cnt;
}

static int mt_led_cc_activate(struct led_classdev *led)
{
	struct mt_led_info *info = (struct mt_led_info *)led;
	int i = 0, ret = 0;

	if (((info->magic_code >> 8) & 0xffffff) != 5526789 ||
		!(info->magic_code & MT_LED_SUPPORT_CC_MODE)) {
		dev_err(led->dev, "not mediatek led which support cc mode\n");
		return -EOPNOTSUPP;
	}
	for (i = 0; i < ARRAY_SIZE(mt_led_cc_mode_attrs); i++) {
		ret = device_create_file(led->dev, mt_led_cc_mode_attrs + i);
		if (ret < 0) {
			dev_err(led->dev, "%s: create file fail %d\n",
				__func__, i);
			goto out_create_file;
		}
	}
	ret = mt_led_change_mode(led, MT_LED_CC_MODE);
	if (ret < 0) {
		dev_err(led->dev, "%s: change mode fail\n", __func__);
		goto out_change_mode;
	}
	return ret;
out_change_mode:
	i = ARRAY_SIZE(mt_led_cc_mode_attrs);
out_create_file:
	while (--i > 0)
		device_remove_file(led->dev, mt_led_cc_mode_attrs + i);
	return -EINVAL;
}

static void mt_led_cc_deactivate(struct led_classdev *led)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(mt_led_cc_mode_attrs); i++)
		device_remove_file(led->dev, mt_led_cc_mode_attrs + i);
}

static ssize_t mt_led_pwm_attr_show(struct device *dev,
	struct device_attribute *attr, char *buf);
static ssize_t mt_led_pwm_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t cnt);

static const struct device_attribute mt_led_pwm_mode_attrs[] = {
	__ATTR(pwm_dim_duty, 0644, mt_led_pwm_attr_show, mt_led_pwm_attr_store),
	__ATTR(pwm_dim_freq, 0644, mt_led_pwm_attr_show, mt_led_pwm_attr_store),
	__ATTR(list_duty, 0444, mt_led_pwm_attr_show, NULL),
	__ATTR(list_freq, 0444, mt_led_pwm_attr_show, NULL),
};

static int mt_led_pwm_get_dim_duty(struct mt_led_info *info)
{
	if (info->ops->get_pwm_dim_duty)
		return info->ops->get_pwm_dim_duty(info);
	return -EOPNOTSUPP;
}

static int mt_led_pwm_set_dim_duty(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_pwm_dim_duty)
		return info->ops->set_pwm_dim_duty(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_pwm_get_dim_freq(struct mt_led_info *info)
{
	if (info->ops->get_pwm_dim_freq)
		return info->ops->get_pwm_dim_freq(info);
	return -EOPNOTSUPP;
}

static int mt_led_pwm_set_dim_freq(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_pwm_dim_freq)
		return info->ops->set_pwm_dim_freq(info, value);
	return -EOPNOTSUPP;
}

static ssize_t mt_led_pwm_attr_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct mt_led_info *info = (struct mt_led_info *)led_cdev;
	int retval = 0, ret = 0;
	ptrdiff_t cmd = attr - mt_led_pwm_mode_attrs;

	buf[0] = '\0';
	dev_info(led_cdev->dev, "%s cmd = %d\n", __func__, (int)cmd);
	switch (cmd) {
	case PWM_MODE_ATTR_DIM_DUTY:
		retval = mt_led_pwm_get_dim_duty(info);
		if (retval < 0)
			return -EINVAL;
		ret = snprintf(buf, PAGE_SIZE, "%d\n", retval);
		if (ret)
			dev_dbg(dev, "snprintf failed (%d)\n", ret);
		break;
	case PWM_MODE_ATTR_DIM_FREQ:
		retval = mt_led_pwm_get_dim_freq(info);
		if (retval < 0)
			return -EINVAL;
		ret = snprintf(buf, PAGE_SIZE, "%d\n", retval);
		if (ret)
			dev_dbg(dev, "snprintf failed (%d)\n", ret);
		break;
	case PWM_MODE_ATTR_LIST_DUTY:
		retval = info->ops->list_pwm_duty(info, buf);
		break;
	case PWM_MODE_ATTR_LIST_FREQ:
		retval = info->ops->list_pwm_freq(info, buf);
		break;
	default:
		return -EINVAL;
	}
	if (retval < 0)
		return retval;
	return strlen(buf);
}

static ssize_t mt_led_pwm_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t cnt)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct mt_led_info *info = (struct mt_led_info *)led_cdev;
	int ret = 0;
	unsigned long value = 0;
	ptrdiff_t cmd = attr - mt_led_pwm_mode_attrs;

	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;
	switch (cmd) {
	case PWM_MODE_ATTR_DIM_DUTY:
		ret = mt_led_pwm_set_dim_duty(info, value);
		break;
	case PWM_MODE_ATTR_DIM_FREQ:
		ret = mt_led_pwm_set_dim_freq(info, value);
		break;
	default:
		return -EINVAL;
	}
	if (ret < 0)
		return ret;
	return cnt;
}

static int mt_led_pwm_activate(struct led_classdev *led)
{
	struct mt_led_info *info = (struct mt_led_info *)led;
	int i = 0, ret = 0;

	if (((info->magic_code >> 8) & 0xffffff) != 5526789 ||
		!(info->magic_code & MT_LED_SUPPORT_PWM_MODE)) {
		dev_err(led->dev, "not mediatek led which support pwm mode\n");
		return -EOPNOTSUPP;
	}
	for (i = 0; i < ARRAY_SIZE(mt_led_pwm_mode_attrs); i++) {
		ret = device_create_file(led->dev, mt_led_pwm_mode_attrs + i);
		if (ret < 0) {
			dev_err(led->dev, "%s: create file fail %d\n",
				__func__, i);
			goto out_create_file;
		}
	}
	ret = mt_led_change_mode(led, MT_LED_PWM_MODE);
	if (ret < 0) {
		dev_err(led->dev, "%s change mode fail\n", __func__);
		goto out_change_mode;
	}
	return ret;
out_change_mode:
	i = ARRAY_SIZE(mt_led_pwm_mode_attrs);
out_create_file:
	while (--i > 0)
		device_remove_file(led->dev, mt_led_pwm_mode_attrs + i);
	return -EINVAL;
}

static void mt_led_pwm_deactivate(struct led_classdev *led)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(mt_led_pwm_mode_attrs); i++)
		device_remove_file(led->dev, mt_led_pwm_mode_attrs + i);
}

static ssize_t mt_led_breath_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t cnt);
static ssize_t mt_led_breath_attr_show(struct device *dev,
	struct device_attribute *attr, char *buf);

static const struct device_attribute mt_led_breath_mode_attrs[] = {
	__ATTR(tr1, 0644, mt_led_breath_attr_show, mt_led_breath_attr_store),
	__ATTR(tr2, 0644, mt_led_breath_attr_show, mt_led_breath_attr_store),
	__ATTR(tf1, 0644, mt_led_breath_attr_show, mt_led_breath_attr_store),
	__ATTR(tf2, 0644, mt_led_breath_attr_show, mt_led_breath_attr_store),
	__ATTR(ton, 0644, mt_led_breath_attr_show, mt_led_breath_attr_store),
	__ATTR(toff, 0644, mt_led_breath_attr_show, mt_led_breath_attr_store),
};

static int mt_led_breath_set_tr1(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_breath_tr1)
		return info->ops->set_breath_tr1(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_breath_set_tr2(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_breath_tr2)
		return info->ops->set_breath_tr2(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_breath_set_tf1(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_breath_tf1)
		return info->ops->set_breath_tf1(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_breath_set_tf2(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_breath_tf2)
		return info->ops->set_breath_tf2(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_breath_set_ton(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_breath_ton)
		return info->ops->set_breath_ton(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_breath_set_toff(struct mt_led_info *info, unsigned long value)
{
	if (info->ops->set_breath_toff)
		return info->ops->set_breath_toff(info, value);
	return -EOPNOTSUPP;
}

static int mt_led_breath_get_tr1(struct mt_led_info *info)
{
	if (info->ops->get_breath_tr1)
		return info->ops->get_breath_tr1(info);
	return -EOPNOTSUPP;
}

static int mt_led_breath_get_tr2(struct mt_led_info *info)
{
	if (info->ops->get_breath_tr2)
		return info->ops->get_breath_tr2(info);
	return -EOPNOTSUPP;
}

static int mt_led_breath_get_tf1(struct mt_led_info *info)
{
	if (info->ops->get_breath_tf1)
		return info->ops->get_breath_tf1(info);
	return -EOPNOTSUPP;
}

static int mt_led_breath_get_tf2(struct mt_led_info *info)
{
	if (info->ops->get_breath_tf2)
		return info->ops->get_breath_tf2(info);
	return -EOPNOTSUPP;
}

static int mt_led_breath_get_ton(struct mt_led_info *info)
{
	if (info->ops->get_breath_ton)
		return info->ops->get_breath_ton(info);
	return -EOPNOTSUPP;
}

static int mt_led_breath_get_toff(struct mt_led_info *info)
{
	if (info->ops->get_breath_toff)
		return info->ops->get_breath_toff(info);
	return -EOPNOTSUPP;
}

static ssize_t mt_led_breath_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t cnt)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct mt_led_info *info = (struct mt_led_info *)led_cdev;
	int ret = 0;
	unsigned long value = 0;
	ptrdiff_t cmd = attr - mt_led_breath_mode_attrs;

	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;
	switch (cmd) {
	case BREATH_MODE_ATTR_TR1:
		ret = mt_led_breath_set_tr1(info, value);
		break;
	case BREATH_MODE_ATTR_TR2:
		ret = mt_led_breath_set_tr2(info, value);
		break;
	case BREATH_MODE_ATTR_TF1:
		ret = mt_led_breath_set_tf1(info, value);
		break;
	case BREATH_MODE_ATTR_TF2:
		ret = mt_led_breath_set_tf2(info, value);
		break;
	case BREATH_MODE_ATTR_TON:
		ret = mt_led_breath_set_ton(info, value);
		break;
	case BREATH_MODE_ATTR_TOFF:
		ret = mt_led_breath_set_toff(info, value);
		break;
	default:
		return -EINVAL;
	}
	if (ret < 0)
		return ret;
	return cnt;
}

static int mt_led_breath_activate(struct led_classdev *led)
{
	struct mt_led_info *info = (struct mt_led_info *)led;
	int i = 0, ret = 0;

	if (((info->magic_code >> 8) & 0xffffff) != 5526789 ||
		!(info->magic_code & MT_LED_SUPPORT_BREATH_MODE)) {
		dev_err(led->dev, "not mediatek led which support breath mode\n");
		return -EOPNOTSUPP;
	}
	for (i = 0; i < ARRAY_SIZE(mt_led_breath_mode_attrs); i++) {
		ret = device_create_file(led->dev, mt_led_breath_mode_attrs + i);
		if (ret < 0) {
			dev_err(led->dev, "%s: create file fail %d\n",
				__func__, i);
			goto out_create_file;
		}
	}
	ret = mt_led_change_mode(led, MT_LED_BREATH_MODE);
	if (ret < 0) {
		dev_err(led->dev, "%s change mode fail\n", __func__);
		goto out_change_mode;
	}
	return 0;
out_change_mode:
	i = ARRAY_SIZE(mt_led_breath_mode_attrs);
out_create_file:
	while (--i > 0)
		device_remove_file(led->dev, mt_led_breath_mode_attrs + i);
	return -EINVAL;
}

static ssize_t mt_led_breath_attr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0, retval = 0;
	ptrdiff_t cmd = attr - mt_led_breath_mode_attrs;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct mt_led_info *info = (struct mt_led_info *)led_cdev;

	buf[0] = '\0';
	switch (cmd) {
	case BREATH_MODE_ATTR_TR1:
		retval = mt_led_breath_get_tr1(info);
		break;
	case BREATH_MODE_ATTR_TR2:
		retval = mt_led_breath_get_tr2(info);
		break;
	case BREATH_MODE_ATTR_TF1:
		retval = mt_led_breath_get_tf1(info);
		break;
	case BREATH_MODE_ATTR_TF2:
		retval = mt_led_breath_get_tf2(info);
		break;
	case BREATH_MODE_ATTR_TON:
		retval = mt_led_breath_get_ton(info);
		break;
	case BREATH_MODE_ATTR_TOFF:
		retval = mt_led_breath_get_toff(info);
		break;
	default:
		return -EINVAL;
	}
	if (retval < 0)
		return retval;
	ret = snprintf(buf, PAGE_SIZE, "%d\n", retval);
	if (ret)
		dev_dbg(dev, "snprintf failed (%d)\n", ret);
	return strlen(buf);
}

static void mt_led_breath_deactivate(struct led_classdev *led)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(mt_led_breath_mode_attrs); i++)
		device_remove_file(led->dev, mt_led_breath_mode_attrs + i);
}

static struct led_trigger mt_led_trigger[] = {
	{
		.name = "cc_mode",
		.activate = mt_led_cc_activate,
		.deactivate = mt_led_cc_deactivate,
	},
	{
		.name = "pwm_mode",
		.activate = mt_led_pwm_activate,
		.deactivate = mt_led_pwm_deactivate,
	},
	{
		.name = "breath_mode",
		.activate = mt_led_breath_activate,
		.deactivate = mt_led_breath_deactivate,
	},
};

static int __init ledtrig_mediatek_init(void)
{
	int ret = 0, i = 0;
	pr_info("%s\n", __func__);
	for (i = 0; i < ARRAY_SIZE(mt_led_trigger); i++) {
		ret = led_trigger_register(&mt_led_trigger[i]);
		if (ret < 0) {
			pr_err("%s register led trigger %d failed\n", __func__, i);
			goto out_led_trigger;
		}
	}
	return 0;
out_led_trigger:
	while (--i > 0)
		led_trigger_unregister(&mt_led_trigger[i]);
	return -EINVAL;
}

static void __exit ledtrig_mediatek_exit(void)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(mt_led_trigger); i++)
		led_trigger_unregister(&mt_led_trigger[i]);
}

subsys_initcall(ledtrig_mediatek_init);
module_exit(ledtrig_mediatek_exit);
MODULE_AUTHOR("Sakya Chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("Mediatek LED Trigger");
MODULE_LICENSE("GPL v2");
