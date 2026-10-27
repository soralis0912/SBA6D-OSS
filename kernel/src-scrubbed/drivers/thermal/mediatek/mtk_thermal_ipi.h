/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_THERMAL_IPI_H__
#define __MTK_THERMAL_IPI_H__
#include <sspm_ipi_id.h>
/*==================================================
 * Definition or macro function
 *==================================================
 */
#define THERMAL_SLOT_NUM (4)
#define THERMAL_IPI_TIMEOUT_MS (2000)

enum thermal_ipi_reply_data {
	IPI_SUCCESS,
	IPI_FAIL,
	IPI_NOT_SUPPORT,
	IPI_WRONG_MSG_TYPE,
	IPI_SEND_LOCK_TIMEOUT,
	NUM_THERMAL_IPI_REPLY
};

enum thermal_ipi_msg_type {
	TRM_IPI_INIT_GRP1,
	TRM_IPI_DISABLE_CMD,
	TIA_IPI_INIT_GRP1,
	AP_WAKEUP_EVENT,
	NUM_THERMAL_IPI_TYPE
};

/*==================================================
 * Data structure
 *==================================================
 */
struct thermal_ipi_data {
	unsigned int cmd;
	union {
		struct {
			int arg[THERMAL_SLOT_NUM - 1];
		} data;
	} u;
};
/*==================================================
 * Extern function
 *==================================================
 */
extern unsigned int thermal_to_sspm(unsigned int cmd,
	struct thermal_ipi_data *thermal_data);
#endif
