// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pinctrl/consumer.h>
#include <linux/uaccess.h>
#include "pcie_common.h"

#define DEVICE_NAME "pcie_dipc"

#define PCIE_DIPC_IOC_MAGIC		'K'
#define PCIE_DIPC_IOC_SET_MODE		_IOW(PCIE_DIPC_IOC_MAGIC, 0, unsigned int)
#define PCIE_DIPC_IOC_SET_ENUM_PORTS	_IOW(PCIE_DIPC_IOC_MAGIC, 1, unsigned int)
#define PCIE_DIPC_IOC_TRIGGER_IRQ	_IOW(PCIE_DIPC_IOC_MAGIC, 2, unsigned int)
#define PCIE_DIPC_IOC_QUERY_LINK_STATUS	_IOWR(PCIE_DIPC_IOC_MAGIC, 3, unsigned int)
#define PCIE_DIPC_IOC_DESTROY_CDEV	_IOWR(PCIE_DIPC_IOC_MAGIC, 4, unsigned int)

static struct class *pcie_dipc_class = NULL;

static void mtk_pcie_gpio_pulldown(struct mtk_pcie_ep *info)
{
	struct pinctrl *pcie_gpio;
	struct pinctrl_state *state;

	pcie_gpio = devm_pinctrl_get(info->dev);
	if (!IS_ERR(pcie_gpio)) {
		state = pinctrl_lookup_state(pcie_gpio, "pull_down");
		if (!IS_ERR(state))
			pinctrl_select_state(pcie_gpio, state);
		else
			dev_info(info->dev, "Can't find pcie gpio pull down state\n");
	} else {
		dev_info(info->dev, "Can't find pcie gpio\n");
	}
}

static long pcie_dipc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int __user *uarg = (int __user *)arg;
	int ret;
	unsigned int pcie_linkup_status;
	struct mtk_pcie_ep *info = file->private_data;

	if (_IOC_TYPE(cmd) != PCIE_DIPC_IOC_MAGIC)
		return -EINVAL;

	if (info->destroyed) {
		pr_info("%s: PCIe device has been destroyed \n", __func__);
		return -EPERM;
	}

	switch(cmd) {
	case PCIE_DIPC_IOC_SET_MODE:
		pr_info("%s: PCIE_DIPC_IOC_SET_MODE %d \n", __func__, (unsigned int)arg);
		pcie_dipc_write_mode(info, (u8)arg);
		break;
	case PCIE_DIPC_IOC_SET_ENUM_PORTS:
		pr_info("%s: PCIE_DIPC_IOC_SET_ENUM_PORTS %d \n", __func__, (unsigned int)arg);
		pcie_dipc_write_port(info, (u16)arg);
		break;
	case PCIE_DIPC_IOC_TRIGGER_IRQ:
		pr_info("%s: PCIE_DIPC_IOC_TRIGGER_IRQ %d \n", __func__, (unsigned int)arg);
		pcie_dipc_trigger_irq(info);
		break;
	case PCIE_DIPC_IOC_QUERY_LINK_STATUS:
		pcie_linkup_status = pcie_is_linkup(info);
		ret = copy_to_user(uarg, &pcie_linkup_status, sizeof(pcie_linkup_status));
		pr_info("%s: PCIE_DIPC_IOC_QUERY_LINK_STATUS size:%d \n", __func__, pcie_linkup_status);
		break;
	case PCIE_DIPC_IOC_DESTROY_CDEV:
		mtk_pcie_destroy_cdev(info);
		mtk_pcie_destroy_ep_cdev(info);
		mtk_pcie_gpio_pulldown(info);
		info->destroyed = true;
		pr_info("%s: PCIE_DIPC_IOC_DESTROY_CDEV\n", __func__);
		break;
	default:
		break;
	}

	return 0;
}

static int pcie_dipc_open(struct inode *inode, struct file *file)
{
	struct mtk_pcie_ep *info;

	info = container_of(inode->i_cdev, struct mtk_pcie_ep, cdev);
	file->private_data = info;

	return 0;
}

static const struct file_operations fops = {
    .unlocked_ioctl = pcie_dipc_ioctl,
    .open = pcie_dipc_open,
};

int mtk_pcie_create_cdev(struct mtk_pcie_ep *info)
{
	dev_t dev = 0;
	int err = 0;
	struct cdev *dev_ctx = &info->cdev;

	pr_info("pcie_dipc_init +\n");

	err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (err) {
		pr_info("pcie_dipc_init alloc_chrdev_region failed -\n");
		return err;
	}

	cdev_init(dev_ctx, &fops);
	dev_ctx->owner = THIS_MODULE;

	err = cdev_add(dev_ctx, dev, 1);
	if (err)
		return err;

	pcie_dipc_class = class_create(THIS_MODULE, DEVICE_NAME);
	device_create(pcie_dipc_class, NULL, dev_ctx->dev, NULL, DEVICE_NAME);

	printk(KERN_INFO "pcie_dipc_init -\n");
	return 0;
}

void mtk_pcie_destroy_cdev(struct mtk_pcie_ep *info)
{
	struct cdev *dev_ctx = &info->cdev;

	printk(KERN_INFO "pcie_dipc_exit +\n");

	device_destroy(pcie_dipc_class, dev_ctx->dev);
	class_destroy(pcie_dipc_class);
	unregister_chrdev_region(dev_ctx->dev, MINORMASK);

	printk(KERN_INFO "pcie_dipc_exit -\n");
}
