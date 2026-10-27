// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek Common
 *
 * Author: fangju.wang <fangju.wang@mediatek.com>
 *
 */

#include <linux/pinctrl/devinfo.h>
#include <linux/of_device.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/if_vlan.h>
#include <linux/reset.h>
#include <linux/tcp.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/mdio.h>
#include <linux/regulator/consumer.h>
#include <net/dsa.h>

#include <linux/pm_wakeup.h>

#include "mtk_eth_soc.h"
#include "mtk_common.h"
#include "mtk_sgmii_pwr.h"
#include "mtk_eth_dbg.h"

static struct pinctrl *eth_gpio;
/*static struct pinctrl_state *eth_gpio_default;*/
static struct pinctrl_state *eth_smi_mdio_pinctl;
static struct pinctrl_state *eth_smi_mdc_pinctl;

int pinctrl_init(struct mtk_eth *eth)
{

	eth_gpio = devm_pinctrl_get(eth->dev);
	if (!IS_ERR(eth_gpio)) {
		eth_smi_mdio_pinctl = pinctrl_lookup_state(eth_gpio, "eth_smi_mdio_pinctl");
		if (!IS_ERR(eth_smi_mdio_pinctl))
			pinctrl_select_state(eth_gpio, eth_smi_mdio_pinctl);
		else {
			dev_info(eth->dev, "[ETH][ERROR] Cannot ETH_MDIO!\n");
			return -1;
		}

		eth_smi_mdc_pinctl = pinctrl_lookup_state(eth_gpio, "eth_smi_mdc_pinctl");
		if (!IS_ERR(eth_smi_mdc_pinctl))
			pinctrl_select_state(eth_gpio, eth_smi_mdc_pinctl);
		else {
			dev_info(eth->dev, "[ETH][ERROR] Cannot ETH_MDC!\n");
			return -1;
		}

	} else {
		dev_info(eth->dev, "[ETH][ERROR] Cannot find eth_gpio!\n");
		return -1;
	}
	return 0;
}


static u32 mii_mgr_cl45_set_address(struct mtk_eth *eth, u32 port_num, u32 devad, u32 reg_addr)
{
	u32 data;

	data =
	    (devad << 25) | (port_num << 20) | (0x00 << 18) | (0x00 << 16) |
	    reg_addr;

	mtk_w32(eth, data, MTK_PHY_IAC);
	mtk_w32(eth, data | (1<<31), MTK_PHY_IAC);

	return 0;
}

u32 _mtk_mdio_read_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 devad)
{
	u32 data;
	u32 d;

	if (mtk_mdio_busy_wait_ext(eth))
		return -1;

	/* set address first */
	mii_mgr_cl45_set_address(eth, phy_addr, devad, phy_register);
	/* udelay(10); */

	if (mtk_mdio_busy_wait_ext(eth))
		return -1;

	data = (devad << PHY_IAC_REG_SHIFT) | (phy_addr << PHY_IAC_ADDR_SHIFT) |
	       (0x03 << MDIO_CMD) | (0x00 << NMDIO_ST) | phy_register;

	mtk_w32(eth, data, MTK_PHY_IAC);
	mtk_w32(eth, data | (1<<31), MTK_PHY_IAC);

	if (mtk_mdio_busy_wait_ext(eth))
		return -1;

	d = mtk_r32(eth, MTK_PHY_IAC) & 0xffff;

	return d;

}

u32 _mtk_mdio_write_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data, u32 devad)
{
	u32 data;

	if (mtk_mdio_busy_wait_ext(eth))
		return -1;

	write_data &= 0xffff;

	/* set address first */
	mii_mgr_cl45_set_address(eth, phy_addr, devad, phy_register);
	/* udelay(10); */

	if (mtk_mdio_busy_wait_ext(eth))
		return -1;
	data =
	    (devad << PHY_IAC_REG_SHIFT) | (phy_addr << PHY_IAC_ADDR_SHIFT) |
	    PHY_IAC_WRITE | (0x00 << NMDIO_ST) |
	    write_data;

	mtk_w32(eth, data, MTK_PHY_IAC);
	mtk_w32(eth, data | (1<<31), MTK_PHY_IAC);

	if (mtk_mdio_busy_wait_ext(eth))
		return -1;

	return 0;
}

u32 mtk_cl45_ind_read(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 *data)
{
	mutex_lock(&eth->mii_bus->mdio_lock);
	*data = _mtk_mdio_read_C45(eth, port, reg, devad);
	mutex_unlock(&eth->mii_bus->mdio_lock);

	return 0;
}

u32 mtk_cl45_ind_write(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 data)
{
	mutex_lock(&eth->mii_bus->mdio_lock);
	_mtk_mdio_write_C45(eth, port, reg, data, devad);
	mutex_unlock(&eth->mii_bus->mdio_lock);

	return 0;
}



int netsys_pm_suspend_poweroff(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct mtk_eth *eth;
	u32 val;
	int ret;
	int enable_hw_idle = 0;
	int i;

	eth = platform_get_drvdata(pdev);

	pr_notice("ethernet suspend time start = %lx\n", jiffies);

	//1. store ethx interface state
	//store item : 1. MCR 2. eth0 eth1 up/down
	memset(&eth->suspend_data, 0x0, sizeof(struct _suspend_data));

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;

		if (eth->soc->aqr113c_phy0_support || eth->soc->aqr113c_phy1_support) {
			if (eth->netdev[i]->flags & IFF_UP) {
				eth->suspend_data.eth_state[i] = 1;
				pr_notice("%s eth%d current open", __func__, i);
			} else {
				eth->suspend_data.eth_state[i] = 0;
				pr_notice("%s eth%d current close", __func__, i);
			}

		} else {
			val = mtk_r32(eth, MTK_MAC_MCR(i));
			eth->suspend_data.mcr[i] = val;

			if (netif_running(eth->netdev[i])) {
				pr_notice("%s netdev %d link up %x", __func__, i, val);
				eth->suspend_data.eth_state[i] = 1;
			} else {
				pr_notice("%s netdev %d link down %x", __func__, i, val);
				eth->suspend_data.eth_state[i] = 0;
			}
		}
	}

	//Suspend Flow start here
	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		//only stop interface which is up
		if (netif_running(eth->netdev[i])) {
			rtnl_lock();
			mtk_stop(eth->netdev[i]);
			rtnl_unlock();
		}
	}

	//4. Vcore set low
	if (eth->soc->sgmii_pm)
		regulator_set_voltage(eth->dvfsrc_vcore_power, SGMII_VCORE_NON_OPS, INT_MAX);

	//8. Special Part
	/*disable ADMA*/
	mtk_w32(eth, 0x00200080, MTK_PDMA_LRO_CTRL_DW0);
	mtk_w32(eth, 0x1c00, MTK_PDMA_GLO_CFG);
	mtk_w32(eth, 0x810001b0, 0);
	/*check ADMA IDLE*/
	if (enable_hw_idle == 0) {
		regmap_write(eth->ethsys, 0xf8, 0x00008202);
		regmap_write(eth->ethsys, 0xf4, 0x7c000000);
		regmap_write(eth->ethsys, 0x10, 0xb6000000);
		regmap_write(eth->ethsys, 0x58, 0x20000000);
		regmap_write(eth->ethsys, 0xfc, 0x007f0000);
		regmap_write(eth->ethsys, 0x10c, 0x00001080);
		enable_hw_idle = 1;
	}

	ret = regmap_read(eth->ethsys, 0x54, &val);
	if (ret)
		return ret;
	pr_notice("0x15000054 is 0x%x\n", val);
	val = val & 0x0C000002;
	if (val == 0x0C000002) {
		mtk_w32(eth, 0x05f2c040, 0x98c);
		usleep_range(1000, 2000);
		mtk_w32(eth, 0x05f28040, 0x98c);
		regmap_read(eth->ethsys, 0x54, &val);
		pr_info("NETSYS hw idle status: 0x%x\n", val);
		usleep_range(3000, 4000);
	} else
		pr_info("ADMA related bus not idle!!!!\n");

	mtk_hw_deinit(eth);

	pr_notice("netsys suspend_noirq done = %lx\n", jiffies);
	return 0;
}



int netsys_pm_resume_poweroff(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct device_node *np;
	struct mtk_eth *eth;
	int err;
	const char *str;
	u32 val = 0;
	int i;

	pr_notice("netsys_pm_resume time start = %lx\n", jiffies);

	eth = platform_get_drvdata(pdev);

	mtk_hw_init(eth);
	pr_info("do hw init done");

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;

		pr_notice("restore mcr%d =%d %x", i,
			eth->suspend_data.eth_state[i],
			eth->suspend_data.mcr[i]);

		//Set phylink_link_state to correct state
		if (eth->suspend_data.eth_state[i] == 1) { //
			eth->mac[i]->interface = PHY_INTERFACE_MODE_NA;
			rtnl_lock();
			mtk_open(eth->netdev[i]);
			rtnl_unlock();
		}
	}

	pr_notice("netsys resume_noirq done = %lx\n", jiffies);

	regmap_read(eth->sgmii->regmap[0], (0x616c), &val);
	pr_notice("netsys 0x1000616c 0x%x\n", val);

	return 0;
}

void mtk_eth_auto_qos_init(struct mtk_eth *eth)
{
	u32 i, orig;

	for (i = 60; i < 64; i++) {

		/* min rate(10Mbps), max rate(1Gbps)
		 * Q60: echo rate 60 1 0 0 1 10 5 > /proc/mtketh/qos
		 * Q61: echo rate 61 1 0 0 1 10 5 > /proc/mtketh/qos
		 * Q62: echo rate 62 1 0 0 1 10 5 > /proc/mtketh/qos
		 * Q63: echo rate 63 1 0 0 1 10 5 > /proc/mtketh/qos
		 */
		mtk_w32(eth, 3, MTK_QDMA_SEL);

		mtk_w32(eth, (QDMA_RES_THRES << 8) | QDMA_RES_THRES,
			MTK_QTX_CFG((i-48)));

		mtk_w32(eth, (TX_SCH_SEL3 << 30) |
			     MIN_RATE_EN | MIN_RATE_MAN(0) | MIN_RATE_EXP(0) |
			     MAX_RATE_EN | MAX_RATE_MAN(10) | MAX_RATE_EXP(5),
			     MTK_QTX_SCH((i-48)));

	}

	/* Set SCH4 as WRR
	 * set sch3 as wrr: echo sch_policy 3 wrr > /proc/mtketh/qos
	 */
	orig = mtk_r32(eth, MTK_QDMA_TX_SCH3_SCH4);
	mtk_w32(eth, orig | BIT(31) , MTK_QDMA_TX_SCH3_SCH4);
}



int mtk_eth_sgmii_set_force_mode(struct mtk_eth *eth, u32 id, int en)
{

	if (eth->mac[id]->sgmii_force_en == en) {
		pr_info("%s: set %d %d status no change", __func__, id, en);
		return -1;
	}

	if (en == 1 || en == -1) {
		eth->mac[id]->sgmii_force_en = ((en == 1)? 1 : -1);
		pr_info("%s: sgmii_force_en %d %d", __func__,
			id, eth->mac[id]->sgmii_force_en);
	} else {
		pr_info("%s: set  force invalid %d %d status no change", __func__, id, en);
		return -1;
	}

	return 0;
}

int mtk_eth_sgmii_runtime_status_action(struct mtk_eth *eth, u32 id, int if_status)
{
	int previous_decision = eth->mac[id]->gmii_decision;
	int gmii_decision = 0;

	if (eth->soc->aqr113c_phy0_support ||
		eth->soc->aqr113c_phy1_support){
		pr_info("%s: %d not support", __func__, id);
		return 0;
	}

	if (eth->mac[id]->sgmii_force_en == 1) {
		gmii_decision = 1;
	} else if (eth->mac[id]->sgmii_force_en == -1) {
		gmii_decision = 0;
	} else if ((id == 1) && (if_status == MTK_IF_UP)\
	    &&(eth->phy_link_7531 == 1) ) {//MT7531
		gmii_decision = 1;
	} else if (id == 0) { //AQR112C only support force mode
		gmii_decision = 1;
	} else {
		gmii_decision = 0;
	}
	eth->mac[id]->gmii_decision = gmii_decision;

	pr_info("%s: %d prev=%d now=%d", __func__, id, previous_decision,
		eth->mac[id]->gmii_decision);

	return mtk_eth_sgmii_runtime_set(eth, id,
		eth->mac[id]->gmii_decision, if_status);
}


int mtk_eth_sgmii_runtime_set(struct mtk_eth *eth,
	u32 id, int en, int if_status)
{
	int ret = 0;
	u32 val = 0;
	int mac = 0;

	if (en) {
		//power on
		ret = sgmii_runtime_control(id, en);
		if (ret < 0) {
			pr_info("%s: set %d %d fail: ret =%d", __func__, id, en, ret);
			return -1;
		}

		if (eth->soc->aqr113c_phy0_support || eth->soc->aqr113c_phy1_support) {
			pr_info("%s: 10g [%d] eth enable", __func__, id);
			mtk_usxgmii_setup_mode_an(eth->sgmii, id, eth->mac[id]->max_speed);
			val = mtk_r32(eth, MTK_XMAC_MCR(id));
			val &= ~(XMAC_MCR_TRX_DISABLE);
			mtk_w32(eth, val, MTK_XMAC_MCR(id));
		} else {
			if (ret > 0) {
				//sgmii patch is done, skip
				pr_info("%s: %d sgmii patch is done, skip", __func__, id);
			} else {
				if(eth->mac[id]->interface == PHY_INTERFACE_MODE_2500BASEX)
					mtk_sgmii_setup_mode_force(eth->sgmii,
					id, eth->mac[id]->max_speed);
				else
					mtk_sgmii_setup_mode_an(eth->sgmii,
					id, eth, eth->mac[id]->max_speed);
			}

			if (!if_status) {
				pr_info("%s: %d currently disable, don't link up", __func__, id);
				return 0;
			}
			//MTK_MAC_MCR link up
			val = mtk_r32(eth, MTK_MAC_MCR(id));
			val = val | MAC_MCR_FORCE_LINK | MAC_MCR_TX_EN | MAC_MCR_RX_EN;
			mtk_w32(eth, val, MTK_MAC_MCR(id));
			pr_info("%s: %d MTK_MAC_MCR val=%x", __func__,
				id, mtk_r32(eth, MTK_MAC_MCR(id)));
		}
		pr_info("%s: enable sgmii %d done val=%x", __func__, id, val);
	} else { //disable
		if (eth->soc->aqr113c_phy0_support || eth->soc->aqr113c_phy1_support) {
			val = mtk_r32(eth, MTK_XMAC_MCR(id));
			val &= 0xfffffff0;
			val |= XMAC_MCR_TRX_DISABLE;
			mtk_w32(eth, val, MTK_XMAC_MCR(id));

			if (((mtk_r32(eth, MTK_XMAC_MCR(0)) & 0xf)!= 0) &&
				((mtk_r32(eth, MTK_XMAC_MCR(1)) & 0xf)!= 0)) {
				regmap_read(eth->sgmii->remap_pll, MT6990_XFI_PLL_OFFSET, &val);
				val &= ~(MT6990_XFI_PLL_EN);
				regmap_write(eth->sgmii->remap_pll, MT6990_XFI_PLL_OFFSET, val);

				pr_info("%s: XFIPLL diable %d done val=%x", __func__, id, val);
			}
		} else {
			val = mtk_r32(eth, MTK_MAC_MCR(id));
			//disable MAC_MCR_FORCE_LINK | MAC_MCR_TX_EN | MAC_MCR_RX_EN;
			val = val & 0xffff9ffe;
			mtk_w32(eth, val, MTK_MAC_MCR(id));

			pr_info("%s: MTK_MAC_MCR link down %d done val=%x", __func__, id,
				mtk_r32(eth, MTK_MAC_MCR(id)));

			//if both eth down, disable PLL
			if (((eth->mac[0] != NULL) && (eth->mac[0]->gmii_decision == 0)) &&
				((eth->mac[1] != NULL) && (eth->mac[1]->gmii_decision == 0))) {
				regmap_read(eth->sgmii->remap_pll, MT6990_XFI_PLL_OFFSET, &val);
				val &= ~(MT6990_XFI_PLL_EN);
				regmap_write(eth->sgmii->remap_pll, MT6990_XFI_PLL_OFFSET, val);
				pr_info("%s: XFIPLL diable %d done val=%x", __func__, id, val);
			}
		}

		//power off
		ret = sgmii_runtime_control(id, en);
		if (ret < 0) {
			pr_info("%s: set %d %d fail: ret =%d", __func__, id, en, ret);
			return -1;
		}
 		pr_info("%s: disable sgmii %d done val=%x", __func__, id, val);
 	}
	return 0;
}

/*
Note: this function can only support for AQR112C status polling
*/
void mtk_mac_status_polling_aqr(struct mtk_eth *eth, int id, int type)
{
	u32 lp_pause, speed_bits, link;
	const char *speed;
	u32 data;
	u32 val = 0;
	u32 adv_cap;
	int phy_id = 0;

	if (type == AQR112C_PHY0) {
		phy_id = 8;
		val = mtk_r32(eth, MTK_MAC_MCR(id));
	}

	/*1. Check Single PHY Flow Control Status & disable EINT*/
	_mtk_mdio_read_C45(eth, phy_id, 0xfc00, 0x1e);
	_mtk_mdio_read_C45(eth, phy_id, 0xFC01, 0x1e);
	_mtk_mdio_read_C45(eth, phy_id, 0xFC00, 0x7);
	_mtk_mdio_read_C45(eth, phy_id, 0xCC01, 0x7);
	//modify FC
	val &= ~(MAC_MCR_FORCE_RX_FC | MAC_MCR_FORCE_TX_FC);

	lp_pause = (_mtk_mdio_read_C45(eth, phy_id, 0x13, 0x7) & BIT(10)) >> 10;
	if (lp_pause) {
		/*if PHY Flow control enable - enable MAC flow control*/
		if (type == AQR112C_PHY0) {
				val |= MAC_MCR_FORCE_RX_FC;
				val |= MAC_MCR_FORCE_TX_FC;
		}
	} else {
		/*if PHY Flow control disable - disable MAC flow control*/
		if (type == AQR112C_PHY0) {
				val &= ~MAC_MCR_FORCE_RX_FC;
				val &= ~MAC_MCR_FORCE_TX_FC;
		}
	}

	adv_cap = (_mtk_mdio_read_C45(eth, phy_id, 0x20, 0x7));

	/*2. Check Single PHY Link/Speed*/
	speed = "link_down";
	link = (_mtk_mdio_read_C45(eth, phy_id, 0xe800, 0x1)) & BIT(0);

	speed_bits = ((_mtk_mdio_read_C45(eth, phy_id, 0xc800, 0x7)) & 0xe) >> 1;
	if (link) {
		switch (speed_bits) {
			case 0:
				speed = "down";
				break;
			case 1:
				speed = "100Mbps";
				if (MTK_PHY_SPEED_UPDATE) {
					val &= ~BIT(3); //0x01
					val |= BIT(2);
				}
				break;
			case 2:
				speed = "1Gbps";
				if (MTK_PHY_SPEED_UPDATE) {
					val |= BIT(3);	//0x10
					val &= ~BIT(2);
				}
				break;
			case 3:
				speed = "10Gbps";
				if (MTK_PHY_SPEED_UPDATE) {
					val |= BIT(3);	//0x11
					val |= BIT(2);
				}
				break;
			case 4:
				speed = "2.5Gbps";
				if (MTK_PHY_SPEED_UPDATE) {
					val |= BIT(3);	//0x10
					val &= ~BIT(2);
				}
				break;
			case 5:
				speed = "5Gbps";
				break;
			}
	} else {
		/*if PHY Link down - reset MAC flow control to default*/
		if (type == AQR112C_PHY0) {
			if (!MTK_PHY_SPEED_UPDATE) {
				val |= MAC_MCR_FORCE_RX_FC;
				val |= MAC_MCR_FORCE_TX_FC;
			}
		}
	}

	if (type == AQR112C_PHY0) {
		mtk_w32(eth, val, MTK_MAC_MCR(id));
	}
	dev_info(eth->dev,
		"%s, type:%d, lp_pause:%d, link: %x, adv_cap: %x, PHY: %d, speed: %s\n",
		__func__, type, lp_pause, link, adv_cap, phy_id, speed);
}


/*
Note: this function can only support for MT7531 status polling
*/
void mtk_mt7531_polling(struct mtk_eth *eth, int id, int if_status)
{
	int i;
	int m7531_link_reg[] = {0x3008, 0x3108, 0x3208, 0x3308};
	int m7531_link_status[4];
	int previous_status;

	previous_status = eth->phy_link_7531;
	eth->phy_link_7531 = 0;

	//To avoid link status change delay
	udelay(2000);
	//example for eth0 mapping to mt7531 port0
	for (i = 0; i < 4; i++) {
		m7531_link_status[i]
			= (mt7530_mdio_r32(eth, m7531_link_reg[i]) & 0x1) ;
		eth->phy_link_7531 |= m7531_link_status[i];
	}

	dev_info(eth->dev, "%s: %d %d %d %d phy_link= %d, prev= %d if_status=%d\n",
			__func__, m7531_link_status[0], m7531_link_status[1],
			m7531_link_status[2], m7531_link_status[3],
			eth->phy_link_7531, previous_status, if_status);

	mtk_eth_sgmii_runtime_status_action(eth, id, if_status);
}

/*
Note: this function can only support for AQR112C/AQR113C flow control
*/
void mtk_flow_ctrl_aqr(struct mtk_eth *eth, int type)
{
	u32 tmp, phy_id = 0;

	if (type == AQR112C_PHY0) {
		phy_id = 8;
	} else if (type == AQR113C_PHY1) {
		phy_id = 0;
	} else if (type == AQR113C_PHY0) {
		phy_id = 8;
	}

	_mtk_mdio_write_C45(eth, phy_id, 0x10, 0x9d81, 7);
	tmp = _mtk_mdio_read_C45(eth, phy_id, 0xd401, 7);
	_mtk_mdio_write_C45(eth, phy_id, 0xd401, (tmp | BIT(0)), 7);
	tmp = _mtk_mdio_read_C45(eth, phy_id, 0xFF01, 0x1e);
	_mtk_mdio_write_C45(eth, phy_id, 0xFF01, (tmp | BIT(12)), 0x1e);
	tmp = _mtk_mdio_read_C45(eth, phy_id, 0xFF00, 0x1e);
	_mtk_mdio_write_C45(eth, phy_id, 0xFF00, (tmp | BIT(0)), 0x1e);
	_mtk_mdio_write_C45(eth, phy_id, 0, 0x3200, 7);
}

/*Notice: The following modification is for Single PHY chip and MT6890 flow control
status Sync. This Solution is only for AQR112C Chip. If Single PHY chip is not
AQR112C, the solution is not avaliable.
If Single PHY chip is not AQR112C, and you encounter the flow control mismatch issue,
the following step you should do:
1. Contact your Single PHY chip vendor and get the details of
    - which Interrupt should listen when link state change
    - how to check link status, and flow control status
2. Modify the code for the customization.
*/

/*
Handle Interrupt for AQR112C PHY Link State Change
*/
irqreturn_t mtk_handle_irq_ext(int irq, void *_eth){

	struct mtk_eth *eth = _eth;

	disable_irq_nosync(eth->irq[6]);
	schedule_work(&eth->link_work);
	return IRQ_HANDLED;
}


/*
Handle Interrupt for MT7531 Change
*/
irqreturn_t mtk_handle_irq_ext1(int irq, void *_eth) {

	struct mtk_eth *eth = _eth;

	disable_irq_nosync(eth->irq[7]);
	schedule_work(&eth->link_work1);
	return IRQ_HANDLED;
}


/*
Schedule Task To Check AQR112C PHY Flow Control Status
*/
void mtk_handle_ext_worker_2p5g_phy0(struct work_struct *work){

	struct mtk_eth *eth;

	eth = container_of(work, struct mtk_eth, link_work);
	mtk_mac_status_polling_aqr(eth, 0, AQR112C_PHY0);
	enable_irq(eth->irq[6]);
}


/*
Schedule Task To Check AQR112C PHY Flow Control Status
*/
void mtk_handle_ext_worker(struct work_struct *work){

	struct mtk_eth *eth;

	eth = container_of(work, struct mtk_eth, link_work);
	mtk_mac_status_polling_aqr(eth, 1, AQR112C_PHY0);
	enable_irq(eth->irq[6]);
}



/*
Schedule Task To Check MT7531 Status
*/
void mtk_handle_ext1_worker(struct work_struct *work) {

	struct mtk_eth *eth;

	eth = container_of(work, struct mtk_eth, link_work1);

	mtk_mt7531_polling(eth, 1,
		((netif_running(eth->netdev[1]))? MTK_IF_UP : MTK_IF_DOWN));
	enable_irq(eth->irq[7]);
}



int mtk_phy_event_init(struct mtk_eth *eth, int type) {

	int err;

	if (type == AQR112C_PHY0) {

		INIT_WORK(&eth->link_work, mtk_handle_ext_worker_2p5g_phy0);
		err = devm_request_irq(eth->dev, eth->irq[6], mtk_handle_irq_ext, 0,
			"ext-handler", eth);
		if (err) {
			pr_info("%s fail\n", __func__);
			return err;
		}

	} else if (type == MT7531_PHY1) {

#if defined(CONFIG_MTK_ETHERNET_DYNAMIC_SGMII)

		INIT_WORK(&eth->link_work1, mtk_handle_ext1_worker);
		err = devm_request_irq(eth->dev, eth->irq[7], mtk_handle_irq_ext1, IRQF_SHARED,
			"ext1-handler", eth);
		if (err) {
			pr_info("%s fail\n", __func__);
			return err;
		}
#endif
	} else {
		pr_info("%s fail: type %d unknown\n", __func__, type);
		return -1;
	}
	return 0;
}


