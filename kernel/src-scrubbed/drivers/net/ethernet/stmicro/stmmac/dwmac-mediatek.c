// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#include <linux/bitfield.h>
#include <linux/gpio.h>

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/of_mdio.h>
#include <linux/regmap.h>
#include <linux/stmmac.h>
#include <linux/regulator/consumer.h>
#include <linux/random.h>

#include "stmmac.h"
#include "stmmac_platform.h"

/* Peri Configuration register for mt2712 */
#define PERI_ETH_PHY_INTF_SEL	0x418
#define PHY_INTF_MII		0
#define PHY_INTF_RGMII		1
#define PHY_INTF_RMII		4
#define RMII_CLK_SRC_RXC	BIT(4)
#define RMII_CLK_SRC_INTERNAL	BIT(5)

#define PERI_ETH_DLY	0x428
#define ETH_DLY_GTXC_INV	BIT(6)
#define ETH_DLY_GTXC_ENABLE	BIT(5)
#define ETH_DLY_GTXC_STAGES	GENMASK(4, 0)
#define ETH_DLY_TXC_INV		BIT(20)
#define ETH_DLY_TXC_ENABLE	BIT(19)
#define ETH_DLY_TXC_STAGES	GENMASK(18, 14)
#define ETH_DLY_RXC_INV		BIT(13)
#define ETH_DLY_RXC_ENABLE	BIT(12)
#define ETH_DLY_RXC_STAGES	GENMASK(11, 7)

#define PERI_ETH_DLY_FINE	0x800
#define ETH_RMII_DLY_TX_INV	BIT(2)
#define ETH_FINE_DLY_GTXC	BIT(1)
#define ETH_FINE_DLY_RXC	BIT(0)

/* Peri Configuration register for mt2737 */
#define MT2737_PERI_ETH_CTRL1		0xB0C
#define MT2737_SGMII_CLK_INTERNAL		BIT(29)
#define MT2737_RMII_CLK_SRC_INTERNAL	BIT(28)
#define MT2737_RMII_CLK_SRC_RXC		BIT(27)
#define MT2737_ETH_INTF_SEL		GENMASK(26, 24)
#define MT2737_ETH_TXC_SHIFT		BIT(22)
#define MT2737_ETH_PHY_SEL		BIT(21)
#define MT2737_DLY_GTXC_INV		BIT(12)
#define MT2737_DLY_GTXC_FINE_EN		BIT(11)
#define MT2737_DLY_GTXC_FINE_STG	GENMASK(10, 6)
#define MT2737_DLY_GTXC_COARSE_EN		BIT(5)
#define MT2737_DLY_GTXC_COARSE_STG	GENMASK(4, 0)

#define MT2737_PERI_ETH_CTRL2		0xB10
#define MT2737_PPS_OUTPUT_EN		GENMASK(31, 28)
#define MT2737_PPS3_OUTPUT_EN		BIT(31)
#define MT2737_PPS2_OUTPUT_EN		BIT(30)
#define MT2737_PPS1_OUTPUT_EN		BIT(29)
#define MT2737_PPS0_OUTPUT_EN		BIT(28)
#define MT2737_DLY_RXC_INV		BIT(25)
#define MT2737_DLY_RXC_FINE_EN		BIT(24)
#define MT2737_DLY_RXC_FINE_STG		GENMASK(23, 19)
#define MT2737_DLY_RXC_COARSE_EN		BIT(18)
#define MT2737_DLY_RXC_COARSE_STG		GENMASK(17, 13)
#define MT2737_DLY_TXC_INV		BIT(12)
#define MT2737_DLY_TXC_FINE_EN		BIT(11)
#define MT2737_DLY_TXC_FINE_STG		GENMASK(10, 6)
#define MT2737_DLY_TXC_COARSE_EN		BIT(5)
#define MT2737_DLY_TXC_COARSE_STG		GENMASK(4, 0)

#define MT2737_PERI_ETH_CTRL3		0xB14
#define MT2737_DLY_RMII_RXC_INV		BIT(25)
#define MT2737_DLY_RMII_RXC_FINE_EN		BIT(24)
#define MT2737_DLY_RMII_RXC_FINE_STG		GENMASK(23, 19)
#define MT2737_DLY_RMII_RXC_COARSE_EN		BIT(18)
#define MT2737_DLY_RMII_RXC_COARSE_STG		GENMASK(17, 13)
#define MT2737_DLY_RMII_TXC_INV		BIT(12)
#define MT2737_DLY_RMII_TXC_FINE_EN		BIT(11)
#define MT2737_DLY_RMII_TXC_FINE_STG		GENMASK(10, 6)
#define MT2737_DLY_RMII_TXC_COARSE_EN		BIT(5)
#define MT2737_DLY_RMII_TXC_COARSE_STG		GENMASK(4, 0)

#define MT2737_PERI_ETH_CTRL4		0x4
#define MT2737_SNPS_MAC_REST		BIT(25)

#define MT2737_PERI_ETH_CTRL5		0x8
#define MT2737_SNPS_MAC_CLEAR		BIT(25)

#define MTK_USE_SGMII		1

/*SGMII require vcore*/
#define SGMII_VCORE_OPS		650000

struct mac_delay_struct {
	u32 tx_delay;
	u32 rx_delay;
	bool tx_inv;
	bool rx_inv;
};

struct mediatek_dwmac_plat_data {
	const struct mediatek_dwmac_variant *variant;
	struct mac_delay_struct mac_delay;
	struct clk *rmii_internal_clk;
	struct clk_bulk_data *clks;
	struct device_node *np;
	struct regmap *peri_regmap;
	struct device *dev;
	struct mtk_sgmii *sgmii;
	int phy_mode;
	bool rmii_clk_from_mac;
	bool rmii_rxc;
	struct regulator	*dvfsrc_vcore_power;
};

struct mediatek_dwmac_variant {
	int (*dwmac_set_phy_interface)(struct mediatek_dwmac_plat_data *plat);
	int (*dwmac_set_delay)(struct mediatek_dwmac_plat_data *plat);
	void (*dwmac_fix_mac_speed)(void *priv, unsigned int speed);

	/* clock ids to be requested */
	const char * const *clk_list;
	int num_clks;

	u32 dma_bit_mask;
	u32 rx_delay_max;
	u32 tx_delay_max;
#if defined(MTK_USE_SGMII)
	u32 ana_rgc3;
#endif

};

/* list of clocks required for mac */
static const char * const mt2712_dwmac_clk_l[] = {
	"axi", "apb", "mac_main", "ptp_ref"
};

static const char * const mt2737_dwmac_clk_l[] = {
	"mac_main", "ptp_ref",  "eth_axi", "eth_overclk"
#if defined(CONFIG_MTK_SGMII_SNPS)
	, "sgmii_sel", "sgmii_sbus_sel"
#endif
};

static int eth_smt_result = 2;

static struct sk_buff *get_skb(struct stmmac_priv *priv,
				  u32 payload_len,
				  u16 queue_id)
{
	struct sk_buff *skb = NULL;
	unsigned char *ppayload;
	struct ethhdr *ehdr;
	u32 size;
	unsigned char bc_addr[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	size = payload_len + sizeof(struct ethhdr);

	skb = netdev_alloc_skb(priv->dev, size);
	if (!skb)
		return NULL;

	ehdr = skb_push(skb, ETH_HLEN);
	prefetchw(skb->data);

	skb_reset_mac_header(skb);
	ether_addr_copy(ehdr->h_source, priv->dev->dev_addr);
	ether_addr_copy(ehdr->h_dest, bc_addr);

	ehdr->h_proto = htons(ETH_P_IP);

	ppayload = skb_put(skb, payload_len);

	// randomize payload
	get_random_bytes(ppayload, payload_len);

	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->dev = priv->dev;

	return skb;
}

static int dev_direct_xmit_internal(struct sk_buff *skb, u16 queue_id)
{
	struct net_device *dev = skb->dev;
	struct sk_buff *orig_skb = skb;
	struct netdev_queue *txq;
	int ret = NETDEV_TX_BUSY;
	bool again = false;

	skb = validate_xmit_skb_list(skb, dev, &again);
	if (skb != orig_skb)
		goto drop;

	skb_set_queue_mapping(skb, queue_id);
	txq = skb_get_tx_queue(dev, skb);

	local_bh_disable();

	dev_xmit_recursion_inc();
	HARD_TX_LOCK(dev, txq, smp_processor_id());
	ret = netdev_start_xmit(skb, dev, txq, false);
	HARD_TX_UNLOCK(dev, txq);
	dev_xmit_recursion_dec();

	local_bh_enable();

	if (!dev_xmit_complete(ret))
		kfree_skb(skb);

	return ret;
drop:
	atomic_long_inc(&dev->tx_dropped);
	kfree_skb_list(skb);
	return NET_XMIT_DROP;
}


static bool __loopback_test(struct stmmac_priv *priv, int num, int delay,
			    bool log, u32 frame_size)
{
	unsigned int rx_framecount_gb, rx_octetcount_gb, rx_octetcount_g;
	unsigned int tx_octetcount_gb, tx_framecount_gb;
	struct sk_buff *skb = NULL;
	int i, j;
	bool ret;

	priv->mmc.mmc_tx_octetcount_gb += readl(priv->mmcaddr + 0x14);
	priv->mmc.mmc_tx_framecount_gb += readl(priv->mmcaddr + 0x18);
	priv->mmc.mmc_rx_framecount_gb += readl(priv->mmcaddr + 0x80);
	priv->mmc.mmc_rx_octetcount_gb += readl(priv->mmcaddr + 0x84);
	priv->mmc.mmc_rx_octetcount_g += readl(priv->mmcaddr + 0x88);
	tx_octetcount_gb = priv->mmc.mmc_tx_octetcount_gb;
	tx_framecount_gb = priv->mmc.mmc_tx_framecount_gb;
	rx_framecount_gb = priv->mmc.mmc_rx_framecount_gb;
	rx_octetcount_gb = priv->mmc.mmc_rx_octetcount_gb;
	rx_octetcount_g = priv->mmc.mmc_rx_octetcount_g;
	usleep_range(delay, delay + 1000);
	for (j = 0; j < priv->plat->tx_queues_to_use; j++) {
		for (i = 0; i < num; i++) {
			skb = get_skb(priv, frame_size, j);
			if (!skb) {
				pr_err("alloc skb failed\n");
				return false;
			}
			ret = dev_direct_xmit_internal(skb, j);
			if (ret)
				pr_err("skb xmit failed q:n %d:%d\n", j, i);
		}
	}

	usleep_range(delay, delay + 1000);
	priv->mmc.mmc_tx_octetcount_gb += readl(priv->mmcaddr + 0x14);
	priv->mmc.mmc_tx_framecount_gb += readl(priv->mmcaddr + 0x18);
	priv->mmc.mmc_rx_framecount_gb += readl(priv->mmcaddr + 0x80);
	priv->mmc.mmc_rx_octetcount_gb += readl(priv->mmcaddr + 0x84);
	priv->mmc.mmc_rx_octetcount_g += readl(priv->mmcaddr + 0x88);
	tx_octetcount_gb = priv->mmc.mmc_tx_octetcount_gb - tx_octetcount_gb;
	tx_framecount_gb = priv->mmc.mmc_tx_framecount_gb - tx_framecount_gb;
	rx_framecount_gb = priv->mmc.mmc_rx_framecount_gb - rx_framecount_gb;
	rx_octetcount_gb = priv->mmc.mmc_rx_octetcount_gb - rx_octetcount_gb;
	rx_octetcount_g = priv->mmc.mmc_rx_octetcount_g - rx_octetcount_g;

	if (tx_framecount_gb == rx_framecount_gb &&
		tx_octetcount_gb == rx_octetcount_gb &&
		rx_octetcount_gb == rx_octetcount_g &&
		tx_framecount_gb == (priv->plat->tx_queues_to_use * num))
		ret = true;
	else
		ret = false;

	if (log) {
		pr_err("loop back %s:\n", ret ? "success" : "fail");
		pr_err("tx_queues:%d\t pkt_num_per_queue:%d\n",
			   priv->plat->tx_queues_to_use, num);
		pr_err("tx_framecount_gb:%u\t tx_octetcount_gb:%u\n",
			   tx_framecount_gb, tx_octetcount_gb);
		pr_err("rx_framecount_gb:%u\t rx_octetcount_gb:%u, rx_octetcount_g:%u\n",
			   rx_framecount_gb, rx_octetcount_gb, rx_octetcount_g);
	}

	return ret;
}

#ifdef CONFIG_DEBUG_FS
static int mmd_write(struct stmmac_priv *priv, int phy_addr,
		  int dev_addr, int reg, int data)
{
	priv->mii->write(priv->mii, phy_addr,
				 0xd, dev_addr);

	priv->mii->write(priv->mii, phy_addr,
				 0xe, reg);

	priv->mii->write(priv->mii, phy_addr,
				 0xd, (1 << 14) | dev_addr);

	priv->mii->write(priv->mii, phy_addr,
				 0xe, data);
	return 0;
}

static int mmd_read(struct stmmac_priv *priv, int phy_addr, int dev_addr, int reg)
{
	int origin;
	priv->mii->write(priv->mii, phy_addr,
				 0xd, dev_addr);

	priv->mii->write(priv->mii, phy_addr,
				 0xe, reg);

	priv->mii->write(priv->mii, phy_addr,
				 0xd, (1 << 14) | dev_addr);

	origin = priv->mii->read(priv->mii, phy_addr, 0xe);

	return origin;
}

static ssize_t stmmac_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(netdev);
	int len = 0;
	int buf_len = (int)PAGE_SIZE;
	struct phy_device *phy_dev = of_phy_find_device(priv->plat->phy_node);

	len += snprintf(buf + len, buf_len - len,
			"stmmac debug commands: in hexadecimal notation\n");
	len += snprintf(buf + len, buf_len - len,
			"mac read: echo er reg_start reg_range > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"mac write: echo ew reg_addr value > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"clause22 read: echo cl22r phy_reg > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"clause22 write: echo cl22w phy_reg value > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"clause45 read: echo cl45r dev_id reg_addr > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"clause45 write: echo cl45w dev_id reg_addr value > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"reg read: echo rr reg_addr > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"reg write: echo wr reg_addr value > stmmac\n");
	len += snprintf(buf + len, buf_len - len,
			"phy addr:%d\n", phy_dev->mdio.addr);
	len += snprintf(buf + len, buf_len - len,
			"eth_smt_result:%d\n", eth_smt_result);

	return len;
}

static ssize_t stmmac_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf,
			    size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(netdev);
	void __iomem *tmp_addr;
	int reg, data, devid, origin, i, reg_addr, phy_addr, dev_addr;
	struct phy_device *phy_dev = of_phy_find_device(priv->plat->phy_node);

	if (!strncmp(buf, "er", 2) &&
	    (sscanf(buf + 2, "%x %x", &reg, &data) == 2)) {
		pr_err("line:%d, readback:0x%08x\n", __LINE__, readl(priv->mmcaddr));
		for (i = 0; i < data / 0x10 + 1; i++) {
			dev_info(dev,
				 "%08x:\t%08x\t%08x\t%08x\t%08x\t\n",
				 reg + i * 16,
				 readl(priv->ioaddr + reg + i * 0x10),
				 readl(priv->ioaddr + reg + i * 0x10 + 0x4),
				 readl(priv->ioaddr + reg + i * 0x10 + 0x8),
				 readl(priv->ioaddr + reg + i * 0x10 + 0xc));
		}
	} else if (!strncmp(buf, "ew", 2) &&
		   (sscanf(buf + 2, "%x %x", &reg, &data) == 2)) {
		origin = readl(priv->ioaddr + reg);
		writel(data, priv->ioaddr + reg);
		dev_info(dev, "mac reg%#x, value:%#x -> %#x\n",
			 reg, origin, readl(priv->ioaddr + reg));
	} else if (!strncmp(buf, "smt", 3)) {
		eth_smt_result = 0;

		if (!phy_dev) {
			dev_info(dev, "null phy_dev\n");
			goto out;
		}

		if(priv->mii->read(priv->mii, phy_dev->mdio.addr, 2) == 0x1c &&
			priv->mii->read(priv->mii, phy_dev->mdio.addr, 3) == 0xc916)
				eth_smt_result = 1;
		dev_info(dev, "eth_smt_result:%d\n", eth_smt_result);
	} else if (!strncmp(buf, "cl22r", 5)) {
		if (sscanf(buf + 5, "%x %x", &phy_addr, &reg) == 2) {
			dev_info(dev, "cl22 phyaddr:%#x, reg:%#x, value:%#x\n",
				 phy_addr,
				 reg,
				 mdiobus_read(priv->mii, phy_addr, reg));
		} else if (sscanf(buf + 5, "%x", &reg) == 1) {
			if (!phy_dev) {
				dev_info(dev, "null phy_dev\n");
				goto out;
			}

			dev_info(dev, "cl22 phyaddr:%#x, reg:%#x, value:%#x\n",
				 phy_dev->mdio.addr,
				 reg,
				 mdiobus_read(priv->mii,
				 	      phy_dev->mdio.addr,
				 	      reg));
		} else {
			dev_info(dev, "format error\n");
		}
	} else if (!strncmp(buf, "cl22w", 5)) {
		if (sscanf(buf + 5, "%x %x %x", &phy_addr, &reg, &data) == 3) {
			origin = mdiobus_read(priv->mii, phy_addr, reg);
			mdiobus_write(priv->mii, phy_addr, reg, data);
			dev_info(dev, "cl22 phyaddr:%#x, reg:%#x, %#x -> %#x\n",
				 phy_addr,
				 reg,
				 origin,
				 mdiobus_read(priv->mii, phy_addr, reg));
		} else if (sscanf(buf + 5, "%x %x", &reg, &data) == 2) {
			if (!phy_dev) {
				dev_info(dev, "null phy_dev\n");
				goto out;
			}

			origin = mdiobus_read(priv->mii, phy_dev->mdio.addr, reg);
			mdiobus_write(priv->mii, phy_dev->mdio.addr, reg, data);
			dev_info(dev, "cl22 phyaddr:%#x, reg:%#x, %#x -> %#x\n",
				 phy_dev->mdio.addr,
				 reg,
				 origin,
				 mdiobus_read(priv->mii,
				 	      phy_dev->mdio.addr,
				 	      reg));
		} else {
			dev_info(dev, "format error\n");
		}
	} else if (!strncmp(buf, "cl45r", 5)) {
		if (sscanf(buf + 5, "%x %x %x", &phy_addr, &devid, &reg) == 3) {
			reg_addr = MII_ADDR_C45 |
				   ((devid & 0x1f) << 16) |
				   (reg & 0xffff);
			dev_info(dev, "cl45 phyaddr:%#x, reg:%#x-%#x, %#x\n",
				 phy_addr, devid, reg,
				 mdiobus_read(priv->mii, phy_addr, reg_addr));
		} else if (sscanf(buf + 5, "%x %x", &devid, &reg) == 2) {
			if (!phy_dev) {
				dev_info(dev, "null phy_dev\n");
				goto out;
			}

			reg_addr = MII_ADDR_C45 |
				   ((devid & 0x1f) << 16) |
				   (reg & 0xffff);
			dev_info(dev, "cl45 phyaddr:%#x, reg:%#x-%#x, %#x\n",
				 phy_dev->mdio.addr,
				 devid,
				 reg,
				 mdiobus_read(priv->mii,
				 	      phy_dev->mdio.addr,
				 	      reg_addr));
		} else {
			dev_info(dev, "format error\n");
		}
	} else if (!strncmp(buf, "cl45w", 5)) {
		if (sscanf(buf + 5, "%x %x %x %x", &phy_addr, &devid,
			   &reg, &data) == 4) {
			reg_addr = MII_ADDR_C45 |
				   ((devid & 0x1f) << 16) |
				   (reg & 0xffff);
			origin = mdiobus_read(priv->mii,
					     phy_addr,
					      reg_addr);
			mdiobus_write(priv->mii,
				      phy_addr,
				      reg_addr,
				      data);
			dev_info(dev, "cl45 phyaddr:%#x, reg:%#x-%#x, %#x -> %#x\n",
				 phy_addr,
				 devid,
				 reg,
				 origin,
				 mdiobus_read(priv->mii,
				 	      phy_addr,
					      reg_addr));
		} else if (sscanf(buf + 5, "%x %x %x", &devid,
				  &reg, &data) == 3) {
			if (!phy_dev) {
				dev_info(dev, "null phy_dev\n");
				goto out;
			}

			reg_addr = MII_ADDR_C45 |
				   ((devid & 0x1f) << 16) |
				   (reg & 0xffff);
			origin = mdiobus_read(priv->mii,
					      phy_dev->mdio.addr,
					      reg_addr);
			mdiobus_write(priv->mii, phy_dev->mdio.addr,
					 reg_addr, data);
			dev_info(dev, "cl45 phyaddr:%#x, reg:%#x-%#x, %#x -> %#x\n",
				 phy_dev->mdio.addr,
				 devid,
				 reg,
				 origin,
				 mdiobus_read(priv->mii,
					      phy_dev->mdio.addr,
					      reg_addr));
		} else {
			dev_info(dev, "format error\n");
		}
	}  else if (!strncmp(buf, "mmdr", 4)) {
		if (sscanf(buf + 4, "%x %x %x", &phy_addr, &dev_addr, &reg) == 3){
			mmd_read(priv, phy_addr, dev_addr, reg);
			dev_info(dev, "mmd phyaddr:%#x, reg:%#x-%#x, %#x\n",
				 phy_addr,
				 dev_addr,
				 reg,
				 mmd_read(priv,
				 	  phy_addr,
				 	  dev_addr,
				 	  reg));
		}else if (sscanf(buf + 4, "%x %x", &dev_addr, &reg) == 2){
			if (!phy_dev) {
				dev_info(dev, "null phy_dev\n");
				goto out;
			}

			mmd_read(priv, phy_dev->mdio.addr, dev_addr, reg);
			dev_info(dev, "mmd phyaddr:%#x, reg:%#x-%#x, %#x\n",
				 phy_dev->mdio.addr,
				 dev_addr,
				 reg,
				 mmd_read(priv,
				 	  phy_dev->mdio.addr,
				 	  dev_addr,
				 	  reg));
		} else {
			dev_info(dev, "format error\n");
		}
	} else if (!strncmp(buf, "mmdw", 4) ) {
		if (sscanf(buf + 4, "%x %x %x %x", &phy_addr,
			   &dev_addr, &reg, &data) == 4){
			origin = mmd_read(priv, phy_addr, dev_addr, reg);
			mmd_write(priv, phy_addr, dev_addr, reg, data);
			dev_info(dev, "mmd phyaddr:%#x, reg:%#x-%#x, %#x -> %#x\n",
				 phy_addr,
				 dev_addr,
				 reg,
				 origin,
				 mmd_read(priv, phy_addr, dev_addr, reg));
		} else if ((sscanf(buf + 4, "%x %x %x", &dev_addr,
				   &reg, &data) == 3)) {
			if (!phy_dev) {
				dev_info(dev, "null phy_dev\n");
				goto out;
			}

			origin = mmd_read(priv,
					  phy_dev->mdio.addr,
					  dev_addr,
					  reg);
			mmd_write(priv,
				  phy_dev->mdio.addr,
				  dev_addr,
				  reg,
				  data);
			dev_info(dev, "mmd phyaddr:%#x, reg:%#x-%#x, %#x -> %#x\n",
				 phy_dev->mdio.addr,
				 dev_addr,
				 reg,
				 origin,
				 mmd_read(priv,
				 	  phy_dev->mdio.addr,
				 	  dev_addr,
				 	  reg));
		} else {
			dev_info(dev, "format error\n");
		}
	} else if (!strncmp(buf, "rr", 2) &&
		   (sscanf(buf + 2, "%x", &reg) == 1)) {
		tmp_addr = ioremap_nocache(reg, 32);
		data = readl(tmp_addr);
		dev_info(dev, "rr reg%#x, value:%#x\n", reg, data);
	} else if (!strncmp(buf, "wr", 2) &&
		   (sscanf(buf + 2, "%x %x", &reg, &data) == 2)) {
		tmp_addr = ioremap_nocache(reg, 32);
		origin = readl(tmp_addr);
		writel(data, tmp_addr);
		dev_info(dev, "reg%#x, value:%#x -> %#x\n",
			 reg, origin, readl(tmp_addr));
	} else if (!strncmp(buf, "dump_mac", 8)) {
		for (i = 0; i < 0x1300 / 0x10 + 1; i++) {
			pr_info("%08x:\t%08x\t%08x\t%08x\t%08x\t\n",
				reg + i * 16,
				readl(priv->ioaddr + i * 0x10),
				readl(priv->ioaddr + i * 0x10 + 0x4),
				readl(priv->ioaddr + i * 0x10 + 0x8),
				readl(priv->ioaddr + i * 0x10 + 0xc));
		}
	} else if (!strncmp(buf, "tx", 2)) {
		if (sscanf(buf + 2, "%d %d %d", &reg, &data, &reg_addr) == 3) {
			__loopback_test(priv, reg, reg_addr, true, data);
		} else if (sscanf(buf + 2, "%d %d", &reg, &data) == 2) {
			__loopback_test(priv, reg, 10000, true, data);
		} else if (sscanf(buf + 2, "%d", &reg) == 1) {
			__loopback_test(priv, reg, 10000, true, 1500);
		}
	} else if (!strncmp(buf, "carrier_on", 10)) {
		netif_carrier_on(priv->dev);
	} else if (!strncmp(buf, "carrier_off", 11)) {
		netif_carrier_off(priv->dev);
	} else {
		dev_info(dev, "Error: command not support\n");
	}
out:
	return count;
}

static DEVICE_ATTR_RW(stmmac);

static int eth_create_attr(struct device *dev)
{
	int err = 0;

	if (!dev)
		return -EINVAL;

	err = device_create_file(dev, &dev_attr_stmmac);
	if (err)
		pr_err("create debug file fail:%s\n", __func__);

	return err;
}

static void eth_remove_attr(struct device *dev)
{
	device_remove_file(dev, &dev_attr_stmmac);
}
#endif

static int mt2712_set_interface(struct mediatek_dwmac_plat_data *plat)
{
	int rmii_clk_from_mac = plat->rmii_clk_from_mac ? RMII_CLK_SRC_INTERNAL : 0;
	int rmii_rxc = plat->rmii_rxc ? RMII_CLK_SRC_RXC : 0;
	u32 intf_val = 0;

	/* select phy interface in top control domain */
	switch (plat->phy_mode) {
	case PHY_INTERFACE_MODE_MII:
		intf_val |= PHY_INTF_MII;
		break;
	case PHY_INTERFACE_MODE_RMII:
		intf_val |= (PHY_INTF_RMII | rmii_rxc | rmii_clk_from_mac);
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
		intf_val |= PHY_INTF_RGMII;
		break;
	default:
		dev_err(plat->dev, "phy interface not supported\n");
		return -EINVAL;
	}

	regmap_write(plat->peri_regmap, PERI_ETH_PHY_INTF_SEL, intf_val);

	return 0;
}

static void mt2712_delay_ps2stage(struct mediatek_dwmac_plat_data *plat)
{
	struct mac_delay_struct *mac_delay = &plat->mac_delay;

	switch (plat->phy_mode) {
	case PHY_INTERFACE_MODE_MII:
	case PHY_INTERFACE_MODE_RMII:
		/* 550ps per stage for MII/RMII */
		mac_delay->tx_delay /= 550;
		mac_delay->rx_delay /= 550;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
		/* 170ps per stage for RGMII */
		mac_delay->tx_delay /= 170;
		mac_delay->rx_delay /= 170;
		break;
	default:
		dev_err(plat->dev, "phy interface not supported\n");
		break;
	}
}

static int mt2712_set_delay(struct mediatek_dwmac_plat_data *plat)
{
	struct mac_delay_struct *mac_delay = &plat->mac_delay;
	u32 delay_val = 0, fine_val = 0;

	mt2712_delay_ps2stage(plat);

	switch (plat->phy_mode) {
	case PHY_INTERFACE_MODE_MII:
		delay_val |= FIELD_PREP(ETH_DLY_TXC_ENABLE, !!mac_delay->tx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_TXC_STAGES, mac_delay->tx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_TXC_INV, mac_delay->tx_inv);

		delay_val |= FIELD_PREP(ETH_DLY_RXC_ENABLE, !!mac_delay->rx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_RXC_STAGES, mac_delay->rx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_RXC_INV, mac_delay->rx_inv);
		break;
	case PHY_INTERFACE_MODE_RMII:
		if (plat->rmii_clk_from_mac) {
			/* case 1: mac provides the rmii reference clock,
			 * and the clock output to TXC pin.
			 * The egress timing can be adjusted by GTXC delay macro circuit.
			 * The ingress timing can be adjusted by TXC delay macro circuit.
			 */
			delay_val |= FIELD_PREP(ETH_DLY_TXC_ENABLE, !!mac_delay->rx_delay);
			delay_val |= FIELD_PREP(ETH_DLY_TXC_STAGES, mac_delay->rx_delay);
			delay_val |= FIELD_PREP(ETH_DLY_TXC_INV, mac_delay->rx_inv);

			delay_val |= FIELD_PREP(ETH_DLY_GTXC_ENABLE, !!mac_delay->tx_delay);
			delay_val |= FIELD_PREP(ETH_DLY_GTXC_STAGES, mac_delay->tx_delay);
			delay_val |= FIELD_PREP(ETH_DLY_GTXC_INV, mac_delay->tx_inv);
		} else {
			/* case 2: the rmii reference clock is from external phy,
			 * and the property "rmii_rxc" indicates which pin(TXC/RXC)
			 * the reference clk is connected to. The reference clock is a
			 * received signal, so rx_delay/rx_inv are used to indicate
			 * the reference clock timing adjustment
			 */
			if (plat->rmii_rxc) {
				/* the rmii reference clock from outside is connected
				 * to RXC pin, the reference clock will be adjusted
				 * by RXC delay macro circuit.
				 */
				delay_val |= FIELD_PREP(ETH_DLY_RXC_ENABLE, !!mac_delay->rx_delay);
				delay_val |= FIELD_PREP(ETH_DLY_RXC_STAGES, mac_delay->rx_delay);
				delay_val |= FIELD_PREP(ETH_DLY_RXC_INV, mac_delay->rx_inv);
			} else {
				/* the rmii reference clock from outside is connected
				 * to TXC pin, the reference clock will be adjusted
				 * by TXC delay macro circuit.
				 */
				delay_val |= FIELD_PREP(ETH_DLY_TXC_ENABLE, !!mac_delay->rx_delay);
				delay_val |= FIELD_PREP(ETH_DLY_TXC_STAGES, mac_delay->rx_delay);
				delay_val |= FIELD_PREP(ETH_DLY_TXC_INV, mac_delay->rx_inv);
			}
			/* tx_inv will inverse the tx clock inside mac relateive to
			 * reference clock from external phy,
			 * and this bit is located in the same register with fine-tune
			 */
			if (mac_delay->tx_inv)
				fine_val = ETH_RMII_DLY_TX_INV;
		}
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
		fine_val = ETH_FINE_DLY_GTXC | ETH_FINE_DLY_RXC;

		delay_val |= FIELD_PREP(ETH_DLY_GTXC_ENABLE, !!mac_delay->tx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_GTXC_STAGES, mac_delay->tx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_GTXC_INV, mac_delay->tx_inv);

		delay_val |= FIELD_PREP(ETH_DLY_RXC_ENABLE, !!mac_delay->rx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_RXC_STAGES, mac_delay->rx_delay);
		delay_val |= FIELD_PREP(ETH_DLY_RXC_INV, mac_delay->rx_inv);
		break;
	default:
		dev_err(plat->dev, "phy interface not supported\n");
		return -EINVAL;
	}
	regmap_write(plat->peri_regmap, PERI_ETH_DLY, delay_val);
	regmap_write(plat->peri_regmap, PERI_ETH_DLY_FINE, fine_val);

	return 0;
}

static const struct mediatek_dwmac_variant mt2712_gmac_variant = {
		.dwmac_set_phy_interface = mt2712_set_interface,
		.dwmac_set_delay = mt2712_set_delay,
		.clk_list = mt2712_dwmac_clk_l,
		.num_clks = ARRAY_SIZE(mt2712_dwmac_clk_l),
		.dma_bit_mask = 33,
		.rx_delay_max = 17600,
		.tx_delay_max = 17600,
};

static int mt2737_set_interface(struct mediatek_dwmac_plat_data *plat)
{
	int rmii_clk_from_mac = plat->rmii_clk_from_mac ? MT2737_RMII_CLK_SRC_INTERNAL : 0;
	int rmii_rxc = plat->rmii_rxc ? MT2737_RMII_CLK_SRC_RXC : 0;
	const struct mediatek_dwmac_variant *variant = plat->variant;
	u32 intf_val = 0;
	u32 err;

	/* select phy interface in top control domain */
	switch (plat->phy_mode) {
	case PHY_INTERFACE_MODE_MII:
		/*MII external clock*/
		intf_val |= FIELD_PREP(MT2737_ETH_INTF_SEL, PHY_INTF_MII);
		intf_val |= MT2737_ETH_PHY_SEL;
		break;
	case PHY_INTERFACE_MODE_RMII:
		intf_val |= (rmii_rxc | rmii_clk_from_mac);
		intf_val |= FIELD_PREP(MT2737_ETH_INTF_SEL, PHY_INTF_RMII);
		intf_val |= MT2737_ETH_PHY_SEL;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
		intf_val |= FIELD_PREP(MT2737_ETH_INTF_SEL, PHY_INTF_RGMII);
		intf_val |= MT2737_ETH_PHY_SEL;
		break;
	case PHY_INTERFACE_MODE_SGMII:
		intf_val |= MT2737_SGMII_CLK_INTERNAL;

		plat->dvfsrc_vcore_power = regulator_get(plat->dev, "dvfsrc-vcore");
		if(!plat->dvfsrc_vcore_power) {
			dev_err(plat->dev, "SGMII vcore get failed\n");
			return -ENOMEM;
		}
		regulator_set_voltage(plat->dvfsrc_vcore_power, SGMII_VCORE_OPS, INT_MAX);
	
		plat->sgmii = devm_kzalloc(plat->dev, sizeof(*plat->sgmii),
								GFP_KERNEL);
		if (!plat->sgmii)
			return -ENOMEM;
	
		regmap_write(plat->peri_regmap, MT2737_PERI_ETH_CTRL4, MT2737_SNPS_MAC_REST);
		mdelay(1);
		regmap_write(plat->peri_regmap, MT2737_PERI_ETH_CTRL5, MT2737_SNPS_MAC_CLEAR);
	
		err = mediatek_sgmii_init(plat->sgmii, plat->np, plat->dev, variant->ana_rgc3);
		break;
	default:
		dev_err(plat->dev, "phy interface not supported\n");
		return -EINVAL;
	}

	regmap_write(plat->peri_regmap, MT2737_PERI_ETH_CTRL1, intf_val);

	return 0;
}

static void mt2737_delay_ps2stage(struct mediatek_dwmac_plat_data *plat)
{
	struct mac_delay_struct *mac_delay = &plat->mac_delay;

	/* 250ps per stage */
	mac_delay->tx_delay /= 250;
	mac_delay->rx_delay /= 250;
}

static void mt2737_delay_stage2ps(struct mediatek_dwmac_plat_data *plat)
{
	struct mac_delay_struct *mac_delay = &plat->mac_delay;

	/* 250ps per stage */
	mac_delay->tx_delay *= 250;
	mac_delay->rx_delay *= 250;
}

static int mt2737_set_delay(struct mediatek_dwmac_plat_data *plat)
{
	struct mac_delay_struct *mac_delay = &plat->mac_delay;
	u32 delay_val = 0, rmii_delay_val = 0, gtxc_delay_val = 0;

	mt2737_delay_ps2stage(plat);

	switch (plat->phy_mode) {
	case PHY_INTERFACE_MODE_MII:
		delay_val &= ~(MT2737_DLY_TXC_COARSE_EN | MT2737_DLY_TXC_COARSE_STG |
					 MT2737_DLY_TXC_INV);
		delay_val |= FIELD_PREP(MT2737_DLY_TXC_COARSE_EN, !!mac_delay->tx_delay);
		delay_val |= FIELD_PREP(MT2737_DLY_TXC_COARSE_STG, mac_delay->tx_delay);
		delay_val |= FIELD_PREP(MT2737_DLY_TXC_INV, mac_delay->tx_inv);

		delay_val &= ~(MT2737_DLY_RXC_COARSE_EN | MT2737_DLY_RXC_COARSE_STG |
					 MT2737_DLY_RXC_INV);
		delay_val |= FIELD_PREP(MT2737_DLY_RXC_COARSE_EN, !!mac_delay->rx_delay);
		delay_val |= FIELD_PREP(MT2737_DLY_RXC_COARSE_STG, mac_delay->rx_delay);
		delay_val |= FIELD_PREP(MT2737_DLY_RXC_INV, mac_delay->rx_inv);
		break;
	case PHY_INTERFACE_MODE_RMII:
		if (plat->rmii_clk_from_mac) {
			/* case 1: mac provides the rmii reference clock,
			 * and the clock output to TXC pin.
			 * The egress timing can be adjusted by RMII_TXC delay macro circuit.
			 * The ingress timing can be adjusted by RMII_RXC delay macro circuit.
			 */
			rmii_delay_val |= FIELD_PREP(MT2737_DLY_RMII_TXC_COARSE_EN,
							 !!mac_delay->tx_delay);
			rmii_delay_val |= FIELD_PREP(MT2737_DLY_RMII_TXC_COARSE_STG,
							 mac_delay->tx_delay);
			rmii_delay_val |= FIELD_PREP(MT2737_DLY_RMII_TXC_INV,
							 mac_delay->tx_inv);

			rmii_delay_val |= FIELD_PREP(MT2737_DLY_RMII_RXC_COARSE_EN,
							 !!mac_delay->rx_delay);
			rmii_delay_val |= FIELD_PREP(MT2737_DLY_RMII_RXC_COARSE_STG,
							 mac_delay->rx_delay);
			rmii_delay_val |= FIELD_PREP(MT2737_DLY_RMII_RXC_INV,
							 mac_delay->rx_inv);
		} else {
			/* case 2: the rmii reference clock is from external phy,
			 * and the property "rmii_rxc" indicates which pin(TXC/RXC)
			 * the reference clk is connected to. The reference clock is a
			 * received signal, so rx_delay/rx_inv are used to indicate
			 * the reference clock timing adjustment
			 */
			if (plat->rmii_rxc) {
				/* the rmii reference clock from outside is connected
				 * to RXC pin, the reference clock will be adjusted
				 * by RXC delay macro circuit.
				 */
				delay_val |= FIELD_PREP(MT2737_DLY_RXC_COARSE_EN,
							!!mac_delay->rx_delay);
				delay_val |= FIELD_PREP(MT2737_DLY_RXC_COARSE_STG,
							mac_delay->rx_delay);
				delay_val |= FIELD_PREP(MT2737_DLY_RXC_INV,
							mac_delay->rx_inv);
			} else {
				/* the rmii reference clock from outside is connected
				 * to TXC pin, the reference clock will be adjusted
				 * by TXC delay macro circuit.
				 */
				delay_val |= FIELD_PREP(MT2737_DLY_TXC_COARSE_EN,
							!!mac_delay->rx_delay);
				delay_val |= FIELD_PREP(MT2737_DLY_TXC_COARSE_STG,
							mac_delay->rx_delay);
				delay_val |= FIELD_PREP(MT2737_DLY_TXC_INV,
							mac_delay->rx_inv);
			}
		}
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
		gtxc_delay_val |= FIELD_PREP(MT2737_DLY_GTXC_COARSE_EN, !!mac_delay->tx_delay);
		gtxc_delay_val |= FIELD_PREP(MT2737_DLY_GTXC_COARSE_STG, mac_delay->tx_delay);
		gtxc_delay_val |= FIELD_PREP(MT2737_DLY_GTXC_INV, mac_delay->tx_inv);

		delay_val |= FIELD_PREP(MT2737_DLY_RXC_COARSE_EN, !!mac_delay->rx_delay);
		delay_val |= FIELD_PREP(MT2737_DLY_RXC_COARSE_STG, mac_delay->rx_delay);
		delay_val |= FIELD_PREP(MT2737_DLY_RXC_INV, mac_delay->rx_inv);
		break;
	case PHY_INTERFACE_MODE_SGMII:
		break;
	default:
		dev_err(plat->dev, "phy interface not supported by SNPS MAC\n");
		return -EINVAL;
	}

	regmap_update_bits(plat->peri_regmap,
			   MT2737_PERI_ETH_CTRL1,
			   MT2737_ETH_TXC_SHIFT |
			   MT2737_DLY_GTXC_INV |
			   MT2737_DLY_GTXC_COARSE_EN |
			   MT2737_DLY_GTXC_COARSE_STG,
			   gtxc_delay_val);
	regmap_write(plat->peri_regmap, MT2737_PERI_ETH_CTRL2, delay_val);
	regmap_write(plat->peri_regmap, MT2737_PERI_ETH_CTRL3, rmii_delay_val);

	mt2737_delay_stage2ps(plat);

	return 0;
}

static void mt2737_fix_mac_speed(void *priv, unsigned int speed)
{
	struct mediatek_dwmac_plat_data *priv_plat = priv;

	if ((phy_interface_mode_is_rgmii(priv_plat->phy_mode))) {
		/* prefer 2ns fixed delay which is controlled by TXC_PHASE_CTRL,
		 * when link speed is 1Gbps with RGMII interface,
		 * Fall back to delay macro circuit for 10/100Mbps link speed.
		 */
		if (speed == SPEED_1000)
			regmap_update_bits(priv_plat->peri_regmap,
					   MT2737_PERI_ETH_CTRL1,
					   MT2737_ETH_TXC_SHIFT |
					   MT2737_DLY_GTXC_COARSE_EN |
					   MT2737_DLY_GTXC_INV |
					   MT2737_DLY_GTXC_COARSE_STG,
					   MT2737_ETH_TXC_SHIFT);
		else
			mt2737_set_delay(priv_plat);
	}
}

static const struct mediatek_dwmac_variant mt2737_gmac_variant = {
	.dwmac_set_phy_interface = mt2737_set_interface,
	.dwmac_set_delay = mt2737_set_delay,
	.dwmac_fix_mac_speed = mt2737_fix_mac_speed,
	.clk_list = mt2737_dwmac_clk_l,
	.num_clks = ARRAY_SIZE(mt2737_dwmac_clk_l),
	.dma_bit_mask = 32,
	.rx_delay_max = 8000,
	.tx_delay_max = 8000,
#if defined(MTK_USE_SGMII)
	.ana_rgc3 = 0x128,
#endif
};

static int mediatek_dwmac_config_dt(struct mediatek_dwmac_plat_data *plat)
{
	struct mac_delay_struct *mac_delay = &plat->mac_delay;
	u32 tx_delay_ps, rx_delay_ps;

	plat->peri_regmap = syscon_regmap_lookup_by_phandle(plat->np, "mediatek,pericfg");
	if (IS_ERR(plat->peri_regmap)) {
		dev_err(plat->dev, "Failed to get pericfg syscon\n");
		return PTR_ERR(plat->peri_regmap);
	}

	plat->phy_mode = of_get_phy_mode(plat->np);
	if (plat->phy_mode < 0) {
		dev_err(plat->dev, "not find phy-mode\n");
		return -EINVAL;
	}

	if (!of_property_read_u32(plat->np, "mediatek,tx-delay-ps", &tx_delay_ps)) {
		if (tx_delay_ps < plat->variant->tx_delay_max) {
			mac_delay->tx_delay = tx_delay_ps;
		} else {
			dev_err(plat->dev, "Invalid TX clock delay: %dps\n", tx_delay_ps);
			return -EINVAL;
		}
	}

	if (!of_property_read_u32(plat->np, "mediatek,rx-delay-ps", &rx_delay_ps)) {
		if (rx_delay_ps < plat->variant->rx_delay_max) {
			mac_delay->rx_delay = rx_delay_ps;
		} else {
			dev_err(plat->dev, "Invalid RX clock delay: %dps\n", rx_delay_ps);
			return -EINVAL;
		}
	}

	mac_delay->tx_inv = of_property_read_bool(plat->np, "mediatek,txc-inverse");
	mac_delay->rx_inv = of_property_read_bool(plat->np, "mediatek,rxc-inverse");
	plat->rmii_rxc = of_property_read_bool(plat->np, "mediatek,rmii-rxc");
	plat->rmii_clk_from_mac = of_property_read_bool(plat->np, "mediatek,rmii-clk-from-mac");

	return 0;
}

static int mediatek_dwmac_clk_init(struct mediatek_dwmac_plat_data *plat)
{
	const struct mediatek_dwmac_variant *variant = plat->variant;
	int i, ret = 0;

	plat->clks = devm_kcalloc(plat->dev, variant->num_clks, sizeof(*plat->clks), GFP_KERNEL);
	if (!plat->clks)
		return -ENOMEM;

	for (i = 0; i < variant->num_clks; i++)
		plat->clks[i].id = variant->clk_list[i];

	ret =  devm_clk_bulk_get(plat->dev, variant->num_clks, plat->clks);
	if (ret)
		return ret;

	/* The clock labeled as "rmii_internal" is needed only in RMII(when
	 * MAC provides the reference clock), and useless for RGMII/MII or
	 * RMII(when PHY provides the reference clock).
	 * So, "rmii_internal" clock is got and configured only when
	 * reference clock of RMII is from MAC.
	 */
	if (plat->rmii_clk_from_mac) {
		plat->rmii_internal_clk = devm_clk_get(plat->dev, "rmii_internal");
		if (IS_ERR(plat->rmii_internal_clk))
			ret = PTR_ERR(plat->rmii_internal_clk);
	} else {
		plat->rmii_internal_clk = NULL;
	}

	return ret;
}

static int mediatek_dwmac_init(struct platform_device *pdev, void *priv)
{
	struct mediatek_dwmac_plat_data *plat = priv;
	const struct mediatek_dwmac_variant *variant = plat->variant;
	int ret;

	ret = dma_set_mask_and_coherent(plat->dev, DMA_BIT_MASK(variant->dma_bit_mask));
	if (ret) {
		dev_err(plat->dev, "No suitable DMA available, err = %d\n", ret);
		return ret;
	}

	ret = variant->dwmac_set_phy_interface(plat);
	if (ret) {
		dev_err(plat->dev, "failed to set phy interface, err = %d\n", ret);
		return ret;
	}

	ret = variant->dwmac_set_delay(plat);
	if (ret) {
		dev_err(plat->dev, "failed to set delay value, err = %d\n", ret);
		return ret;
	}

	ret = clk_bulk_prepare_enable(variant->num_clks, plat->clks);
	if (ret) {
		dev_err(plat->dev, "failed to enable clks, err = %d\n", ret);
		return ret;
	}

	if (plat->phy_mode == PHY_INTERFACE_MODE_SGMII) {
		ret = mediatek_sgmii_path_setup(plat->sgmii, 0);
	}

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	return 0;
}

static void mediatek_dwmac_exit(struct platform_device *pdev, void *priv)
{
	struct mediatek_dwmac_plat_data *plat = priv;
	const struct mediatek_dwmac_variant *variant = plat->variant;

	clk_bulk_disable_unprepare(variant->num_clks, plat->clks);

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
}

static int mediatek_dwmac_probe(struct platform_device *pdev)
{
	struct mediatek_dwmac_plat_data *priv_plat;
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	int ret, i;

	priv_plat = devm_kzalloc(&pdev->dev, sizeof(*priv_plat), GFP_KERNEL);
	if (!priv_plat)
		return -ENOMEM;

	priv_plat->variant = of_device_get_match_data(&pdev->dev);
	if (!priv_plat->variant) {
		dev_err(&pdev->dev, "Missing dwmac-mediatek variant\n");
		return -EINVAL;
	}

	priv_plat->dev = &pdev->dev;
	priv_plat->np = pdev->dev.of_node;

	ret = mediatek_dwmac_config_dt(priv_plat);
	if (ret)
		return ret;

	ret = mediatek_dwmac_clk_init(priv_plat);
	if (ret)
		return ret;

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
	if (IS_ERR(plat_dat))
		return PTR_ERR(plat_dat);

	plat_dat->interface = priv_plat->phy_mode;
	plat_dat->has_gmac4 = 1;
	plat_dat->has_gmac = 0;
	plat_dat->pmt = 0;
	plat_dat->riwt_off = 1;
	plat_dat->tso_en = 1;
	plat_dat->maxmtu = ETH_DATA_LEN;
	plat_dat->bsp_priv = priv_plat;
	plat_dat->init = mediatek_dwmac_init;
	plat_dat->exit = mediatek_dwmac_exit;
	if (priv_plat->variant->dwmac_fix_mac_speed)
		plat_dat->fix_mac_speed = priv_plat->variant->dwmac_fix_mac_speed;
	mediatek_dwmac_init(pdev, priv_plat);


	for (i = 0; i < plat_dat->tx_queues_to_use; i++) {
		/* Default TX Q0 to use TSO and rest TXQ for TBS */
		if (i > 0)
			plat_dat->tx_queues_cfg[i].tbs_en = 1;
	}

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret) {
		stmmac_remove_config_dt(pdev, plat_dat);
		return ret;
	}

#ifdef CONFIG_DEBUG_FS
	eth_create_attr(&pdev->dev);
#endif
	return 0;
}

static int mediatek_dwmac_remove(struct platform_device *pdev)
{
#ifdef CONFIG_DEBUG_FS
	eth_remove_attr(&pdev->dev);
#endif
	stmmac_pltfr_remove(pdev);

	return 0;
}

static const struct of_device_id mediatek_dwmac_match[] = {
	{ .compatible = "mediatek,mt2712-gmac",
	  .data = &mt2712_gmac_variant },
	{ .compatible = "mediatek,mt2737-gmac",
	  .data = &mt2737_gmac_variant },
	{ }
};

MODULE_DEVICE_TABLE(of, mediatek_dwmac_match);

static struct platform_driver mediatek_dwmac_driver = {
	.probe  = mediatek_dwmac_probe,
	.remove = mediatek_dwmac_remove,
	.driver = {
		.name           = "dwmac-mediatek",
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = mediatek_dwmac_match,
	},
};
module_platform_driver(mediatek_dwmac_driver);

MODULE_AUTHOR("Biao Huang <biao.huang@mediatek.com>");
MODULE_DESCRIPTION("MediaTek DWMAC specific glue layer");
MODULE_LICENSE("GPL v2");
