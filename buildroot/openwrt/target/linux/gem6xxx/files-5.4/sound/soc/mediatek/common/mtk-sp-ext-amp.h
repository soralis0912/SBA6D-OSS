/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2018 MediaTek Inc.
 */

#ifndef _MTK_SP_EXT_AMP_H
#define _MTK_SP_EXT_AMP_H

struct mtk_ext_spi_ctrl {
	int (*spi_probe)(struct spi_device *spi,struct spi_driver *spi_drv);
	int (*spi_remove)(struct spi_device *spi);
	const char *stream_name;
	const char *codec_dai_name;
	const char *codec_name;
};

#define MTK_EXT_NOT_SMARTPA_STR "MTK_EXT_NOT_SMARTPA"
#define MTK_EXT_RICHTEK_RT5509_STR "MTK_EXT_RICHTEK_RT5509"
#define MTK_EXT_MEDIATEK_MT6660_STR "MTK_EXT_MEDIATEK_MT6660"
#define MTK_EXT_NXP_TFA98XX_STR "MTK_EXT_NXP_TFAXXXX"
#if IS_ENABLED(CONFIG_SND_SOC_SMA1301)
#define MTK_EXT_SILICON_SM1301_STR "MTK_EXT_SILICON_SM1301"
#endif

#define MTK_EXT_I2S_0_STR "MTK_EXT_I2S_0"
#define MTK_EXT_I2S_1_STR "MTK_EXT_I2S_1"
#define MTK_EXT_I2S_2_STR "MTK_EXT_I2S_2"
#define MTK_EXT_I2S_3_STR "MTK_EXT_I2S_3"
#define MTK_EXT_I2S_5_STR "MTK_EXT_I2S_5"
#define MTK_EXT_I2S_6_STR "MTK_EXT_I2S_6"
#define MTK_EXT_I2S_7_STR "MTK_EXT_I2S_7"
#define MTK_EXT_I2S_8_STR "MTK_EXT_I2S_8"
#define MTK_EXT_I2S_9_STR "MTK_EXT_I2S_9"
#define MTK_EXT_TINYCONN_I2S_0_STR "MTK_EXT_TINYCONN_I2S_0"


enum mtk_ext_type {
	MTK_EXT_NOT_SMARTPA = 0,
	MTK_EXT_PROSLIC,
	MTK_EXT_PROSLIC2,
	MTK_EXT_TYPE_NUM
};

enum mtk_ext_i2s_type {
	MTK_EXT_I2S_TYPE_INVALID = -1,
	MTK_EXT_I2S_0,
	MTK_EXT_I2S_1,
	MTK_EXT_I2S_2,
	MTK_EXT_I2S_3,
	MTK_EXT_I2S_5,
	MTK_EXT_I2S_6,
	MTK_EXT_I2S_7,
	MTK_EXT_I2S_8,
	MTK_EXT_I2S_9,
	MTK_EXT_TINYCONN_I2S_0,
	MTK_EXT_TINYCONN_I2S_1,
	MTK_EXT_TINYCONN_I2S_2,
	MTK_EXT_TINYCONN_I2S_3,
	MTK_EXT_TINYCONN_I2S_5,
	MTK_EXT_TINYCONN_I2S_6,
	MTK_EXT_TINYCONN_I2S_7,
	MTK_EXT_TINYCONN_I2S_8,
	MTK_EXT_TINYCONN_I2S_9,
	MTK_EXT_I2S_TYPE_NUM
};

int mtk_ext_get_type(void);
int mtk_ext_update_dai_link(struct snd_soc_card *card,
			    struct platform_device *pdev,
			    const struct snd_soc_ops *i2s_ops);

#endif

