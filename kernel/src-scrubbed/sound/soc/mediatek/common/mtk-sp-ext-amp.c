// SPDX-License-Identifier: GPL-2.0
//
// Copyright (C) 2018 MediaTek Inc.

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/spi/spi.h>

/* alsa sound header */
#include <sound/soc.h>
#include <sound/pcm_params.h>

#include "mtk-sp-ext-amp.h"

#if IS_ENABLED(CONFIG_SND_SOC_PROSLIC)
#include "../../codecs/proslic/si321xx.h"
#endif

static unsigned int mtk_ext_type;
static struct mtk_ext_spi_ctrl mtk_ext_list[MTK_EXT_TYPE_NUM] = {
	[MTK_EXT_NOT_SMARTPA] = {
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
#if IS_ENABLED(CONFIG_SND_SOC_PROSLIC)
	[MTK_EXT_PROSLIC] = {
		.spi_probe = si321xx_spi_probe,
		.spi_remove = si321xx_spi_remove,
	},
#endif
};
static int mtk_ext_spi_probe(struct spi_device *spi);
static int mtk_ext_spi_remove(struct spi_device *spi)
{
	dev_info(&spi->dev, "%s()\n", __func__);

	if (mtk_ext_list[mtk_ext_type].spi_remove)
		mtk_ext_list[mtk_ext_type].spi_remove(spi);

	return 0;
}

int mtk_ext_get_type(void)
{
	return mtk_ext_type;
}
EXPORT_SYMBOL(mtk_ext_get_type);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id mtk_ext_match_table[] = {
	{.compatible = "silabs,proslic_spi",},
	{},
};
MODULE_DEVICE_TABLE(of, mtk_ext_match_table);
#endif /* #if IS_ENABLED(CONFIG_OF) */

static struct spi_driver mtk_ext_spi_driver = {
	.driver = {
		.name = "proslic_spi",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_ext_match_table),
	},
	.probe = mtk_ext_spi_probe,
	.remove = mtk_ext_spi_remove,
};

module_spi_driver(mtk_ext_spi_driver);
static int mtk_ext_spi_probe(struct spi_device *spi)
{
	unsigned int i;
	int ret = 0;

	dev_err(&spi->dev, "%s()\n", __func__);

	mtk_ext_type = MTK_EXT_NOT_SMARTPA;
	for (i = 0; i < MTK_EXT_TYPE_NUM; i++) {
		if (!mtk_ext_list[i].spi_probe)
			continue;

		ret = mtk_ext_list[i].spi_probe(spi, &mtk_ext_spi_driver);
		if (ret)
			continue;

		mtk_ext_type = i;
		break;
	}

	return ret;
}

MODULE_DESCRIPTION("Mediatek speaker amp register driver");
MODULE_AUTHOR("Shane Chien <shane.chien@mediatek.com>");
MODULE_LICENSE("GPL v2");
