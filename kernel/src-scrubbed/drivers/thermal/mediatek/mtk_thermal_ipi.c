// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include "mtk_thermal_ipi.h"
#include "thermal_risk_monitor.h"

static DEFINE_MUTEX(ipi_s_lock);
static int is_s_ipi_registered;
static int g_ack_data;
static unsigned int g_ipi_reply;
static struct thermal_ipi_data g_ipi_recv_data;

static int thermal_ipi_recv_handler(unsigned int id, void *prdata, void *in_msg, unsigned int len)
{
	struct thermal_ipi_data *data = (struct thermal_ipi_data *)in_msg;
	unsigned int cmd;

	cmd = data->cmd;
	pr_info("[thermal ipi] Recv thermal cmd=%d\n", cmd);

	g_ipi_reply = IPI_SUCCESS;
	switch (cmd) {
		case AP_WAKEUP_EVENT:
			keep_ap_wake();
			break;
		default:
			pr_err("ERROR: Unknown ipi request from sspm side!\n");
			g_ipi_reply = IPI_WRONG_MSG_TYPE;
	}

	return 0;
}

static int thermal_ipi_recv_thread(void *data)
{
	int ret;

	ret = mtk_ipi_register(&sspm_ipidev, IPIR_C_THERMAL, thermal_ipi_recv_handler, NULL,
		(void *)&g_ipi_recv_data);

	if (ret != 0) {
		pr_err("[thermal ipi] Fail to register IPIR_C_THERMAL ret:%d\n",
			ret);
		return -1;
	}

	while (!kthread_should_stop()) {
		mtk_ipi_recv(&sspm_ipidev, IPIR_C_THERMAL);
	}

	return 0;
}

unsigned int thermal_to_sspm(
	unsigned int cmd, struct thermal_ipi_data *thermal_data)
{
	unsigned int ack_data;
	int ret;

	mutex_lock(&ipi_s_lock);
	if (!is_s_ipi_registered) {
		ret = mtk_ipi_register(&sspm_ipidev, IPIS_C_THERMAL, NULL, NULL,
				(void *)&g_ack_data);

		if (ret != 0) {
			pr_err("[thermal ipi] Fail to register IPIS_C_THERMAL ret:%d\n",
					ret);
			ack_data = IPI_NOT_SUPPORT;
			goto end;
		}

		is_s_ipi_registered = 1;
	}

	thermal_data->cmd = cmd;
	ret = mtk_ipi_send_compl(&sspm_ipidev, IPIS_C_THERMAL, IPI_SEND_POLLING,
		(void *)thermal_data, THERMAL_SLOT_NUM, THERMAL_IPI_TIMEOUT_MS);

	if (ret != 0) {
		pr_err("[thermal ipi] send cmd(%d) error ret:%d\n", cmd, ret);
		ack_data = IPI_FAIL;
		goto end;
	}

	ack_data = g_ack_data;

	if (ack_data != IPI_SUCCESS)
		pr_info("[thermal ipi] cmd(%d) reply data:%u\n", cmd, ack_data);
end:
	mutex_unlock(&ipi_s_lock);

	return ack_data;
}

static int __init mtk_thermal_ipi_init(void)
{
	struct task_struct *ts1;
	int err;

	ts1 = kthread_run(thermal_ipi_recv_thread, NULL, "sspm_to_thermal");
	if (IS_ERR(ts1)) {
		err = PTR_ERR(ts1);
		pr_err("[thermal ipi] Fail to create a thread for ipi recv, %d\n",
			err);

		return err;
	}

	return 0;
}

static void __exit mtk_thermal_ipi_exit(void)
{
	mtk_ipi_unregister(&sspm_ipidev, IPIS_C_THERMAL);
	mtk_ipi_unregister(&sspm_ipidev, IPIR_C_THERMAL);
}

module_init(mtk_thermal_ipi_init);
module_exit(mtk_thermal_ipi_exit);
