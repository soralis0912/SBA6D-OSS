/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef SAFEXCEL_PROC_H
#define SAFEXCEL_PROC_H

extern int dbg_disable_eip97;
extern int dbg_enable_log;
extern int dbg_eip97_vcore_max;
extern int dbg_eip97_vcore_min;

#define DEFAULT_DISABLE_EIP97			0
#define DEFAULT_ENABLE_LOG				1
#define DEFAULT_VCORE_MAX				750000
#define DEFAULT_VCORE_MIN_MT6890		550000
#define DEFAULT_VCORE_MIN_MT6990		575000

#define PROC_UPDATE_DISABLE_EIP97		BIT(0)
#define PROC_UPDATE_ENABLE_LOG			BIT(1)
#define PROC_UPDATE_EIP97_VCORE			BIT(2)

typedef void (*debug_proc_update_func)(long stat, void *priv);

int safexcel_proc_init(debug_proc_update_func callback, void *priv);
void safexcel_proc_exit(void);

#endif
