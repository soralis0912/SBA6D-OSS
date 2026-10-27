// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/types.h>
#include <linux/platform_data/sec_process.h>
#include <linux/vmalloc.h>

#define DRIVER_VERSION              "1.0"

/* Action of crypto SMC call */
#define ACTION_PROCESS_QUERY_STATUS 0x10000000

/* lock of API */
static DEFINE_MUTEX(secure_status_lock);

/*
 * sec_query_status() - Query the status of security device
 *
 * @flag: Which status need to be queried
 * @status: Output data of query
 *
 * Return: Status of execution
 */
u32 sec_query_status(u32 flag, u32 *status)
{
	struct arm_smccc_res res;
	u32 cmd = ACTION_PROCESS_QUERY_STATUS;
	int rc = ERR_OK;

	mutex_lock(&secure_status_lock);

	arm_smccc_smc(MTK_SIP_CRYPTO_CONTROL,
		cmd, flag, 0, 0, 0, 0, 0, &res);

	*status = res.a1;
	rc = res.a0;

	mutex_unlock(&secure_status_lock);
	return rc;
}
EXPORT_SYMBOL(sec_query_status);

/* module load/unload record keeping */
static int __init secure_query_init(void)
{
	return 0;
}

static void __exit secure_query_exit(void)
{

}

module_init(secure_query_init);
module_exit(secure_query_exit);

MODULE_AUTHOR("Mediatek");
MODULE_DESCRIPTION("Mediatek Secure Query Driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

