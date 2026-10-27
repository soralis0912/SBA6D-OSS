// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017-2018, Bootlin
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

#define CONFIG_ILI9881D_DEBUG_FS 	1

struct ili9881d {
	struct drm_panel	panel;
	struct mipi_dsi_device	*dsi;

	struct backlight_device *backlight;
	struct regulator	*power;
	struct gpio_desc	*reset;
};

enum ili9881d_op {
	ILI9881D_SWITCH_PAGE,
	ILI9881D_COMMAND,
};

struct ili9881d_instr {
	enum ili9881d_op	op;

	union arg {
		struct cmd {
			u8	cmd;
			u8	data;
		} cmd;
		u8	page;
	} arg;
};

#define ILI9881D_SWITCH_PAGE_INSTR(_page)	\
	{					\
		.op = ILI9881D_SWITCH_PAGE,	\
		.arg = {			\
			.page = (_page),	\
		},				\
	}

#define ILI9881D_COMMAND_INSTR(_cmd, _data)		\
	{						\
		.op = ILI9881D_COMMAND,		\
		.arg = {				\
			.cmd = {			\
				.cmd = (_cmd),		\
				.data = (_data),	\
			},				\
		},					\
	}

#if 0 
//origin para
static const struct ili9881d_instr ili9881d_init[] = {
	ILI9881D_SWITCH_PAGE_INSTR(0x3),
		ILI9881D_COMMAND_INSTR(0x01, 0x00),
		ILI9881D_COMMAND_INSTR(0x02, 0x00),
		ILI9881D_COMMAND_INSTR(0x03, 0x73),
		ILI9881D_COMMAND_INSTR(0x04, 0x00),
		ILI9881D_COMMAND_INSTR(0x05, 0x00),
		ILI9881D_COMMAND_INSTR(0x06, 0x0A),
		ILI9881D_COMMAND_INSTR(0x07, 0x00),
		ILI9881D_COMMAND_INSTR(0x08, 0x00),
		ILI9881D_COMMAND_INSTR(0x09, 0x01),
		ILI9881D_COMMAND_INSTR(0x0a, 0x00),
		ILI9881D_COMMAND_INSTR(0x0b, 0x00),
		ILI9881D_COMMAND_INSTR(0x0c, 0x01),
		ILI9881D_COMMAND_INSTR(0x0d, 0x00),
		ILI9881D_COMMAND_INSTR(0x0e, 0x00),
		ILI9881D_COMMAND_INSTR(0x0f, 0x1D),
		ILI9881D_COMMAND_INSTR(0x10, 0x0D),
		ILI9881D_COMMAND_INSTR(0x11, 0x00),
		ILI9881D_COMMAND_INSTR(0x12, 0x00),
		ILI9881D_COMMAND_INSTR(0x13, 0x00),
		ILI9881D_COMMAND_INSTR(0x14, 0x00),
		ILI9881D_COMMAND_INSTR(0x15, 0x00),
		ILI9881D_COMMAND_INSTR(0x16, 0x00),
		ILI9881D_COMMAND_INSTR(0x17, 0x00),
		ILI9881D_COMMAND_INSTR(0x18, 0x00),
		ILI9881D_COMMAND_INSTR(0x19, 0x00),
		ILI9881D_COMMAND_INSTR(0x1a, 0x00),
		ILI9881D_COMMAND_INSTR(0x1b, 0x00),
		ILI9881D_COMMAND_INSTR(0x1c, 0x00),
		ILI9881D_COMMAND_INSTR(0x1d, 0x00),
		ILI9881D_COMMAND_INSTR(0x1e, 0x40),
		ILI9881D_COMMAND_INSTR(0x1f, 0x80),
		ILI9881D_COMMAND_INSTR(0x20, 0x06),
		ILI9881D_COMMAND_INSTR(0x21, 0x02),
		ILI9881D_COMMAND_INSTR(0x22, 0x00),
		ILI9881D_COMMAND_INSTR(0x23, 0x00),
		ILI9881D_COMMAND_INSTR(0x24, 0x00),
		ILI9881D_COMMAND_INSTR(0x25, 0x00),
		ILI9881D_COMMAND_INSTR(0x26, 0x00),
		ILI9881D_COMMAND_INSTR(0x27, 0x00),
		ILI9881D_COMMAND_INSTR(0x28, 0x33),
		ILI9881D_COMMAND_INSTR(0x29, 0x03),
		ILI9881D_COMMAND_INSTR(0x2a, 0x00),
		ILI9881D_COMMAND_INSTR(0x2b, 0x00),
		ILI9881D_COMMAND_INSTR(0x2c, 0x00),
		ILI9881D_COMMAND_INSTR(0x2d, 0x00),
		ILI9881D_COMMAND_INSTR(0x2e, 0x00),
		ILI9881D_COMMAND_INSTR(0x2f, 0x00),
		ILI9881D_COMMAND_INSTR(0x30, 0x00),
		ILI9881D_COMMAND_INSTR(0x31, 0x00),
		ILI9881D_COMMAND_INSTR(0x32, 0x00),
		ILI9881D_COMMAND_INSTR(0x33, 0x00),
		ILI9881D_COMMAND_INSTR(0x34, 0x04),
		ILI9881D_COMMAND_INSTR(0x35, 0x00),
		ILI9881D_COMMAND_INSTR(0x36, 0x00),
		ILI9881D_COMMAND_INSTR(0x37, 0x00),
		ILI9881D_COMMAND_INSTR(0x38, 0x3C),
		ILI9881D_COMMAND_INSTR(0x39, 0x35),
		ILI9881D_COMMAND_INSTR(0x3a, 0x01),
		ILI9881D_COMMAND_INSTR(0x3b, 0x40),
		ILI9881D_COMMAND_INSTR(0x3c, 0x00),
		ILI9881D_COMMAND_INSTR(0x3d, 0x01),
		ILI9881D_COMMAND_INSTR(0x3e, 0x00),
		ILI9881D_COMMAND_INSTR(0x3f, 0x00),
		ILI9881D_COMMAND_INSTR(0x40, 0x00),
		ILI9881D_COMMAND_INSTR(0x41, 0x88),
		ILI9881D_COMMAND_INSTR(0x42, 0x00),
		ILI9881D_COMMAND_INSTR(0x43, 0x00),
		ILI9881D_COMMAND_INSTR(0x44, 0x1F),
		ILI9881D_COMMAND_INSTR(0x50, 0x01),
		ILI9881D_COMMAND_INSTR(0x51, 0x23),
		ILI9881D_COMMAND_INSTR(0x52, 0x45),
		ILI9881D_COMMAND_INSTR(0x53, 0x67),
		ILI9881D_COMMAND_INSTR(0x54, 0x89),
		ILI9881D_COMMAND_INSTR(0x55, 0xab),
		ILI9881D_COMMAND_INSTR(0x56, 0x01),
		ILI9881D_COMMAND_INSTR(0x57, 0x23),
		ILI9881D_COMMAND_INSTR(0x58, 0x45),
		ILI9881D_COMMAND_INSTR(0x59, 0x67),
		ILI9881D_COMMAND_INSTR(0x5a, 0x89),
		ILI9881D_COMMAND_INSTR(0x5b, 0xab),
		ILI9881D_COMMAND_INSTR(0x5c, 0xcd),
		ILI9881D_COMMAND_INSTR(0x5d, 0xef),
		ILI9881D_COMMAND_INSTR(0x5e, 0x11),
		ILI9881D_COMMAND_INSTR(0x5f, 0x01),
		ILI9881D_COMMAND_INSTR(0x60, 0x00),
		ILI9881D_COMMAND_INSTR(0x61, 0x15),
		ILI9881D_COMMAND_INSTR(0x62, 0x14),
		ILI9881D_COMMAND_INSTR(0x63, 0x0E),
		ILI9881D_COMMAND_INSTR(0x64, 0x0F),
		ILI9881D_COMMAND_INSTR(0x65, 0x0C),
		ILI9881D_COMMAND_INSTR(0x66, 0x0D),
		ILI9881D_COMMAND_INSTR(0x67, 0x06),
		ILI9881D_COMMAND_INSTR(0x68, 0x02),
		ILI9881D_COMMAND_INSTR(0x69, 0x07),
		ILI9881D_COMMAND_INSTR(0x6a, 0x02),
		ILI9881D_COMMAND_INSTR(0x6b, 0x02),
		ILI9881D_COMMAND_INSTR(0x6c, 0x02),
		ILI9881D_COMMAND_INSTR(0x6d, 0x02),
		ILI9881D_COMMAND_INSTR(0x6e, 0x02),
		ILI9881D_COMMAND_INSTR(0x6f, 0x02),
		ILI9881D_COMMAND_INSTR(0x70, 0x02),
		ILI9881D_COMMAND_INSTR(0x71, 0x02),
		ILI9881D_COMMAND_INSTR(0x72, 0x02),
		ILI9881D_COMMAND_INSTR(0x73, 0x02),
		ILI9881D_COMMAND_INSTR(0x74, 0x02),
		ILI9881D_COMMAND_INSTR(0x75, 0x01),
		ILI9881D_COMMAND_INSTR(0x76, 0x00),
		ILI9881D_COMMAND_INSTR(0x77, 0x14),
		ILI9881D_COMMAND_INSTR(0x78, 0x15),
		ILI9881D_COMMAND_INSTR(0x79, 0x0E),
		ILI9881D_COMMAND_INSTR(0x7a, 0x0F),
		ILI9881D_COMMAND_INSTR(0x7b, 0x0C),
		ILI9881D_COMMAND_INSTR(0x7c, 0x0D),
		ILI9881D_COMMAND_INSTR(0x7d, 0x06),
		ILI9881D_COMMAND_INSTR(0x7e, 0x02),
		ILI9881D_COMMAND_INSTR(0x7f, 0x07),
		ILI9881D_COMMAND_INSTR(0x80, 0x02),
		ILI9881D_COMMAND_INSTR(0x81, 0x02),
		ILI9881D_COMMAND_INSTR(0x82, 0x02),
		ILI9881D_COMMAND_INSTR(0x83, 0x02),
		ILI9881D_COMMAND_INSTR(0x84, 0x02),
		ILI9881D_COMMAND_INSTR(0x85, 0x02),
		ILI9881D_COMMAND_INSTR(0x86, 0x02),
		ILI9881D_COMMAND_INSTR(0x87, 0x02),
		ILI9881D_COMMAND_INSTR(0x88, 0x02),
		ILI9881D_COMMAND_INSTR(0x89, 0x02),
		ILI9881D_COMMAND_INSTR(0x8A, 0x02),
		ILI9881D_SWITCH_PAGE_INSTR(0x4),
		ILI9881D_COMMAND_INSTR(0x6D, 0x08),
		ILI9881D_COMMAND_INSTR(0x6F, 0x05),
		ILI9881D_COMMAND_INSTR(0x70, 0x00),
		ILI9881D_COMMAND_INSTR(0x71, 0x00),
		ILI9881D_COMMAND_INSTR(0x82, 0x0F),
		ILI9881D_COMMAND_INSTR(0x84, 0x0F),
		ILI9881D_COMMAND_INSTR(0x85, 0x0D),
		ILI9881D_COMMAND_INSTR(0x32, 0xAC),
		ILI9881D_COMMAND_INSTR(0x8C, 0x80),
		ILI9881D_COMMAND_INSTR(0x3C, 0xF5),
		ILI9881D_COMMAND_INSTR(0x3A, 0x24),
		ILI9881D_COMMAND_INSTR(0xB5, 0x07),
		ILI9881D_COMMAND_INSTR(0x31, 0x45),
		ILI9881D_COMMAND_INSTR(0x88, 0x33),
		ILI9881D_SWITCH_PAGE_INSTR(0x1),
		ILI9881D_COMMAND_INSTR(0x22, 0x09),
		ILI9881D_COMMAND_INSTR(0x31, 0x00),
		ILI9881D_COMMAND_INSTR(0x53, 0x8A),
		ILI9881D_COMMAND_INSTR(0x55, 0xA2),
		ILI9881D_COMMAND_INSTR(0x50, 0x81),
		ILI9881D_COMMAND_INSTR(0x51, 0x85),
		ILI9881D_COMMAND_INSTR(0x60, 0x20),
		ILI9881D_COMMAND_INSTR(0xA0, 0x00),
		ILI9881D_COMMAND_INSTR(0xA1, 0x1A),
		ILI9881D_COMMAND_INSTR(0xA2, 0x28),
		ILI9881D_COMMAND_INSTR(0xA3, 0x13),
		ILI9881D_COMMAND_INSTR(0xA4, 0x16),
		ILI9881D_COMMAND_INSTR(0xA5, 0x29),
		ILI9881D_COMMAND_INSTR(0xA6, 0x1D),
		ILI9881D_COMMAND_INSTR(0xA7, 0x1E),
		ILI9881D_COMMAND_INSTR(0xA8, 0x84),
		ILI9881D_COMMAND_INSTR(0xA9, 0x1C),
		ILI9881D_COMMAND_INSTR(0xAA, 0x28),
		ILI9881D_COMMAND_INSTR(0xAB, 0x75),
		ILI9881D_COMMAND_INSTR(0xAC, 0x1A),
		ILI9881D_COMMAND_INSTR(0xAD, 0x19),
		ILI9881D_COMMAND_INSTR(0xAE, 0x4D),
		ILI9881D_COMMAND_INSTR(0xAF, 0x22),
		ILI9881D_COMMAND_INSTR(0xB0, 0x28),
		ILI9881D_COMMAND_INSTR(0xB1, 0x54),
		ILI9881D_COMMAND_INSTR(0xB2, 0x66),
		ILI9881D_COMMAND_INSTR(0xB3, 0x39),
		ILI9881D_COMMAND_INSTR(0xC0, 0x00),
		ILI9881D_COMMAND_INSTR(0xC1, 0x1A),
		ILI9881D_COMMAND_INSTR(0xC2, 0x28),
		ILI9881D_COMMAND_INSTR(0xC3, 0x13),
		ILI9881D_COMMAND_INSTR(0xC4, 0x16),
		ILI9881D_COMMAND_INSTR(0xC5, 0x29),
		ILI9881D_COMMAND_INSTR(0xC6, 0x1D),
		ILI9881D_COMMAND_INSTR(0xC7, 0x1E),
		ILI9881D_COMMAND_INSTR(0xC8, 0x84),
		ILI9881D_COMMAND_INSTR(0xC9, 0x1C),
		ILI9881D_COMMAND_INSTR(0xCA, 0x28),
		ILI9881D_COMMAND_INSTR(0xCB, 0x75),
		ILI9881D_COMMAND_INSTR(0xCC, 0x1A),
		ILI9881D_COMMAND_INSTR(0xCD, 0x19),
		ILI9881D_COMMAND_INSTR(0xCE, 0x4D),
		ILI9881D_COMMAND_INSTR(0xCF, 0x22),
		ILI9881D_COMMAND_INSTR(0xD0, 0x28),
		ILI9881D_COMMAND_INSTR(0xD1, 0x54),
		ILI9881D_COMMAND_INSTR(0xD2, 0x66),
		ILI9881D_COMMAND_INSTR(0xD3, 0x39),
		//ILI9881D_SWITCH_PAGE_INSTR(0x0),
		//ILI9881D_COMMAND_INSTR(0x35, 0x00),
		//ILI9881D_COMMAND_INSTR(0x36, 0x03),
		//ILI9881D_COMMAND_INSTR(0x11, 0x00),
		//ILI9881D_COMMAND_INSTR(0x29, 0x00),
};
#endif

#if 0
static const struct ili9881d_instr ili9881d_init[] = {
	ILI9881D_SWITCH_PAGE_INSTR(3),
	ILI9881D_COMMAND_INSTR(0x01, 0x00),
	ILI9881D_COMMAND_INSTR(0x02, 0x00),
	ILI9881D_COMMAND_INSTR(0x03, 0x73),
	ILI9881D_COMMAND_INSTR(0x04, 0x00),
	ILI9881D_COMMAND_INSTR(0x05, 0x00),
	ILI9881D_COMMAND_INSTR(0x06, 0x08),
	ILI9881D_COMMAND_INSTR(0x07, 0x00),
	ILI9881D_COMMAND_INSTR(0x08, 0x00),
	ILI9881D_COMMAND_INSTR(0x09, 0x00),
	ILI9881D_COMMAND_INSTR(0x0A, 0x01),
	ILI9881D_COMMAND_INSTR(0x0B, 0x01),
	ILI9881D_COMMAND_INSTR(0x0C, 0x00),
	ILI9881D_COMMAND_INSTR(0x0D, 0x01),
	ILI9881D_COMMAND_INSTR(0x0E, 0x01),
	ILI9881D_COMMAND_INSTR(0x0F, 0x00),
	ILI9881D_COMMAND_INSTR(0x10, 0x00),
	ILI9881D_COMMAND_INSTR(0x11, 0x00),
	ILI9881D_COMMAND_INSTR(0x12, 0x00),
	ILI9881D_COMMAND_INSTR(0x13, 0x00),
	ILI9881D_COMMAND_INSTR(0x14, 0x00),
	ILI9881D_COMMAND_INSTR(0x15, 0x00),
	ILI9881D_COMMAND_INSTR(0x16, 0x00),
	ILI9881D_COMMAND_INSTR(0x17, 0x00),
	ILI9881D_COMMAND_INSTR(0x18, 0x00),
	ILI9881D_COMMAND_INSTR(0x19, 0x00),
	ILI9881D_COMMAND_INSTR(0x1A, 0x00),
	ILI9881D_COMMAND_INSTR(0x1B, 0x00),
	ILI9881D_COMMAND_INSTR(0x1C, 0x00),
	ILI9881D_COMMAND_INSTR(0x1D, 0x00),
	ILI9881D_COMMAND_INSTR(0x1E, 0x40),
	ILI9881D_COMMAND_INSTR(0x1F, 0xC0),
	ILI9881D_COMMAND_INSTR(0x20, 0x06),
	ILI9881D_COMMAND_INSTR(0x21, 0x01),
	ILI9881D_COMMAND_INSTR(0x22, 0x06),
	ILI9881D_COMMAND_INSTR(0x23, 0x01),
	ILI9881D_COMMAND_INSTR(0x24, 0x88),
	ILI9881D_COMMAND_INSTR(0x25, 0x88),
	ILI9881D_COMMAND_INSTR(0x26, 0x00),
	ILI9881D_COMMAND_INSTR(0x27, 0x00),
	ILI9881D_COMMAND_INSTR(0x28, 0x3B),
	ILI9881D_COMMAND_INSTR(0x29, 0x03),
	ILI9881D_COMMAND_INSTR(0x2A, 0x00),
	ILI9881D_COMMAND_INSTR(0x2B, 0x00),
	ILI9881D_COMMAND_INSTR(0x2C, 0x00),
	ILI9881D_COMMAND_INSTR(0x2D, 0x00),
	ILI9881D_COMMAND_INSTR(0x2E, 0x00),
	ILI9881D_COMMAND_INSTR(0x2F, 0x00),
	ILI9881D_COMMAND_INSTR(0x30, 0x00),
	ILI9881D_COMMAND_INSTR(0x31, 0x00),
	ILI9881D_COMMAND_INSTR(0x32, 0x00),
	ILI9881D_COMMAND_INSTR(0x33, 0x00),
	ILI9881D_COMMAND_INSTR(0x34, 0x00), /* GPWR1/2 non overlap time 2.62us */
	ILI9881D_COMMAND_INSTR(0x35, 0x00),
	ILI9881D_COMMAND_INSTR(0x36, 0x00),
	ILI9881D_COMMAND_INSTR(0x37, 0x00),
	ILI9881D_COMMAND_INSTR(0x38, 0x00),
	ILI9881D_COMMAND_INSTR(0x39, 0x00),
	ILI9881D_COMMAND_INSTR(0x3A, 0x00),
	ILI9881D_COMMAND_INSTR(0x3B, 0x00),
	ILI9881D_COMMAND_INSTR(0x3C, 0x00),
	ILI9881D_COMMAND_INSTR(0x3D, 0x00),
	ILI9881D_COMMAND_INSTR(0x3E, 0x00),
	ILI9881D_COMMAND_INSTR(0x3F, 0x00),
	ILI9881D_COMMAND_INSTR(0x40, 0x00),
	ILI9881D_COMMAND_INSTR(0x41, 0x00),
	ILI9881D_COMMAND_INSTR(0x42, 0x00),
	ILI9881D_COMMAND_INSTR(0x43, 0x00),
	ILI9881D_COMMAND_INSTR(0x44, 0x00),
	ILI9881D_COMMAND_INSTR(0x50, 0x01),
	ILI9881D_COMMAND_INSTR(0x51, 0x23),
	ILI9881D_COMMAND_INSTR(0x52, 0x45),
	ILI9881D_COMMAND_INSTR(0x53, 0x67),
	ILI9881D_COMMAND_INSTR(0x54, 0x89),
	ILI9881D_COMMAND_INSTR(0x55, 0xAB),
	ILI9881D_COMMAND_INSTR(0x56, 0x01),
	ILI9881D_COMMAND_INSTR(0x57, 0x23),
	ILI9881D_COMMAND_INSTR(0x58, 0x45),
	ILI9881D_COMMAND_INSTR(0x59, 0x67),
	ILI9881D_COMMAND_INSTR(0x5A, 0x89),
	ILI9881D_COMMAND_INSTR(0x5B, 0xAB),
	ILI9881D_COMMAND_INSTR(0x5C, 0xCD),
	ILI9881D_COMMAND_INSTR(0x5D, 0xEF),
	ILI9881D_COMMAND_INSTR(0x5E, 0x00),
	ILI9881D_COMMAND_INSTR(0x5F, 0x01),
	ILI9881D_COMMAND_INSTR(0x60, 0x01),
	ILI9881D_COMMAND_INSTR(0x61, 0x06),
	ILI9881D_COMMAND_INSTR(0x62, 0x06),
	ILI9881D_COMMAND_INSTR(0x63, 0x07),
	ILI9881D_COMMAND_INSTR(0x64, 0x07),
	ILI9881D_COMMAND_INSTR(0x65, 0x00),
	ILI9881D_COMMAND_INSTR(0x66, 0x00),
	ILI9881D_COMMAND_INSTR(0x67, 0x02),
	ILI9881D_COMMAND_INSTR(0x68, 0x02),
	ILI9881D_COMMAND_INSTR(0x69, 0x05),
	ILI9881D_COMMAND_INSTR(0x6A, 0x05),
	ILI9881D_COMMAND_INSTR(0x6B, 0x02),
	ILI9881D_COMMAND_INSTR(0x6C, 0x0D),
	ILI9881D_COMMAND_INSTR(0x6D, 0x0D),
	ILI9881D_COMMAND_INSTR(0x6E, 0x0C),
	ILI9881D_COMMAND_INSTR(0x6F, 0x0C),
	ILI9881D_COMMAND_INSTR(0x70, 0x0F),
	ILI9881D_COMMAND_INSTR(0x71, 0x0F),
	ILI9881D_COMMAND_INSTR(0x72, 0x0E),
	ILI9881D_COMMAND_INSTR(0x73, 0x0E),
	ILI9881D_COMMAND_INSTR(0x74, 0x02),
	ILI9881D_COMMAND_INSTR(0x75, 0x01),
	ILI9881D_COMMAND_INSTR(0x76, 0x01),
	ILI9881D_COMMAND_INSTR(0x77, 0x06),
	ILI9881D_COMMAND_INSTR(0x78, 0x06),
	ILI9881D_COMMAND_INSTR(0x79, 0x07),
	ILI9881D_COMMAND_INSTR(0x7A, 0x07),
	ILI9881D_COMMAND_INSTR(0x7B, 0x00),
	ILI9881D_COMMAND_INSTR(0x7C, 0x00),
	ILI9881D_COMMAND_INSTR(0x7D, 0x02),
	ILI9881D_COMMAND_INSTR(0x7E, 0x02),
	ILI9881D_COMMAND_INSTR(0x7F, 0x05),
	ILI9881D_COMMAND_INSTR(0x80, 0x05),
	ILI9881D_COMMAND_INSTR(0x81, 0x02),
	ILI9881D_COMMAND_INSTR(0x82, 0x0D),
	ILI9881D_COMMAND_INSTR(0x83, 0x0D),
	ILI9881D_COMMAND_INSTR(0x84, 0x0C),
	ILI9881D_COMMAND_INSTR(0x85, 0x0C),
	ILI9881D_COMMAND_INSTR(0x86, 0x0F),
	ILI9881D_COMMAND_INSTR(0x87, 0x0F),
	ILI9881D_COMMAND_INSTR(0x88, 0x0E),
	ILI9881D_COMMAND_INSTR(0x89, 0x0E),
	ILI9881D_COMMAND_INSTR(0x8A, 0x02),
	ILI9881D_SWITCH_PAGE_INSTR(4),
	ILI9881D_COMMAND_INSTR(0x3B, 0xC0), /* ILI4003D sel */
	ILI9881D_COMMAND_INSTR(0x6C, 0x15), /* Set VCORE voltage = 1.5V */
	ILI9881D_COMMAND_INSTR(0x6E, 0x2A), /* di_pwr_reg=0 for power mode 2A, VGH clamp 18V */
	ILI9881D_COMMAND_INSTR(0x6F, 0x33), /* pumping ratio VGH=5x VGL=-3x */
	ILI9881D_COMMAND_INSTR(0x8D, 0x1B), /* VGL clamp -10V */
	ILI9881D_COMMAND_INSTR(0x87, 0xBA), /* ESD */
	ILI9881D_COMMAND_INSTR(0x3A, 0x24), /* POWER SAVING */
	ILI9881D_COMMAND_INSTR(0x26, 0x76),
	ILI9881D_COMMAND_INSTR(0xB2, 0xD1),
	ILI9881D_SWITCH_PAGE_INSTR(1),
	ILI9881D_COMMAND_INSTR(0x22, 0x0A), /* BGR, SS */
	ILI9881D_COMMAND_INSTR(0x31, 0x00), /* Zigzag type3 inversion */
	ILI9881D_COMMAND_INSTR(0x40, 0x53), /* ILI4003D sel */
	ILI9881D_COMMAND_INSTR(0x43, 0x66),
	ILI9881D_COMMAND_INSTR(0x53, 0x4C),
	ILI9881D_COMMAND_INSTR(0x50, 0x87),
	ILI9881D_COMMAND_INSTR(0x51, 0x82),
	ILI9881D_COMMAND_INSTR(0x60, 0x15),
	ILI9881D_COMMAND_INSTR(0x61, 0x01),
	ILI9881D_COMMAND_INSTR(0x62, 0x0C),
	ILI9881D_COMMAND_INSTR(0x63, 0x00),
	ILI9881D_COMMAND_INSTR(0xA0, 0x00),
	ILI9881D_COMMAND_INSTR(0xA1, 0x13), /* VP251 */
	ILI9881D_COMMAND_INSTR(0xA2, 0x23), /* VP247 */
	ILI9881D_COMMAND_INSTR(0xA3, 0x14), /* VP243 */
	ILI9881D_COMMAND_INSTR(0xA4, 0x16), /* VP239 */
	ILI9881D_COMMAND_INSTR(0xA5, 0x29), /* VP231 */
	ILI9881D_COMMAND_INSTR(0xA6, 0x1E), /* VP219 */
	ILI9881D_COMMAND_INSTR(0xA7, 0x1D), /* VP203 */
	ILI9881D_COMMAND_INSTR(0xA8, 0x86), /* VP175 */
	ILI9881D_COMMAND_INSTR(0xA9, 0x1E), /* VP144 */
	ILI9881D_COMMAND_INSTR(0xAA, 0x29), /* VP111 */
	ILI9881D_COMMAND_INSTR(0xAB, 0x74), /* VP80 */
	ILI9881D_COMMAND_INSTR(0xAC, 0x19), /* VP52 */
	ILI9881D_COMMAND_INSTR(0xAD, 0x17), /* VP36 */
	ILI9881D_COMMAND_INSTR(0xAE, 0x4B), /* VP24 */
	ILI9881D_COMMAND_INSTR(0xAF, 0x20), /* VP16 */
	ILI9881D_COMMAND_INSTR(0xB0, 0x26), /* VP12 */
	ILI9881D_COMMAND_INSTR(0xB1, 0x4C), /* VP8 */
	ILI9881D_COMMAND_INSTR(0xB2, 0x5D), /* VP4 */
	ILI9881D_COMMAND_INSTR(0xB3, 0x3F), /* VP0 */
	ILI9881D_COMMAND_INSTR(0xC0, 0x00), /* VN255 GAMMA N */
	ILI9881D_COMMAND_INSTR(0xC1, 0x13), /* VN251 */
	ILI9881D_COMMAND_INSTR(0xC2, 0x23), /* VN247 */
	ILI9881D_COMMAND_INSTR(0xC3, 0x14), /* VN243 */
	ILI9881D_COMMAND_INSTR(0xC4, 0x16), /* VN239 */
	ILI9881D_COMMAND_INSTR(0xC5, 0x29), /* VN231 */
	ILI9881D_COMMAND_INSTR(0xC6, 0x1E), /* VN219 */
	ILI9881D_COMMAND_INSTR(0xC7, 0x1D), /* VN203 */
	ILI9881D_COMMAND_INSTR(0xC8, 0x86), /* VN175 */
	ILI9881D_COMMAND_INSTR(0xC9, 0x1E), /* VN144 */
	ILI9881D_COMMAND_INSTR(0xCA, 0x29), /* VN111 */
	ILI9881D_COMMAND_INSTR(0xCB, 0x74), /* VN80 */
	ILI9881D_COMMAND_INSTR(0xCC, 0x19), /* VN52 */
	ILI9881D_COMMAND_INSTR(0xCD, 0x17), /* VN36 */
	ILI9881D_COMMAND_INSTR(0xCE, 0x4B), /* VN24 */
	ILI9881D_COMMAND_INSTR(0xCF, 0x20), /* VN16 */
	ILI9881D_COMMAND_INSTR(0xD0, 0x26), /* VN12 */
	ILI9881D_COMMAND_INSTR(0xD1, 0x4C), /* VN8 */
	ILI9881D_COMMAND_INSTR(0xD2, 0x5D), /* VN4 */
	ILI9881D_COMMAND_INSTR(0xD3, 0x3F), /* VN0 */
};
#endif

static const struct ili9881d_instr ili9881d_init[] = {
	ILI9881D_SWITCH_PAGE_INSTR(3),
	ILI9881D_COMMAND_INSTR(0x01, 0x00),
	ILI9881D_COMMAND_INSTR(0x02, 0x00),
	ILI9881D_COMMAND_INSTR(0x03, 0x53),
	ILI9881D_COMMAND_INSTR(0x04, 0x53),
	ILI9881D_COMMAND_INSTR(0x05, 0x13),
	ILI9881D_COMMAND_INSTR(0x06, 0x04),
	ILI9881D_COMMAND_INSTR(0x07, 0x02),
	ILI9881D_COMMAND_INSTR(0x08, 0x02),
	ILI9881D_COMMAND_INSTR(0x09, 0x00),
	ILI9881D_COMMAND_INSTR(0x0A, 0x00),
	ILI9881D_COMMAND_INSTR(0x0B, 0x00),
	ILI9881D_COMMAND_INSTR(0x0C, 0x00),
	ILI9881D_COMMAND_INSTR(0x0D, 0x00),
	ILI9881D_COMMAND_INSTR(0x0E, 0x00),
	ILI9881D_COMMAND_INSTR(0x0F, 0x00),

	ILI9881D_COMMAND_INSTR(0x10, 0x00),
	ILI9881D_COMMAND_INSTR(0x11, 0x00),
	ILI9881D_COMMAND_INSTR(0x12, 0x00),
	ILI9881D_COMMAND_INSTR(0x13, 0x00),
	ILI9881D_COMMAND_INSTR(0x14, 0x00),
	ILI9881D_COMMAND_INSTR(0x15, 0x08),
	ILI9881D_COMMAND_INSTR(0x16, 0x10),
	ILI9881D_COMMAND_INSTR(0x17, 0x00),
	ILI9881D_COMMAND_INSTR(0x18, 0x08),
	ILI9881D_COMMAND_INSTR(0x19, 0x00),
	ILI9881D_COMMAND_INSTR(0x1A, 0x00),
	ILI9881D_COMMAND_INSTR(0x1B, 0x00),
	ILI9881D_COMMAND_INSTR(0x1C, 0x00),
	ILI9881D_COMMAND_INSTR(0x1D, 0x00),
	ILI9881D_COMMAND_INSTR(0x1E, 0xC0),
	ILI9881D_COMMAND_INSTR(0x1F, 0x80),

	ILI9881D_COMMAND_INSTR(0x20, 0x02),
	ILI9881D_COMMAND_INSTR(0x21, 0x09),
	ILI9881D_COMMAND_INSTR(0x22, 0x00),
	ILI9881D_COMMAND_INSTR(0x23, 0x00),
	ILI9881D_COMMAND_INSTR(0x24, 0x00),
	ILI9881D_COMMAND_INSTR(0x25, 0x00),
	ILI9881D_COMMAND_INSTR(0x26, 0x00),
	ILI9881D_COMMAND_INSTR(0x27, 0x00),
	ILI9881D_COMMAND_INSTR(0x28, 0x55),
	ILI9881D_COMMAND_INSTR(0x29, 0x03),
	ILI9881D_COMMAND_INSTR(0x2A, 0x00),
	ILI9881D_COMMAND_INSTR(0x2B, 0x00),
	ILI9881D_COMMAND_INSTR(0x2C, 0x00),
	ILI9881D_COMMAND_INSTR(0x2D, 0x00),
	ILI9881D_COMMAND_INSTR(0x2E, 0x00),
	ILI9881D_COMMAND_INSTR(0x2F, 0x00),

	ILI9881D_COMMAND_INSTR(0x30, 0x00),
	ILI9881D_COMMAND_INSTR(0x31, 0x00),
	ILI9881D_COMMAND_INSTR(0x32, 0x00),
	ILI9881D_COMMAND_INSTR(0x33, 0x00),
	ILI9881D_COMMAND_INSTR(0x34, 0x04),
	ILI9881D_COMMAND_INSTR(0x35, 0x05),
	ILI9881D_COMMAND_INSTR(0x36, 0x05),
	ILI9881D_COMMAND_INSTR(0x37, 0x00),
	ILI9881D_COMMAND_INSTR(0x38, 0x3C),
	ILI9881D_COMMAND_INSTR(0x39, 0x35),
	ILI9881D_COMMAND_INSTR(0x3A, 0x00),
	ILI9881D_COMMAND_INSTR(0x3B, 0x40),
	ILI9881D_COMMAND_INSTR(0x3C, 0x00),
	ILI9881D_COMMAND_INSTR(0x3D, 0x00),
	ILI9881D_COMMAND_INSTR(0x3E, 0x00),
	ILI9881D_COMMAND_INSTR(0x3F, 0x00),

	ILI9881D_COMMAND_INSTR(0x40, 0x00),
	ILI9881D_COMMAND_INSTR(0x41, 0x88),
	ILI9881D_COMMAND_INSTR(0x42, 0x00),
	ILI9881D_COMMAND_INSTR(0x43, 0x00),
	ILI9881D_COMMAND_INSTR(0x44, 0x1F),

	ILI9881D_COMMAND_INSTR(0x50, 0x01),
	ILI9881D_COMMAND_INSTR(0x51, 0x23),
	ILI9881D_COMMAND_INSTR(0x52, 0x45),
	ILI9881D_COMMAND_INSTR(0x53, 0x67),
	ILI9881D_COMMAND_INSTR(0x54, 0x89),
	ILI9881D_COMMAND_INSTR(0x55, 0xaB),
	ILI9881D_COMMAND_INSTR(0x56, 0x01),
	ILI9881D_COMMAND_INSTR(0x57, 0x23),
	ILI9881D_COMMAND_INSTR(0x58, 0x45),
	ILI9881D_COMMAND_INSTR(0x59, 0x67),
	ILI9881D_COMMAND_INSTR(0x5A, 0x89),
	ILI9881D_COMMAND_INSTR(0x5B, 0xAB),
	ILI9881D_COMMAND_INSTR(0x5C, 0xCD),
	ILI9881D_COMMAND_INSTR(0x5D, 0xEF),
	ILI9881D_COMMAND_INSTR(0x5E, 0x03),
	ILI9881D_COMMAND_INSTR(0x5F, 0x14),

	ILI9881D_COMMAND_INSTR(0x60, 0x15),
	ILI9881D_COMMAND_INSTR(0x61, 0x0C),
	ILI9881D_COMMAND_INSTR(0x62, 0x0D),
	ILI9881D_COMMAND_INSTR(0x63, 0x0E),
	ILI9881D_COMMAND_INSTR(0x64, 0x0F),
	ILI9881D_COMMAND_INSTR(0x65, 0x10),
	ILI9881D_COMMAND_INSTR(0x66, 0x11),
	ILI9881D_COMMAND_INSTR(0x67, 0x08),
	ILI9881D_COMMAND_INSTR(0x68, 0x02),
	ILI9881D_COMMAND_INSTR(0x69, 0x0A),
	ILI9881D_COMMAND_INSTR(0x6A, 0x02),
	ILI9881D_COMMAND_INSTR(0x6B, 0x02),
	ILI9881D_COMMAND_INSTR(0x6C, 0x02),
	ILI9881D_COMMAND_INSTR(0x6D, 0x02),
	ILI9881D_COMMAND_INSTR(0x6E, 0x02),
	ILI9881D_COMMAND_INSTR(0x6F, 0x02),

	ILI9881D_COMMAND_INSTR(0x70, 0x02),
	ILI9881D_COMMAND_INSTR(0x71, 0x02),
	ILI9881D_COMMAND_INSTR(0x72, 0x06),
	ILI9881D_COMMAND_INSTR(0x73, 0x02),
	ILI9881D_COMMAND_INSTR(0x74, 0x02),
	ILI9881D_COMMAND_INSTR(0x75, 0x14),
	ILI9881D_COMMAND_INSTR(0x76, 0x15),
	ILI9881D_COMMAND_INSTR(0x77, 0x0F),
	ILI9881D_COMMAND_INSTR(0x78, 0x0E),
	ILI9881D_COMMAND_INSTR(0x79, 0x0D),
	ILI9881D_COMMAND_INSTR(0x7A, 0x0C),
	ILI9881D_COMMAND_INSTR(0x7B, 0x11),
	ILI9881D_COMMAND_INSTR(0x7C, 0x10),
	ILI9881D_COMMAND_INSTR(0x7D, 0x06),
	ILI9881D_COMMAND_INSTR(0x7E, 0x02),
	ILI9881D_COMMAND_INSTR(0x7F, 0x0A),

	ILI9881D_COMMAND_INSTR(0x80, 0x02),
	ILI9881D_COMMAND_INSTR(0x81, 0x02),
	ILI9881D_COMMAND_INSTR(0x82, 0x02),
	ILI9881D_COMMAND_INSTR(0x83, 0x02),
	ILI9881D_COMMAND_INSTR(0x84, 0x02),
	ILI9881D_COMMAND_INSTR(0x85, 0x02),
	ILI9881D_COMMAND_INSTR(0x86, 0x02),
	ILI9881D_COMMAND_INSTR(0x87, 0x02),
	ILI9881D_COMMAND_INSTR(0x88, 0x08),
	ILI9881D_COMMAND_INSTR(0x89, 0x02),
	ILI9881D_COMMAND_INSTR(0x8A, 0x02),

	ILI9881D_SWITCH_PAGE_INSTR(4),
	ILI9881D_COMMAND_INSTR(0x00, 0x80),
	ILI9881D_COMMAND_INSTR(0x70, 0x00),
	ILI9881D_COMMAND_INSTR(0x71, 0x00),
	ILI9881D_COMMAND_INSTR(0x66, 0xFE),
	ILI9881D_COMMAND_INSTR(0x82, 0x15),
	ILI9881D_COMMAND_INSTR(0x84, 0x15),
	ILI9881D_COMMAND_INSTR(0x85, 0x15),
	ILI9881D_COMMAND_INSTR(0x3a, 0x24),
	ILI9881D_COMMAND_INSTR(0x32, 0xAC),
	ILI9881D_COMMAND_INSTR(0x8C, 0x80),
	ILI9881D_COMMAND_INSTR(0x3C, 0xF5),
	ILI9881D_COMMAND_INSTR(0x88, 0x33),

	ILI9881D_SWITCH_PAGE_INSTR(1),
	ILI9881D_COMMAND_INSTR(0x22, 0x0A),
	ILI9881D_COMMAND_INSTR(0x31, 0x00),
	ILI9881D_COMMAND_INSTR(0x53, 0x78),
	ILI9881D_COMMAND_INSTR(0x50, 0x5B),
	ILI9881D_COMMAND_INSTR(0x51, 0x5B),
	ILI9881D_COMMAND_INSTR(0x60, 0x20),
	ILI9881D_COMMAND_INSTR(0x61, 0x00),
	ILI9881D_COMMAND_INSTR(0x62, 0x0D),
	ILI9881D_COMMAND_INSTR(0x63, 0x00),

	ILI9881D_COMMAND_INSTR(0xA0, 0x00),
	ILI9881D_COMMAND_INSTR(0xA1, 0x10),
	ILI9881D_COMMAND_INSTR(0xA2, 0x1C),
	ILI9881D_COMMAND_INSTR(0xA3, 0x13),
	ILI9881D_COMMAND_INSTR(0xA4, 0x15),
	ILI9881D_COMMAND_INSTR(0xA5, 0x26),
	ILI9881D_COMMAND_INSTR(0xA6, 0x1A),
	ILI9881D_COMMAND_INSTR(0xA7, 0x1D),
	ILI9881D_COMMAND_INSTR(0xA8, 0x67),
	ILI9881D_COMMAND_INSTR(0xA9, 0x1C),
	ILI9881D_COMMAND_INSTR(0xAA, 0x29),
	ILI9881D_COMMAND_INSTR(0xAB, 0x5B),
	ILI9881D_COMMAND_INSTR(0xAC, 0x26),
	ILI9881D_COMMAND_INSTR(0xAD, 0x28),
	ILI9881D_COMMAND_INSTR(0xAE, 0x5C),
	ILI9881D_COMMAND_INSTR(0xAF, 0x30),
	ILI9881D_COMMAND_INSTR(0xB0, 0x31),
	ILI9881D_COMMAND_INSTR(0xB1, 0x2E),
	ILI9881D_COMMAND_INSTR(0xB2, 0x32),
	ILI9881D_COMMAND_INSTR(0xB3, 0x00),

	ILI9881D_COMMAND_INSTR(0xC0, 0x00),
	ILI9881D_COMMAND_INSTR(0xC1, 0x10),
	ILI9881D_COMMAND_INSTR(0xC2, 0x1C),
	ILI9881D_COMMAND_INSTR(0xC3, 0x13),
	ILI9881D_COMMAND_INSTR(0xC4, 0x15),
	ILI9881D_COMMAND_INSTR(0xC5, 0x26),
	ILI9881D_COMMAND_INSTR(0xC6, 0x1A),
	ILI9881D_COMMAND_INSTR(0xC7, 0x1D),
	ILI9881D_COMMAND_INSTR(0xC8, 0x67),
	ILI9881D_COMMAND_INSTR(0xC9, 0x1C),
	ILI9881D_COMMAND_INSTR(0xCA, 0x29),
	ILI9881D_COMMAND_INSTR(0xCB, 0x5B),
	ILI9881D_COMMAND_INSTR(0xCC, 0x26),
	ILI9881D_COMMAND_INSTR(0xCD, 0x28),
	ILI9881D_COMMAND_INSTR(0xCE, 0x5C),
	ILI9881D_COMMAND_INSTR(0xCF, 0x30),
	ILI9881D_COMMAND_INSTR(0xD0, 0x31),
	ILI9881D_COMMAND_INSTR(0xD1, 0x2E),
	ILI9881D_COMMAND_INSTR(0xD2, 0x32),
	ILI9881D_COMMAND_INSTR(0xD3, 0x00),
	ILI9881D_SWITCH_PAGE_INSTR(0),
};



static inline struct ili9881d *panel_to_ili9881d(struct drm_panel *panel)
{
	return container_of(panel, struct ili9881d, panel);
}

/*
 * The panel seems to accept some private DCS commands that map
 * directly to registers.
 *
 * It is organised by page, with each page having its own set of
 * registers, and the first page looks like it's holding the standard
 * DCS commands.
 *
 * So before any attempt at sending a command or data, we have to be
 * sure if we're in the right page or not.
 */
static int ili9881d_switch_page(struct ili9881d *ctx, u8 page)
{
	u8 buf[4] = { 0xff, 0x98, 0x81, page };
	int ret;

	ret = mipi_dsi_dcs_write_buffer(ctx->dsi, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return 0;
}

static int ili9881d_send_cmd_data(struct ili9881d *ctx, u8 cmd, u8 data)
{
	u8 buf[2] = { cmd, data };
	u8 rbuf[2] = {0};
	int ret;

	ret = mipi_dsi_dcs_write_buffer(ctx->dsi, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	#if 1
	ret = mipi_dsi_dcs_read(ctx->dsi, cmd, rbuf, 1);
	if (ret > 0)
	{
		pr_info("ili9881 write cmd=0x%02x, data=0x%02x \n", cmd, data);
		pr_info("ili9881 read back cmd=0x%02x, data=0x%02x 0x%02x\n", cmd, rbuf[0], rbuf[1]);
	}
	#endif

	return 0;
}

static int ili9881d_readid(struct ili9881d *ctx)
{
	uint8_t read_buf[4] = {0};

	ili9881d_switch_page(ctx, 6);

	mipi_dsi_dcs_read(ctx->dsi, 0xF0, &read_buf[0], 1);
	mipi_dsi_dcs_read(ctx->dsi, 0xF1, &read_buf[1], 1);
	mipi_dsi_dcs_read(ctx->dsi, 0xF2, &read_buf[2], 1);

	if ((read_buf[0] == 0x98) && (read_buf[1] == 0x81) && (read_buf[2] == 0x0d)) 
	{
		pr_info("ili9881d read id successful! ID:%X-%X-%X\n",read_buf[0],read_buf[1],read_buf[2]);
		return 0;
	}

	pr_err("ili9881d read id failed!,%d,%d\n",read_buf[0],read_buf[1],read_buf[2]);
	return -1;
}

static int ili9881d_prepare(struct drm_panel *panel)
{
	struct ili9881d *ctx = panel_to_ili9881d(panel);
	unsigned int i;
	int ret;

	/* Power the panel */
	ret = regulator_enable(ctx->power);
	if (ret)
		return ret;
	msleep(10);

	/* And reset it */
	gpiod_set_value(ctx->reset, 1);
	msleep(20);
	gpiod_set_value(ctx->reset, 0);
	msleep(20);
	gpiod_set_value(ctx->reset, 1);
	msleep(120);

	ili9881d_readid(ctx);

	for (i = 0; i < ARRAY_SIZE(ili9881d_init); i++) {
		const struct ili9881d_instr *instr = &ili9881d_init[i];

		if (instr->op == ILI9881D_SWITCH_PAGE)
			ret = ili9881d_switch_page(ctx, instr->arg.page);
		else if (instr->op == ILI9881D_COMMAND){
			pr_debug("ili9811 send_data[0x%02x, 0x%02x]\n", instr->arg.cmd.cmd,
				instr->arg.cmd.data);
			ret = ili9881d_send_cmd_data(ctx, instr->arg.cmd.cmd,
						      instr->arg.cmd.data);
		}

		if (ret){
			pr_err("ili9811 send_cmd_data fail, %d\n", ret);
			return ret;
		}
	}

	ret = ili9881d_switch_page(ctx, 0);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_set_tear_on(ctx->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(ctx->dsi);
	if (ret)
		return ret;

	msleep(120);
        mipi_dsi_dcs_set_display_on(ctx->dsi); //set display on here, not do it in ili9881d_enable api
	msleep(10);

	return 0;
}

static int ili9881d_enable(struct drm_panel *panel)
{
	struct ili9881d *ctx = panel_to_ili9881d(panel);

	msleep(120);

	/*
	//dsi_mode had been switch to video_mode, so cmd will not be sent.
	mipi_dsi_dcs_set_display_on(ctx->dsi); 
	msleep(10);
	*/

	backlight_enable(ctx->backlight);
	pr_debug("ili9881 enable\n");

	return 0;
}

static int ili9881d_disable(struct drm_panel *panel)
{
	struct ili9881d *ctx = panel_to_ili9881d(panel);
	pr_debug("ili9881 disable\n");
	backlight_disable(ctx->backlight);
	return mipi_dsi_dcs_set_display_off(ctx->dsi);
}

static int ili9881d_unprepare(struct drm_panel *panel)
{
	struct ili9881d *ctx = panel_to_ili9881d(panel);

	mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
	regulator_disable(ctx->power);
	gpiod_set_value(ctx->reset, 0);

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock		= 62000,
	.vrefresh	= 60,

	.hdisplay	= 720,
	.hsync_start	= 720 + 60,
	.hsync_end	= 720 + 60 + 20,
	.htotal		= 720 + 60 + 20 + 90,

	.vdisplay	= 1280,
	.vsync_start	= 1280 + 24,
	.vsync_end	= 1280 + 24 + 4,
	.vtotal		= 1280 + 24 + 4 + 24,
};


static int ili9881d_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct ili9881d *ctx = panel_to_ili9881d(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(&ctx->dsi->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay,
			default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	panel->connector->display_info.width_mm = mode->width_mm;// 62;
	panel->connector->display_info.height_mm = mode->height_mm;//110;

	return 1;
}

static const struct drm_panel_funcs ili9881d_funcs = {
	.prepare	= ili9881d_prepare,
	.unprepare	= ili9881d_unprepare,
	.enable		= ili9881d_enable,
	.disable	= ili9881d_disable,
	.get_modes	= ili9881d_get_modes,
};


#ifdef CONFIG_ILI9881D_DEBUG_FS
static u8 regaddr = 0;

static int ili9881_control_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "enable\n"
				  "prepare\n"
				  "start\n");
	return 0;
}

static int ili9881_control_open(struct inode *inode, struct file *file)
{
	return single_open(file, ili9881_control_show, inode->i_private);
}

static ssize_t ili9881_control_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct ili9881d *ctx = s->private;
	char			buf[32] = {};

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "enable", 6)){
		//mode = DWC3_GCTL_PRTCAP_HOST;
		ili9881d_enable(&ctx->panel);
	}
	else if (!strncmp(buf, "disable", 7)){
		ili9881d_disable(&ctx->panel);
	}
	else if (!strncmp(buf, "start", 5)){
		ili9881d_prepare(&ctx->panel);
	}

	return count;
}

static const struct file_operations ili9881_control_fops = {
	.open			= ili9881_control_open,
	.write			= ili9881_control_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int ili9881_reg_show(struct seq_file *s, void *unused)
{
	struct ili9881d *ctx = s->private;
	u8 rbuf[2] = {0};

	mipi_dsi_dcs_read(ctx->dsi, regaddr, rbuf, 1);
	seq_printf(s, "cmd=0x%02x, val=0x%02x\n", regaddr, rbuf[0]);
    
	return 0;
}

static int ili9881_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, ili9881_reg_show, inode->i_private);
}

static ssize_t ili9881_reg_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct ili9881d *ctx = s->private;
	u8			cmd = 0;
	char			buf[32] = {};

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	
	if (!strncmp(buf, "page", 4)){
		//mode = DWC3_GCTL_PRTCAP_HOST;
		sscanf((buf+4), "%x", &cmd);
		pr_info("ili9881 switch page %d\n", cmd);
		ili9881d_switch_page(ctx, cmd);
	}
	else if (!strncmp(buf, "reg", 3)){
		sscanf((buf+3), "%x", &cmd);
		regaddr = cmd;
		pr_warn("ili9881 set regcmd = 0x%02x\n", cmd);
	} else if (!strncmp(buf, "write", 5)){
		u8 data;
		sscanf((buf+5), "%x %x", &cmd, &data);
		u8 buf[2] = {cmd, data};
		mipi_dsi_dcs_write_buffer(ctx->dsi, buf, sizeof(buf));
	}

	return count;
}


static const struct file_operations ili9881_reg_fops = {
	.open			= ili9881_reg_open,
	.write			= ili9881_reg_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

struct dentry *ili9881_root = NULL;
static int ili9881_debugfs_init(struct ili9881d *ctx)
{
	ili9881_root = debugfs_create_dir("dsi_panel", NULL);

	if (ili9881_root){
		debugfs_create_file("control", S_IRUGO | S_IWUSR, ili9881_root, ctx,
			    	&ili9881_control_fops);
		debugfs_create_file("reg", S_IRUGO | S_IWUSR, ili9881_root, ctx,
			    	&ili9881_reg_fops);
	}
	return 0;
}

#endif

static int ili9881d_dsi_probe(struct mipi_dsi_device *dsi)
{
	//struct device_node *np;
	struct ili9881d *ctx;
	int ret;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->dsi = dsi;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = &dsi->dev;
	ctx->panel.funcs = &ili9881d_funcs;

	ctx->power = devm_regulator_get(&dsi->dev, "power");
	if (IS_ERR(ctx->power)) {
		dev_err(&dsi->dev, "Couldn't get our power regulator\n");
		return PTR_ERR(ctx->power);
	}

	ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(ctx->reset);
	}

	ili9881_debugfs_init(ctx);
	pr_info("ili9881_debugfs_init successful\n");

	//np = of_parse_phandle(dsi->dev.of_node, "backlight", 0);
	//if (np) {
	//	ctx->backlight = of_find_backlight_by_node(np);
//		of_node_put(np);
//
//		if (!ctx->backlight)
//			return -EPROBE_DEFER;
//	}

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0)
		return ret;

	//dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
				
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST
			 	| MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET;

	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	return mipi_dsi_attach(dsi);
}

static int ili9881d_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct ili9881d *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	if (ctx->backlight)
		put_device(&ctx->backlight->dev);

	return 0;
}

static const struct of_device_id ili9881d_of_match[] = {
	{ .compatible = "ilitek,ili9881d" },
	{ }
};
MODULE_DEVICE_TABLE(of, ili9881d_of_match);

static struct mipi_dsi_driver ili9881d_dsi_driver = {
	.probe		= ili9881d_dsi_probe,
	.remove		= ili9881d_dsi_remove,
	.driver = {
		.name		= "ili9881d-dsi",
		.of_match_table	= ili9881d_of_match,
	},
};
module_mipi_dsi_driver(ili9881d_dsi_driver);

MODULE_AUTHOR("johnson.qiao@quectel.com");
MODULE_DESCRIPTION("Ilitek ILI9881D Controller Driver");
MODULE_LICENSE("GPL v2");
