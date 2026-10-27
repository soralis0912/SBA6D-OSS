// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 * Author: Sakya Chang <jeff_chang@richtek.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <dt-bindings/iio/mediatek,mt6361_adc.h>
#include <linux/iio/consumer.h>

#define MT6361_REG_HK_TOP_CLK_CON0	(0x0A00)
#define MT6361_REG_HK_TOP_WKEY_L	(0x0A0C)
#define MT6361_REG_AUXADC_ADC2_L	(0x0A40)
#define MT6361_REG_AUXADC_ADC2_H	(0x0A41)
#define MT6361_REG_AUXADC_ADC4_L	(0x0A42)
#define MT6361_REG_AUXADC_ADC4_H	(0x0A43)
#define MT6361_REG_AUXADC_ADC12_L	(0x0A44)
#define MT6361_REG_AUXADC_ADC12_H	(0x0A45)
#define MT6361_REG_AUXADC_ADC14_L	(0x0A46)
#define MT6361_REG_AUXADC_ADC14_H	(0x0A47)
#define MT6361_REG_AUXADC_ADC38_L	(0x0A4C)
#define MT6361_REG_AUXADC_ADC38_H	(0x0A4D)
#define MT6361_ADC_RDY_MASK		BIT(15)
#define MT6361_REG_EXT_THR_CON0		(0x0AB5)
#define MT6361_SRC_SEL_MASK		(GENMASK(7, 4) | BIT(0))
#define MT6361_SRC_SEL_SFT		(4)
#define MT6361_REG_AUXADC_RQST0		(0x0A80)
#define MT6361_REG_AUXADC_RQST1		(0x0A81)
#define MT6361_REG_AUXADC_RQST3		(0x0A83)
#define MT6361_REG_AUXADC_CON0		(0x0A90)
#define MT6361_REG_AUXADC_CON41		(0x0AAE)

#define VOLT_FULL	(1820000)


#define MT6361_RQST_TRIGGER_MASK	BIT(4)
#define ADC_CONV_TIME_US	(10) /* spec is 9 us */
#define ADC_POLL_TIME_US	(100) /* TODO */
#define ADC_POLL_TIMEOUT_US	(1000) /* TODO */

struct mt6361_adc_data {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock;
	struct lock_class_key info_exist_key;
	struct iio_channel *adc_iio;
};

static int mt6361_adc_read_channel(struct mt6361_adc_data *adata, int channel,
				   int *val)
{
	__be16 be_val = 0;
	int ret = 0;
	unsigned int regval = 0;
	unsigned int rq_reg = 0, rq_mask = 0;
	unsigned int rdy_reg = 0, rdy_mask = 0;
	unsigned int result_reg = 0;
	int offset = 0;

	mutex_lock(&adata->lock);
	pm_stay_awake(adata->dev);

	switch (channel) {
	case AUXADC_VDET:
		rq_reg = MT6361_REG_AUXADC_RQST0;
		rq_mask = BIT(2);
		rdy_reg = MT6361_REG_AUXADC_ADC2_H;
		rdy_mask = BIT(7);
		result_reg = MT6361_REG_AUXADC_ADC2_L;
		break;
	case AUXADC_VSYSNS:
		rq_reg = MT6361_REG_AUXADC_RQST1;
		rq_mask = BIT(6);
		rdy_reg = MT6361_REG_AUXADC_ADC14_H;
		rdy_mask = BIT(7);
		result_reg = MT6361_REG_AUXADC_ADC14_L;
		break;
	case AUXADC_CHIP_TEMP:
		rq_reg = MT6361_REG_AUXADC_RQST0;
		rq_mask = BIT(4);
		rdy_reg = MT6361_REG_AUXADC_ADC4_H;
		rdy_mask = BIT(7);
		result_reg = MT6361_REG_AUXADC_ADC4_L;
		break;
	case AUXADC_VS1_TEMP...AUXADC_MODEM_TEMP:
		offset = channel - AUXADC_VS1_TEMP;
		rq_reg = MT6361_REG_AUXADC_RQST3;
		rq_mask = BIT(offset);
		rdy_reg = MT6361_REG_AUXADC_ADC38_H + (offset * 2);
		rdy_mask = BIT(7);
		result_reg = MT6361_REG_AUXADC_ADC38_L + (offset * 2);
		break;
	case AUXADC_VIN1...AUXADC_VIN9:
		offset = channel - AUXADC_VIN1 + 1;
		/* Select the ADC Channel */
		ret = regmap_update_bits(adata->regmap, MT6361_REG_EXT_THR_CON0,
			MT6361_SRC_SEL_MASK,
			(offset << MT6361_SRC_SEL_SFT) | BIT(0));
		if (ret)
			goto out_adc_lock;
		usleep_range(ADC_CONV_TIME_US, ADC_CONV_TIME_US*2);
		rq_reg = MT6361_REG_AUXADC_RQST1;
		rq_mask = BIT(4);
		rdy_reg = MT6361_REG_AUXADC_ADC12_H;
		rdy_mask = BIT(7);
		result_reg = MT6361_REG_AUXADC_ADC12_L;
		break;
	}

	dev_info(adata->dev, "rq_reg = %0x, rq_mask = %x, rdy_reg = %x, rdy_mask = %x, result = %x\n",
		 rq_reg, rq_mask, rdy_reg, rdy_mask, result_reg);

	/* enable adc */
	ret = regmap_update_bits(adata->regmap, rq_reg, rq_mask, rq_mask);
	if (ret) {
		dev_err(adata->dev, "request adc failed, ret = %d\n", ret);
		goto out_adc_lock;
	}

	usleep_range(2800, 3000);

	/* polling rdy bit*/
	ret = regmap_read_poll_timeout(adata->regmap, rdy_reg, regval,
				       (regval & rdy_mask), ADC_POLL_TIME_US,
				       ADC_POLL_TIMEOUT_US);
	if (ret == -ETIMEDOUT) {
		dev_err(adata->dev, "read channel%d TimeOut\n", channel);
		goto out_adc_lock;
	}

	ret = regmap_bulk_read(adata->regmap, result_reg, &be_val, sizeof(be_val));
	if (ret) {
		dev_err(adata->dev, "get adc result failed, ret = %d\n", ret);
		goto out_adc_lock;
	}
	*val = be_val & ~(0x8000);
	dev_info(adata->dev, "val = %d, 0x%x\n", *val, *val);
	ret = IIO_VAL_INT;
out_adc_lock:
	pm_relax(adata->dev);
	mutex_unlock(&adata->lock);
	return ret;
}


static int mt6361_adc_read_scale(struct mt6361_adc_data *adata, int chan, int *val1)
{
	switch (chan) {
	case AUXADC_VDET:
		*val1 = (VOLT_FULL >> 12);
		return IIO_VAL_INT;
	case AUXADC_VSYSNS:
		*val1 = (VOLT_FULL >> 15) * 3;
		return IIO_VAL_INT;
	case AUXADC_CHIP_TEMP:
	case AUXADC_VS1_TEMP...AUXADC_MODEM_TEMP:
		*val1 = (VOLT_FULL >> 12);
		return IIO_VAL_INT;
	case AUXADC_VIN1...AUXADC_VIN9:
		*val1 = (VOLT_FULL >> 15);
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static int mt6361_adc_processed(struct mt6361_adc_data *adata, int chan, int *val)
{
	int ret = 0;
	int val1 = 0, val2 = 0;
	int raw = 0;

	ret = mt6361_adc_read_channel(adata, chan, &val2);
	if (ret < 0)
		return ret;
	raw = val2;
	ret = mt6361_adc_read_scale(adata, chan, &val1);
	if (ret < 0)
		return ret;
	*val = (val1 * raw) / 1000;
	return IIO_VAL_INT;
}

static int mt6361_adc_read_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int *val,
			       int *val2, long mask)
{
	struct mt6361_adc_data *adata = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		return mt6361_adc_read_channel(adata, chan->channel, val);
	case IIO_CHAN_INFO_SCALE:
		return mt6361_adc_read_scale(adata, chan->channel, val);
	case IIO_CHAN_INFO_PROCESSED:
		return mt6361_adc_processed(adata, chan->channel, val);
	}
	return -EINVAL;
}

static const struct iio_info mt6361_iio_info = {
	.read_raw = mt6361_adc_read_raw,
};

#define MT6361_ADC_CHAN(_idx, _type) {				\
	.type = _type,						\
	.channel = AUXADC_##_idx,				\
	.scan_index = AUXADC_##_idx,				\
	.datasheet_name = __stringify(_idx),			\
	.extend_name = __stringify(_idx),			\
	.scan_type =  {						\
		.sign = 'u',					\
		.realbits = 16,					\
		.storagebits = 16,				\
		.endianness = IIO_CPU,				\
	},							\
	.indexed = 1,						\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |		\
				BIT(IIO_CHAN_INFO_SCALE) |	\
				BIT(IIO_CHAN_INFO_PROCESSED),	\
}

static const struct iio_chan_spec mt6361_adc_chls[] = {
	MT6361_ADC_CHAN(VDET, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VSYSNS, IIO_VOLTAGE),
	MT6361_ADC_CHAN(CHIP_TEMP, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VS1_TEMP, IIO_VOLTAGE),
	MT6361_ADC_CHAN(DIGRF_TEMP, IIO_VOLTAGE),
	MT6361_ADC_CHAN(MODEM_TEMP, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN1, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN2, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN3, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN4, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN5, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN6, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN7, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN8, IIO_VOLTAGE),
	MT6361_ADC_CHAN(VIN9, IIO_VOLTAGE),
	IIO_CHAN_SOFT_TIMESTAMP(AUXADC_MAX_CHANNEL),
};

static int mt6361_adc_reg_init(struct mt6361_adc_data *adata)
{
	int ret = 0;
	unsigned int hm = 0x6363;

	ret = regmap_raw_write(adata->regmap, MT6361_REG_HK_TOP_WKEY_L, &hm, 2);
	ret = regmap_write(adata->regmap, MT6361_REG_HK_TOP_CLK_CON0, 0x15);
	ret |= regmap_update_bits(adata->regmap, MT6361_REG_AUXADC_CON0, BIT(7), 0);
	ret |= regmap_update_bits(adata->regmap, MT6361_REG_AUXADC_CON41, BIT(0), BIT(0));
	hm = 0;
	ret = regmap_raw_write(adata->regmap, MT6361_REG_HK_TOP_WKEY_L, &hm, 2);

	return ret;
}

static int mt6361_adc_reset(struct mt6361_adc_data *adata)
{
	int ret = 0;

	ret = mt6361_adc_reg_init(adata);

	return ret;
}

static int mt6361_adc_probe(struct platform_device *pdev)
{
	struct mt6361_adc_data *ddata;
	struct regmap *regmap;
	struct iio_dev *indio_dev;
	int ret = 0;

	regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!regmap) {
		dev_err(&pdev->dev, "failed to get regmap\n");
		return -ENODEV;
	}

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*ddata));
	if (!indio_dev)
		return -ENOMEM;

	ddata = iio_priv(indio_dev);
	ddata->dev = &pdev->dev;
	ddata->regmap = regmap;

	mutex_init(&ddata->lock);
	/* enable as a wakeup device */
	device_init_wakeup(&pdev->dev, true);
	platform_set_drvdata(pdev, ddata);

	ret = mt6361_adc_reset(ddata);
	if (ret) {
		dev_err(&pdev->dev, "failed to reset\n");
		return ret;
	}


	/* config iio device */
	lockdep_register_key(&ddata->info_exist_key);
	lockdep_set_class(&indio_dev->info_exist_lock, &ddata->info_exist_key);
	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->info = &mt6361_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = mt6361_adc_chls;
	indio_dev->num_channels = ARRAY_SIZE(mt6361_adc_chls);

	ret = devm_iio_device_register(ddata->dev, indio_dev);
	if (ret)
		return ret;
	return 0;
}

static const struct of_device_id __maybe_unused mt6361_adc_of_match[] = {
	{	.compatible = "mediatek,mt6361-adc", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt6361_adc_of_match);

static struct platform_driver mt6361_adc_driver = {
	.probe = mt6361_adc_probe,
	.driver = {
		.name = "mt6361-adc",
		.of_match_table = mt6361_adc_of_match,
	},
};

module_platform_driver(mt6361_adc_driver);
MODULE_AUTHOR("Sakya Chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("MT6361 ADC Driver");
MODULE_LICENSE("GPL v2");
