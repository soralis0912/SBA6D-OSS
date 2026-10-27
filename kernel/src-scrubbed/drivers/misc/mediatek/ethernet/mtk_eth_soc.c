// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc.
 */


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
#include <linux/phylink.h>
#include <linux/mdio.h>
#include <linux/regulator/consumer.h>
#include <net/dsa.h>
#include <linux/io.h>

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#include <linux/of_gpio.h>
#define NET_TAG "MTK_ETH_SOC"
#endif
#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"
#include "mtk_common.h"
#include "mtk_sfq.h"
#include "mtk_hwlro_dbg.h"
#if defined(CONFIG_HW_NAT)
#include <net/ra_nat.h>
#endif

#if defined(CONFIG_QUECTEL_KERNRL_RAWDATA_NAND) || defined(CONFIG_QUECTEL_KERNRL_RAWDATA_EMMC)
#include "ql_raw_data_kernel_api.h"
#define __RAWDATA_MAC_ADDR
#endif

#include "mtk_sgmii_pwr.h"
#ifdef CONFIG_MOTORCOMM_PHY
#include "ql_phy_rw_yt8821.h"
#endif

#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
#define NL_INITIATIVE 1
#define NL_PASSIVE 0


#include <linux/miscdevice.h>  //for support mido_tool
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	extern struct mac_phy_of_adapt *mac_phy_adapt;
	void ql_update_eth_data(struct platform_device *pdev, struct mtk_soc_data **soc, \
				struct mac_phy_of_adapt *eth_phy_data, int mac_id);
#endif

#define PHY_WAN_LAN_ADAPT_NETLINK_PORT   51
#define PHY_WAN_LAN_ADAPT_NETLINK_MSG    24
struct sock *phy_nl_fd = NULL;
struct rtlphy_link_status {
	   int     type ;
       int     num ;
       int status ;
       int event_type ;
}phy_state;

struct phy_link_status_def{
       int phy_current;
       struct rtlphy_link_status rtl_phy_link_status[10];
}rtl822x_status;

int phy_netlink_enable = 0;

int ql_get_netlink_status(void)
{
       return phy_netlink_enable;
}
EXPORT_SYMBOL_GPL(ql_get_netlink_status);
int ql_update_link_table(u32 type, int port, int status)
{
	   QLOGD("send to app\n");
	   phy_state.type = type;
       phy_state.num = port;
       phy_state.status = status;
       phy_state.event_type = NL_PASSIVE;
       return 0;

}
EXPORT_SYMBOL_GPL(ql_update_link_table);

int ql_pin_to_gpio(int pin)
{
	if(pin > 0)
		return pin - MTK_GPIO_OFFSET;
	return -1;
}
EXPORT_SYMBOL_GPL(ql_pin_to_gpio);

int ql_gpio_to_pin(int gpio)
{
	if(gpio > 0)
		return gpio + MTK_GPIO_OFFSET;
	return -1;
}
EXPORT_SYMBOL_GPL(ql_gpio_to_pin);
#endif
static int mtk_msg_level = -1;
module_param_named(msg_level, mtk_msg_level, int, 0);
MODULE_PARM_DESC(msg_level, "Message level (-1=defaults,0=none,...,16=all)");

#define MTK_ETHTOOL_STAT(x) { #x, \
			      offsetof(struct mtk_hw_stats, x) / sizeof(u64) }

/* strings used by ethtool */
static const struct mtk_ethtool_stats {
	char str[ETH_GSTRING_LEN];
	u32 offset;
} mtk_ethtool_stats[] = {
	MTK_ETHTOOL_STAT(tx_bytes),
	MTK_ETHTOOL_STAT(tx_packets),
	MTK_ETHTOOL_STAT(tx_skip),
	MTK_ETHTOOL_STAT(tx_collisions),
	MTK_ETHTOOL_STAT(rx_bytes),
	MTK_ETHTOOL_STAT(rx_packets),
	MTK_ETHTOOL_STAT(rx_overflow),
	MTK_ETHTOOL_STAT(rx_fcs_errors),
	MTK_ETHTOOL_STAT(rx_short_errors),
	MTK_ETHTOOL_STAT(rx_long_errors),
	MTK_ETHTOOL_STAT(rx_checksum_errors),
	MTK_ETHTOOL_STAT(rx_flow_control_packets),
};

static const char * const mtk_clks_source_name[] = {
	"ethif", "sgmiitop", "esw", "gp0", "gp1", "gp2", "fe", "trgpll",
	"sgmii_tx250m", "sgmii_rx250m", "sgmii_cdr_ref", "sgmii_cdr_fb",
	"sgmii2_tx250m", "sgmii2_rx250m", "sgmii2_cdr_ref", "sgmii2_cdr_fb",
	"sgmii_ck", "eth2pll", "sgmii_sel", "sgmii_sbus_sel",
	"sgmii_0_sel","sgmii_sbus_0_sel", "sgmii_1_sel", "sgmii_sbus_1_sel",
	"usxgmii_sbus_0_sel",	"usxgmii_sbus_1_sel",
};
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
struct mtk_eth *ext_eth;

EXPORT_SYMBOL_GPL(ext_eth);

u8 ql_match_mac_id_from_phy_addr(int addr)
{
	u8 mac_id = 0;
	if(addr == ext_eth->mac_data[0].sgmii_phy_addr)
	{
		mac_id = 0;
	}
	else if(addr == ext_eth->mac_data[1].sgmii_phy_addr)
	{
		mac_id = 1;
	}
	return mac_id;
}
EXPORT_SYMBOL_GPL(ql_match_mac_id_from_phy_addr);

u8 ql_match_phy_addr_from_mac_id(int mac_id)
{
	u8 addr = 0;
	switch (mac_id)
	{
	case 0:
		addr = ext_eth->mac_data[0].sgmii_phy_addr;
		break;
	case 1:
		addr = ext_eth->mac_data[1].sgmii_phy_addr;
		break;
	default:
		break;
	}
	return addr;
}
EXPORT_SYMBOL_GPL(ql_match_phy_addr_from_mac_id);

static int mtk_change_mtu(struct net_device *dev);
#endif

#ifdef __RAWDATA_MAC_ADDR
char *pop_rawdata_mac_addr(int mac_nr)
{
#if defined(CONFIG_RAWDATA_ENABLE_MODULE)
	return NULL;
#else
	int i = 0;

	static ql_raw_data_item_mac_addr mac_addr_cfg = {0};
	ql_raw_data_item_cache *item_cache;
	uint32_t item_total_len;
	item_total_len = sizeof(ql_raw_data_item_cache) + sizeof(ql_raw_data_item_mac_addr);
	item_cache = kmalloc(item_total_len, GFP_KERNEL);
	memset(item_cache, 0, item_total_len);
	item_cache->head.data_id = RAW_DATA_MAC_ADDR_ID;
	item_cache->head.data_len = sizeof(ql_raw_data_item_mac_addr);

	ql_rawdata_get_item(item_cache);
	memcpy(&mac_addr_cfg, item_cache->data, item_cache->head.data_len);
	pr_info("\n===\n %s dump rawdata mac%d addr\r\n", TAG_RAWDATA, mac_nr);
#if 0
	for (i = 0; i< ETH_ALEN; i++)
	{
		pr_info("%X ", (mac_nr==0? mac_addr_cfg.mac0_addr[i]:mac_addr_cfg.mac1_addr[i]));
	}
	pr_info("\n");
#endif
	if(item_cache != NULL)
	{
		kfree(item_cache);
	}
	return mac_nr==0? mac_addr_cfg.mac0_addr:mac_addr_cfg.mac1_addr;
#endif
}

bool push_rawdata_mac_addr(int mac_nr, const char *mac_addr)
{
	bool ret = false;
	int i = 0;

	const char *tmp_mac_addr_src, *tmp_mac_addr_dst;
	static ql_raw_data_item_mac_addr mac_addr_cfg = {0};
	ql_raw_data_item_cache *item_cache;
	uint32_t item_total_len;
	item_total_len = sizeof(ql_raw_data_item_cache) + sizeof(ql_raw_data_item_mac_addr);
	item_cache = kmalloc(item_total_len, GFP_KERNEL);
	memset(item_cache, 0, item_total_len);
	item_cache->head.data_id = RAW_DATA_MAC_ADDR_ID;
	item_cache->head.data_len = sizeof(ql_raw_data_item_mac_addr);

	//first two mac addr to mac_addr_cfg
	for(i = 0; i< MTK_MAX_DEVS; i++)
	{
		memcpy(&mac_addr_cfg + i*ETH_ALEN, pop_rawdata_mac_addr(i), ETH_ALEN);
	}
	//second copy mac_nr to mac_addr_cfg suitable postion
	tmp_mac_addr_dst = (mac_nr==0?mac_addr_cfg.mac0_addr:mac_addr_cfg.mac1_addr);
	memcpy(tmp_mac_addr_dst, mac_addr, ETH_ALEN);

	//last copy mac_addr_cfg to rawdata
	memcpy(item_cache->data, &mac_addr_cfg, item_cache->head.data_len);
	ret = ql_rawdata_set_item(item_cache);
	if(item_cache != NULL)
	{
		kfree(item_cache);
	}
	return ret;
}

#endif

#if defined(CONFIG_HW_NAT)
static struct net_device *mtk_fast_path[MAX_IF_NUM] = {0};

/* The fast paths transmit skb to PPE via Netsys eth.
 * We register them to HWNAT module only when at least one Netsys eth is up.
 * Confirm the fast paths were hooked before Netsys eth goes up.
 */

void mtk_hook_fast_path(struct net_device *dev)
{
	int i = 0;

	for (i = 0; i < MAX_IF_NUM; i++) {
		if (mtk_fast_path[i] == dev) {
			pr_notice("%s : %s fast path hooked(%d)\n",
				  __func__, dev->name, i);
			return;
		}
		if (!mtk_fast_path[i]) {
			mtk_fast_path[i] = dev;
			pr_notice("%s : %s fast path(%d)\n",
				  __func__, dev->name, i);
			return;
		}
	}
}

static void mtk_register_fast_path(void)
{
	int i = 0;

#ifdef CONFIG_MTK_HNAT_FAST_SUPPORT
	return;
#endif

	if (ppe_dev_register_hook) {
		for (i = 0; i < MAX_IF_NUM; i++) {
			if (mtk_fast_path[i]) {
				ppe_dev_register_hook(mtk_fast_path[i]);
				pr_notice("%s : %s fast path(%d)",
					  __func__, mtk_fast_path[i]->name, i);
			}
		}
	}
}

static void mtk_unregister_fast_path(void)
{
	int i = 0;

#ifdef CONFIG_MTK_HNAT_FAST_SUPPORT
	return;
#endif

	if (ppe_dev_register_hook) {
		for (i = 0; i < MAX_IF_NUM; i++) {
			if (mtk_fast_path[i]) {
				ppe_dev_unregister_hook(mtk_fast_path[i]);
				pr_notice("%s : %s fast path(%d)",
					  __func__, mtk_fast_path[i]->name, i);
			}
		}
	}
}
#endif

struct eth_alloc_cache {
	struct page_frag_cache page;
};
static DEFINE_PER_CPU(struct eth_alloc_cache, eth_alloc_cache);


static void *mtketh_alloc_frag(unsigned int fragsz)
{
	struct eth_alloc_cache *nc = this_cpu_ptr(&eth_alloc_cache);
	unsigned int fragszz = SKB_DATA_ALIGN(fragsz);

	return page_frag_alloc(&nc->page, fragszz, GFP_ATOMIC | GFP_DMA32);
}



void mtk_w32(struct mtk_eth *eth, u32 val, unsigned reg)
{
	__raw_writel(val, eth->base + reg);
}

u32 mtk_r32(struct mtk_eth *eth, unsigned reg)
{
	return __raw_readl(eth->base + reg);
}

u32 mtk_m32(struct mtk_eth *eth, u32 mask, u32 set, unsigned reg)
{
	u32 val;

	val = mtk_r32(eth, reg);
	val &= ~mask;
	val |= set;
	mtk_w32(eth, val, reg);
	return reg;
}

static int mtk_mdio_busy_wait(struct mtk_eth *eth)
{
	unsigned long t_start = jiffies;

	while (1) {
		if (!(mtk_r32(eth, MTK_PHY_IAC) & PHY_IAC_ACCESS))
			return 0;
		if (time_after(jiffies, t_start + PHY_IAC_TIMEOUT))
			break;
		cond_resched();
	}

	dev_err(eth->dev, "mdio: MDIO timeout\n");
	return -1;
}

static u32 _mtk_mdio_write(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data)
{
	if (mtk_mdio_busy_wait(eth))
		return -1;

	write_data &= 0xffff;

	mtk_w32(eth, PHY_IAC_ACCESS | PHY_IAC_START | PHY_IAC_WRITE |
		(phy_register << PHY_IAC_REG_SHIFT) |
		(phy_addr << PHY_IAC_ADDR_SHIFT) | write_data,
		MTK_PHY_IAC);

	if (mtk_mdio_busy_wait(eth))
		return -1;

	return 0;
}

static u32 _mtk_mdio_read(struct mtk_eth *eth, int phy_addr, int phy_reg)
{
	u32 d;

	if (mtk_mdio_busy_wait(eth))
		return 0xffff;

	mtk_w32(eth, PHY_IAC_ACCESS | PHY_IAC_START | PHY_IAC_READ |
		(phy_reg << PHY_IAC_REG_SHIFT) |
		(phy_addr << PHY_IAC_ADDR_SHIFT),
		MTK_PHY_IAC);

	if (mtk_mdio_busy_wait(eth))
		return 0xffff;

	d = mtk_r32(eth, MTK_PHY_IAC) & 0xffff;

	return d;
}

static int mtk_mdio_write(struct mii_bus *bus, int phy_addr,
			  int phy_reg, u16 val)
{
	struct mtk_eth *eth = bus->priv;
	u32 devad;

	if (phy_reg & MII_ADDR_C45) {
		devad = (phy_reg >> 16) & 0x1f;
		return _mtk_mdio_write_C45(eth, phy_addr, phy_reg, val, devad);

	} else {
		return _mtk_mdio_write(eth, phy_addr, phy_reg, val);
	}
}

static int mtk_mdio_read(struct mii_bus *bus, int phy_addr, int phy_reg)
{
	struct mtk_eth *eth = bus->priv;
	u16 dev_addr, devad;
	u32 tmp;

	if (phy_reg & MII_ADDR_C45) {
		dev_addr = (phy_reg >> 16) & 0x1f;

		tmp = _mtk_mdio_read_C45(eth, phy_addr, (phy_reg & 0xffff), dev_addr);

		return tmp;
	} else {

		return _mtk_mdio_read(eth, phy_addr, phy_reg);
	}
}

static int mt7621_gmac0_rgmii_adjust(struct mtk_eth *eth,
				     phy_interface_t interface)
{
	u32 val;

	/* Check DDR memory type.
	 * Currently TRGMII mode with DDR2 memory is not supported.
	 */
	regmap_read(eth->ethsys, ETHSYS_SYSCFG, &val);
	if (interface == PHY_INTERFACE_MODE_TRGMII &&
	    val & SYSCFG_DRAM_TYPE_DDR2) {
		dev_err(eth->dev,
			"TRGMII mode with DDR2 memory is not supported!\n");
		return -EOPNOTSUPP;
	}

	val = (interface == PHY_INTERFACE_MODE_TRGMII) ?
		ETHSYS_TRGMII_MT7621_DDR_PLL : 0;

	regmap_update_bits(eth->ethsys, ETHSYS_CLKCFG0,
			   ETHSYS_TRGMII_MT7621_MASK, val);

	return 0;
}

static void mtk_gmac0_rgmii_adjust(struct mtk_eth *eth,
				   phy_interface_t interface, int speed)
{
	u32 val;
	int ret;

	if (interface == PHY_INTERFACE_MODE_TRGMII) {
		mtk_w32(eth, TRGMII_MODE, INTF_MODE);
		val = 500000000;
		ret = clk_set_rate(eth->clks[MTK_CLK_TRGPLL], val);
		if (ret)
			dev_err(eth->dev, "Failed to set trgmii pll: %d\n", ret);
		return;
	}

	val = (speed == SPEED_1000) ?
		INTF_MODE_RGMII_1000 : INTF_MODE_RGMII_10_100;
	mtk_w32(eth, val, INTF_MODE);

	regmap_update_bits(eth->ethsys, ETHSYS_CLKCFG0,
			   ETHSYS_TRGMII_CLK_SEL362_5,
			   ETHSYS_TRGMII_CLK_SEL362_5);

	val = (speed == SPEED_1000) ? 250000000 : 500000000;
	ret = clk_set_rate(eth->clks[MTK_CLK_TRGPLL], val);
	if (ret)
		dev_err(eth->dev, "Failed to set trgmii pll: %d\n", ret);

	val = (speed == SPEED_1000) ?
		RCK_CTRL_RGMII_1000 : RCK_CTRL_RGMII_10_100;
	mtk_w32(eth, val, TRGMII_RCK_CTRL);

	val = (speed == SPEED_1000) ?
		TCK_CTRL_RGMII_1000 : TCK_CTRL_RGMII_10_100;
	mtk_w32(eth, val, TRGMII_TCK_CTRL);
}

void ql_phylink_fixed_state(struct net_device *dev,struct phylink_link_state *state)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	state->link = mac->ql_sgmii_phy_data.link;
	// pr_err("%s.%d mac:%d link:%d \n", __func__, __LINE__, mac->id, state->link);
}

static void mtk_mac_config(struct phylink_config *config, unsigned int mode,
			   const struct phylink_link_state *state)
{
	struct mtk_mac *mac = container_of(config, struct mtk_mac,
					   phylink_config);
	struct mtk_eth *eth = mac->hw;
	u32 mcr_cur = 0, mcr_new = 0, sid = 0, i = 0;
	int val, ge_mode, err = 0;

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	if(state->interface == PHY_INTERFACE_MODE_2500BASEX)
	{
		if((mac->id == 0 && mac->hw->soc->rtl8221_phy0_support) || (mac->id == 1 && mac->hw->soc->rtl8221_phy1_support))
		{
			int link_changed = ql_phy_pop_link_change(eth, mac->id);

			if(link_changed == 0 && eth->mac_data[mac->id].is_def_cfg != 1)
			{
				// dev_err(eth->dev, "%s:Ignore MAC SGMII setting Because link is equality\n", __func__);
				return;
			}
		}
	}
#endif

	if ((MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK))) {
		dev_err(eth->dev, "%s:Ignore MAC SGMII setting for FPGA\n", __func__);
		return;
	}

	/* MT76x8 has no hardware settings between for the MAC */
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628) &&
	    mac->interface != state->interface) {

		/* Setup soc pin functions */
		switch (state->interface) {
		case PHY_INTERFACE_MODE_TRGMII:
			if (mac->id)
				goto err_phy;
			if (!MTK_HAS_CAPS(mac->hw->soc->caps,
					  MTK_GMAC1_TRGMII))
				goto err_phy;
			/* fall through */
		case PHY_INTERFACE_MODE_RGMII_TXID:
		case PHY_INTERFACE_MODE_RGMII_RXID:
		case PHY_INTERFACE_MODE_RGMII_ID:
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_MII:
		case PHY_INTERFACE_MODE_REVMII:
		case PHY_INTERFACE_MODE_RMII:
			if (MTK_HAS_CAPS(eth->soc->caps, MTK_RGMII)) {
				err = mtk_gmac_rgmii_path_setup(eth, mac->id);
				if (err)
					goto init_err;
			}
			break;
		case PHY_INTERFACE_MODE_1000BASEX:
		case PHY_INTERFACE_MODE_2500BASEX:
		case PHY_INTERFACE_MODE_SGMII:
			if (MTK_HAS_CAPS(eth->soc->caps, MTK_SGMII)) {
				err = mtk_gmac_sgmii_path_setup(eth, mac->id);
				if (err)
					goto init_err;
			}
			break;
		case PHY_INTERFACE_MODE_GMII:
			if (MTK_HAS_CAPS(eth->soc->caps, MTK_GEPHY)) {
				err = mtk_gmac_gephy_path_setup(eth, mac->id);
				if (err)
					goto init_err;
			}
			break;
		case PHY_INTERFACE_MODE_USXGMII:
		case PHY_INTERFACE_MODE_10GKR:
			if (MTK_HAS_CAPS(eth->soc->caps, MTK_USXGMII)) {
				err = mtk_gmac_usxgmii_path_setup(eth, mac->id);
				if (err)
					goto init_err;
			}
			break;
		default:
			goto err_phy;
		}

		/* Setup clock for 1st gmac */
		if (!mac->id && state->interface != PHY_INTERFACE_MODE_SGMII &&
		    !phy_interface_mode_is_8023z(state->interface) &&
		    MTK_HAS_CAPS(mac->hw->soc->caps, MTK_GMAC1_TRGMII)) {
			if (MTK_HAS_CAPS(mac->hw->soc->caps,
					 MTK_TRGMII_MT7621_CLK)) {
				if (mt7621_gmac0_rgmii_adjust(mac->hw,
							      state->interface))
					goto err_phy;
			} else {
				mtk_gmac0_rgmii_adjust(mac->hw,
						       state->interface,
						       state->speed);

				/* mt7623_pad_clk_setup */
				for (i = 0 ; i < NUM_TRGMII_CTRL; i++)
					mtk_w32(mac->hw,
						TD_DM_DRVP(8) | TD_DM_DRVN(8),
						TRGMII_TD_ODT(i));

				/* Assert/release MT7623 RXC reset */
				mtk_m32(mac->hw, 0, RXC_RST | RXC_DQSISEL,
					TRGMII_RCK_CTRL);
				mtk_m32(mac->hw, RXC_RST, 0, TRGMII_RCK_CTRL);
			}
		}

		ge_mode = 0;
		switch (state->interface) {
		case PHY_INTERFACE_MODE_MII:
		case PHY_INTERFACE_MODE_GMII:
			ge_mode = 1;
			break;
		case PHY_INTERFACE_MODE_REVMII:
			ge_mode = 2;
			break;
		case PHY_INTERFACE_MODE_RMII:
			if (mac->id)
				goto err_phy;
			ge_mode = 3;
			break;
		default:
			break;
		}

		/* put the gmac into the right mode */
		//for CPE product ge_mode always 0, so skip it
		/*regmap_read(eth->ethsys, ETHSYS_SYSCFG0, &val);
		val &= ~SYSCFG0_GE_MODE(SYSCFG0_GE_MASK, mac->id);
		val |= SYSCFG0_GE_MODE(ge_mode, mac->id);
		regmap_write(eth->ethsys, ETHSYS_SYSCFG0, val);*/

		mac->interface = state->interface;
	}

	/* SGMII */
	if (state->interface == PHY_INTERFACE_MODE_SGMII ||
	    phy_interface_mode_is_8023z(state->interface)) {
		/* The path GMAC to SGMII will be enabled once the SGMIISYS is
		 * being setup done.
		 */
		 /* unnecessary code
		regmap_read(eth->ethsys, ETHSYS_SYSCFG0, &val);

		regmap_update_bits(eth->ethsys, ETHSYS_SYSCFG0,
				   SYSCFG0_SGMII_MASK,
				   ~(u32)SYSCFG0_SGMII_MASK);
		*/

		/* Decide how GMAC and SGMIISYS be mapped */
		sid = (MTK_HAS_CAPS(eth->soc->caps, MTK_SHARED_SGMII)) ?
		       0 : mac->id;

#if defined(CONFIG_MTK_ETHERNET_DYNAMIC_SGMII)
		//check dynamic sgmii status
		if (mac->gmii_decision == 0) {
			pr_info("%s: [%d] dynamic sgmii force disable", __func__, sid);
		} else {
#endif
		if (mac->sgmii_force_en == -1) {  //force disable
			pr_info("%s: [%d] force sgmii disable, skip sgmii init", __func__, sid);
		} else 	if (state->interface != PHY_INTERFACE_MODE_SGMII) {
			pr_info("%s: [%d] mtk_sgmii_setup_mode_force", __func__, sid);
			err = mtk_sgmii_setup_mode_force(eth->sgmii, sid, state->speed);
		} else if (phylink_autoneg_inband(mode)) {
			pr_info("%s: [%d] dmtk_sgmii_setup_mode_an", __func__, sid);
			err = mtk_sgmii_setup_mode_an(eth->sgmii, sid, eth, state->speed);
		}
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
		else if((mac->id == 0 && mac->hw->soc->en8811_phy0_support) || (mac->id == 1 && mac->hw->soc->en8811_phy1_support) || \
				(mac->id == 0 && mac->hw->soc->yt8821_phy0_support) || (mac->id == 1 && mac->hw->soc->yt8821_phy1_support))
		{
			pr_info("%s: [%d] mtk_sgmii_setup_mode_force SPEED_2500", __func__, sid);
			err = mtk_sgmii_setup_mode_force(eth->sgmii, sid, SPEED_2500);
		}
#endif
		else {
			pr_info("%s: [%d] interface Error: Go AN mode", __func__, sid);
			err = mtk_sgmii_setup_mode_an(eth->sgmii, sid, eth, state->speed);
		}
		if (err) {
			goto init_err;
		}
#if defined(CONFIG_MTK_ETHERNET_DYNAMIC_SGMII)
		}
#endif

		/* unnecessary code
		regmap_update_bits(eth->ethsys, ETHSYS_SYSCFG0,
				   SYSCFG0_SGMII_MASK, val);
		*/

	} else if (state->interface == PHY_INTERFACE_MODE_USXGMII ||
		state->interface == PHY_INTERFACE_MODE_10GKR) {

		sid = mac->id;

		if(state->speed != SPEED_UNKNOWN) {
			err = mtk_usxgmii_setup_mode_an(eth->sgmii, sid, mac->max_speed);
			if (err)
				goto init_err;
		}

	} else if (phylink_autoneg_inband(mode)) {
		dev_err(eth->dev,
			"In-band mode not supported in non SGMII mode!\n");
		return;
	}

	/* Setup gmac */

	if (state->interface == PHY_INTERFACE_MODE_USXGMII ||
		state->interface == PHY_INTERFACE_MODE_10GKR) {
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
		mtk_w32(mac->hw, 0x80000000, 0x504 + mac->id*0x1000);
		mtk_w32(mac->hw, 0x00008000, MTK_MAC_MCR(mac->id));
#else
		mtk_w32(mac->hw, 0x80000000, 0x504);
		mtk_w32(mac->hw, 0x80000000, 0x1504);

		mtk_w32(mac->hw, 0x00008000, MTK_MAC_MCR(0));
		mtk_w32(mac->hw, 0x00008000, MTK_MAC_MCR(1));
#endif

	} else {
		mcr_cur = mtk_r32(mac->hw, MTK_MAC_MCR(mac->id));
		mcr_new = mcr_cur;
		mcr_new &= ~(MAC_MCR_SPEED_100 | MAC_MCR_SPEED_1000 |
			     MAC_MCR_FORCE_DPX | MAC_MCR_FORCE_TX_FC |
			     MAC_MCR_FORCE_RX_FC);
		mcr_new |= MAC_MCR_MAX_RX_1536 | MAC_MCR_IPG_CFG | MAC_MCR_FORCE_MODE |
			   MAC_MCR_BACKOFF_EN | MAC_MCR_BACKPR_EN | MAC_MCR_FORCE_LINK;

		switch (state->speed) {
		case SPEED_2500:
		case SPEED_1000:
			mcr_new |= MAC_MCR_SPEED_1000;
			break;
		case SPEED_100:
			mcr_new |= MAC_MCR_SPEED_100;
			break;
		}
		if (state->duplex == DUPLEX_FULL) {
			mcr_new |= MAC_MCR_FORCE_DPX;
			if (state->pause & MLO_PAUSE_TX)
				mcr_new |= MAC_MCR_FORCE_TX_FC;
			if (state->pause & MLO_PAUSE_RX)
				mcr_new |= MAC_MCR_FORCE_RX_FC;
		}

		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
			mcr_new &= (~MAC_MCR_DIFF_RXFIFO_WCLR);
			if (mac->id == 0)
				mcr_new |= MAC_MCR_DEL_RXFIFO_CLR;
		}
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
		mtk_change_mtu(eth->netdev[mac->id]);
#endif

		/* Only update control register when needed! */
		if (mcr_new != mcr_cur) {

		if (mac->sgmii_force_en == -1) {  //force disable
			mcr_new &= ~(MAC_MCR_FORCE_LINK);
			pr_info("%s: [%d] force sgmii disable new mcr=%x",
				__func__, sid, mcr_new);
		}
#if defined(CONFIG_MTK_ETHERNET_DYNAMIC_SGMII)
			if (mac->gmii_decision == 0) {
				mcr_new &= ~(MAC_MCR_FORCE_LINK);
			}
#endif
			mtk_w32(mac->hw, mcr_new, MTK_MAC_MCR(mac->id));
			dev_err(eth->dev, "%s: mac[%d] MAC_MCR: %x\n", __func__, mac->id, mcr_new);

			if (mac->id == 0 && mac->hw->soc->aqr112c_phy0_support && MTK_PHY_FC_UPDATE) {
				mtk_mac_status_polling_aqr(eth, mac->id, AQR112C_PHY0);
			}
		}
	}
	return;

err_phy:
	dev_err(eth->dev, "%s: GMAC%d mode %s not supported!\n", __func__,
		mac->id, phy_modes(state->interface));
	return;

init_err:
	dev_err(eth->dev, "%s: GMAC%d mode %s err: %d!\n", __func__,
		mac->id, phy_modes(state->interface), err);
}

static int mtk_mac_link_state(struct phylink_config *config,
			      struct phylink_link_state *state)
{
	struct mtk_mac *mac = container_of(config, struct mtk_mac,
					   phylink_config);
	u32 pmsr = mtk_r32(mac->hw, MTK_MAC_MSR(mac->id));
	u32 gmac_sts = mtk_r32(mac->hw, MTK_XGMAC_STS);
	u32 val = 0, phy_id = 0;

	if ((mac->hw->soc->aqr113c_phy0_support && mac->id == 0) || \
		(mac->hw->soc->aqr113c_phy1_support && mac->id == 1) || \
		(mac->hw->soc->rtl8251_phy0_support && mac->id == 0) || \
		(mac->hw->soc->rtl8251_phy1_support && mac->id == 1)) {

		gmac_sts = mtk_r32(mac->hw, MTK_XGMAC_STS);

		val = (mac->id == 0) ? ((gmac_sts &  0xE) >> 1) :
			((gmac_sts &  0xE0000) >> 17);
		switch (val) {
			case 0x0:
				state->speed = SPEED_10000;
				break;
			case 0x1:
				state->speed = SPEED_5000;
				break;
			case 0x2:
				state->speed = SPEED_2500;
				break;
			case 0x3:
				state->speed = SPEED_1000;
				break;
			default:
				state->speed = SPEED_100;
		}

		state->duplex = 1;
		state->link = (mac->id == 0) ? ((gmac_sts &  0x1)) :
			((gmac_sts &  0x10000) >> 16);
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
		phy_id = ql_match_phy_addr_from_mac_id(mac->id);
#else
		phy_id = (mac->id == 0) ? 8 : 0;
#endif

		state->pause =
			(_mtk_mdio_read_C45(mac->hw, phy_id, 0x13, 0x7) & 0x400) >> 10;

		dev_err(mac->hw->dev,
			"%s: mac (%d) sts= %x speed %d link %x pause %x\n",
			__func__, mac->id, gmac_sts, state->speed, state->link,
			state->pause);

	} else {
		state->link = (pmsr & MAC_MSR_LINK);
		state->duplex = (pmsr & MAC_MSR_DPX) >> 1;

		switch (pmsr & (MAC_MSR_SPEED_1000 | MAC_MSR_SPEED_100)) {
		case 0:
			state->speed = SPEED_10;
			break;
		case MAC_MSR_SPEED_100:
			state->speed = SPEED_100;
			break;
		case MAC_MSR_SPEED_1000:
			state->speed = SPEED_1000;
			break;
		default:
			state->speed = SPEED_UNKNOWN;
			break;
		}

		state->pause &= (MLO_PAUSE_RX | MLO_PAUSE_TX);
		if (pmsr & MAC_MSR_RX_FC)
			state->pause |= MLO_PAUSE_RX;
		if (pmsr & MAC_MSR_TX_FC)
			state->pause |= MLO_PAUSE_TX;

		dev_err(mac->hw->dev,
			"%s: mac (%d) duplex %d speed %d link %x pause %x\n",
			__func__, mac->id, state->duplex, state->speed, state->link,
			state->pause);
	}
	return 1;
}

static void mtk_mac_an_restart(struct phylink_config *config)
{
	struct mtk_mac *mac = container_of(config, struct mtk_mac,
					   phylink_config);

	if ((MTK_HAS_CAPS(mac->hw->soc->caps, MTK_FPGA_CLK))) {
		dev_err(mac->hw->dev, "%s:Ignore MAC SGMII setting for FPGA\n", __func__);
		return;
	}

	mtk_sgmii_restart_an(mac->hw, mac->id);
}


static void mtk_pse_port_link_set(struct mtk_mac *mac, bool up)
{
	u32 fe_glo_cfg = 0;

	fe_glo_cfg = mtk_r32(mac->hw, MTK_FE_GLO_CFG);
	if (!up) {
		fe_glo_cfg |= (mac->id == MTK_GMAC1_ID) ?
			MTK_FE_LINK_DOWN_P1 : MTK_FE_LINK_DOWN_P2;
		dev_err(mac->hw->dev, "%s: mac[%d] fe_glo_cfg = %x\n",
			__func__, mac->id, fe_glo_cfg);
		mtk_w32(mac->hw, fe_glo_cfg, MTK_FE_GLO_CFG);
	} else {
		fe_glo_cfg &= ~((mac->id == MTK_GMAC1_ID) ?
			MTK_FE_LINK_DOWN_P1 : MTK_FE_LINK_DOWN_P2);
		mtk_w32(mac->hw, fe_glo_cfg, MTK_FE_GLO_CFG);
		dev_err(mac->hw->dev, "%s: mac[%d] fe_glo_cfg = %x\n",
			__func__, mac->id, fe_glo_cfg);
	}
}


static void mtk_mac_link_down(struct phylink_config *config, unsigned int mode,
			      phy_interface_t interface)
{
	struct mtk_mac *mac = container_of(config, struct mtk_mac,
					   phylink_config);
	u32 mcr;

	if ((mac->hw->soc->aqr113c_phy0_support && mac->id == 0) || \
		(mac->hw->soc->aqr113c_phy1_support && mac->id == 1) || \
		(mac->hw->soc->rtl8251_phy0_support && mac->id == 0) || \
		(mac->hw->soc->rtl8251_phy1_support && mac->id == 1)) {
		mcr = mtk_r32(mac->hw, MTK_XMAC_MCR(mac->id));
		mcr &= 0xfffffff0;
		mcr |= XMAC_MCR_TRX_DISABLE;
		mtk_w32(mac->hw, mcr, MTK_XMAC_MCR(mac->id));

		//disable usxgmii
		mtk_eth_sgmii_runtime_set(mac->hw, mac->id, 0, MTK_IF_DOWN);

	} else {
		mtk_pse_port_link_set(mac, false);
		mcr = mtk_r32(mac->hw, MTK_MAC_MCR(mac->id));
		mcr &= ~(MAC_MCR_TX_EN | MAC_MCR_RX_EN);
		if(interface == PHY_INTERFACE_MODE_SGMII) {
			mcr &= ~MAC_MCR_FORCE_LINK;
		}
		mtk_w32(mac->hw, mcr, MTK_MAC_MCR(mac->id));

#if defined(CONFIG_MTK_ETHERNET_DYNAMIC_SGMII)
		if (mac->id == 1 && mac->hw->soc->mt7531_phy1_support) {
			mtk_eth_sgmii_runtime_status_action(mac->hw, mac->id, MTK_IF_DOWN);
		}
		if (mac->id == 0 && mac->hw->soc->mt7531_phy0_support) {
			mtk_eth_sgmii_runtime_status_action(mac->hw, mac->id, MTK_IF_DOWN);
		}
#endif
	}

	dev_err(mac->hw->dev, "%s: mac[%d] mcr = %x\n", __func__,mac->id, MTK_MAC_MCR(mac->id));

}

static void mtk_mac_link_up(struct phylink_config *config,
	struct phy_device *phy,
	unsigned int mode, phy_interface_t interface,
	int speed, int duplex, bool tx_pause, bool rx_pause){

	struct mtk_mac *mac = container_of(config, struct mtk_mac,
		phylink_config);
	u32 mcr;
	u32 lp_pause = 0, phy_id = 0;

	if ((mac->hw->soc->aqr113c_phy0_support && mac->id == 0) || \
		(mac->hw->soc->aqr113c_phy1_support && mac->id == 1) || \
		(mac->hw->soc->rtl8251_phy0_support && mac->id == 0) || \
		(mac->hw->soc->rtl8251_phy1_support && mac->id == 1)) {
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
		phy_id = ql_match_phy_addr_from_mac_id(mac->id);
#else
		phy_id = (mac->id == 0)? 8 : 0;
#endif
		lp_pause = (_mtk_mdio_read_C45(mac->hw, phy_id, 0x13, 0x7) & 0x400) >> 10;

		mcr = mtk_r32(mac->hw, MTK_XMAC_MCR(mac->id));
		mcr &= ~(MAC_MCR_FORCE_TX_FC |	MAC_MCR_FORCE_RX_FC);
		if (lp_pause) {
			mcr |= MAC_MCR_FORCE_TX_FC;
			mcr |= MAC_MCR_FORCE_RX_FC;
		}
		mcr &= ~(XMAC_MCR_TRX_DISABLE);
		mtk_w32(mac->hw, mcr, MTK_XMAC_MCR(mac->id));
		dev_err(mac->hw->dev, "%s: mac[%d] mcr = %x pause =%x\n", __func__,
			mac->id, mcr, lp_pause);

	} else {
		mcr = mtk_r32(mac->hw, MTK_MAC_MCR(mac->id));

		mcr &= ~(MAC_MCR_SPEED_100 | MAC_MCR_SPEED_1000 |
			MAC_MCR_FORCE_DPX | MAC_MCR_FORCE_TX_FC |
			MAC_MCR_FORCE_RX_FC);
		/* Configure speed */
		switch (speed) {
			case SPEED_2500:
			case SPEED_1000:
				mcr |= MAC_MCR_SPEED_1000;
				break;
			case SPEED_100:
				mcr |= MAC_MCR_SPEED_100;
				break;
			}
		/* Configure duplex */
		if (duplex == DUPLEX_FULL)
			mcr |= MAC_MCR_FORCE_DPX;
		/* Configure pause modes - phylink will avoid these for half duplex */
		if (tx_pause)
			mcr |= MAC_MCR_FORCE_TX_FC;
		if (rx_pause)
			mcr |= MAC_MCR_FORCE_RX_FC;

		mcr |= MAC_MCR_TX_EN | MAC_MCR_RX_EN;


		if (mac->sgmii_force_en == -1) { //force disable
			mcr &= ~(MAC_MCR_TX_EN | MAC_MCR_RX_EN);
		}

		mtk_w32(mac->hw, mcr, MTK_MAC_MCR(mac->id));
		mtk_pse_port_link_set(mac, true);

		dev_err(mac->hw->dev, "%s: mac[%d] mcr = 0x%x speed:%d duplex:%d tx_pause:%d rx_pause:%d", __func__,
			mac->id, mcr, speed, duplex, tx_pause, rx_pause);

		if (mac->id == 0 && mac->hw->soc->aqr112c_phy0_support) {
			mtk_mac_status_polling_aqr(mac->hw, mac->id, AQR112C_PHY0);
		}

#if defined(CONFIG_MTK_ETHERNET_DYNAMIC_SGMII)
		if (mac->id == 1 && mac->hw->soc->mt7531_phy1_support) {
			mtk_mt7531_polling(mac->hw, mac->id, MTK_IF_UP);
		}
		if (mac->id == 0 && mac->hw->soc->mt7531_phy0_support) {
			mtk_mt7531_polling(mac->hw, mac->id, MTK_IF_UP);
		}
#endif
	}
}

static void mtk_validate(struct phylink_config *config,
			 unsigned long *supported,
			 struct phylink_link_state *state)
{
	struct mtk_mac *mac = container_of(config, struct mtk_mac,
					   phylink_config);

	__ETHTOOL_DECLARE_LINK_MODE_MASK(mask) = { 0, };

	if (state->interface != PHY_INTERFACE_MODE_NA &&
	    state->interface != PHY_INTERFACE_MODE_MII &&
	    state->interface != PHY_INTERFACE_MODE_GMII &&
	    !(MTK_HAS_CAPS(mac->hw->soc->caps, MTK_RGMII) &&
	      phy_interface_mode_is_rgmii(state->interface)) &&
	    !(MTK_HAS_CAPS(mac->hw->soc->caps, MTK_TRGMII) &&
	      !mac->id && state->interface == PHY_INTERFACE_MODE_TRGMII) &&
	    !(MTK_HAS_CAPS(mac->hw->soc->caps, MTK_SGMII) &&
	      (state->interface == PHY_INTERFACE_MODE_SGMII ||
		phy_interface_mode_is_8023z(state->interface))) &&
		!(MTK_HAS_CAPS(mac->hw->soc->caps, MTK_USXGMII) &&
				  (state->interface == PHY_INTERFACE_MODE_USXGMII)) &&
		!(MTK_HAS_CAPS(mac->hw->soc->caps, MTK_USXGMII) &&
						  (state->interface == PHY_INTERFACE_MODE_10GKR))
	) {
		pr_err("%s.%d linkmode_zero!\n", __func__, __LINE__);
		linkmode_zero(supported);
		return;
	}

	phylink_set_port_modes(mask);
	phylink_set(mask, Autoneg);

	switch (state->interface) {
	case PHY_INTERFACE_MODE_USXGMII:
	case PHY_INTERFACE_MODE_10GKR:

		phylink_set(mask, 10000baseKR_Full);
		phylink_set(mask, 10000baseT_Full);
		phylink_set(mask, 10000baseCR_Full);
		phylink_set(mask, 10000baseSR_Full);
		phylink_set(mask, 10000baseLR_Full);
		phylink_set(mask, 10000baseLRM_Full);
		phylink_set(mask, 10000baseER_Full);
		phylink_set(mask, 10baseT_Half);
		phylink_set(mask, 10baseT_Full);
		phylink_set(mask, 100baseT_Half);
		phylink_set(mask, 100baseT_Full);
		phylink_set(mask, 1000baseT_Half);
		phylink_set(mask, 1000baseT_Full);
		phylink_set(mask, 1000baseX_Full);
		break;
	case PHY_INTERFACE_MODE_TRGMII:
		phylink_set(mask, 1000baseT_Full);
		break;
	case PHY_INTERFACE_MODE_1000BASEX:
	case PHY_INTERFACE_MODE_2500BASEX:
		phylink_set(mask, 1000baseX_Full);
		phylink_set(mask, 2500baseX_Full);
		break;
	case PHY_INTERFACE_MODE_GMII:
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		phylink_set(mask, 1000baseT_Half);
		/* fall through */
	case PHY_INTERFACE_MODE_SGMII:
		phylink_set(mask, 1000baseT_Full);
		phylink_set(mask, 1000baseX_Full);
		/* fall through */
	case PHY_INTERFACE_MODE_MII:
	case PHY_INTERFACE_MODE_RMII:
	case PHY_INTERFACE_MODE_REVMII:
	case PHY_INTERFACE_MODE_NA:
	default:
		phylink_set(mask, 10baseT_Half);
		phylink_set(mask, 10baseT_Full);
		phylink_set(mask, 100baseT_Half);
		phylink_set(mask, 100baseT_Full);
		break;
	}

	if (state->interface == PHY_INTERFACE_MODE_NA) {
		if (MTK_HAS_CAPS(mac->hw->soc->caps, MTK_USXGMII)) {

			phylink_set(mask, 10000baseKR_Full);
			phylink_set(mask, 10000baseSR_Full);
			phylink_set(mask, 10000baseLR_Full);
			phylink_set(mask, 10000baseLRM_Full);
			phylink_set(mask, 10000baseER_Full);
			phylink_set(mask,1000baseKX_Full);
			phylink_set(mask, 1000baseT_Full);
			phylink_set(mask, 1000baseX_Full);
			phylink_set(mask, 2500baseX_Full);
		}
		if (MTK_HAS_CAPS(mac->hw->soc->caps, MTK_SGMII)) {
			phylink_set(mask, 1000baseT_Full);
			phylink_set(mask, 1000baseX_Full);
			phylink_set(mask, 2500baseX_Full);
		}
		if (MTK_HAS_CAPS(mac->hw->soc->caps, MTK_RGMII)) {
			phylink_set(mask, 1000baseT_Full);
			phylink_set(mask, 1000baseT_Half);
			phylink_set(mask, 1000baseX_Full);
		}
		if (MTK_HAS_CAPS(mac->hw->soc->caps, MTK_GEPHY)) {
			phylink_set(mask, 1000baseT_Full);
			phylink_set(mask, 1000baseT_Half);
		}
	}

	phylink_set(mask, Pause);
	phylink_set(mask, Asym_Pause);
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	if ((mac->id == 0 && mac->hw->soc->en8801_phy0_support) ||
	 	(mac->id == 1 && mac->hw->soc->en8801_phy1_support) ||
		(mac->id == 0 && mac->hw->soc->en8811_phy0_support) ||
	 	(mac->id == 1 && mac->hw->soc->en8811_phy1_support) ||
		(mac->id == 0 && mac->hw->soc->yt8821_phy0_support) ||
		(mac->id == 1 && mac->hw->soc->yt8821_phy1_support)) {
		*supported = mask;
	}
	if ((mac->id == 0 && mac->hw->soc->aqr113c_phy0_support) ||
		(mac->id == 1 && mac->hw->soc->aqr113c_phy1_support) ||
		(mac->id == 0 && mac->hw->soc->rtl8251_phy0_support) ||
		(mac->id == 1 && mac->hw->soc->rtl8251_phy1_support)) {
		phylink_set(mask, 10000baseKR_Full);
		*supported = mask;
	} else {
#endif
		linkmode_and(supported, supported, mask);
		linkmode_and(state->advertising, state->advertising, mask);
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	}
#endif

	/* We can only operate at 2500BaseX or 1000BaseX. If requested
	 * to advertise both, only report advertising at 2500BaseX.
	 */
	phylink_helper_basex_speed(state);
}

static const struct phylink_mac_ops mtk_phylink_ops = {
	.validate = mtk_validate,
	.mac_link_state = mtk_mac_link_state,
	.mac_an_restart = mtk_mac_an_restart,
	.mac_config = mtk_mac_config,
	.mac_link_down = mtk_mac_link_down,
	.mac_link_up = mtk_mac_link_up,
};
#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
int ql_phy_send_msg(void *unused)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;

    int ret;
    nl_skb = nlmsg_new(sizeof(struct rtlphy_link_status), GFP_ATOMIC);
    if(!nl_skb)
    {
        pr_err("netlink_alloc_skb error\n");
        return -1;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, PHY_WAN_LAN_ADAPT_NETLINK_MSG, sizeof(struct rtlphy_link_status), 0);

    if(nlh == NULL)
    {
        pr_err("nlmsg_put() error\n");
        nlmsg_free(nl_skb);
        return -1;
    }
    memcpy(nlmsg_data(nlh), &phy_state, sizeof(struct rtlphy_link_status));

    ret = netlink_unicast(phy_nl_fd, nl_skb, PHY_WAN_LAN_ADAPT_NETLINK_PORT, MSG_DONTWAIT);

    return ret;
}
EXPORT_SYMBOL_GPL(ql_phy_send_msg);
static void recv_cb(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    void *data = "NULL";

    pr_info("skb->len:%u\n", skb->len);
    if(skb->len >= nlmsg_total_size(0))
    {
        nlh = nlmsg_hdr(skb);
        data = NLMSG_DATA(nlh);
        if(data)
        {
            pr_info("kernel receive data: %s\n", (char *)data);
                       phy_state.event_type = NL_INITIATIVE;
            ql_phy_send_msg((void *)&phy_state);
        }
    }
}

struct netlink_kernel_cfg cfg =
{
    .input = recv_cb,
};
#endif
static int mtk_mdio_init(struct mtk_eth *eth)
{
	struct device_node *mii_np;
	int ret;

	mii_np = of_get_child_by_name(eth->dev->of_node, "mdio-bus");
	if (!mii_np) {
		dev_err(eth->dev, "no %s child node found", "mdio-bus");
		return -ENODEV;
	}

	if (!of_device_is_available(mii_np)) {
		ret = -ENODEV;
		goto err_put_node;
	}

	eth->mii_bus = devm_mdiobus_alloc(eth->dev);
	if (!eth->mii_bus) {
		ret = -ENOMEM;
		goto err_put_node;
	}

	eth->mii_bus->name = "mdio";
	eth->mii_bus->read = mtk_mdio_read;
	eth->mii_bus->write = mtk_mdio_write;
	eth->mii_bus->priv = eth;
	eth->mii_bus->parent = eth->dev;

	if (!snprintf(eth->mii_bus->id, MII_BUS_ID_SIZE, "%pOFn", mii_np)) {
		dev_err(eth->dev, "mii_bus->id wrong");
		ret = -ENODEV;
		goto err_put_node;
	}
	ret = of_mdiobus_register(eth->mii_bus, mii_np);
	#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
	phy_nl_fd = netlink_kernel_create(&init_net, PHY_WAN_LAN_ADAPT_NETLINK_MSG, &cfg);
	if (phy_nl_fd != NULL)
			phy_netlink_enable = 1;
	else
	{
			pr_err("[quectel_phy] get a NULL netlink fd!!!");
			phy_netlink_enable = 0;
	}
	#endif
err_put_node:
	of_node_put(mii_np);
	return ret;
}

static void mtk_mdio_cleanup(struct mtk_eth *eth)
{
	if (!eth->mii_bus)
		return;

	mdiobus_unregister(eth->mii_bus);
}

static inline void mtk_tx_irq_disable(struct mtk_eth *eth, u32 mask)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&eth->tx_irq_lock, flags);
	val = mtk_r32(eth, eth->tx_int_mask_reg);
	mtk_w32(eth, val & ~mask, eth->tx_int_mask_reg);
	spin_unlock_irqrestore(&eth->tx_irq_lock, flags);
}

static inline void mtk_tx_irq_enable(struct mtk_eth *eth, u32 mask)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&eth->tx_irq_lock, flags);
	val = mtk_r32(eth, eth->tx_int_mask_reg);
	mtk_w32(eth, val | mask, eth->tx_int_mask_reg);
	spin_unlock_irqrestore(&eth->tx_irq_lock, flags);
}

static inline void mtk_rx_irq_disable(struct mtk_eth *eth, u32 mask)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&eth->rx_irq_lock, flags);
	val = mtk_r32(eth, eth->rx_int_mask_reg);
	mtk_w32(eth, val & ~mask, eth->rx_int_mask_reg);
	spin_unlock_irqrestore(&eth->rx_irq_lock, flags);
}

void mtk_rx_irq_enable(struct mtk_eth *eth, u32 mask)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&eth->rx_irq_lock, flags);
	val = mtk_r32(eth, eth->rx_int_mask_reg);
	mtk_w32(eth, val | mask, eth->rx_int_mask_reg);
	spin_unlock_irqrestore(&eth->rx_irq_lock, flags);
}

static int mtk_set_mac_address(struct net_device *dev, void *p)
{
	int ret = eth_mac_addr(dev, p);
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	const char *macaddr = dev->dev_addr;

	if (ret)
		return ret;

	if (unlikely(test_bit(MTK_RESETTING, &mac->hw->state)))
		return -EBUSY;

	spin_lock_bh(&mac->hw->page_lock);
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628)) {
		mtk_w32(mac->hw, (macaddr[0] << 8) | macaddr[1],
			MT7628_SDM_MAC_ADRH);
		mtk_w32(mac->hw, (macaddr[2] << 24) | (macaddr[3] << 16) |
			(macaddr[4] << 8) | macaddr[5],
			MT7628_SDM_MAC_ADRL);
	} else {
		mtk_w32(mac->hw, (macaddr[0] << 8) | macaddr[1],
			MTK_GDMA_MAC_ADRH(mac->id));
		mtk_w32(mac->hw, (macaddr[2] << 24) | (macaddr[3] << 16) |
			(macaddr[4] << 8) | macaddr[5],
			MTK_GDMA_MAC_ADRL(mac->id));
	}
	spin_unlock_bh(&mac->hw->page_lock);

#ifdef __RAWDATA_MAC_ADDR
	dev_info(eth->dev, "%s.%d Set MAC%d address:%x:%x:%x:%x:%x:%x \n", __func__, __LINE__, mac->id, \
					macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	if(push_rawdata_mac_addr(mac->id, macaddr) != true){
		ret = -EPERM;
	}
	return ret;
#endif

	return 0;
}

void mtk_stats_update_mac(struct mtk_mac *mac)
{
	struct mtk_hw_stats *hw_stats = mac->hw_stats;
	unsigned int base = MTK_GDM1_TX_GBCNT;
	u64 stats;
	u32 val = 0;

	base += hw_stats->reg_offset;

	u64_stats_update_begin(&hw_stats->syncp);

	hw_stats->rx_bytes += mtk_r32(mac->hw, base);
	stats =  mtk_r32(mac->hw, base + 0x04);
	if (stats)
		hw_stats->rx_bytes += (stats << 32);
	hw_stats->rx_packets += mtk_r32(mac->hw, base + 0x08);
	stats =  mtk_r32(mac->hw, base + 0x0c);
	if (stats && MTK_PACKETS_64)
		hw_stats->rx_packets += (stats << 32);
	hw_stats->rx_overflow += mtk_r32(mac->hw, base + 0x10);
	hw_stats->rx_fcs_errors += mtk_r32(mac->hw, base + 0x14);
	hw_stats->rx_short_errors += mtk_r32(mac->hw, base + 0x18);
	hw_stats->rx_long_errors += mtk_r32(mac->hw, base + 0x1c);
	hw_stats->rx_checksum_errors += mtk_r32(mac->hw, base + 0x20);

	hw_stats->tx_skip += mtk_r32(mac->hw, base + MTK_TX_SKIP_OFFSET);
	hw_stats->tx_collisions += mtk_r32(mac->hw, base + MTK_TX_COLLISION_OFFSET);
	hw_stats->tx_bytes += mtk_r32(mac->hw, base + MTK_TX_BYTES_OFFSET);
	stats =  mtk_r32(mac->hw, base + MTK_TX_BYTES_OFFSET + 0x04);
	if (stats)
		hw_stats->tx_bytes += (stats << 32);
	hw_stats->tx_packets += mtk_r32(mac->hw, base + MTK_TX_PACKETS_OFFSET);
	stats =  mtk_r32(mac->hw, base + MTK_TX_PACKETS_OFFSET + 0x04);
	if (stats && MTK_PACKETS_64)
		hw_stats->tx_packets += (stats << 32);

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	hw_stats->rx_multicast += mtk_r32(mac->hw, MTK_RX_MULTICAST_OFFSET(mac->id));
	stats =  mtk_r32(mac->hw, MTK_RX_MULTICAST_OFFSET(mac->id) + 0x04);
	if (stats)
		hw_stats->rx_multicast += (stats << 32);
#endif

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	if ((mac->id == 0 && mac->hw->soc->rtl8221_phy0_support) || (mac->id == 1 && mac->hw->soc->rtl8221_phy1_support) || \
		(mac->id == 0 && mac->hw->soc->aqr113c_phy0_support) || (mac->id == 1 && mac->hw->soc->aqr113c_phy1_support) || \
		(mac->id == 0 && mac->hw->soc->rtl8251_phy0_support) || (mac->id == 1 && mac->hw->soc->rtl8251_phy1_support) || \
		(mac->id == 0 && mac->hw->soc->en8801_phy0_support) || (mac->id == 1 && mac->hw->soc->en8801_phy1_support) || \
		(mac->id == 0 && mac->hw->soc->en8811_phy0_support) || (mac->id == 1 && mac->hw->soc->en8811_phy1_support) || \
		(mac->id == 0 && mac->hw->soc->yt8821_phy0_support) || (mac->id == 1 && mac->hw->soc->yt8821_phy1_support)
	) {
#else
	if (mac->hw->soc->aqr113c_phy0_support ||
				mac->hw->soc->aqr113c_phy1_support) {
#endif
		val = mtk_r32(mac->hw, MTK_XMAC_RX_PAUSE_CNT(mac->id));
		hw_stats->rx_flow_control_packets +=
			(val & 0xffff);
		//clear global cnt
		val = mtk_r32(mac->hw, MTK_XMAC_CNT_CTRL(mac->id));

		val &= 0xffffffe;
		val |= 0x1;

		mtk_w32(mac->hw, val, MTK_XMAC_CNT_CTRL(mac->id));
	} else {
		hw_stats->rx_flow_control_packets +=
					mtk_r32(mac->hw, base + 0x24);
	}


	u64_stats_update_end(&hw_stats->syncp);

}

static void mtk_stats_update(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->mac[i] || !eth->mac[i]->hw_stats)
			continue;
		if (spin_trylock(&eth->mac[i]->hw_stats->stats_lock)) {
			mtk_stats_update_mac(eth->mac[i]);
			spin_unlock(&eth->mac[i]->hw_stats->stats_lock);
		}
	}
}

static void mtk_get_stats64(struct net_device *dev,
			    struct rtnl_link_stats64 *storage)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_hw_stats *hw_stats = mac->hw_stats;
	unsigned int start;


	if (unlikely(!test_bit(MTK_HW_INIT, &mac->hw->state)))
		return ;

	if (netif_running(dev) && netif_device_present(dev)) {
		if (spin_trylock_bh(&hw_stats->stats_lock)) {
			mtk_stats_update_mac(mac);
			spin_unlock_bh(&hw_stats->stats_lock);
		}
	}

	do {
		start = u64_stats_fetch_begin_irq(&hw_stats->syncp);
		storage->rx_packets = hw_stats->rx_packets;
		storage->tx_packets = hw_stats->tx_packets;
		storage->rx_bytes = hw_stats->rx_bytes;
		storage->tx_bytes = hw_stats->tx_bytes;
		storage->collisions = hw_stats->tx_collisions;
		storage->rx_length_errors = hw_stats->rx_short_errors +
			hw_stats->rx_long_errors;
		storage->rx_over_errors = hw_stats->rx_overflow;
		storage->rx_crc_errors = hw_stats->rx_fcs_errors;
		storage->rx_errors = hw_stats->rx_checksum_errors;
		storage->tx_aborted_errors = hw_stats->tx_skip;

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		storage->multicast = hw_stats->rx_multicast;
#endif

	} while (u64_stats_fetch_retry_irq(&hw_stats->syncp, start));

	storage->tx_errors = dev->stats.tx_errors;
	storage->rx_dropped = dev->stats.rx_dropped;
	storage->tx_dropped = dev->stats.tx_dropped;
}

#ifndef CONFIG_QUECTEL_ETHERNET_PHY
static inline int mtk_max_frag_size(int mtu)
{
	/* make sure buf_size will be at least MTK_MAX_RX_LENGTH */
	if (mtu + MTK_RX_ETH_HLEN < MTK_MAX_RX_LENGTH)
		mtu = MTK_MAX_RX_LENGTH - MTK_RX_ETH_HLEN;

	return SKB_DATA_ALIGN(MTK_RX_HLEN + mtu) +
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
}

static inline int mtk_max_buf_size(int frag_size)
{
	int buf_size = frag_size - NET_SKB_PAD - NET_IP_ALIGN -
		       SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	WARN_ON(buf_size < MTK_MAX_RX_LENGTH);

	return buf_size;
}
#else
static inline int mtk_max_frag_size(int mtu)
{
	/* make sure buf_size will be at least MTK_MAX_RX_LENGTH_2K */
	if (mtu + MTK_RX_ETH_HLEN < MTK_MAX_RX_LENGTH_2K)
		mtu = MTK_MAX_RX_LENGTH_2K - MTK_RX_ETH_HLEN;
	return SKB_DATA_ALIGN(MTK_RX_HLEN + mtu) +
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
}
static inline int mtk_max_buf_size(int frag_size)
{
	int buf_size = frag_size - NET_SKB_PAD - NET_IP_ALIGN -
		       SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	WARN_ON(buf_size < MTK_MAX_RX_LENGTH_2K);
	return buf_size;
}
#endif

static inline bool mtk_rx_get_desc(struct mtk_rx_dma *rxd,
				   struct mtk_rx_dma *dma_rxd)
{
	rxd->rxd2 = READ_ONCE(dma_rxd->rxd2);
	if (!(rxd->rxd2 & RX_DMA_DONE))
		return false;

	rxd->rxd1 = READ_ONCE(dma_rxd->rxd1);
	rxd->rxd3 = READ_ONCE(dma_rxd->rxd3);
	rxd->rxd4 = READ_ONCE(dma_rxd->rxd4);
#if defined(CONFIG_MTK_ADMAV2)
	rxd->rxd5 = READ_ONCE(dma_rxd->rxd5);
	rxd->rxd6 = READ_ONCE(dma_rxd->rxd6);
#endif

	return true;
}

/* the qdma core needs scratch memory to be setup */
static int mtk_init_fq_dma(struct mtk_eth *eth)
{
	dma_addr_t phy_ring_tail;
	int cnt = MTK_DMA_SIZE;
	dma_addr_t dma_addr;
	int i;
	u64 addr64_1 = 0;

	if (!(eth->soc->has_sram_qdma)) {
		dev_err(eth->dev, "%s: eth->scratch_ring no sram\n",__func__);
		eth->scratch_ring = dma_alloc_coherent(eth->dev,
			cnt * sizeof(struct mtk_tx_dma),
			&eth->phy_scratch_ring,
			GFP_KERNEL | __GFP_ZERO | GFP_DMA32);

	} else {
		eth->scratch_ring = eth->base + MTK_ETH_SRAM_OFFSET;
		dev_err(eth->dev, "%s: eth->scratch_ring %x %x %llx\n",
			__func__, eth->base, eth->scratch_ring, eth->phy_scratch_ring);
	}

	if (unlikely(!eth->scratch_ring))
		return -ENOMEM;

	eth->scratch_head = kcalloc(cnt, MTK_QDMA_PAGE_SIZE,
				    GFP_KERNEL);
	if (unlikely(!eth->scratch_head))
		return -ENOMEM;

	dma_addr = dma_map_single(eth->dev,
				  eth->scratch_head, cnt * MTK_QDMA_PAGE_SIZE,
				  DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(eth->dev, dma_addr)))
		return -ENOMEM;

	eth->phy_scratch_head = dma_addr;
	eth->scratch_head_size = cnt * MTK_QDMA_PAGE_SIZE;

	phy_ring_tail = eth->phy_scratch_ring +
			(sizeof(struct mtk_tx_dma) * (cnt - 1));

	for (i = 0; i < cnt; i++) {
		eth->scratch_ring[i].txd1 =
					(dma_addr + (i * MTK_QDMA_PAGE_SIZE));
		if (i < cnt - 1)
			eth->scratch_ring[i].txd2 = (eth->phy_scratch_ring +
				((i + 1) * sizeof(struct mtk_tx_dma)));

		addr64_1 = TX_DMA_SDP1(dma_addr);
		eth->scratch_ring[i].txd3 = TX_DMA_SDL(MTK_QDMA_PAGE_SIZE) | (addr64_1 & 0xf);

		eth->scratch_ring[i].txd4 = 0;

		if (eth->soc->has_sram_qdma && ((sizeof(struct mtk_tx_dma)) > 16)) {
			eth->scratch_ring[i].txd5 = 0;
			eth->scratch_ring[i].txd6 = 0;
			eth->scratch_ring[i].txd7 = 0;
			eth->scratch_ring[i].txd8 = 0;
		}
	}

	mtk_w32(eth, eth->phy_scratch_ring, MTK_QDMA_FQ_HEAD);
	mtk_w32(eth, phy_ring_tail, MTK_QDMA_FQ_TAIL);
	mtk_w32(eth, (cnt << 16) | cnt, MTK_QDMA_FQ_CNT);
	mtk_w32(eth, MTK_QDMA_PAGE_SIZE << 16, MTK_QDMA_FQ_BLEN);

	return 0;
}

static inline void *mtk_qdma_phys_to_virt(struct mtk_tx_ring *ring, u32 desc)
{
	void *ret = ring->dma;

	return ret + (desc - ring->phys);
}

static inline struct mtk_tx_buf *mtk_desc_to_tx_buf(struct mtk_tx_ring *ring,
						    struct mtk_tx_dma *txd)
{
	int idx = txd - ring->dma;

	return &ring->buf[idx];
}

static struct mtk_tx_dma *qdma_to_pdma(struct mtk_tx_ring *ring,
				       struct mtk_tx_dma *dma)
{
	return ring->dma_pdma - ring->dma + dma;
}

static int txd_to_idx(struct mtk_tx_ring *ring, struct mtk_tx_dma *dma)
{
	return ((void *)dma - (void *)ring->dma) / sizeof(*dma);
}

static void mtk_tx_unmap(struct mtk_eth *eth, struct mtk_tx_buf *tx_buf,
			 bool napi)
{
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		if (tx_buf->flags & MTK_TX_FLAGS_SINGLE0) {
			dma_unmap_single(eth->dev,
					 dma_unmap_addr(tx_buf, dma_addr0),
					 dma_unmap_len(tx_buf, dma_len0),
					 DMA_TO_DEVICE);
		} else if (tx_buf->flags & MTK_TX_FLAGS_PAGE0) {
			dma_unmap_page(eth->dev,
				       dma_unmap_addr(tx_buf, dma_addr0),
				       dma_unmap_len(tx_buf, dma_len0),
				       DMA_TO_DEVICE);
		}
	} else {
		if (dma_unmap_len(tx_buf, dma_len0)) {
			dma_unmap_page(eth->dev,
				       dma_unmap_addr(tx_buf, dma_addr0),
				       dma_unmap_len(tx_buf, dma_len0),
				       DMA_TO_DEVICE);
		}

		if (dma_unmap_len(tx_buf, dma_len1)) {
			dma_unmap_page(eth->dev,
				       dma_unmap_addr(tx_buf, dma_addr1),
				       dma_unmap_len(tx_buf, dma_len1),
				       DMA_TO_DEVICE);
		}
	}

	tx_buf->flags = 0;
	if (tx_buf->skb &&
	    (tx_buf->skb != (struct sk_buff *)MTK_DMA_DUMMY_DESC)) {
		if (napi)
			napi_consume_skb(tx_buf->skb, napi);
		else
			dev_kfree_skb_any(tx_buf->skb);
	}
	tx_buf->skb = NULL;
}

static void setup_tx_buf(struct mtk_eth *eth, struct mtk_tx_buf *tx_buf,
			 struct mtk_tx_dma *txd, dma_addr_t mapped_addr,
			 size_t size, int idx)
{
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		dma_unmap_addr_set(tx_buf, dma_addr0, mapped_addr);
		dma_unmap_len_set(tx_buf, dma_len0, size);
	} else {
		if (idx & 1) {
			txd->txd3 = mapped_addr;
			txd->txd2 |= TX_DMA_PLEN1(size);
			dma_unmap_addr_set(tx_buf, dma_addr1, mapped_addr);
			dma_unmap_len_set(tx_buf, dma_len1, size);
		} else {
			tx_buf->skb = (struct sk_buff *)MTK_DMA_DUMMY_DESC;
			txd->txd1 = mapped_addr;
			txd->txd2 = TX_DMA_PLEN0(size);
			dma_unmap_addr_set(tx_buf, dma_addr0, mapped_addr);
			dma_unmap_len_set(tx_buf, dma_len0, size);
		}
	}
}

static int mtk_tx_map(struct sk_buff *skb, struct net_device *dev,
		      int tx_num, struct mtk_tx_ring *ring, bool gso)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	struct mtk_tx_dma *itxd, *txd;
	struct mtk_tx_dma *itxd_pdma, *txd_pdma;
	struct mtk_tx_buf *itx_buf, *tx_buf;
	dma_addr_t mapped_addr;
	unsigned int nr_frags;
	int i, n_desc = 1;
	u32 txd4 = 0, fport;
	int k = 0;
	u32 txd5 = 0;
	u64 addr64_1;

	itxd = ring->next_free;
	itxd_pdma = qdma_to_pdma(ring, itxd);
	if (itxd == ring->last_free)
		return -ENOMEM;

	/* set the forward port */
	fport = (mac->id + 1) << TX_DMA_FPORT_SHIFT;

#if defined(CONFIG_HW_NAT)
	if (ppe_hook_eth_tx_fport && ppe_hook_eth_tx_fport(skb))
	   fport = (3 << TX_DMA_FPORT_SHIFT);
#endif /* CONFIG_HW_NAT */

	txd4 |= fport;

	itx_buf = mtk_desc_to_tx_buf(ring, itxd);
	memset(itx_buf, 0, sizeof(*itx_buf));

	if (gso) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890))
			txd5 |= TX_DMA_TSO;
		else
			txd4 |= TX_DMA_TSO;
	}

	/* TX Checksum offload */
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890))
			txd5 |= TX_DMA_CHKSUM;
		else
			txd4 |= TX_DMA_CHKSUM;
	}

	if ((MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK))) {
		txd5 |= TX_DMA_CHKSUM;
	}


	/* sfq */
	sfq_fill_txd5(skb, &txd5);

#if defined(CONFIG_NET_MEDIATEK_HW_QOS)
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {

		qid = skb->mark & (MTK_QDMA_TX_MASK);
	/* Disable Qos CPU Path
		if ((qid != 0) && (skb->mark < 64)) {
			txd4 = txd4 | ((M2Q_table[skb->mark] & 0x3f) << 16);
		}
	*/
	}
#endif
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {

		/* VLAN header offload */
		if (skb_vlan_tag_present(skb))
			txd4 |= TX_DMA_INS_VLAN | skb_vlan_tag_get(skb);
	}


	mapped_addr = dma_map_single(eth->dev, skb->data,
				     skb_headlen(skb), DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(eth->dev, mapped_addr)))
		return -ENOMEM;

	WRITE_ONCE(itxd->txd1, mapped_addr);

	addr64_1 = TX_DMA_SDP1(mapped_addr);

	itx_buf->flags |= MTK_TX_FLAGS_SINGLE0;
	itx_buf->flags |= (!mac->id) ? MTK_TX_FLAGS_FPORT0 :
			  MTK_TX_FLAGS_FPORT1;
	setup_tx_buf(eth, itx_buf, itxd_pdma, mapped_addr, skb_headlen(skb),
		     k++);

	/* TX SG offload */
	txd = itxd;
	txd_pdma = qdma_to_pdma(ring, txd);
	nr_frags = skb_shinfo(skb)->nr_frags;

	for (i = 0; i < nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		unsigned int offset = 0;
		int frag_size = skb_frag_size(frag);

		while (frag_size) {
			bool last_frag = false;
			unsigned int frag_map_size;
			bool new_desc = true;

			if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA) ||
			    (i & 0x1)) {
				txd = mtk_qdma_phys_to_virt(ring, txd->txd2);
				txd_pdma = qdma_to_pdma(ring, txd);
				if (txd == ring->last_free)
					goto err_dma;

				n_desc++;
			} else {
				new_desc = false;
			}


			frag_map_size = min(frag_size, MTK_TX_DMA_BUF_LEN);
			mapped_addr = skb_frag_dma_map(eth->dev, frag, offset,
						       frag_map_size,
						       DMA_TO_DEVICE);
			if (unlikely(dma_mapping_error(eth->dev, mapped_addr)))
				goto err_dma;

			if (i == nr_frags - 1 &&
			    (frag_size - frag_map_size) == 0)
				last_frag = true;

			WRITE_ONCE(txd->txd1, mapped_addr);

			if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
				WRITE_ONCE(txd->txd3, (TX_DMA_PLEN0(frag_map_size) |
						       last_frag * TX_DMA_LS0) |
						       TX_DMA_SDP1(mapped_addr));
				txd4 = TX_DMA_SWC | fport;
				WRITE_ONCE(txd->txd4, txd4);
				WRITE_ONCE(txd->txd5, 0);
			} else {
				WRITE_ONCE(txd->txd3, (TX_DMA_SWC |
							   TX_DMA_PLEN0(frag_map_size) |
							   last_frag * TX_DMA_LS0));
				WRITE_ONCE(txd->txd4, fport);
			}



			tx_buf = mtk_desc_to_tx_buf(ring, txd);
			if (new_desc)
				memset(tx_buf, 0, sizeof(*tx_buf));
			tx_buf->skb = (struct sk_buff *)MTK_DMA_DUMMY_DESC;
			tx_buf->flags |= MTK_TX_FLAGS_PAGE0;
			tx_buf->flags |= (!mac->id) ? MTK_TX_FLAGS_FPORT0 :
					 MTK_TX_FLAGS_FPORT1;

			setup_tx_buf(eth, tx_buf, txd_pdma, mapped_addr,
				     frag_map_size, k++);

			frag_size -= frag_map_size;
			offset += frag_map_size;
		}
	}

	/* store skb to cleanup */
	itx_buf->skb = skb;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {

		WRITE_ONCE(itxd->txd5, txd5);
			WRITE_ONCE(itxd->txd6, 0);
			WRITE_ONCE(itxd->txd7, 0);
			WRITE_ONCE(itxd->txd8, 0);
			WRITE_ONCE(itxd->txd3, (TX_DMA_PLEN0(skb_headlen(skb)) |
						(!nr_frags * TX_DMA_LS0)) |
						addr64_1);
			txd4 = txd4 | TX_DMA_SWC;
			WRITE_ONCE(itxd->txd4, txd4);
	} else {

		WRITE_ONCE(itxd->txd4, txd4);
		WRITE_ONCE(itxd->txd3, (TX_DMA_SWC | TX_DMA_PLEN0(skb_headlen(skb)) |
					(!nr_frags * TX_DMA_LS0)));
		if (!MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
			if (k & 0x1)
				txd_pdma->txd2 |= TX_DMA_LS0;
			else
				txd_pdma->txd2 |= TX_DMA_LS1;
		}
	}

	netdev_sent_queue(dev, skb->len);
	skb_tx_timestamp(skb);

	ring->next_free = mtk_qdma_phys_to_virt(ring, txd->txd2);
	atomic_sub(n_desc, &ring->free_count);

	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
			mtk_w32(eth, txd->txd2, MTK_QTX_CTX_PTR);
		} else {
			if (netif_xmit_stopped(netdev_get_tx_queue(dev, 0)) ||
			    !netdev_xmit_more())
				mtk_w32(eth, txd->txd2, MTK_QTX_CTX_PTR);
		}
	} else {
		int next_idx = NEXT_DESP_IDX(txd_to_idx(ring, txd),
					     ring->dma_size);
		mtk_w32(eth, next_idx, MT7628_TX_CTX_IDX0);
	}
	return 0;

err_dma:
	do {
		tx_buf = mtk_desc_to_tx_buf(ring, itxd);

		/* unmap dma */
		mtk_tx_unmap(eth, tx_buf, false);

		itxd->txd3 = TX_DMA_LS0 | TX_DMA_OWNER_CPU;
		if (!MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA))
			itxd_pdma->txd2 = TX_DMA_DESP2_DEF;

		itxd = mtk_qdma_phys_to_virt(ring, itxd->txd2);
		itxd_pdma = qdma_to_pdma(ring, itxd);
	} while (itxd != txd);

	return -ENOMEM;
}

static inline int mtk_cal_txd_req(struct sk_buff *skb)
{
	int i, nfrags;
	skb_frag_t *frag;

	nfrags = 1;
	if (skb_is_gso(skb)) {
		for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
			frag = &skb_shinfo(skb)->frags[i];
			nfrags += DIV_ROUND_UP(skb_frag_size(frag),
						MTK_TX_DMA_BUF_LEN);
		}
	} else {
		nfrags += skb_shinfo(skb)->nr_frags;
	}

	return nfrags;
}

static int mtk_queue_stopped(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		if (netif_queue_stopped(eth->netdev[i]))
			return 1;
	}

	return 0;
}

static void mtk_wake_queue(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		netif_wake_queue(eth->netdev[i]);
	}
}

static int mtk_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	struct mtk_tx_ring *ring = &eth->tx_ring;
	struct net_device_stats *stats = &dev->stats;
	bool gso = false;
	int tx_num;

	/* normally we can rely on the stack not calling this more than once,
	 * however we have 2 queues running on the same ring so we need to lock
	 * the ring access
	 */
	spin_lock(&eth->page_lock);

	if (unlikely(test_bit(MTK_RESETTING, &eth->state)))
		goto drop;


#if defined(CONFIG_HW_NAT)
	if (ppe_hook_tx_eth) {
		if (ppe_hook_tx_eth(skb, mac->id + 1) != 1)
			goto drop;
	}
#endif /* CONFIG_HW_NAT */


	tx_num = mtk_cal_txd_req(skb);
	if (unlikely(atomic_read(&ring->free_count) <= tx_num)) {
		netif_stop_queue(dev);
		netif_err(eth, tx_queued, dev,
			  "Tx Ring full when queue awake!\n");
		spin_unlock(&eth->page_lock);
		return NETDEV_TX_BUSY;
	}

	/* TSO: fill MSS info in tcp checksum field */
	if (skb_is_gso(skb)) {
		if (skb_cow_head(skb, 0)) {
			netif_warn(eth, tx_err, dev,
				   "GSO expand head fail.\n");
			goto drop;
		}

		if (skb_shinfo(skb)->gso_type &
				(SKB_GSO_TCPV4 | SKB_GSO_TCPV6)) {
			gso = true;
			tcp_hdr(skb)->check = htons(skb_shinfo(skb)->gso_size);
		}
	}

	if (mtk_tx_map(skb, dev, tx_num, ring, gso) < 0)
		goto drop;

	if (unlikely(atomic_read(&ring->free_count) <= ring->thresh))
		netif_stop_queue(dev);

	spin_unlock(&eth->page_lock);

	return NETDEV_TX_OK;

drop:
	spin_unlock(&eth->page_lock);
	stats->tx_dropped++;
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

static struct mtk_rx_ring *mtk_get_rx_ring(struct mtk_eth *eth)
{
	int i;
	struct mtk_rx_ring *ring;
	int idx;

	if (!eth->hwlro)
		return &eth->rx_ring[0];

#if defined(CONFIG_MTK_ADMAV2)
		//normal ring
			for (i = 0; i < 1; i++) {
				ring = &eth->rx_ring[i];
				idx = NEXT_DESP_IDX(ring->calc_idx, ring->dma_size);
				if (ring->dma[idx].rxd2 & RX_DMA_DONE) {
					ring->calc_idx_update = true;
					//dev_err(eth->dev, "Rx LRO ring %d idx %d\n", i, idx);
					return ring;
				}
			}
		//LRO ring
			for (i = eth->rx_lro_base; i < eth->max_rx_ring_num; i++) {
				ring = &eth->rx_ring[i];
				idx = NEXT_DESP_IDX(ring->calc_idx, ring->dma_size);
				if (ring->dma[idx].rxd2 & RX_DMA_DONE) {
					ring->calc_idx_update = true;
					//dev_err(eth->dev, "Rx LRO ring %d idx %d\n", i, idx);
					return ring;
				}
			}

#else

		for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
			ring = &eth->rx_ring[i];
			idx = NEXT_DESP_IDX(ring->calc_idx, ring->dma_size);
			if (ring->dma[idx].rxd2 & RX_DMA_DONE) {
				ring->calc_idx_update = true;
				return ring;
			}
		}
#endif


	return NULL;
}

static void mtk_update_rx_cpu_idx(struct mtk_eth *eth)
{
	struct mtk_rx_ring *ring;
	int i;

	if (!eth->hwlro) {
		ring = &eth->rx_ring[0];
		mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
	} else {
#if defined(CONFIG_MTK_ADMAV2)
		for (i = 0; i < 1; i++) {
			ring = &eth->rx_ring[i];
			if (ring->calc_idx_update) {
				ring->calc_idx_update = false;
				mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
				//pr_info("LRO update calc_idx_update ring %d CIDX =%d", i, ring->calc_idx);

			}
		}
		for (i = eth->rx_lro_base; i < eth->max_rx_ring_num; i++) {
			ring = &eth->rx_ring[i];
			if (ring->calc_idx_update) {
				ring->calc_idx_update = false;
				mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
				//pr_info("LRO update calc_idx_update ring %d CIDX =%d", i, ring->calc_idx);
			}
		}
#else
		for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
			ring = &eth->rx_ring[i];
			if (ring->calc_idx_update) {
				ring->calc_idx_update = false;
				mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
			}
		}
#endif

	}
}

int mtk_poll_rx(struct napi_struct *napi, int budget,
		       struct mtk_eth *eth)
{
	struct mtk_rx_ring *ring;
	int idx;
	struct sk_buff *skb;
	u8 *data, *new_data;
	struct mtk_rx_dma *rxd, trxd;
	int done = 0;
	u32 tmp = 0;
	int result = 1;
	u64 addr64 = 0;

	while (done < budget) {
		struct net_device *netdev;
		unsigned int pktlen;
		dma_addr_t dma_addr = 0;
		dma_addr_t dma_addr2 = 0;
		int mac =0;

		if (mtk_is_rss_ring(eth, napi)){
			ring = mtk_get_rx_ring_rss(eth, napi);
		}else {
			ring = mtk_get_rx_ring(eth);
		}
		if (unlikely(!ring))
			goto rx_done;

		idx = NEXT_DESP_IDX(ring->calc_idx, ring->dma_size);
		rxd = &ring->dma[idx];
		data = ring->data[idx];

		if (!mtk_rx_get_desc(&trxd, rxd))
			break;

		if (eth->dbg_flag > 1) {
			dev_err(eth->dev, "RXD %d, %08x %08x %08x %08x\n", idx,
			   trxd.rxd1, trxd.rxd2, trxd.rxd3,
			   trxd.rxd4);
		}

		/* find out which mac the packet come from. values start at 1 */
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628)) {
			mac = 0;
		} else {
#if defined(CONFIG_MTK_ADMAV2)
			/* find out which mac the packet come from. values start at 1 */
			mac = (trxd.rxd5 >> RX_DMA_FPORT_SHIFT) &
			RX_DMA_FPORT_MASK;
			mac--;
#else
			/* find out which mac the packet come from. values start at 1 */
			mac = (trxd.rxd4 >> RX_DMA_FPORT_SHIFT) &
				  RX_DMA_FPORT_MASK;
			mac--;
#endif
		}

#if defined(CONFIG_HW_NAT)
		/* fast path FP is QDMA. default uses mac1(eth0) */
		if (mac > 1) {
			netdev = eth->netdev[1];
			mac = (netdev && netif_running(netdev)) ? 1 : 0;
		}
#endif
		if (unlikely(mac < 0 || mac >= MTK_MAC_COUNT ||
			     !eth->netdev[mac]))
			goto release_desc;

		netdev = eth->netdev[mac];

		if (unlikely(test_bit(MTK_RESETTING, &eth->state)))
			goto release_desc;

		/* alloc new buffer */
		new_data = mtketh_alloc_frag(ring->frag_size);
		if (unlikely(!new_data)) {
			netdev->stats.rx_dropped++;
			goto release_desc;
		}
		dma_addr = dma_map_single(eth->dev,
					  new_data + NET_SKB_PAD +
					  eth->ip_align,
					  ring->buf_size,
					  DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(eth->dev, dma_addr))) {
			skb_free_frag(new_data);
			netdev->stats.rx_dropped++;
			goto release_desc;
		}

#if defined(CONFIG_MTK_ADMAV2)
		addr64 =  (u64)(trxd.rxd2 & 0xf) << 32;
		dma_addr2 =  ((u64)trxd.rxd1) | addr64;
		dma_unmap_single(eth->dev, dma_addr2,
				 ring->buf_size, DMA_FROM_DEVICE);
#else
		dma_unmap_single(eth->dev, trxd.rxd1,
				 ring->buf_size, DMA_FROM_DEVICE);
#endif
		/* receive data */
		skb = build_skb(data, ring->frag_size);
		if (unlikely(!skb)) {
			skb_free_frag(data);
			netdev->stats.rx_dropped++;
			goto skip_rx;
		}
		skb_reserve(skb, NET_SKB_PAD + NET_IP_ALIGN);

		pktlen = RX_DMA_GET_PLEN0(trxd.rxd2);
		skb->dev = netdev;
		skb_put(skb, pktlen);
		if ((!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2) &&
			(trxd.rxd4 & eth->rx_dma_l4_valid)) ||
			(MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2) &&
			(trxd.rxd3 & eth->rx_dma_l4_valid)))
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		else
			skb_checksum_none_assert(skb);

		if ((MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK))) {
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		}

		skb->protocol = eth_type_trans(skb, netdev);

#if defined(CONFIG_HW_NAT)
#if defined(CONFIG_MTK_ADMAV2)
		tmp = trxd.rxd5;
#else
		tmp = trxd.rxd4;
#endif
		if (ppe_hook_rx_eth) {
			if (IS_SPACE_AVAILABLE_HEAD(skb)) {
				*(u32 *)(FOE_INFO_START_ADDR_HEAD(skb)) =
					tmp;
				FOE_ALG_HEAD(skb) = 0;
				FOE_MAGIC_TAG_HEAD(skb) = FOE_MAGIC_GE;
				FOE_TAG_PROTECT_HEAD(skb) = TAG_PROTECT;
			}
			if (IS_SPACE_AVAILABLE_TAIL(skb)) {
				*(u32 *)(FOE_INFO_START_ADDR_TAIL(skb) + 2) =
					tmp;
				FOE_ALG_TAIL(skb) = 0;
				FOE_MAGIC_TAG_TAIL(skb) = FOE_MAGIC_GE;
				FOE_TAG_PROTECT_TAIL(skb) = TAG_PROTECT;
			}
		}
#endif /* CONFIG_HW_NAT */


		/* hwlro debug */
		if (eth->hwlro) {
			hw_lro_stats_update(ring, &trxd);
			hw_lro_flush_stats_update(ring, &trxd);
		}


#if defined(CONFIG_MTK_ADMAV2)
		//Note: Colgin not support NETIF_F_HW_VLAN_CTAG_RX
		//		if (netdev->features & NETIF_F_HW_VLAN_CTAG_RX &&
		//			RX_DMA_VID(trxd.rxd3))
		//			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
		//						   RX_DMA_VID(trxd.rxd3));
#else
		if (netdev->features & NETIF_F_HW_VLAN_CTAG_RX &&
			RX_DMA_VID(trxd.rxd3))
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
				RX_DMA_VID(trxd.rxd3));
#endif

#if defined(CONFIG_HW_NAT)
		/* ppe_hook_rx_eth return 1 --> continue
		 * ppe_hook_rx_eth return 0 --> FWD & without netif_rx
		 */
		result = (!ppe_hook_rx_eth) || (ppe_hook_rx_eth && ppe_hook_rx_eth(skb));
#endif /* CONFIG_HW_NAT */

		if (result) {
			skb_record_rx_queue(skb, 0);
			if (!(MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK))) {
				napi_gro_receive(napi, skb);
			} else {
				netif_receive_skb(skb);
			}
		}
skip_rx:
		ring->data[idx] = new_data;
		rxd->rxd1 = (unsigned int)dma_addr;

release_desc:
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628))
			rxd->rxd2 = RX_DMA_LSO;
		else
			rxd->rxd2 = RX_DMA_PLEN0(ring->buf_size) | RX_DMA_SDP1(dma_addr);

		if(idx != ring->calc_idx)
			ring->calc_idx_update = true;
		ring->calc_idx = idx;

		done++;
	}
rx_done:
	if (done) {
		/* make sure that all changes to the dma ring are flushed before
		 * we continue
		 */
		wmb();
		if(!eth->hwrss)
			mtk_update_rx_cpu_idx(eth);
		else {
			if(mtk_is_rss_ring(eth, napi)) {
				mtk_update_rx_cpu_idx_rss(eth, ring);
			} else
				mtk_update_rx_cpu_idx(eth);
		}
	}
	return done;
}

static int mtk_poll_tx_qdma(struct mtk_eth *eth, int budget,
			    unsigned int *done, unsigned int *bytes)
{
	struct mtk_tx_ring *ring = &eth->tx_ring;
	struct mtk_tx_dma *desc;
	struct sk_buff *skb;
	struct mtk_tx_buf *tx_buf;
	u32 cpu, dma;

	cpu = ring->last_free_ptr;
	dma = mtk_r32(eth, MTK_QTX_DRX_PTR);

	desc = mtk_qdma_phys_to_virt(ring, cpu);

	while ((cpu != dma) && budget) {
		u32 next_cpu = desc->txd2;
		int mac = 0;

		if ((desc->txd3 & TX_DMA_OWNER_CPU) == 0)
			break;
		desc = mtk_qdma_phys_to_virt(ring, desc->txd2);

		tx_buf = mtk_desc_to_tx_buf(ring, desc);
		if (tx_buf->flags & MTK_TX_FLAGS_FPORT1)
			mac = 1;

		skb = tx_buf->skb;
		if (!skb)
			break;

		if (skb != (struct sk_buff *)MTK_DMA_DUMMY_DESC) {
			bytes[mac] += skb->len;
			done[mac]++;
			budget--;
		}
		mtk_tx_unmap(eth, tx_buf, true);

		ring->last_free = desc;
		atomic_inc(&ring->free_count);

		cpu = next_cpu;
	}

	ring->last_free_ptr = cpu;
	mtk_w32(eth, cpu, MTK_QTX_CRX_PTR);

	return budget;
}

static int mtk_poll_tx_pdma(struct mtk_eth *eth, int budget,
			    unsigned int *done, unsigned int *bytes)
{
	struct mtk_tx_ring *ring = &eth->tx_ring;
	struct mtk_tx_dma *desc;
	struct sk_buff *skb;
	struct mtk_tx_buf *tx_buf;
	u32 cpu, dma;

	cpu = ring->cpu_idx;
	dma = mtk_r32(eth, MT7628_TX_DTX_IDX0);

	while ((cpu != dma) && budget) {
		tx_buf = &ring->buf[cpu];
		skb = tx_buf->skb;
		if (!skb)
			break;

		if (skb != (struct sk_buff *)MTK_DMA_DUMMY_DESC) {
			bytes[0] += skb->len;
			done[0]++;
			budget--;
		}

		mtk_tx_unmap(eth, tx_buf, true);

		desc = &ring->dma[cpu];
		ring->last_free = desc;
		atomic_inc(&ring->free_count);

		cpu = NEXT_DESP_IDX(cpu, ring->dma_size);
	}

	ring->cpu_idx = cpu;

	return budget;
}

static int mtk_poll_tx(struct mtk_eth *eth, int budget)
{
	struct mtk_tx_ring *ring = &eth->tx_ring;
	unsigned int done[MTK_MAX_DEVS];
	unsigned int bytes[MTK_MAX_DEVS];
	int total = 0, i;

	memset(done, 0, sizeof(done));
	memset(bytes, 0, sizeof(bytes));

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA))
		budget = mtk_poll_tx_qdma(eth, budget, done, bytes);
	else
		budget = mtk_poll_tx_pdma(eth, budget, done, bytes);

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i] || !done[i])
			continue;
		netdev_completed_queue(eth->netdev[i], done[i], bytes[i]);
		total += done[i];
	}

	if (mtk_queue_stopped(eth) &&
	    (atomic_read(&ring->free_count) > ring->thresh))
		mtk_wake_queue(eth);

	return total;
}

void mtk_handle_status_irq(struct mtk_eth *eth)
{
	u32 status2 = mtk_r32(eth, MTK_INT_STATUS2);

	if (unlikely(status2 & (MTK_GDM1_AF | MTK_GDM2_AF))) {
		mtk_stats_update(eth);
		mtk_w32(eth, (MTK_GDM1_AF | MTK_GDM2_AF),
			MTK_INT_STATUS2);
	}
}

static int mtk_napi_tx(struct napi_struct *napi, int budget)
{
	struct mtk_eth *eth = container_of(napi, struct mtk_eth, tx_napi);
	u32 status, mask;
	int tx_done = 0;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA))
		mtk_handle_status_irq(eth);
	mtk_w32(eth, MTK_TX_DONE_INT | MTK_TX_DONE_INT0, eth->tx_int_status_reg);
	tx_done = mtk_poll_tx(eth, budget);

	if (unlikely(netif_msg_intr(eth))) {
		status = mtk_r32(eth, eth->tx_int_status_reg);
		mask = mtk_r32(eth, eth->tx_int_mask_reg);
		dev_info(eth->dev,
			 "done tx %d, intr 0x%08x/0x%x\n",
			 tx_done, status, mask);
	}

	if (tx_done == budget)
		return budget;

	status = mtk_r32(eth, eth->tx_int_status_reg);
	if (status & MTK_TX_DONE_INT)
		return budget;

	if (napi_complete(napi))
		mtk_tx_irq_enable(eth, MTK_TX_DONE_INT);

	return tx_done;
}

static int mtk_napi_rx(struct napi_struct *napi, int budget)
{
	struct mtk_eth *eth = container_of(napi, struct mtk_eth, rx_napi);
	u32 status, mask;
	int rx_done = 0;
	int remain_budget = budget;

	mtk_handle_status_irq(eth);

poll_again:
	//pr_info(" napi rx: %x", mtk_r32(eth,MTK_PDMA_INT_STATUS0));
//	mtk_w32(eth, MTK_RX_DONE_INT, eth->rx_int_status_reg);
	mtk_w32(eth, MTK_RX_DONE_DLY0 | MTK_RX_DONE_INT0, eth->rx_int_status_reg);

	rx_done = mtk_poll_rx(napi, remain_budget, eth);

	if (unlikely(netif_msg_intr(eth))) {
		status = mtk_r32(eth, eth->rx_int_status_reg);
		mask = mtk_r32(eth, eth->rx_int_mask_reg);
		dev_info(eth->dev,
			 "done rx %d, intr 0x%08x/0x%x\n",
			 rx_done, status, mask);
	}
	if (rx_done == remain_budget)
		return budget;

	status = mtk_r32(eth, eth->rx_int_status_reg);
	if (status & MTK_RX_DONE_DLY0) {
		remain_budget -= rx_done;
		goto poll_again;
	}

	if (napi_complete(napi))
		mtk_rx_irq_enable(eth, MTK_RX_DONE_DLY0);

	return rx_done + budget - remain_budget;
}

static int mtk_tx_alloc(struct mtk_eth *eth)
{
	struct mtk_tx_ring *ring = &eth->tx_ring;
	int i, sz = sizeof(*ring->dma);

	ring->buf = kcalloc(MTK_DMA_SIZE, sizeof(*ring->buf),
			       GFP_KERNEL);

	if (!ring->buf)
		goto no_tx_mem;

	if (!eth->soc->has_sram_qdma) {
		ring->dma = dma_alloc_coherent(eth->dev,
			MTK_DMA_SIZE * sz,
			&ring->phys,
			GFP_KERNEL | __GFP_ZERO | GFP_DMA32);
	} else {
		pr_info("%s : QDMA_SW_SRAM_SUPPORT", __func__);
		ring->dma =  eth->scratch_ring + MTK_DMA_SIZE;
		ring->phys = eth->phy_scratch_ring + MTK_DMA_SIZE * sz;
	}


	if (!ring->dma)
		goto no_tx_mem;

	for (i = 0; i < MTK_DMA_SIZE; i++) {
		int next = (i + 1) % MTK_DMA_SIZE;
		u32 next_ptr = ring->phys + next * sz;

		ring->dma[i].txd2 = next_ptr;
		ring->dma[i].txd3 = TX_DMA_LS0 | TX_DMA_OWNER_CPU;
		ring->dma[i].txd4 = 0;

		if (eth->soc->has_sram_qdma && (sz > 16)) {
			ring->dma[i].txd5 = 0;
			ring->dma[i].txd6 = 0;
			ring->dma[i].txd7 = 0;
			ring->dma[i].txd8 = 0;
		}
	}

	/* On MT7688 (PDMA only) this driver uses the ring->dma structs
	 * only as the framework. The real HW descriptors are the PDMA
	 * descriptors in ring->dma_pdma.
	 */
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		ring->dma_pdma = dma_alloc_coherent(eth->dev, MTK_DMA_SIZE * sz,
						    &ring->phys_pdma,
						    GFP_KERNEL);
		if (!ring->dma_pdma)
			goto no_tx_mem;

		for (i = 0; i < MTK_DMA_SIZE; i++) {
			ring->dma_pdma[i].txd2 = TX_DMA_DESP2_DEF;
			ring->dma_pdma[i].txd4 = 0;
		}
	}

	ring->dma_size = MTK_DMA_SIZE;
	atomic_set(&ring->free_count, MTK_DMA_SIZE - 2);
	ring->next_free = &ring->dma[0];
	ring->last_free = &ring->dma[MTK_DMA_SIZE - 1];
	ring->last_free_ptr = (u32)(ring->phys + ((MTK_DMA_SIZE - 1) * sz));
	ring->thresh = MAX_SKB_FRAGS;

	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		mtk_w32(eth, ring->phys, MTK_QTX_CTX_PTR);
		mtk_w32(eth, ring->phys, MTK_QTX_DTX_PTR);
		mtk_w32(eth,
			ring->phys + ((MTK_DMA_SIZE - 1) * sz),
			MTK_QTX_CRX_PTR);
		mtk_w32(eth, ring->last_free_ptr, MTK_QTX_DRX_PTR);
		mtk_w32(eth, (QDMA_RES_THRES << 8) | QDMA_RES_THRES,
			MTK_QTX_CFG(0));
	} else {
		mtk_w32(eth, ring->phys_pdma, MT7628_TX_BASE_PTR0);
		mtk_w32(eth, MTK_DMA_SIZE, MT7628_TX_MAX_CNT0);
		mtk_w32(eth, 0, MT7628_TX_CTX_IDX0);
		mtk_w32(eth, MT7628_PST_DTX_IDX0, MTK_PDMA_RST_IDX);
	}

	return 0;

no_tx_mem:
	return -ENOMEM;
}

static void mtk_tx_clean(struct mtk_eth *eth)
{
	struct mtk_tx_ring *ring = &eth->tx_ring;
	int i;

	if (ring->buf) {
		for (i = 0; i < MTK_DMA_SIZE; i++)
			mtk_tx_unmap(eth, &ring->buf[i], false);
		kfree(ring->buf);
		ring->buf = NULL;
	}

	if (!eth->soc->has_sram_qdma && ring->dma) {
		dma_free_coherent(eth->dev,
				  MTK_DMA_SIZE * sizeof(*ring->dma),
				  ring->dma,
				  ring->phys);
		ring->dma = NULL;
	}

	if (ring->dma_pdma) {
		dma_free_coherent(eth->dev,
				  MTK_DMA_SIZE * sizeof(*ring->dma_pdma),
				  ring->dma_pdma,
				  ring->phys_pdma);
		ring->dma_pdma = NULL;
	}
}

static int mtk_rx_alloc(struct mtk_eth *eth, int ring_no, int rx_flag)
{
	struct mtk_rx_ring *ring;
	int rx_data_len, rx_dma_size;
	int i;

	if (rx_flag == MTK_RX_FLAGS_QDMA) {
		if (ring_no)
			return -EINVAL;
		ring = &eth->rx_ring_qdma;
	} else {
		ring = &eth->rx_ring[ring_no];
	}

	if (rx_flag == MTK_RX_FLAGS_HWLRO) {
		rx_data_len = MTK_MAX_LRO_RX_LENGTH;
		rx_dma_size = MTK_HW_LRO_DMA_SIZE;
	} else if (rx_flag == MTK_RX_FLAGS_QDMA) {
		rx_data_len = ETH_DATA_LEN;
		rx_dma_size = 16;
	} else {
		rx_data_len = ETH_DATA_LEN;
		rx_dma_size = MTK_DMA_SIZE;
	}

	ring->frag_size = mtk_max_frag_size(rx_data_len);
	ring->buf_size = mtk_max_buf_size(ring->frag_size);
	ring->data = kcalloc(rx_dma_size, sizeof(*ring->data),
			     GFP_KERNEL);

	if (!ring->data)
		return -ENOMEM;

	for (i = 0; i < rx_dma_size; i++) {
		ring->data[i] = mtketh_alloc_frag(ring->frag_size);
		if (!ring->data[i])
			return -ENOMEM;
	}
	if ((!eth->soc->has_sram_adma) ||
		(eth->soc->has_sram_adma
				&& ((rx_flag != MTK_RX_FLAGS_NORMAL) && (rx_flag != MTK_RX_FLAGS_HWRSS))
				)) {
		ring->dma = dma_alloc_coherent(eth->dev,
					       rx_dma_size * sizeof(*ring->dma),
					       &ring->phys, GFP_KERNEL | __GFP_ZERO | GFP_DMA32);
	} else {
		pr_info("%s : ADMA_SRAM_SUPPORT %d", __func__, ring_no);
		struct mtk_tx_ring *tx_ring = &eth->tx_ring;
		ring->dma = (struct mtk_rx_dma *)(tx_ring->dma +
			     MTK_DMA_SIZE * (ring_no + 1));
		ring->phys = tx_ring->phys + MTK_DMA_SIZE *
			     sizeof(*tx_ring->dma) * (ring_no + 1);
	}

	if (!ring->dma)
		return -ENOMEM;

	for (i = 0; i < rx_dma_size; i++) {
		dma_addr_t dma_addr = dma_map_single(eth->dev,
				ring->data[i] + NET_SKB_PAD + eth->ip_align,
				ring->buf_size,
				DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(eth->dev, dma_addr)))
			return -ENOMEM;
		ring->dma[i].rxd1 = (unsigned int)dma_addr;

		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628))
			ring->dma[i].rxd2 = RX_DMA_LSO;
		else
			ring->dma[i].rxd2 = RX_DMA_PLEN0(ring->buf_size) | RX_DMA_SDP1(dma_addr);

		ring->dma[i].rxd3 = 0;
		ring->dma[i].rxd4 = 0;
#if defined(CONFIG_MTK_ADMAV2)
		if (eth->soc->has_sram_adma && ((sizeof(struct mtk_rx_dma)) > 16)) {
			ring->dma[i].rxd5 = 0;
			ring->dma[i].rxd6 = 0;
			ring->dma[i].rxd7 = 0;
			ring->dma[i].rxd8 = 0;
		}
#endif

	}
	ring->dma_size = rx_dma_size;
	ring->calc_idx_update = false;
	ring->calc_idx = rx_dma_size - 1;
	ring->crx_idx_reg = (rx_flag == MTK_RX_FLAGS_QDMA) ?
			     MTK_QRX_CRX_IDX_CFG(ring_no) :
			     MTK_PRX_CRX_IDX_CFG(ring_no);
	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();

	if (rx_flag == MTK_RX_FLAGS_QDMA) {
		mtk_w32(eth, ring->phys, MTK_QRX_BASE_PTR_CFG(ring_no));
		mtk_w32(eth, rx_dma_size, MTK_QRX_MAX_CNT_CFG(ring_no));
		mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
		mtk_w32(eth, MTK_PST_DRX_IDX_CFG(ring_no), MTK_QDMA_RST_IDX);
	} else {
		mtk_w32(eth, ring->phys, MTK_PRX_BASE_PTR_CFG(ring_no));
		mtk_w32(eth, rx_dma_size, MTK_PRX_MAX_CNT_CFG(ring_no));
		mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
		mtk_w32(eth, MTK_PST_DRX_IDX_CFG(ring_no), MTK_PDMA_RST_IDX);
	}

	return 0;
}

static void mtk_rx_clean(struct mtk_eth *eth, struct mtk_rx_ring *ring,  int in_sram)
{
	int i;

	if (ring->data && ring->dma) {
		for (i = 0; i < ring->dma_size; i++) {
			if (!ring->data[i])
				continue;
			if (!ring->dma[i].rxd1)
				continue;
#if defined(CONFIG_MTK_ADMAV2)
			dma_unmap_single(eth->dev,
					 (u64)(ring->dma[i].rxd1 | ((((u64)(ring->dma[i].rxd2 & 0xf)) << 32))),
					 ring->buf_size,
					 DMA_FROM_DEVICE);
#else
			dma_unmap_single(eth->dev, ring->dma[i].rxd1,
					 ring->buf_size, DMA_FROM_DEVICE);
#endif
			skb_free_frag(ring->data[i]);
		}
		kfree(ring->data);
		ring->data = NULL;
	}

	if(in_sram)
		return;

	if (ring->dma) {
		dma_free_coherent(eth->dev,
				  ring->dma_size * sizeof(*ring->dma),
				  ring->dma,
				  ring->phys);
		ring->dma = NULL;
	}
}

static int mtk_hwlro_rx_init(struct mtk_eth *eth)
{
	int i;
	u32 ring0_ctrl_dw2 = 0;
	u32 ring_ctrl_dw1 = 0, ring_ctrl_dw2 = 0, ring_ctrl_dw3 = 0;
	u32 lro_ctrl_dw0 = 0, lro_ctrl_dw1 = 0, lro_ctrl_dw3 = 0;
	int ring_num;

	u32 tmp = 0;

	// porting from raeth_hwlro.c, set_fe_lro_auto_cfg()
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		lro_ctrl_dw0 = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0);
	}

	// 0 set LAN IP: 192.168.1.1
	mtk_w32(eth, 0xc0a80101, MTK_LRO_RX_RING0_DIP_DW0);
	mtk_w32(eth, 0, MTK_LRO_RX_RING0_DIP_DW1);
	mtk_w32(eth, 0, MTK_LRO_RX_RING0_DIP_DW2);
	mtk_w32(eth, 0, MTK_LRO_RX_RING0_DIP_DW3);
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		ring0_ctrl_dw2 |= MTK_RING_MYIP_VLD;
	}

	// 1 set LRO rings to auto-learn modes
	ring_ctrl_dw2 |= MTK_RING_AUTO_LERAN_MODE;

	// 2 validate LRO ring
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		ring0_ctrl_dw2 |= MTK_RING_VLD;
	}
	ring_ctrl_dw2 |= MTK_RING_VLD;

	// 3 set AGE timer (unit: 20us)
	ring_ctrl_dw2 |= MTK_RING_AGE_TIME_H;
	ring_ctrl_dw1 |= MTK_RING_AGE_TIME_L;

	// 4 set max AGG timer (unit: 20us)
	ring_ctrl_dw2 |= MTK_RING_MAX_AGG_TIME;

	// 5 set max LRO AGG count
	ring_ctrl_dw2 |= MTK_RING_MAX_AGG_CNT_L;
	ring_ctrl_dw3 |= MTK_RING_MAX_AGG_CNT_H;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		//ring0
		mtk_w32(eth, ring_ctrl_dw1, MTK_LRO_CTRL_DW1_CFG(0));
		ring0_ctrl_dw2 = ring_ctrl_dw2 | MTK_RING_MYIP_VLD;
		mtk_w32(eth, ring0_ctrl_dw2, MTK_LRO_CTRL_DW2_CFG(0));
		mtk_w32(eth, ring_ctrl_dw3, MTK_LRO_CTRL_DW3_CFG(0));
	} else {
		mtk_w32(eth, ring0_ctrl_dw2, MTK_LRO_CTRL_DW2_CFG(0));
	}

	for (i = 1; i < MTK_MAX_RX_RING_NUM; i++) {
		mtk_w32(eth, ring_ctrl_dw1, MTK_LRO_CTRL_DW1_CFG(i));
		mtk_w32(eth, ring_ctrl_dw2, MTK_LRO_CTRL_DW2_CFG(i));
		mtk_w32(eth, ring_ctrl_dw3, MTK_LRO_CTRL_DW3_CFG(i));
	}

	// 6 IPV6 enable
	lro_ctrl_dw0 |= LRO_IPv6_EN;

	// 7 IPv4 checksum update enable
	//mark to sync raeth
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		lro_ctrl_dw0 |= MTK_L3_CKS_UPD_EN;
	}

	// 8 RXD prefetch enable setting
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		pr_info("MTK_HAS_CAPS NOT MTK_ADMAV2\n");
		lro_ctrl_dw0 |= MTK_MULTI_RXD_PREFETCH_EN;
		lro_ctrl_dw0 |= MTK_RXD_PREFETCH_EN;
	}

	// 9 switch priority comparison to packet count mode
	lro_ctrl_dw0 |= MTK_LRO_ALT_PKT_CNT_MODE;

	//mark to sync raeth
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		lro_ctrl_dw0 |= MTK_LRO_L4_CTRL_PSH_EN;
	} else {
		lro_ctrl_dw0 &= (~MTK_LRO_L4_CTRL_PSH_EN);
	}

	// 10 bandwidth threshold setting
	mtk_w32(eth, MTK_HW_LRO_BW_THRE, MTK_PDMA_LRO_CTRL_DW2);

	// 11 auto-learn score delta setting
	mtk_w32(eth, MTK_HW_LRO_REPLACE_DELTA, MTK_PDMA_LRO_ALT_SCORE_DELTA);

	// 12 set refresh timer for altering flows to 1 sec. (unit: 20us)
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		mtk_w32(eth, (MTK_HW_LRO_TIMER_UNIT << 16) | MTK_HW_LRO_REFRESH_TIME,
			MTK_PDMA_LRO_ALT_REFRESH_TIMER);
	} else {
#if defined(CONFIG_MTK_ADMAV2)
		mtk_w32(eth, (MTK_HW_LRO_TIMER_UNIT << 16) | MTK_HW_LRO_REFRESH_TIME,
			LRO2_ALT_TIMER);
#endif
	}

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		// 13 set HW LRO mode & the max aggregation count for rx packets
		lro_ctrl_dw3 |= MTK_ADMA_MODE | (MTK_HW_LRO_MAX_AGG_CNT & 0xff);
	}

	// 14 the minimal remaining room of SDL0 in RXD for lro aggregation
	lro_ctrl_dw3 |= MTK_LRO_MIN_RXD_SDL;

	// 15 enable LRO vlan
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		lro_ctrl_dw3 |= MTK_LRO_VLAN_EN;
	}

#if defined(CONFIG_MTK_ADMAV2)
		//set SDL CR
		tmp = mtk_r32(eth, MTK_PDMA_FREEQ_THRES);
		tmp |= 0x3000 << 16;
		mtk_w32(eth, tmp, MTK_PDMA_FREEQ_THRES);
		pr_info("MTK_PDMA_FREEQ_THRES val:%x \n",tmp);

		tmp = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0);
		tmp &=0xfff80007;  //clear bit 18~3 and set relinglish
		tmp |= (0x3000 << 3);
		mtk_w32(eth, tmp, MTK_PDMA_LRO_CTRL_DW0);

		// 15 Polling relinguish
		for (i = 0; i < 10; i++) {
			if (!((mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0) & MTK_LRO_RING_RELINQUISH_DONE)==0xf0000000) ) {
				pr_info("Polling HW LRO RELINGUISH...%d\n", i);
				msleep(20);
				continue;
			} else
				break;
		}
#else
		// 15 Polling relinguish
		for (i = 0; i < 10; i++) {
			if (mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0) & MTK_LRO_RING_RELINQUISH_DONE) {
				pr_info("Polling HW LRO RELINGUISH...%d\n", i);
				msleep(20);
				continue;
			} else
				break;
		}
#endif

	// 16 enable HW LRO
	lro_ctrl_dw0 |= MTK_LRO_EN;

	// 17 enable cpu reason black list
	lro_ctrl_dw0 |= MTK_LRO_CRSN_BNW;

	// 18 no use PPE cpu reason 0xff
	lro_ctrl_dw1 |= MTK_LRO_CPU_REASON;

	mtk_w32(eth, lro_ctrl_dw3, MTK_PDMA_LRO_CTRL_DW3);
	mtk_w32(eth, lro_ctrl_dw1, MTK_PDMA_LRO_CTRL_DW1);

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		lro_ctrl_dw0 &= ~(0x0007fff8);
		lro_ctrl_dw0 |= ((0x3000)<<3);
	}

	mtk_w32(eth, lro_ctrl_dw0, MTK_PDMA_LRO_CTRL_DW0);

	pr_info("MTK_PDMA_LRO_CTRL_DW0 %x\n", mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0));
	pr_info("MTK_PDMA_LRO_CTRL_DW1 %x\n", mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW1));
	pr_info("MTK_PDMA_LRO_CTRL_DW3 %x\n", mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW3));


#if defined(CONFIG_MTK_ADMAV2)
		// ADMAV2 BIT8 DMA LRO EN
		tmp = mtk_r32(eth, MTK_PDMA_GLO_CFG);
		tmp |= MTK_RX_DMA_LRO_EN;
		mtk_w32(eth, tmp, MTK_PDMA_GLO_CFG);
#endif

	return 0;
}

static void mtk_hwlro_rx_uninit(struct mtk_eth *eth)
{
	int i;
	u32 val;

	/* relinquish lro rings, flush aggregated packets */
	mtk_w32(eth, MTK_LRO_RING_RELINQUISH_REQ, MTK_PDMA_LRO_CTRL_DW0);

	/* wait for relinquishments done */
	for (i = 0; i < 10; i++) {
		val = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0);
		if (val & MTK_LRO_RING_RELINQUISH_DONE) {
			msleep(20);
			continue;
		}
		break;
	}

	/* invalidate lro rings */
	for (i = 1; i < MTK_MAX_RX_RING_NUM; i++)
		mtk_w32(eth, 0, MTK_LRO_CTRL_DW2_CFG(i));

	/* disable HW LRO */
	mtk_w32(eth, 0, MTK_PDMA_LRO_CTRL_DW0);

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
		/* reset ADMA prefetch DESC write 0x1 and write 0x0*/
		val = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW3);
		val = val | MTK_ADMA_FW_RSTN_REQ;
		mtk_w32(eth, val, MTK_PDMA_LRO_CTRL_DW3);
		val = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW3);
		mtk_w32(eth, (val & ~MTK_ADMA_FW_RSTN_REQ), MTK_PDMA_LRO_CTRL_DW3);
	}
}

static void mtk_hwlro_val_ipaddr(struct mtk_eth *eth, int idx, __be32 ip)
{
	u32 reg_val;

	reg_val = mtk_r32(eth, MTK_LRO_CTRL_DW2_CFG(idx));

	/* invalidate the IP setting */
	mtk_w32(eth, (reg_val & ~MTK_RING_MYIP_VLD), MTK_LRO_CTRL_DW2_CFG(idx));

	mtk_w32(eth, ip, MTK_LRO_DIP_DW0_CFG(idx));

	/* validate the IP setting */
	mtk_w32(eth, (reg_val | MTK_RING_MYIP_VLD), MTK_LRO_CTRL_DW2_CFG(idx));
}

static void mtk_hwlro_inval_ipaddr(struct mtk_eth *eth, int idx)
{
	u32 reg_val;

	reg_val = mtk_r32(eth, MTK_LRO_CTRL_DW2_CFG(idx));

	/* invalidate the IP setting */
	mtk_w32(eth, (reg_val & ~MTK_RING_MYIP_VLD), MTK_LRO_CTRL_DW2_CFG(idx));

	mtk_w32(eth, 0, MTK_LRO_DIP_DW0_CFG(idx));
}

static int mtk_hwlro_get_ip_cnt(struct mtk_mac *mac)
{
	int cnt = 0;
	int i;

	for (i = 0; i < MTK_MAX_LRO_IP_CNT; i++) {
		if (mac->hwlro_ip[i])
			cnt++;
	}

	return cnt;
}

static int mtk_hwlro_add_ipaddr(struct net_device *dev,
				struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	int hwlro_idx;

	if ((fsp->flow_type != TCP_V4_FLOW) ||
	    (!fsp->h_u.tcp_ip4_spec.ip4dst) ||
	    (fsp->location > 1))
		return -EINVAL;

	mac->hwlro_ip[fsp->location] = htonl(fsp->h_u.tcp_ip4_spec.ip4dst);
	hwlro_idx = (mac->id * MTK_MAX_LRO_IP_CNT) + fsp->location;

	mac->hwlro_ip_cnt = mtk_hwlro_get_ip_cnt(mac);

	mtk_hwlro_val_ipaddr(eth, hwlro_idx, mac->hwlro_ip[fsp->location]);

	return 0;
}

static int mtk_hwlro_del_ipaddr(struct net_device *dev,
				struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	int hwlro_idx;

	if (fsp->location > 1)
		return -EINVAL;

	mac->hwlro_ip[fsp->location] = 0;
	hwlro_idx = (mac->id * MTK_MAX_LRO_IP_CNT) + fsp->location;

	mac->hwlro_ip_cnt = mtk_hwlro_get_ip_cnt(mac);

	mtk_hwlro_inval_ipaddr(eth, hwlro_idx);

	return 0;
}

static void mtk_hwlro_netdev_disable(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	int i, hwlro_idx;

	for (i = 0; i < MTK_MAX_LRO_IP_CNT; i++) {
		mac->hwlro_ip[i] = 0;
		hwlro_idx = (mac->id * MTK_MAX_LRO_IP_CNT) + i;

		mtk_hwlro_inval_ipaddr(eth, hwlro_idx);
	}

	mac->hwlro_ip_cnt = 0;
}

static int mtk_hwlro_get_fdir_entry(struct net_device *dev,
				    struct ethtool_rxnfc *cmd)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;

	/* only tcp dst ipv4 is meaningful, others are meaningless */
	fsp->flow_type = TCP_V4_FLOW;
	fsp->h_u.tcp_ip4_spec.ip4dst = ntohl(mac->hwlro_ip[fsp->location]);
	fsp->m_u.tcp_ip4_spec.ip4dst = 0;

	fsp->h_u.tcp_ip4_spec.ip4src = 0;
	fsp->m_u.tcp_ip4_spec.ip4src = 0xffffffff;
	fsp->h_u.tcp_ip4_spec.psrc = 0;
	fsp->m_u.tcp_ip4_spec.psrc = 0xffff;
	fsp->h_u.tcp_ip4_spec.pdst = 0;
	fsp->m_u.tcp_ip4_spec.pdst = 0xffff;
	fsp->h_u.tcp_ip4_spec.tos = 0;
	fsp->m_u.tcp_ip4_spec.tos = 0xff;

	return 0;
}

static int mtk_hwlro_get_fdir_all(struct net_device *dev,
				  struct ethtool_rxnfc *cmd,
				  u32 *rule_locs)
{
	struct mtk_mac *mac = netdev_priv(dev);
	int cnt = 0;
	int i;

	for (i = 0; i < MTK_MAX_LRO_IP_CNT; i++) {
		if (mac->hwlro_ip[i]) {
			rule_locs[cnt] = i;
			cnt++;
		}
	}

	cmd->rule_cnt = cnt;

	return 0;
}

static netdev_features_t mtk_fix_features(struct net_device *dev,
					  netdev_features_t features)
{
	if (!(features & NETIF_F_LRO)) {
		struct mtk_mac *mac = netdev_priv(dev);
		int ip_cnt = mtk_hwlro_get_ip_cnt(mac);

		if (ip_cnt) {
			netdev_info(dev, "RX flow is programmed, LRO should keep on\n");

			features |= NETIF_F_LRO;
		}
	}

	return features;
}

static int mtk_set_features(struct net_device *dev, netdev_features_t features)
{
	int err = 0;
	struct mtk_mac *mac = netdev_priv(dev);

	if (!((dev->features ^ features) & NETIF_F_LRO))
		return 0;

	if (unlikely(!test_bit(MTK_HW_INIT, &mac->hw->state)))
		return -EBUSY;

	if (!(features & NETIF_F_LRO))
		mtk_hwlro_netdev_disable(dev);

	return err;
}

/* wait for DMA to finish whatever it is doing before we start using it again */
static int mtk_dma_busy_wait(struct mtk_eth *eth)
{
	unsigned long t_start = jiffies;

	while (1) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
			if (!(mtk_r32(eth, MTK_QDMA_GLO_CFG) &
			      (MTK_RX_DMA_BUSY | MTK_TX_DMA_BUSY)))
				return 0;
		} else {
			if (!(mtk_r32(eth, MTK_PDMA_GLO_CFG) &
			      (MTK_RX_DMA_BUSY | MTK_TX_DMA_BUSY)))
				return 0;
		}

		if (time_after(jiffies, t_start + MTK_DMA_BUSY_TIMEOUT))
			break;
	}

	dev_err(eth->dev, "DMA init timeout\n");
	return -1;
}

static int mtk_dma_init(struct mtk_eth *eth)
{
	int err;
	u32 i;
#if defined(CONFIG_MTK_ADMAV2)
	int rss_base, rss_max, lro_base, lro_max;
#endif

	dev_err(eth->dev, "%s: mtk_dma_init in\n", __func__);

	if (mtk_dma_busy_wait(eth))
		return -EBUSY;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		/* QDMA needs scratch memory for internal reordering of the
		 * descriptors
		 */
		err = mtk_init_fq_dma(eth);
		if (err)
			return err;
	}

	err = mtk_tx_alloc(eth);
	if (err)
		return err;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		err = mtk_rx_alloc(eth, 0, MTK_RX_FLAGS_QDMA);
		if (err)
			return err;
	}

	err = mtk_rx_alloc(eth, 0, MTK_RX_FLAGS_NORMAL);
	if (err)
		return err;

#if defined(CONFIG_MTK_ADMAV2)
	if (eth->hwrss || eth->hwlro) {
		rss_base = 1;
		rss_max = MTK_MAX_RSS_RING_NUM; //RSS queue 1~3
		lro_base = MTK_LRO_RX_RING_BASE; //LRO queue 4~7
		lro_max = MTK_MAX_RX_RING_NUM;
	}

	if (eth->hwrss) {
		for (i = rss_base; i < rss_max; i++) {
			err = mtk_rx_alloc(eth, i, MTK_RX_FLAGS_HWRSS);
			if (err)
				return err;
		}
		err = mtk_hwrss_rx_init(eth);
		if (err)
			return err;
	}

	if (eth->hwlro) {
		for (i = lro_base; i < lro_max; i++) {
			err = mtk_rx_alloc(eth, i, MTK_RX_FLAGS_HWLRO);
			if (err)
				return err;
		}
		err = mtk_hwlro_rx_init(eth);
		if (err)
			return err;
	}
#else

	if (eth->hwlro) {
		for (i = 1; i < MTK_MAX_RX_RING_NUM; i++) {
			err = mtk_rx_alloc(eth, i, MTK_RX_FLAGS_HWLRO);
			if (err)
				return err;
		}
		err = mtk_hwlro_rx_init(eth);
		if (err)
			return err;
	}

	if (eth->hwrss) {
		for (i = 1; i < MTK_MAX_RX_RING_NUM; i++) {
			err = mtk_rx_alloc(eth, i, MTK_RX_FLAGS_HWRSS);
			if (err)
				return err;
		}
		err = mtk_hwrss_rx_init(eth);
		if (err)
			return err;
	}
#endif


	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		/* Enable random early drop and set drop threshold
		 * automatically
		 */
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
				mtk_w32(eth, FC_THRES_HW_DROP_EN | FC_THRES_SW_DROP_EN |
					FC_THRES_MIN, MTK_QDMA_FC_THRES);
				for (i = 0; i < 64; i++) {
					if (i <= 15) {
						mtk_w32(eth, 0, MTK_QDMA_SEL);
						mtk_w32(eth, (QDMA_RES_THRES << 8) | QDMA_RES_THRES, MTK_QTX_CFG(i));
						mtk_w32(eth, (TX_SCH_SEL0 << 30) , MTK_QTX_SCH(i));
					} else if (i > 15 && i <= 31) {
						mtk_w32(eth, 1, MTK_QDMA_SEL);
						mtk_w32(eth, (QDMA_RES_THRES << 8) | QDMA_RES_THRES, MTK_QTX_CFG((i-16)));
						mtk_w32(eth, (TX_SCH_SEL1 << 30) , MTK_QTX_SCH((i-16)));
					} else if (i > 31 && i <= 47) {
						mtk_w32(eth, 2, MTK_QDMA_SEL);
						mtk_w32(eth, (QDMA_RES_THRES << 8) | QDMA_RES_THRES, MTK_QTX_CFG((i-32)));
						mtk_w32(eth, (TX_SCH_SEL2 << 30) , MTK_QTX_SCH((i-32)));
					} else if (i > 47 && i <= 59) {
						mtk_w32(eth, 3, MTK_QDMA_SEL);
						mtk_w32(eth, (QDMA_RES_THRES << 8) | QDMA_RES_THRES, MTK_QTX_CFG((i-48)));
						mtk_w32(eth, (TX_SCH_SEL3 << 30) , MTK_QTX_SCH((i-48)));
					}
				}

				/* Pause frame enhancement */
				mtk_eth_auto_qos_init(eth);

				mtk_w32(eth, 0, MTK_QDMA_SEL);

		}else {
			mtk_w32(eth, FC_THRES_DROP_MODE | FC_THRES_DROP_EN |
				FC_THRES_MIN, MTK_QDMA_FC_THRES);
		}
		mtk_w32(eth, 0x0, MTK_QDMA_HRED2);
	}

	return 0;
}

static void mtk_dma_free(struct mtk_eth *eth)
{
	int i;
#if defined(CONFIG_MTK_ADMAV2)
	int rss_base, rss_max, lro_base, lro_max;
#endif


	dev_err(eth->dev, "%s: in\n", __func__);

	for (i = 0; i < MTK_MAC_COUNT; i++)
		if (eth->netdev[i])
			netdev_reset_queue(eth->netdev[i]);
	if (!eth->soc->has_sram_qdma && eth->scratch_ring) {
		dma_free_coherent(eth->dev,
				  MTK_DMA_SIZE * sizeof(struct mtk_tx_dma),
				  eth->scratch_ring,
				  eth->phy_scratch_ring);
		eth->scratch_ring = NULL;
		eth->phy_scratch_ring = 0;
	}
	mtk_tx_clean(eth);
	mtk_rx_clean(eth, &eth->rx_ring[0], 0);
	mtk_rx_clean(eth, &eth->rx_ring_qdma, 0);

#if defined(CONFIG_MTK_ADMAV2)
	if (eth->hwrss && eth->hwlro) {
		rss_base = 1;
		rss_max = MTK_MAX_RSS_RING_NUM;
		lro_base = MTK_LRO_RX_RING_BASE;
		lro_max = MTK_MAX_RX_RING_NUM;
	} else {
		rss_base = 1;
		rss_max = MTK_MAX_RSS_RING_NUM;
		lro_base = 1;
		lro_max = MTK_MAX_LRO_RING_NUM;
	}
	if (eth->hwrss) {
		for (i = rss_base; i < rss_max; i++)
			mtk_rx_clean(eth, &eth->rx_ring[i], 0);
	}

	if (eth->hwlro) {
		mtk_hwlro_rx_uninit(eth);
		for (i = lro_base; i < lro_max; i++)
			mtk_rx_clean(eth, &eth->rx_ring[i], 0);
	}
#else

	if (eth->hwlro) {
		mtk_hwlro_rx_uninit(eth);
		for (i = 1; i < MTK_MAX_RX_RING_NUM; i++)
			mtk_rx_clean(eth, &eth->rx_ring[i], 0);
	}

	if (eth->hwrss) {
		for (i = 1; i < MTK_MAX_RX_RING_NUM; i++)
			mtk_rx_clean(eth, &eth->rx_ring[i], 0);
	}
#endif

	if (eth->phy_scratch_head) {
		dma_unmap_single(eth->dev, eth->phy_scratch_head,
			eth->scratch_head_size,	DMA_FROM_DEVICE);
		eth->phy_scratch_head  = 0;
		eth->scratch_head_size = 0;
	}

	if (eth->scratch_head) {
		kfree(eth->scratch_head);
		eth->scratch_head = NULL;
	}
}
#if(0)
static void mtk_tx_timeout(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;

	eth->netdev[mac->id]->stats.tx_errors++;
	netif_err(eth, tx_err, dev,
		  "transmit timed out\n");
	schedule_work(&eth->pending_work);
}
#endif

static irqreturn_t mtk_handle_irq_rx(int irq, void *_eth)
{
	struct mtk_eth *eth = _eth;

	if (likely(napi_schedule_prep(&eth->rx_napi))) {
		__napi_schedule(&eth->rx_napi);
		mtk_rx_irq_disable(eth, MTK_RX_DONE_INT);
	}

	return IRQ_HANDLED;
}

static irqreturn_t mtk_handle_irq_tx(int irq, void *_eth)
{
	struct mtk_eth *eth = _eth;

	if (likely(napi_schedule_prep(&eth->tx_napi))) {
		__napi_schedule(&eth->tx_napi);
		mtk_tx_irq_disable(eth, MTK_TX_DONE_INT);
	}

	return IRQ_HANDLED;
}

static irqreturn_t mtk_handle_irq(int irq, void *_eth)
{
	struct mtk_eth *eth = _eth;

	if (mtk_r32(eth, MTK_PDMA_INT_MASK) & MTK_RX_DONE_INT) {
		if (mtk_r32(eth, MTK_PDMA_INT_STATUS) & MTK_RX_DONE_INT)
			mtk_handle_irq_rx(irq, _eth);
	}
	if (mtk_r32(eth, eth->tx_int_mask_reg) & MTK_TX_DONE_INT) {
		if (mtk_r32(eth, eth->tx_int_status_reg) & MTK_TX_DONE_INT)
			mtk_handle_irq_tx(irq, _eth);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void mtk_poll_controller(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;

	mtk_tx_irq_disable(eth, MTK_TX_DONE_INT);

	if (eth->hwrss) {
		mtk_rx_irq_disable_rss_all(eth);
		mtk_handle_irq_rrs(eth->irq[2], dev);
		mtk_handle_irq_rrs(eth->irq[3], dev);
		mtk_handle_irq_rrs(eth->irq[4], dev);
		mtk_handle_irq_rrs(eth->irq[5], dev);

		mtk_rx_irq_enable_rss_all(eth);
	} else {
		mtk_rx_irq_disable(eth, MTK_RX_DONE_INT);
		mtk_handle_irq_rx(eth->irq[2], dev);
		mtk_rx_irq_enable(eth, MTK_RX_DONE_INT);
	}

	mtk_tx_irq_enable(eth, MTK_TX_DONE_INT);
}
#endif

static int mtk_start_dma(struct mtk_eth *eth)
{
	u32 rx_2b_offset = (NET_IP_ALIGN == 2) ? MTK_RX_2B_OFFSET : 0;
	int err;
	u32 tmp = 0;

	err = mtk_dma_init(eth);
	if (err) {
		mtk_dma_free(eth);
		return err;
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
		//0x95404535
		tmp = mtk_r32(eth, MTK_QDMA_GLO_CFG);
		tmp &= ~CSR_CLKGATE_BYP;
		tmp &= ~(MTK_QDMA_GLO_RESV_BUF(0xff));
		tmp = tmp | rx_2b_offset | MTK_RX_DMA_EN | MTK_TX_DMA_EN |
			MTK_QDMA_GLO_RESV_BUF(0x80);
		mtk_w32(eth, tmp, MTK_QDMA_GLO_CFG);

		tmp = mtk_r32(eth, MTK_PDMA_GLO_CFG);
#if defined(CONFIG_MTK_ADMAV2)
		//0x0x50404d74 or 0x50404c74
		tmp &= ~(MTK_ADMA_GLO_RESV_BUF(0xff));
		tmp |= MTK_RX_DMA_EN | rx_2b_offset | MTK_MULTI_EN |
			MTK_ADMA_GLO_RESV_BUF(0x80);;
#else
		tmp |= MTK_RX_DMA_EN | rx_2b_offset |
			MTK_RX_BT_32DWORDS | MTK_MULTI_EN;
#endif
		mtk_w32(eth, tmp, MTK_PDMA_GLO_CFG);

	} else if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		mtk_w32(eth,
			MTK_TX_WB_DDONE | MTK_TX_DMA_EN |
			MTK_DMA_SIZE_16DWORDS | MTK_NDP_CO_PRO |
			MTK_RX_DMA_EN | MTK_RX_2B_OFFSET |
			MTK_RX_BT_32DWORDS,
			MTK_QDMA_GLO_CFG);

		mtk_w32(eth,
			MTK_RX_DMA_EN | rx_2b_offset |
			MTK_RX_BT_32DWORDS | MTK_MULTI_EN,
			MTK_PDMA_GLO_CFG);
	} else {
		mtk_w32(eth, MTK_TX_WB_DDONE | MTK_TX_DMA_EN | MTK_RX_DMA_EN |
			MTK_MULTI_EN | MTK_PDMA_SIZE_8DWORDS,
			MTK_PDMA_GLO_CFG);
	}

	return 0;
}

void mtk_gdm_config(struct mtk_eth *eth, u32 config)
{
	int i;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		u32 val = mtk_r32(eth, MTK_GDMA_FWD_CFG(i));

		/* default setup the forward port to send frame to PDMA */
		val &= ~0xffff;
#if defined(CONFIG_HW_NAT)
		if (ppe_hook_tx_eth)
			val |= MTK_GDMA_PPE;
#endif

		/* Enable RX checksum */
		val |= MTK_GDMA_ICS_EN | MTK_GDMA_TCS_EN | MTK_GDMA_UCS_EN;

		val |= config;

		if (eth->netdev[i] && netdev_uses_dsa(eth->netdev[i])) {
			val |= MTK_GDMA_SPECIAL_TAG;
			pr_info("%s mac %d sp enable\n", __func__, i);
		} else {
			pr_info("%s mac %d sp NOT enable\n", __func__, i);
		}

		mtk_w32(eth, val, MTK_GDMA_FWD_CFG(i));
	}
	/* Reset and enable PSE */
	mtk_w32(eth, RST_GL_PSE, MTK_RST_GL);
	mtk_w32(eth, 0, MTK_RST_GL);
}

int mtk_open(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	int err;
	dev_err(eth->dev, "%s: in mac[%d]\n", __func__, mac->id);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	static int is_load_fw = 0;

	u16 phy_cr1, addr;
	if((mac->id == 0 && eth->soc->aqr113c_phy0_support) || (mac->id == 1 && eth->soc->aqr113c_phy1_support))
	{
		addr = mac->ql_sgmii_phy_data.phy_addr;
		//Aqr113c, close mdio broadcast respond
		phy_cr1 = _mtk_mdio_read_C45(eth, addr, 0xC441, MDIO_MMD_VEND1);
		ClrBit(phy_cr1, 14);
		_mtk_mdio_write_C45(eth, addr, 0xC441, phy_cr1, MDIO_MMD_VEND1);
	}
	if((mac->id == 0 && eth->soc->rtl8251_phy0_support) || (mac->id == 1 && eth->soc->rtl8251_phy1_support))
	{
		addr = mac->ql_sgmii_phy_data.phy_addr;
		//Rtl8221 || Rtl8251, close mdio broadcast respond
		phy_cr1 = _mtk_mdio_read_C45(eth, addr, 0xA430, MDIO_MMD_VEND2);
		ClrBit(phy_cr1, 13);
		_mtk_mdio_write_C45(eth, addr, 0xA430, phy_cr1, MDIO_MMD_VEND2);
	}
	if((mac->id == 0 && eth->soc->yt8821_phy0_support) || (mac->id == 1 && eth->soc->yt8821_phy1_support))
	{
		addr = mac->ql_sgmii_phy_data.phy_addr;
		//Yt8821, close mdio broadcast respond
		phy_cr1 = ytphy_read_ext(eth, addr, 0xa005);
		phy_cr1 &= ~(BIT(5) | BIT(6));
		ytphy_write_ext(eth, addr, 0xa005, phy_cr1);
	}
	if(is_load_fw == 0 && (eth->soc->aqr113c_phy0_support || eth->soc->aqr113c_phy1_support))
	{
		is_load_fw = 1;
		if(eth->soc->aqr113c_phy0_support) {
			ql_phy_load_firmware(eth, 0, AQR113C_PHY0);
		}
		if(eth->soc->aqr113c_phy1_support) {
			ql_phy_load_firmware(eth, 1, AQR113C_PHY1);
		}
	}
	if (mac->id == 0 && eth->soc->rtl8221_phy0_support) {
		ql_phy_start(eth, RTL8221_PHY0);
		disable_irq_nosync(eth->irq[6]);
		enable_irq(eth->irq[6]);
		pr_err("%s.%d RTL8221_PHY0 start\n", __func__, __LINE__);
	}
	if (mac->id == 1 && eth->soc->rtl8221_phy1_support) {
		ql_phy_start(eth, RTL8221_PHY1);
		disable_irq_nosync(eth->irq[7]);
		enable_irq(eth->irq[7]);
		pr_err("%s.%d RTL8221_PHY1 start\n", __func__, __LINE__);
	}
#endif
	if (eth->soc->sgmii_pm) {
		regulator_set_voltage(eth->dvfsrc_vcore_power, SGMII_VCORE_OPS, INT_MAX);
	}

	if (mac->id == 0 && eth->soc->aqr112c_phy0_support) {
		disable_irq_nosync(eth->irq[6]);
		enable_irq(eth->irq[6]);
	}

	err = phylink_of_phy_connect(mac->phylink, mac->of_node, 0);
	if (err) {
		netdev_err(dev, "%s: could not attach PHY: %d\n", __func__,
			   err);
		return err;
	}

	dev_err(eth->dev, "%s: mtk_open refcount  %d\n",
		__func__, refcount_read(&eth->dma_refcnt));

	/* Indicates CDM to parse the MTK special tag from CPU */
	if (netdev_uses_dsa(dev)) {
		u32 val;
		val = mtk_r32(eth, MTK_CDMQ_IG_CTRL);
		mtk_w32(eth, val | MTK_CDMQ_STAG_EN, MTK_CDMQ_IG_CTRL);
		val = mtk_r32(eth, MTK_CDMP_IG_CTRL);
		mtk_w32(eth, val | MTK_CDMP_STAG_EN, MTK_CDMP_IG_CTRL);
		dev_err(eth->dev, "%s: netdev_uses_dsa in mac[%d]\n", __func__,mac->id);
	} else
		dev_err(eth->dev, "%s: netdev_uses_dsa NOT in mac[%d]\n", __func__,mac->id);


	/* we run 2 netdevs on the same dma ring so we only bring it up once */
	if (!refcount_read(&eth->dma_refcnt)) {
		err = mtk_start_dma(eth);

		if (err){
			dev_err(eth->dev, "%s: mtk_start_dma err\n", __func__);
			return err;
		}
		mtk_gdm_config(eth, MTK_GDMA_ICS_EN | MTK_GDMA_TCS_EN | MTK_GDMA_UCS_EN);

		napi_enable(&eth->tx_napi);
		if(eth->hwrss) {
			napi_enable(&eth->rx_rss_napi[0].rx_napi);
			napi_enable(&eth->rx_rss_napi[1].rx_napi);
			napi_enable(&eth->rx_rss_napi[2].rx_napi);
			napi_enable(&eth->rx_rss_napi[3].rx_napi);
		} else {
			napi_enable(&eth->rx_napi);
		}
		mtk_tx_irq_enable(eth, MTK_TX_DONE_INT);
		if(eth->hwrss) {
			mtk_rx_irq_enable_rss_all(eth);
		} else {
			mtk_rx_irq_enable(eth, MTK_RX_DONE_INT);
		}

#if defined(CONFIG_HW_NAT)
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_AUTO))
			mtk_register_fast_path();
#endif

		refcount_set(&eth->dma_refcnt, 1);
	}
	else {
		refcount_inc(&eth->dma_refcnt);
	}

	phylink_start(mac->phylink);

	netif_start_queue(dev);

	return 0;
}

static void mtk_stop_dma(struct mtk_eth *eth, u32 glo_cfg)
{
	u32 val;
	int i;

	/* stop the dma engine */
	spin_lock_bh(&eth->page_lock);
	val = mtk_r32(eth, glo_cfg);
	mtk_w32(eth, val & ~(MTK_TX_WB_DDONE | MTK_RX_DMA_EN | MTK_TX_DMA_EN),
		glo_cfg);
	spin_unlock_bh(&eth->page_lock);

	/* wait for dma stop */
	for (i = 0; i < 10; i++) {
		val = mtk_r32(eth, glo_cfg);
		if (val & (MTK_TX_DMA_BUSY | MTK_RX_DMA_BUSY)) {
			msleep(20);
			continue;
		}
		break;
	}
}

int mtk_stop(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;

	dev_err(eth->dev, "%s: in mac[%d]\n", __func__, mac->id);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	if (mac->id == 0 && eth->soc->rtl8221_phy0_support) {
		ql_phy_stop(eth, RTL8221_PHY0);
	}
	if (mac->id == 1 && eth->soc->rtl8221_phy1_support) {
		ql_phy_stop(eth, RTL8221_PHY1);
	}
#endif

	phylink_stop(mac->phylink);

	netif_tx_disable(dev);

	phylink_disconnect_phy(mac->phylink);


	dev_err(eth->dev, "%s: in mac[%d] done\n", __func__, mac->id);

	/* only shutdown DMA if this is the last user */
	if (!refcount_dec_and_test(&eth->dma_refcnt))
		return 0;

#if defined(CONFIG_HW_NAT)
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_AUTO))
		mtk_unregister_fast_path();
#endif

	mtk_gdm_config(eth, MTK_GDMA_DROP_ALL);

	mtk_tx_irq_disable(eth, MTK_TX_DONE_INT);

	if(eth->hwrss) {
		mtk_rx_irq_disable_rss_all(eth);
	} else {
		mtk_rx_irq_disable(eth, MTK_RX_DONE_INT);
	}

	napi_disable(&eth->tx_napi);

	if(eth->hwrss) {
		napi_disable(&eth->rx_rss_napi[0].rx_napi);
		napi_disable(&eth->rx_rss_napi[1].rx_napi);
		napi_disable(&eth->rx_rss_napi[2].rx_napi);
		napi_disable(&eth->rx_rss_napi[3].rx_napi);
	} else {
		napi_disable(&eth->rx_napi);
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA))
		mtk_stop_dma(eth, MTK_QDMA_GLO_CFG);
	mtk_stop_dma(eth, MTK_PDMA_GLO_CFG);

	mtk_dma_free(eth);
	if (eth->soc->sgmii_pm) {
		regulator_set_voltage(eth->dvfsrc_vcore_power, SGMII_VCORE_NON_OPS, INT_MAX);
	}
	return 0;
}

static void ethsys_reset(struct mtk_eth *eth, u32 reset_bits)
{
	regmap_update_bits(eth->ethsys, ETHSYS_RSTCTRL,
			   reset_bits,
			   reset_bits);

	usleep_range(1000, 1100);
	regmap_update_bits(eth->ethsys, ETHSYS_RSTCTRL,
			   reset_bits,
			   ~reset_bits);
	mdelay(10);
}

static void mtk_clk_disable(struct mtk_eth *eth)
{
	int clk;

	for (clk = MTK_CLK_MAX - 1; clk >= 0; clk--) {
		pr_notice("%s clk_disable_unprepare %d %s",
			__func__, clk, mtk_clks_source_name[clk]);
		clk_disable_unprepare(eth->clks[clk]);

	}
}

static int mtk_clk_enable(struct mtk_eth *eth)
{
	int clk, ret;

	for (clk = 0; clk < MTK_CLK_MAX ; clk++) {

		ret = clk_prepare_enable(eth->clks[clk]);
		if (ret) {
			pr_notice("%s clk_prepare_enable fail %d %s, ret:%d",
				__func__, clk, mtk_clks_source_name[clk], ret);
			goto err_disable_clks;
		}
	}

	return 0;

err_disable_clks:
	while (--clk >= 0)
		clk_disable_unprepare(eth->clks[clk]);

	return ret;
}



int mtk_hw_init(struct mtk_eth *eth)
{
	int i, ret;
	u32 reg_val = 0;

	if (test_and_set_bit(MTK_HW_INIT, &eth->state))
		return 0;

	/* acquire netsys frame engine resource before init eth. */
	if(!eth->frame_engine)
		acquire_FE("ethernet", &eth->frame_engine);

	if (!(MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK))) {
		ret = mtk_clk_enable(eth);
		if (ret)
			goto err_disable_pm;
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628)) {
		ret = device_reset(eth->dev);
		if (ret) {
			dev_err(eth->dev, "MAC reset failed!\n");
			goto err_disable_pm;
		}

		/* enable interrupt delay for RX */
		mtk_w32(eth, MTK_PDMA_DELAY_RX_DELAY, MTK_PDMA_DELAY_INT);

		/* disable delay and normal interrupt */
		mtk_tx_irq_disable(eth, ~0);
		mtk_rx_irq_disable(eth, ~0);

		return 0;
	}

	/* Non-MT7628 handling... */
	//ethsys_reset(eth, RSTCTRL_FE);
	ethsys_reset(eth, RSTCTRL_PPE);

	/* Set FE to PDMAv2 if necessary */
#if defined(CONFIG_MTK_ADMAV2)
	mtk_w32(eth, mtk_r32(eth, MTK_FE_GLO_MISC) | ADMA_VER_SEL, MTK_FE_GLO_MISC);
#endif


	/* mark need test on colgin
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
		regmap_read(eth->ethsys, ETHSYS_SYSCFG0, &val);
		for (i = 0; i < MTK_MAC_COUNT; i++) {
			if (!eth->mac[i])
				continue;
			val &= ~SYSCFG0_GE_MODE(SYSCFG0_GE_MASK, eth->mac[i]->id);
			val |= SYSCFG0_GE_MODE(0, eth->mac[i]->id);
		}
		regmap_write(eth->ethsys, ETHSYS_SYSCFG0, val);
	}*/

	if (eth->pctl) {
		/* Set GE2 driving and slew rate */
		regmap_write(eth->pctl, GPIO_DRV_SEL10, 0xa00);

		/* set GE2 TDSEL */
		regmap_write(eth->pctl, GPIO_OD33_CTRL8, 0x5);

		/* set GE2 TUNE */
		regmap_write(eth->pctl, GPIO_BIAS_CTRL, 0x0);
	}

	/* Set linkdown as the default for each GMAC. Its own MCR would be set
	 * up with the more appropriate value when mtk_mac_config call is being
	 * invoked.
	 */
	for (i = 0; i < MTK_MAC_COUNT; i++)
		mtk_w32(eth, 0, MTK_MAC_MCR(i));
		//mtk_w32(eth, MAC_MCR_FORCE_LINK_DOWN, MTK_MAC_MCR(i));

	/* Indicates CDM to parse the MTK special tag from CPU
	 * which also is working out for untag packets.
	 */
	//val = mtk_r32(eth, MTK_CDMQ_IG_CTRL);
	//mtk_w32(eth, val | MTK_CDMQ_STAG_EN, MTK_CDMQ_IG_CTRL);

	/* Disable RX VLan Offloading */
	mtk_w32(eth, 0, MTK_CDMP_EG_CTRL);

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
#if defined(CONFIG_MTK_ADMAV2)
		mtk_w32(eth, 0x8f0f8f0f, MTK_PDMA_DELAY_INT);
		//*ADMA only use RX, DO WE need enable TX DELY INT????*/
#else
		mtk_w32(eth, 0x8f0f8f0f, MTK_PDMA_DELAY_INT);
		if (eth->hwrss) {
			mtk_w32(eth, 0x8f0f8f0f, MTK_PDMA1_DELAY_INT);
			mtk_w32(eth, 0x8f0f8f0f, MTK_PDMA2_DELAY_INT);
			mtk_w32(eth, 0x8f0f8f0f, MTK_PDMA3_DELAY_INT);
		}
#endif
		mtk_w32(eth, 0x8f0f8f0f, MTK_QDMA_DELAY_INT);

		mtk_tx_irq_disable(eth, ~0);

		if (eth->hwrss) {
			mtk_rx_irq_disable_rss_all(eth);
		} else {
			mtk_rx_irq_disable(eth, ~0);
		}

		/* FE int grouping */
		/* Enable multipe rx ring delay interrupt */
		if (eth->hwrss) {
			reg_val = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0);
			reg_val |= MTK_LRO_DLY_INT_EN;
			mtk_w32(eth, reg_val, MTK_PDMA_LRO_CTRL_DW0);
			mtk_w32(eth, 0x2020000, MTK_PDMA_INT_GRP1);
			mtk_w32(eth, 0x4040000, MTK_PDMA_INT_GRP2);
			mtk_w32(eth, 0x8080000, MTK_PDMA_INT_GRP3);
			mtk_w32(eth, 0x01000000, MTK_FE_INT_GRP);
		} else {
			//RX use MTK_PDMA_INT_GRP0
			mtk_w32(eth, 0x0, MTK_PDMA_INT_GRP1);
			mtk_w32(eth, 0x0, MTK_PDMA_INT_GRP2);
			mtk_w32(eth, 0x0, MTK_PDMA_INT_GRP3);

			//mtk_w32(eth, MTK_RX_DONE_INT, MTK_PDMA_INT_GRP2);
			mtk_w32(eth, 0x01000000, MTK_FE_INT_GRP); //set FE_INT no RX ADMA INT
		}
		mtk_w32(eth, MTK_TX_DONE_INT, MTK_QDMA_INT_GRP1);
		//mtk_w32(eth, MTK_RX_DONE_INT, MTK_QDMA_INT_GRP2); ///maybe no need to set

		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			mtk_w32(eth, 0x04000404, 0x164c);
#else
			//#Global FC threshold
			mtk_w32(eth, 0x01fa01f4, 0x104);
			//#Per CDM Input threshold update
			mtk_w32(eth, 0x001a000e, 0x140);
			mtk_w32(eth, 0x01ff001a, 0x144);
			mtk_w32(eth, 0x000e01ff, 0x148);
			mtk_w32(eth, 0x000e000e, 0x14c);
			mtk_w32(eth, 0x000e000e, 0x150);
			mtk_w32(eth, 0x000e000e, 0x154);
			mtk_w32(eth, 0x000e000e, 0x158);
			mtk_w32(eth, 0x000e000e, 0x15c);
			//#Per port Output threshold update
			mtk_w32(eth, 0x000f000a, 0x160);
			mtk_w32(eth, 0x001a000f, 0x164);
			mtk_w32(eth, 0x000f001a, 0x168);
			mtk_w32(eth, 0x01ff000f, 0x16c);
			mtk_w32(eth, 0x000f000f, 0x170);
			mtk_w32(eth, 0x0006000f, 0x174);
			mtk_w32(eth, 0x00060006, 0x178);
			mtk_w32(eth, 0x00060006, 0x17c);
			mtk_w32(eth, 0x00000004, 0x1530);
			mtk_w32(eth, 0x00000004, 0x164c);
			mtk_w32(eth, 0x00000004, 0x1650);
			mtk_w32(eth, 0x00000004, 0x1654);
			mtk_w32(eth, 0x00000004, 0x1658);
			mtk_w32(eth, 0x00000004, 0x165c);
#endif
		}

		/*Notice: The following modification is to set Single PHY chip
		This Solution is only for AQR112C Chip. If Single PHY chip is not
		AQR112C, the solution is not avaliable.
		If Single PHY chip is not AQR112C, the following step you should do:
		1. Contact your Single PHY chip vendor and get the details of
			- how to enable flow control
			- how to Enables link status change alarm
			- how to Enables vendor Auto-Neg alarm
			- how to Enables Chip-wide vendor alarm
			- restart AN
		2. Modify the code for the customization.
		*/
		if (eth->soc->aqr112c_phy0_support)
			mtk_flow_ctrl_aqr(eth, AQR112C_PHY0);
#if !defined(CONFIG_QUECTEL_ETHERNET_PHY)
		if (eth->soc->aqr113c_phy0_support)
			mtk_flow_ctrl_aqr(eth, AQR113C_PHY0);

		/* 10G Reference design: AQR113C */
		if (eth->soc->aqr113c_phy1_support)
			mtk_flow_ctrl_aqr(eth, AQR113C_PHY1);
#endif
		/*Modify End*/
	} else {

		/* enable interrupt delay for RX */
		mtk_w32(eth, MTK_PDMA_DELAY_RX_DELAY, MTK_PDMA_DELAY_INT);

		/* disable delay and normal interrupt */
		mtk_w32(eth, 0, MTK_QDMA_DELAY_INT);
		mtk_tx_irq_disable(eth, ~0);
		mtk_rx_irq_disable(eth, ~0);

		/* FE int grouping */
		mtk_w32(eth, MTK_TX_DONE_INT, MTK_PDMA_INT_GRP1);
		mtk_w32(eth, MTK_RX_DONE_INT, MTK_PDMA_INT_GRP2);
		mtk_w32(eth, MTK_TX_DONE_INT, MTK_QDMA_INT_GRP1);
		mtk_w32(eth, MTK_RX_DONE_INT, MTK_QDMA_INT_GRP2);
		mtk_w32(eth, 0x21021000, MTK_FE_INT_GRP);
		for (i = 0; i < MTK_MAC_COUNT; i++) {
			u32 val = mtk_r32(eth, MTK_GDMA_FWD_CFG(i));
			/* setup the forward port to send frame to PDMA */
			val &= ~0xffff;
			/* Enable RX checksum */
			val |= MTK_GDMA_ICS_EN | MTK_GDMA_TCS_EN | MTK_GDMA_UCS_EN;
			/* setup the mac dma */
			mtk_w32(eth, val, MTK_GDMA_FWD_CFG(i));
		}
	}
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	//Modify mdio frequency
	reg_val = mtk_r32(eth, MTK_MAC_PPSC);
	if(!(eth->soc->en8801_phy0_support==true || eth->soc->en8801_phy0_support==true))
	{
		reg_val &= ~(0x3f <<24);
		reg_val |= 0x1<<24;

		mtk_w32(eth, reg_val, MTK_MAC_PPSC);
		pr_info("Modify mdio frequency to 5.27M\n");
	}
#endif
	return 0;

err_disable_pm:
	return ret;
}

int mtk_hw_deinit(struct mtk_eth *eth)
{
	if (!test_and_clear_bit(MTK_HW_INIT, &eth->state))
		return 0;

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK)) {
		mtk_clk_disable(eth);
	}

	/* release netsys frame engine resource after all finished. */
	if (eth->frame_engine) {
		release_FE(eth->frame_engine);
		eth->frame_engine = 0;
	}

	return 0;
}

static int __init mtk_init(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	//struct mtk_eth *eth = mac->hw;
	const char *mac_addr;

	//check if set mac addr in qraw
#ifdef __RAWDATA_MAC_ADDR
char *pop_rawdata_mac_addr(int mac_nr);
	struct mtk_eth *eth = mac->hw;
	mac_addr = pop_rawdata_mac_addr(mac->id);
	if (is_valid_ether_addr(mac_addr)) {
		ether_addr_copy(dev->dev_addr, mac_addr);
	}
	else {
		dev_err(eth->dev, "Rawdata Pop MAC%d address is invalid\n", mac->id);
#endif
		mac_addr = of_get_mac_address(mac->of_node);
		if (!IS_ERR(mac_addr))
			ether_addr_copy(dev->dev_addr, mac_addr);

		/* If the mac address is invalid, use random mac address  */
		if (!is_valid_ether_addr(dev->dev_addr)) {
			eth_hw_addr_random(dev);
			//sensitive log is not allowed
			//dev_err(eth->dev, "generated random MAC address\n");
		}
#ifdef __RAWDATA_MAC_ADDR
	}
#endif
	return 0;
}

static void mtk_uninit(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;

	phylink_disconnect_phy(mac->phylink);
	mtk_tx_irq_disable(eth, ~0);
	mtk_rx_irq_disable(eth, ~0);
}

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static int mtk_change_mtu(struct net_device *dev)
{
	int new_mtu = dev->mtu;
	int length = new_mtu + MTK_RX_ETH_HLEN;
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	u32 mcr_cur, mcr_new;
	mcr_cur = mtk_r32(mac->hw, MTK_MAC_MCR(mac->id));
	mcr_new = mcr_cur & ~MAC_MCR_MAX_RX_MASK;
	if (length <= 1518)
		mcr_new |= MAC_MCR_MAX_RX(MAC_MCR_MAX_RX_1518);
	else if (length <= 1536)
		mcr_new |= MAC_MCR_MAX_RX(MAC_MCR_MAX_RX_1536);
	else if (length <= 1552)
		mcr_new |= MAC_MCR_MAX_RX(MAC_MCR_MAX_RX_1552);
	else
		mcr_new |= MAC_MCR_MAX_RX(MAC_MCR_MAX_RX_2048);
	if (mcr_new != mcr_cur)
		mtk_w32(mac->hw, mcr_new, MTK_MAC_MCR(mac->id));
	pr_info("[quectel_phy] %s.%d addr:0x%X mtu:%d\r\n", __func__, __LINE__, mac->hw->base + MAC_MCR_MAX_RX(MAC_MCR_MAX_RX_2048), new_mtu);
	return 0;
}
#endif

static int mtk_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct mtk_mac *mac = netdev_priv(dev);


	if (unlikely(!test_bit(MTK_HW_INIT, &mac->hw->state)))
		return -EBUSY;

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return phylink_mii_ioctl(mac->phylink, ifr, cmd);
	default:
		/* default invoke the mtk_eth_dbg handler */
		return mtk_do_priv_ioctl(dev, ifr, cmd);
		break;
	}

	return -EOPNOTSUPP;
}
#if(0)
static void mtk_pending_work(struct work_struct *work)
{
	struct mtk_eth *eth = container_of(work, struct mtk_eth, pending_work);
	int err, i;
	unsigned long restart = 0;

	rtnl_lock();

	dev_dbg(eth->dev, "[%s][%d] reset\n", __func__, __LINE__);

	while (test_and_set_bit_lock(MTK_RESETTING, &eth->state))
		cpu_relax();

	dev_dbg(eth->dev, "[%s][%d] mtk_stop starts\n", __func__, __LINE__);
	/* stop all devices to make sure that dma is properly shut down */
	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		mtk_stop(eth->netdev[i]);
		__set_bit(i, &restart);
	}
	dev_dbg(eth->dev, "[%s][%d] mtk_stop ends\n", __func__, __LINE__);

	/* restart underlying hardware such as power, clock, pin mux
	 * and the connected phy
	 */
	mtk_hw_deinit(eth);

	if (eth->dev->pins)
		pinctrl_select_state(eth->dev->pins->p,
				     eth->dev->pins->default_state);
	mtk_hw_init(eth);

	/* restart DMA and enable IRQs */
	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!test_bit(i, &restart))
			continue;
		err = mtk_open(eth->netdev[i]);
		if (err) {
			netif_alert(eth, ifup, eth->netdev[i],
			      "Driver up/down cycle failed, closing device.\n");
			dev_close(eth->netdev[i]);
		}
	}

	dev_dbg(eth->dev, "[%s][%d] reset done\n", __func__, __LINE__);

	clear_bit_unlock(MTK_RESETTING, &eth->state);

	rtnl_unlock();
}
#endif
static int mtk_free_dev(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		free_netdev(eth->netdev[i]);
	}

	return 0;
}

static int mtk_unreg_dev(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		unregister_netdev(eth->netdev[i]);
	}

	return 0;
}

static int mtk_cleanup(struct mtk_eth *eth)
{
	mtk_unreg_dev(eth);
	mtk_free_dev(eth);
	cancel_work_sync(&eth->pending_work);

	return 0;
}

static int mtk_get_link_ksettings(struct net_device *ndev,
				  struct ethtool_link_ksettings *cmd)
{
	struct mtk_mac *mac = netdev_priv(ndev);

	if (unlikely(test_bit(MTK_RESETTING, &mac->hw->state)))
		return -EBUSY;

	return phylink_ethtool_ksettings_get(mac->phylink, cmd);
}

static int mtk_set_link_ksettings(struct net_device *ndev,
				  const struct ethtool_link_ksettings *cmd)
{
	struct mtk_mac *mac = netdev_priv(ndev);

	if (unlikely(test_bit(MTK_RESETTING, &mac->hw->state)))
		return -EBUSY;

	return phylink_ethtool_ksettings_set(mac->phylink, cmd);
}

static void mtk_get_drvinfo(struct net_device *dev,
			    struct ethtool_drvinfo *info)
{
	struct mtk_mac *mac = netdev_priv(dev);

	strlcpy(info->driver, mac->hw->dev->driver->name, sizeof(info->driver));
	strlcpy(info->bus_info, dev_name(mac->hw->dev), sizeof(info->bus_info));
	info->n_stats = ARRAY_SIZE(mtk_ethtool_stats);
}

static u32 mtk_get_msglevel(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);

	return mac->hw->msg_enable;
}

static void mtk_set_msglevel(struct net_device *dev, u32 value)
{
	struct mtk_mac *mac = netdev_priv(dev);

	mac->hw->msg_enable = value;
}

static int mtk_nway_reset(struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);

	if (unlikely(test_bit(MTK_RESETTING, &mac->hw->state)))
		return -EBUSY;

	if (!mac->phylink)
		return -ENOTSUPP;

	return phylink_ethtool_nway_reset(mac->phylink);
}

static void mtk_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	int i;

	switch (stringset) {
	case ETH_SS_STATS:
		for (i = 0; i < ARRAY_SIZE(mtk_ethtool_stats); i++) {
			memcpy(data, mtk_ethtool_stats[i].str, ETH_GSTRING_LEN);
			data += ETH_GSTRING_LEN;
		}
		break;
	}
}

static int mtk_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(mtk_ethtool_stats);
	default:
		return -EOPNOTSUPP;
	}
}

static void mtk_get_ethtool_stats(struct net_device *dev,
				  struct ethtool_stats *stats, u64 *data)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_hw_stats *hwstats = mac->hw_stats;
	u64 *data_src, *data_dst;
	unsigned int start;
	int i;

	if (unlikely(test_bit(MTK_RESETTING, &mac->hw->state)))
		return;

	if (netif_running(dev) && netif_device_present(dev)) {
		if (spin_trylock_bh(&hwstats->stats_lock)) {
			mtk_stats_update_mac(mac);
			spin_unlock_bh(&hwstats->stats_lock);
		}
	}

	data_src = (u64 *)hwstats;

	do {
		data_dst = data;
		start = u64_stats_fetch_begin_irq(&hwstats->syncp);

		for (i = 0; i < ARRAY_SIZE(mtk_ethtool_stats); i++)
			*data_dst++ = *(data_src + mtk_ethtool_stats[i].offset);
	} while (u64_stats_fetch_retry_irq(&hwstats->syncp, start));
}

static int mtk_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
			 u32 *rule_locs)
{
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		if (dev->hw_features & NETIF_F_LRO) {
			cmd->data = MTK_MAX_RX_RING_NUM;
			ret = 0;
		}
		break;
	case ETHTOOL_GRXCLSRLCNT:
		if (dev->hw_features & NETIF_F_LRO) {
			struct mtk_mac *mac = netdev_priv(dev);

			cmd->rule_cnt = mac->hwlro_ip_cnt;
			ret = 0;
		}
		break;
	case ETHTOOL_GRXCLSRULE:
		if (dev->hw_features & NETIF_F_LRO)
			ret = mtk_hwlro_get_fdir_entry(dev, cmd);
		break;
	case ETHTOOL_GRXCLSRLALL:
		if (dev->hw_features & NETIF_F_LRO)
			ret = mtk_hwlro_get_fdir_all(dev, cmd,
						     rule_locs);
		break;
	default:
		break;
	}

	return ret;
}

static int mtk_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd)
{
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		if (dev->hw_features & NETIF_F_LRO)
			ret = mtk_hwlro_add_ipaddr(dev, cmd);
		break;
	case ETHTOOL_SRXCLSRLDEL:
		if (dev->hw_features & NETIF_F_LRO)
			ret = mtk_hwlro_del_ipaddr(dev, cmd);
		break;
	default:
		break;
	}

	return ret;
}

static const struct ethtool_ops mtk_ethtool_ops = {
	.get_link_ksettings	= mtk_get_link_ksettings,
	.set_link_ksettings	= mtk_set_link_ksettings,
	.get_drvinfo		= mtk_get_drvinfo,
	.get_msglevel		= mtk_get_msglevel,
	.set_msglevel		= mtk_set_msglevel,
	.nway_reset		= mtk_nway_reset,
	.get_link		= ethtool_op_get_link,
	.get_strings		= mtk_get_strings,
	.get_sset_count		= mtk_get_sset_count,
	.get_ethtool_stats	= mtk_get_ethtool_stats,
	.get_rxnfc		= mtk_get_rxnfc,
	.set_rxnfc              = mtk_set_rxnfc,
};

static const struct net_device_ops mtk_netdev_ops = {
	.ndo_init		= mtk_init,
	.ndo_uninit		= mtk_uninit,
	.ndo_open		= mtk_open,
	.ndo_stop		= mtk_stop,
	.ndo_start_xmit		= mtk_start_xmit,
	.ndo_set_mac_address	= mtk_set_mac_address,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= mtk_do_ioctl,
//	.ndo_tx_timeout		= mtk_tx_timeout,
	.ndo_get_stats64        = mtk_get_stats64,
	.ndo_fix_features	= mtk_fix_features,
	.ndo_set_features	= mtk_set_features,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= mtk_poll_controller,
#endif
};

static int mtk_add_mac(struct mtk_eth *eth, struct device_node *np)
{
	const __be32 *_id = of_get_property(np, "reg", NULL);
	struct phylink *phylink;
	int phy_mode, id = 0, err;
	struct mtk_mac *mac;
	struct device_node *fixed_link_node;

	if (!_id) {
		dev_err(eth->dev, "missing mac id\n");
		return -EINVAL;
	}

	id = be32_to_cpup(_id);
	if (id >= MTK_MAC_COUNT  || id < 0) {
		dev_err(eth->dev, "%d is not a valid mac id\n", id);
		return -EINVAL;
	}

	if (eth->netdev[id]) {
		dev_err(eth->dev, "duplicate mac id found: %d\n", id);
		return -EINVAL;
	}

	eth->netdev[id] = alloc_etherdev(sizeof(*mac));
	if (!eth->netdev[id]) {
		dev_err(eth->dev, "alloc_etherdev failed\n");
		return -ENOMEM;
	}
	mac = netdev_priv(eth->netdev[id]);
	eth->mac[id] = mac;
	mac->id = id;
	mac->hw = eth;
	mac->of_node = np;
	mac->sgmii_force_en = 0;

	memset(mac->hwlro_ip, 0, sizeof(mac->hwlro_ip));
	mac->hwlro_ip_cnt = 0;

	mac->hw_stats = devm_kzalloc(eth->dev,
				     sizeof(*mac->hw_stats),
				     GFP_KERNEL);
	if (!mac->hw_stats) {
		dev_err(eth->dev, "failed to allocate counter memory\n");
		err = -ENOMEM;
		goto free_netdev;
	}
	spin_lock_init(&mac->hw_stats->stats_lock);
	u64_stats_init(&mac->hw_stats->syncp);
	mac->hw_stats->reg_offset = id * MTK_STAT_OFFSET;

	/* phylink create */
	phy_mode = of_get_phy_mode(np);
	if (phy_mode < 0) {
		dev_err(eth->dev, "incorrect phy-mode\n");
		err = -EINVAL;
		goto free_netdev;
	}

	if (phy_mode == PHY_INTERFACE_MODE_10GKR ||
		phy_mode == PHY_INTERFACE_MODE_USXGMII) {
		mac->max_speed = 10000;
	} else {

		fixed_link_node = of_get_child_by_name(np, "fixed-link");
		if (fixed_link_node) {
			if (of_property_read_u32(fixed_link_node, "speed", &mac->max_speed)) {
				of_node_put(fixed_link_node);
				pr_info("%s.%d: Warning: can't get max_speed id: %d\n", __func__, __LINE__, id);
				mac->max_speed = 1000;
			}
		} else {
			pr_info("%s.%d: Warning: can't get max_speed id: %d\n", __func__, __LINE__, id);
			mac->max_speed = 1000;
		}
	}

	/* mac config is not set */
	mac->interface = PHY_INTERFACE_MODE_NA;
	mac->mode = MLO_AN_PHY;
	mac->speed = SPEED_UNKNOWN;

	mac->phylink_config.dev = &eth->netdev[id]->dev;
	mac->phylink_config.type = PHYLINK_NETDEV;


	phylink = phylink_create(&mac->phylink_config,
				 of_fwnode_handle(mac->of_node),
				 phy_mode, &mtk_phylink_ops);
	if (IS_ERR(phylink)) {
		err = PTR_ERR(phylink);
		goto free_netdev;
	}

	mac->phylink = phylink;
	mac->gmii_decision = -1;

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	//For fixed-link phy(such as 2.5G phy) link status hook.
	if((mac->id == 0 && mac->hw->soc->rtl8221_phy0_support) || (mac->id == 1 && mac->hw->soc->rtl8221_phy1_support))
	{
		phylink_fixed_state_cb(phylink, ql_phylink_fixed_state);
	}
#endif

	SET_NETDEV_DEV(eth->netdev[id], eth->dev);
	eth->netdev[id]->watchdog_timeo = 5 * HZ;
	eth->netdev[id]->netdev_ops = &mtk_netdev_ops;
	eth->netdev[id]->base_addr = (unsigned long)eth->base;

	eth->netdev[id]->hw_features = eth->soc->hw_features;
	if (eth->hwlro)
		eth->netdev[id]->hw_features |= NETIF_F_LRO;

	eth->netdev[id]->vlan_features = eth->soc->hw_features &
		~(NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX);
	eth->netdev[id]->features |= eth->soc->hw_features;
	eth->netdev[id]->ethtool_ops = &mtk_ethtool_ops;

	eth->netdev[id]->irq = eth->irq[0];
	eth->netdev[id]->dev.of_node = np;
#ifndef CONFIG_QUECTEL_ETHERNET_PHY
	eth->netdev[id]->max_mtu = MTK_MAX_RX_LENGTH - MTK_RX_ETH_HLEN;
#endif
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	struct device_node *phy_gpios_node;
	char interrupt_pin_str[64];
	int ret;

	eth->netdev[id]->max_mtu = MTK_MAX_RX_LENGTH_2K - MTK_RX_ETH_HLEN;
	phy_gpios_node = of_get_child_by_name(np, "phy-gpios");
	if (phy_gpios_node) {
		mac->ql_sgmii_phy_data.reset_pin = of_get_named_gpio(phy_gpios_node, "reset-gpios", 0);
		if (!gpio_is_valid(mac->ql_sgmii_phy_data.reset_pin)) {
			dev_err(eth->dev, "failed to parse mac:%d phy reset gpio\n", id);
			mac->ql_sgmii_phy_data.reset_pin = -1;
		}
		mac->ql_sgmii_phy_data.interrupt_pin = of_get_named_gpio(phy_gpios_node, "interrupt-gpios", 0);
		if (!gpio_is_valid(mac->ql_sgmii_phy_data.interrupt_pin)) {
			dev_err(eth->dev, "failed to parse mac:%d phy interrupt gpio\n", id);
			mac->ql_sgmii_phy_data.interrupt_pin = -1;
		}
		mac->ql_sgmii_phy_data.phy_power_pin = of_get_named_gpio(phy_gpios_node, "phy_power-gpios", 0);
		if (!gpio_is_valid(mac->ql_sgmii_phy_data.phy_power_pin)) {
			dev_err(eth->dev, "failed to parse mac:%d phy power gpio\n", id);
			mac->ql_sgmii_phy_data.phy_power_pin = -1;
		}
		of_node_put(phy_gpios_node);
		dev_info(eth->dev, "%s.%d record mac:%d reset_pin:%d interrupt_pin:%d phy_power_pin:%d\n", \
				__func__, __LINE__, id, ql_pin_to_gpio(mac->ql_sgmii_phy_data.reset_pin), \
				ql_pin_to_gpio(mac->ql_sgmii_phy_data.interrupt_pin), \
				ql_pin_to_gpio(mac->ql_sgmii_phy_data.phy_power_pin));
	}

	//request
	snprintf(interrupt_pin_str, sizeof(interrupt_pin_str), "interrupt-gpios:%d", id);
	if(mac->ql_sgmii_phy_data.interrupt_pin > 0)
	{
		ret = devm_gpio_request(eth->dev, mac->ql_sgmii_phy_data.interrupt_pin, interrupt_pin_str);
		if (ret) {
			dev_err(eth->dev, "Failed to request mac:%d interrupt_pin:%d\n",
				id, ql_pin_to_gpio(mac->ql_sgmii_phy_data.interrupt_pin));
		}
		gpio_direction_input(mac->ql_sgmii_phy_data.interrupt_pin);
	}
	//phy-addr
	mac->ql_sgmii_phy_data.phy_addr = ql_match_phy_addr_from_mac_id(mac->id);
	//Init All suitable phy callback & data.
	if (mac->id == 0 && eth->soc->rtl8221_phy0_support) {
		init_callback_func(mac, RTL8221_PHY0);
	}
	if (mac->id == 1 && eth->soc->rtl8221_phy1_support) {
		init_callback_func(mac, RTL8221_PHY1);
	}
	if (mac->id == 0 && eth->soc->aqr113c_phy0_support) {
		init_callback_func(mac, AQR113C_PHY0);
	}
	if (mac->id == 1 && eth->soc->aqr113c_phy1_support) {
		init_callback_func(mac, AQR113C_PHY1);
	}
#endif
	return 0;

free_netdev:
	free_netdev(eth->netdev[id]);
	return err;
}

static int mtk_probe(struct platform_device *pdev)
{
	struct device_node *mac_np;
	struct mtk_eth *eth;
	int err, i;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	u32 tmp;
	s32 dma_mask_result;
	int ret;

	dev_err(&pdev->dev, "%s in\n", __func__);

	eth = devm_kzalloc(&pdev->dev, sizeof(*eth), GFP_KERNEL);
	if (!eth)
		return -ENOMEM;

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	ext_eth = eth;
	eth->mac_data[0].sgmii_phy_addr = CONFIG_SGMII0_PHY_ADDR;
	eth->mac_data[1].sgmii_phy_addr = CONFIG_SGMII1_PHY_ADDR;
#endif
	eth->soc = of_device_get_match_data(&pdev->dev);

	eth->dev = &pdev->dev;
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	eth->base = mac_phy_adapt->base;
	for(i = 0; i< MTK_MAX_DEVS; i++)
	{
		eth->mac_data[i].sgmii_phy_addr = mac_phy_adapt->mac_data[i].sgmii_phy_addr;
		eth->mac_data[i].sgmii_type = mac_phy_adapt->mac_data[i].sgmii_type;
		eth->mac_data[i].is_def_cfg = mac_phy_adapt->mac_data[i].is_def_cfg;
		eth->mac_data[i].sgmii_phy_id = mac_phy_adapt->mac_data[i].sgmii_phy_id;
		memcpy(eth->mac_data[i].compatible, mac_phy_adapt->mac_data[i].compatible, \
				sizeof(eth->mac_data[i].compatible));
	}
#else
	eth->base = devm_ioremap_resource(&pdev->dev, res);
#endif
	if (IS_ERR(eth->base)) {
		dev_err(&pdev->dev, "%s Error can't get reg!\n", __func__);
		return PTR_ERR(eth->base);
	}

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	ql_update_eth_data(pdev, &eth->soc, mac_phy_adapt, 0);
	ql_update_eth_data(pdev, &eth->soc, mac_phy_adapt, 1);
#endif

	if(eth->soc->has_sram_qdma || eth->soc->has_sram_adma) {
		eth->phy_scratch_ring = res->start + MTK_ETH_SRAM_OFFSET;
		dev_err(&pdev->dev, "SRAM phy_scratch_ring = %x res->start=%x\n",
			eth->phy_scratch_ring, res->start);
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_QDMA)) {
		eth->tx_int_mask_reg = MTK_QDMA_INT_MASK;
		eth->tx_int_status_reg = MTK_QDMA_INT_STATUS;
	} else {
		eth->tx_int_mask_reg = MTK_PDMA_INT_MASK;
		eth->tx_int_status_reg = MTK_PDMA_INT_STATUS;
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628)) {
		eth->rx_dma_l4_valid = RX_DMA_L4_VALID_PDMA;
		eth->ip_align = NET_IP_ALIGN;
	} else {
#if defined(CONFIG_MTK_ADMAV2)
		eth->rx_dma_l4_valid = RX_DMA_L4_VALID_V2;
#else
		eth->rx_dma_l4_valid = RX_DMA_L4_VALID;
#endif
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
		eth->rx_int_mask_reg = MTK_PDMA_INT_MASK0;
		eth->rx_int_status_reg = MTK_PDMA_INT_STATUS0;
	} else {
		eth->rx_int_mask_reg = MTK_PDMA_INT_MASK;
		eth->rx_int_status_reg = MTK_PDMA_INT_STATUS;
	}

#if defined(CONFIG_MTK_ADMAV2)
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_HWRSS) &&
		MTK_HAS_CAPS(eth->soc->caps, MTK_HWLRO)) {
		eth->rx_rss_base = 0;
		eth->rx_lro_base = 4;
		eth->max_rx_ring_num = MTK_MAX_RX_RING_NUM;
	} else if(MTK_HAS_CAPS(eth->soc->caps, MTK_HWRSS)) {
		eth->max_rx_ring_num = MTK_MAX_RSS_RING_NUM;
	} else {
		eth->max_rx_ring_num = MTK_MAX_LRO_RING_NUM;
	}
#endif

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
		return ret;
	}

	ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(33));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_mask: %d\n", ret);
		return ret;
	}
#endif


	spin_lock_init(&eth->page_lock);
	spin_lock_init(&eth->tx_irq_lock);
	spin_lock_init(&eth->rx_irq_lock);

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628)) {
		eth->ethsys = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							      "mediatek,ethsys");
		if (IS_ERR(eth->ethsys)) {
			dev_err(&pdev->dev, "no ethsys regmap found\n");
			return PTR_ERR(eth->ethsys);
		}
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)) {
		if (!MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK)) {
			if (!MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS2)) {
				eth->wo = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
					"mediatek,wo");
				if (IS_ERR(eth->wo)) {
					dev_err(&pdev->dev, "no wo regmap found\n");
					return PTR_ERR(eth->wo);
				}
				regmap_write(eth->wo, 0x0070, 0xb);
				regmap_write(eth->wo, 0x0074, 0xb);
			}
		}
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_INFRA)) {
		eth->infra = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							     "mediatek,infracfg");
		if (IS_ERR(eth->infra)) {
			dev_err(&pdev->dev, "no infracfg regmap found\n");
			//return PTR_ERR(eth->infra);
		}
	}

	if (eth->soc->sgmii_pm) {
		eth->dvfsrc_vcore_power = regulator_get(&pdev->dev, "dvfsrc-vcore");
		regulator_set_voltage(eth->dvfsrc_vcore_power, SGMII_VCORE_OPS, INT_MAX);
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SGMII) ||
		MTK_HAS_CAPS(eth->soc->caps, MTK_USXGMII)) {
		if (!MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK)) {
			eth->sgmii = devm_kzalloc(eth->dev, sizeof(*eth->sgmii),
						  GFP_KERNEL);
			if (!eth->sgmii)
				return -ENOMEM;

			err = mtk_sgmii_init(eth->sgmii, pdev->dev.of_node,
					     eth->soc->ana_rgc3);
			if (err){
				pr_info("%s: mtk_sgmii_init fail\n", __func__);
				return err;
			}

			if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890))
				err =  mtk_sgmii_phy_init(eth->sgmii, pdev->dev.of_node, eth);

			if (err){
				pr_info("%s: mtk_sgmii_phy_init fail\n", __func__);
				return err;
			}

			if (MTK_HAS_CAPS(eth->soc->caps, MTK_USXGMII))
					err =  mtk_usxgmii_phy_init(eth->sgmii,
				pdev->dev.of_node, eth);

			if (err) {
				pr_info("%s: mtk_sgmii_uxphy_init fail\n", __func__);
				return err;
			}

			if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS2))
				err =  mtk_sgmii_pll_init(eth->sgmii,
					pdev->dev.of_node, eth);
			if (err){
				pr_info("%s: mtk_sgmii_pll_init fail\n", __func__);
				return err;
			}
		}
	}

	if (eth->soc->required_pctl) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)){
			pinctrl_init(eth);
		} else {
			eth->pctl = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
								    "mediatek,pctl");
			if (IS_ERR(eth->pctl)) {
				dev_err(&pdev->dev, "no pctl regmap found\n");
				return PTR_ERR(eth->pctl);
			}
		}
	}
	if (eth->soc->irq_num > platform_irq_count(pdev)) {
		dev_err(&pdev->dev, "Interrupts in DTS not correct.\n");
		return -EINVAL;
	}

	for (i = 0; i < eth->soc->irq_num; i++) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SHARED_INT) && i > 0)
			eth->irq[i] = eth->irq[0];
		else{
			eth->irq[i] = platform_get_irq(pdev, i);
			dev_info(&pdev->dev, "IRQ%d is %d \n", i, eth->irq[i]);
		}
		if (eth->irq[i] < 0) {
			dev_err(&pdev->dev, "no IRQ%d resource found\n", i);
			return -ENXIO;
		}
	}

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_FPGA_CLK)) {
		for (i = 0; i < ARRAY_SIZE(eth->clks); i++) {
			eth->clks[i] = devm_clk_get(eth->dev, mtk_clks_source_name[i]);
			if (IS_ERR(eth->clks[i])) {
				if (PTR_ERR(eth->clks[i]) == -EPROBE_DEFER)
					return -EPROBE_DEFER;
				if (eth->soc->required_clks & BIT(i)) {
					dev_err(&pdev->dev, "clock %s not found\n",
						mtk_clks_source_name[i]);
					return -EINVAL;
				}
				eth->clks[i] = NULL;
			}
		}
	}

	eth->msg_enable = netif_msg_init(mtk_msg_level, MTK_DEFAULT_MSG_ENABLE);
	if(!MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6890)){
		//INIT_WORK(&eth->pending_work, mtk_pending_work);
	}
	eth->hwrss = MTK_HAS_CAPS(eth->soc->caps, MTK_HWRSS);
	eth->hwlro = MTK_HAS_CAPS(eth->soc->caps, MTK_HWLRO);
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
		///power
#if defined(CONFIG_SGMII0_PHY_POWER_PIN)
		ql_phy_power_on(eth->dev, CONFIG_SGMII0_PHY_POWER_PIN, 0, 1);
#endif
#if defined(CONFIG_SGMII1_PHY_POWER_PIN)
		ql_phy_power_on(eth->dev, CONFIG_SGMII1_PHY_POWER_PIN, 1, 1);
#endif
#if (defined(CONFIG_SGMII0_PHY_POWER_PIN) || defined(CONFIG_SGMII1_PHY_POWER_PIN))
		mdelay(150);
#endif
		///reset
#if defined(CONFIG_SGMII0_PHY_RESET_PIN)
	if (!(eth->soc->mt7531_phy0_support == false || eth->soc->en8801_phy0_support == false || \
		eth->soc->en8811_phy0_support == false || eth->soc->yt8821_phy0_support == false)) {
		pr_info("%s reset mac0\n", __func__);
		ql_phy_reset(eth->dev, CONFIG_SGMII0_PHY_RESET_PIN, 0);
	}
#endif
#if defined(CONFIG_SGMII1_PHY_RESET_PIN)
	if (!(eth->soc->mt7531_phy1_support == false || eth->soc->en8801_phy1_support == false || \
		eth->soc->en8811_phy1_support == false || eth->soc->yt8821_phy1_support == false)) {
		pr_info("%s reset mac1\n", __func__);
		ql_phy_reset(eth->dev, CONFIG_SGMII1_PHY_RESET_PIN, 1);
	}
#endif
#if (defined(CONFIG_SGMII0_PHY_RESET_PIN) || defined(CONFIG_SGMII1_PHY_RESET_PIN))
		mdelay(150);
#endif
#endif
	err = mtk_hw_init(eth);
	if (err)
		return err;


	/* No MT7628/88 support yet */
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT7628)) {
		err = mtk_mdio_init(eth);
		if (err)
			goto err_free_dev;
		}

	for_each_child_of_node(pdev->dev.of_node, mac_np) {
		if (!of_device_is_compatible(mac_np,
					     "mediatek,eth-mac"))
			continue;

		if (!of_device_is_available(mac_np))
			continue;

		err = mtk_add_mac(eth, mac_np);
		if (err) {
			of_node_put(mac_np);
			goto err_deinit_hw;
		}
	}


	//For AQR112C, if MAX speed is 1G, disable 2.5G capability
	if ((eth->mac[0]->max_speed == 1000) && MTK_PHY_SPEED_UPDATE) {
		_mtk_mdio_write_C45(eth, 8, 0x20, 0x0060, 0x7);
		_mtk_mdio_write_C45(eth, 8, 0xC400, 0x8040, 0x7);

		//restart AN
		tmp = _mtk_mdio_read_C45(eth, 8, 0x20, 0x7);
		_mtk_mdio_write_C45(eth, 8, 0x20, (tmp | BIT(9)), 0x7);
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SHARED_INT)) {
		err = devm_request_irq(eth->dev, eth->irq[0],
				       mtk_handle_irq, 0,
				       dev_name(eth->dev), eth);
	} else {
		err = devm_request_irq(eth->dev, eth->irq[1],
				       mtk_handle_irq_tx, IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
				       "eth-tx", eth);
		if (err)
			goto err_free_dev;

		if(eth->hwrss) {
			err = devm_request_irq(eth->dev, eth->irq[2], mtk_handle_irq_rrs,
						   IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
						   "eth-rx0", eth);
			err = devm_request_irq(eth->dev, eth->irq[3], mtk_handle_irq_rrs,
						   IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
						   "eth-rx1", eth);
			err = devm_request_irq(eth->dev, eth->irq[4], mtk_handle_irq_rrs,
						   IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
						   "eth-rx2", eth);
			err = devm_request_irq(eth->dev, eth->irq[5], mtk_handle_irq_rrs,
						   IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
						   "eth-rx3", eth);
		} else {

			err = devm_request_irq(eth->dev, eth->irq[2],
					       mtk_handle_irq_rx, IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
					       "eth-rx", eth);
		}
	}

	if (err)
		goto err_free_dev;

	/* 3rd Party PHY Init handling function*/
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	if (eth->soc->rtl8221_phy0_support) {
		ql_phy_init(eth, RTL8221_PHY0);
	}
	if (eth->soc->rtl8221_phy1_support) {
		ql_phy_init(eth, RTL8221_PHY1);
	}

	if (eth->soc->rtl8221_phy0_support) {
		mtk_phy_event_init(eth, RTL8221_PHY0);
	}
	if (eth->soc->rtl8221_phy1_support) {
		mtk_phy_event_init(eth, RTL8221_PHY1);
	}
#else
	if (eth->soc->aqr112c_phy0_support) {
		mtk_phy_event_init(eth, AQR112C_PHY0);
	}
#endif

	if (eth->soc->mt7531_phy1_support) {
		mtk_phy_event_init(eth, MT7531_PHY1);
	}

	for (i = (MTK_MAX_DEVS-1); i >= 0; i--) {
		if (!eth->netdev[i])
			continue;

		err = register_netdev(eth->netdev[i]);
		if (err) {
			dev_err(eth->dev, "error bringing up device\n");
			goto err_deinit_mdio;
		} else
			netif_info(eth, probe, eth->netdev[i],
				   "mac_id:%d mediatek frame engine at 0x%08lx, irq %d\n",
				   i, eth->netdev[i]->base_addr, eth->irq[0]);
	}

	/* we run 2 devices on the same DMA ring so we need a dummy device
	 * for NAPI to work
	 */
	init_dummy_netdev(&eth->dummy_dev);
	netif_napi_add(&eth->dummy_dev, &eth->tx_napi, mtk_napi_tx,
		       MTK_NAPI_WEIGHT);
	if(eth->hwrss) {
		netif_napi_add(&eth->dummy_dev, &eth->rx_rss_napi[0].rx_napi, mtk_napi_rx_ext,
						   MTK_NAPI_WEIGHT);
		netif_napi_add(&eth->dummy_dev, &eth->rx_rss_napi[1].rx_napi, mtk_napi_rx_ext,
						   MTK_NAPI_WEIGHT);
		netif_napi_add(&eth->dummy_dev, &eth->rx_rss_napi[2].rx_napi, mtk_napi_rx_ext,
						   MTK_NAPI_WEIGHT);
		netif_napi_add(&eth->dummy_dev, &eth->rx_rss_napi[3].rx_napi, mtk_napi_rx_ext,
						   MTK_NAPI_WEIGHT);
		init_rss_napi(eth);
	} else {
		netif_napi_add(&eth->dummy_dev, &eth->rx_napi, mtk_napi_rx,
			       MTK_NAPI_WEIGHT);
	}

	mtketh_debugfs_init(eth);
	debug_proc_init(eth);

	platform_set_drvdata(pdev, eth);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	if (eth->soc->rtl8221_phy0_support) {
		ql_phy_post_init(eth, RTL8221_PHY0);
	}
	if (eth->soc->rtl8221_phy1_support) {
		ql_phy_post_init(eth, RTL8221_PHY1);
	}
#endif

	dev_err(eth->dev, "%s: mtk_probe done\n", __func__);
	return 0;

err_deinit_mdio:
	mtk_mdio_cleanup(eth);
err_free_dev:
	mtk_free_dev(eth);
err_deinit_hw:
	mtk_hw_deinit(eth);

	return err;
}

static int mtk_remove(struct platform_device *pdev)
{
	struct mtk_eth *eth = platform_get_drvdata(pdev);
	struct mtk_mac *mac;
	int i;

	/* stop all devices to make sure that dma is properly shut down */
	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;
		mtk_stop(eth->netdev[i]);
		mac = netdev_priv(eth->netdev[i]);
		phylink_disconnect_phy(mac->phylink);
	}

	mtk_hw_deinit(eth);

	debug_proc_exit();
	mtketh_debugfs_exit(eth);

	netif_napi_del(&eth->tx_napi);
	if (eth->hwrss) {
		netif_napi_del(&eth->rx_rss_napi[0].rx_napi);
		netif_napi_del(&eth->rx_rss_napi[1].rx_napi);
		netif_napi_del(&eth->rx_rss_napi[2].rx_napi);
		netif_napi_del(&eth->rx_rss_napi[3].rx_napi);
	} else {
		netif_napi_del(&eth->rx_napi);
	}
	mtk_cleanup(eth);
	mtk_mdio_cleanup(eth);

	return 0;
}


/*=======MT6890 priepietary function start =====*/
u32 mtk_mdio_read_ext(struct mtk_eth *eth, int phy_addr, int phy_reg)
{
	return _mtk_mdio_read(eth, phy_addr, phy_reg);
}

u32 mtk_mdio_write_ext(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data)
{
	return _mtk_mdio_write(eth, phy_addr, phy_register, write_data);
}

int mtk_mdio_busy_wait_ext(struct mtk_eth *eth)
{
	return mtk_mdio_busy_wait(eth);
}

void mtk_rx_get_desc_ext(struct mtk_rx_dma *rxd,
				   struct mtk_rx_dma *dma_rxd)
{
	mtk_rx_get_desc(rxd, dma_rxd);
}

void mtk_handle_status_irq_ext(struct mtk_eth *eth)
{
	mtk_handle_status_irq(eth);
}
void mtk_rx_irq_disable_ext(struct mtk_eth *eth, u32 mask)
{
	mtk_rx_irq_disable(eth, mask);
}

void mtk_tx_irq_disable_ext(struct mtk_eth *eth, u32 mask)
{
	mtk_tx_irq_disable(eth, mask);
}

void mtk_clk_disable_ext(struct mtk_eth *eth)
{
	mtk_clk_disable(eth);
	/* disable clks controlled by frame engine. */
	if (eth->frame_engine)
		mtk_fe_pm_clk(eth->frame_engine, 1);
}

int mtk_clk_enable_ext(struct mtk_eth *eth)
{
	/* enable clks controlled by frame engine. */
	if (eth->frame_engine){
		pr_notice("%s mtk_fe_pm_clk",  __func__);
		mtk_fe_pm_clk(eth->frame_engine, 0);
	}
	return mtk_clk_enable(eth);

}


void mtk_tx_irq_enable_ext(struct mtk_eth *eth, u32 mask)
{
	mtk_tx_irq_enable(eth, mask);

}

void mtk_rx_irq_enable_ext(struct mtk_eth *eth, u32 mask)
{
	mtk_rx_irq_enable(eth, mask);

}

/*=======MT6890 priepietary function end =====*/

#if !defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
static const struct mtk_soc_data mt2701_data = {
	.irq_num = 3,
	.caps = MT7623_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES,
	.required_clks = MT7623_CLKS_BITMAP,
	.required_pctl = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,

};

static const struct mtk_soc_data mt7621_data = {
	.irq_num = 3,
	.caps = MT7621_CAPS,
	.hw_features = MTK_HW_FEATURES,
	.required_clks = MT7621_CLKS_BITMAP,
	.required_pctl = false,
	.has_sram_qdma = false,
	.has_sram_adma = false,
};

static const struct mtk_soc_data mt7622_data = {
	.irq_num = 3,
	.ana_rgc3 = 0x2028,
	.caps = MT7622_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES,
	.required_clks = MT7622_CLKS_BITMAP,
	.required_pctl = false,
	.has_sram_qdma = false,
	.has_sram_adma = false,
};

static const struct mtk_soc_data mt7623_data = {
	.irq_num = 3,
	.caps = MT7623_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES,
	.required_clks = MT7623_CLKS_BITMAP,
	.required_pctl = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,
};

static const struct mtk_soc_data mt7629_data = {
	.ana_rgc3 = 0x128,
	.irq_num = 3,
	.caps = MT7629_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES,
	.required_clks = MT7629_CLKS_BITMAP,
	.required_pctl = false,
	.has_sram_qdma = false,
	.has_sram_adma = false,
};

static const struct mtk_soc_data rt5350_data = {
	.caps = MT7628_CAPS,
	.irq_num = 3,
	.hw_features = MTK_HW_FEATURES_MT7628,
	.required_clks = MT7628_CLKS_BITMAP,
	.required_pctl = false,
	.has_sram_qdma = false,
	.has_sram_adma = false,
};

static const struct mtk_soc_data fpga_data = {
	.irq_num = 6,
	.required_clks = MT6890_FPGA_CLKS_BITMAP,
	.caps = BOLLINGER_FPGA_CAPS | MTK_HWLRO | MTK_HWRSS,
	.hw_features = MTK_HW_FEATURES_MT6890,
	.required_pctl = false,
	.sgmii_pm = false,
	.has_sram_qdma = false,
	.has_sram_adma = false,
	.aqr112c_phy0_support = false,
	.mt7531_phy1_support = false,
	.aqr113c_phy0_support = false,
	.aqr113c_phy1_support = false,
};

static const struct mtk_soc_data mt6890_data = {
	.ana_rgc3 = 0x128,
	.irq_num = 7,
	.caps = MT6890_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES_MT6890,
	.required_clks = MT6890_CLKS_BITMAP,
	.required_pctl = true,
	.sgmii_pm = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,
	.aqr112c_phy0_support = true,
	.mt7531_phy1_support = false,
	.aqr113c_phy0_support = false,
	.aqr113c_phy1_support = false,
};
#endif

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
static struct mtk_soc_data mt6990_data = {
#else
static const struct mtk_soc_data mt6990_data = {
#endif
	.ana_rgc3 = 0x128,
	.irq_num = 8,
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	//.caps will set in ql_update_eth_data
	.caps = MT6990_CAPS_BASE | MTK_HWLRO,
#else
	.caps = MT6990_CAPS | MTK_HWLRO,
#endif
	.hw_features = MTK_HW_FEATURES_MT6890,
	.required_clks = MT6990_CLKS_BITMAP,
	.required_pctl = true,
	.sgmii_pm = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	//default all is false, will be set in ql_update_eth_data
	.mt7531_phy0_support = false,
	.mt7531_phy1_support = false,
	.rtl8221_phy0_support = false,
	.rtl8221_phy1_support = false,
	.aqr112c_phy0_support = false,
	.aqr113c_phy0_support = false,
	.aqr113c_phy1_support = false,
	.rtl8251_phy0_support = false,
	.rtl8251_phy1_support = false,
	.en8801_phy0_support = false,
	.en8801_phy1_support = false,
	.en8811_phy0_support = false,
	.en8811_phy1_support = false,
	.yt8821_phy0_support = false,
	.yt8821_phy1_support = false,
#else
	.aqr112c_phy0_support = false,
	.mt7531_phy1_support = true,
	.aqr113c_phy0_support = false,
	.aqr113c_phy1_support = false,
#endif
};

static const struct mtk_soc_data mt6990_10g_data = {
	.ana_rgc3 = 0x128,
	.irq_num = 8,
	.caps = MT6990_10G_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES_MT6890,
	.required_clks = MT6990_CLKS_BITMAP,
	.required_pctl = true,
	.sgmii_pm = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,
	.aqr112c_phy0_support = false,
	.mt7531_phy1_support = false,
	.aqr113c_phy0_support = true,
	.aqr113c_phy1_support = true,
};

#if !defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
static const struct mtk_soc_data mt2737_data = {
	.ana_rgc3 = 0x128,
	.irq_num = 8,
	.caps = MT2737_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES_MT6890,
	.required_clks = MT6990_CLKS_BITMAP,
	.required_pctl = true,
	.sgmii_pm = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,
	.aqr112c_phy0_support = true,
	.mt7531_phy1_support = true,
	.aqr113c_phy0_support = false,
	.aqr113c_phy1_support = false,
};

static const struct mtk_soc_data mt2737_10g_data = {
	.ana_rgc3 = 0x128,
	.irq_num = 8,
	.caps = MT2737_10G_CAPS | MTK_HWLRO,
	.hw_features = MTK_HW_FEATURES_MT6890,
	.required_clks = MT6990_CLKS_BITMAP,
	.required_pctl = true,
	.sgmii_pm = true,
	.has_sram_qdma = false,
	.has_sram_adma = false,
	.aqr112c_phy0_support = false,
	.mt7531_phy1_support = false,
	.aqr113c_phy0_support = true,
	.aqr113c_phy1_support = true,
};
#endif

static const struct dev_pm_ops netsys_pm_ops = {
	.suspend = netsys_pm_suspend_poweroff,
	.resume = netsys_pm_resume_poweroff,
};

const struct of_device_id of_mtk_match[] = {
#if !defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	{ .compatible = "mediatek,mt2701-eth", .data = &mt2701_data},
	{ .compatible = "mediatek,mt7621-eth", .data = &mt7621_data},
	{ .compatible = "mediatek,mt7622-eth", .data = &mt7622_data},
	{ .compatible = "mediatek,mt7623-eth", .data = &mt7623_data},
	{ .compatible = "mediatek,mt7629-eth", .data = &mt7629_data},
	{ .compatible = "ralink,rt5350-eth", .data = &rt5350_data},
	{ .compatible = "mediatek,mt6890-eth", .data = &mt6890_data},
#endif
	{ .compatible = "mediatek,mt6990-eth", .data = &mt6990_data},
	{ .compatible = "mediatek,mt6990-10geth", .data = &mt6990_10g_data},
#if !defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	{ .compatible = "mediatek,mt2737-eth", .data = &mt2737_data},
	{ .compatible = "mediatek,mt2737-10geth", .data = &mt2737_10g_data},
	{ .compatible = "mediatek,fpga-eth", .data = &fpga_data},
#endif
	{},
};
MODULE_DEVICE_TABLE(of, of_mtk_match);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
void ql_update_eth_data(struct platform_device *pdev, struct mtk_soc_data **soc, \
			struct mac_phy_of_adapt *eth_phy_data, int mac_id)
{
	int ret = 0;

	QLOGD("dump mac_id:%d eth card support", mac_id);
	if(eth_phy_data->mac_data[mac_id].sgmii_type == QL_USXGMII_TYP) {
		switch (mac_id)
		{
			case 0:
				if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == AQR113C_1_PHY_ID || eth_phy_data->mac_data[mac_id].sgmii_phy_id == AQR113C_2_PHY_ID) {
					mt6990_data.aqr113c_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_USXGMII | MTK_MUX_GMAC1_TO_GEPHY_USXGMII;
					pr_info("have MTK_GMAC1_USXGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == RTL8251_PHY_ID) {
					mt6990_data.rtl8251_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_USXGMII | MTK_MUX_GMAC1_TO_GEPHY_USXGMII;
					pr_info("have MTK_GMAC1_USXGMII\n");
				}
				break;
			case 1:
				if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == AQR113C_1_PHY_ID || eth_phy_data->mac_data[mac_id].sgmii_phy_id == AQR113C_2_PHY_ID) {
					mt6990_data.aqr113c_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC2_USXGMII | MTK_MUX_GMAC2_TO_GEPHY_USXGMII;
					pr_info("have MTK_GMAC2_USXGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == RTL8251_PHY_ID) {
					mt6990_data.rtl8251_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC1_USXGMII | MTK_MUX_GMAC1_TO_GEPHY_USXGMII;
					pr_info("have MTK_GMAC1_USXGMII\n");
				}
				break;
			default:
				break;
		}
	}
	else if(eth_phy_data->mac_data[mac_id].sgmii_type == QL_SGMII_TYP) {
		switch (mac_id)
		{
			case 0:
				if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == RTL8221_1_PHY_ID || \
						eth_phy_data->mac_data[mac_id].sgmii_phy_id == RTL8221_2_PHY_ID) {
					mt6990_data.rtl8221_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_SGMII | MTK_MUX_GMAC1_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC1_SGMII\n");
				}else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == MT7531_PHY_ID) {
					mt6990_data.mt7531_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_SGMII | MTK_MUX_GMAC1_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC1_SGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == EN8801_PHY_ID) {
					mt6990_data.en8801_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_SGMII | MTK_MUX_GMAC1_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC1_SGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == EN8811_PHY_ID) {
					mt6990_data.en8811_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_SGMII | MTK_MUX_GMAC1_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC1_SGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == YT8821_PHY_ID) {
					mt6990_data.yt8821_phy0_support = true;
					mt6990_data.caps |= MTK_GMAC1_SGMII | MTK_MUX_GMAC1_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC1_SGMII\n");
				}
				break;
			case 1:
				if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == RTL8221_1_PHY_ID || \
						eth_phy_data->mac_data[mac_id].sgmii_phy_id == RTL8221_2_PHY_ID) {
					mt6990_data.rtl8221_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC2_SGMII | MTK_MUX_GMAC2_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC2_SGMII\n");
				}else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == MT7531_PHY_ID) {
					mt6990_data.mt7531_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC2_SGMII | MTK_MUX_GMAC2_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC2_SGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == EN8801_PHY_ID) {
					mt6990_data.en8801_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC2_SGMII | MTK_MUX_GMAC2_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC2_SGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == EN8811_PHY_ID) {
					mt6990_data.en8811_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC2_SGMII | MTK_MUX_GMAC2_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC2_SGMII\n");
				}
				else if(eth_phy_data->mac_data[mac_id].sgmii_phy_id == YT8821_PHY_ID) {
					mt6990_data.yt8821_phy1_support = true;
					mt6990_data.caps |= MTK_GMAC2_SGMII | MTK_MUX_GMAC2_TO_GEPHY_SGMII;
					pr_info("have MTK_GMAC1_SGMII\n");
				}
				break;
			default:
				break;
		}
	}
	*soc = &mt6990_data;
	if(mac_id == 1) {
		pr_info("== SGMII TYP:\n \
				rtl8221_phy0_support:%d rtl8221_phy1_support:%d\n \
				mt7531_phy0_support:%d mt7531_phy1_support:%d\n \
				en8801_phy0_support:%d en8801_phy1_support:%d\n \
				en8811_phy0_support:%d en8811_phy1_support:%d\n \
				yt8821_phy0_support:%d yt8821_phy1_support:%d\n \
				== USXGMII TYP:\n \
				aqr113c_phy0_support:%d aqr113c_phy1_support:%d\n \
				rtl8251_phy0_support:%d rtl8251_phy1_support:%d\n", \
				mt6990_data.rtl8221_phy0_support, mt6990_data.rtl8221_phy1_support, \
				mt6990_data.mt7531_phy0_support, mt6990_data.mt7531_phy1_support, \
				mt6990_data.en8801_phy0_support, mt6990_data.en8801_phy1_support, \
				mt6990_data.en8811_phy0_support, mt6990_data.en8811_phy1_support, \
				mt6990_data.yt8821_phy0_support, mt6990_data.yt8821_phy1_support, \
				mt6990_data.aqr113c_phy0_support, mt6990_data.aqr113c_phy1_support, \
				mt6990_data.rtl8251_phy0_support, mt6990_data.rtl8251_phy1_support);
		pr_info("dump caps 0x%lx \n", (*soc)->caps);
	}
Error_mac0_np:
	return;
}
#endif

static struct platform_driver mtk_driver = {
	.probe = mtk_probe,
	.remove = mtk_remove,
	.driver = {
		.name = "mtk_soc_eth",
		.pm = &netsys_pm_ops,
		.of_match_table = of_mtk_match,
	},
};

module_platform_driver(mtk_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Crispin <blogic@openwrt.org>");
MODULE_DESCRIPTION("Ethernet driver for MediaTek SoC");
