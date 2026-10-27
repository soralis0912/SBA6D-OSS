/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */
struct mtk_extcon_info {
	struct device *dev;
	struct extcon_dev *edev;
	struct usb_role_switch *role_sw;
	unsigned int c_role; /* current data role */
	struct workqueue_struct *extcon_wq;
	bool vbus_on;
	struct gpio_desc *id_gpiod;
	unsigned int id_irq;
	struct gpio_desc *drvbus_gpiod;
	unsigned int usb_mode;
	unsigned int vdet_irq;
	struct regmap *vdet_regmap;
	bool vdet_detect;
};

struct usb_role_info {
	struct mtk_extcon_info *extcon;
	struct delayed_work dwork;
	unsigned int d_role; /* desire data role */
};

enum {
	DUAL_PROP_DR_HOST = 0,
	DUAL_PROP_DR_DEVICE,
	DUAL_PROP_DR_NONE,
};

static const unsigned int usb_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_NONE,
};

static const char * const dual_prop_dr_string[] = {
	"DUAL_PROP_DR_HOST",
	"DUAL_PROP_DR_DEVICE",
	"DUAL_PROP_DR_NONE",
};

enum {
	OTG_AND_DEVICE = 0,
	DEVICE_ONLY,
	OTG_ONLY,
};
