// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/delay.h>

#include "mt753x.h"
#include "mt753x_regs.h"
#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
#include "mt753x_swconfig.h"

struct phy_link_status_def phy_link_table;
struct switch_phy_link_status send_table;
extern int send_msg(struct switch_phy_link_status pbuf, int len);
extern int mt7531_netlink_enable;
#endif

#ifdef CONFIG_QUECTEL_BR_TRIG
#include <linux/notifier.h>
extern struct blocking_notifier_head br_trig_chain_head;
#endif

void mt753x_irq_enable(struct gsw_mt753x *gsw)
{
	u32 val;
	int i;

	/* Record initial PHY link status */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		val = gsw->mii_read(gsw, i, MII_BMSR);
		if (val & BMSR_LSTATUS)
			gsw->phy_link_sts |= BIT(i);
	}

	val = BIT(MT753X_NUM_PHYS) - 1;

	mt753x_reg_write(gsw, SYS_INT_EN, val);
}

static void display_port_link_status(struct gsw_mt753x *gsw, u32 port)
{
	u32 pmsr, speed_bits;
	const char *speed;

	pmsr = mt753x_reg_read(gsw, PMSR(port));

	speed_bits = (pmsr & MAC_SPD_STS_M) >> MAC_SPD_STS_S;

	switch (speed_bits) {
	case MAC_SPD_10:
		speed = "10Mbps";
		break;
	case MAC_SPD_100:
		speed = "100Mbps";
		break;
	case MAC_SPD_1000:
		speed = "1Gbps";
		break;
	case MAC_SPD_2500:
		speed = "2.5Gbps";
		break;
	}

	if (pmsr & MAC_LNK_STS) {
		dev_info(gsw->dev, "Port %d Link is Up - %s/%s\n",
			 port, speed, (pmsr & MAC_DPX_STS) ? "Full" : "Half");
			 #ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
				if (mt7531_netlink_enable)
				{
						phy_link_table.mt753x_state[port].type = MT7531_PHY_ID;
						phy_link_table.mt753x_state[port].num = port ;
						phy_link_table.mt753x_state[port].status = 1 ;
						phy_link_table.mt753x_state[port].event_type = NL_PASSIVE;
						send_msg(phy_link_table.mt753x_state[port],sizeof(send_table));
				}
				#endif
	} else {
		dev_info(gsw->dev, "Port %d Link is Down\n", port);
		#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
		if (mt7531_netlink_enable)
		{
				phy_link_table.mt753x_state[port].type = MT7531_PHY_ID;
				phy_link_table.mt753x_state[port].num = port ;
				phy_link_table.mt753x_state[port].status = 0 ;
				phy_link_table.mt753x_state[port].event_type = NL_PASSIVE;
				send_msg(phy_link_table.mt753x_state[port],sizeof(send_table));
		}
		#endif
	}
}


#ifdef CONFIG_QUECTEL_BR_TRIG
enum BR_DEV_NO {
	SWITCH_P0,
	SWITCH_P1,
	SWITCH_P2,
	SWITCH_P3,
	SWITCH_P4,
};
unsigned long __pack_devno_evt(enum BR_DEV_NO dev_no, unsigned long evt);
int __netsw_trig_notify(enum BR_DEV_NO dev_no, unsigned long evt)
{
	int ret = blocking_notifier_call_chain(&br_trig_chain_head, __pack_devno_evt(dev_no, evt), NULL);
	return notifier_to_errno(ret);
}
#endif

void mt753x_irq_worker(struct work_struct *work)
{
	struct gsw_mt753x *gsw;
	u32 sts, __physts = 0, physts, laststs;
	int i;

	gsw = container_of(work, struct gsw_mt753x, irq_worker);

	sts = mt753x_reg_read(gsw, SYS_INT_STS);

	/* Check for changed PHY link status */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		if (!(sts & PHY_LC_INT(i)))
			continue;

		laststs = gsw->phy_link_sts & BIT(i);
		physts = !!(gsw->mii_read(gsw, i, MII_BMSR) & BMSR_LSTATUS);
		__physts = physts;
		physts <<= i;

		if (physts ^ laststs) {
			gsw->phy_link_sts ^= BIT(i);
			display_port_link_status(gsw, i);
#ifdef CONFIG_QUECTEL_BR_TRIG
			__netsw_trig_notify(i, __physts?NETDEV_UP:NETDEV_DOWN);
#endif
		}
	}

	mt753x_reg_write(gsw, SYS_INT_STS, sts);

	enable_irq(gsw->irq);
}
