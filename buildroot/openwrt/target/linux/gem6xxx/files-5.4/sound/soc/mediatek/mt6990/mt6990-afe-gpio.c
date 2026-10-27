// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>

#include "mt6990-afe-common.h"
#include "mt6990-afe-gpio.h"

struct pinctrl *aud_pinctrl;

enum mt6990_afe_gpio {
	MT6990_AFE_GPIO_I2S0_OFF,
	MT6990_AFE_GPIO_I2S0_ON,
	MT6990_AFE_GPIO_I2S1_OFF,
	MT6990_AFE_GPIO_I2S1_ON,
	MT6990_AFE_GPIO_I2S2_OFF,
	MT6990_AFE_GPIO_I2S2_ON,
	MT6990_AFE_GPIO_I2S3_OFF,
	MT6990_AFE_GPIO_I2S3_ON,
	MT6990_AFE_GPIO_I2S4_OFF,
	MT6990_AFE_GPIO_I2S4_ON,
	MT6990_AFE_GPIO_I2S5_OFF,
	MT6990_AFE_GPIO_I2S5_ON,
	MT6990_AFE_GPIO_I2S6_OFF,
	MT6990_AFE_GPIO_I2S6_ON,
	MT6990_AFE_GPIO_PROSLIC_OFF,
	MT6990_AFE_GPIO_PROSLIC_ON,
	MT6990_AFE_GPIO_TDM_OFF,
	MT6990_AFE_GPIO_TDM_ON,
	MT6990_AFE_GPIO_EXTAMP_HIGH,
	MT6990_AFE_GPIO_EXTAMP_LOW,
	MT6990_AFE_GPIO_PCM0_OFF,
	MT6990_AFE_GPIO_PCM0_ON,
	MT6990_AFE_GPIO_PCM1_OFF,
	MT6990_AFE_GPIO_PCM1_ON,
	MT6990_AFE_GPIO_VOLTAGE_ON,
	MT6990_AFE_GPIO_VOLTAGE_OFF,
	MT6990_AFE_GPIO_GPIO_NUM
};

struct audio_gpio_attr {
	const char *name;
	bool gpio_prepare;
	struct pinctrl_state *gpioctrl;
};

static struct audio_gpio_attr aud_gpios[MT6990_AFE_GPIO_GPIO_NUM] = {
	[MT6990_AFE_GPIO_I2S0_OFF] = {"aud_gpio_i2s0_off", false, NULL},
	[MT6990_AFE_GPIO_I2S0_ON] = {"aud_gpio_i2s0_on", false, NULL},
	[MT6990_AFE_GPIO_I2S1_OFF] = {"aud_gpio_i2s1_off", false, NULL},
	[MT6990_AFE_GPIO_I2S1_ON] = {"aud_gpio_i2s1_on", false, NULL},
	[MT6990_AFE_GPIO_I2S2_OFF] = {"aud_gpio_i2s2_off", false, NULL},
	[MT6990_AFE_GPIO_I2S2_ON] = {"aud_gpio_i2s2_on", false, NULL},
	[MT6990_AFE_GPIO_I2S3_OFF] = {"aud_gpio_i2s3_off", false, NULL},
	[MT6990_AFE_GPIO_I2S3_ON] = {"aud_gpio_i2s3_on", false, NULL},
	[MT6990_AFE_GPIO_I2S4_OFF] = {"aud_gpio_i2s4_off", false, NULL},
	[MT6990_AFE_GPIO_I2S4_ON] = {"aud_gpio_i2s4_on", false, NULL},
	[MT6990_AFE_GPIO_I2S5_OFF] = {"aud_gpio_i2s5_off", false, NULL},
	[MT6990_AFE_GPIO_I2S5_ON] = {"aud_gpio_i2s5_on", false, NULL},
	[MT6990_AFE_GPIO_I2S6_OFF] = {"aud_gpio_i2s6_off", false, NULL},
	[MT6990_AFE_GPIO_I2S6_ON] = {"aud_gpio_i2s6_on", false, NULL},
	[MT6990_AFE_GPIO_PROSLIC_OFF] = {"aud_gpio_proslic_off", false, NULL},
	[MT6990_AFE_GPIO_PROSLIC_ON] = {"aud_gpio_proslic_on", false, NULL},
	[MT6990_AFE_GPIO_TDM_OFF] = {"aud_gpio_tdm_off", false, NULL},
	[MT6990_AFE_GPIO_TDM_ON] = {"aud_gpio_tdm_on", false, NULL},
	[MT6990_AFE_GPIO_EXTAMP_HIGH] = {"extamp-pullhigh", false, NULL},
	[MT6990_AFE_GPIO_EXTAMP_LOW] = {"extamp-pulllow", false, NULL},
	[MT6990_AFE_GPIO_PCM0_OFF] = {"aud_gpio_pcm0_off", false, NULL},
	[MT6990_AFE_GPIO_PCM0_ON] = {"aud_gpio_pcm0_on", false, NULL},
	[MT6990_AFE_GPIO_PCM1_OFF] = {"aud_gpio_pcm1_off", false, NULL},
	[MT6990_AFE_GPIO_PCM1_ON] = {"aud_gpio_pcm1_on", false, NULL},
	[MT6990_AFE_GPIO_VOLTAGE_ON] = {"aud_gpio_voltage_on", false, NULL},
	[MT6990_AFE_GPIO_VOLTAGE_OFF] = {"aud_gpio_voltage_off", false, NULL},
};

static DEFINE_MUTEX(gpio_request_mutex);

int mt6990_afe_gpio_init(struct mtk_base_afe *afe)
{
	int ret;
	int i = 0;

	aud_pinctrl = devm_pinctrl_get(afe->dev);
	if (IS_ERR(aud_pinctrl)) {
		ret = PTR_ERR(aud_pinctrl);
		dev_err(afe->dev, "%s(), ret %d, cannot get aud_pinctrl!\n",
			__func__, ret);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(aud_gpios); i++) {
		aud_gpios[i].gpioctrl = pinctrl_lookup_state(aud_pinctrl,
							     aud_gpios[i].name);
		if (IS_ERR(aud_gpios[i].gpioctrl)) {
			ret = PTR_ERR(aud_gpios[i].gpioctrl);
			dev_err(afe->dev, "%s(), pinctrl_lookup_state %s fail, ret %d\n",
				__func__, aud_gpios[i].name, ret);
		} else {
			aud_gpios[i].gpio_prepare = true;
		}
	}

	return 0;
}

static int mt6990_afe_gpio_select(struct mtk_base_afe *afe,
				  enum mt6990_afe_gpio type)
{
	int ret = 0;

	if (type >= MT6990_AFE_GPIO_GPIO_NUM) {
		dev_err(afe->dev, "%s(), error, invaild gpio type %d\n",
			__func__, type);
		return -EINVAL;
	}

	if (!aud_gpios[type].gpio_prepare) {
		dev_warn(afe->dev, "%s(), error, gpio type %d not prepared\n",
			 __func__, type);
		return -EIO;
	}

	ret = pinctrl_select_state(aud_pinctrl,
				   aud_gpios[type].gpioctrl);
	if (ret)
		dev_err(afe->dev, "%s(), error, can not set gpio type %d\n",
			__func__, type);

	return ret;
}

int mt6990_afe_gpio_request(struct mtk_base_afe *afe, bool enable,
			    int dai, int uplink)
{
	mutex_lock(&gpio_request_mutex);
	switch (dai) {
	case MT6990_DAI_I2S_0:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S0_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S0_OFF);
		break;
	case MT6990_DAI_I2S_1:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S1_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S1_OFF);
		break;
	case MT6990_DAI_I2S_2:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S2_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S2_OFF);
		break;
	case MT6990_DAI_I2S_3:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S3_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S3_OFF);
		break;
#if 0
	case MT6990_DAI_I2S_4:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S4_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S4_OFF);
		break;
	case MT6990_DAI_I2S_5:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S5_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S5_OFF);
		break;
#endif
	case MT6990_DAI_I2S_6:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S6_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S6_OFF);
		break;
	case MT6990_DAI_ETDM:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_PROSLIC_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_PROSLIC_OFF);
		break;
	case MT6990_DAI_ETDM1:
		if (enable) {
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S4_ON);
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S5_ON);
		} else {
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S4_OFF);
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_I2S5_OFF);
		}
		break;
	case MT6990_DAI_TDM:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_TDM_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_TDM_OFF);
		break;
	case MT6990_DAI_EXAMP:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_EXTAMP_HIGH);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_EXTAMP_LOW);
		break;
	case MT6990_DAI_PCM_0:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_PCM0_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_PCM0_OFF);
		break;
	case MT6990_DAI_PCM_1:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_PCM1_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_PCM1_OFF);
		break;
	case MT6990_DAI_VOLTAGE:
		if (enable)
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_VOLTAGE_ON);
		else
			mt6990_afe_gpio_select(afe, MT6990_AFE_GPIO_VOLTAGE_OFF);
		break;
	default:
		mutex_unlock(&gpio_request_mutex);
		dev_warn(afe->dev, "%s(), invalid dai %d\n", __func__, dai);
		return -EINVAL;
	}
	mutex_unlock(&gpio_request_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(mt6990_afe_gpio_request);

