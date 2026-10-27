// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <sound/soc.h>

#include "../common/mtk-afe-debug.h"
#include "../common/mtk-afe-platform-driver.h"
#include "../common/mtk-afe-fe-dai.h"
#include "../common/mtk-sp-pcm-ops.h"
#include "../common/mtk-sram-manager.h"

#include "mt6990-afe-common.h"
#include "mt6990-afe-clk.h"
#include "mt6990-afe-gpio.h"
#include "mt6990-interconnection.h"

#if IS_ENABLED(CONFIG_SND_SOC_PROSLIC)
#include "../../codecs/proslic/si32185.h"
#include "../../codecs/proslic/si32185_2.h"
#endif

//int init=0;
//static struct task_struct *dc_trim_task;
#if defined(CONFIG_SND_SOC_PROSLIC)
static int mt6990_set_etdm(struct device *dev, bool enable);
static int mt6990_set_etdm1(struct device *dev, bool enable);
#endif

/* FORCE_FPGA_ENABLE_IRQ use irq in fpga */
/* #define FORCE_FPGA_ENABLE_IRQ */
//#define PROSLIC_ALWAYS_ON
//#define CONFIG_FPGA_EARLY_PORTING
#define PROSLIC_INIT_WHEN_PROBE
#define PROSLIC_USE_PCM1

static const struct snd_pcm_hardware mt6990_afe_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE |
		    SNDRV_PCM_FMTBIT_S24_LE |
		    SNDRV_PCM_FMTBIT_S32_LE),
	.period_bytes_min = 256,
	.period_bytes_max = 4 * 48 * 1024,
	.periods_min = 2,
	.periods_max = 256,
	.buffer_bytes_max = 8 * 48 * 1024,
	.fifo_size = 0,
};

static int mt6990_fe_startup(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int memif_num = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	const struct snd_pcm_hardware *mtk_afe_hardware = afe->mtk_afe_hardware;
	int ret;

	memif->substream = substream;

	snd_pcm_hw_constraint_step(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 16);

	snd_soc_set_runtime_hwparams(substream, mtk_afe_hardware);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		dev_err(afe->dev, "snd_pcm_hw_constraint_integer failed\n");

	/* dynamic allocate irq to memif */
	if (memif->irq_usage < 0) {
		int irq_id = mtk_dynamic_irq_acquire(afe);

		if (irq_id != afe->irqs_size) {
			/* link */
			memif->irq_usage = irq_id;
		} else {
			dev_err(afe->dev, "%s() error: no more asys irq\n",
				__func__);
			ret = -EBUSY;
		}
	}

	return ret;
}

void mt6990_fe_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	int irq_id = memif->irq_usage;

	memif->substream = NULL;
	afe_priv->irq_cnt[memif_num] = 0;
	afe_priv->xrun_assert[memif_num] = 0;

	if (!memif->const_irq) {
		mtk_dynamic_irq_release(afe, irq_id);
		memif->irq_usage = -1;
		memif->substream = NULL;
	}
}

int mt6990_fe_trigger(struct snd_pcm_substream *substream, int cmd,
		      struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	int irq_id = memif->irq_usage;
	struct mtk_base_afe_irq *irqs = &afe->irqs[irq_id];
	const struct mtk_base_irq_data *irq_data = irqs->irq_data;
	unsigned int counter = runtime->period_size;
	unsigned int rate = runtime->rate;
	int fs;
	int ret;

	dev_info(afe->dev, "%s(), %s cmd %d, irq_id %d\n",
		 __func__, memif->data->name, cmd, irq_id);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		ret = mtk_memif_set_enable(afe, id);

		/*
		 * for small latency record
		 * ul memif need read some data before irq enable
		 */
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			if ((runtime->period_size * 1000) / rate <= 10)
				udelay(300);
		}

		if (ret) {
			dev_err(afe->dev, "%s(), error, id %d, memif enable, ret %d\n",
				__func__, id, ret);
			return ret;
		}

		/* set irq counter */
		if (afe_priv->irq_cnt[id] > 0)
			counter = afe_priv->irq_cnt[id];

		regmap_update_bits(afe->regmap, irq_data->irq_cnt_reg,
				   irq_data->irq_cnt_maskbit
				   << irq_data->irq_cnt_shift,
				   counter << irq_data->irq_cnt_shift);

		/* set irq fs */
		fs = afe->irq_fs(substream, runtime->rate);

		if (fs < 0)
			return -EINVAL;

		regmap_update_bits(afe->regmap, irq_data->irq_fs_reg,
				   irq_data->irq_fs_maskbit
				   << irq_data->irq_fs_shift,
				   fs << irq_data->irq_fs_shift);
		/* enable interrupt */
		regmap_update_bits(afe->regmap, irq_data->irq_en_reg,
				   1 << irq_data->irq_en_shift,
				   1 << irq_data->irq_en_shift);

		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (afe_priv->xrun_assert[id] > 0) {
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				int avail = snd_pcm_capture_avail(runtime);

				if (avail >= runtime->buffer_size) {
					dev_warn(afe->dev, "%s(), id %d, xrun assert\n",
						 __func__, id);
				}
			}
		}
		ret = mtk_memif_set_disable(afe, id);
		if (ret) {
			dev_err(afe->dev, "%s(), error, id %d, memif enable, ret %d\n",
				__func__, id, ret);
		}

		/* disable interrupt */
		regmap_update_bits(afe->regmap, irq_data->irq_en_reg,
				   1 << irq_data->irq_en_shift,
				   0 << irq_data->irq_en_shift);
		/* and clear pending IRQ */
		regmap_write(afe->regmap, irq_data->irq_clr_reg,
			     1 << irq_data->irq_clr_shift);
		return ret;
	default:
		return -EINVAL;
	}
}

static int mt6990_memif_fs(struct snd_pcm_substream *substream,
			   unsigned int rate)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, AFE_PCM_NAME);
	struct mtk_base_afe *afe;
	int id = rtd->cpu_dai->id;

	if (component == NULL)
		return -EINVAL;
	afe = snd_soc_component_get_drvdata(component);
	return mt6990_rate_transform(afe->dev, rate, id);
}

static int mt6990_get_dai_fs(struct mtk_base_afe *afe,
			     int dai_id, unsigned int rate)
{
	return mt6990_rate_transform(afe->dev, rate, dai_id);
}

static int mt6990_irq_fs(struct snd_pcm_substream *substream, unsigned int rate)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, AFE_PCM_NAME);
	struct mtk_base_afe *afe;

	if (component == NULL)
		return -EINVAL;
	afe = snd_soc_component_get_drvdata(component);
	return mt6990_general_rate_transform(afe->dev, rate);
}

int mt6990_get_memif_pbuf_size(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	if ((runtime->period_size * 1000) / runtime->rate > 10)
		return MT6990_MEMIF_PBUF_SIZE_256_BYTES;
	else
		return MT6990_MEMIF_PBUF_SIZE_32_BYTES;
}

/* FE DAIs */
static const struct snd_soc_dai_ops mt6990_memif_dai_ops = {
	.startup	= mt6990_fe_startup,
	.shutdown	= mt6990_fe_shutdown,
	.hw_params	= mtk_afe_fe_hw_params,
	.hw_free	= mtk_afe_fe_hw_free,
	.prepare	= mtk_afe_fe_prepare,
	.trigger	= mt6990_fe_trigger,
};

#define MTK_PCM_RATES (SNDRV_PCM_RATE_8000_48000 |\
		       SNDRV_PCM_RATE_88200 |\
		       SNDRV_PCM_RATE_96000 |\
		       SNDRV_PCM_RATE_176400 |\
		       SNDRV_PCM_RATE_192000)

#define MTK_PCM_DAI_RATES (SNDRV_PCM_RATE_8000 |\
			   SNDRV_PCM_RATE_16000 |\
			   SNDRV_PCM_RATE_32000 |\
			   SNDRV_PCM_RATE_48000)

#define MTK_PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			 SNDRV_PCM_FMTBIT_S24_LE |\
			 SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver mt6990_memif_dai_driver[] = {
	/* FE DAIs: memory intefaces to CPU */
	{
		.name = "DL1",
		.id = MT6990_MEMIF_DL1,
		.playback = {
			.stream_name = "DL1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "DL12",
		.id = MT6990_MEMIF_DL12,
		.playback = {
			.stream_name = "DL12",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "DL2",
		.id = MT6990_MEMIF_DL2,
		.playback = {
			.stream_name = "DL2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "DL3",
		.id = MT6990_MEMIF_DL3,
		.playback = {
			.stream_name = "DL3",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "DL4",
		.id = MT6990_MEMIF_DL4,
		.playback = {
			.stream_name = "DL4",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL1",
		.id = MT6990_MEMIF_VUL,
		.capture = {
			.stream_name = "UL1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL2",
		.id = MT6990_MEMIF_AWB,
		.capture = {
			.stream_name = "UL2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL3",
		.id = MT6990_MEMIF_VUL2,
		.capture = {
			.stream_name = "UL3",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL4",
		.id = MT6990_MEMIF_AWB2,
		.capture = {
			.stream_name = "UL4",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL5",
		.id = MT6990_MEMIF_VUL3,
		.capture = {
			.stream_name = "UL5",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL6",
		.id = MT6990_MEMIF_VUL4,
		.capture = {
			.stream_name = "UL6",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL7",
		.id = MT6990_MEMIF_VUL5,
		.capture = {
			.stream_name = "UL7",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL_MONO_1",
		.id = MT6990_MEMIF_MOD_DAI,
		.capture = {
			.stream_name = "UL_MONO_1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_DAI_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL_MONO_2",
		.id = MT6990_MEMIF_DAI,
		.capture = {
			.stream_name = "UL_MONO_2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_DAI_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "UL_MONO_3",
		.id = MT6990_MEMIF_DAI2,
		.capture = {
			.stream_name = "UL_MONO_3",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MTK_PCM_DAI_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
	{
		.name = "HDMI",
		.id = MT6990_MEMIF_HDMI,
		.playback = {
			.stream_name = "HDMI",
			.channels_min = 2,
			.channels_max = 8,
			.rates = MTK_PCM_RATES,
			.formats = MTK_PCM_FORMATS,
		},
		.ops = &mt6990_memif_dai_ops,
	},
};

/* kcontrol */
static int mt6990_irq_cnt1_get(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] =
		afe_priv->irq_cnt[MT6990_PRIMARY_MEMIF];
	return 0;
}

static int mt6990_irq_cnt1_set(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_PRIMARY_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	int irq_id = memif->irq_usage;
	int irq_cnt = afe_priv->irq_cnt[memif_num];

	dev_info(afe->dev, "%s(), irq_id %d, irq_cnt = %d, value = %ld\n",
		 __func__,
		 irq_id, irq_cnt,
		 ucontrol->value.integer.value[0]);

	if (irq_cnt == ucontrol->value.integer.value[0])
		return 0;

	irq_cnt = ucontrol->value.integer.value[0];
	afe_priv->irq_cnt[memif_num] = irq_cnt;

	if (pm_runtime_status_suspended(afe->dev) || irq_id < 0) {
		dev_info(afe->dev, "%s(), suspended || irq_id %d, not set\n",
			 __func__, irq_id);
	} else {
		struct mtk_base_afe_irq *irqs = &afe->irqs[irq_id];
		const struct mtk_base_irq_data *irq_data = irqs->irq_data;

		regmap_update_bits(afe->regmap, irq_data->irq_cnt_reg,
				   irq_data->irq_cnt_maskbit
				   << irq_data->irq_cnt_shift,
				   irq_cnt << irq_data->irq_cnt_shift);
	}

	return 0;
}

static int mt6990_irq_cnt2_get(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] =
		afe_priv->irq_cnt[MT6990_RECORD_MEMIF];
	return 0;
}

static int mt6990_irq_cnt2_set(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_RECORD_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	int irq_id = memif->irq_usage;
	int irq_cnt = afe_priv->irq_cnt[memif_num];

	dev_info(afe->dev, "%s(), irq_id %d, irq_cnt = %d, value = %ld\n",
		 __func__,
		 irq_id, irq_cnt,
		 ucontrol->value.integer.value[0]);

	if (irq_cnt == ucontrol->value.integer.value[0])
		return 0;

	irq_cnt = ucontrol->value.integer.value[0];
	afe_priv->irq_cnt[memif_num] = irq_cnt;

	if (pm_runtime_status_suspended(afe->dev) || irq_id < 0) {
		dev_info(afe->dev, "%s(), suspended || irq_id %d, not set\n",
			 __func__, irq_id);
	} else {
		struct mtk_base_afe_irq *irqs = &afe->irqs[irq_id];
		const struct mtk_base_irq_data *irq_data = irqs->irq_data;

		regmap_update_bits(afe->regmap, irq_data->irq_cnt_reg,
				   irq_data->irq_cnt_maskbit
				   << irq_data->irq_cnt_shift,
				   irq_cnt << irq_data->irq_cnt_shift);
	}

	return 0;
}

static int mt6990_deep_irq_cnt_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->irq_cnt[MT6990_DEEP_MEMIF];
	return 0;
}

static int mt6990_deep_irq_cnt_set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_DEEP_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	int irq_id = memif->irq_usage;
	int irq_cnt = afe_priv->irq_cnt[memif_num];

	dev_info(afe->dev, "%s(), irq_id %d, irq_cnt = %d, value = %ld\n",
		 __func__,
		 irq_id, irq_cnt,
		 ucontrol->value.integer.value[0]);

	if (irq_cnt == ucontrol->value.integer.value[0])
		return 0;

	irq_cnt = ucontrol->value.integer.value[0];
	afe_priv->irq_cnt[memif_num] = irq_cnt;

	if (pm_runtime_status_suspended(afe->dev) || irq_id < 0) {
		dev_info(afe->dev, "%s(), suspended || irq_id %d, not set\n",
			 __func__, irq_id);
	} else {
		struct mtk_base_afe_irq *irqs = &afe->irqs[irq_id];
		const struct mtk_base_irq_data *irq_data = irqs->irq_data;

		regmap_update_bits(afe->regmap, irq_data->irq_cnt_reg,
				   irq_data->irq_cnt_maskbit
				   << irq_data->irq_cnt_shift,
				   irq_cnt << irq_data->irq_cnt_shift);
	}

	return 0;
}

static int mt6990_voip_rx_irq_cnt_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->irq_cnt[MT6990_VOIP_MEMIF];
	return 0;
}

static int mt6990_voip_rx_irq_cnt_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_VOIP_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	int irq_id = memif->irq_usage;
	int irq_cnt = afe_priv->irq_cnt[memif_num];

	dev_info(afe->dev, "%s(), irq_id %d, irq_cnt = %d, value = %ld\n",
		 __func__,
		 irq_id, irq_cnt,
		 ucontrol->value.integer.value[0]);

	if (irq_cnt == ucontrol->value.integer.value[0])
		return 0;

	irq_cnt = ucontrol->value.integer.value[0];
	afe_priv->irq_cnt[memif_num] = irq_cnt;

	if (pm_runtime_status_suspended(afe->dev) || irq_id < 0) {
		dev_info(afe->dev, "%s(), suspended || irq_id %d, not set\n",
			 __func__, irq_id);
	} else {
		struct mtk_base_afe_irq *irqs = &afe->irqs[irq_id];
		const struct mtk_base_irq_data *irq_data = irqs->irq_data;

		regmap_update_bits(afe->regmap, irq_data->irq_cnt_reg,
				   irq_data->irq_cnt_maskbit
				   << irq_data->irq_cnt_shift,
				   irq_cnt << irq_data->irq_cnt_shift);
	}

	return 0;
}

static int mt6990_deep_scene_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->deep_playback_state;
	return 0;
}

static int mt6990_deep_scene_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_DEEP_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];

	afe_priv->deep_playback_state = ucontrol->value.integer.value[0];

	if (afe_priv->deep_playback_state == 1)
		memif->ack_enable = true;
	else
		memif->ack_enable = false;

	return 0;
}

static int mt6990_fast_scene_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->fast_playback_state;
	return 0;
}

static int mt6990_fast_scene_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_FAST_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];

	afe_priv->fast_playback_state = ucontrol->value.integer.value[0];

	if (afe_priv->fast_playback_state == 1)
		memif->use_dram_only = 1;
	else
		memif->use_dram_only = 0;

	return 0;
}

static int mt6990_primary_scene_get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->primary_playback_state;
	return 0;
}

static int mt6990_primary_scene_set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_PRIMARY_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];

	afe_priv->primary_playback_state = ucontrol->value.integer.value[0];

	if (afe_priv->primary_playback_state == 1)
		memif->use_dram_only = 1;
	else
		memif->use_dram_only = 0;

	return 0;
}

static int mt6990_voip_scene_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->voip_rx_state;
	return 0;
}

static int mt6990_voip_scene_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int memif_num = MT6990_VOIP_MEMIF;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];

	afe_priv->voip_rx_state = ucontrol->value.integer.value[0];

	if (afe_priv->voip_rx_state == 1)
		memif->use_dram_only = 1;
	else
		memif->use_dram_only = 0;

	return 0;
}

static int mt6990_record_xrun_assert_get(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int xrun_assert = afe_priv->xrun_assert[MT6990_RECORD_MEMIF];

	ucontrol->value.integer.value[0] = xrun_assert;
	return 0;
}

static int mt6990_record_xrun_assert_set(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int xrun_assert = ucontrol->value.integer.value[0];

	dev_info(afe->dev, "%s(), xrun_assert %d\n", __func__, xrun_assert);
	afe_priv->xrun_assert[MT6990_RECORD_MEMIF] = xrun_assert;
	return 0;
}

static int mt6990_echo_ref_xrun_assert_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int xrun_assert = afe_priv->xrun_assert[MT6990_ECHO_REF_MEMIF];

	ucontrol->value.integer.value[0] = xrun_assert;
	return 0;
}

static int mt6990_echo_ref_xrun_assert_set(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int xrun_assert = ucontrol->value.integer.value[0];

	dev_info(afe->dev, "%s(), xrun_assert %d\n", __func__, xrun_assert);
	afe_priv->xrun_assert[MT6990_ECHO_REF_MEMIF] = xrun_assert;
	return 0;
}

static int mt6990_sram_size_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);

	ucontrol->value.integer.value[0] =
		mtk_audio_sram_get_size(afe->sram, MTK_AUDIO_SRAM_NORMAL_MODE);
	ucontrol->value.integer.value[1] =
		mtk_audio_sram_get_size(afe->sram, MTK_AUDIO_SRAM_COMPACT_MODE);

	return 0;
}
static int mt6990_pcm_clock_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->pcm_clock_state;
	return 0;
}

static int mt6990_pcm_clock_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	afe_priv->pcm_clock_state = ucontrol->value.integer.value[0];

#if defined(CONFIG_SND_SOC_PROSLIC)
	if (afe_priv->pcm_clock_state) {
		pm_runtime_get_sync(afe->dev);
		mt6990_apll2_enable(afe);
		mt6990_set_etdm(afe->dev, true);
	} else {
		mt6990_set_etdm(afe->dev, false);
		mt6990_apll2_disable(afe);
		pm_runtime_put_sync(afe->dev);
	}
#endif

	return 0;
}

static int mt6990_pcm_clock2_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->pcm_clock_state;
	return 0;
}

static int mt6990_pcm_clock2_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(cmpnt);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	afe_priv->pcm_clock_state = ucontrol->value.integer.value[0];

#if defined(CONFIG_SND_SOC_PROSLIC)
	if (afe_priv->pcm_clock_state) {
		pm_runtime_get_sync(afe->dev);
		mt6990_apll2_enable(afe);
		mt6990_set_etdm1(afe->dev, true);
	} else {
		mt6990_set_etdm1(afe->dev, false);
		mt6990_apll2_disable(afe);
		pm_runtime_put_sync(afe->dev);
	}
#endif

	return 0;
}

static int mt6990_slic_init_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int mt6990_slic_init_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#if defined(CONFIG_SND_SOC_PROSLIC)
	return ProSLIC_HWInit();
#else
	return 0;
#endif
}

static int mt6990_slic_init2_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int mt6990_slic_init2_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#if defined(CONFIG_SND_SOC_PROSLIC)
	return ProSLIC_HWInit2();
#else
	return 0;
#endif
}

static const struct snd_kcontrol_new mt6990_pcm_kcontrols[] = {
	SOC_SINGLE_EXT("Audio IRQ1 CNT", SND_SOC_NOPM, 0, 0x3ffff, 0,
		       mt6990_irq_cnt1_get, mt6990_irq_cnt1_set),
	SOC_SINGLE_EXT("Audio IRQ2 CNT", SND_SOC_NOPM, 0, 0x3ffff, 0,
		       mt6990_irq_cnt2_get, mt6990_irq_cnt2_set),
	SOC_SINGLE_EXT("deep_buffer_irq_cnt", SND_SOC_NOPM, 0, 0x3ffff, 0,
		       mt6990_deep_irq_cnt_get, mt6990_deep_irq_cnt_set),
	SOC_SINGLE_EXT("voip_rx_irq_cnt", SND_SOC_NOPM, 0, 0x3ffff, 0,
		       mt6990_voip_rx_irq_cnt_get, mt6990_voip_rx_irq_cnt_set),
	SOC_SINGLE_EXT("deep_buffer_scenario", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_deep_scene_get, mt6990_deep_scene_set),
	SOC_SINGLE_EXT("record_xrun_assert", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_record_xrun_assert_get,
		       mt6990_record_xrun_assert_set),
	SOC_SINGLE_EXT("echo_ref_xrun_assert", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_echo_ref_xrun_assert_get,
		       mt6990_echo_ref_xrun_assert_set),
	SOC_SINGLE_EXT("fast_play_scenario", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_fast_scene_get, mt6990_fast_scene_set),
	SOC_SINGLE_EXT("primary_play_scenario", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_primary_scene_get, mt6990_primary_scene_set),
	SOC_SINGLE_EXT("voip_rx_scenario", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_voip_scene_get, mt6990_voip_scene_set),
	SOC_DOUBLE_EXT("sram_size", SND_SOC_NOPM, 0, 1, 0xffffffff, 0,
		       mt6990_sram_size_get, NULL),
	SOC_SINGLE_EXT("pcm_clock", SND_SOC_NOPM, 0, 0x1, 0,
		       mt6990_pcm_clock_get, mt6990_pcm_clock_set),
	SOC_SINGLE_EXT("pcm_clock2", SND_SOC_NOPM, 0, 0x1, 0,
			   mt6990_pcm_clock2_get, mt6990_pcm_clock2_set),
	SOC_SINGLE_EXT("slic_init", SND_SOC_NOPM, 0, 0x1, 0,
			   mt6990_slic_init_get, mt6990_slic_init_set),
	SOC_SINGLE_EXT("slic_init2", SND_SOC_NOPM, 0, 0x1, 0,
			   mt6990_slic_init2_get, mt6990_slic_init2_set),
};

/* dma widget & routes*/
static const struct snd_kcontrol_new memif_ul1_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN9,
				    I_ADDA_UL_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM_CAP_CH1", AFE_CONN9,
				    I_ETDM_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH1", AFE_CONN9,
				    I_PCM_0_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN9,
				    I_PCM_1_CAP_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul1_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH2", AFE_CONN10,
				    I_ADDA_UL_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM_CAP_CH1", AFE_CONN10,
				    I_ETDM_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH2", AFE_CONN10_1,
				    I_PCM_0_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH2", AFE_CONN10,
				    I_PCM_1_CAP_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul2_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I2S0_CH1", AFE_CONN5,
				    I_I2S0_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH1", AFE_CONN5,
				    I_DL1_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL12_CH1", AFE_CONN5,
				    I_DL12_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH1", AFE_CONN5,
				    I_DL2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL3_CH1", AFE_CONN5,
				    I_DL3_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL4_CH1", AFE_CONN5_1,
				    I_DL4_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH1", AFE_CONN5_1,
				    I_PCM_0_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN5,
				    I_PCM_1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN5,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S2_CH1", AFE_CONN5,
				    I_I2S2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM1_CAP_CH1", AFE_CONN5,
				    I_ETDM1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S6_CH1", AFE_CONN5,
					I_I2S6_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("CONNSYS_I2S_CH1", AFE_CONN5_1,
				    I_CONNSYS_I2S_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("SRC_1_OUT_CH1", AFE_CONN5_1,
				    I_SRC_1_OUT_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul2_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I2S0_CH2", AFE_CONN6,
				    I_I2S0_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH2", AFE_CONN6,
				    I_DL1_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL12_CH2", AFE_CONN6,
				    I_DL12_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH2", AFE_CONN6,
				    I_DL2_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL3_CH2", AFE_CONN6,
				    I_DL3_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL4_CH2", AFE_CONN6_1,
				    I_DL4_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH2", AFE_CONN6_1,
				    I_PCM_0_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH2", AFE_CONN6,
				    I_PCM_1_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN6,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH2", AFE_CONN6,
				    I_PCM_2_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S2_CH2", AFE_CONN6,
				    I_I2S2_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM1_CAP_CH2", AFE_CONN6,
				    I_ETDM1_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S6_CH2", AFE_CONN6,
					I_I2S6_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("CONNSYS_I2S_CH2", AFE_CONN6_1,
				    I_CONNSYS_I2S_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("SRC_1_OUT_CH2", AFE_CONN6_1,
				    I_SRC_1_OUT_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul3_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("CONNSYS_I2S_CH1", AFE_CONN32_1,
				    I_CONNSYS_I2S_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH1", AFE_CONN32,
				    I_DL1_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH1", AFE_CONN32,
				    I_DL2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S6_CH3", AFE_CONN32_1,
				    I_I2S6_CH3, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH1", AFE_CONN32_1,
				    I_PCM_0_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN32,
				    I_PCM_1_CAP_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul3_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("CONNSYS_I2S_CH2", AFE_CONN33_1,
				    I_CONNSYS_I2S_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S6_CH4", AFE_CONN33_1,
				    I_I2S6_CH4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH2", AFE_CONN33_1,
				    I_PCM_0_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH2", AFE_CONN33,
				    I_PCM_1_CAP_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul4_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN38,
				    I_ADDA_UL_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S0_CH1", AFE_CONN38,
				    I_I2S0_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM1_CAP_CH1", AFE_CONN38,
				    I_ETDM1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S6_CH1", AFE_CONN38,
				    I_I2S6_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul4_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH2", AFE_CONN39,
				    I_ADDA_UL_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S0_CH2", AFE_CONN39,
				    I_I2S0_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM1_CAP_CH2", AFE_CONN39,
				    I_ETDM1_CAP_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S6_CH2", AFE_CONN39,
				    I_I2S6_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul5_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN44,
				    I_ADDA_UL_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul5_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH2", AFE_CONN45,
				    I_ADDA_UL_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul6_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN46,
				    I_ADDA_UL_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH1", AFE_CONN46,
				    I_DL1_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL12_CH1", AFE_CONN46,
				    I_DL12_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH1", AFE_CONN46,
				    I_DL2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL3_CH1", AFE_CONN46,
				    I_DL3_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL4_CH1", AFE_CONN46_1,
				    I_DL4_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul6_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH2", AFE_CONN47,
				    I_ADDA_UL_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL1_CH2", AFE_CONN47,
				    I_DL1_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL12_CH2", AFE_CONN47,
				    I_DL12_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL2_CH2", AFE_CONN47,
				    I_DL2_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL3_CH2", AFE_CONN47,
				    I_DL3_CH2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("DL4_CH2", AFE_CONN47_1,
				    I_DL4_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul7_ch1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN48,
				    I_ADDA_UL_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul7_ch2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH2", AFE_CONN49,
				    I_ADDA_UL_CH2, 1, 0),
};

static const struct snd_kcontrol_new memif_ul_mono_1_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN12,
				    I_PCM_1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN12,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM_CAP_CH1", AFE_CONN12,
				    I_ETDM_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S0_CH1", AFE_CONN12,
				    I_I2S0_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S2_CH1", AFE_CONN12,
				    I_I2S2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH1", AFE_CONN12_1,
				    I_PCM_0_CAP_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul_mono_2_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_1_CAP_CH1", AFE_CONN11,
				    I_PCM_1_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_2_CAP_CH1", AFE_CONN11,
				    I_PCM_2_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("ETDM_CAP_CH1", AFE_CONN11,
				    I_ETDM_CAP_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S0_CH1", AFE_CONN11,
				    I_I2S0_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I2S2_CH1", AFE_CONN11,
				    I_I2S2_CH1, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("PCM_0_CAP_CH1", AFE_CONN11_1,
				    I_PCM_0_CAP_CH1, 1, 0),
};

static const struct snd_kcontrol_new memif_ul_mono_3_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("ADDA_UL_CH1", AFE_CONN35,
				    I_ADDA_UL_CH1, 1, 0),
};

static const struct snd_soc_dapm_widget mt6990_memif_widgets[] = {
	/* inter-connections */
	SND_SOC_DAPM_MIXER("UL1_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul1_ch1_mix, ARRAY_SIZE(memif_ul1_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL1_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul1_ch2_mix, ARRAY_SIZE(memif_ul1_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL2_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul2_ch1_mix, ARRAY_SIZE(memif_ul2_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL2_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul2_ch2_mix, ARRAY_SIZE(memif_ul2_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL3_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul3_ch1_mix, ARRAY_SIZE(memif_ul3_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL3_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul3_ch2_mix, ARRAY_SIZE(memif_ul3_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL4_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul4_ch1_mix, ARRAY_SIZE(memif_ul4_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL4_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul4_ch2_mix, ARRAY_SIZE(memif_ul4_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL5_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul5_ch1_mix, ARRAY_SIZE(memif_ul5_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL5_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul5_ch2_mix, ARRAY_SIZE(memif_ul5_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL6_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul6_ch1_mix, ARRAY_SIZE(memif_ul6_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL6_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul6_ch2_mix, ARRAY_SIZE(memif_ul6_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL7_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul7_ch1_mix, ARRAY_SIZE(memif_ul7_ch1_mix)),
	SND_SOC_DAPM_MIXER("UL7_CH2", SND_SOC_NOPM, 0, 0,
			   memif_ul7_ch2_mix, ARRAY_SIZE(memif_ul7_ch2_mix)),

	SND_SOC_DAPM_MIXER("UL_MONO_1_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul_mono_1_mix,
			   ARRAY_SIZE(memif_ul_mono_1_mix)),

	SND_SOC_DAPM_MIXER("UL_MONO_2_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul_mono_2_mix,
			   ARRAY_SIZE(memif_ul_mono_2_mix)),

	SND_SOC_DAPM_MIXER("UL_MONO_3_CH1", SND_SOC_NOPM, 0, 0,
			   memif_ul_mono_3_mix,
			   ARRAY_SIZE(memif_ul_mono_3_mix)),

	SND_SOC_DAPM_INPUT("UL1_VIRTUAL_INPUT"),
	SND_SOC_DAPM_INPUT("UL2_VIRTUAL_INPUT"),
	SND_SOC_DAPM_INPUT("UL6_VIRTUAL_INPUT"),

};

static const struct snd_soc_dapm_route mt6990_memif_routes[] = {
	{"UL1", NULL, "UL1_CH1"},
	{"UL1", NULL, "UL1_CH2"},
	{"UL1_CH1", "ADDA_UL_CH1", "ADDA_UL_Mux"},
	{"UL1_CH2", "ADDA_UL_CH2", "ADDA_UL_Mux"},

	{"UL1_CH1", "ETDM_CAP_CH1", "ETDM Capture"},
	{"UL1_CH2", "ETDM_CAP_CH1", "ETDM Capture"},
	{"UL1_CH1", "PCM_0_CAP_CH1", "PCM 0 Capture"},
	{"UL1_CH2", "PCM_0_CAP_CH2", "PCM 0 Capture"},
	{"UL1_CH1", "PCM_1_CAP_CH1", "PCM 1 Capture"},
	{"UL1_CH2", "PCM_1_CAP_CH2", "PCM 1 Capture"},


	{"UL2", NULL, "UL2_CH1"},
	{"UL2", NULL, "UL2_CH2"},

	/* cannot connect FE to FE directly */
	{"UL2_CH1", "DL1_CH1", "Hostless_UL2 UL"},
	{"UL2_CH2", "DL1_CH2", "Hostless_UL2 UL"},
	{"UL2_CH1", "DL12_CH1", "Hostless_UL2 UL"},
	{"UL2_CH2", "DL12_CH2", "Hostless_UL2 UL"},
	{"UL2_CH1", "DL2_CH1", "Hostless_UL2 UL"},
	{"UL2_CH2", "DL2_CH2", "Hostless_UL2 UL"},
	{"UL2_CH1", "DL3_CH1", "Hostless_UL2 UL"},
	{"UL2_CH2", "DL3_CH2", "Hostless_UL2 UL"},
	{"UL2_CH1", "DL4_CH1", "Hostless_UL2 UL"},
	{"UL2_CH2", "DL4_CH2", "Hostless_UL2 UL"},

	{"Hostless_UL2 UL", NULL, "UL2_VIRTUAL_INPUT"},

	{"UL2_CH1", "I2S0_CH1", "I2S0"},
	{"UL2_CH2", "I2S0_CH2", "I2S0"},
	{"UL2_CH1", "I2S2_CH1", "I2S2"},
	{"UL2_CH2", "I2S2_CH2", "I2S2"},
	{"UL2_CH1", "ETDM1_CAP_CH1", "ETDM1 Capture"},
	{"UL2_CH2", "ETDM1_CAP_CH2", "ETDM1 Capture"},
	{"UL2_CH1", "I2S6_CH1", "I2S6"},
	{"UL2_CH2", "I2S6_CH2", "I2S6"},

	{"UL2_CH1", "PCM_0_CAP_CH1", "PCM 0 Capture"},
	{"UL2_CH2", "PCM_0_CAP_CH2", "PCM 0 Capture"},
	{"UL2_CH1", "PCM_1_CAP_CH1", "PCM 1 Capture"},
	{"UL2_CH2", "PCM_1_CAP_CH2", "PCM 1 Capture"},
	{"UL2_CH1", "PCM_2_CAP_CH1", "PCM 2 Capture"},
	{"UL2_CH2", "PCM_2_CAP_CH1", "PCM 2 Capture"},
	{"UL2_CH2", "PCM_2_CAP_CH2", "PCM 2 Capture"},

	{"UL_MONO_1", NULL, "UL_MONO_1_CH1"},
	{"UL_MONO_1_CH1", "PCM_0_CAP_CH1", "PCM 0 Capture"},
	{"UL_MONO_1_CH1", "PCM_1_CAP_CH1", "PCM 1 Capture"},
	{"UL_MONO_1_CH1", "PCM_2_CAP_CH1", "PCM 2 Capture"},
	{"UL_MONO_1_CH1", "ETDM_CAP_CH1", "ETDM Capture"},
	{"UL_MONO_1_CH1", "I2S0_CH1", "I2S0"},
	{"UL_MONO_1_CH1", "I2S2_CH1", "I2S2"},

	{"UL_MONO_2", NULL, "UL_MONO_2_CH1"},
	{"UL_MONO_2_CH1", "PCM_0_CAP_CH1", "PCM 0 Capture"},
	{"UL_MONO_2_CH1", "PCM_1_CAP_CH1", "PCM 1 Capture"},
	{"UL_MONO_2_CH1", "PCM_2_CAP_CH1", "PCM 2 Capture"},
	{"UL_MONO_2_CH1", "ETDM_CAP_CH1", "ETDM Capture"},
	{"UL_MONO_2_CH1", "I2S0_CH1", "I2S0"},
	{"UL_MONO_2_CH1", "I2S2_CH1", "I2S2"},

	{"UL_MONO_3", NULL, "UL_MONO_3_CH1"},
	{"UL_MONO_3_CH1", "ADDA_UL_CH1", "ADDA_UL_Mux"},

	{"UL2_CH1", "CONNSYS_I2S_CH1", "Connsys I2S"},
	{"UL2_CH2", "CONNSYS_I2S_CH2", "Connsys I2S"},

	{"UL2_CH1", "SRC_1_OUT_CH1", "HW_SRC_1_Out"},
	{"UL2_CH2", "SRC_1_OUT_CH2", "HW_SRC_1_Out"},

	{"UL3", NULL, "UL3_CH1"},
	{"UL3", NULL, "UL3_CH2"},
	{"UL3_CH1", "CONNSYS_I2S_CH1", "Connsys I2S"},
	{"UL3_CH2", "CONNSYS_I2S_CH2", "Connsys I2S"},
	{"UL3_CH1", "I2S6_CH3", "I2S6"},
	{"UL3_CH2", "I2S6_CH4", "I2S6"},
	{"UL3_CH1", "PCM_0_CAP_CH1", "PCM 0 Capture"},
	{"UL3_CH2", "PCM_0_CAP_CH2", "PCM 0 Capture"},
	{"UL3_CH1", "PCM_1_CAP_CH1", "PCM 1 Capture"},
	{"UL3_CH2", "PCM_1_CAP_CH2", "PCM 1 Capture"},

	{"UL4", NULL, "UL4_CH1"},
	{"UL4", NULL, "UL4_CH2"},

	{"UL4_CH1", "ADDA_UL_CH1", "ADDA_UL_Mux"},
	{"UL4_CH2", "ADDA_UL_CH2", "ADDA_UL_Mux"},
	{"UL4_CH1", "I2S0_CH1", "I2S0"},
	{"UL4_CH2", "I2S0_CH2", "I2S0"},
	{"UL4_CH1", "ETDM1_CAP_CH1", "ETDM1 Capture"},
	{"UL4_CH2", "ETDM1_CAP_CH2", "ETDM1 Capture"},
	{"UL4_CH1", "I2S6_CH1", "I2S6"},
	{"UL4_CH2", "I2S6_CH2", "I2S6"},

	{"UL5", NULL, "UL5_CH1"},
	{"UL5", NULL, "UL5_CH2"},
	{"UL5_CH1", "ADDA_UL_CH1", "ADDA_UL_Mux"},
	{"UL5_CH2", "ADDA_UL_CH2", "ADDA_UL_Mux"},

	{"UL6", NULL, "UL6_CH1"},
	{"UL6", NULL, "UL6_CH2"},

	{"UL6_CH1", "ADDA_UL_CH1", "ADDA_UL_Mux"},
	{"UL6_CH2", "ADDA_UL_CH2", "ADDA_UL_Mux"},
	{"UL6_CH1", "DL1_CH1", "Hostless_UL6 UL"},
	{"UL6_CH2", "DL1_CH2", "Hostless_UL6 UL"},
	{"UL6_CH1", "DL2_CH1", "Hostless_UL6 UL"},
	{"UL6_CH2", "DL2_CH2", "Hostless_UL6 UL"},
	{"UL6_CH1", "DL12_CH1", "Hostless_UL6 UL"},
	{"UL6_CH2", "DL12_CH2", "Hostless_UL6 UL"},
	{"UL6_CH1", "DL3_CH1", "Hostless_UL6 UL"},
	{"UL6_CH2", "DL3_CH2", "Hostless_UL6 UL"},
	{"UL6_CH1", "DL4_CH1", "Hostless_UL6 UL"},
	{"UL6_CH2", "DL4_CH2", "Hostless_UL6 UL"},
	{"Hostless_UL6 UL", NULL, "UL6_VIRTUAL_INPUT"},

	{"UL7", NULL, "UL7_CH1"},
	{"UL7", NULL, "UL7_CH2"},
	{"UL7_CH1", "ADDA_UL_CH1", "ADDA_UL_Mux"},
	{"UL7_CH2", "ADDA_UL_CH2", "ADDA_UL_Mux"},

};

static const struct mtk_base_memif_data memif_data[MT6990_MEMIF_NUM] = {
	[MT6990_MEMIF_DL1] = {
		.name = "DL1",
		.id = MT6990_MEMIF_DL1,
		.reg_ofs_base = AFE_DL1_BASE,
		.reg_ofs_cur = AFE_DL1_CUR,
		.reg_ofs_end = AFE_DL1_END,
		.reg_ofs_base_msb = AFE_DL1_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL1_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL1_END_MSB,
		.fs_reg = AFE_DL1_CON0,
		.fs_shift = DL1_MODE_SFT,
		.fs_maskbit = DL1_MODE_MASK,
		.mono_reg = AFE_DL1_CON0,
		.mono_shift = DL1_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DL1_ON_SFT,
		.hd_reg = AFE_DL1_CON0,
		.hd_shift = DL1_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL1_CON0,
		.pbuf_mask = DL1_PBUF_SIZE_MASK,
		.pbuf_shift = DL1_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL1_CON0,
		.minlen_mask = DL1_MINLEN_MASK,
		.minlen_shift = DL1_MINLEN_SFT,
	},
	[MT6990_MEMIF_DL12] = {
		.name = "DL12",
		.id = MT6990_MEMIF_DL12,
		.reg_ofs_base = AFE_DL12_BASE,
		.reg_ofs_cur = AFE_DL12_CUR,
		.reg_ofs_end = AFE_DL12_END,
		.reg_ofs_base_msb = AFE_DL12_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL12_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL12_END_MSB,
		.fs_reg = AFE_DL12_CON0,
		.fs_shift = DL12_MODE_SFT,
		.fs_maskbit = DL12_MODE_MASK,
		.mono_reg = AFE_DL12_CON0,
		.mono_shift = DL12_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DL12_ON_SFT,
		.hd_reg = AFE_DL12_CON0,
		.hd_shift = DL12_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL12_CON0,
		.pbuf_mask = DL12_PBUF_SIZE_MASK,
		.pbuf_shift = DL12_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL12_CON0,
		.minlen_mask = DL12_MINLEN_MASK,
		.minlen_shift = DL12_MINLEN_SFT,
	},
	[MT6990_MEMIF_DL2] = {
		.name = "DL2",
		.id = MT6990_MEMIF_DL2,
		.reg_ofs_base = AFE_DL2_BASE,
		.reg_ofs_cur = AFE_DL2_CUR,
		.reg_ofs_end = AFE_DL2_END,
		.reg_ofs_base_msb = AFE_DL2_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL2_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL2_END_MSB,
		.fs_reg = AFE_DL2_CON0,
		.fs_shift = DL2_MODE_SFT,
		.fs_maskbit = DL2_MODE_MASK,
		.mono_reg = AFE_DL2_CON0,
		.mono_shift = DL2_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DL2_ON_SFT,
		.hd_reg = AFE_DL2_CON0,
		.hd_shift = DL2_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL2_CON0,
		.pbuf_mask = DL2_PBUF_SIZE_MASK,
		.pbuf_shift = DL2_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL2_CON0,
		.minlen_mask = DL2_MINLEN_MASK,
		.minlen_shift = DL2_MINLEN_SFT,
	},
	[MT6990_MEMIF_DL3] = {
		.name = "DL3",
		.id = MT6990_MEMIF_DL3,
		.reg_ofs_base = AFE_DL3_BASE,
		.reg_ofs_cur = AFE_DL3_CUR,
		.reg_ofs_end = AFE_DL3_END,
		.reg_ofs_base_msb = AFE_DL3_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL3_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL3_END_MSB,
		.fs_reg = AFE_DL3_CON0,
		.fs_shift = DL3_MODE_SFT,
		.fs_maskbit = DL3_MODE_MASK,
		.mono_reg = AFE_DL3_CON0,
		.mono_shift = DL3_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DL3_ON_SFT,
		.hd_reg = AFE_DL3_CON0,
		.hd_shift = DL3_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL3_CON0,
		.pbuf_mask = DL3_PBUF_SIZE_MASK,
		.pbuf_shift = DL3_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL3_CON0,
		.minlen_mask = DL3_MINLEN_MASK,
		.minlen_shift = DL3_MINLEN_SFT,
	},
	[MT6990_MEMIF_DL4] = {
		.name = "DL4",
		.id = MT6990_MEMIF_DL4,
		.reg_ofs_base = AFE_DL4_BASE,
		.reg_ofs_cur = AFE_DL4_CUR,
		.reg_ofs_end = AFE_DL4_END,
		.reg_ofs_base_msb = AFE_DL4_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL4_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL4_END_MSB,
		.fs_reg = AFE_DL4_CON0,
		.fs_shift = DL4_MODE_SFT,
		.fs_maskbit = DL4_MODE_MASK,
		.mono_reg = AFE_DL4_CON0,
		.mono_shift = DL4_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DL4_ON_SFT,
		.hd_reg = AFE_DL4_CON0,
		.hd_shift = DL4_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL4_CON0,
		.pbuf_mask = DL4_PBUF_SIZE_MASK,
		.pbuf_shift = DL4_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL4_CON0,
		.minlen_mask = DL4_MINLEN_MASK,
		.minlen_shift = DL4_MINLEN_SFT,
	},
	[MT6990_MEMIF_DAI] = {
		.name = "DAI",
		.id = MT6990_MEMIF_DAI,
		.reg_ofs_base = AFE_DAI_BASE,
		.reg_ofs_cur = AFE_DAI_CUR,
		.reg_ofs_end = AFE_DAI_END,
		.reg_ofs_base_msb = AFE_DAI_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DAI_CUR_MSB,
		.reg_ofs_end_msb = AFE_DAI_END_MSB,
		.fs_reg = AFE_DAI_CON0,
		.fs_shift = DAI_MODE_SFT,
		.fs_maskbit = DAI_MODE_MASK,
		.mono_reg = AFE_DAI_CON0,
		.mono_shift = DAI_DUPLICATE_WR_SFT,
		.mono_invert = 1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DAI_ON_SFT,
		.hd_reg = AFE_DAI_CON0,
		.hd_shift = DAI_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_MOD_DAI] = {
		.name = "MOD_DAI",
		.id = MT6990_MEMIF_MOD_DAI,
		.reg_ofs_base = AFE_MOD_DAI_BASE,
		.reg_ofs_cur = AFE_MOD_DAI_CUR,
		.reg_ofs_end = AFE_MOD_DAI_END,
		.reg_ofs_base_msb = AFE_MOD_DAI_BASE_MSB,
		.reg_ofs_cur_msb = AFE_MOD_DAI_CUR_MSB,
		.reg_ofs_end_msb = AFE_MOD_DAI_END_MSB,
		.fs_reg = AFE_MOD_DAI_CON0,
		.fs_shift = MOD_DAI_MODE_SFT,
		.fs_maskbit = MOD_DAI_MODE_MASK,
		.mono_reg = AFE_MOD_DAI_CON0,
		.mono_shift = MOD_DAI_DUPLICATE_WR_SFT,
		.mono_invert = 1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = MOD_DAI_ON_SFT,
		.hd_reg = AFE_MOD_DAI_CON0,
		.hd_shift = MOD_DAI_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_DAI2] = {
		.name = "DAI2",
		.id = MT6990_MEMIF_DAI2,
		.reg_ofs_base = AFE_DAI2_BASE,
		.reg_ofs_cur = AFE_DAI2_CUR,
		.reg_ofs_end = AFE_DAI2_END,
		.reg_ofs_base_msb = AFE_DAI2_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DAI2_CUR_MSB,
		.reg_ofs_end_msb = AFE_DAI2_END_MSB,
		.fs_reg = AFE_DAI2_CON0,
		.fs_shift = DAI2_MODE_SFT,
		.fs_maskbit = DAI2_MODE_MASK,
		.mono_reg = AFE_DAI2_CON0,
		.mono_shift = DAI2_DUPLICATE_WR_SFT,
		.mono_invert = 1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = DAI2_ON_SFT,
		.hd_reg = AFE_DAI2_CON0,
		.hd_shift = DAI2_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_VUL] = {
		.name = "VUL",
		.id = MT6990_MEMIF_VUL,
		.reg_ofs_base = AFE_VUL_BASE,
		.reg_ofs_cur = AFE_VUL_CUR,
		.reg_ofs_end = AFE_VUL_END,
		.reg_ofs_base_msb = AFE_VUL_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL_END_MSB,
		.fs_reg = AFE_VUL_CON0,
		.fs_shift = VUL_MODE_SFT,
		.fs_maskbit = VUL_MODE_MASK,
		.mono_reg = AFE_VUL_CON0,
		.mono_shift = VUL_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = VUL_ON_SFT,
		.hd_reg = AFE_VUL_CON0,
		.hd_shift = VUL_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_VUL2] = {
		.name = "VUL2",
		.id = MT6990_MEMIF_VUL2,
		.reg_ofs_base = AFE_VUL2_BASE,
		.reg_ofs_cur = AFE_VUL2_CUR,
		.reg_ofs_end = AFE_VUL2_END,
		.reg_ofs_base_msb = AFE_VUL2_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL2_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL2_END_MSB,
		.fs_reg = AFE_VUL2_CON0,
		.fs_shift = VUL2_MODE_SFT,
		.fs_maskbit = VUL2_MODE_MASK,
		.mono_reg = AFE_VUL2_CON0,
		.mono_shift = VUL2_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = VUL2_ON_SFT,
		.hd_reg = AFE_VUL2_CON0,
		.hd_shift = VUL2_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_AWB] = {
		.name = "AWB",
		.id = MT6990_MEMIF_AWB,
		.reg_ofs_base = AFE_AWB_BASE,
		.reg_ofs_cur = AFE_AWB_CUR,
		.reg_ofs_end = AFE_AWB_END,
		.reg_ofs_base_msb = AFE_AWB_BASE_MSB,
		.reg_ofs_cur_msb = AFE_AWB_CUR_MSB,
		.reg_ofs_end_msb = AFE_AWB_END_MSB,
		.fs_reg = AFE_AWB_CON0,
		.fs_shift = AWB_MODE_SFT,
		.fs_maskbit = AWB_MODE_MASK,
		.mono_reg = AFE_AWB_CON0,
		.mono_shift = AWB_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = AWB_ON_SFT,
		.hd_reg = AFE_AWB_CON0,
		.hd_shift = AWB_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_AWB2] = {
		.name = "AWB2",
		.id = MT6990_MEMIF_AWB2,
		.reg_ofs_base = AFE_AWB2_BASE,
		.reg_ofs_cur = AFE_AWB2_CUR,
		.reg_ofs_end = AFE_AWB2_END,
		.reg_ofs_base_msb = AFE_AWB2_BASE_MSB,
		.reg_ofs_cur_msb = AFE_AWB2_CUR_MSB,
		.reg_ofs_end_msb = AFE_AWB2_END_MSB,
		.fs_reg = AFE_AWB2_CON0,
		.fs_shift = AWB2_MODE_SFT,
		.fs_maskbit = AWB2_MODE_MASK,
		.mono_reg = AFE_AWB2_CON0,
		.mono_shift = AWB2_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = AWB2_ON_SFT,
		.hd_reg = AFE_AWB2_CON0,
		.hd_shift = AWB2_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_VUL3] = {
		.name = "VUL3",
		.id = MT6990_MEMIF_VUL3,
		.reg_ofs_base = AFE_VUL3_BASE,
		.reg_ofs_cur = AFE_VUL3_CUR,
		.reg_ofs_end = AFE_VUL3_END,
		.reg_ofs_base_msb = AFE_VUL3_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL3_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL3_END_MSB,
		.fs_reg = AFE_VUL3_CON0,
		.fs_shift = VUL3_MODE_SFT,
		.fs_maskbit = VUL3_MODE_MASK,
		.mono_reg = AFE_VUL3_CON0,
		.mono_shift = VUL3_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = VUL3_ON_SFT,
		.hd_reg = AFE_VUL3_CON0,
		.hd_shift = VUL3_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_VUL4] = {
		.name = "VUL4",
		.id = MT6990_MEMIF_VUL4,
		.reg_ofs_base = AFE_VUL4_BASE,
		.reg_ofs_cur = AFE_VUL4_CUR,
		.reg_ofs_end = AFE_VUL4_END,
		.reg_ofs_base_msb = AFE_VUL4_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL4_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL4_END_MSB,
		.fs_reg = AFE_VUL4_CON0,
		.fs_shift = VUL4_MODE_SFT,
		.fs_maskbit = VUL4_MODE_MASK,
		.mono_reg = AFE_VUL4_CON0,
		.mono_shift = VUL4_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = VUL4_ON_SFT,
		.hd_reg = AFE_VUL4_CON0,
		.hd_shift = VUL4_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_VUL5] = {
		.name = "VUL5",
		.id = MT6990_MEMIF_VUL5,
		.reg_ofs_base = AFE_VUL5_BASE,
		.reg_ofs_cur = AFE_VUL5_CUR,
		.reg_ofs_end = AFE_VUL5_END,
		.reg_ofs_base_msb = AFE_VUL5_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL5_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL5_END_MSB,
		.fs_reg = AFE_VUL5_CON0,
		.fs_shift = VUL5_MODE_SFT,
		.fs_maskbit = VUL5_MODE_MASK,
		.mono_reg = AFE_VUL5_CON0,
		.mono_shift = VUL5_MONO_SFT,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = VUL5_ON_SFT,
		.hd_reg = AFE_VUL5_CON0,
		.hd_shift = VUL5_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT6990_MEMIF_HDMI] = {
		.name = "HDMI",
		.id = MT6990_MEMIF_HDMI,
		.reg_ofs_base = AFE_HDMI_OUT_BASE,
		.reg_ofs_cur = AFE_HDMI_OUT_CUR,
		.reg_ofs_end = AFE_HDMI_OUT_END,
		.reg_ofs_base_msb = AFE_HDMI_OUT_BASE_MSB,
		.reg_ofs_cur_msb = AFE_HDMI_OUT_CUR_MSB,
		.reg_ofs_end_msb = AFE_HDMI_OUT_END_MSB,
		.fs_reg = -1,
		.fs_shift = -1,
		.fs_maskbit = -1,
		.mono_reg = -1,
		.mono_shift = -1,
		.enable_reg = -1,	/* control in tdm for sync start */
		.enable_shift = -1,
		.hd_reg = AFE_HDMI_OUT_CON0,
		.hd_shift = HDMI_OUT_HD_MODE_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_HDMI_OUT_CON0,
		.pbuf_mask = HDMI_OUT_PBUF_SIZE_MASK,
		.pbuf_shift = HDMI_OUT_PBUF_SIZE_SFT,
		.minlen_reg = AFE_HDMI_OUT_CON0,
		.minlen_mask = HDMI_OUT_MINLEN_MASK,
		.minlen_shift = HDMI_OUT_MINLEN_SFT,
	},
};

static const struct mtk_base_irq_data irq_data[MT6990_IRQ_NUM] = {
	[MT6990_IRQ_0] = {
		.id = MT6990_IRQ_0,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT0,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ0_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ0_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ0_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ0_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ0_MCU_EN_SFT,
	},
	[MT6990_IRQ_1] = {
		.id = MT6990_IRQ_1,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT1,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ1_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ1_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ1_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ1_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ1_MCU_EN_SFT,
	},
	[MT6990_IRQ_2] = {
		.id = MT6990_IRQ_2,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT2,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ2_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ2_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ2_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ2_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ2_MCU_EN_SFT,
	},
	[MT6990_IRQ_3] = {
		.id = MT6990_IRQ_3,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT3,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ3_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ3_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ3_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ3_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ3_MCU_EN_SFT,
	},
	[MT6990_IRQ_4] = {
		.id = MT6990_IRQ_4,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT4,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ4_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ4_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ4_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ4_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ4_MCU_EN_SFT,
	},
	[MT6990_IRQ_5] = {
		.id = MT6990_IRQ_5,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT5,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ5_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ5_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ5_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ5_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ5_MCU_EN_SFT,
	},
	[MT6990_IRQ_6] = {
		.id = MT6990_IRQ_6,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT6,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ6_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ6_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ6_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ6_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ6_MCU_EN_SFT,
	},
	[MT6990_IRQ_7] = {
		.id = MT6990_IRQ_7,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT7,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON1,
		.irq_fs_shift = IRQ7_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ7_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ7_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ7_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ7_MCU_EN_SFT,
	},
	[MT6990_IRQ_8] = {
		.id = MT6990_IRQ_8,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT8,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON2,
		.irq_fs_shift = IRQ8_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ8_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ8_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ8_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ8_MCU_EN_SFT,
	},
	[MT6990_IRQ_11] = {
		.id = MT6990_IRQ_11,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT11,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON2,
		.irq_fs_shift = IRQ11_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ11_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ11_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ11_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ11_MCU_EN_SFT,
	},
	[MT6990_IRQ_12] = {
		.id = MT6990_IRQ_12,
		.irq_cnt_reg = AFE_IRQ_MCU_CNT12,
		.irq_cnt_shift = AFE_IRQ_CNT_SHIFT,
		.irq_cnt_maskbit = AFE_IRQ_CNT_MASK,
		.irq_fs_reg = AFE_IRQ_MCU_CON2,
		.irq_fs_shift = IRQ12_MCU_MODE_SFT,
		.irq_fs_maskbit = IRQ12_MCU_MODE_MASK,
		.irq_en_reg = AFE_IRQ_MCU_CON0,
		.irq_en_shift = IRQ12_MCU_ON_SFT,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = IRQ12_MCU_CLR_SFT,
		.irq_ap_en_reg = AFE_IRQ_MCU_EN,
		.irq_ap_en_shift = IRQ12_MCU_EN_SFT,
	},
};

static const int memif_irq_usage[MT6990_MEMIF_NUM] = {
	/* TODO: verify each memif & irq */
	[MT6990_MEMIF_DL1] = MT6990_IRQ_0,
	[MT6990_MEMIF_DL2] = MT6990_IRQ_1,
	[MT6990_MEMIF_DL3] = MT6990_IRQ_2,
	[MT6990_MEMIF_DL4] = MT6990_IRQ_3,
	[MT6990_MEMIF_DL12] = MT6990_IRQ_4,
	[MT6990_MEMIF_DAI] = MT6990_IRQ_7,
	[MT6990_MEMIF_MOD_DAI] = MT6990_IRQ_11,
	[MT6990_MEMIF_DAI2] = MT6990_IRQ_12,
	[MT6990_MEMIF_VUL] = MT6990_IRQ_5,
	[MT6990_MEMIF_VUL2] = MT6990_IRQ_6,
	[MT6990_MEMIF_AWB] = MT6990_IRQ_0,
	[MT6990_MEMIF_AWB2] = MT6990_IRQ_0,
	[MT6990_MEMIF_VUL3] = MT6990_IRQ_0,
	[MT6990_MEMIF_VUL4] = MT6990_IRQ_0,
	[MT6990_MEMIF_VUL5] = MT6990_IRQ_0,
	[MT6990_MEMIF_HDMI] = MT6990_IRQ_8,
};

static bool mt6990_is_volatile_reg(struct device *dev, unsigned int reg)
{
	/* these auto-gen reg has read-only bit, so put it as volatile */
	/* volatile reg cannot be cached, so cannot be set when power off */
	switch (reg) {
	case AUDIO_TOP_CON0:	/* reg bit controlled by CCF */
	case AUDIO_TOP_CON1:	/* reg bit controlled by CCF */
	case AUDIO_TOP_CON2:
	case AUDIO_TOP_CON3:
	case AFE_DL1_CUR_MSB:
	case AFE_DL1_CUR:
	case AFE_DL1_END:
	case AFE_DL2_CUR_MSB:
	case AFE_DL2_CUR:
	case AFE_DL2_END:
	case AFE_DL3_CUR_MSB:
	case AFE_DL3_CUR:
	case AFE_DL3_END:
	case AFE_DL4_CUR_MSB:
	case AFE_DL4_CUR:
	case AFE_DL4_END:
	case AFE_DL12_CUR_MSB:
	case AFE_DL12_CUR:
	case AFE_DL12_END:
	case AFE_ADDA_SRC_DEBUG_MON0:
	case AFE_ADDA_SRC_DEBUG_MON1:
	case AFE_ADDA_UL_SRC_MON0:
	case AFE_ADDA_UL_SRC_MON1:
	case AFE_VUL_CUR_MSB:
	case AFE_VUL_CUR:
	case AFE_VUL_END:
	case AFE_SIDETONE_MON:
	case AFE_SIDETONE_CON0:
	case AFE_SIDETONE_COEFF:
	case AFE_VUL2_CUR_MSB:
	case AFE_VUL2_CUR:
	case AFE_VUL2_END:
	case AFE_VUL3_CUR_MSB:
	case AFE_VUL3_CUR:
	case AFE_VUL3_END:
	case AFE_MRGIF_MON0:
	case AFE_MRGIF_MON1:
	case AFE_MRGIF_MON2:
	case AFE_I2S_MON:
	case AFE_DAC_MON:
	case AFE_IRQ0_MCU_CNT_MON:
	case AFE_IRQ6_MCU_CNT_MON:
	case AFE_VUL4_CUR_MSB:
	case AFE_VUL4_CUR:
	case AFE_VUL4_END:
	case AFE_IRQ3_MCU_CNT_MON:
	case AFE_IRQ4_MCU_CNT_MON:
	case AFE_IRQ_MCU_STATUS:
	case AFE_IRQ_MCU_CLR:
	case AFE_IRQ_MCU_MON2:
	case AFE_IRQ1_MCU_CNT_MON:
	case AFE_IRQ2_MCU_CNT_MON:
	case AFE_IRQ5_MCU_CNT_MON:
	case AFE_IRQ7_MCU_CNT_MON:
	case AFE_GAIN1_CUR:
	case AFE_GAIN2_CUR:
	case AFE_SRAM_DELSEL_CON0:
	case AFE_SRAM_DELSEL_CON2:
	case AFE_SRAM_DELSEL_CON3:
	case PCM_INTF_CON2:
	case FPGA_CFG0:
	case FPGA_CFG1:
	case FPGA_CFG2:
	case FPGA_CFG3:
	case AUDIO_TOP_DBG_MON0:
	case AUDIO_TOP_DBG_MON1:
	case AFE_IRQ8_MCU_CNT_MON:
	case AFE_IRQ11_MCU_CNT_MON:
	case AFE_IRQ12_MCU_CNT_MON:
	case AFE_CBIP_MON0:
	case AFE_CBIP_SLV_MUX_MON0:
	case AFE_CBIP_SLV_DECODER_MON0:
	case AFE_AWB_CUR_MSB:
	case AFE_AWB_CUR:
	case AFE_AWB_END:
	case AFE_AWB2_CUR_MSB:
	case AFE_AWB2_CUR:
	case AFE_AWB2_END:
	case AFE_DAI_CUR_MSB:
	case AFE_DAI_CUR:
	case AFE_DAI_END:
	case AFE_DAI2_CUR_MSB:
	case AFE_DAI2_CUR:
	case AFE_DAI2_END:
	case AFE_MOD_DAI_CUR_MSB:
	case AFE_MOD_DAI_CUR:
	case AFE_MOD_DAI_END:
	case AFE_HDMI_OUT_CUR_MSB:
	case AFE_HDMI_OUT_CUR:
	case AFE_HDMI_OUT_END:
	case AFE_AWB_RCH_MON:
	case AFE_AWB_LCH_MON:
	case AFE_VUL_RCH_MON:
	case AFE_VUL_LCH_MON:
	case AFE_VUL2_RCH_MON:
	case AFE_VUL2_LCH_MON:
	case AFE_DAI_DATA_MON:
	case AFE_MOD_DAI_DATA_MON:
	case AFE_DAI2_DATA_MON:
	case AFE_AWB2_RCH_MON:
	case AFE_AWB2_LCH_MON:
	case AFE_VUL3_RCH_MON:
	case AFE_VUL3_LCH_MON:
	case AFE_VUL4_RCH_MON:
	case AFE_VUL4_LCH_MON:
	case AFE_VUL5_RCH_MON:
	case AFE_VUL5_LCH_MON:
	case AFE_DL1_RCH_MON:
	case AFE_DL1_LCH_MON:
	case AFE_DL2_RCH_MON:
	case AFE_DL2_LCH_MON:
	case AFE_DL12_RCH1_MON:
	case AFE_DL12_LCH1_MON:
	case AFE_DL12_RCH2_MON:
	case AFE_DL12_LCH2_MON:
	case AFE_DL3_RCH_MON:
	case AFE_DL3_LCH_MON:
	case AFE_DL4_RCH_MON:
	case AFE_DL4_LCH_MON:
	case AFE_VUL5_CUR_MSB:
	case AFE_VUL5_CUR:
	case AFE_VUL5_END:
	case AFE_ADDA_DL_SDM_FIFO_MON:
	case AFE_ADDA_DL_SRC_LCH_MON:
	case AFE_ADDA_DL_SRC_RCH_MON:
	case AFE_ADDA_DL_SDM_OUT_MON:
	case AFE_GENERAL1_ASRC_2CH_CON0:
	case AFE_GENERAL1_ASRC_2CH_CON2:
	case AFE_GENERAL1_ASRC_2CH_CON3:
	case AFE_GENERAL1_ASRC_2CH_CON4:
	case AFE_GENERAL1_ASRC_2CH_CON5:
	case AFE_GENERAL1_ASRC_2CH_CON7:
	case AFE_GENERAL1_ASRC_2CH_CON8:
	case AFE_GENERAL1_ASRC_2CH_CON12:
	case AFE_GENERAL1_ASRC_2CH_CON13:
	case AFE_GENERAL2_ASRC_2CH_CON0:
	case AFE_GENERAL2_ASRC_2CH_CON2:
	case AFE_GENERAL2_ASRC_2CH_CON3:
	case AFE_GENERAL2_ASRC_2CH_CON4:
	case AFE_GENERAL2_ASRC_2CH_CON5:
	case AFE_GENERAL2_ASRC_2CH_CON7:
	case AFE_GENERAL2_ASRC_2CH_CON8:
	case AFE_GENERAL2_ASRC_2CH_CON12:
	case AFE_GENERAL2_ASRC_2CH_CON13:
	case AFE_APLL1_TUNER_CFG:	/* [20:31] is monitor */
	case AFE_APLL2_TUNER_CFG:	/* [20:31] is monitor */
	case AFE_I2SIN_ASRC_2CH_CON0:
	case AFE_I2SIN_ASRC_2CH_CON2:
	case AFE_I2SIN_ASRC_2CH_CON3:
	case AFE_I2SIN_ASRC_2CH_CON4:
	case AFE_I2SIN_ASRC_2CH_CON5:
	case AFE_I2SIN_ASRC_2CH_CON7:
	case AFE_I2SIN_ASRC_2CH_CON8:
	case AFE_I2SIN_ASRC_2CH_CON12:
	case AFE_I2SIN_ASRC_2CH_CON13:
	case AFE_I2SOUT_ASRC_2CH_CON0:
	case AFE_I2SOUT_ASRC_2CH_CON2:
	case AFE_I2SOUT_ASRC_2CH_CON3:
	case AFE_I2SOUT_ASRC_2CH_CON4:
	case AFE_I2SOUT_ASRC_2CH_CON5:
	case AFE_I2SOUT_ASRC_2CH_CON7:
	case AFE_I2SOUT_ASRC_2CH_CON8:
	case AFE_I2SOUT_ASRC_2CH_CON12:
	case AFE_I2SOUT_ASRC_2CH_CON13:
	case ETDM_IN0_MON:
	case ETDM_OUT0_MON:
	case ETDM_IN1_MON:
	case ETDM_OUT1_MON:
		return true;
	default:
		return false;
	};
}

static const struct regmap_config mt6990_afe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,

	.volatile_reg = mt6990_is_volatile_reg,

	.max_register = AFE_MAX_REGISTER,
	.num_reg_defaults_raw = AFE_MAX_REGISTER,

	.cache_type = REGCACHE_FLAT,
};

static irqreturn_t mt6990_afe_irq_handler(int irq_id, void *dev)
{
	struct mtk_base_afe *afe = dev;
	struct mtk_base_afe_irq *irq;
	unsigned int status = 0;
	unsigned int status_mcu;
	unsigned int mcu_en = 0;
	int ret;
	int i;

	/* get irq that is sent to MCU */
	regmap_read(afe->regmap, AFE_IRQ_MCU_EN, &mcu_en);

	ret = regmap_read(afe->regmap, AFE_IRQ_MCU_STATUS, &status);
	/* only care IRQ which is sent to MCU */
	status_mcu = status & mcu_en & AFE_IRQ_STATUS_BITS;

	if (ret || status_mcu == 0) {
		dev_err(afe->dev, "%s(), irq status err, ret %d, status 0x%x, mcu_en 0x%x\n",
			__func__, ret, status, mcu_en);

		goto err_irq;
	}

	for (i = 0; i < MT6990_MEMIF_NUM; i++) {
		struct mtk_base_afe_memif *memif = &afe->memif[i];

		if (!memif->substream)
			continue;

		if (memif->irq_usage < 0)
			continue;

		irq = &afe->irqs[memif->irq_usage];

		if (status_mcu & (1 << irq->irq_data->irq_en_shift))
			snd_pcm_period_elapsed(memif->substream);
	}

err_irq:
	/* clear irq */
	regmap_write(afe->regmap,
		     AFE_IRQ_MCU_CLR,
		     status_mcu);

	return IRQ_HANDLED;
}

static int mt6990_afe_runtime_suspend(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);
	unsigned int value;
	int ret;
	dev_err(afe->dev, "%s()\n", __func__);

	if (!afe->regmap)
		goto skip_regmap;

	/* disable AFE */
	regmap_update_bits(afe->regmap, AFE_DAC_CON0, AFE_ON_MASK_SFT, 0x0 << AFE_ON_SFT);

	ret = regmap_read_poll_timeout(afe->regmap,
				       AFE_DAC_MON,
				       value,
				       (value & AFE_ON_RETM_MASK_SFT) == 0,
				       20,
				       1 * 1000 * 1000);
	if (ret)
		dev_warn(afe->dev, "%s(), ret %d\n", __func__, ret);

	/* make sure all irq status are cleared */
	regmap_write(afe->regmap, AFE_IRQ_MCU_CLR, 0xffffffff);
	regmap_write(afe->regmap, AFE_IRQ_MCU_CLR, 0xffffffff);

	/* reset sgen */
	regmap_write(afe->regmap, AFE_SINEGEN_CON0, 0x0);
	regmap_update_bits(afe->regmap, AFE_SINEGEN_CON2,
			   INNER_LOOP_BACK_MODE_MASK_SFT,
			   0x3f << INNER_LOOP_BACK_MODE_SFT);

	/* cache only */
	regcache_cache_only(afe->regmap, true);
	regcache_mark_dirty(afe->regmap);
skip_regmap:
	mt6990_afe_disable_clock(afe);
	return 0;
}


static int mt6990_afe_runtime_resume(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	int ret;
	ret = mt6990_afe_enable_clock(afe);
	if (ret)
		return ret;

	dev_err(afe->dev, "%s()\n", __func__);

	if (!afe->regmap)
		goto skip_regmap;

	regcache_cache_only(afe->regmap, false);
	regcache_sync(afe->regmap);

	/* enable audio sys DCM for power saving */
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0, AHB_IDLE_EN_EXT_MASK_SFT,
			0x1 << AHB_IDLE_EN_EXT_SFT);

	/* force cpu use 8_24 format when writing 32bit data */
	regmap_update_bits(afe->regmap, AFE_MEMIF_CON0,
			   CPU_HD_ALIGN_MASK_SFT, 0 << CPU_HD_ALIGN_SFT);

	/* set all output port to 24bit */
	regmap_write(afe->regmap, AFE_CONN_24BIT, 0xffffffff);
	regmap_write(afe->regmap, AFE_CONN_24BIT_1, 0xffffffff);

	/* enable AFE */
	regmap_update_bits(afe->regmap, AFE_DAC_CON0, AFE_ON_MASK_SFT, 0x1 << AFE_ON_SFT);

	mt6990_afe_gpio_request(afe, true, MT6990_DAI_VOLTAGE, 0);
skip_regmap:
	return 0;
}

#if defined(CONFIG_SND_SOC_PROSLIC)
static int mt6990_set_etdm(struct device *dev, bool enable)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	if (enable == true) {
		/* keep provide pcm_clk to Proslic */
		regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
						   0x1 << 20,
						   0x0 << 20);
		/* enable Out control */
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, AFE_ON_MASK_SFT, 0x1 << AFE_ON_SFT);
		regmap_update_bits(afe->regmap, AFE_DAC_CON0,
						   HDMI_OUT_ON_MASK_SFT,
						   0x1 << HDMI_OUT_ON_SFT);
		regmap_write(afe->regmap, AFE_APLL2_TUNER_CFG, 0x435);
		regmap_write(afe->regmap, AFE_HD_ENGEN_ENABLE, 0x2);
		regmap_write(afe->regmap, ETDM_IN0_CON0, 0x118F7940);
		regmap_write(afe->regmap, ETDM_IN0_CON2, 0x04108442);
		regmap_write(afe->regmap, ETDM_IN0_CON3, 0x09F80000);
		regmap_write(afe->regmap, ETDM_IN0_CON4, 0x06400100);
		regmap_write(afe->regmap, ETDM_OUT0_CON0, 0x018F7940);
		regmap_write(afe->regmap, ETDM_OUT0_CON4, 0x04000442);

		mt6990_afe_gpio_request(afe, true, MT6990_DAI_VOLTAGE, 0);
#ifdef PROSLIC_USE_PCM1
		mt6990_afe_gpio_request(afe, true, MT6990_DAI_PCM_1, 0);
		/* Use PCM1 */
		regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
			ETDM_OUT0_PCM_MUX_SEL_MASK_SFT,
			0x1 << ETDM_OUT0_PCM_MUX_SEL_SFT);
#else
		/* Use PCM0 */
		mt6990_afe_gpio_request(afe, true, MT6990_DAI_PCM_0, 0);
		regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
			ETDM_OUT0_PCM0_MUX_SEL_MASK_SFT,
			0x1 << ETDM_OUT0_PCM0_MUX_SEL_SFT);
#endif

		regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
			ETDM_OUT0_LRCK_DIV2_MASK_SFT,
			0x1 << ETDM_OUT0_LRCK_DIV2_SFT);

		regmap_write(afe->regmap, ETDM_IN0_CON0, 0x118F7941);
		regmap_write(afe->regmap, ETDM_OUT0_CON0, 0x018F7941);
	} else {
#ifndef PROSLIC_ALWAYS_ON
#ifdef PROSLIC_USE_PCM1
		/* Use PCM1 */
		mt6990_afe_gpio_request(afe, false, MT6990_DAI_PCM_1, 0);
#else
		/* Use PCM0 */
		mt6990_afe_gpio_request(afe, false, MT6990_DAI_PCM_0, 0);
#endif
		regmap_write(afe->regmap, ETDM_IN0_CON0, 0x118F7940);
		regmap_write(afe->regmap, ETDM_OUT0_CON0, 0x018F7940);
#endif
	}
	return 0;

}

static int mt6990_set_etdm1(struct device *dev, bool enable)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	if (enable == true) {
		/* keep provide pcm_clk to Proslic */
		regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
						   0x1 << 20,
						   0x0 << 20);
		/* enable Out control */
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, AFE_ON_MASK_SFT, 0x1 << AFE_ON_SFT);
		regmap_update_bits(afe->regmap, AFE_DAC_CON0,
						   HDMI_OUT_ON_MASK_SFT,
						   0x1 << HDMI_OUT_ON_SFT);
		regmap_write(afe->regmap, AFE_APLL2_TUNER_CFG, 0x435);
		regmap_write(afe->regmap, AFE_HD_ENGEN_ENABLE, 0x2);

		mt6990_afe_gpio_request(afe, true, MT6990_DAI_VOLTAGE, 0);
		mt6990_afe_gpio_request(afe, true, MT6990_DAI_ETDM1, 0);

		regmap_write(afe->regmap, ETDM_IN1_CON0, 0x118F7940);
		regmap_write(afe->regmap, ETDM_IN1_CON2, 0x04108442);
		regmap_write(afe->regmap, ETDM_IN1_CON3, 0x09F80000);
		regmap_write(afe->regmap, ETDM_IN1_CON4, 0x06400100);
		regmap_write(afe->regmap, ETDM_OUT1_CON0, 0x018F7940);
		regmap_write(afe->regmap, ETDM_OUT1_CON4, 0x04000442);
		regmap_update_bits(afe->regmap, ETDM_0_3_COWORK_CON3,
			ETDM_OUT1_LRCK_DIV2_MASK_SFT,
			0x1 << ETDM_OUT1_LRCK_DIV2_SFT);

		regmap_write(afe->regmap, ETDM_IN1_CON0, 0x118F7941);
		regmap_write(afe->regmap, ETDM_OUT1_CON0, 0x018F7941);
	} else {
#ifndef PROSLIC_ALWAYS_ON
		mt6990_afe_gpio_request(afe, false, MT6990_DAI_ETDM1, 0);
		regmap_write(afe->regmap, ETDM_IN1_CON0, 0x118F7940);
		regmap_write(afe->regmap, ETDM_OUT1_CON0, 0x018F7940);
#endif
	}
	return 0;

}
#endif

static int mt6990_set_memif_sram_mode(struct device *dev,
				      enum mtk_audio_sram_mode sram_mode)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);
	int reg_bit = sram_mode == MTK_AUDIO_SRAM_NORMAL_MODE ? 1 : 0;

	regmap_update_bits(afe->regmap, AFE_DL1_CON0,
			   DL1_NORMAL_MODE_MASK_SFT,
			   reg_bit << DL1_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_DL2_CON0,
			   DL2_NORMAL_MODE_MASK_SFT,
			   reg_bit << DL2_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_DL3_CON0,
			   DL3_NORMAL_MODE_MASK_SFT,
			   reg_bit << DL3_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_DL4_CON0,
			   DL4_NORMAL_MODE_MASK_SFT,
			   reg_bit << DL4_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_DL12_CON0,
			   DL12_NORMAL_MODE_MASK_SFT,
			   reg_bit << DL12_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_AWB_CON0,
			   AWB_NORMAL_MODE_MASK_SFT,
			   reg_bit << AWB_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_AWB2_CON0,
			   AWB2_NORMAL_MODE_MASK_SFT,
			   reg_bit << AWB2_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_VUL_CON0,
			   VUL_NORMAL_MODE_MASK_SFT,
			   reg_bit << VUL_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_VUL2_CON0,
			   VUL2_NORMAL_MODE_MASK_SFT,
			   reg_bit << VUL2_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_VUL3_CON0,
			   VUL3_NORMAL_MODE_MASK_SFT,
			   reg_bit << VUL3_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_VUL4_CON0,
			   VUL4_NORMAL_MODE_MASK_SFT,
			   reg_bit << VUL4_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_VUL5_CON0,
			   VUL5_NORMAL_MODE_MASK_SFT,
			   reg_bit << VUL5_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_DAI_CON0,
			   DAI_NORMAL_MODE_MASK_SFT,
			   reg_bit << DAI_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_DAI2_CON0,
			   DAI2_NORMAL_MODE_MASK_SFT,
			   reg_bit << DAI2_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_MOD_DAI_CON0,
			   MOD_DAI_NORMAL_MODE_MASK_SFT,
			   reg_bit << MOD_DAI_NORMAL_MODE_SFT);
	regmap_update_bits(afe->regmap, AFE_HDMI_OUT_CON0,
			   HDMI_OUT_NORMAL_MODE_MASK_SFT,
			   reg_bit << HDMI_OUT_NORMAL_MODE_SFT);
	return 0;
}

static int mt6990_set_sram_mode(struct device *dev,
				enum mtk_audio_sram_mode sram_mode)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	/* set memif sram mode */
	mt6990_set_memif_sram_mode(dev, sram_mode);

	if (sram_mode == MTK_AUDIO_SRAM_COMPACT_MODE)
		/* cpu use compact mode when access sram data */
		regmap_update_bits(afe->regmap, AFE_MEMIF_CON0,
				   CPU_COMPACT_MODE_MASK_SFT,
				   0x1 << CPU_COMPACT_MODE_SFT);
	else
		/* cpu use normal mode when access sram data */
		regmap_update_bits(afe->regmap, AFE_MEMIF_CON0,
				   CPU_COMPACT_MODE_MASK_SFT,
				   0x0 << CPU_COMPACT_MODE_SFT);

	return 0;
}

static const struct mtk_audio_sram_ops mt6990_sram_ops = {
	.set_sram_mode = mt6990_set_sram_mode,
};

static int mt6990_afe_component_probe(struct snd_soc_component *component)
{
	mtk_afe_add_sub_dai_control(component);
	mt6990_add_misc_control(component);
	return 0;
}

static const struct snd_soc_component_driver mt6990_afe_component = {
	.name = AFE_PCM_NAME,
	.ops = &mtk_afe_pcm_ops,
	.pcm_new = mtk_afe_pcm_new,
	.pcm_free = mtk_afe_pcm_free,
	.probe = mt6990_afe_component_probe,
};

static ssize_t mt6990_debugfs_read(struct file *file, char __user *buf,
				    size_t count, loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	const int size = 16384;
	char *buffer = NULL; /* for reduce kernel stack */
	int n = 0;
	int ret = 0;
	unsigned int value;
	int i;

	buffer = kmalloc(size, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	n += scnprintf(buffer + n, size - n,
		       "mtkaif_calibration_ok %d\n",
		       afe_priv->mtkaif_calibration_ok);

	n += scnprintf(buffer + n, size - n,
		       "mtkaif calibration phase %d, %d, %d, %d\n",
		       afe_priv->mtkaif_chosen_phase[0],
		       afe_priv->mtkaif_chosen_phase[1],
		       afe_priv->mtkaif_chosen_phase[2],
		       afe_priv->mtkaif_chosen_phase[3]);

	n += scnprintf(buffer + n, size - n,
		       "mtkaif calibration cycle %d, %d, %d, %d\n",
		       afe_priv->mtkaif_phase_cycle[0],
		       afe_priv->mtkaif_phase_cycle[1],
		       afe_priv->mtkaif_phase_cycle[2],
		       afe_priv->mtkaif_phase_cycle[3]);

	for (i = 0; i < afe->memif_size; i++) {
		n += scnprintf(buffer + n, size - n,
			       "memif[%d], irq_usage %d\n",
			       i, afe->memif[i].irq_usage);
	}

	regmap_read(afe->regmap, AUDIO_TOP_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AUDIO_TOP_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AUDIO_TOP_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AUDIO_TOP_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAC_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAC_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_I2S_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_I2S_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAIBT_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAIBT_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN4, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN4 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_I2S_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_I2S_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_I2S_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_I2S_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MRGIF_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MRGIF_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN5, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN5 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_24BIT, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_24BIT = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN6, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN6 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SRC2_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SRC2_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SRC2_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SRC2_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_UL_SRC_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_UL_SRC_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_UL_SRC_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_UL_SRC_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_TOP_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_TOP_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_UL_DL_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_UL_DL_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_SRC_DEBUG, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_SRC_DEBUG = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_SRC_DEBUG_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_SRC_DEBUG_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_SRC_DEBUG_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_SRC_DEBUG_MON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_UL_SRC_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_UL_SRC_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_UL_SRC_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_UL_SRC_MON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SIDETONE_DEBUG, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SIDETONE_DEBUG = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SIDETONE_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SIDETONE_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SINEGEN_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SINEGEN_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SIDETONE_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SIDETONE_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SIDETONE_COEFF, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SIDETONE_COEFF = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SIDETONE_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SIDETONE_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SIDETONE_GAIN, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SIDETONE_GAIN = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SINEGEN_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SINEGEN_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_TOP_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_TOP_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_BUS_CFG, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_BUS_CFG = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_PREDIS_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_PREDIS_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_PREDIS_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_PREDIS_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MRGIF_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MRGIF_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MRGIF_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MRGIF_MON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MRGIF_MON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MRGIF_MON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_I2S_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_I2S_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_IIR_COEF_02_01, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_IIR_COEF_02_01 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_IIR_COEF_04_03, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_IIR_COEF_04_03 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_IIR_COEF_06_05, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_IIR_COEF_06_05 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_IIR_COEF_08_07, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_IIR_COEF_08_07 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_IIR_COEF_10_09, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_IIR_COEF_10_09 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAC_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAC_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT6, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT6 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT8, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT8 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ0_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ0_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ6_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ6_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_CONN0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_CONN0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ3_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ3_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ4_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ4_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_STATUS, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_STATUS = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CLR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_EN, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_EN = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_MON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_MON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT5, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT5 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ1_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ1_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ2_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ2_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ5_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ5_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT7, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT7 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ7_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ7_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT4, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT4 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT11, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT11 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_APLL1_TUNER_CFG, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_APLL1_TUNER_CFG = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_APLL2_TUNER_CFG, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_APLL2_TUNER_CFG = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN33, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN33 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ_MCU_CNT12, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ_MCU_CNT12 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN1_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN1_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN1_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN1_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN1_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN1_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN1_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN1_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN7, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN7 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN1_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN1_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN2_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN2_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN2_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN2_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN2_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN2_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN2_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN2_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN8, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN8 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GAIN2_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GAIN2_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN9, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN9 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN10, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN10 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN11, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN11 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN12, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN12 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN13, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN13 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN14, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN14 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN15, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN15 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN16, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN16 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN17, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN17 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN18, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN18 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN19, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN19 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN20, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN20 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN21, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN21 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN22, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN22 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN23, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN23 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN24, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN24 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_RS, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_RS = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_DI, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_DI = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN25, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN25 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN26, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN26 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN27, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN27 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN28, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN28 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN29, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN29 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN30, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN30 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN31, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN31 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN32, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN32 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SRAM_DELSEL_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SRAM_DELSEL_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SRAM_DELSEL_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SRAM_DELSEL_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_SRAM_DELSEL_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_SRAM_DELSEL_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, PCM0_INTF_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "PCM0_INTF_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, PCM0_INTF_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "PCM0_INTF_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, PCM_INTF_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "PCM_INTF_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, PCM_INTF_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "PCM_INTF_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, PCM2_INTF_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "PCM2_INTF_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_TDM_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_TDM_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_TDM_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_TDM_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN34, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN34 = 0x%x\n", value);
	regmap_read(afe->regmap, FPGA_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "FPGA_CFG0 = 0x%x\n", value);
	regmap_read(afe->regmap, FPGA_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "FPGA_CFG1 = 0x%x\n", value);
	regmap_read(afe->regmap, FPGA_CFG2, &value);
	n += scnprintf(buffer + n, size - n,
		       "FPGA_CFG2 = 0x%x\n", value);
	regmap_read(afe->regmap, FPGA_CFG3, &value);
	n += scnprintf(buffer + n, size - n,
		       "FPGA_CFG3 = 0x%x\n", value);
	regmap_read(afe->regmap, AUDIO_TOP_DBG_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_DBG_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AUDIO_TOP_DBG_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_DBG_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AUDIO_TOP_DBG_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AUDIO_TOP_DBG_MON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ8_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ8_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ11_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ11_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_IRQ12_MCU_CNT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_IRQ12_MCU_CNT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG4, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG4 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG5, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG5 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG6, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG6 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG7, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG7 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG8, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG8 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG9, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG9 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG10, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG10 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG11, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG11 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG12, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG12 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG13, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG13 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG14, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG14 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL_REG15, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL_REG15 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CBIP_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CBIP_CFG0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CBIP_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CBIP_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CBIP_SLV_MUX_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CBIP_SLV_MUX_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CBIP_SLV_DECODER_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CBIP_SLV_DECODER_MON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MEMIF_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MEMIF_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN0_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN0_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN1_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN1_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN2_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN2_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN3_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN3_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN4_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN4_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN5_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN5_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN6_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN6_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN7_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN7_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN8_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN8_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN9_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN9_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN10_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN10_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN11_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN11_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN12_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN12_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN13_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN13_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN14_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN14_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN15_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN15_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN16_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN16_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN17_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN17_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN18_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN18_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN19_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN19_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN20_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN20_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN21_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN21_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN22_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN22_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN23_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN23_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN24_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN24_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN25_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN25_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN26_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN26_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN27_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN27_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN28_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN28_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN29_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN29_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN30_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN30_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN31_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN31_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN32_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN32_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN33_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN33_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN34_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN34_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_RS_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_RS_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_DI_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_DI_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_24BIT_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_24BIT_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN_REG, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN_REG = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN35, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN35 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN36, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN36 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN37, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN37 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN38, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN38 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN35_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN35_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN36_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN36_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN37_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN37_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN38_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN38_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN39, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN39 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN40, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN40 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN41, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN41 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN42, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN42 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN39_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN39_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN40_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN40_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN41_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN41_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN42_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN42_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_I2S_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_I2S_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN43, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN43 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN43_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN43_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HDMI_OUT_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HDMI_OUT_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL2_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL2_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI_DATA_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI_DATA_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_MOD_DAI_DATA_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_MOD_DAI_DATA_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DAI2_DATA_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DAI2_DATA_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_AWB2_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_AWB2_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL3_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL3_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL4_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL4_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL1_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL1_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL2_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL2_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_RCH1_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_RCH1_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_LCH1_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_LCH1_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_RCH2_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_RCH2_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL12_LCH2_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL12_LCH2_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL3_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL3_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_DL4_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_DL4_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_BASE_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_BASE_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_BASE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_BASE = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_CUR_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_CUR_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_CUR, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_CUR = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_END_MSB, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_END_MSB = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_VUL5_END, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_VUL5_END = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SDM_DCCOMP_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SDM_DCCOMP_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SDM_TEST, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SDM_TEST = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_DC_COMP_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_DC_COMP_CFG0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_DC_COMP_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_DC_COMP_CFG1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SDM_FIFO_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SDM_FIFO_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SRC_LCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SRC_LCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SRC_RCH_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SRC_RCH_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SDM_OUT_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SDM_OUT_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_ADDA_DL_SDM_DITHER_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_ADDA_DL_SDM_DITHER_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN44, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN44 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN45, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN45 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN46, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN46 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN47, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN47 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN44_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN44_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN45_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN45_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN46_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN46_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN47_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN47_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_HD_ENGEN_ENABLE, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_HD_ENGEN_ENABLE = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON5 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON6 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_CON7 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON5 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON6 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON7 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_CON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_CON8 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON5 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON6 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_CON7 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON5 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON6 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON7 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_CON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_CON8 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_0_3_COWORK_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_0_3_COWORK_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_0_3_COWORK_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_0_3_COWORK_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_0_3_COWORK_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_0_3_COWORK_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN0_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN0_MON = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT0_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT0_MON = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_IN1_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_IN1_MON = 0x%x\n", value);
	regmap_read(afe->regmap, ETDM_OUT1_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "ETDM_OUT1_MON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON5 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON6 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON7 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON8 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON9, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON9 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON10, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON10 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON12, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON12 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL1_ASRC_2CH_CON13, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL1_ASRC_2CH_CON13 = 0x%x\n", value);
	regmap_read(afe->regmap, GENERAL_ASRC_MODE, &value);
	n += scnprintf(buffer + n, size - n,
		       "GENERAL_ASRC_MODE = 0x%x\n", value);
	regmap_read(afe->regmap, GENERAL_ASRC_EN_ON, &value);
	n += scnprintf(buffer + n, size - n,
		       "GENERAL_ASRC_EN_ON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN48, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN48 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN49, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN49 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN50, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN50 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN51, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN51 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN52, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN52 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN53, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN53 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CM_CON, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CM_CON = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN48_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN48_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN49_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN49_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN50_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN50_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN51_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN51_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN52_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN52_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_CONN53_1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_CONN53_1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON0 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON1 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON2 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON3 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON4 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON5 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON6 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON7 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON8 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON9, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON9 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON10, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON10 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON12, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON12 = 0x%x\n", value);
	regmap_read(afe->regmap, AFE_GENERAL2_ASRC_2CH_CON13, &value);
	n += scnprintf(buffer + n, size - n,
		       "AFE_GENERAL2_ASRC_2CH_CON13 = 0x%x\n", value);

	ret = simple_read_from_buffer(buf, count, pos, buffer, n);
	kfree(buffer);
	return ret;
}

static const struct mtk_afe_debug_cmd mt6990_debug_cmds[] = {
	MTK_AFE_DBG_CMD("write_reg", mtk_afe_debug_write_reg),
	{}
};

static const struct file_operations mt6990_debugfs_ops = {
	.open = mtk_afe_debugfs_open,
	.write = mtk_afe_debugfs_write,
	.read = mt6990_debugfs_read,
};

static const struct snd_soc_component_driver mt6990_afe_pcm_component = {
	.name = "mt6990-afe-pcm-dai",
};

static int mt6990_dai_memif_register(struct mtk_base_afe *afe)
{
	struct mtk_base_afe_dai *dai;

	dai = devm_kzalloc(afe->dev, sizeof(*dai), GFP_KERNEL);
	if (!dai)
		return -ENOMEM;

	list_add(&dai->list, &afe->sub_dais);

	dai->dai_drivers = mt6990_memif_dai_driver;
	dai->num_dai_drivers = ARRAY_SIZE(mt6990_memif_dai_driver);

	dai->controls = mt6990_pcm_kcontrols;
	dai->num_controls = ARRAY_SIZE(mt6990_pcm_kcontrols);
	dai->dapm_widgets = mt6990_memif_widgets;
	dai->num_dapm_widgets = ARRAY_SIZE(mt6990_memif_widgets);
	dai->dapm_routes = mt6990_memif_routes;
	dai->num_dapm_routes = ARRAY_SIZE(mt6990_memif_routes);
	return 0;
}

typedef int (*dai_register_cb)(struct mtk_base_afe *);
static const dai_register_cb dai_register_cbs[] = {
	mt6990_dai_adda_register,
	mt6990_dai_i2s_register,
	mt6990_dai_hw_gain_register,
	mt6990_dai_src_register,
	mt6990_dai_pcm_register,
	mt6990_dai_tdm_register,
	mt6990_dai_etdm_register,
	mt6990_dai_hostless_register,
	mt6990_dai_memif_register,
};



static int mt6990_afe_pcm_dev_probe(struct platform_device *pdev)
{
	int ret, i;
	int irq_id;
	struct mtk_base_afe *afe;
	struct mt6990_afe_private *afe_priv;
	struct resource *res;
	struct device *dev;
/*	struct arm_smccc_res smccc_res; */


	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(34));
	if (ret)
		return ret;

	afe = devm_kzalloc(&pdev->dev, sizeof(*afe), GFP_KERNEL);
	if (!afe)
		return -ENOMEM;
	platform_set_drvdata(pdev, afe);

	afe->platform_priv = devm_kzalloc(&pdev->dev, sizeof(*afe_priv),
					  GFP_KERNEL);
	if (!afe->platform_priv)
		return -ENOMEM;
	afe_priv = afe->platform_priv;

	afe->dev = &pdev->dev;
	dev = afe->dev;
		
	pr_info("%s()!!!!!!!!!!!!!!!!!!!!!!!!!n", __func__);
	/* init audio related clock */
	ret = mt6990_init_clock(afe);
	if (ret) {
		dev_err(dev, "init clock error\n");
		return ret;
	}
	pm_runtime_enable(&pdev->dev);
	if (!pm_runtime_enabled(&pdev->dev))
		goto err_pm_disable;

	/* regmap init */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	afe->base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(afe->base_addr))
		return PTR_ERR(afe->base_addr);

	/* enable clock for regcache get default value from hw */
	pm_runtime_get_sync(&pdev->dev);

	afe->regmap = devm_regmap_init_mmio(&pdev->dev, afe->base_addr,
					    &mt6990_afe_regmap_config);
	if (IS_ERR(afe->regmap))
		return PTR_ERR(afe->regmap);

	pm_runtime_put_sync(&pdev->dev);

	regcache_cache_only(afe->regmap, true);
	regcache_mark_dirty(afe->regmap);

	/* init gpio */
	ret = mt6990_afe_gpio_init(afe);
	if (ret)
		dev_err(dev, "init gpio error\n");

	/* init sram */
	afe->sram = devm_kzalloc(&pdev->dev, sizeof(struct mtk_audio_sram),
				GFP_KERNEL);
	if (!afe->sram)
		return -ENOMEM;

	ret = mtk_audio_sram_init(dev, afe->sram, &mt6990_sram_ops);
	if (ret)
		return ret;
	/* init memif */
	afe->memif_size = MT6990_MEMIF_NUM;
	afe->memif = devm_kcalloc(dev, afe->memif_size, sizeof(*afe->memif),
				  GFP_KERNEL);

	if (!afe->memif)
		return -ENOMEM;

	for (i = 0; i < afe->memif_size; i++) {
		afe->memif[i].data = &memif_data[i];
		afe->memif[i].irq_usage = memif_irq_usage[i];
		afe->memif[i].const_irq = 1;
	}
	afe->memif[MT6990_DEEP_MEMIF].ack = mtk_sp_clean_written_buffer_ack;

	mutex_init(&afe->irq_alloc_lock);	/* needed when dynamic irq */
	/* init irq */
	afe->irqs_size = MT6990_IRQ_NUM;
	afe->irqs = devm_kcalloc(dev, afe->irqs_size, sizeof(*afe->irqs),
				 GFP_KERNEL);

	if (!afe->irqs)
		return -ENOMEM;

	for (i = 0; i < afe->irqs_size; i++)
		afe->irqs[i].irq_data = &irq_data[i];

	/* request irq */
	irq_id = platform_get_irq(pdev, 0);
	if (irq_id <= 0) {
		dev_err(dev, "%s no irq found\n", dev->of_node->name);
		return irq_id < 0 ? irq_id : -ENXIO;
	}
	ret = devm_request_irq(dev, irq_id, mt6990_afe_irq_handler,
			       IRQF_TRIGGER_NONE, "Afe_ISR_Handle", (void *)afe);
	if (ret) {
		dev_err(dev, "could not request_irq for Afe_ISR_Handle\n");
		return ret;
	}
#if 0
	/* init arm_smccc_smc call */
	arm_smccc_smc(MTK_SIP_AUDIO_CONTROL, MTK_AUDIO_SMC_OP_INIT,
		      0, 0, 0, 0, 0, 0, &smccc_res);
#endif
	dev_err(afe->dev, "%s()line %d\n", __func__,__LINE__);

	/* init sub_dais */
	INIT_LIST_HEAD(&afe->sub_dais);

	for (i = 0; i < ARRAY_SIZE(dai_register_cbs); i++) {
		ret = dai_register_cbs[i](afe);
		if (ret) {
			dev_warn(afe->dev, "dai register i %d fail, ret %d\n",
				 i, ret);
			goto err_pm_disable;
		}
	}

	/* init dai_driver and component_driver */
	ret = mtk_afe_combine_sub_dai(afe);
	if (ret) {
		dev_warn(afe->dev, "mtk_afe_combine_sub_dai fail, ret %d\n",
			 ret);
		goto err_pm_disable;
	}

	/* others */
	afe->mtk_afe_hardware = &mt6990_afe_hardware;
	afe->memif_fs = mt6990_memif_fs;
	afe->irq_fs = mt6990_irq_fs;
	afe->get_dai_fs = mt6990_get_dai_fs;
	afe->get_memif_pbuf_size = mt6990_get_memif_pbuf_size;

	afe->runtime_resume = mt6990_afe_runtime_resume;
	afe->runtime_suspend = mt6990_afe_runtime_suspend;

	afe->request_dram_resource = mt6990_afe_dram_request;
	afe->release_dram_resource = mt6990_afe_dram_release;
	/* debugfs */
	afe->debug_cmds = mt6990_debug_cmds;
	afe->debugfs = debugfs_create_file("mtksocaudio",
					   S_IFREG | 0440, NULL,
					   afe, &mt6990_debugfs_ops);

	/* register component */
	ret = devm_snd_soc_register_component(&pdev->dev,
					      &mt6990_afe_component,
					      NULL, 0);
	if (ret) {
		dev_warn(dev, "err_platform\n");
		goto err_pm_disable;
	}


	ret = devm_snd_soc_register_component(&pdev->dev,
					      &mt6990_afe_pcm_component,
					      afe->dai_drivers,
					      afe->num_dai_drivers);
	if (ret) {
		dev_warn(dev, "err_dai_component\n");
		goto err_dai_component;
	}

#if defined(CONFIG_SND_SOC_PROSLIC)
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef PROSLIC_INIT_WHEN_PROBE
	pm_runtime_get_sync(&pdev->dev);

	mt6990_apll2_enable(afe);
	mt6990_apll1_enable(afe);
	mt6990_set_etdm(&pdev->dev, true);
	mt6990_set_etdm1(&pdev->dev, true);
	ProSLIC_HWInit();
	mt6990_set_etdm(&pdev->dev, false);
	ProSLIC_HWInit2();
	mt6990_set_etdm1(&pdev->dev, false);
	mt6990_apll2_disable(afe);
	mt6990_apll1_disable(afe);

	pm_runtime_put_sync(&pdev->dev);
#endif
#endif
#endif

	return 0;

err_dai_component:
	snd_soc_unregister_component(&pdev->dev);

err_pm_disable:
	pm_runtime_disable(&pdev->dev);

	return ret;
}

static int mt6990_afe_pcm_dev_remove(struct platform_device *pdev)
{
#ifndef PROSLIC_ALWAYS_ON
	struct mtk_base_afe *afe = platform_get_drvdata(pdev);
#endif
	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		mt6990_afe_runtime_suspend(&pdev->dev);
#ifndef PROSLIC_ALWAYS_ON 
	/* disable afe clock */
	mt6990_afe_disable_clock(afe);
#endif
	return 0;
}

static const struct of_device_id mt6990_afe_pcm_dt_match[] = {
	{ .compatible = "mediatek,mt6990-sound", },
	{},
};
MODULE_DEVICE_TABLE(of, mt6990_afe_pcm_dt_match);

static int mt6990_afe_suspend(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	dev_info(afe->dev, "%s()\n", __func__);

	mt6990_afe_suspend_clock(afe);

	return 0;
}

static int mt6990_afe_resume(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	dev_info(afe->dev, "%s()\n", __func__);

	mt6990_afe_resume_clock(afe);

	return 0;
}

static const struct dev_pm_ops mt6990_afe_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mt6990_afe_suspend,
				mt6990_afe_resume)
	SET_RUNTIME_PM_OPS(mt6990_afe_runtime_suspend,
			   mt6990_afe_runtime_resume, NULL)
};

static struct platform_driver mt6990_afe_pcm_driver = {
	.driver = {
		   .name = "mt6990-audio",
		   .of_match_table = mt6990_afe_pcm_dt_match,
#if IS_ENABLED(CONFIG_PM)
		   .pm = &mt6990_afe_pm_ops,
#endif
	},
	.probe = mt6990_afe_pcm_dev_probe,
	.remove = mt6990_afe_pcm_dev_remove,
};

module_platform_driver(mt6990_afe_pcm_driver);

MODULE_DESCRIPTION("Mediatek ALSA SoC AFE platform driver for 6990");
MODULE_AUTHOR("Yen-Chang Chen <yen-chang.chen@mediatek.com>");
MODULE_LICENSE("GPL v2");
