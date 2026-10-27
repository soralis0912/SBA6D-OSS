/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __THERMAL_RISK_MONITOR_H__
#define __THERMAL_RISK_MONITOR_H__
/*==================================================
 * Definition or macro function
 *==================================================
 */
enum thermal_sensor {
	SEN_SOC_MAX,
	SEN_CPU_LITTLE,
	SEN_CPU_BIG,
	SEN_CPU_BB,
	SEN_GPU,
	SEN_MD,
	SEN_SOC_NTC,
	SEN_LTEPA_NTC,
	SEN_NRPA_NTC,
	SEN_RF_NTC,
	NUM_THERMAL_SENSOR
};

struct trm_sensor_data {
	enum thermal_sensor id;
	int threshold;                /* Threshold to wake up AP in m'C*/
	int hysteresis;               /* To debounce in m'C */
	struct list_head node;
};

struct trm_data {
	struct list_head sen_list;
	struct mutex sen_list_lock;
	char *id_to_tz_map;
	struct device *dev;
};

#define TRM_ATTR_RO(_name) \
static struct kobj_attribute _name##_attr = __ATTR_RO(_name)
#define TRM_ATTR_WO(_name) \
static struct kobj_attribute _name##_attr = __ATTR_WO(_name)
/*==================================================
 * Data structure
 *==================================================
 */
/*==================================================
 * Extern function
 *==================================================
 */
extern void keep_ap_wake(void);
#endif
