/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#ifndef __DEVAPC_MT69XX_H__
#define __DEVAPC_MT69XX_H__

/******************************************************************************
 * Configuration
 ******************************************************************************/
#define DEVAPC_ENABLE_KE			(1)

/*******************************************************************************
* Register
*
*******************************************************************************/
#define DEVAPC_INFRA_PD_BASE 		(0x10205000)
#define DEVAPC_PERI_PD_BASE 		(0x10274000)
#define DEVAPC_PERI2_PD_BASE 		(0x10275000)
#define DEVAPC_PERI_PAR_PD_BASE 	(0x11031000)
#define DEVAPC_EMI_PD_BASE 		(0x10371000)
#define DEVAPC_VLP_PD_BASE 		(0x1201C000)
#define INFRACFG_BASE 				(0x1020E000)

/*******************************************************************************
* Data Structure
*
*******************************************************************************/
enum DEVAPC_SLAVE_TYPE {
	SLAVE_TYPE_INFRA = 0,
	SLAVE_TYPE_PERI,
	SLAVE_TYPE_PERI2,
	SLAVE_TYPE_PERI_PAR,
	SLAVE_TYPE_VLP,
	SLAVE_TYPE_EMI,
	SLAVE_TYPE_NUM,
};

enum DEVAPC_VIO_MASK_STA_NUM {
	VIO_MASK_STA_NUM_INFRA = 12,
	VIO_MASK_STA_NUM_PERI = 3,
	VIO_MASK_STA_NUM_PERI2 = 5,
	VIO_MASK_STA_NUM_PERI_PAR = 3,
	VIO_MASK_STA_NUM_EMI = 1,
	VIO_MASK_STA_NUM_VLP = 3,
};

enum DEVAPC_VIO_SLAVE_NUM {
	VIO_SLAVE_NUM_INFRA = 198,
	VIO_SLAVE_NUM_PERI = 63,
	VIO_SLAVE_NUM_PERI2 = 74,
	VIO_SLAVE_NUM_PERI_PAR = 34,
	VIO_SLAVE_NUM_EMI = 4,
	VIO_SLAVE_NUM_VLP = 46,
};

#define TYPE2_VIO_NULL (-1)

/******************************************************************************
 * PLATFORM DEFINATION
 ******************************************************************************/
/* For SRAMROM VIO */
#define SRAMROM_SLAVE_TYPE			SLAVE_TYPE_INFRA
#define SRAMROM_VIO_INDEX 			(346)

#endif /* __DEVAPC_MT69XX_H__ */

