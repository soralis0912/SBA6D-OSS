/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: David Cheng <pei-hsuan.cheng@mediatek.com>
 */

#ifndef __DRV_CLKCHK_mt6980_H
#define __DRV_CLKCHK_mt6980_H

enum chk_sys_id {
	top = 0,
	ifrao = 1,
	infracfg = 2,
	apmixed = 3,
	gce = 4,
	imp_iic_top_wrap = 5,
	perao = 6,
	usb_sif = 7,
	afe = 8,
	msdc0 = 9,
	msdc1 = 10,
	usb_t5ff = 11,
	spm = 12,
	vlpcfg = 13,
	vlp_ck = 14,
	sspm_cfg = 15,
	mfg_top = 16,
	mdp = 17,
	dbgao = 18,
	dem = 19,
	chk_sys_num = 20,
};

#if IS_ENABLED(CONFIG_MTK_DVFSRC_HELPER)
extern int get_sw_req_vcore_opp(void);
#endif
#endif	/* __DRV_CLKDBG_mt6980_H */

