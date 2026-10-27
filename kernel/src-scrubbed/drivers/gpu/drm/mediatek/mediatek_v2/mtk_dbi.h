/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#ifndef _MTK_DBI_H
#define _MTK_DBI_H

#include <linux/types.h>

typedef enum {
	MTK_DBI_LCM_CTRL_TYPE_NONE = 0,
	MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI, // DBI-B
	MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI,   // DBI-C
} mtk_dbi_lcm_ctrl_type;

typedef enum {
	MTK_DBI_LCM_CLOCK_FREQ_26M,
	MTK_DBI_LCM_CLOCK_FREQ_78M,
	MTK_DBI_LCM_CLOCK_FREQ_91M,
	MTK_DBI_LCM_CLOCK_FREQ_104M,
	MTK_DBI_LCM_CLOCK_FREQ_125M,
	MTK_DBI_LCM_CLOCK_FREQ_137M,
	MTK_DBI_LCM_CLOCK_FREQ_156M,
	MTK_DBI_LCM_CLOCK_FREQ_182M,
} mtk_dbi_lcm_clock_freq;

typedef enum {  //Need to confirm definiation.
	MTK_DBI_LCM_DRIVING_CURRENT_DEFAULT,
	MTK_DBI_LCM_DRIVING_CURRENT_2MA = 0,
	MTK_DBI_LCM_DRIVING_CURRENT_4MA = 1,
	MTK_DBI_LCM_DRIVING_CURRENT_6MA = 2,
	MTK_DBI_LCM_DRIVING_CURRENT_8MA = 3,
	MTK_DBI_LCM_DRIVING_CURRENT_10MA = 4,
	MTK_DBI_LCM_DRIVING_CURRENT_12MA = 5,
	MTK_DBI_LCM_DRIVING_CURRENT_14MA = 6,
	MTK_DBI_LCM_DRIVING_CURRENT_16MA = 7,
	MTK_DBI_LCM_DRIVING_CURRENT_20MA = 8,
	MTK_DBI_LCM_DRIVING_CURRENT_24MA = 9,
	MTK_DBI_LCM_DRIVING_CURRENT_28MA = 10,
	MTK_DBI_LCM_DRIVING_CURRENT_32MA = 11,
} mtk_dbi_lcm_driving_current;

typedef enum {
	MTK_DBI_LCM_COLOR_ORDER_RGB = 0,
	MTK_DBI_LCM_COLOR_ORDER_BGR = 1
} mtk_dbi_lcm_color_order;

typedef enum {
	MTK_DBI_LCM_COLOR_FORMAT_RGB332 = 0,
	MTK_DBI_LCM_COLOR_FORMAT_RGB444 = 1,
	MTK_DBI_LCM_COLOR_FORMAT_RGB565 = 2,
	MTK_DBI_LCM_COLOR_FORMAT_RGB666 = 3,
	MTK_DBI_LCM_COLOR_FORMAT_RGB888 = 4
} mtk_dbi_lcm_color_format;

//INTERFACE_SIZE
//00: 8 bits, 01: 16 bits, 10: 9 bits, 11: 24bits
typedef enum {
	MTK_DBI_LCM_INTERFACE_SIZE_8BITS = 0,
	MTK_DBI_LCM_INTERFACE_SIZE_9BITS = 2,
	MTK_DBI_LCM_INTERFACE_SIZE_16BITS = 1,
	MTK_DBI_LCM_INTERFACE_SIZE_18BITS = 3,
} mtk_dbi_lcm_interface_size;

//Wire number
typedef enum {
	MTK_DBI_LCM_INTERFACE_3WIRE = 0,
	MTK_DBI_LCM_INTERFACE_4WIRE = 1,
} mtk_dbi_lcm_interface_wire;

typedef enum {
	MTK_DBI_LCM_DATA_WIDTH_8BITS = 0,
	MTK_DBI_LCM_DATA_WIDTH_9BITS = 1,
	MTK_DBI_LCM_DATA_WIDTH_16BITS = 2,
	MTK_DBI_LCM_DATA_WIDTH_18BITS = 3,
	MTK_DBI_LCM_DATA_WIDTH_24BITS = 4,
	MTK_DBI_LCM_DATA_WIDTH_32BITS = 5,
} mtk_dbi_lcm_data_width;

struct mipi_dbi_host_ops {
	void (*send_cmd)(struct drm_panel *panel, unsigned int cmd);
	void (*send_data)(struct drm_panel *panel, unsigned int data);
	unsigned int (*read_data)(struct drm_panel *panel);
	void (*occupy_data_pin)(struct drm_panel *panel);   //Only used under DBI-B mode
	void (*release_data_pin)(struct drm_panel *panel);  //Only used under DBI-B mode
};

struct mipi_dbi_b_rw_timing {
	int cs_to_read_hold_time;   // C2RH
	int cs_to_read_setup_time;  // C2RS, (C2RS must <= RLT)
	int read_latency_time;      // RLT
	int cs_to_write_hold_time;  // C2HS
	int cs_to_write_setup_time; // C2WS, (C2WS must <=WST)
	int write_wait_state_time;  // WST
	int cs_remain_high_time;    // CHW, (configure in DBI_PDW)
};

struct mipi_dbi_c_rw_timing {
	int cs_setup_time;  // Chip selection setup time
	int cs_hold_time;   // Chip selection hold time
	int read_1st;       // The first phase timing of LSCK when read transfer
	int read_2nd;       // The second phase timing of LSCK when read transfer
	int write_1st;      // The first phase timing of LSCK when write transfer
	int write_2nd;      // The second phase timing of LSCK when write transfer

	// Slow down the serial interface timing
	// 0: Disable
	// 1: Enable
	int slow_down;

	// The default value of LSCK for serial interface when not transfer data
	// 0: The default of LSCK is low
	// 1: The default of LSCK is high
	int lsck_default;

	// The first phase polarity of LSCK for serial interface 0
	// 0: The first phase of LSCK is low
	// 1: The first phase of LSCK is high
	int lsck_1st_pol;

	// Set to 1 to read data from LSDI pin
	// otherwise LCD will use the bi-directional LSDA pin
	int read_data_from_sdi;
};

struct mipi_dbi_drving_current {
	mtk_dbi_lcm_driving_current group1;  // for PAD_LPA0,PAD_LSDI
	mtk_dbi_lcm_driving_current group2;  // for PAD_LPCE0B,PAD_LPRDB,PAD_LPRSTB,PAD_LPWRB
	mtk_dbi_lcm_driving_current group3;  // for PAD_NLD2,PAD_NLD4,PAD_NLD6,PAD_NLD7
                                         // for PAD_NLD1,PAD_NLD3,PAD_NLD5,PAD_NLD0
};

struct mipi_dbi_te_control {
	// Software TE : Software emulated TE signal.
	// Write this bit from 0 to 1 will allow DBI to act like a TE
	// signal has been received.
	// This is only used when SYNC_MODE = 0.
	int software_te;

	// TE Repeat
	// 0: update LCM once every TE signal coming
	// 1: repeat updaing LCM after TE signal coming
	int te_repeat;

	// TE sync mode: Selects TE type to use
	// 0: DBI updates when a TE edge is detected and
	//    a specified delay has passed.
	// 1: DBI updates when software reads the current LCM scanline
	//    and DBI has counted from the current scanline the specified
	//    update scanline.
	int sync_mode;

	// Selects TE edge: Selects which edge is used to detect a TE signal
	// 0: Rising edge
	// 1: Falling edge
	int te_edge;

	// Enables sync: Enables/Disables DBI TE control
	// 0: Disable
	// 1: Enable
	int te_enable;
};

struct mipi_dbi_b_params {
	mtk_dbi_lcm_interface_size itf_size;

	struct mipi_dbi_b_rw_timing rw_timing;
};

struct mipi_dbi_c_params {
	mtk_dbi_lcm_interface_wire wire_num;

	struct mipi_dbi_c_rw_timing rw_timing;

	// Enable the CS_Stay_Low mode for frame pixel data transmission
	// in both 1-data-pin and 2-data-pin protocol
	// 0: Disable
	// 1: Enable
	int cs_stay_low;

	// Enable the Single A0 mode for frame pixel data transmission in 1-data-pin or
	// 2-data-pin protocol. This bit only takes effect when the 3-wire mode is enabled.
	// 0: Disable
	// 1: Enable
	int single_a0_mode;

	// Enable 2-data-pin protocol
	// 0: Disable
	// 1: Enable
	int two_data_pin_enable;

	// Interface size of Serial interface 0 in 2-data-pin protocol.
	// This size configuration takes effect only when data is actually
	// transmitted in 2-data-pin protocol. Each transaction would be
	// transmitted in bit specified as field description below.
	// 010: 16 bits
	// 011: 18 bits
	// 100: 24 bits
	// 110: 32 bits
	mtk_dbi_lcm_data_width two_data_pin_itf_size;
};

struct mipi_dbi_device {
	mtk_dbi_lcm_ctrl_type ctrl_type;
	mtk_dbi_lcm_clock_freq clk_freq;
	mtk_dbi_lcm_data_width data_width;
	mtk_dbi_lcm_color_order color_order;
	mtk_dbi_lcm_color_format color_format;

	struct mipi_dbi_drving_current driving_current;

	struct mipi_dbi_te_control te_control;

	struct mipi_dbi_b_params dbi_b_params; // DBI-B only
	struct mipi_dbi_c_params dbi_c_params; // DBI-C only
};

/* DBI GPIO pinctrl */
enum dbi_gpio {
	DBI_GPIO_DISABLE,
	DBI_GPIO_TYPE_B_9BITS,
	DBI_GPIO_TYPE_B_8BITS,
	DBI_GPIO_TYPE_C,
	DBI_GPIO_CO_NFI_DATA_PINS,
	DBI_GPIO_SEPARATE_DATA_PINS,
	DBI_GPIO_NUM
};

struct dbi_gpio_attr {
	const char *name;
	bool gpio_prepare;
	struct pinctrl_state *gpioctrl;
};

static struct dbi_gpio_attr dbi_gpios[DBI_GPIO_NUM] = {
	[DBI_GPIO_DISABLE] = {"default", false, NULL},
	[DBI_GPIO_TYPE_B_9BITS] = {"type_b_9bits", false, NULL},
	[DBI_GPIO_TYPE_B_8BITS] = {"type_b_8bits", false, NULL},
	[DBI_GPIO_TYPE_C] = {"type_c", false, NULL},
	[DBI_GPIO_CO_NFI_DATA_PINS] = {"co_nfi_data_pin", false, NULL},
	[DBI_GPIO_SEPARATE_DATA_PINS] = {"separate_data_pin", false, NULL},
};

void mtk_dbi_host_attach(struct drm_panel *panel,
		struct mipi_dbi_device *host, const struct mipi_dbi_host_ops **ops);

void mtk_dbi_host_detach(struct mipi_dbi_device *host);

#endif /* _MTK_DBI_H */
