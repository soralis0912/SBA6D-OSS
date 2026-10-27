/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MT753X_SWCONFIG_H_
#define _MT753X_SWCONFIG_H_

#ifdef CONFIG_SWCONFIG
#include <linux/switch.h>
#include "mt753x.h"

#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
#define NL_INITIATIVE 1
#define NL_PASSIVE 0

enum ADAPT_PHY_TYPE {
	AQR113C_1_PHY_ID = 0x31c31c12,
	AQR113C_2_PHY_ID = 0x31c31c13,
	RTL8221_1_PHY_ID = 0x1cc848,
	RTL8221_2_PHY_ID = 0x1cc849,
	MT7531_PHY_ID = 0x181d,		//cut down id
};

struct switch_phy_link_status{
       int     type;
       int     num ;
       int status ;
       int event_type ;
};

struct phy_link_status_def{
       struct switch_phy_link_status mt753x_state[10];
};
#endif
int mt753x_swconfig_init(struct gsw_mt753x *gsw);
void mt753x_swconfig_destroy(struct gsw_mt753x *gsw);
#else
static inline int mt753x_swconfig_init(struct gsw_mt753x *gsw)
{
	mt753x_apply_vlan_config(gsw);

	return 0;
}

static inline void mt753x_swconfig_destroy(struct gsw_mt753x *gsw)
{
}
#endif

#endif /* _MT753X_SWCONFIG_H_ */
