// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/bitops.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

#include "mtk_ccci_common.h"
#include "mtk_mhccif.h"
#include "pcie_common.h"

/* PCIe print funciton */
#define pcie_info(fmt, args...)      pr_info("[PCIE] " fmt, ##args)

#define PCIE_EP_DEV_NAME	"pcie_ep"
#define PCIE_EP_IOC_MAGIC	'B'
#define PCIE_EP_IOC_REMOTE_WAKE	_IO(PCIE_EP_IOC_MAGIC, 0)

#define PCIE_CMD_REMOTE_WAKE	1
#define PCIE_CMD_REMOTE_WAKE_KE	2

static void mtk_reg_set_field(void __iomem *reg, int h, int l, u32 data)
{
	int val;

	val = readl(reg);
	val &= ~GENMASK(h, l);
	val |= ((data << l) & GENMASK(h, l));
	writel(val, reg);
}

static int mtk_pcie_set_dipc_data(struct mtk_pcie_ep *info, u32 data)
{
	writel(data, info->pcie_reg + PCIE_DIPC_REG);

	return 0;
}

static int mtk_pcie_set_stage_data(struct mtk_pcie_ep *info, u32 data)
{
	mtk_reg_set_field(info->pcie_reg + PCIE_DIPC_REG, PCIE_DIPC_STAGE_HB,
			  PCIE_DIPC_STAGE_LB, data);

	return 0;
}

static int mtk_pcie_set_event_data(struct mtk_pcie_ep *info, u32 data)
{
	mtk_reg_set_field(info->pcie_reg + PCIE_DIPC_REG, PCIE_DIPC_LINUX_HB,
			  PCIE_DIPC_LINUX_LB, data);
	return 0;
}

static int mtk_pcie_set_port_data(struct mtk_pcie_ep *info, u32 data)
{
	mtk_reg_set_field(info->pcie_reg + PCIE_DIPC_REG, PCIE_DIPC_PORT_HB,
			  PCIE_DIPC_PORT_LB, data);

	return 0;
}

static int mtk_pcie_irq_to_host(struct mtk_pcie_ep *info)
{
	mtk_mhccif_irq_to_host(info->irq_channel);

	return 0;
}

static int mtk_pcie_get_ltssm_status(struct mtk_pcie_ep *info)
{
	int val;

	val = readl(info->pcie_reg + PCIE_LTSSM_STATUS);

	return (val & GENMASK(28, 24)) >> 24;
}

static int mtk_pcie_get_link_status(struct mtk_pcie_ep *info)
{
	return readl(info->pcie_reg + PCIE_LINK_STATUS);
}

int mtk_pcie_is_brom_download_mode(struct mtk_pcie_ep *info)
{
	int val;

	val = readl(info->pcie_reg + PCIE_DIPC_REG);
	val = (val & GENMASK(PCIE_DIPC_HOST_HB, PCIE_DIPC_HOST_LB))
		>> PCIE_DIPC_HOST_LB;

	return val != PCIE_DIPC_HOST_FASTBOOT;
}

static struct pcie_dipc_ops dipc_ops = {
	.set_data = mtk_pcie_set_dipc_data,
	.set_port_data = mtk_pcie_set_port_data,
	.set_event_data = mtk_pcie_set_event_data,
	.set_stage_data = mtk_pcie_set_stage_data,
	.irq_to_host = mtk_pcie_irq_to_host,
	.get_ltssm_status = mtk_pcie_get_ltssm_status,
	.get_link_status = mtk_pcie_get_link_status,
};

static long mtk_pcie_ep_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err;
	struct pcie_ccci_msg msg_cmd;
	struct mtk_pcie_ep *info = file->private_data;

	if (info->destroyed) {
		pr_info("%s: PCIe device has been destroyed \n", __func__);
		return -EPERM;
	}

	switch(cmd) {
	case PCIE_EP_IOC_REMOTE_WAKE:
		msg_cmd.cmd = PCIE_CMD_REMOTE_WAKE;
		err = exec_ccci_kern_func_by_md_id(MD_SYS1, SYSMSGSV_PCIE_PM_NOTIFY,
			(char *)&msg_cmd, sizeof(unsigned int));
		if (err) {
			pr_info("[%d] remote wakeup send failed\n", err);
			return err;
		}
		pr_info("PCIe remote wakeup send success\n");
		break;
	default:
		break;
	}
	return 0;
}

static int mtk_pcie_ep_open(struct inode *inode, struct file *file)
{
	struct mtk_pcie_ep *info;

	info = container_of(inode->i_cdev, struct mtk_pcie_ep, ep_cdev);
	file->private_data = info;

	return 0;
}

static const struct file_operations fops = {
    .unlocked_ioctl = mtk_pcie_ep_ioctl,
    .open = mtk_pcie_ep_open,
};

/* Create sysfs nodes */
static ssize_t link_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pcie_ep *ep_dev = dev_get_drvdata(dev);
	bool link_state;

	link_state = pcie_is_linkup(ep_dev);

	dev_info(ep_dev->dev, "PCIe link %s\n", link_state ? "UP" : "Down");

	if (!link_state)
		dev_info(ep_dev->dev, "Current ltssm state: %#x\n",
			 ep_dev->ops->get_ltssm_status(ep_dev));


	return sprintf(buf, "%d", link_state);
}

static ssize_t ltssm_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pcie_ep *ep_dev = dev_get_drvdata(dev);
	int ltssm_state;

	ltssm_state = ep_dev->ops->get_ltssm_status(ep_dev);

	dev_info(ep_dev->dev, "PCIe ltssm state: %#x\n", ltssm_state);

	return sprintf(buf, "%d", ltssm_state);
}

static ssize_t pcie_info_show(struct device *dev, struct device_attribute *atte, char *buf)
{
	struct mtk_pcie_ep *ep_dev = dev_get_drvdata(dev);
	u32 val;
	u16 device_id, vendor_id;
	u32 class_code;

	val = readl(ep_dev->pcie_reg + PCIE_DEVICE_ID);
	device_id = (val >> 16) & 0xffff;
	vendor_id = val & 0xffff;

	class_code = readl(ep_dev->pcie_reg + PCIE_CLASS_CODE);
	dev_info(ep_dev->dev, "device id: %#x\n", device_id);
	dev_info(ep_dev->dev, "vendor id: %#x\n", vendor_id);
	dev_info(ep_dev->dev, "class code: %#x\n", class_code);

	return sprintf(buf, "%x-%x-%x", device_id, vendor_id, class_code);
}

DEVICE_ATTR_RO(link_state);
DEVICE_ATTR_RO(ltssm_state);
DEVICE_ATTR_RO(pcie_info);

static struct attribute *pcie_sysfs_attrs[] = {
	&dev_attr_link_state.attr,
	&dev_attr_ltssm_state.attr,
	&dev_attr_pcie_info.attr,
	NULL,
};

ATTRIBUTE_GROUPS(pcie_sysfs);

static int mtk_pcie_create_ep_cdev(struct mtk_pcie_ep *info)
{
	dev_t dev = 0;
	int err = 0;
	struct cdev *dev_ctx = &info->ep_cdev;

	err = alloc_chrdev_region(&dev, 0, 1, PCIE_EP_DEV_NAME);
	if (err) {
		pr_info("pcie_dipc_init alloc_chrdev_region failed -\n");
		return err;
	}

	cdev_init(dev_ctx, &fops);
	dev_ctx->owner = THIS_MODULE;

	err = cdev_add(dev_ctx, dev, 1);
	if (err)
		return err;

	info->ep_class = class_create(THIS_MODULE, PCIE_EP_DEV_NAME);
	info->class_dev = device_create_with_groups(info->ep_class, NULL,
						    dev_ctx->dev, info,
						    pcie_sysfs_groups,
						    PCIE_EP_DEV_NAME);
	if (IS_ERR(info->class_dev)) {
		dev_notice(info->dev, "Failed to create sysfs files\n");
		return PTR_ERR(info->class_dev);
	}

	return 0;
}

void mtk_pcie_destroy_ep_cdev(struct mtk_pcie_ep *info)
{
	struct cdev *dev_ctx = &info->ep_cdev;

	device_destroy(info->ep_class, dev_ctx->dev);
	class_destroy(info->ep_class);
	unregister_chrdev_region(dev_ctx->dev, MINORMASK);
}

static int wakeup_remote_pcie(void)
{
	int err;
	struct pcie_ccci_msg msg_cmd;

	msg_cmd.cmd = PCIE_CMD_REMOTE_WAKE_KE;
	err = exec_ccci_kern_func_by_md_id(MD_SYS1, SYSMSGSV_PCIE_PM_NOTIFY,
		(char *)&msg_cmd, sizeof(unsigned int));
	if (err)
		pcie_info("[%d] remote wakeup send failed\n", err);
	else
		pcie_info("PCIe remote wakeup send success\n");
	return err;
}

static int ipanic(struct notifier_block *this, unsigned long event, void *ptr)
{
	pcie_info("panic\n");

	wakeup_remote_pcie();

	return 0;
}

static int ipanic_die(struct notifier_block *self, unsigned long cmd, void *ptr)
{
	pcie_info("oops\n");

	wakeup_remote_pcie();

	return 0;
}

static struct notifier_block panic_blk = {
	.notifier_call = ipanic,
	.priority = INT_MAX,
};

static struct notifier_block die_blk = {
	.notifier_call = ipanic_die,
	.priority = INT_MAX,
};

static int mtk_pcie_ep_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct mtk_pcie_ep *ep_dev;
	struct device *dev = &pdev->dev;

	pcie_info("pcie init\n");

	ep_dev = devm_kzalloc(&pdev->dev, sizeof(*ep_dev), GFP_KERNEL);
	if (!ep_dev) {
		pcie_info("No mem for dipc!\n");
		return -ENOMEM;
	}

	ep_dev->dev = dev;
	ep_dev->ops = &dipc_ops;
	ep_dev->irq_channel = MHCCIF_D2H_INT_DIPC;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ep_dev->pcie_reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(ep_dev->pcie_reg)) {
		pcie_info("PCIe register get failed!\n");
		ret = PTR_ERR(ep_dev->pcie_reg);
		goto free_resource;
	}

	/* phy power on and enable pipe clock */
	ep_dev->phy = devm_phy_optional_get(dev, "pcie-phy");
	if (IS_ERR(ep_dev->phy))
		ep_dev->phy = NULL;

	ret = phy_set_mode(ep_dev->phy, PHY_MODE_PCIE_EP);
	if (ret) {
		dev_notice(dev, "failed to set phy mode\n");
		goto free_resource;
	}

	ret = phy_init(ep_dev->phy);
	if (ret) {
		dev_err(dev, "failed to initialize pcie phy\n");
		goto free_resource;
	}

	ret = phy_power_on(ep_dev->phy);
	if (ret) {
		dev_err(dev, "failed to power on pcie phy\n");
		goto err_phy_on;
	}

	platform_set_drvdata(pdev, ep_dev);
	mtk_pcie_create_cdev(ep_dev);
	mtk_pcie_create_ep_cdev(ep_dev);

	pcie_info("register ke notifiers\n");
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	register_die_notifier(&die_blk);

	return 0;

err_phy_on:
	phy_exit(ep_dev->phy);
free_resource:
	devm_kfree(dev, ep_dev);

	return ret;
}

static int mtk_pcie_ep_remove(struct platform_device *pdev)
{
	struct mtk_pcie_ep *info = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	pcie_info("pcie exit\n");

	mtk_pcie_destroy_cdev(info);
	mtk_pcie_destroy_ep_cdev(info);
	devm_iounmap(dev, info->pcie_reg);
	devm_kfree(dev, info);

	pcie_info("unregister ke notifiers\n");
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_blk);
	unregister_die_notifier(&die_blk);

	return 0;
}

static int mtk_pcie_suspend(struct device *dev)
{
	pcie_info("%s: %d\n", __func__, __LINE__);
	return 0;
}

static int mtk_pcie_resume(struct device *dev)
{
	pcie_info("%s: %d\n", __func__, __LINE__);
	return 0;
}

static const struct dev_pm_ops mtk_pcie_pm_ops = {
	.suspend = mtk_pcie_suspend,
	.resume = mtk_pcie_resume,
};

static const struct of_device_id pcie_dipc_of_ids[] = {
	{ .compatible = "mediatek,mt6279-pcie-ep", },
	{ .compatible = "mediatek,mt6880-pcie-ep", },
	{}
};

static struct platform_driver mtk_pcie_ep_driver = {
	.probe = mtk_pcie_ep_probe,
	.remove = mtk_pcie_ep_remove,
	.driver = {
		.name = "mtk-pcie-ep",
		.owner = THIS_MODULE,
		.pm = &mtk_pcie_pm_ops,
		.of_match_table = of_match_ptr(pcie_dipc_of_ids),
	},
};

module_platform_driver(mtk_pcie_ep_driver);
MODULE_LICENSE("GPL");
