// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>


#define CHECK_CHIP_ID_TIME_MAX          0x05
#define C_I2C_FIFO_SIZE                 0x08
#define PRESSURE_SENSOR_NAME            "bmp380"
#define BOSCH_BMP380_ID                 0x50

#define BOSCH_BMP380_REG_RESET          0x7e
#define BOSCH_BMP380_REG_DIG_T1         0x31
#define BOSCH_BMP380_REG_ID             0x00
#define BOSCH_BMP380_REG_CTRL_ODR       0x1d  //Control the Output Data Rate
#define BOSCH_BMP380_REG_CTRL_OSR       0x1c  //Control the OverSampling
#define BOSCH_BMP380_REG_CTRL_PWR       0x1b
#define BOSCH_BMP380_REG_CONFIG         0x1f  //iir filter coefficents
#define BOSCH_BMP380_REG_PRESS_LSB      0x04
#define BOSCH_BMP380_REG_FIFO_WTM_1     0x16
#define BOSCH_BMP380_REG_FIFO_WTM_0     0x15
#define BOSCH_BMP380_REG_FIFO_CONFIG_1  0x17
#define BOSCH_BMP380_REG_FIFO_CONFIG_2  0x18

#define BOSCH_BMP380_GAIN_VALUE         100

struct BMP380CompParams {
	uint16_t par_t1;
	uint16_t par_t2;
	int8_t par_t3;

	int16_t par_p1;
	int16_t par_p2;
	int8_t par_p3;
	int8_t par_p4;
	u16 par_p5;
	u16 par_p6;
	int8_t par_p7;
	int8_t par_p8;
	int16_t par_p9;
	int8_t par_p10;
	int8_t par_p11;
	s64 t_lin;
} __attribute__((packed));

struct bmp380_device_priv {
	uint32_t i2c_num;
	uint32_t i2c_addr;
	struct i2c_client *client;
	struct BMP380CompParams comp;
	atomic_t raw_enable;
};

static const struct iio_chan_spec bmp380_channels[] = {
	{
		.type = IIO_PRESSURE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},
};


/* I2C operation functions */
static int bmp_i2c_read_block(struct i2c_client *client,
				uint8_t addr, uint8_t *data, uint8_t len)
{
	int err = 0;
	uint8_t beg = addr;
	struct i2c_msg msgs[2] = {
		{/*.addr = client->addr,*/
		.flags = 0,
		.len = 1,
		.buf = &beg},
		{
			/*.addr = client->addr*/
			.flags = I2C_M_RD,
			.len = len,
			.buf = data,
		} };
	if (!client)
		return -EINVAL;
	msgs[0].addr = client->addr;
	msgs[1].addr = client->addr;

	err = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (err != 2) {
		pr_err_ratelimited("bmp380 i2c_trans err:%x %x (%d %p %d) %d\n",
				msgs[0].addr, client->addr, addr, data, len,
				err);
		err = -EIO;
	} else {
		err = 0; /*no error*/
	}
	return err;
}

static int bmp_i2c_write_block(struct i2c_client *client,
				uint8_t addr, uint8_t *data, uint8_t len)
{
	/* because address also occupies one byte,
	 * the maximum length for write is 7 bytes
	 */
	int err = 0, idx = 0, num = 0;
	char buf[32];

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		pr_err_ratelimited("bmp380 len %d fi %d\n", len,
				   C_I2C_FIFO_SIZE);
		return -EINVAL;
	}
	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		pr_err_ratelimited("bmp380 send command error!!\n");
		return -EFAULT;
	}

	return err;
}

static int bmp380_soft_reset(struct i2c_client *client)
{
	uint8_t data = 0xb6;
	return bmp_i2c_write_block(client, BOSCH_BMP380_REG_RESET, &data, 1);
}

static int bmp380_check_chip_id(struct i2c_client *client)
{
	int err = -1;
	uint8_t chip_id = 0;
	uint8_t read_count = 0;

	while (read_count++ < CHECK_CHIP_ID_TIME_MAX) {
		bmp_i2c_read_block(client, BOSCH_BMP380_REG_ID,
				   &chip_id, 1);

		if (chip_id != BOSCH_BMP380_ID) {
			mdelay(1);
			pr_err("%s fail(0x%2x).\n",
				__func__, chip_id);
		} else {
			err = 0;
			pr_info("%s success(0x%2x).\n",
				__func__, chip_id);
			break;
		}
	}
	return err;
}

static int bmp380_init_device(struct i2c_client *client, struct bmp380_device_priv *priv)
{
	int err = -1;
	uint8_t tx_buf[2] = {0};
	uint8_t rx_buf[8] = {0};

	err = bmp380_soft_reset(client);
	if (err < 0) {
		pr_err("bmp380 soft reset fail,exit probe!\n");
		goto i2c_fail;
	}

	mdelay(2);

	tx_buf[0] = BOSCH_BMP380_REG_DIG_T1;
	err = bmp_i2c_read_block(client, tx_buf[0], rx_buf, 0x08);
	if (err < 0) {
		pr_err("bmp380 read block BOSCH_BMP380_REG_DIG_T1 fail\n");
		goto i2c_fail;
	}
	memcpy((uint8_t*)&priv->comp, rx_buf, 8);

	tx_buf[0] = BOSCH_BMP380_REG_DIG_T1 + 8;
	err = bmp_i2c_read_block(client, tx_buf[0], rx_buf, 0x08);
	if (err < 0) {
		pr_err("bmp380 read block BOSCH_BMP380_REG_DIG_T1+8 fail\n");
		goto i2c_fail;
	}
	memcpy((uint8_t*)((uint8_t*)&priv->comp + 8), rx_buf, 8);

	tx_buf[0] = BOSCH_BMP380_REG_DIG_T1 + 16;
	err = bmp_i2c_read_block(client, tx_buf[0], rx_buf, 0x05);
	if (err < 0) {
		pr_err("bmp380 read block BOSCH_BMP380_REG_DIG_T1+16 fail\n");
		goto i2c_fail;
	}
	memcpy((uint8_t*)((uint8_t*)&priv->comp + 16), rx_buf, 5);

	tx_buf[0] = ((4 << 3) | (1 << 0));//config oversampling: baro:16x, temp:2x
	err = bmp_i2c_write_block(client, BOSCH_BMP380_REG_CTRL_OSR,
				  tx_buf, 0x01);
	if (err < 0) {
		pr_err("bmp380 write block BOSCH_BMP380_REG_CTRL_OSR fail!\n");
	}

	tx_buf[0] = 4;//config standy time: 62.5ms
	err = bmp_i2c_write_block(client, BOSCH_BMP380_REG_CTRL_ODR,
				  tx_buf, 0x01);
	if (err < 0) {
		pr_err("bmp380 write block BOSCH_BMP380_REG_CTRL_ODR fail!\n");
		goto i2c_fail;
	}

i2c_fail:
	return err;
}

static int64_t compensate_temp(struct bmp380_device_priv *priv,
				uint32_t uncomp_temp)
{

	uint64_t partial_data1;
	uint64_t partial_data2;
	uint64_t partial_data3;
	int64_t partial_data4;
	int64_t partial_data5;
	int64_t partial_data6;
	int64_t comp_temp;

	partial_data1 = uncomp_temp - (256 * priv->comp.par_t1);
	partial_data2 = priv->comp.par_t2 * partial_data1;
	partial_data3 = partial_data1 * partial_data1;
	partial_data4 = (int64_t)partial_data3 * priv->comp.par_t3;
	partial_data5 = ((int64_t)(partial_data2 * 262144) + partial_data4);
	partial_data6 = partial_data5 / 4294967296;
	priv->comp.t_lin = partial_data6;
	comp_temp = (int64_t)((partial_data6 * 25) / 16384);

	//return the tempeature in the unit of 0.01 centigrade.
	return comp_temp;
}

static int64_t compensate_baro(struct bmp380_device_priv *priv,
					uint32_t uncomp_press)
{
	int64_t partial_data1;
	int64_t partial_data2;
	int64_t partial_data3;
	int64_t partial_data4;
	int64_t partial_data5;
	int64_t partial_data6;
	int64_t offset;
	int64_t sensitivity;
	uint64_t comp_press;

	partial_data1 = priv->comp.t_lin * priv->comp.t_lin;
	partial_data2 = partial_data1 / 64;
	partial_data3 = (partial_data2 * priv->comp.t_lin) / 256;
	partial_data4 = (priv->comp.par_p8 * partial_data3) / 32;
	partial_data5 = (priv->comp.par_p7 * partial_data1) * 16;
	partial_data6 = (priv->comp.par_p6 * priv->comp.t_lin)
		* 4194304;
	offset = (priv->comp.par_p5 * 140737488355328) + partial_data4
		+ partial_data5 + partial_data6;

	partial_data2 = (priv->comp.par_p4 * partial_data3) / 32;
	partial_data4 = (priv->comp.par_p3 * partial_data1) * 4;
	partial_data5 = (priv->comp.par_p2 - 16384)
		* priv->comp.t_lin * 2097152;
	sensitivity = ((priv->comp.par_p1 - 16384) * 70368744177664)
		+ partial_data2 + partial_data4	+ partial_data5;

	partial_data1 = (sensitivity / 16777216) * uncomp_press;
	partial_data2 = priv->comp.par_p10 * priv->comp.t_lin;
	partial_data3 = partial_data2 + (65536 * priv->comp.par_p9);
	partial_data4 = (partial_data3 * uncomp_press) / 8192;
	partial_data5 = (partial_data4 * uncomp_press / 10) / 512 * 10;
	partial_data6 = (int64_t)((uint64_t)uncomp_press
		* (uint64_t)uncomp_press);
	partial_data2 = (priv->comp.par_p11 * partial_data6) / 65536;
	partial_data3 = (partial_data2 * uncomp_press) / 128;
	partial_data4 = (offset / 4) + partial_data1 + partial_data5
		+ partial_data3;
	comp_press = (((uint64_t)partial_data4 * 25)
		/ (uint64_t)1099511627776);

	//return the press in the unit of the 0.01 Pa.
	return comp_press;
}

static int bmp_get_pressure(struct bmp380_device_priv *priv, s32 *p_buf)
{
	uint32_t press_adc;
	uint32_t temp_adc;
	int ret = 0;
	uint32_t data_xlsb;
	uint32_t data_lsb;
	uint32_t data_msb;
	uint8_t tx_buf[1] = {0};
	uint8_t rx_buf[6] = {0};
	int64_t temp = 0, press = 0;
	struct i2c_client *client = priv->client;

	tx_buf[0] = BOSCH_BMP380_REG_PRESS_LSB;
	ret = bmp_i2c_read_block(client, tx_buf[0], rx_buf, 6);
	if (ret < 0) {
		pr_err("%s failed\n", __func__);
		return ret;
	}

	data_xlsb = (uint32_t)rx_buf[0];
	data_lsb = (uint32_t)rx_buf[1] << 8;
	data_msb = (uint32_t)rx_buf[2] << 16;
	press_adc  = data_msb | data_lsb | data_xlsb;

	data_xlsb = (uint32_t)rx_buf[3];
	data_lsb = (uint32_t)rx_buf[4] << 8;
	data_msb = (uint32_t)rx_buf[5] << 16;
	temp_adc = data_msb | data_lsb | data_xlsb;

	temp = compensate_temp(priv, temp_adc);
	press = compensate_baro(priv, press_adc);
	*p_buf = (s32)press * BOSCH_BMP380_GAIN_VALUE / 10000;

	return 0;
}

static int bmp380_sample(struct bmp380_device_priv *priv)
{
	s32 value = 0;
	int err = 0;

	if (!priv) {
		pr_err("bmp380 sample failed:invalid bmp380_device_priv\n");
		return -1;
	}

	err = bmp_get_pressure(priv, &value);
	if (err) {
		pr_err("bmp_get_pressure failed\n");
		return err;
	}

	//pr_info("%s successfully get sample: %d\n", __func__, value);
	return value;
}

static int bmp380_enable(struct bmp380_device_priv *priv, int en)
{
	int ret = 0;
	struct i2c_client *client;
	uint8_t tx_buf[1] = {0};
	int retry = 0;
	client = priv->client;

	if (en)
		tx_buf[0] = (3 << 4 | 1 << 1 | 1 << 0);
	else
		tx_buf[0] = (2 << 5 | 5 << 2);

	for (retry = 0; retry < 3; retry++) {
		ret = bmp_i2c_write_block(client,
					BOSCH_BMP380_REG_CTRL_PWR,
					tx_buf, 0x01);
		if (ret >= 0) {
			pr_debug("bmp380_enable (%d) done(retry:%d)\n",
				en, retry);
			break;
		}
	}

	return ret;
}

static int bmp380_read_raw(struct iio_dev *indio_dev,
			 struct iio_chan_spec const *chan,
			 int *val, int *val2, long mask)
{
	struct bmp380_device_priv *priv = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_PRESSURE:
			bmp380_enable(priv, 1);
			*val = bmp380_sample(priv);
			bmp380_enable(priv, 0);
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static const struct iio_info bmp380_info = {
	.read_raw   = &bmp380_read_raw,
};

static int bmp380_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	struct device *dev = &client->dev;
	struct iio_dev *indio_dev;
	struct bmp380_device_priv *priv;
	int ret;

	err = bmp380_check_chip_id(client);
	if (err < 0) {
		pr_err("bmp380 chip id mismatch,exit probe!\n");
		goto findHW_fail;
	}

	indio_dev = devm_iio_device_alloc(dev, sizeof(*priv));
	if (!indio_dev) {
		err = -ENOMEM;
		goto malloc_fail;
	}

	priv = iio_priv(indio_dev);
	priv->client = client;

	indio_dev->dev.parent = dev;
	indio_dev->name = id->name;
	indio_dev->channels = bmp380_channels;
	indio_dev->num_channels = ARRAY_SIZE(bmp380_channels);
	indio_dev->info = &bmp380_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(dev, "Failed to register IIO device\n");
		goto init_fail;
	}

	i2c_set_clientdata(client, indio_dev);

	err = bmp380_init_device(client, priv);
	if (err < 0) {
		pr_err("%s fail\n", __func__);
		goto init_fail;
	}

	bmp380_enable(priv, 1);
	ret = bmp380_sample(priv);
	bmp380_enable(priv, 0);

	pr_info("%s success! Pressure value: %d\n", __func__, ret);
	return 0;

init_fail:
//create_manager_fail:
	kfree(indio_dev);
malloc_fail:
findHW_fail:
	pr_err("%s fail!\n", __func__);
	return err;
}

static int bmp380_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);

	iio_device_unregister(indio_dev);
	kfree(indio_dev);
	return 0;
}

static const struct i2c_device_id bmp380_id[] = {
	{PRESSURE_SENSOR_NAME, 0},
	{},
};

static const struct of_device_id bmp380_of_match[] = {
	{.compatible = "mediatek,barometer"},
	{},
};

static struct i2c_driver bmp380_driver = {
	.driver = {
		.name = PRESSURE_SENSOR_NAME,
		.bus = &i2c_bus_type,
		.owner = THIS_MODULE,
		.of_match_table = bmp380_of_match,
	},
	.probe = bmp380_probe,
	.remove = bmp380_remove,
	.id_table = bmp380_id
};

module_i2c_driver(bmp380_driver);

MODULE_DESCRIPTION("bmp380 driver");
MODULE_AUTHOR("Mediatek");
MODULE_LICENSE("GPL");
