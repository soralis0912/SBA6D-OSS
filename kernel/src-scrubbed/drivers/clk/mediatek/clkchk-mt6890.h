/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: David Cheng <pei-hsuan.cheng@mediatek.com>
 */

#ifndef __DRV_CLKCHK_MT6890_H
#define __DRV_CLKCHK_MT6890_H

enum chk_sys_id {
	top,
	ifrao,
	infracfg_ao_bus,
	spm,
	apmixed,
	audsys,
	dbgsys_dem,
	gce,
	impe,
	mfg,
	mm,
	chk_sys_num,
};

#if IS_ENABLED(CONFIG_MTK_DVFSRC_HELPER)
extern int get_sw_req_vcore_opp(void);
#endif
#endif	/* __DRV_CLKDBG_MT6890_H */

