// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#include <linux/regmap.h>
#include <sound/pcm_params.h>
#include "mt6880-afe-clk.h"
#include "mt6880-afe-common.h"
#include "mt6880-afe-gpio.h"
#include "mt6880-interconnection.h"

#if IS_ENABLED(CONFIG_SND_SOC_PROSLIC)
#include "../../codecs/proslic/si32185.h"
#endif

#define MTK_AFE_ETDM_KCONTROL_NAME "ETDM_HD_Mux"
#define ETDM_HD_EN_W_NAME "ETDM_HD_EN"

struct mtk_afe_etdm_priv {
	int etdm_rate;
	int low_jitter_en;
};

/* ETDMFMT_T = ETDM_TDM */
enum {
	ETDM_RATE_8K = 0x0,
	ETDM_RATE_16K = 0x2,
};

enum {
	ETDM_CONN_8K = 0x0,
	ETDM_CONN_16K = 0x4,
};

enum {
	ETDM_WLEN_8_BIT = 0x7,
	ETDM_WLEN_16_BIT = 0xf,
};

enum {
	ETDM_8_BIT = 0x7,
	ETDM_16_BIT = 0xf,
};

enum ETDM_PCM_EN {
	ETDM_PCM_EN_DISABLE = 0,
	ETDM_PCM_EN_ENABLE = 1
};

static unsigned int get_etdm_wlen(snd_pcm_format_t format)
{
	return snd_pcm_format_physical_width(format) < 16 ?
	       ETDM_WLEN_8_BIT : ETDM_WLEN_16_BIT;
}

static unsigned int get_etdm_lrck_width(snd_pcm_format_t format)
{
	return snd_pcm_format_physical_width(format) - 1;
}

static unsigned int get_etdm_ch(unsigned int rate, snd_pcm_format_t format)
{
	int etdm_ch = 0;
	if (snd_pcm_format_physical_width(format) < 16)
	{
		etdm_ch = 15;
	} else {
		if (rate == 8000)
			etdm_ch = 7;
		else
			etdm_ch = 3;
	}
	return etdm_ch;
}

static unsigned int get_etdm_rate(unsigned int rate)
{
	return (rate == 8000)? ETDM_RATE_8K : ETDM_RATE_16K;
}

static unsigned int get_etdm_inconn_rate(unsigned int rate)
{
	return (rate == 8000)? ETDM_CONN_8K : ETDM_CONN_16K;
}

static const char *const etdm_pcm0_mux_map[] = {
	"Off", "PCM",
};

static const char *const etdm_pcm1_mux_map[] = {
	"Off", "PCM",
};

static const char *const etdm_lpbk_in_mux_map[] = {
	"Off", "LPBK",
};

static const char *const etdm_lpbk_out_mux_map[] = {
	"Off", "LPBK",
};

static int etdm_pcm_mux_map_value[] = {
	ETDM_PCM_EN_DISABLE,
	ETDM_PCM_EN_ENABLE,
};

static int etdm_pcm_mux_map_invert_value[] = {
	ETDM_PCM_EN_ENABLE,
	ETDM_PCM_EN_DISABLE,
};

static SOC_VALUE_ENUM_SINGLE_DECL(etdm_pcm0_mux_map_enum,
				  ETDM_0_3_COWORK_CON0,
				  ETDM_OUT0_PCM0_MUX_SEL_SFT,
				  ETDM_OUT0_PCM0_MUX_SEL_MASK,
				  etdm_pcm0_mux_map,
				  etdm_pcm_mux_map_value);

static const struct snd_kcontrol_new etdm_pcm0_mux_control =
	SOC_DAPM_ENUM("ETDM_PCM0_MUX", etdm_pcm0_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(etdm_pcm1_mux_map_enum,
				  ETDM_0_3_COWORK_CON0,
				  ETDM_OUT0_PCM_MUX_SEL_SFT,
				  ETDM_OUT0_PCM_MUX_SEL_MASK,
				  etdm_pcm1_mux_map,
				  etdm_pcm_mux_map_value);

static const struct snd_kcontrol_new etdm_pcm1_mux_control =
	SOC_DAPM_ENUM("ETDM_PCM1_MUX", etdm_pcm1_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(
				  etdm_lpbk_in_mux_map_enum,
				  ETDM_0_3_COWORK_CON1,
				  ETDM_IN0_SDATA0_SEL_SFT,
				  ETDM_IN0_SDATA0_SEL_MASK,
				  etdm_lpbk_in_mux_map,
				  etdm_pcm_mux_map_value);

static const struct snd_kcontrol_new etdm_lpbk_in_mux_control =
	SOC_DAPM_ENUM("ETDM_LPBK_IN_MUX", etdm_lpbk_in_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(
				  etdm_lpbk_out_mux_map_enum,
				  ETDM_0_3_COWORK_CON0,
				  ETDM_IN0_SDATA0_SEL_SFT,
				  ETDM_IN0_SDATA0_SEL_MASK,
				  etdm_lpbk_out_mux_map,
				  etdm_pcm_mux_map_invert_value);

static const struct snd_kcontrol_new etdm_lpbk_out_mux_control =
	SOC_DAPM_ENUM("ETDM_LPBK_OUT_MUX", etdm_lpbk_out_mux_map_enum);

/* low jitter control */
static const char * const mt6880_etdm_hd_str[] = {
	"Normal", "Low_Jitter"
};

static const struct soc_enum mt6880_etdm_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mt6880_etdm_hd_str),
			    mt6880_etdm_hd_str),
};

static int mt6880_etdm_hd_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);

	struct mt6880_afe_private *afe_priv = afe->platform_priv;
	struct mtk_afe_etdm_priv *etdm_priv = afe_priv->dai_priv[MT6880_DAI_ETDM];

	ucontrol->value.integer.value[0] = etdm_priv->low_jitter_en;

	return 0;
}
static int mt6880_etdm_hd_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6880_afe_private *afe_priv = afe->platform_priv;
	struct mtk_afe_etdm_priv *etdm_priv = afe_priv->dai_priv[MT6880_DAI_ETDM];
	int hd_en;

	hd_en = ucontrol->value.integer.value[0];

	etdm_priv->low_jitter_en = hd_en;

	return 0;
}

static const struct snd_kcontrol_new mtk_dai_etdm_controls[] = {
	SOC_ENUM_EXT(MTK_AFE_ETDM_KCONTROL_NAME, mt6880_etdm_enum[0],
		     mt6880_etdm_hd_get, mt6880_etdm_hd_set),
};

/* dai component */
static const struct snd_kcontrol_new mtk_etdm_playback_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH1", AFE_CONN2,
				    I_DL1_CH1, 1, 0),
   	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH2", AFE_CONN2,
				    I_DL1_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN2,
				    I_ADDA_UL_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN2,
				    I_PCM_1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN2,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH2", AFE_CONN2,
				    I_PCM_2_CAP_CH2, 1, 0),
};

enum {
	SUPPLY_SEQ_ETDM_HD_EN,
	SUPPLY_SEQ_APLL,
	SUPPLY_SEQ_ETDM_EN,
	SUPPLY_SEQ_CODEC_PW,
};

static int mtk_etdm_en_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mt6880_afe_gpio_request(afe, true, MT6880_DAI_ETDM, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mt6880_afe_gpio_request(afe, false, MT6880_DAI_ETDM, 0);
		break;
	default:
		break;
	}

	return 0;
}
static int mtk_etdm_hd_en_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);

	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	return 0;
}

static int mtk_apll_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);

	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (strcmp(w->name, APLL1_W_NAME) == 0)
			mt6880_apll1_enable(afe);
		else
			mt6880_apll2_enable(afe);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		if (strcmp(w->name, APLL1_W_NAME) == 0)
			mt6880_apll1_disable(afe);
		else
			mt6880_apll2_disable(afe);
		break;
	default:
		break;
	}

	return 0;
}

static int mtk_codec_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	unsigned int value = 0, value2 = 0;
	regmap_read(afe->regmap,ETDM_IN0_CON0, &value);
	regmap_read(afe->regmap,ETDM_OUT0_CON0, &value2);
	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x ETDM_IN0_CON0 0x%x  ETDM_OUT0_CON0 0x%x\n",
		 __func__, w->name, event,
		 value, value2);

	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ProSLIC_LP(false);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		ProSLIC_LP(true);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget mtk_dai_etdm_widgets[] = {
	/* inter-connections */
	SND_SOC_DAPM_MIXER("ETDM_PB_CH1", SND_SOC_NOPM, 0, 0,
			   mtk_etdm_playback_ch1_mix,
			   ARRAY_SIZE(mtk_etdm_playback_ch1_mix)),
	/* ETDM PCM MUX */


	SND_SOC_DAPM_MUX("ETDM_LPBK_IN_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm_lpbk_in_mux_control),

	SND_SOC_DAPM_MUX("ETDM_LPBK_OUT_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm_lpbk_out_mux_control),

	/* etdm en */
	SND_SOC_DAPM_SUPPLY_S("ETDM_IN_EN", SUPPLY_SEQ_ETDM_EN,
				  ETDM_IN0_CON0, REG_ETDM_IN_EN_SFT, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("ETDM_OUT_EN", SUPPLY_SEQ_ETDM_EN,
				  ETDM_OUT0_CON0, REG_ETDM_OUT_EN_SFT, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* etdm hd en */
	SND_SOC_DAPM_SUPPLY_S(ETDM_HD_EN_W_NAME, SUPPLY_SEQ_ETDM_HD_EN,
				  ETDM_IN0_CON2, IN0_REG_CLOCK_SOURCE_SEL_SFT, 0,
				  mtk_etdm_hd_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	/* apll */
	SND_SOC_DAPM_SUPPLY_S(APLL1_W_NAME, SUPPLY_SEQ_APLL,
			      SND_SOC_NOPM, 0, 0,
			      mtk_apll_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY_S(APLL2_W_NAME, SUPPLY_SEQ_APLL,
			      SND_SOC_NOPM, 0, 0,
			      mtk_apll_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),
	/* Codec PW */
	SND_SOC_DAPM_SUPPLY_S("CODEC_PW", SUPPLY_SEQ_CODEC_PW,
			      SND_SOC_NOPM, 0, 0,
			      mtk_codec_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUTPUT("AFE_TO_ETDM"),
	SND_SOC_DAPM_MUX("ETDM_PCM0_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm_pcm0_mux_control),
	SND_SOC_DAPM_MUX("ETDM_PCM1_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm_pcm1_mux_control),
};

static int mtk_afe_etdm_hd_connect(struct snd_soc_dapm_widget *source,
				  struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6880_afe_private *afe_priv = afe->platform_priv;
	struct mtk_afe_etdm_priv *etdm_priv = afe_priv->dai_priv[MT6880_DAI_ETDM];

	return etdm_priv->low_jitter_en;
}

static int mtk_afe_etdm_apll_connect(struct snd_soc_dapm_widget *source,
				    struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6880_afe_private *afe_priv = afe->platform_priv;
	struct mtk_afe_etdm_priv *etdm_priv = afe_priv->dai_priv[MT6880_DAI_ETDM];

	int cur_apll;
	int apll;

	/* which apll */
	cur_apll = mt6880_get_apll_by_name(afe, source->name);

	apll = mt6880_get_apll_by_rate(afe, etdm_priv->etdm_rate);

	return (apll == cur_apll) ? 1 : 0;
}

static const struct snd_soc_dapm_route mtk_dai_etdm_routes[] = {

	{"ETDM_PB_CH1", "DL1_CH1", "DL1"},
	{"ETDM_PB_CH1", "DL1_CH2", "DL1"},
	{"ETDM Playback", NULL, "ETDM_PB_CH1"},

	{"ETDM_LPBK_IN_MUX", "LPBK", "ETDM Playback"},
	{"ETDM_LPBK_OUT_MUX", "LPBK", "ETDM Playback"},
	{"ETDM_LPBK_IN_MUX", "Off", "ETDM Playback"},
	{"ETDM_LPBK_OUT_MUX", "Off", "ETDM Playback"},

	{"ETDM Capture", "PCM", "ETDM_PCM0_MUX"},
	{"ETDM Capture", "PCM", "ETDM_PCM1_MUX"},
	{"ETDM Capture", NULL, "ETDM_LPBK_IN_MUX"},
	{"ETDM Capture", NULL, "ETDM_LPBK_OUT_MUX"},

	{"ETDM Playback", NULL, "ETDM_OUT_EN"},
	{"ETDM Playback", NULL, "ETDM_IN_EN"},
	{"ETDM Capture", NULL, "ETDM_OUT_EN"},
	{"ETDM Capture", NULL, "ETDM_IN_EN"},

	{"ETDM Playback", NULL, ETDM_HD_EN_W_NAME, mtk_afe_etdm_hd_connect},
	{"ETDM Capture", NULL, ETDM_HD_EN_W_NAME, mtk_afe_etdm_hd_connect},

	{ETDM_HD_EN_W_NAME, NULL, APLL1_W_NAME, mtk_afe_etdm_apll_connect},
	{ETDM_HD_EN_W_NAME, NULL, APLL2_W_NAME, mtk_afe_etdm_apll_connect},
	{"ETDM Playback", NULL, "CODEC_PW"},
	{"ETDM Capture", NULL, "CODEC_PW"},

	/* allow etdm on without codec on */
	{"ETDM_PCM0_MUX", "PCM", "ETDM Playback"},
	{"ETDM_PCM1_MUX", "PCM", "ETDM Playback"},
	{"AFE_TO_ETDM", NULL, "ETDM_PCM0_MUX"},
	{"AFE_TO_ETDM", NULL, "ETDM_PCM1_MUX"},
};

/* dai ops */
static int mtk_dai_etdm_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);

	struct mt6880_afe_private *afe_priv = afe->platform_priv;
	int etdm_id = dai->id;
	struct mtk_afe_etdm_priv *etdm_priv = afe_priv->dai_priv[etdm_id]; 
	unsigned int rate = params_rate(params);
	/* unsigned int channels = params_channels(params);8*/
	snd_pcm_format_t format = params_format(params);
	unsigned int etdm_con_in, etdm_con_out = 0;

	etdm_priv->etdm_rate = rate;

	dev_info(afe->dev, "%s(), id %d, rate %d, format %d etdm_priv->etdm_rate %d\n",
		 __func__,
		 etdm_id,
		 rate, format, etdm_priv->etdm_rate);
	if (dai->playback_widget->active || dai->capture_widget->active)
		return 0;

	/* set etdm in*/
	etdm_con_in = 0x5 << REG_FMT_SFT; /* ETDM_TDM */
	etdm_con_in |= etdm_priv->low_jitter_en << REG_RELATCH_1X_EN_SEL_DOMAIN_SFT; /* use apll */
	etdm_con_in |= get_etdm_wlen(format) << REG_WORD_LENGTH_SFT;
	etdm_con_in |= get_etdm_lrck_width(format) << REG_BIT_LENGTH_SFT;
	etdm_con_in |= get_etdm_ch(rate, format) << REG_CH_NUM_SFT;
	regmap_update_bits(afe->regmap, ETDM_IN0_CON0, 0xFFFFFFF0, etdm_con_in);

	/* use apll: 3'b1:0: h26m clock 1: apll clock */
	regmap_update_bits(afe->regmap, ETDM_IN0_CON2,
			   IN0_REG_CLOCK_SOURCE_SEL_MASK_SFT,
			   etdm_priv->low_jitter_en << IN0_REG_CLOCK_SOURCE_SEL_SFT);
	regmap_update_bits(afe->regmap, ETDM_IN0_CON3,
			   IN0_REG_FS_TIMING_SEL_MASK_SFT,
			   get_etdm_rate(rate) << IN0_REG_FS_TIMING_SEL_SFT);
	regmap_update_bits(afe->regmap, ETDM_IN0_CON4,
			   IN0_REG_RELATCH_1X_EN_SEL_MASK_SFT,
			   get_etdm_inconn_rate(rate) <<
			   IN0_REG_RELATCH_1X_EN_SEL_SFT);

	/* set etdm out*/
	etdm_con_out = 0x5 << REG_FMT_SFT; /* ETDM_TDM */
	etdm_con_out |= get_etdm_wlen(format) << REG_WORD_LENGTH_SFT;
	etdm_con_out |= get_etdm_lrck_width(format) << REG_BIT_LENGTH_SFT;
	etdm_con_out |= get_etdm_ch(rate, format) << REG_CH_NUM_SFT;
	regmap_update_bits(afe->regmap, ETDM_OUT0_CON0, 0xFFFFFFF0, etdm_con_out);

	/* use apll: 3'b1:0: h26m clock 1: apll clock */
	regmap_update_bits(afe->regmap, ETDM_OUT0_CON4,
			   OUT0_REG_CLOCK_SOURCE_SEL_MASK_SFT,
			   etdm_priv->low_jitter_en << OUT0_REG_CLOCK_SOURCE_SEL_SFT);
	regmap_update_bits(afe->regmap, ETDM_OUT0_CON4,
			   OUT0_REG_FS_TIMING_SEL_MASK_SFT,
			   get_etdm_rate(rate) << OUT0_REG_FS_TIMING_SEL_SFT);
	regmap_update_bits(afe->regmap, ETDM_OUT0_CON4,
			   OUT0_INTERCONN_OUT_EN_SEL_MASK_SFT,
			   get_etdm_inconn_rate(rate) <<
			   OUT0_INTERCONN_OUT_EN_SEL_SFT);

	regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON0,
	   ETDM_OUT0_LRCK_DIV2_MASK_SFT,
	   (rate == 8000)? 0x0 : 0x1  <<
	   ETDM_OUT0_LRCK_DIV2_SFT);

	return 0;
}

static int mtk_dai_etdm_trigger(struct snd_pcm_substream *substream,
			       int cmd,
			       struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);

	dev_info(afe->dev, "%s(), cmd %d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops mtk_dai_etdm_ops = {
	.hw_params = mtk_dai_etdm_hw_params,
	.trigger = mtk_dai_etdm_trigger,
};

/* dai driver */
#define MTK_ETDM_RATES (SNDRV_PCM_RATE_8000 |\
		       SNDRV_PCM_RATE_16000)

#define MTK_ETDM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			 SNDRV_PCM_FMTBIT_S24_LE |\
			 SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver mtk_dai_etdm_driver[] = {
	{
		.name = "ETDM",
		.id = MT6880_DAI_ETDM,
		.playback = {
			.stream_name = "ETDM Playback",
			.channels_min = 1,
			.channels_max = 1,
			.rates = MTK_ETDM_RATES,
			.formats = MTK_ETDM_FORMATS,
		},
		.capture = {
			.stream_name = "ETDM Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = MTK_ETDM_RATES,
			.formats = MTK_ETDM_FORMATS,
		},
		.ops = &mtk_dai_etdm_ops,
		.symmetric_rates = 1,
		.symmetric_samplebits = 1,
	},
};

int mt6880_dai_etdm_register(struct mtk_base_afe *afe)
{
	struct mt6880_afe_private *afe_priv = afe->platform_priv;
	struct mtk_afe_etdm_priv *etdm_priv;
	struct mtk_base_afe_dai *dai;

	dev_info(afe->dev, "%s()\n", __func__);

	dai = devm_kzalloc(afe->dev, sizeof(*dai), GFP_KERNEL);
	if (!dai)
		return -ENOMEM;

	list_add(&dai->list, &afe->sub_dais);

	dai->dai_drivers = mtk_dai_etdm_driver;
	dai->num_dai_drivers = ARRAY_SIZE(mtk_dai_etdm_driver);
	dai->controls = mtk_dai_etdm_controls;
	dai->num_controls = ARRAY_SIZE(mtk_dai_etdm_controls);
	dai->dapm_widgets = mtk_dai_etdm_widgets;
	dai->num_dapm_widgets = ARRAY_SIZE(mtk_dai_etdm_widgets);
	dai->dapm_routes = mtk_dai_etdm_routes;
	dai->num_dapm_routes = ARRAY_SIZE(mtk_dai_etdm_routes);

	etdm_priv = devm_kzalloc(afe->dev, sizeof(struct mtk_afe_etdm_priv),
				GFP_KERNEL);
	if (!etdm_priv)
		return -ENOMEM;

	etdm_priv->etdm_rate = 16000;
	afe_priv->dai_priv[MT6880_DAI_ETDM] = etdm_priv;

	return 0;
}
