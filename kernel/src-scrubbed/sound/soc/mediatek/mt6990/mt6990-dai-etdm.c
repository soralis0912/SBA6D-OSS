// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#include <linux/regmap.h>
#include <sound/pcm_params.h>
#include "mt6990-afe-clk.h"
#include "mt6990-afe-common.h"
#include "mt6990-afe-gpio.h"
#include "mt6990-interconnection.h"

#if IS_ENABLED(CONFIG_SND_SOC_PROSLIC)
#include "../../codecs/proslic/si321xx.h"
#include "../../codecs/proslic/si32185_2.h"
#include "../../codecs/proslic/proslic_sys.h"
#endif

#define MTK_AFE_ETDM_KCONTROL_NAME "ETDM_HD_Mux"
#define MTK_AFE_ETDM1_KCONTROL_NAME "ETDM1_HD_Mux"
#define MTK_AFE_ETDM_I2S_MODE "ETDM1_I2S_Mode"

#define ETDM_HD_EN_W_NAME "ETDM_HD_EN"
#define ETDM1_HD_EN_W_NAME "ETDM1_HD_EN"

#define ETDM1_MCLK_EN_W_NAME "ETDM1_MCLK_EN"

struct mtk_afe_etdm_priv {
	int id;
	int etdm_rate;
	int low_jitter_en;
	int i2s_mode;

	int mclk_id;
	int mclk_rate;
	int mclk_apll;
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

enum {
	ETDM_PCM_FMT_I2S = 0,
	ETDM_PCM_FMT_LJ = 1,
	ETDM_PCM_FMT_RJ = 2,
	ETDM_PCM_FMT_EAIJ = 3,
	ETDM_PCM_FMT_PCMA = 4,
	ETDM_PCM_FMT_PCMB = 5,
	ETDM_PCM_FMT_DSD = 6,
};

enum {
	ETDM_PCM_CLK_512K = 2,
	ETDM_PCM_CLK_768K = 3,
	ETDM_PCM_CLK_1024K = 4,
	ETDM_PCM_CLK_1536K = 6,
	ETDM_PCM_CLK_2048K = 8,
};

static unsigned int need_invert_bclk(void)
{
	return 1;
}

static unsigned int need_fsync_remain_8k(void)
{
	int ret = 0;
	if(quectel_slic_id == QUECTEL_SLIC_LE9643)
	{
		ret = 0;
	}
	else if(quectel_slic_id == QUECTEL_SLIC_SI321XX)
	{
		ret = 1;
	}
	else
	{
		ret = 1;	//dxs101
	}
	return ret;
}

static unsigned int get_etdm_format(void)
{
	unsigned int ret = ETDM_PCM_FMT_PCMB;
	if(quectel_slic_id == QUECTEL_SLIC_LE9643)
	{
		ret = ETDM_PCM_FMT_PCMA;
	}
	else if(quectel_slic_id == QUECTEL_SLIC_SI321XX)
	{
		ret = ETDM_PCM_FMT_PCMB;
	}
	else
	{
		ret = ETDM_PCM_FMT_PCMA;	//dxs101
	}
	return ret;
}

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
	int pclk_f = ETDM_PCM_CLK_2048K;
	int etdm_ch = 0;
	if (snd_pcm_format_physical_width(format) < 16)
	{
		etdm_ch = pclk_f * 4 - 1 ;
	} else {
		if (rate == 8000)
			etdm_ch = pclk_f * 2 - 1;
		else
			etdm_ch = pclk_f - 1;
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
				  ETDM_0_3_COWORK_CON3,
				  ETDM_OUT0_PCM0_MUX_SEL_SFT,
				  ETDM_OUT0_PCM0_MUX_SEL_MASK,
				  etdm_pcm0_mux_map,
				  etdm_pcm_mux_map_value);

static const struct snd_kcontrol_new etdm_pcm0_mux_control =
	SOC_DAPM_ENUM("ETDM_PCM0_MUX", etdm_pcm0_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(etdm_pcm1_mux_map_enum,
				  ETDM_0_3_COWORK_CON3,
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
				  ETDM_OUT0_DATA_SEL_SFT,
				  ETDM_OUT0_DATA_SEL_MASK,
				  etdm_lpbk_out_mux_map,
				  etdm_pcm_mux_map_invert_value);

static const struct snd_kcontrol_new etdm_lpbk_out_mux_control =
	SOC_DAPM_ENUM("ETDM_LPBK_OUT_MUX", etdm_lpbk_out_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(
				  etdm1_lpbk_in_mux_map_enum,
				  ETDM_0_3_COWORK_CON1,
				  ETDM_IN1_SDATA0_SEL_SFT,
				  ETDM_IN1_SDATA0_SEL_MASK,
				  etdm_lpbk_in_mux_map,
				  etdm_pcm_mux_map_value);

static const struct snd_kcontrol_new etdm1_lpbk_in_mux_control =
	SOC_DAPM_ENUM("ETDM1_LPBK_IN_MUX", etdm1_lpbk_in_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(
				  etdm1_lpbk_out_mux_map_enum,
				  ETDM_0_3_COWORK_CON0,
				  ETDM_OUT1_DATA_SEL_SFT,
				  ETDM_OUT1_DATA_SEL_MASK,
				  etdm_lpbk_out_mux_map,
				  etdm_pcm_mux_map_invert_value);

static const struct snd_kcontrol_new etdm1_lpbk_out_mux_control =
	SOC_DAPM_ENUM("ETDM1_LPBK_OUT_MUX", etdm1_lpbk_out_mux_map_enum);

static int get_etdm_id_by_name(struct mtk_base_afe *afe,
			      const char *name)
{
	if (strncmp(name, "ETDM1", 5) == 0)
		return MT6990_DAI_ETDM1;
	else if (strncmp(name, "ETDM", 4) == 0)
		return MT6990_DAI_ETDM;
	else
		return -EINVAL;
}

static struct mtk_afe_etdm_priv *get_etdm_priv_by_name(struct mtk_base_afe *afe,
						     const char *name)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int dai_id = get_etdm_id_by_name(afe, name);

	if (dai_id < 0 || dai_id >= MT6990_DAI_NUM)
		return NULL;

	return afe_priv->dai_priv[dai_id];
}

/* low jitter control */
static const char * const mt6990_etdm_hd_str[] = {
	"Normal", "Low_Jitter"
};

/* I2S mode control */
static const char * const mt6990_etdm_i2s_str[] = {
	"Normal", "I2S"
};

static const struct soc_enum mt6990_etdm_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mt6990_etdm_hd_str),
			    mt6990_etdm_hd_str),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mt6990_etdm_i2s_str),
			    mt6990_etdm_i2s_str),
};

static int mt6990_etdm_hd_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);

	struct mtk_afe_etdm_priv *etdm_priv;

	etdm_priv = get_etdm_priv_by_name(afe, kcontrol->id.name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	ucontrol->value.integer.value[0] = etdm_priv->low_jitter_en;

	return 0;
}
static int mt6990_etdm_hd_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;
	int hd_en;

	hd_en = ucontrol->value.integer.value[0];

	etdm_priv = get_etdm_priv_by_name(afe, kcontrol->id.name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	etdm_priv->low_jitter_en = hd_en;

	return 0;
}

static int mt6990_etdm_i2s_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);

	struct mtk_afe_etdm_priv *etdm_priv;

	etdm_priv = get_etdm_priv_by_name(afe, kcontrol->id.name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	ucontrol->value.integer.value[0] = etdm_priv->i2s_mode;

	return 0;
}
static int mt6990_etdm_i2s_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;
	int i2s_mode;

	i2s_mode = ucontrol->value.integer.value[0];

	etdm_priv = get_etdm_priv_by_name(afe, kcontrol->id.name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	etdm_priv->i2s_mode = i2s_mode;

	return 0;
}

static const struct snd_kcontrol_new mtk_dai_etdm_controls[] = {
	SOC_ENUM_EXT(MTK_AFE_ETDM_KCONTROL_NAME, mt6990_etdm_enum[0],
		     mt6990_etdm_hd_get, mt6990_etdm_hd_set),
	SOC_ENUM_EXT(MTK_AFE_ETDM1_KCONTROL_NAME, mt6990_etdm_enum[0],
		     mt6990_etdm_hd_get, mt6990_etdm_hd_set),
	SOC_ENUM_EXT(MTK_AFE_ETDM_I2S_MODE, mt6990_etdm_enum[1],
		     mt6990_etdm_i2s_get, mt6990_etdm_i2s_set),
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

static const struct snd_kcontrol_new mtk_etdm1_playback_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH1", AFE_CONN30,
				    I_DL1_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH2", AFE_CONN30,
				    I_DL1_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH1", AFE_CONN30,
				    I_DL2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH2", AFE_CONN30,
				    I_DL2_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN30,
				    I_ADDA_UL_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN30,
				    I_PCM_1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN30,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH2", AFE_CONN30,
				    I_PCM_2_CAP_CH2, 1, 0),
};

static const struct snd_kcontrol_new mtk_etdm1_playback_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH1", AFE_CONN31,
				    I_DL1_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH2", AFE_CONN31,
				    I_DL1_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH1", AFE_CONN31,
				    I_DL2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH2", AFE_CONN31,
				    I_DL2_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN31,
				    I_ADDA_UL_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN31,
				    I_PCM_1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN31,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH2", AFE_CONN31,
				    I_PCM_2_CAP_CH2, 1, 0),
};

enum {
	SUPPLY_SEQ_ETDM_MCLK_EN,
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
	struct mtk_afe_etdm_priv *etdm_priv;
	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	etdm_priv = get_etdm_priv_by_name(afe, w->name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mt6990_afe_gpio_request(afe, true, etdm_priv->id, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mt6990_afe_gpio_request(afe, false, etdm_priv->id, 0);
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
			mt6990_apll1_enable(afe);
		else
			mt6990_apll2_enable(afe);
		break;
	case SND_SOC_DAPM_POST_PMD:
		//if (strcmp(w->name, APLL1_W_NAME) == 0)
		//	mt6990_apll1_disable(afe);
		//else
		//	mt6990_apll2_disable(afe);
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
#if defined(CONFIG_SND_SOC_PROSLIC)
		//ProSLIC_LP(false);
#endif
		break;
	case SND_SOC_DAPM_PRE_PMD:
#if defined(CONFIG_SND_SOC_PROSLIC)
		//ProSLIC_LP(true);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int mtk_codec_event1(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	unsigned int value = 0, value2 = 0;
	regmap_read(afe->regmap,ETDM_IN1_CON0, &value);
	regmap_read(afe->regmap,ETDM_OUT1_CON0, &value2);
	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x ETDM_IN1_CON0 0x%x  ETDM_OUT1_CON0 0x%x\n",
		 __func__, w->name, event,
		 value, value2);

	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
#if defined(CONFIG_SND_SOC_PROSLIC)
        //ProSLIC_LP2(false);
#endif
		break;
	case SND_SOC_DAPM_PRE_PMD:
#if defined(CONFIG_SND_SOC_PROSLIC)
        //ProSLIC_LP2(true);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int mtk_mclk_en_event(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol,
			     int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;

	dev_info(cmpnt->dev, "%s(), name %s, event 0x%x\n",
		 __func__, w->name, event);

	etdm_priv = get_etdm_priv_by_name(afe, w->name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mt6990_mck_enable(afe, etdm_priv->mclk_id, etdm_priv->mclk_rate);
		break;
	case SND_SOC_DAPM_POST_PMD:
		//etdm_priv->mclk_rate = 0;
		//mt6990_mck_disable(afe, etdm_priv->mclk_id);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget mtk_dai_etdm_widgets[] = {
	/* ETDM0 */
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
#if 1
	SND_SOC_DAPM_SUPPLY_S("ETDM_IN_EN", SUPPLY_SEQ_ETDM_EN,
				  ETDM_IN0_CON0, REG_ETDM_IN_EN_SFT, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("ETDM_OUT_EN", SUPPLY_SEQ_ETDM_EN,
				  ETDM_OUT0_CON0, REG_ETDM_OUT_EN_SFT, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#else
	SND_SOC_DAPM_SUPPLY_S("ETDM_IN_EN", SUPPLY_SEQ_ETDM_EN,
				  SND_SOC_NOPM, 0, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("ETDM_OUT_EN", SUPPLY_SEQ_ETDM_EN,
				  SND_SOC_NOPM, 0, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#endif

	SND_SOC_DAPM_CLOCK_SUPPLY("aud_etdm0_clk"),

	/* etdm hd en */
	SND_SOC_DAPM_SUPPLY_S(ETDM_HD_EN_W_NAME, SUPPLY_SEQ_ETDM_HD_EN,
				  ETDM_IN0_CON2, IN0_REG_CLOCK_SOURCE_SEL_SFT, 0,
				  mtk_etdm_hd_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	/* apll */
	SND_SOC_DAPM_SUPPLY_S(APLL1_W_NAME, SUPPLY_SEQ_APLL,
			      SND_SOC_NOPM, 0, 0,
			      mtk_apll_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S(APLL2_W_NAME, SUPPLY_SEQ_APLL,
			      SND_SOC_NOPM, 0, 0,
			      mtk_apll_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	/* Codec PW */
	SND_SOC_DAPM_SUPPLY_S("CODEC_PW", SUPPLY_SEQ_CODEC_PW,
			      SND_SOC_NOPM, 0, 0,
			      mtk_codec_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_SUPPLY_S("CODEC_PW1", SUPPLY_SEQ_CODEC_PW,
			      SND_SOC_NOPM, 0, 0,
			      mtk_codec_event1,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUTPUT("AFE_TO_ETDM"),
	SND_SOC_DAPM_MUX("ETDM_PCM0_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm_pcm0_mux_control),
	SND_SOC_DAPM_MUX("ETDM_PCM1_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm_pcm1_mux_control),


	/* ETDM1 */
	/* inter-connections */
	SND_SOC_DAPM_MIXER("ETDM1_PB_CH1", SND_SOC_NOPM, 0, 0,
			   mtk_etdm1_playback_ch1_mix,
			   ARRAY_SIZE(mtk_etdm1_playback_ch1_mix)),
	SND_SOC_DAPM_MIXER("ETDM1_PB_CH2", SND_SOC_NOPM, 0, 0,
			   mtk_etdm1_playback_ch2_mix,
			   ARRAY_SIZE(mtk_etdm1_playback_ch1_mix)),
	/* ETDM PCM MUX */


	SND_SOC_DAPM_MUX("ETDM1_LPBK_IN_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm1_lpbk_in_mux_control),

	SND_SOC_DAPM_MUX("ETDM1_LPBK_OUT_MUX", SND_SOC_NOPM, 0, 0,
			 &etdm1_lpbk_out_mux_control),

	/* etdm en */
#if 1
	SND_SOC_DAPM_SUPPLY_S("ETDM1_IN_EN", SUPPLY_SEQ_ETDM_EN,
				  ETDM_IN1_CON0, REG_ETDM_IN_EN_SFT, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("ETDM1_OUT_EN", SUPPLY_SEQ_ETDM_EN,
				  ETDM_OUT1_CON0, REG_ETDM_OUT_EN_SFT, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#else
	SND_SOC_DAPM_SUPPLY_S("ETDM1_IN_EN", SUPPLY_SEQ_ETDM_EN,
				  SND_SOC_NOPM, 0, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("ETDM1_OUT_EN", SUPPLY_SEQ_ETDM_EN,
				  SND_SOC_NOPM, 0, 0,
				  mtk_etdm_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#endif


	/* etdm hd en */
#if 1
	SND_SOC_DAPM_SUPPLY_S(ETDM1_HD_EN_W_NAME, SUPPLY_SEQ_ETDM_HD_EN,
				  ETDM_IN1_CON2, IN0_REG_CLOCK_SOURCE_SEL_SFT, 0,
				  mtk_etdm_hd_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#else
	SND_SOC_DAPM_SUPPLY_S(ETDM_HD_EN_W_NAME, SUPPLY_SEQ_ETDM_HD_EN,
				  SND_SOC_NOPM, 0, 0,
				  mtk_etdm_hd_en_event,
				  SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#endif


	/* i2s mclk en */
	SND_SOC_DAPM_SUPPLY_S(ETDM1_MCLK_EN_W_NAME, SUPPLY_SEQ_ETDM_MCLK_EN,
			      SND_SOC_NOPM, 0, 0,
			      mtk_mclk_en_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_OUTPUT("AFE_TO_ETDM1"),
};

static int mtk_afe_etdm_hd_connect(struct snd_soc_dapm_widget *source,
				  struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;

	etdm_priv = get_etdm_priv_by_name(afe, w->name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return 0;
	}

	return etdm_priv->low_jitter_en;
}

static int mtk_afe_etdm_apll_connect(struct snd_soc_dapm_widget *source,
				    struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;
	int cur_apll;
	int apll;

	etdm_priv = get_etdm_priv_by_name(afe, w->name);
	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return 0;
	}

	/* which apll */
	cur_apll = mt6990_get_apll_by_name(afe, source->name);

	apll = mt6990_get_apll_by_rate(afe, etdm_priv->etdm_rate);

	return (apll == cur_apll) ? 1 : 0;
}

static int mtk_afe_etdm_mclk_connect(struct snd_soc_dapm_widget *source,
				    struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;

	etdm_priv = get_etdm_priv_by_name(afe, sink->name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return 0;
	}

	return (etdm_priv->mclk_rate > 0) ? 1 : 0;
}

static int mtk_afe_mclk_apll_connect(struct snd_soc_dapm_widget *source,
				     struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mtk_afe_etdm_priv *etdm_priv;
	int cur_apll;

	etdm_priv = get_etdm_priv_by_name(afe, w->name);

	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return 0;
	}

	/* which apll */
	cur_apll = mt6990_get_apll_by_name(afe, source->name);

	return (etdm_priv->mclk_apll == cur_apll) ? 1 : 0;
}


static const struct snd_soc_dapm_route mtk_dai_etdm_routes[] = {
	/* ETDM0 */
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

	{"ETDM Playback", NULL, "aud_etdm0_clk"},
	{"ETDM Capture", NULL, "aud_etdm0_clk"},

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

	/* ETDM1 */
	{"ETDM1_PB_CH1", "DL1_CH1", "DL1"},
	{"ETDM1_PB_CH1", "DL1_CH2", "DL1"},
	{"ETDM1_PB_CH1", "DL2_CH1", "DL2"},
	{"ETDM1_PB_CH1", "DL2_CH2", "DL2"},
	{"ETDM1 Playback", NULL, "ETDM1_PB_CH1"},

	{"ETDM1_LPBK_IN_MUX", "LPBK", "ETDM1 Playback"},
	{"ETDM1_LPBK_OUT_MUX", "LPBK", "ETDM1 Playback"},
	{"ETDM1_LPBK_IN_MUX", "Off", "ETDM1 Playback"},
	{"ETDM1_LPBK_OUT_MUX", "Off", "ETDM1 Playback"},

	{"ETDM1 Capture", NULL, "ETDM1_LPBK_IN_MUX"},
	{"ETDM1 Capture", NULL, "ETDM1_LPBK_OUT_MUX"},

	{"ETDM1 Playback", NULL, "ETDM1_OUT_EN"},
	{"ETDM1 Playback", NULL, "ETDM1_IN_EN"},
	{"ETDM1 Capture", NULL, "ETDM1_OUT_EN"},
	{"ETDM1 Capture", NULL, "ETDM1_IN_EN"},

	{"ETDM1 Playback", NULL, ETDM1_HD_EN_W_NAME, mtk_afe_etdm_hd_connect},
	{"ETDM1 Capture", NULL, ETDM1_HD_EN_W_NAME, mtk_afe_etdm_hd_connect},

	{ETDM1_HD_EN_W_NAME, NULL, APLL1_W_NAME, mtk_afe_etdm_apll_connect},
	{ETDM1_HD_EN_W_NAME, NULL, APLL2_W_NAME, mtk_afe_etdm_apll_connect},
	{"ETDM1 Playback", NULL, "CODEC_PW1"},
	{"ETDM1 Capture", NULL, "CODEC_PW1"},

	/* allow etdm on without codec on */
	{"AFE_TO_ETDM", NULL, "ETDM1 Playback"},

	{"ETDM1 Playback", NULL, ETDM1_MCLK_EN_W_NAME, mtk_afe_etdm_mclk_connect},
	{ETDM1_MCLK_EN_W_NAME, NULL, APLL1_W_NAME, mtk_afe_mclk_apll_connect},
};

/* dai ops */
static int mtk_dai_etdm_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);

	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int etdm_id = dai->id;
	struct mtk_afe_etdm_priv *etdm_priv;
	unsigned int rate = params_rate(params);
	/* unsigned int channels = params_channels(params);8*/
	snd_pcm_format_t format = params_format(params);
	unsigned int etdm_con_in, etdm_con_out = 0;

	if (etdm_id < 0 || etdm_id >= MT6990_DAI_NUM)
		return -EINVAL;
	etdm_priv = afe_priv->dai_priv[etdm_id];
	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	etdm_priv->etdm_rate = rate;

	dev_info(afe->dev, "%s(), id %d, rate %d, format %d etdm_priv->etdm_rate %d\n",
		 __func__,
		 etdm_id,
		 rate, format, etdm_priv->etdm_rate);
	if (dai->playback_widget->active || dai->capture_widget->active)
		return 0;

	if (etdm_id == MT6990_DAI_ETDM) {
		/* set etdm in*/
		etdm_con_in = get_etdm_format() << REG_FMT_SFT; /* ETDM_TDM */
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
		etdm_con_out = get_etdm_format() << REG_FMT_SFT; /* ETDM_TDM */
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
				   get_etdm_inconn_rate(rate) << OUT0_INTERCONN_OUT_EN_SEL_SFT);
		if (need_fsync_remain_8k())
		{
			regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
					ETDM_OUT0_LRCK_DIV2_MASK_SFT,
					(rate == 8000) ? 0x0 : 0x1 << ETDM_OUT0_LRCK_DIV2_SFT);
		}
		else{
			regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
					ETDM_OUT0_LRCK_DIV2_MASK_SFT,
					0x0 << ETDM_OUT0_LRCK_DIV2_SFT);
		}
		if (need_invert_bclk()) {
			regmap_update_bits(afe->regmap, ETDM_OUT0_CON5,
				   OUT0_REG_MASTER_BCK_INV_MASK_SFT,
				   0x1 << OUT0_REG_MASTER_BCK_INV_SFT);
			regmap_update_bits(afe->regmap, ETDM_IN0_CON4,
				   IN0_REG_MASTER_BCK_INV_MASK_SFT,
				   0x1 << IN0_REG_MASTER_BCK_INV_SFT);
		}
	} else if (etdm_id == MT6990_DAI_ETDM1) {
			/* set etdm in*/
			etdm_con_in = get_etdm_format() << REG_FMT_SFT; /* ETDM_TDM */
			etdm_con_in |= etdm_priv->low_jitter_en << REG_RELATCH_1X_EN_SEL_DOMAIN_SFT; /* use apll */
			etdm_con_in |= get_etdm_wlen(format) << REG_WORD_LENGTH_SFT;
			etdm_con_in |= get_etdm_lrck_width(format) << REG_BIT_LENGTH_SFT;
			etdm_con_in |= get_etdm_ch(rate, format) << REG_CH_NUM_SFT;
			regmap_update_bits(afe->regmap, ETDM_IN1_CON0, 0xFFFFFFF0, etdm_con_in);

			/* use apll: 3'b1:0: h26m clock 1: apll clock */
			regmap_update_bits(afe->regmap, ETDM_IN1_CON2,
					   IN0_REG_CLOCK_SOURCE_SEL_MASK_SFT,
					   etdm_priv->low_jitter_en << IN0_REG_CLOCK_SOURCE_SEL_SFT);
			regmap_update_bits(afe->regmap, ETDM_IN1_CON3,
					   IN0_REG_FS_TIMING_SEL_MASK_SFT,
					   get_etdm_rate(rate) << IN0_REG_FS_TIMING_SEL_SFT);
			regmap_update_bits(afe->regmap, ETDM_IN1_CON4,
					   IN0_REG_RELATCH_1X_EN_SEL_MASK_SFT,
					   get_etdm_inconn_rate(rate) <<
					   IN0_REG_RELATCH_1X_EN_SEL_SFT);

			/* set etdm out*/
			etdm_con_out = get_etdm_format() << REG_FMT_SFT; /* ETDM_TDM */
			etdm_con_out |= get_etdm_wlen(format) << REG_WORD_LENGTH_SFT;
			etdm_con_out |= get_etdm_lrck_width(format) << REG_BIT_LENGTH_SFT;
			etdm_con_out |= get_etdm_ch(rate, format) << REG_CH_NUM_SFT;
			regmap_update_bits(afe->regmap, ETDM_OUT1_CON0, 0xFFFFFFF0, etdm_con_out);

			/* use apll: 3'b1:0: h26m clock 1: apll clock */
			regmap_update_bits(afe->regmap, ETDM_OUT1_CON4,
					   OUT0_REG_CLOCK_SOURCE_SEL_MASK_SFT,
					   etdm_priv->low_jitter_en << OUT0_REG_CLOCK_SOURCE_SEL_SFT);
			regmap_update_bits(afe->regmap, ETDM_OUT1_CON4,
					   OUT0_REG_FS_TIMING_SEL_MASK_SFT,
					   get_etdm_rate(rate) << OUT0_REG_FS_TIMING_SEL_SFT);
			regmap_update_bits(afe->regmap, ETDM_OUT1_CON4,
					   OUT0_INTERCONN_OUT_EN_SEL_MASK_SFT,
					   get_etdm_inconn_rate(rate) <<
					   OUT0_INTERCONN_OUT_EN_SEL_SFT);
			if (need_fsync_remain_8k())
			{
				regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
						ETDM_OUT1_LRCK_DIV2_MASK_SFT,
						(rate == 8000) ? 0x0 : 0x1 << ETDM_OUT1_LRCK_DIV2_SFT);
			}
			else{
				regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
						ETDM_OUT1_LRCK_DIV2_MASK_SFT,
						0x0 << ETDM_OUT1_LRCK_DIV2_SFT);
			}
			if (need_invert_bclk()) {
				regmap_update_bits(afe->regmap, ETDM_OUT1_CON5,
					   OUT0_REG_MASTER_BCK_INV_MASK_SFT,
					   0x1 << OUT0_REG_MASTER_BCK_INV_SFT);
				regmap_update_bits(afe->regmap, ETDM_IN1_CON4,
					   IN0_REG_MASTER_BCK_INV_MASK_SFT,
					   0x1 << IN0_REG_MASTER_BCK_INV_SFT);
			}
		}

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

static int mtk_dai_etdm_set_sysclk(struct snd_soc_dai *dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dai->dev);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	struct mtk_afe_etdm_priv *etdm_priv;
	int apll;
	int apll_rate;

	if (dai->id < 0 || dai->id >= MT6990_DAI_NUM)
		return -EINVAL;
	etdm_priv = afe_priv->dai_priv[dai->id];
	if (!etdm_priv) {
		dev_warn(afe->dev, "%s(), etdm_priv == NULL", __func__);
		return -EINVAL;
	}

	if (dir != SND_SOC_CLOCK_OUT) {
		dev_warn(afe->dev, "%s(), dir != SND_SOC_CLOCK_OUT", __func__);
		return -EINVAL;
	}

	dev_info(afe->dev, "%s(), freq %d\n", __func__, freq);

	apll = mt6990_get_apll_by_rate(afe, freq);
	apll_rate = mt6990_get_apll_rate(afe, apll);

	if (freq > apll_rate) {
		dev_warn(afe->dev, "%s(), freq > apll rate", __func__);
		return -EINVAL;
	}

	if (apll_rate % freq != 0) {
		dev_warn(afe->dev, "%s(), APLL cannot generate freq Hz",
			 __func__);
		return -EINVAL;
	}

	etdm_priv->mclk_rate = freq;
	etdm_priv->mclk_apll = apll;
	return 0;
}

static const struct snd_soc_dai_ops mtk_dai_etdm_ops = {
	.hw_params = mtk_dai_etdm_hw_params,
	.trigger = mtk_dai_etdm_trigger,
	.set_sysclk = mtk_dai_etdm_set_sysclk,
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
		.id = MT6990_DAI_ETDM,
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
	{
		.name = "ETDM1",
		.id = MT6990_DAI_ETDM1,
		.playback = {
			.stream_name = "ETDM1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_ETDM_RATES,
			.formats = MTK_ETDM_FORMATS,
		},
		.capture = {
			.stream_name = "ETDM1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_ETDM_RATES,
			.formats = MTK_ETDM_FORMATS,
		},
		.ops = &mtk_dai_etdm_ops,
		.symmetric_rates = 1,
		.symmetric_samplebits = 1,
	},
};

enum {
	DAI_ETDM0 = 0,
	DAI_ETDM1,
	DAI_ETDM_NUM,
};

static const struct mtk_afe_etdm_priv mt6990_etdm_priv[DAI_ETDM_NUM] = {
	[DAI_ETDM0] = {
		.id = MT6990_DAI_ETDM,
		.etdm_rate = 16000,
		.low_jitter_en = 1,
		.i2s_mode = 0,
	},
	[DAI_ETDM1] = {
		.id = MT6990_DAI_ETDM1,
		.etdm_rate = 16000,
		.mclk_id = MT6990_I2S4_MCK,
		.low_jitter_en = 1,
		.i2s_mode = 0,
	},
};

static int mt6990_dai_etdm_set_priv(struct mtk_base_afe *afe)
{
	int i;
	int ret;

	for (i = 0; i < DAI_ETDM_NUM; i++) {
		ret = mt6990_dai_set_priv(afe, mt6990_etdm_priv[i].id,
					  sizeof(struct mtk_afe_etdm_priv),
					  &mt6990_etdm_priv[i]);
		if (ret)
			return ret;
	}

	return 0;
}

int mt6990_dai_etdm_register(struct mtk_base_afe *afe)
{
	struct mtk_base_afe_dai *dai;
	int ret;

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

	/* set all dai etdm private data */
	ret = mt6990_dai_etdm_set_priv(afe);
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mt6990_dai_etdm_register);

