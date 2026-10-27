// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/bits.h>
#include "soc_temp_lvts.h"
/*==================================================
 * LVTS v5 common definition or macro function
 *==================================================
 */
#define STOP_COUNTING (DEVICE_WRITE | RG_TSFM_CTRL_0 << 8 | 0x00)
#define SET_RG_TSFM_LPDLY (DEVICE_WRITE | RG_TSFM_CTRL_4 << 8 | 0xA6)
#define SET_COUNTING_WINDOW_20US1 (DEVICE_WRITE | RG_TSFM_CTRL_2 << 8 | 0x00)
#define SET_COUNTING_WINDOW_20US2 (DEVICE_WRITE | RG_TSFM_CTRL_1 << 8 | 0x20)
#define TSV2F_CHOP_CKSEL_AND_TSV2F_EN (DEVICE_WRITE | RG_TSV2F_CTRL_2 << 8	\
						| 0x8C)
#define TSBG_DEM_CKSEL_X_TSBG_CHOP_EN (DEVICE_WRITE | RG_TSV2F_CTRL_4 << 8	\
						| 0xFC)
#define SET_TS_RSV (DEVICE_WRITE | RG_TSV2F_CTRL_1 << 8 | 0x8D)
#define SET_TS_CHOP_CONTROL (DEVICE_WRITE | RG_TSV2F_CTRL_0 << 8 | 0xF1)
#define SET_LVTS_AUTO_RCK (DEVICE_WRITE | RG_TSV2F_CTRL_6 << 8 | 0x01)
#define SELECT_SENSOR_RCK(id) (DEVICE_WRITE | RG_TSV2F_CTRL_5 << 8 | id)
#define SET_DEVICE_SINGLE_MODE (DEVICE_WRITE | RG_TSFM_CTRL_3 << 8 | 0x78)
#define SET_TS_EN_AND_DIV_EN (DEVICE_WRITE | RG_TSV2F_CTRL_0 << 8 | 0xF5)
#define TOGGLE_VOC_RST1 (DEVICE_WRITE | RG_TSV2F_CTRL_0 << 8 | 0xFD)
#define TOGGLE_VOC_RST2 (DEVICE_WRITE | RG_TSV2F_CTRL_0 << 8 | 0xF5)
#define KICK_OFF_RCK_COUNTING (DEVICE_WRITE | RG_TSFM_CTRL_0 << 8 | 0x02)
#define DISABLE_TS_EN (DEVICE_WRITE | RG_TSV2F_CTRL_0 << 8 | 0xF1)
#define SET_SENSOR_NO_RCK (DEVICE_WRITE | RG_TSV2F_CTRL_5 << 8 | 0x10)
#define SET_DEVICE_LOW_POWER_SINGLE_MODE (DEVICE_WRITE | RG_TSFM_CTRL_3 << 8	\
						| 0xB8)
/*==================================================
 * LVTS MT6880 and MT6890
 *==================================================
 */
#define MT6880_NUM_LVTS (ARRAY_SIZE(mt6880_tc_settings))

enum mt6880_lvts_domain {
	MT6880_AP_DOMAIN,
	MT6880_MCU_DOMAIN,
	MT6880_NUM_DOMAIN
};

enum mt6880_lvts_sensor_enum {
	MT6880_TS1_0,
	MT6880_TS1_1,
	MT6880_TS1_2,
	MT6880_TS1_3,
	MT6880_TS2_0,
	MT6880_TS2_1,
	MT6880_TS2_2,
	MT6880_TS2_3,
	MT6880_TS3_0,
	MT6880_TS3_1,
	MT6880_TS3_2,
	MT6880_NUM_TS
};

static void mt6880_efuse_to_cal_data(struct lvts_data *lvts_data)
{
	struct sensor_cal_data *cal_data = &lvts_data->cal_data;

	cal_data->golden_temp = GET_CAL_DATA_BITMASK(2, 31, 24);
	cal_data->count_r[MT6880_TS1_0] = GET_CAL_DATA_BITMASK(0, 15, 0);
	cal_data->count_r[MT6880_TS1_1] = GET_CAL_DATA_BITMASK(0, 31, 16);
	cal_data->count_r[MT6880_TS1_2] = GET_CAL_DATA_BITMASK(1, 15, 0);
	cal_data->count_r[MT6880_TS1_3] = GET_CAL_DATA_BITMASK(1, 31, 16);
	cal_data->count_rc[MT6880_TS1_0] = GET_CAL_DATA_BITMASK(2, 23, 0);

	cal_data->count_r[MT6880_TS2_0] = GET_CAL_DATA_BITMASK(3, 15, 0);
	cal_data->count_r[MT6880_TS2_1] = GET_CAL_DATA_BITMASK(3, 31, 16);
	cal_data->count_r[MT6880_TS2_2] = GET_CAL_DATA_BITMASK(4, 15, 0);
	cal_data->count_r[MT6880_TS2_3] = GET_CAL_DATA_BITMASK(4, 31, 16);
	cal_data->count_rc[MT6880_TS2_0] = GET_CAL_DATA_BITMASK(5, 23, 0);

	cal_data->count_r[MT6880_TS3_0] = GET_CAL_DATA_BITMASK(6, 15, 0);
	cal_data->count_r[MT6880_TS3_1] = GET_CAL_DATA_BITMASK(6, 31, 16);
	cal_data->count_r[MT6880_TS3_2] = GET_CAL_DATA_BITMASK(7, 15, 0);
	cal_data->count_rc[MT6880_TS3_0] = GET_CAL_DATA_BITMASK(8, 23, 0);
}

static struct tc_settings mt6880_tc_settings[] = {
	[0] = {
		.domain_index = MT6880_MCU_DOMAIN,
		.addr_offset = 0x0,
		.num_sensor = 4,
		.sensor_map = {MT6880_TS1_0, MT6880_TS1_1, MT6880_TS1_2,
				MT6880_TS1_3},
		.tc_speed = SET_TC_SPEED_IN_US(118, 9333, 118, 118),
		.hw_filter = LVTS_FILTER_2_OF_4,
		.dominator_sensing_point = SENSING_POINT2,
		.hw_reboot_trip_point = 117000,
		.irq_bit = BIT(1),
	},
	[1] = {
		.domain_index = MT6880_AP_DOMAIN,
		.addr_offset = 0x0,
		.num_sensor = 4,
		.sensor_map = {MT6880_TS2_0, MT6880_TS2_1, MT6880_TS2_2,
				MT6880_TS2_3},
		.tc_speed = SET_TC_SPEED_IN_US(118, 9333, 118, 118),
		.hw_filter = LVTS_FILTER_2_OF_4,
		.dominator_sensing_point = SENSING_POINT0,
		.hw_reboot_trip_point = 117000,
		.irq_bit = BIT(1),
	},
	[2] = {
		.domain_index = MT6880_AP_DOMAIN,
		.addr_offset = 0x100,
		.num_sensor = 3,
		.sensor_map = {MT6880_TS3_0, MT6880_TS3_1, MT6880_TS3_2},
		.tc_speed = SET_TC_SPEED_IN_US(118, 9333, 118, 118),
		.hw_filter = LVTS_FILTER_2_OF_4,
		.dominator_sensing_point = SENSING_POINT1,
		.hw_reboot_trip_point = 117000,
		.irq_bit = BIT(2),
	}
};

static struct lvts_data mt6880_lvts_data = {
	.num_domain = MT6880_NUM_DOMAIN,
	.num_tc = MT6880_NUM_LVTS,
	.tc = mt6880_tc_settings,
	.num_sensor = MT6880_NUM_TS,
	.ops = {
		.efuse_to_cal_data = mt6880_efuse_to_cal_data,
	},
	.feature_bitmap = FEATURE_DEVICE_AUTO_RCK | FEATURE_CK26M_ACTIVE,
	.num_efuse_addr = 9,
	.num_efuse_block = 1,
	.cal_data = {
		.default_golden_temp = 50,
		.default_count_r = 35000,
		.default_count_rc = 2750,
	},
	.coeff = {
		.a = -250460,
		.b = 250460,
	},
};
/*==================================================
 * Platform data
 *==================================================
 */
static struct match_entry lvts_v5_match_table[] = {
	{
		.chip = "mt6880",
		.lvts_data = &mt6880_lvts_data,
	},
	{
	},
};
/*==================================================
 * LVTS v5 common code
 *==================================================
 */
static void device_enable_and_init_v5(struct lvts_data *lvts_data)
{
	unsigned int i;

	for (i = 0; i < lvts_data->num_tc; i++) {
		lvts_write_device(lvts_data, STOP_COUNTING,  i);
		lvts_write_device(lvts_data, SET_RG_TSFM_LPDLY,  i);
		lvts_write_device(lvts_data, SET_COUNTING_WINDOW_20US1,  i);
		lvts_write_device(lvts_data, SET_COUNTING_WINDOW_20US2,  i);
		lvts_write_device(lvts_data, TSV2F_CHOP_CKSEL_AND_TSV2F_EN,  i);
		lvts_write_device(lvts_data, TSBG_DEM_CKSEL_X_TSBG_CHOP_EN,  i);
		lvts_write_device(lvts_data, SET_TS_RSV,  i);
		lvts_write_device(lvts_data, SET_TS_CHOP_CONTROL,  i);
	}

	lvts_data->counting_window_us = 20;
}

static void device_enable_auto_rck_v5(struct lvts_data *lvts_data)
{
	unsigned int i;

	for (i = 0; i < lvts_data->num_tc; i++)
		lvts_write_device(lvts_data, SET_LVTS_AUTO_RCK,  i);
}

static int device_read_count_rc_n_v5(struct lvts_data *lvts_data)
{

	/* Resistor-Capacitor Calibration */
	/* count_RC_N: count RC now */
	struct device *dev = lvts_data->dev;
	struct tc_settings *tc = lvts_data->tc;
	struct sensor_cal_data *cal_data = &lvts_data->cal_data;
	unsigned int size, s_index, data;
	void __iomem *base;
	int i, j, offset;
	char buffer[512];

	cal_data->count_rc_now = devm_kcalloc(dev, lvts_data->num_sensor,
				      sizeof(*cal_data->count_rc_now), GFP_KERNEL);
	if (!cal_data->count_rc_now)
		return -ENOMEM;


	for (i = 0; i < lvts_data->num_tc; i++) {
		base = GET_BASE_ADDR(i);
		for (j = 0; j < tc[i].num_sensor; j++) {
			s_index = tc[i].sensor_map[j];

			lvts_write_device(lvts_data, SELECT_SENSOR_RCK(j),  i);
			lvts_write_device(lvts_data, SET_DEVICE_SINGLE_MODE,  i);
			lvts_write_device(lvts_data, SET_TS_EN_AND_DIV_EN,  i);
			lvts_write_device(lvts_data, TOGGLE_VOC_RST1,  i);
			lvts_write_device(lvts_data, TOGGLE_VOC_RST2,  i);
			udelay(10);

			lvts_write_device(lvts_data, KICK_OFF_RCK_COUNTING,  i);
			udelay(30);

			device_wait_counting_finished(lvts_data, i);

			lvts_write_device(lvts_data, DISABLE_TS_EN,  i);
			data = lvts_read_device(lvts_data, 0x00, i);

			cal_data->count_rc_now[s_index] = (data & GENMASK(23,0));
		}

		/* Recover Setting for Normal Access on
		 * temperature fetch
		 */
		lvts_write_device(lvts_data, SET_SENSOR_NO_RCK,  i);
		lvts_write_device(lvts_data, SET_DEVICE_LOW_POWER_SINGLE_MODE,  i);
	}

	size = sizeof(buffer);
	offset = snprintf(buffer, size, "[COUNT_RC_NOW] ");
	if (offset < 0) {
		dev_err(dev, "%s-%d:sprintf fail,ret=%d\n",
				__func__, __LINE__, offset);
		return -EINVAL;
	}
	for (i = 0; i < lvts_data->num_sensor; i++)
		offset += snprintf(buffer + offset, size - offset, "%d:%d ",
				i, cal_data->count_rc_now[i]);

	buffer[offset] = '\0';
	dev_info(dev, "%s\n", buffer);

	return 0;
}

static void set_calibration_data_v5(struct lvts_data *lvts_data)
{
	struct tc_settings *tc = lvts_data->tc;
	struct sensor_cal_data *cal_data = &lvts_data->cal_data;
	unsigned int i, j, s_index, e_data;
	void __iomem *base;

	for (i = 0; i < lvts_data->num_tc; i++) {
		base = GET_BASE_ADDR(i);

		for (j = 0; j < tc[i].num_sensor; j++) {
			s_index = tc[i].sensor_map[j];
			if (IS_ENABLE(FEATURE_DEVICE_AUTO_RCK))
				e_data = cal_data->count_r[s_index];
			else
				e_data = (((unsigned long long int)
					cal_data->count_rc_now[s_index]) *
					cal_data->count_r[s_index]) >> 14;

			lvts_writel_print(e_data,
					LVTSEDATA00_0 + base + 0x4 * j);
		}
	}
}

static void init_controller_v5(struct lvts_data *lvts_data)
{
	struct device *dev = lvts_data->dev;
	unsigned int i;
	void __iomem *base;

	for (i = 0; i < lvts_data->num_tc; i++) {
		base = GET_BASE_ADDR(i);

		lvts_write_device(lvts_data, SET_DEVICE_LOW_POWER_SINGLE_MODE,  i);

		lvts_writel_print(SET_SENSOR_INDEX, LVTSTSSEL_0 + base);
		lvts_writel_print(SET_CALC_SCALE_RULES, LVTSCALSCALE_0 + base);

		set_polling_speed(lvts_data, i);
		set_hw_filter(lvts_data, i);

		dev_info(dev, "lvts%d: read all %d sensors in %d us, one in %d us\n",
			i, GET_TC_SENSOR_NUM(i), GROUP_LATENCY_US(i),
			SENSOR_LATENCY_US(i));
	}

}

static void set_up_v5_common_callbacks(struct lvts_data *lvts_data)
{
	struct platform_ops *ops = &lvts_data->ops;

	ops->device_enable_and_init = device_enable_and_init_v5;
	ops->device_enable_auto_rck = device_enable_auto_rck_v5;
	ops->device_read_count_rc_n = device_read_count_rc_n_v5;
	ops->set_cal_data = set_calibration_data_v5;
	ops->init_controller = init_controller_v5;
}
/*==================================================
 * Extern function
 *==================================================
 */
struct lvts_match_data lvts_v5_match_data = {
	.hw_version = 5,
	.table = lvts_v5_match_table,
	.set_up_common_callbacks = set_up_v5_common_callbacks,
};

struct lvts_match_data *register_v5_lvts_match_data(void)
{
	return &lvts_v5_match_data;
}
EXPORT_SYMBOL_GPL(register_v5_lvts_match_data);
