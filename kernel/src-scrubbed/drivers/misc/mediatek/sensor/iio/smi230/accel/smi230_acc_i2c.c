// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 MediaTek Inc.
 */


#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "smi230_acc.h"

#define SMI230_MAX_RETRY_I2C_XFER 10
#define SMI230_I2C_WRITE_DELAY_TIME 10

static struct i2c_adapter *smi230_i2c_adapter;

static struct smi230_dev smi230_i2c_dev;

static int8_t smi230_i2c_read(uint8_t dev_addr, uint8_t reg_addr,
		uint8_t *data, uint16_t len)
{
	int32_t retry;

	struct i2c_msg msg[] = {
		{
		 .addr = dev_addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &reg_addr,
		  },

		{
		 .addr = dev_addr,
		 .flags = I2C_M_RD,
		 .len = len,
		 .buf = data,
		  },
	};
	for (retry = 0; retry < SMI230_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(smi230_i2c_adapter, msg, ARRAY_SIZE(msg)) > 0)
			break;

		usleep_range(SMI230_I2C_WRITE_DELAY_TIME * 1000,
				 SMI230_I2C_WRITE_DELAY_TIME * 1000);
	}

	if (retry >= SMI230_MAX_RETRY_I2C_XFER) {
		pr_err("I2C xfer error");
		return -EIO;
	}

	return 0;
}

static int8_t smi230_i2c_write(uint8_t dev_addr,
			       uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	int32_t retry;
	struct i2c_msg msg = {
		.addr = dev_addr,
		.flags = 0,
		.len = len + 1,
		.buf = NULL,
	};

	msg.buf = kmalloc(len + 1, GFP_KERNEL);
	if (!msg.buf)
		return -ENOMEM;

	msg.buf[0] = reg_addr;
	memcpy(&msg.buf[1], data, len);

	for (retry = 0; retry < SMI230_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(smi230_i2c_adapter, &msg, 1) > 0)
			break;

		usleep_range(SMI230_I2C_WRITE_DELAY_TIME * 1000,
				 SMI230_I2C_WRITE_DELAY_TIME * 1000);
	}

	kfree(msg.buf);

	if (retry >= SMI230_MAX_RETRY_I2C_XFER) {
		pr_err("I2C xfer error");
		return -EIO;
	}

	return 0;
}

static int smi230_acc_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality error!");
		err = -EIO;
		return err;
	}

	smi230_i2c_adapter = client->adapter;
	smi230_i2c_dev.accel_id = client->addr;

	err = smi230_acc_init(&smi230_i2c_dev);
	if (err == SMI230_OK)
		pr_info("Bosch Sensor Device %s initialized", SENSOR_ACC_NAME);
	else {
		pr_err("Bosch Sensor Device %s initialization failed, error %d",
		       SENSOR_ACC_NAME, err);
	}

	return smi230_acc_probe(&client->dev, &smi230_i2c_dev);
}

static const struct i2c_device_id smi230_acc_id[] = {
	{ SENSOR_ACC_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, smi230_acc_id);

static const struct of_device_id smi230_acc_of_match[] = {
	{.compatible = "mediatek,accsmi230",}, //SENSOR_ACC_NAME, },
	{ }
};

MODULE_DEVICE_TABLE(of, smi230_acc_of_match);

struct i2c_driver smi230_acc_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_ACC_NAME,
		   .of_match_table = smi230_acc_of_match,
		    },
	.class = I2C_CLASS_HWMON,
	.id_table = smi230_acc_id,
	.probe = smi230_acc_i2c_probe,
	.remove = NULL,
};

static int __init smi230_module_init(void)
{
	int err = 0;

	smi230_i2c_dev.delay_ms = smi230_delay;
	smi230_i2c_dev.read_write_len = 32;
	smi230_i2c_dev.intf = SMI230_I2C_INTF;
	smi230_i2c_dev.read = smi230_i2c_read;
	smi230_i2c_dev.write = smi230_i2c_write;

	err |= i2c_add_driver(&smi230_acc_driver);

	return err;
}

static void __exit smi230_module_exit(void)
{
	i2c_del_driver(&smi230_acc_driver);
}

late_initcall(smi230_module_init);
//module_init(smi230_module_init);
module_exit(smi230_module_exit);

MODULE_DESCRIPTION("SMI230 ACC SENSOR DRIVER");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRIVER_VERSION);
