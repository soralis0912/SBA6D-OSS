/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */
#ifndef __MTK_SIP_SVC_H
#define __MTK_SIP_SVC_H

/* Error Code */
#define SIP_SVC_E_SUCCESS               0
#define SIP_SVC_E_NOT_SUPPORTED         -1
#define SIP_SVC_E_INVALID_PARAMS        -2
#define SIP_SVC_E_INVALID_RANGE         -3
#define SIP_SVC_E_PERMISSION_DENIED     -4

#ifdef CONFIG_ARM64
#define MTK_SIP_SMC_CONVENTION          ARM_SMCCC_SMC_64
#else
#define MTK_SIP_SMC_CONVENTION          ARM_SMCCC_SMC_32
#endif

#define MTK_SIP_SMC_CMD(fn_id) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, MTK_SIP_SMC_CONVENTION, \
			   ARM_SMCCC_OWNER_SIP, fn_id)

/* SCDK related SMC call */
#define MTK_SIP_KERNEL_MTK_SCDK \
        MTK_SIP_SMC_CMD(0x3B0)

/* Encrypt/Decrypt data with HW IK and ONLY support 64bit */
#define MTK_SIP_HW_IK \
	MTK_SIP_SMC_CMD(0x3EE)

/* VCOREFS */
#define MTK_SIP_VCOREFS_CONTROL \
    MTK_SIP_SMC_CMD(0x506)

/* Debug feature and ATF related */
#define MTK_SIP_KERNEL_WDT \
	MTK_SIP_SMC_CMD(0x200)
#define MTK_SIP_KERNEL_TIME_SYNC \
	MTK_SIP_SMC_CMD(0x202)
#define MTK_SIP_KERNEL_ATF_DEBUG \
	MTK_SIP_SMC_CMD(0x204)

/* AMMS related SMC call */
#define MTK_SIP_KERNEL_AMMS_GET_FREE_ADDR \
	MTK_SIP_SMC_CMD(0x250)
#define MTK_SIP_KERNEL_AMMS_GET_FREE_LENGTH \
	MTK_SIP_SMC_CMD(0x251)
#define MTK_SIP_KERNEL_AMMS_GET_PENDING \
	MTK_SIP_SMC_CMD(0x252)
#define MTK_SIP_KERNEL_AMMS_ACK_PENDING \
	MTK_SIP_SMC_CMD(0x253)
#define MTK_SIP_KERNEL_AMMS_GET_SEQ_ID \
	MTK_SIP_SMC_CMD(0x258)

/* Security related SMC call */
/* TRNG */
#define MTK_SIP_KERNEL_GET_RND \
	MTK_SIP_SMC_CMD(0x26A)
/* DEVAPC */
#define MTK_SIP_KERNEL_DAPC_PERM_GET \
	MTK_SIP_SMC_CMD(0x26B)
#define MTK_SIP_KERNEL_CLR_SRAMROM_VIO \
	MTK_SIP_SMC_CMD(0x26C)
#define MTK_SIP_KERNEL_DAPC_DUMP \
	MTK_SIP_SMC_CMD(0x26D)

/* HSM */
#define MTK_SIP_HSM_RESET \
	MTK_SIP_SMC_CMD(0x2C0)
#define MTK_SIP_HSM_CLOCK_ON \
	MTK_SIP_SMC_CMD(0x2C1)
#define MTK_SIP_HSM_CLOCK_OFF \
	MTK_SIP_SMC_CMD(0x2C2)

/* IK SMC call */
#define MTK_SIP_KERNEL_HW_IK \
	MTK_SIP_SMC_CMD(0x3EE)

/* Debug related SMC call */
#define MTK_SIP_KERNEL_CCCI_CONTROL \
	MTK_SIP_SMC_CMD(0x505)

/* MTK LPM */
#define MTK_SIP_MTK_LPM_CONTROL \
	MTK_SIP_SMC_CMD(0x507)

/* EMI MPU */
#define MTK_SIP_EMIMPU_CONTROL \
	MTK_SIP_SMC_CMD(0x50B)

/* MTK Data Crypto */
#define MTK_SIP_CRYPTO_CONTROL \
	MTK_SIP_SMC_CMD(0x510)

/* PCIe control related SMC call */
#define MTK_SIP_PCIE_CONTROL \
	MTK_SIP_SMC_CMD(0x522)

/* USB control related SMC call */
#define MTK_SIP_USB_CONTROL \
	MTK_SIP_SMC_CMD(0x527)

/* MTK MEDMCU */
#define MTK_SIP_KERNEL_MEDMCU_CONTROL \
	MTK_SIP_SMC_CMD(0x528)

/* connsys control related SMC call */
#define MTK_SIP_KERNEL_CONNSYS_CONTROL \
	MTK_SIP_SMC_CMD(0x534)

/* gps control related SMC call */
#define MTK_SIP_KERNEL_GPS_CONTROL \
	MTK_SIP_SMC_CMD(0x537)

/* sspm control related SMC call */
#define MTK_SIP_KERNEL_SSPM_CONTROL \
	MTK_SIP_SMC_CMD(0x539)

#endif

