/*
 * ak4940.c  --  audio driver for AK4940
 *
 * Copyright (C) 2018 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      18/08/10	    1.0 Kernel 4_9_XX
 *                      20/09/02	    1.1 Kernel 4_19_XX
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "ak4940.h"

#define AK4940_DEBUG

#ifdef AK4940_DEBUG
#define akdbgprt printk
#else
#define akdbgprt(format, arg...) do {} while (0)
#endif

/* AK4940 Codec Private Data */
struct ak4940_priv {
	struct snd_soc_component *component;
	struct i2c_client *i2c;
	struct regmap *regmap;
	int pdn_gpio;
	int fs;
	int aif_format;
	int sxxlebit;
	int bitfs;
	int dfsx;
	int ifSetMode;
};

/* ak4940 register cache & default register settings */
static const struct reg_default ak4940_reg[] = {
	{ 0x00, 0x00 },  /* AK4940_00_CLOCK_MANAGEMENT	*/
	{ 0x01, 0x00 },  /* AK4940_01_AUDIO_IF_SETTING	*/
	{ 0x02, 0x00 },  /* AK4940_02_ADC_CONTROL			*/
	{ 0x03, 0x00 },  /* AK4940_03_LINEOUT_CONTROL		*/
	{ 0x04, 0x00 },  /* AK4940_04_POWER_MANAGEMENT	*/
	{ 0x05, 0x08 },  /* AK4940_05_INPUT_GAIN			*/
	{ 0x06, 0x30 },  /* AK4940_06_ADC_DIGITAL_VOLUME  */
	{ 0x07, 0x18 },  /* AK4940_07_DAC_DIGITAL_VOLUME  */
	{ 0x08, 0x00 },  /* AK4940_08_SOFTMUTE_CONTROL	*/
};


// MIC Gain control:
// from -6 to 27 dB in 3 dB steps
static DECLARE_TLV_DB_SCALE(mgain_tlv, -600, 300, 0);

// ADC Digital volume control: (VOLAD)
// from -103.5 to 24.0 dB in 0.5 dB steps mute instead of -103.5 dB)
static DECLARE_TLV_DB_SCALE(volad_tlv, -10350, 50, 1);


// DAC Digital volume control: (VOLDA)
// (This can be used I/F output volume)
// from -115.5 to 12.0 dB in 0.5 dB steps mute instead of -115.5 dB)
static DECLARE_TLV_DB_SCALE(volda_tlv, -11550, 50, 1);

#ifdef AK4940_DEBUG
static const char * const test_reg_select[] = {
	"read AK4940 Reg 00:08",
};

static const struct soc_enum ak4940_enum2[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(test_reg_select), test_reg_select),
};
#endif

static const char * const ak4940_outaddiffn_select_texts[] = {
	"Differential", "Single-end",
};

static const struct soc_enum ak4940_outaddiffn_enum[] = {
	SOC_ENUM_SINGLE(AK4940_04_POWER_MANAGEMENT, 2,
	ARRAY_SIZE(ak4940_outaddiffn_select_texts), ak4940_outaddiffn_select_texts),
};

static const char * const ak4940_outnsel_select_texts[] = {
	"Differential", "Single-end",
};

static const struct soc_enum ak4940_outnsel_enum[] = {
	SOC_ENUM_SINGLE(AK4940_03_LINEOUT_CONTROL, 7,
		ARRAY_SIZE(ak4940_outnsel_select_texts), ak4940_outnsel_select_texts),
};

static const char * const ak4940_bckp_select_texts[] = {
	"Falling", "Rising",
};

static const struct soc_enum ak4940_bckp_enum[] = {
	SOC_ENUM_SINGLE(AK4940_02_ADC_CONTROL, 7,
		ARRAY_SIZE(ak4940_bckp_select_texts), ak4940_bckp_select_texts),
};

static const char * const ak4940_bick_freq_texts[] = {
	"64fs", "48fs", "32fs",
};

static const struct soc_enum ak4940_bickfreq_enum[] = {
	SOC_ENUM_SINGLE(AK4940_01_AUDIO_IF_SETTING, 0,
		ARRAY_SIZE(ak4940_bick_freq_texts), ak4940_bick_freq_texts),
};

static int get_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	int nTestRegNo = 0;

	/* Get the current output routing */
	ucontrol->value.enumerated.item[0] = nTestRegNo;

	return 0;
}

static int set_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	u32    currMode = ucontrol->value.enumerated.item[0];
	int    i, ret;
	int	   regs, rege;
	unsigned int value;
	int nTestRegNo = 0;

	nTestRegNo = currMode;
	regs = 0x0;
	rege = 0x09;

	for (i = regs; i < rege; i++) {
		ret = snd_soc_component_read(component, i, &value);
		if (ret < 0) {
			akdbgprt("\t[AK4940] %s(%d), ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
		akdbgprt("***AK4940 Addr,Reg=(%x, %x)\n", i, value);
	}
	return(0);
}

static const struct snd_kcontrol_new ak4940_snd_controls[] = {
	SOC_SINGLE_TLV("Mic Gain Control",
			AK4940_05_INPUT_GAIN, 0, 0x0B, 0, mgain_tlv),
	SOC_SINGLE_TLV("ADC Digital Volume (VOLAD)",
			AK4940_06_ADC_DIGITAL_VOLUME, 0, 0xFF, 1, volad_tlv),
	SOC_SINGLE_TLV("DAC Digital Volume (VOLDA)",
			AK4940_07_DAC_DIGITAL_VOLUME, 0, 0xFF, 1, volda_tlv),

	SOC_SINGLE("ADC Soft Mute Control", AK4940_08_SOFTMUTE_CONTROL, 5, 1, 0),
	SOC_SINGLE("DAC Soft Mute Control", AK4940_08_SOFTMUTE_CONTROL, 1, 1, 0),
	SOC_SINGLE("DAC Invert", AK4940_03_LINEOUT_CONTROL, 6, 1, 0),

	SOC_ENUM("ADC Input Type", ak4940_outaddiffn_enum[0]),
	SOC_ENUM("Lineout Type", ak4940_outnsel_enum[0]),
	SOC_ENUM("BICK Edge Direction", ak4940_bckp_enum[0]),
	SOC_ENUM("BICK Frequency", ak4940_bickfreq_enum[0]),

#ifdef AK4940_DEBUG
	SOC_ENUM_EXT("Reg Read", ak4940_enum2[0], get_test_reg, set_test_reg),
#endif
};

static int ak4940_ClockReset(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		akdbgprt("\t[AK4940] SND_SOC_DAPM_PRE_PMU\n");
		/* CKRESETN bit = 1 */
		snd_soc_component_update_bits(component, AK4940_00_CLOCK_MANAGEMENT, 0x08, 0x08);
		mdelay(10);
		/* CRESETN bit = 1 */
		snd_soc_component_update_bits(component, AK4940_04_POWER_MANAGEMENT, 0x01, 0x01);
		break;

	case SND_SOC_DAPM_POST_PMD:
		akdbgprt("\t[AK4940] SND_SOC_DAPM_POST_PMD\n");
		/* CKRESETN bit = 0 */
		snd_soc_component_update_bits(component, AK4940_00_CLOCK_MANAGEMENT, 0x08, 0x00);
		/* CRESETN bit = 0 */
		snd_soc_component_update_bits(component, AK4940_04_POWER_MANAGEMENT, 0x01, 0x00);
		break;
	}
	return 0;
}

static const char * const ak4940_ain_select_texts[] = {
	"AIN1", "AIN2"		/* INP1/INN1, INP2/INN2 */
};

static const struct soc_enum ak4940_ain_mux_enum =
	SOC_ENUM_SINGLE(AK4940_04_POWER_MANAGEMENT, 3,
			ARRAY_SIZE(ak4940_ain_select_texts), ak4940_ain_select_texts);

static const struct snd_kcontrol_new ak4940_ain_mux_control =
	SOC_DAPM_ENUM("AIN Select", ak4940_ain_mux_enum);

static const char * const ak4940_dacsw1_select_texts[] = {
	 "VCOM", "DIRECT", "DAC"
};

static const struct soc_enum ak4940_dacsw1_mux_enum =
	SOC_ENUM_SINGLE(AK4940_03_LINEOUT_CONTROL, 0,
			ARRAY_SIZE(ak4940_dacsw1_select_texts), ak4940_dacsw1_select_texts);

static const struct snd_kcontrol_new ak4940_dacsw1_mux_control =
	SOC_DAPM_ENUM("OUT1 MUX", ak4940_dacsw1_mux_enum);

static const char * const ak4940_dacsw2_select_texts[] = {
	 "VCOM", "DIRECT", "DAC"
};

static const struct soc_enum ak4940_dacsw2_mux_enum =
	SOC_ENUM_SINGLE(AK4940_03_LINEOUT_CONTROL, 2,
			ARRAY_SIZE(ak4940_dacsw2_select_texts), ak4940_dacsw2_select_texts);

static const struct snd_kcontrol_new ak4940_dacsw2_mux_control =
	SOC_DAPM_ENUM("OUT2 MUX", ak4940_dacsw2_mux_enum);

static const char * const ak4940_dacsw3_select_texts[] = {
	 "VCOM", "DAC Lineout(+)", "DAC Lineout(-)"
};

static const struct soc_enum ak4940_dacsw3_mux_enum =
	SOC_ENUM_SINGLE(AK4940_03_LINEOUT_CONTROL, 4,
			ARRAY_SIZE(ak4940_dacsw3_select_texts), ak4940_dacsw3_select_texts);

static const struct snd_kcontrol_new ak4940_dacsw3_mux_control =
	SOC_DAPM_ENUM("OUT3 MUX", ak4940_dacsw3_mux_enum);

static const struct snd_soc_dapm_widget ak4940_dapm_widgets[] = {
	SND_SOC_DAPM_SUPPLY_S("Clock Power", 1, SND_SOC_NOPM, 0, 0,
			 ak4940_ClockReset, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

/* ADC */
	SND_SOC_DAPM_ADC("ADC", NULL, AK4940_04_POWER_MANAGEMENT, 5, 0),

/* DAC */
	SND_SOC_DAPM_DAC("DAC", NULL, AK4940_04_POWER_MANAGEMENT, 4, 0),

/* Analog Input */
	SND_SOC_DAPM_MUX("AIN MUX", SND_SOC_NOPM, 0, 0,	&ak4940_ain_mux_control),
	SND_SOC_DAPM_INPUT("AIN1"),
	SND_SOC_DAPM_INPUT("AIN2"),

/* Analog Output */
	SND_SOC_DAPM_OUTPUT("AOUT1"),
	SND_SOC_DAPM_OUTPUT("AOUT2"),
	SND_SOC_DAPM_OUTPUT("AOUT3"),

	SND_SOC_DAPM_MUX("OUT1 MUX", SND_SOC_NOPM, 0, 0, &ak4940_dacsw1_mux_control),
	SND_SOC_DAPM_MUX("OUT2 MUX", SND_SOC_NOPM, 0, 0, &ak4940_dacsw2_mux_control),
	SND_SOC_DAPM_MUX("OUT3 MUX", SND_SOC_NOPM, 0, 0, &ak4940_dacsw3_mux_control),

	SND_SOC_DAPM_AIF_OUT("SDTO", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("SDTI", "Playback", 0, SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route ak4940_intercon[] = {
	{ "ADC", NULL, "Clock Power" },
	{ "DAC", NULL, "Clock Power" },

	{ "AIN MUX", "AIN1", "AIN1" },
	{ "AIN MUX", "AIN2", "AIN2" },
	{ "ADC", NULL, "AIN MUX" },
	{ "SDTO", NULL, "ADC" },

	{ "DAC", NULL, "SDTI" },

	{ "OUT1 MUX", "DAC", "DAC" },
	{ "OUT1 MUX", "DIRECT", "AIN1" },
	{ "AOUT1", NULL, "OUT1 MUX" },

	{ "OUT2 MUX", "DAC", "DAC" },
	{ "OUT2 MUX", "DIRECT", "AIN2" },
	{ "AOUT2", NULL, "OUT2 MUX" },

	{ "OUT3 MUX", "DAC Lineout(+)", "DAC" },
	{ "OUT3 MUX", "DAC Lineout(-)", "DAC" },
	{ "AOUT3", NULL, "OUT3 MUX" },
};

static int ak4940_set_audioif(struct snd_soc_component *component, int mode)
{
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);
	int lrif_doif, bitfs;
	int ret;
	u8 format;
	unsigned int value;	

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	ak4940->ifSetMode |= mode;

	bitfs = snd_soc_component_read(component, AK4940_01_AUDIO_IF_SETTING, &value);
	bitfs &= AK4940_MASK_BITFS;			/* D1-D0 bit */

	format = 0;
	ret = 0;
	akdbgprt("\t[AK4940] %s ak4940->aif_format(%d)\n", __func__, ak4940->aif_format);
	akdbgprt("\t[AK4940] %s bitfs(%d)\n", __func__, bitfs);
	akdbgprt("\t[AK4940] %s ak4940->sxxlebit(%d)\n", __func__, ak4940->sxxlebit);

	switch (ak4940->aif_format) {
	case SND_SOC_DAIFMT_I2S:		/* I2S I/F */
		if (bitfs == 2) {			/* 32fs */
			if (ak4940->sxxlebit == 16)
				lrif_doif = AK4940_DIF_16I2S_MODE;
			else
				ret = -EINVAL;
		} else {
			lrif_doif = AK4940_DIF_16I2S_MODE;
		}
		break;
	case SND_SOC_DAIFMT_LEFT_J:		/* Left Justified */
		if (bitfs == 0)				/* 64fs */
			lrif_doif = AK4940_DIF_24MSB_MODE;
		else
			ret = -EINVAL;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:	/* Right Justified */
		if (ak4940->sxxlebit == 16)
			format = AK4940_DIF_16LSB_MODE;
		else if (bitfs == 2)
			ret = -EINVAL;
		else if (ak4940->sxxlebit == 20)
			format = AK4940_DIF_20LSB_MODE;
		else if (ak4940->sxxlebit == 24)
			format = AK4940_DIF_24LSB_MODE;
		break;
	case SND_SOC_DAIFMT_DSP_A:		/* PCM Short Frame */
		if (bitfs == 0)				/* 64fs */
			lrif_doif = AK4940_DIF_24PCMSHORT_MODE;
		else
			ret = -EINVAL;
		break;
	case SND_SOC_DAIFMT_DSP_B:		/* PCM Long Frame */
		if (bitfs == 0)				/* 64fs */
			lrif_doif = AK4940_DIF_24PCMLONG_MODE;
		else
			ret = -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	if (ret != 0) {
		akdbgprt("\t[AK4940] %s bitfs(%d) ifSetMode(%d)\n", __func__, ret, ak4940->ifSetMode);
		if (ak4940->ifSetMode == 3) {
			ak4940->ifSetMode = 0;
			return ret;
		} else {
			return 0;
		}
	}
	/* D7-D2 bit */
	snd_soc_component_update_bits(component, AK4940_01_AUDIO_IF_SETTING,
		AK4940_MASK_LRIF_DOIF, lrif_doif);

	if (ak4940->ifSetMode == 3)
		ak4940->ifSetMode = 0;
	return 0;
}

static int ak4940_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);
	int ret;

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	ak4940->fs = params_rate(params);
	pr_info("\t[AK4940] %s fs=%d\n", __func__, ak4940->fs);

	if ((ak4940->fs == 8000) || (ak4940->fs == 7350)) {
		ak4940->dfsx = 0x00;
	} else if ((ak4940->fs == 12000) || (ak4940->fs == 11025)) {
		ak4940->dfsx = 0x01;
	} else if ((ak4940->fs == 16000) || (ak4940->fs == 14700)) {
		ak4940->dfsx = 0x02;
	} else if ((ak4940->fs == 24000) || (ak4940->fs == 22050)) {
		ak4940->dfsx = 0x03;
	} else if ((ak4940->fs == 32000) || (ak4940->fs == 29400)) {
		ak4940->dfsx = 0x04;
	} else if ((ak4940->fs == 48000) || (ak4940->fs == 44100)) {
		ak4940->dfsx = 0x05;
	} else {
		dev_err(component->dev, "sampling frequency unsupported");
		return -EINVAL;
	}
	/* DFS[2:0] */
	snd_soc_component_update_bits(component, AK4940_00_CLOCK_MANAGEMENT, 0x07, ak4940->dfsx);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		akdbgprt("\t[AK4940]S16_LE\n");
		ak4940->sxxlebit = 16;
		break;

	case SNDRV_PCM_FORMAT_S20_3LE:
		akdbgprt("\t[AK4940]S20_3LE\n");
		ak4940->sxxlebit = 20;
		break;

	case SNDRV_PCM_FORMAT_S24_LE:
		akdbgprt("\t[AK4940]S24_LE\n");
		ak4940->sxxlebit = 24;
		break;

	default:
		pr_err("%s: invalid Audio format %u\n", __func__, params_format(params));
		return -EINVAL;
	}
	ret = ak4940_set_audioif(component, 2);

	return ret;
}

static int ak4940_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	return 0;
}

static int ak4940_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	int ret;

	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:		/* BICK, LRCK (slave) */
		break;
	case SND_SOC_DAIFMT_CBM_CFM:		/* BICK, LRCK input (master) */
	case SND_SOC_DAIFMT_CBS_CFM:		/* N/A */
	case SND_SOC_DAIFMT_CBM_CFS:		/* N/A */
	default:
		dev_err(component->dev, "Clock mode unsupported master/slave");
		return -EINVAL;
	}
	ak4940->aif_format = (fmt & SND_SOC_DAIFMT_FORMAT_MASK);
	ret = ak4940_set_audioif(component, 1);

	return ret;
}

static bool ak4940_volatile(struct device *dev, unsigned int reg)
{
	bool ret;

#ifdef AK4940_DEBUG
	ret = true;
#else
	ret = false;
#endif

	return ret;
}

static bool ak4940_readable(struct device *dev, unsigned int reg)
{
	if (reg <= AK4940_MAX_REGISTER)
		return true;
	else
		return false;
}

static bool ak4940_writeable(struct device *dev, unsigned int reg)
{
	if (reg <= AK4940_MAX_REGISTER)
		return true;
	else
		return false;
}

/* * for AK4940 */
static int ak4940_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	return ret;
}

static int ak4940_set_bias_level(struct snd_soc_component *component,
		enum snd_soc_bias_level level)
{
/*	u8 reg; */
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(component);

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}

	dapm->bias_level = level;

	return 0;
}

static int ak4940_set_dai_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_component *component = dai->component;
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);
	int ret = 0;
	int ndt;

	akdbgprt("\t[AK4940] %s mute[%s]\n", __func__, mute ? "ON":"OFF");

	ndt = 4080000 / ak4940->fs;
	if (mute) {
		/* SMUTE: 1 , MUTE */
		ret = snd_soc_component_update_bits(component, AK4940_08_SOFTMUTE_CONTROL, 0x22, 0x02);
		mdelay(ndt);
	} else{
		/* SMUTE:  0  ,NORMAL operation */
		ret = snd_soc_component_update_bits(component, AK4940_08_SOFTMUTE_CONTROL, 0x22, 0x00);
		mdelay(ndt);
	}
	return ret;
}

#define AK4940_RATES	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
						SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
						SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
						SNDRV_PCM_RATE_48000)

#define AK4940_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
						SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops ak4940_dai_ops = {
	.hw_params	= ak4940_hw_params,
	.set_sysclk	= ak4940_set_dai_sysclk,
	.set_fmt	= ak4940_set_dai_fmt,
	.trigger = ak4940_trigger,
	.digital_mute = ak4940_set_dai_mute,
};

struct snd_soc_dai_driver ak4940_dai[] = {
	{
		.name = "ak4940-aif",
		.playback = {
		       .stream_name = "Playback",
		       .channels_min = 1,
		       .channels_max = 1,
		       .rates = AK4940_RATES,
		       .formats = AK4940_FORMATS,
		},
		.capture = {
		       .stream_name = "Capture",
		       .channels_min = 1,
		       .channels_max = 1,
		       .rates = AK4940_RATES,
		       .formats = AK4940_FORMATS,
		},
		.ops = &ak4940_dai_ops,
	},
};

static int ak4940_init_reg(struct snd_soc_component *component)
{
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	if (ak4940->pdn_gpio != -1) {
		gpio_set_value(ak4940->pdn_gpio, 0);
		mdelay(1);
		gpio_set_value(ak4940->pdn_gpio, 1);
		mdelay(1);
	}
	return(0);
}

static int ak4940_parse_dt(struct ak4940_priv *ak4940)
{
	struct device *dev;
	struct device_node *np;

	dev = &(ak4940->i2c->dev);

	np = dev->of_node;

	if (!np)
		return 0;

	akdbgprt("Read PDN pin from device tree\n");

	ak4940->pdn_gpio = of_get_named_gpio(np, "ak4940,pdn-gpio", 0);
	if (ak4940->pdn_gpio < 0) {
		ak4940->pdn_gpio = -1;
		return 0;
	}

	if (!gpio_is_valid(ak4940->pdn_gpio)) {
		akdbgprt(KERN_ERR "ak4940 pdn pin(%u) is invalid\n", ak4940->pdn_gpio);
		ak4940->pdn_gpio = -1;
	}
	return 0;
}

static int ak4940_probe(struct snd_soc_component *component)
{
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);
	int ret = 0;

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	ret = ak4940_parse_dt(ak4940);

	if (ak4940->pdn_gpio != -1) {
		ret = gpio_request(ak4940->pdn_gpio, "ak4940 pdn");
		akdbgprt("\t[AK4940] %s : gpio_request ret = %d\n", __func__, ret);
		gpio_direction_output(ak4940->pdn_gpio, 0);
	}
	ak4940_init_reg(component);

	ak4940->fs = 48000;
	ak4940->ifSetMode = 0;

	return ret;
}

static void ak4940_remove(struct snd_soc_component *component)
{
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	ak4940_set_bias_level(component, SND_SOC_BIAS_OFF);
	if (ak4940->pdn_gpio != -1) {
		gpio_set_value(ak4940->pdn_gpio, 0);
		mdelay(1);
		gpio_free(ak4940->pdn_gpio);
		mdelay(1);
	}

	return;
}

static int ak4940_suspend(struct snd_soc_component *component)
{
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);

	ak4940_set_bias_level(component, SND_SOC_BIAS_OFF);

	regcache_cache_only(ak4940->regmap, true);
	regcache_mark_dirty(ak4940->regmap);

	if (ak4940->pdn_gpio != -1) {
		gpio_set_value(ak4940->pdn_gpio, 0);
		akdbgprt("\t[AK4940] %s External PDN[OFF]\n", __func__);
		mdelay(1);
	}

	return 0;
}

static int ak4940_resume(struct snd_soc_component *component)
{
	struct ak4940_priv *ak4940 = snd_soc_component_get_drvdata(component);

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	if (ak4940->pdn_gpio != -1) {
		gpio_set_value(ak4940->pdn_gpio, 0);
		akdbgprt("\t[AK4940] %s External PDN[OFF]\n", __func__);
		mdelay(1);
		gpio_set_value(ak4940->pdn_gpio, 1);
		akdbgprt("\t[AK4940] %s External PDN[ON]\n", __func__);
		mdelay(1);
	}

	regcache_cache_only(ak4940->regmap, false);
	regcache_sync(ak4940->regmap);

	return 0;
}

static const struct snd_soc_component_driver soc_codec_dev_ak4940 = {
	.probe = ak4940_probe,
	.remove = ak4940_remove,
	.suspend =	ak4940_suspend,
	.resume =	ak4940_resume,

	.set_bias_level = ak4940_set_bias_level,

	.controls = ak4940_snd_controls,
	.num_controls = ARRAY_SIZE(ak4940_snd_controls),
	.dapm_widgets = ak4940_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak4940_dapm_widgets),
	.dapm_routes = ak4940_intercon,
	.num_dapm_routes = ARRAY_SIZE(ak4940_intercon),

	.idle_bias_on = 1,
	.endianness = 1,
	.non_legacy_dai_naming = 1,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_ak4940);

static const struct regmap_config ak4940_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = AK4940_MAX_REGISTER,
	.volatile_reg = ak4940_volatile,
	.writeable_reg = ak4940_writeable,
	.readable_reg = ak4940_readable,

	.reg_defaults = ak4940_reg,
	.num_reg_defaults = ARRAY_SIZE(ak4940_reg),
	.cache_type = REGCACHE_RBTREE,
};

static const struct of_device_id ak4940_i2c_dt_ids[] = {
	{ .compatible = "akm,ak4940"},
	{ }
};
MODULE_DEVICE_TABLE(of, ak4940_i2c_dt_ids);

static int ak4940_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct ak4940_priv *ak4940;
	int ret = 0;

	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	ak4940 = devm_kzalloc(&i2c->dev, sizeof(struct ak4940_priv), GFP_KERNEL);
	if (ak4940 == NULL)
		return -ENOMEM;

	ak4940->regmap = devm_regmap_init_i2c(i2c, &ak4940_regmap);
	if (IS_ERR(ak4940->regmap)) {
		devm_kfree(&i2c->dev, ak4940);
		return PTR_ERR(ak4940->regmap);
	}

	i2c_set_clientdata(i2c, ak4940);
	ak4940->i2c = i2c;

	ret = devm_snd_soc_register_component(&i2c->dev,
			&soc_codec_dev_ak4940, &ak4940_dai[0], ARRAY_SIZE(ak4940_dai));
	if (ret < 0) {
		devm_kfree(&i2c->dev, ak4940);
		akdbgprt("\t[AK4940 Error!] %s(%d)\n", __func__, __LINE__);
	}
	return ret;
}

static int ak4940_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_component(&client->dev);
	return 0;
}

static const struct i2c_device_id ak4940_i2c_id[] = {
	{ "ak4940", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak4940_i2c_id);

static struct i2c_driver ak4940_i2c_driver = {
	.driver = {
		.name = "ak4940",
		.of_match_table = of_match_ptr(ak4940_i2c_dt_ids),
	},
	.probe = ak4940_i2c_probe,
	.remove = ak4940_i2c_remove,
	.id_table = ak4940_i2c_id,
};

static int __init ak4940_modinit(void)
{
	akdbgprt("\t[AK4940] %s(%d)\n", __func__, __LINE__);

	return i2c_add_driver(&ak4940_i2c_driver);
}

module_init(ak4940_modinit);

static void __exit ak4940_exit(void)
{
	i2c_del_driver(&ak4940_i2c_driver);
}
module_exit(ak4940_exit);

MODULE_DESCRIPTION("ASoC ak4940 driver");
MODULE_LICENSE("GPL v2");
