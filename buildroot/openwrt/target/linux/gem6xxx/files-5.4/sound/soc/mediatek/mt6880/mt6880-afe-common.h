/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#ifndef _MT_6880_AFE_COMMON_H_
#define _MT_6880_AFE_COMMON_H_
#include <sound/soc.h>
#include <linux/list.h>
#include <linux/regmap.h>
#include "mt6880-reg.h"
#include "../common/mtk-base-afe.h"

enum {
	MT6880_MEMIF_DL1,
	MT6880_MEMIF_DL12,
	MT6880_MEMIF_DL2,
	MT6880_MEMIF_DL3,
	MT6880_MEMIF_DL4,
	MT6880_MEMIF_DAI,
	MT6880_MEMIF_DAI2,
	MT6880_MEMIF_MOD_DAI,
	MT6880_MEMIF_VUL,
	MT6880_MEMIF_VUL2,
	MT6880_MEMIF_VUL3,
	MT6880_MEMIF_VUL4,
	MT6880_MEMIF_VUL5,
	MT6880_MEMIF_AWB,
	MT6880_MEMIF_AWB2,
	MT6880_MEMIF_HDMI,
	MT6880_MEMIF_NUM,
	MT6880_DAI_ADDA = MT6880_MEMIF_NUM,
	MT6880_DAI_AP_DMIC,
	MT6880_DAI_AP_DMIC_CH34,
	MT6880_DAI_VOW,
	MT6880_DAI_CONNSYS_I2S,
	MT6880_DAI_I2S_0,
	MT6880_DAI_I2S_1,
	MT6880_DAI_I2S_2,
	MT6880_DAI_I2S_3,
	MT6880_DAI_I2S_4,
	MT6880_DAI_I2S_5,
	MT6880_DAI_I2S_6,
	MT6880_DAI_HW_GAIN_1,
	MT6880_DAI_HW_GAIN_2,
	MT6880_DAI_SRC_1,
	MT6880_DAI_SRC_2,
	MT6880_DAI_PCM_1,
	MT6880_DAI_PCM_2,
	MT6880_DAI_TDM,
	MT6880_DAI_ETDM,
	MT6880_DAI_HOSTLESS_LPBK,
	MT6880_DAI_HOSTLESS_FM,
	MT6880_DAI_HOSTLESS_SPEECH,
	MT6880_DAI_HOSTLESS_SPH_ECHO_REF,
	MT6880_DAI_HOSTLESS_SPK_INIT,
	MT6880_DAI_HOSTLESS_IMPEDANCE,
	MT6880_DAI_HOSTLESS_SRC_1,	/* just an exmpale */
	MT6880_DAI_HOSTLESS_SRC_BARGEIN,
	MT6880_DAI_HOSTLESS_UL1,
	MT6880_DAI_HOSTLESS_UL2,
	MT6880_DAI_HOSTLESS_UL3,
	MT6880_DAI_HOSTLESS_UL6,
	MT6880_DAI_HOSTLESS_DSP_DL,
	MT6880_DAI_NUM,
};

#define MT6880_RECORD_MEMIF MT6880_MEMIF_VUL
#define MT6880_ECHO_REF_MEMIF MT6880_MEMIF_AWB
#define MT6880_PRIMARY_MEMIF MT6880_MEMIF_DL1
#define MT6880_FAST_MEMIF MT6880_MEMIF_DL2
#define MT6880_DEEP_MEMIF MT6880_MEMIF_DL3
#define MT6880_VOIP_MEMIF MT6880_MEMIF_DL12
#define MT6880_BARGEIN_MEMIF MT6880_MEMIF_AWB

enum {
	MT6880_IRQ_0,
	MT6880_IRQ_1,
	MT6880_IRQ_2,
	MT6880_IRQ_3,
	MT6880_IRQ_4,
	MT6880_IRQ_5,
	MT6880_IRQ_6,
	MT6880_IRQ_7,
	MT6880_IRQ_8,/* used only for TDM */
	MT6880_IRQ_11,
	MT6880_IRQ_12,
	MT6880_IRQ_NUM,
};

enum {
	MTKAIF_PROTOCOL_1 = 0,
	MTKAIF_PROTOCOL_2,
	MTKAIF_PROTOCOL_2_CLK_P2,
};

enum {
	MTK_AFE_ADDA_DL_GAIN_MUTE = 0,
	MTK_AFE_ADDA_DL_GAIN_NORMAL = 0xf74f,
	/* SA suggest apply -0.3db to audio/speech path */
};

/* MCLK */
enum {
	MT6880_I2S0_MCK = 0,
	MT6880_I2S1_MCK,
	MT6880_I2S2_MCK,
	MT6880_I2S3_MCK,
	MT6880_I2S4_MCK,
	MT6880_TDM_MCK,
	MT6880_TDM_BCK,
	MT6880_I2S5_MCK,
	MT6880_I2S6_MCK,
	MT6880_MCK_NUM,
};

/* SMC CALL Operations */
enum mtk_audio_smc_call_op {
	MTK_AUDIO_SMC_OP_INIT = 0,
	MTK_AUDIO_SMC_OP_DRAM_REQUEST,
	MTK_AUDIO_SMC_OP_DRAM_RELEASE,
	MTK_AUDIO_SMC_OP_FM_REQUEST,
	MTK_AUDIO_SMC_OP_FM_RELEASE,
	MTK_AUDIO_SMC_OP_ADSP_REQUEST,
	MTK_AUDIO_SMC_OP_ADSP_RELEASE,
	MTK_AUDIO_SMC_OP_NUM
};

struct snd_pcm_substream;
struct mtk_base_irq_data;
struct clk;

struct mt6880_afe_private {
	struct clk **clk;
	struct regmap *topckgen;
	int irq_cnt[MT6880_MEMIF_NUM];
	int stf_positive_gain_db;
	int dram_resource_counter;
	int sgen_mode;
	int sgen_rate;
	int sgen_amplitude;
	/* usb call */
	int usb_call_echo_ref_enable;
	int usb_call_echo_ref_size;
	bool usb_call_echo_ref_reallocate;
	/* deep buffer playback */
	int deep_playback_state;
	/* fast playback */
	int fast_playback_state;
	/* primary playback */
	int primary_playback_state;
	/* voip rx */
	int voip_rx_state;
	/* xrun assert */
	int xrun_assert[MT6880_MEMIF_NUM];

	/* dai */
	bool dai_on[MT6880_DAI_NUM];
	void *dai_priv[MT6880_DAI_NUM];

	/* adda */
	int mtkaif_protocol;
	bool mtkaif_calibration_ok;
	int mtkaif_chosen_phase[4];
	int mtkaif_phase_cycle[4];
	int mtkaif_calibration_num_phase;
	int mtkaif_dmic;
	int mtkaif_dmic_ch34;

	/* mck */
	int mck_rate[MT6880_MCK_NUM];
};

int mt6880_dai_adda_register(struct mtk_base_afe *afe);
int mt6880_dai_i2s_register(struct mtk_base_afe *afe);
int mt6880_dai_hw_gain_register(struct mtk_base_afe *afe);
int mt6880_dai_src_register(struct mtk_base_afe *afe);
int mt6880_dai_pcm_register(struct mtk_base_afe *afe);
int mt6880_dai_tdm_register(struct mtk_base_afe *afe);
int mt6880_dai_etdm_register(struct mtk_base_afe *afe);

int mt6880_dai_hostless_register(struct mtk_base_afe *afe);

int mt6880_add_misc_control(struct snd_soc_component *component);

unsigned int mt6880_general_rate_transform(struct device *dev,
					   unsigned int rate);
unsigned int mt6880_rate_transform(struct device *dev,
				   unsigned int rate, int aud_blk);
int mt6880_dai_set_priv(struct mtk_base_afe *afe, int id,
			int priv_size, const void *priv_data);
#endif
