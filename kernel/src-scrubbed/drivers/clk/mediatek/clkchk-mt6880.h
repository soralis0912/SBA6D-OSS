/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef __DRV_CLKCHK_MT6880_H
#define __DRV_CLKCHK_MT6880_H

enum chk_sys_id {
	top,
	dbgsys_dem,
	ifrao,
	infracfg_ao_bus,
	peri,
	spm,
	apmixed,
	gce,
	audsys,
	impe,
	mfgcfg,
	mm,
	chk_sys_num,
};

#if IS_ENABLED(CONFIG_MTK_DVFSRC_HELPER)
extern int get_sw_req_vcore_opp(void);
#endif
#endif	/* __DRV_CLKDBG_MT6880_H */
