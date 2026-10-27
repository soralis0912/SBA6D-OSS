// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#include "mt6990-afe-common.h"
#include "mt6990-afe-clk.h"

static DEFINE_MUTEX(mutex_request_dram);

enum {
	CLK_AFE = 0,
	/*CLK_DAC,*/
	/*CLK_DAC_PREDIS,*/
	/*CLK_ADC,*/
	CLK_TML,
	CLK_APLL22M,
	CLK_APLL24M,
	CLK_APLL1_TUNER,
	CLK_APLL2_TUNER,
	CLK_TDM,
	CLK_I2S0,
	CLK_I2S1,
	CLK_I2S2,
	CLK_I2S4,
	CLK_I2S5,
	CLK_I2S6,
	CLK_SRC2,
	CLK_ETDM0,
	/* CLK_SCP_SYS_AUD, */
	CLK_INFRA_SYS_AUDIO,
	CLK_MTKAIF_26M,
	CLK_APMIXED_APLL1,
	CLK_APMIXED_APLL2,
	CLK_MUX_AUDIO,
	CLK_MUX_AUDIOINTBUS,
	CLK_TOP_MAINPLL_D2_D4,
	/* apll related mux */
	CLK_TOP_MUX_AUD_1,
	CLK_TOP_APLL1_CK,
	CLK_TOP_MUX_AUD_2,
	CLK_TOP_APLL2_CK,
	CLK_TOP_MUX_AUD_ENG1,
	CLK_TOP_APLL1_D8,
	CLK_TOP_MUX_AUD_ENG2,
	CLK_TOP_APLL2_D8,
	CLK_TOP_I2S0_M_SEL,
	CLK_TOP_I2S1_M_SEL,
	CLK_TOP_I2S2_M_SEL,
	CLK_TOP_I2S4_M_SEL,
	CLK_TOP_TDM_M_SEL,
	CLK_TOP_I2S5_M_SEL,
	CLK_TOP_I2S6_M_SEL,
	CLK_TOP_APLL12_DIV0,
	CLK_TOP_APLL12_DIV1,
	CLK_TOP_APLL12_DIV2,
	CLK_TOP_APLL12_DIV4,
	CLK_TOP_APLL12_DIVM,
	CLK_TOP_APLL12_DIVB,
	CLK_TOP_APLL12_DIV5,
	CLK_TOP_APLL12_DIV6,
	CLK_CLK26M,
	CLK_NUM
};

static const char *aud_clks[CLK_NUM] = {
	[CLK_AFE] = "aud_afe_clk",
	/*[CLK_DAC] = "aud_dac_clk",*/
	/*[CLK_DAC_PREDIS] = "aud_dac_predis_clk",*/
	/*[CLK_ADC] = "aud_adc_clk",*/
	[CLK_TML] = "aud_tml_clk",
	[CLK_APLL22M] = "aud_apll22m_clk",
	[CLK_APLL24M] = "aud_apll24m_clk",
	[CLK_APLL1_TUNER] = "aud_apll1_tuner_clk",
	[CLK_APLL2_TUNER] = "aud_apll2_tuner_clk",
	[CLK_TDM] = "aud_tdm_clk",
	[CLK_I2S0] = "aud_i2s0_clk",
	[CLK_I2S1] = "aud_i2s1_clk",
	[CLK_I2S2] = "aud_i2s2_clk",
	[CLK_I2S4] = "aud_i2s4_clk",
	[CLK_I2S5] = "aud_i2s5_clk",
	[CLK_I2S6] = "aud_i2s6_clk",
	[CLK_SRC2] = "aud_src2_clk",
	[CLK_ETDM0] = "aud_etdm0_clk",
	/* [CLK_SCP_SYS_AUD] = "scp_sys_audio", */
	[CLK_INFRA_SYS_AUDIO] = "aud_infra_clk",
	[CLK_MTKAIF_26M] = "mtkaif_26m_clk",
	[CLK_APMIXED_APLL1] = "aud_clk_apmixed_apll1",
	[CLK_APMIXED_APLL2] = "aud_clk_apmixed_apll2",
	[CLK_MUX_AUDIO] = "top_mux_audio",
	[CLK_MUX_AUDIOINTBUS] = "top_mux_audio_int",
	[CLK_TOP_MAINPLL_D2_D4] = "top_mainpll_d2_d4",
	[CLK_TOP_MUX_AUD_1] = "top_mux_aud_1",
	[CLK_TOP_APLL1_CK] = "top_apll1_ck",
	[CLK_TOP_MUX_AUD_2] = "top_mux_aud_2",
	[CLK_TOP_APLL2_CK] = "top_apll2_ck",
	[CLK_TOP_MUX_AUD_ENG1] = "top_mux_aud_eng1",
	[CLK_TOP_APLL1_D8] = "top_apll1_d8",
	[CLK_TOP_MUX_AUD_ENG2] = "top_mux_aud_eng2",
	[CLK_TOP_APLL2_D8] = "top_apll2_d8",
	[CLK_TOP_I2S0_M_SEL] = "top_i2s0_m_sel",
	[CLK_TOP_I2S1_M_SEL] = "top_i2s1_m_sel",
	[CLK_TOP_I2S2_M_SEL] = "top_i2s2_m_sel",
	[CLK_TOP_I2S4_M_SEL] = "top_i2s4_m_sel",
	[CLK_TOP_TDM_M_SEL] = "top_tdm_m_sel",
	[CLK_TOP_I2S5_M_SEL] = "top_i2s5_m_sel",
	[CLK_TOP_I2S6_M_SEL] = "top_i2s6_m_sel",
	[CLK_TOP_APLL12_DIV0] = "top_apll12_div0",
	[CLK_TOP_APLL12_DIV1] = "top_apll12_div1",
	[CLK_TOP_APLL12_DIV2] = "top_apll12_div2",
	[CLK_TOP_APLL12_DIV4] = "top_apll12_div4",
	[CLK_TOP_APLL12_DIVM] = "top_apll12_divm",
	[CLK_TOP_APLL12_DIVB] = "top_apll12_divb",
	[CLK_TOP_APLL12_DIV5] = "top_apll12_div5",
	[CLK_TOP_APLL12_DIV6] = "top_apll12_div6",
	[CLK_CLK26M] = "top_clk26m_clk",
};

static int mt6990_set_audio_int_bus_parent(struct mtk_base_afe *afe,
					   int clk_id)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	if (clk_id < 0 || clk_id >= CLK_NUM)
		return -EINVAL;
	ret = clk_set_parent(afe_priv->clk[CLK_MUX_AUDIOINTBUS],
			     afe_priv->clk[clk_id]);
	if (ret) {
		dev_err(afe->dev, "%s() clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[CLK_MUX_AUDIOINTBUS],
		       aud_clks[clk_id], ret);
	}

	return ret;
}

static int apll1_mux_setting(struct mtk_base_afe *afe, bool enable)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	if (enable) {
		ret = clk_prepare_enable(afe_priv->clk[CLK_TOP_MUX_AUD_1]);
		if (ret) {
			dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_1], ret);
			goto ERR_ENABLE_CLK_TOP_MUX_AUD_1;
		}
		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_1],
				     afe_priv->clk[CLK_TOP_APLL1_CK]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_1],
				aud_clks[CLK_TOP_APLL1_CK], ret);
			goto ERR_SELECT_CLK_TOP_MUX_AUD_1;
		}

		/* 180.6336 / 8 = 22.5792MHz */
		ret = clk_prepare_enable(afe_priv->clk[CLK_TOP_MUX_AUD_ENG1]);
		if (ret) {
			dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_ENG1], ret);
			goto ERR_ENABLE_CLK_TOP_MUX_AUD_ENG1;
		}
		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_ENG1],
				     afe_priv->clk[CLK_TOP_APLL1_D8]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_ENG1],
				aud_clks[CLK_TOP_APLL1_D8], ret);
			goto ERR_SELECT_CLK_TOP_MUX_AUD_ENG1;
		}
	} else {
		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_ENG1],
				     afe_priv->clk[CLK_CLK26M]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_ENG1],
				aud_clks[CLK_CLK26M], ret);
			goto EXIT;
		}
		clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_ENG1]);

		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_1],
				     afe_priv->clk[CLK_CLK26M]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_1],
				aud_clks[CLK_CLK26M], ret);
			goto EXIT;
		}
		clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_1]);
	}

	return 0;

ERR_SELECT_CLK_TOP_MUX_AUD_ENG1:
	clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_ENG1],
		       afe_priv->clk[CLK_CLK26M]);
	clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_ENG1]);
ERR_ENABLE_CLK_TOP_MUX_AUD_ENG1:
ERR_SELECT_CLK_TOP_MUX_AUD_1:
	clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_1],
		       afe_priv->clk[CLK_CLK26M]);
	clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_1]);
ERR_ENABLE_CLK_TOP_MUX_AUD_1:
EXIT:
	return ret;
}

static int apll2_mux_setting(struct mtk_base_afe *afe, bool enable)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	if (enable) {
		ret = clk_prepare_enable(afe_priv->clk[CLK_TOP_MUX_AUD_2]);
		if (ret) {
			dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_2], ret);
			goto ERR_ENABLE_CLK_TOP_MUX_AUD_2;
		}
		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_2],
				     afe_priv->clk[CLK_TOP_APLL2_CK]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_2],
				aud_clks[CLK_TOP_APLL2_CK], ret);
			goto ERR_SELECT_CLK_TOP_MUX_AUD_2;
		}

		/* 196.608 / 8 = 24.576MHz */
		ret = clk_prepare_enable(afe_priv->clk[CLK_TOP_MUX_AUD_ENG2]);
		if (ret) {
			dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_ENG2], ret);
			goto ERR_ENABLE_CLK_TOP_MUX_AUD_ENG2;
		}
		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_ENG2],
				     afe_priv->clk[CLK_TOP_APLL2_D8]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_ENG2],
				aud_clks[CLK_TOP_APLL2_D8], ret);
			goto ERR_SELECT_CLK_TOP_MUX_AUD_ENG2;
		}
	} else {
		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_ENG2],
				     afe_priv->clk[CLK_CLK26M]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_ENG2],
				aud_clks[CLK_CLK26M], ret);
			goto EXIT;
		}
		clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_ENG2]);

		ret = clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_2],
				     afe_priv->clk[CLK_CLK26M]);
		if (ret) {
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[CLK_TOP_MUX_AUD_2],
				aud_clks[CLK_CLK26M], ret);
			goto EXIT;
		}
		clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_2]);
	}

	return 0;

ERR_SELECT_CLK_TOP_MUX_AUD_ENG2:
	clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_ENG2],
		       afe_priv->clk[CLK_CLK26M]);
	clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_ENG2]);
ERR_ENABLE_CLK_TOP_MUX_AUD_ENG2:
ERR_SELECT_CLK_TOP_MUX_AUD_2:
	clk_set_parent(afe_priv->clk[CLK_TOP_MUX_AUD_2],
		       afe_priv->clk[CLK_CLK26M]);
	clk_disable_unprepare(afe_priv->clk[CLK_TOP_MUX_AUD_2]);
ERR_ENABLE_CLK_TOP_MUX_AUD_2:
EXIT:
	return ret;
}

int mt6990_afe_enable_clock(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	dev_err(afe->dev, "%s()\n", __func__);

	ret = clk_prepare_enable(afe_priv->clk[CLK_MTKAIF_26M]);
	if (ret) {
		dev_err(afe->dev, "%s(), clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_MTKAIF_26M], ret);
		goto CLK_26M_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clk[CLK_INFRA_SYS_AUDIO]);
	if (ret) {
		dev_err(afe->dev, "%s(), clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_INFRA_SYS_AUDIO], ret);
		goto CLK_INFRA_SYS_AUDIO_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clk[CLK_MUX_AUDIO]);
	if (ret) {
		dev_err(afe->dev, "%s(), clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_MUX_AUDIO], ret);
		goto CLK_MUX_AUDIO_ERR;
	}

	ret = clk_set_parent(afe_priv->clk[CLK_MUX_AUDIO],
			     afe_priv->clk[CLK_CLK26M]);
	if (ret) {
		dev_err(afe->dev, "%s(), clk_set_parent %s-%s fail %d\n",
			__func__, aud_clks[CLK_MUX_AUDIO],
			aud_clks[CLK_CLK26M], ret);
		goto CLK_MUX_AUDIO_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
	if (ret) {
		dev_err(afe->dev, "%s(), clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_MUX_AUDIOINTBUS], ret);
		goto CLK_MUX_AUDIO_INTBUS_ERR;
	}

	ret = mt6990_set_audio_int_bus_parent(afe,
					      CLK_CLK26M);
	if (ret)
		goto CLK_MUX_AUDIO_INTBUS_PARENT_ERR;

	ret = clk_prepare_enable(afe_priv->clk[CLK_AFE]);
	if (ret) {
		dev_err(afe->dev, "%s() clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_AFE], ret);
		goto CLK_AFE_ERR;
	}

	return 0;

CLK_AFE_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_AFE]);
CLK_MUX_AUDIO_INTBUS_PARENT_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
CLK_MUX_AUDIO_INTBUS_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIO]);
CLK_MUX_AUDIO_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_INFRA_SYS_AUDIO]);
CLK_INFRA_SYS_AUDIO_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_MTKAIF_26M]);
CLK_26M_ERR:
	return ret;

}

void mt6990_afe_disable_clock(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	dev_info(afe->dev, "%s()\n", __func__);

	clk_disable_unprepare(afe_priv->clk[CLK_AFE]);

	mt6990_set_audio_int_bus_parent(afe, CLK_CLK26M);
	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIO]);

	clk_disable_unprepare(afe_priv->clk[CLK_INFRA_SYS_AUDIO]);
	clk_disable_unprepare(afe_priv->clk[CLK_MTKAIF_26M]);
}

int mt6990_afe_suspend_clock(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	/* set audio int bus to 26M */
	ret = clk_prepare_enable(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_MUX_AUDIOINTBUS], ret);
		goto CLK_MUX_AUDIO_INTBUS_ERR;
	}
	ret = mt6990_set_audio_int_bus_parent(afe, CLK_CLK26M);

	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);

	return 0;

CLK_MUX_AUDIO_INTBUS_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
	return ret;
}

int mt6990_afe_resume_clock(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	/* set audio int bus to 26M */
	ret = clk_prepare_enable(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_MUX_AUDIOINTBUS], ret);
		goto CLK_MUX_AUDIO_INTBUS_ERR;
	}
	ret = mt6990_set_audio_int_bus_parent(afe,
					      CLK_CLK26M);

	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);

	return 0;

CLK_MUX_AUDIO_INTBUS_ERR:
	clk_disable_unprepare(afe_priv->clk[CLK_MUX_AUDIOINTBUS]);
	return ret;
}

int mt6990_afe_dram_request(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	/* struct arm_smccc_res res;*/

	dev_info(dev, "%s(), dram_resource_counter %d\n",
		 __func__, afe_priv->dram_resource_counter);

	mutex_lock(&mutex_request_dram);
	afe_priv->dram_resource_counter++;
#if 0 
	/* use arm_smccc_smc to notify SPM */
	arm_smccc_smc(MTK_SIP_AUDIO_CONTROL, MTK_AUDIO_SMC_OP_DRAM_REQUEST,
		      0, 0, 0, 0, 0, 0, &res);
#endif
	mutex_unlock(&mutex_request_dram);
	return 0;
}

int mt6990_afe_dram_release(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
/*	struct arm_smccc_res res;*/

	dev_info(dev, "%s(), dram_resource_counter %d\n",
		 __func__, afe_priv->dram_resource_counter);

	mutex_lock(&mutex_request_dram);
	afe_priv->dram_resource_counter--;
#if 0 
	/* use arm_smccc_smc to notify SPM */
	arm_smccc_smc(MTK_SIP_AUDIO_CONTROL, MTK_AUDIO_SMC_OP_DRAM_RELEASE,
		      0, 0, 0, 0, 0, 0, &res);
#endif
	if (afe_priv->dram_resource_counter < 0) {
		dev_warn(dev, "%s(), dram_resource_counter %d\n",
			 __func__, afe_priv->dram_resource_counter);
		afe_priv->dram_resource_counter = 0;
	}
	mutex_unlock(&mutex_request_dram);
	return 0;
}


int mt6990_apll1_enable(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	/* setting for APLL */
	apll1_mux_setting(afe, true);

	ret = clk_prepare_enable(afe_priv->clk[CLK_APLL22M]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_APLL22M], ret);
		goto ERR_CLK_APLL22M;
	}

	ret = clk_prepare_enable(afe_priv->clk[CLK_APLL1_TUNER]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_APLL1_TUNER], ret);
		goto ERR_CLK_APLL1_TUNER;
	}

	regmap_update_bits(afe->regmap, AFE_APLL1_TUNER_CFG,
			   0x0000FFF7, 0x00000432);
	regmap_update_bits(afe->regmap, AFE_APLL1_TUNER_CFG,
			   FREQ_TUNER_EN_MASK_SFT,
			   0x1 << FREQ_TUNER_EN_SFT);

	regmap_update_bits(afe->regmap, AFE_HD_ENGEN_ENABLE,
			   AFE_22M_ON_MASK_SFT,
			   0x1 << AFE_22M_ON_SFT);

	return 0;

ERR_CLK_APLL1_TUNER:
	clk_disable_unprepare(afe_priv->clk[CLK_APLL22M]);
ERR_CLK_APLL22M:
	return ret;
}

void mt6990_apll1_disable(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	regmap_update_bits(afe->regmap, AFE_HD_ENGEN_ENABLE,
			   AFE_22M_ON_MASK_SFT,
			   0x0 << AFE_22M_ON_SFT);

	regmap_update_bits(afe->regmap, AFE_APLL1_TUNER_CFG,
			   FREQ_TUNER_EN_MASK_SFT,
			   0x0 << FREQ_TUNER_EN_SFT);

	clk_disable_unprepare(afe_priv->clk[CLK_APLL1_TUNER]);
	clk_disable_unprepare(afe_priv->clk[CLK_APLL22M]);

	apll1_mux_setting(afe, false);
}

int mt6990_apll2_enable(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int ret;

	/* setting for APLL */
	apll2_mux_setting(afe, true);

	ret = clk_prepare_enable(afe_priv->clk[CLK_APLL24M]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_APLL24M], ret);
		goto ERR_CLK_APLL24M;
	}

	ret = clk_prepare_enable(afe_priv->clk[CLK_APLL2_TUNER]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[CLK_APLL2_TUNER], ret);
		goto ERR_CLK_APLL2_TUNER;
	}

	regmap_update_bits(afe->regmap, AFE_APLL2_TUNER_CFG,
			   0x0000FFF7, 0x00000434);
	regmap_update_bits(afe->regmap, AFE_APLL2_TUNER_CFG, 0x1, 0x1);

	regmap_update_bits(afe->regmap, AFE_HD_ENGEN_ENABLE,
			   AFE_24M_ON_MASK_SFT,
			   0x1 << AFE_24M_ON_SFT);

	return 0;

ERR_CLK_APLL2_TUNER:
	clk_disable_unprepare(afe_priv->clk[CLK_APLL24M]);
ERR_CLK_APLL24M:
	return ret;
}

void mt6990_apll2_disable(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;

	regmap_update_bits(afe->regmap, AFE_HD_ENGEN_ENABLE,
			   AFE_24M_ON_MASK_SFT,
			   0x0 << AFE_24M_ON_SFT);

	regmap_update_bits(afe->regmap, AFE_APLL2_TUNER_CFG, 0x1, 0x0);

	clk_disable_unprepare(afe_priv->clk[CLK_APLL2_TUNER]);
	clk_disable_unprepare(afe_priv->clk[CLK_APLL24M]);

	apll2_mux_setting(afe, false);
}

int mt6990_get_apll_rate(struct mtk_base_afe *afe, int apll)
{
	return (apll == MT6990_APLL1) ? 180633600 : 196608000;
}

int mt6990_get_apll_by_rate(struct mtk_base_afe *afe, int rate)
{
	return ((rate % 8000) == 0) ? MT6990_APLL2 : MT6990_APLL1;
}

int mt6990_get_apll_by_name(struct mtk_base_afe *afe, const char *name)
{
	if (strcmp(name, APLL1_W_NAME) == 0)
		return MT6990_APLL1;
	else
		return MT6990_APLL2;
}

/* mck */
struct mt6990_mck_div {
	int m_sel_id;
	int div_clk_id;
};

static const struct mt6990_mck_div mck_div[MT6990_MCK_NUM] = {
	[MT6990_I2S0_MCK] = {
		.m_sel_id = CLK_TOP_I2S0_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIV0,
	},
	[MT6990_I2S1_MCK] = {
		.m_sel_id = CLK_TOP_I2S1_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIV1,
	},
	[MT6990_I2S2_MCK] = {
		.m_sel_id = CLK_TOP_I2S2_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIV2,
	},
	[MT6990_I2S4_MCK] = {
		.m_sel_id = CLK_TOP_I2S4_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIV4,
	},
	[MT6990_TDM_MCK] = {
		.m_sel_id = CLK_TOP_TDM_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIVM,
	},
	[MT6990_TDM_BCK] = {
		.m_sel_id = -1,
		.div_clk_id = CLK_TOP_APLL12_DIVB,
	},
	[MT6990_I2S5_MCK] = {
		.m_sel_id = CLK_TOP_I2S5_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIV5,
	},
	[MT6990_I2S6_MCK] = {
		.m_sel_id = CLK_TOP_I2S6_M_SEL,
		.div_clk_id = CLK_TOP_APLL12_DIV6,
	},
};

int mt6990_mck_enable(struct mtk_base_afe *afe, int mck_id, int rate)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int apll = mt6990_get_apll_by_rate(afe, rate);
	int apll_clk_id = apll == MT6990_APLL1 ?
			  CLK_TOP_MUX_AUD_1 : CLK_TOP_MUX_AUD_2;
	int m_sel_id;
	int div_clk_id;
	int ret;

	if (mck_id < 0 || mck_id >= MT6990_MCK_NUM)
		return -EINVAL;
	m_sel_id = mck_div[mck_id].m_sel_id;
	div_clk_id = mck_div[mck_id].div_clk_id;

	/* i2s5 mck not support */
	if (mck_id == MT6990_I2S5_MCK)
		return 0;

	/* select apll */
	if (m_sel_id >= 0 && m_sel_id < CLK_NUM) {
		ret = clk_prepare_enable(afe_priv->clk[m_sel_id]);
		if (ret) {
			dev_err(afe->dev, "%s(), clk_prepare_enable %s fail %d\n",
				__func__, aud_clks[m_sel_id], ret);
			goto ERR_ENABLE_MCLK;
		}
		ret = clk_set_parent(afe_priv->clk[m_sel_id],
				     afe_priv->clk[apll_clk_id]);
		if (ret) {
			dev_err(afe->dev, "%s(), clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[m_sel_id],
				aud_clks[apll_clk_id], ret);
			goto ERR_SELECT_MCLK;
		}
	}

	/* enable div, set rate */
	if (div_clk_id >= 0 && div_clk_id < CLK_NUM) {
		ret = clk_prepare_enable(afe_priv->clk[div_clk_id]);
		if (ret) {
			dev_err(afe->dev, "%s(), clk_prepare_enable %s fail %d\n",
				__func__, aud_clks[div_clk_id], ret);
			goto ERR_ENABLE_MCLK_DIV;
		}
		ret = clk_set_rate(afe_priv->clk[div_clk_id], rate);
		if (ret) {
			dev_err(afe->dev, "%s(), clk_set_rate %s, rate %d, fail %d\n",
				__func__, aud_clks[div_clk_id],
				rate, ret);
			goto ERR_SET_MCLK_RATE;
			return ret;
		}
	}

	return 0;

ERR_SET_MCLK_RATE:
	if (div_clk_id >= 0 && div_clk_id < CLK_NUM)
		clk_disable_unprepare(afe_priv->clk[div_clk_id]);
ERR_ENABLE_MCLK_DIV:
ERR_SELECT_MCLK:
	if (m_sel_id >= 0 && m_sel_id < CLK_NUM)
		clk_disable_unprepare(afe_priv->clk[m_sel_id]);
ERR_ENABLE_MCLK:
	return ret;
}

void mt6990_mck_disable(struct mtk_base_afe *afe, int mck_id)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int m_sel_id;
	int div_clk_id;

	if (mck_id < 0 || mck_id >= MT6990_MCK_NUM)
		return;
	m_sel_id = mck_div[mck_id].m_sel_id;
	div_clk_id = mck_div[mck_id].div_clk_id;
	/* i2s5 mck not support */
	if (mck_id == MT6990_I2S5_MCK)
		return;

	clk_disable_unprepare(afe_priv->clk[div_clk_id]);
	if (m_sel_id >= 0)
		clk_disable_unprepare(afe_priv->clk[m_sel_id]);
}

int mt6990_init_clock(struct mtk_base_afe *afe)
{
	struct mt6990_afe_private *afe_priv = afe->platform_priv;
	int i = 0;
	int ret = 0;
	int value = 0;

	afe_priv->clk = devm_kcalloc(afe->dev, CLK_NUM, sizeof(*afe_priv->clk),
				     GFP_KERNEL);
	if (!afe_priv->clk)
		return -ENOMEM;

	for (i = 0; i < CLK_NUM; i++) {
		afe_priv->clk[i] = devm_clk_get(afe->dev, aud_clks[i]);
		if (IS_ERR(afe_priv->clk[i])) {
			dev_warn(afe->dev, "%s devm_clk_get %s fail, ret %ld\n",
				 __func__,
				 aud_clks[i], PTR_ERR(afe_priv->clk[i]));
			/* return PTR_ERR(clks[i]); */
			afe_priv->clk[i] = NULL;
		}
	}

	afe_priv->apmixed = syscon_regmap_lookup_by_phandle(afe->dev->of_node, "apmixedsys");
 	if (IS_ERR(afe_priv->apmixed)) {
 		dev_err(afe->dev, "%s() Cannot find apmixedsys: %ld\n",
 			__func__, PTR_ERR(afe_priv->apmixed));
 	}

 	ret = clk_set_rate(afe_priv->clk[CLK_APMIXED_APLL1], 180633601);
 	if (ret) {
 		dev_err(afe->dev, "%s(), clk_set_rate %s, rate 180633601, fail %d\n",
 			__func__, aud_clks[CLK_APMIXED_APLL1], ret);
 		return ret;
 	}
 	ret = clk_set_rate(afe_priv->clk[CLK_APMIXED_APLL1], 180633600);
 	if (ret) {
 		dev_err(afe->dev, "%s(), clk_set_rate %s, rate 180633600, fail %d\n",
 			__func__, aud_clks[CLK_APMIXED_APLL1], ret);
		return ret;
 	}
#if 0
 	ret = clk_set_rate(afe_priv->clk[CLK_APMIXED_APLL2], 196608001);
 	if (ret) {
 		dev_err(afe->dev, "%s(), clk_set_rate %s, rate 196608001, fail %d\n",
 			__func__, aud_clks[CLK_APMIXED_APLL2], ret);
 		return ret;
 	}
 	ret = clk_set_rate(afe_priv->clk[CLK_APMIXED_APLL2], 196608000);
 	if (ret) {
 		dev_err(afe->dev, "%s(), clk_set_rate %s, rate 196608000, fail %d\n",
 			__func__, aud_clks[CLK_APMIXED_APLL2], ret);
 		return ret;
 	}
#endif

 	if (afe_priv->apmixed != NULL) {
 		regmap_read(afe_priv->apmixed, APLL1_TUNER_CON0, &value);
 		dev_info(afe->dev, "%s() APLL1_TUNER_CON0 = 0x%x\n", __func__, value);

 		regmap_read(afe_priv->apmixed, APLL2_TUNER_CON0, &value);
 		dev_info(afe->dev, "%s() APLL2_TUNER_CON0 = 0x%x\n", __func__, value);
 	}

	afe_priv->topckgen = syscon_regmap_lookup_by_phandle(afe->dev->of_node,
								 "topckgen");
	if (IS_ERR(afe_priv->topckgen)) {
		dev_err(afe->dev, "Cannot find topckgen controller: %ld\n",
			PTR_ERR(afe_priv->topckgen));
		return PTR_ERR(afe_priv->topckgen);
	}

	afe_priv->infracfg = syscon_regmap_lookup_by_phandle(
				afe->dev->of_node,
				"infracfg");
	if (IS_ERR(afe_priv->infracfg)) {
		dev_err(afe->dev, "%s() Cannot find infracfg: %ld\n",
				__func__, PTR_ERR(afe_priv->infracfg));
		return PTR_ERR(afe_priv->infracfg);
	}

	return 0;
}
