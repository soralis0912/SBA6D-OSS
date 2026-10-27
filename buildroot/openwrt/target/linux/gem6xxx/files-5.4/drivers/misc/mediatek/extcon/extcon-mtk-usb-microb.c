// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/extcon-provider.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/usb/role.h>
#include <linux/workqueue.h>
#include <linux/regmap.h>

#include "extcon-mtk-usb-microb.h"

#define MT6361_TOP_STAT1_ADDR 0x00c0
#define MT6361_STAT_VDET_SHIFT 2

static bool mtk_usb_extcon_otg_status(struct mtk_extcon_info *extcon)
{
    int id;
    /* check ID and update cable state */
    id = extcon->id_gpiod ?
	gpiod_get_value_cansleep(extcon->id_gpiod) : 1;

    dev_info(extcon->dev, "iddig value : %d - %s\n", id,
	    id ? "OTG not detected!" : "OTG detected");
    return id ? false : true;
}

static void mtk_usb_extcon_update_role(struct work_struct *work)
{
    struct usb_role_info *role = container_of(to_delayed_work(work),
	    struct usb_role_info, dwork);
    struct mtk_extcon_info *extcon = role->extcon;
    unsigned int cur_dr, new_dr;

    cur_dr = extcon->c_role;
    new_dr = role->d_role;

    dev_info(extcon->dev, "cur_dr(%s) new_dr(%s)\n",
	    dual_prop_dr_string[cur_dr], dual_prop_dr_string[new_dr]);

    /* none -> device */
    if (cur_dr == DUAL_PROP_DR_NONE &&
	    new_dr == DUAL_PROP_DR_DEVICE) {
	extcon_set_state_sync(extcon->edev, EXTCON_USB, true);
	/* none -> host */
    } else if (cur_dr == DUAL_PROP_DR_NONE &&
	    new_dr == DUAL_PROP_DR_HOST) {
	extcon_set_state_sync(extcon->edev, EXTCON_USB_HOST, true);
	/* device -> none */
    } else if (cur_dr == DUAL_PROP_DR_DEVICE &&
	    new_dr == DUAL_PROP_DR_NONE) {
	extcon_set_state_sync(extcon->edev, EXTCON_USB, false);
	/* host -> none */
    } else if (cur_dr == DUAL_PROP_DR_HOST &&
	    new_dr == DUAL_PROP_DR_NONE) {
	extcon_set_state_sync(extcon->edev, EXTCON_USB_HOST, false);
	/* device -> host */
    } else if (cur_dr == DUAL_PROP_DR_DEVICE &&
	    new_dr == DUAL_PROP_DR_HOST) {
	extcon_set_state_sync(extcon->edev, EXTCON_USB, false);
	extcon_set_state_sync(extcon->edev,	EXTCON_USB_HOST, true);
	/* host -> device */
    } else if (cur_dr == DUAL_PROP_DR_HOST &&
	    new_dr == DUAL_PROP_DR_DEVICE) {
	extcon_set_state_sync(extcon->edev, EXTCON_USB_HOST, false);
	extcon_set_state_sync(extcon->edev,	EXTCON_USB, true);
    }

    /* usb role switch */
    if (extcon->role_sw) {
	if (new_dr == DUAL_PROP_DR_DEVICE)
	    usb_role_switch_set_role(extcon->role_sw,
		    USB_ROLE_DEVICE);
	else if (new_dr == DUAL_PROP_DR_HOST)
	    usb_role_switch_set_role(extcon->role_sw,
		    USB_ROLE_HOST);
	else
	    usb_role_switch_set_role(extcon->role_sw,
		    USB_ROLE_NONE);
    }

    extcon->c_role = new_dr;
    kfree(role);
}

static int mtk_usb_extcon_set_role(struct mtk_extcon_info *extcon,
	unsigned int role)
{
    struct usb_role_info *role_info;

    /* create and prepare worker */
    role_info = kzalloc(sizeof(*role_info), GFP_KERNEL);
    if (!role_info)
	return -ENOMEM;

    INIT_DELAYED_WORK(&role_info->dwork, mtk_usb_extcon_update_role);

    role_info->extcon = extcon;
    role_info->d_role = role;
    /* issue connection work */
    queue_delayed_work(extcon->extcon_wq, &role_info->dwork, 0);

    return 0;
}

static int mtk_usb_extcon_set_vbus(struct mtk_extcon_info *extcon,
	bool is_on)
{
    struct device *dev = extcon->dev;

    if (extcon->vbus_on == is_on)
	return 0;

    dev_info(dev, "vbus turn %s\n", is_on ? "on" : "off");
    gpiod_set_value(extcon->drvbus_gpiod, is_on);

    extcon->vbus_on = is_on;

    return 0;
}

static irqreturn_t mtk_usb_extcon_iddig_handle(int irq, void *dev_id)
{
    struct mtk_extcon_info *extcon = dev_id;
    bool otg_state = mtk_usb_extcon_otg_status(extcon);

    if (otg_state) {
	mtk_usb_extcon_set_vbus(extcon, true);
	mtk_usb_extcon_set_role(extcon, DUAL_PROP_DR_HOST);
    } else {
	mtk_usb_extcon_set_vbus(extcon, false);
	mtk_usb_extcon_set_role(extcon, DUAL_PROP_DR_NONE);
    }

    return IRQ_HANDLED;
}

static int mtk_usb_extcon_iddig_pin_init(struct mtk_extcon_info *extcon)
{
    int ret = 0;
    bool otg_state;

    extcon->id_gpiod = devm_gpiod_get(extcon->dev, "id", GPIOD_IN);

    if (!extcon->id_gpiod || IS_ERR(extcon->id_gpiod)) {
	dev_err(extcon->dev, "failed to get id gpio\n");
	extcon->id_gpiod = NULL;
	return -EINVAL;
    }

    extcon->id_irq = gpiod_to_irq(extcon->id_gpiod);
    if (extcon->id_irq < 0) {
	dev_err(extcon->dev, "failed to get ID IRQ\n");
	extcon->id_gpiod = NULL;
	return -EINVAL;
    }

    ret = devm_request_threaded_irq(extcon->dev, extcon->id_irq, NULL,
	    mtk_usb_extcon_iddig_handle, IRQF_TRIGGER_RISING |
	    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
	    dev_name(extcon->dev), extcon);

    if (ret < 0) {
	dev_err(extcon->dev, "failed to request handler for ID IRQ\n");
	extcon->id_gpiod = NULL;
	return ret;
    }
    enable_irq_wake(extcon->id_irq);

    // get id pin value when boot on
    mtk_usb_extcon_iddig_handle(0, extcon);

    return 0;
}

static irqreturn_t mtk_usb_extcon_vdet_handle(int irq, void *data)
{
    struct mtk_extcon_info *extcon = data;
    u32 top_status = 0;
    bool otg_state = mtk_usb_extcon_otg_status(extcon);

    regmap_read(extcon->vdet_regmap,MT6361_TOP_STAT1_ADDR, &top_status);
    extcon->vdet_detect = top_status & (1 << MT6361_STAT_VDET_SHIFT);

    if (otg_state) {
	dev_info(extcon->dev, "device mode bypassed due to OTG detected!\n");
	return IRQ_HANDLED;
    }

    dev_info(extcon->dev,
	    "device mode %s!\n", extcon->vdet_detect ? "detected" : "undetected");
    if (extcon->vdet_detect)
	mtk_usb_extcon_set_role(extcon, DUAL_PROP_DR_DEVICE);
    else
	mtk_usb_extcon_set_role(extcon, DUAL_PROP_DR_NONE);

    return IRQ_HANDLED;
}

static int mtk_usb_extcon_vdet_pin_init(struct mtk_extcon_info *extcon)
{
    int ret = 0;
    struct platform_device *pdev = to_platform_device(extcon->dev);
    extcon->vdet_regmap = dev_get_regmap(pdev->dev.parent, NULL);
    if (!extcon->vdet_regmap) {
	dev_err(extcon->dev, "failed to get VDET regmap\n");
	extcon->vdet_regmap = NULL;
	return -ENODEV;
    }

    extcon->vdet_irq = platform_get_irq_byname(pdev, "vdet");
    if (extcon->vdet_irq < 0) {
	dev_err(extcon->dev, "failed to get VDET IRQ\n");
	extcon->vdet_regmap = NULL;
	extcon->vdet_irq = 0;
	return -EINVAL;
    }

    ret = devm_request_threaded_irq(extcon->dev, extcon->vdet_irq, NULL,
	    mtk_usb_extcon_vdet_handle, IRQF_ONESHOT | IRQF_TRIGGER_HIGH,
	    "mtk-pmic-chrdet", extcon);

    if (ret < 0) {
	dev_err(extcon->dev, "failed to request handler for VDET IRQ\n");
	extcon->vdet_regmap = NULL;
	extcon->vdet_irq = 0;
	return ret;
    }

    enable_irq_wake(extcon->vdet_irq);

    // get vdet pin value when boot on
    mtk_usb_extcon_vdet_handle(0, extcon);

    return 0;
}

static int mtk_usb_extcon_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct mtk_extcon_info *extcon;
    int ret;

    extcon = devm_kzalloc(&pdev->dev, sizeof(*extcon), GFP_KERNEL);
    if (!extcon)
	return -ENOMEM;

    extcon->dev = dev;

    /* extcon */
    extcon->edev = devm_extcon_dev_allocate(dev, usb_extcon_cable);
    if (IS_ERR(extcon->edev)) {
	dev_err(dev, "failed to allocate extcon device\n");
	return -ENOMEM;
    }

    ret = devm_extcon_dev_register(dev, extcon->edev);
    if (ret < 0) {
	dev_info(dev, "failed to register extcon device\n");
	return ret;
    }

    /* usb role switch */
    extcon->role_sw = usb_role_switch_get(extcon->dev);
    if (IS_ERR(extcon->role_sw)) {
	dev_err(dev, "failed to get usb role\n");
	return PTR_ERR(extcon->role_sw);
    }

    /* vbus */
    extcon->drvbus_gpiod =
	devm_gpiod_get(extcon->dev, "drvbus", GPIOD_OUT_LOW);

    if (!extcon->drvbus_gpiod || IS_ERR(extcon->drvbus_gpiod)) {
	dev_err(extcon->dev, "failed to get drvbus gpio\n");
	extcon->drvbus_gpiod = NULL;
	return -EINVAL;
    }

    extcon->extcon_wq = create_singlethread_workqueue("extcon_usb");
    extcon->c_role = DUAL_PROP_DR_NONE;
    ret = of_property_read_u32(dev->of_node, "usb_mode", &extcon->usb_mode);
    if (ret) {
	dev_err(dev, "some properties not found!\n");
	return ret;
    }

    /*get id resources*/
    if (extcon->usb_mode != DEVICE_ONLY) {
	ret = mtk_usb_extcon_iddig_pin_init(extcon);
	if (ret < 0)
	    dev_info(dev, "failed to init id pin\n");
    }

    if (extcon->usb_mode != OTG_ONLY) {
	ret = mtk_usb_extcon_vdet_pin_init(extcon);
	if (ret < 0)
	    dev_info(dev, "failed to init vdet pin\n");
    }

    platform_set_drvdata(pdev, extcon);
    return 0;
}

static const struct of_device_id mtk_usb_extcon_of_match[] = {
    { .compatible = "mediatek,extcon-usb-microb", },
    { },
};
MODULE_DEVICE_TABLE(of, mtk_usb_extcon_of_match);

static struct platform_driver mtk_usb_extcon_driver = {
    .probe		= mtk_usb_extcon_probe,
    .driver		= {
	.name	= "mtk-extcon-usb-microb",
	.of_match_table = mtk_usb_extcon_of_match,
    },
};

static int __init mtk_usb_extcon_init(void)
{
    return platform_driver_register(&mtk_usb_extcon_driver);
}
late_initcall(mtk_usb_extcon_init);

static void __exit mtk_usb_extcon_exit(void)
{
    platform_driver_unregister(&mtk_usb_extcon_driver);
}
module_exit(mtk_usb_extcon_exit);

