// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2022 MediaTek Inc.
 */
 
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/printk.h>
#include <linux/syscore_ops.h>

enum TRACKER_ID {
	TRACKER,
	INFRA_TRACKER,
	PERI_TRACKER,
};

static struct wakeup_source *mtk_suspend_lock_demo;

static int systracker_syssuspend(void)
{
 	printk("systracker_syssuspend\n");
	return 0;
}

static void systracker_sysresume(void)
{
 	printk("systracker_sysresume\n");

	/* This is for DVT testing to verify tracker can work after suspend/resume. */
#if 0
	mtk_suspend_lock_demo = wakeup_source_register(NULL, "mtk_suspend_lock_demo");
	if (!mtk_suspend_lock_demo) {
		pr_info("%s %d: init wakeup source fail!", __func__, __LINE__);
		return -1;
	}
	__pm_stay_awake(mtk_suspend_lock_demo);
#endif
}

static struct syscore_ops systracker_sysops = {
	.suspend = systracker_syssuspend,
	.resume = systracker_sysresume,
};

static inline irqreturn_t systracker_handle_irq(int irq, void *_tracker_id)
{
	u64 idx = (u64)_tracker_id;

 	printk("systracker_handle_irq: %d\n", idx);
	return IRQ_HANDLED;
}

static int systracker_request_irq(struct platform_device *pdev, char *irq_name,
				     irq_handler_t handler, void *irq_priv)
{
	int ret, irq;
	struct device *dev = &pdev->dev;

	irq = platform_get_irq_byname(pdev, irq_name);
	if (irq < 0) {
		dev_err(dev, "unable to get IRQ '%s' (err %d)\n",
			irq_name, irq);
		return irq;
	}

	ret = devm_request_irq(dev, irq, handler,
				IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
				irq_name, irq_priv);
	if (ret) {
		dev_err(dev, "unable to request IRQ %d\n", irq);
		return ret;
	}

	dev_info(dev, "%s: %d success!\n", __func__, irq);
	return irq;
}

static int systracker_probe(struct platform_device *pdev)
{
	int ret, irq;

	dev_info(&pdev->dev, "driver probed\n");

	systracker_request_irq(pdev, "tracker-irq", systracker_handle_irq, (void *)TRACKER);
	systracker_request_irq(pdev, "infra-tracker-irq", systracker_handle_irq, (void *)INFRA_TRACKER);
	systracker_request_irq(pdev, "peri-tracker-irq", systracker_handle_irq, (void *)PERI_TRACKER);

	register_syscore_ops(&systracker_sysops);
	return 0;
}

static int systracker_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "driver removed\n");

	return 0;
}
 
static const struct of_device_id systracker_of_ids[] = {
	{ .compatible = "mediatek,sys-tracker", .data = (void *)0 },
	{}
};

static struct platform_driver systracker_drv = {
	.driver = {
	.name = "sysTracker",
	.bus = &platform_bus_type,
	.owner = THIS_MODULE,
	.of_match_table = systracker_of_ids,
	},
	.probe = systracker_probe,
	.remove = systracker_remove,
};

static int __init systracker_init(void)
{
	int ret;

 	printk("tracker module init\n");
	ret = platform_driver_register(&systracker_drv);
	if (ret)
		return ret;

	return 0;
}
 
static __exit void systracker_exit(void)
{
 	printk("tracker module exit\n");
	platform_driver_unregister(&systracker_drv);
}

module_init(systracker_init);
module_exit(systracker_exit);

MODULE_DESCRIPTION("MediaTek SysTracker Driver");
MODULE_LICENSE("GPL v2");
