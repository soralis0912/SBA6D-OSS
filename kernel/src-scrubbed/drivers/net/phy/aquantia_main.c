// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for Aquantia PHY
 *
 * Author: Shaohui Xie <Shaohui.Xie@freescale.com>
 *
 * Copyright 2015 Freescale Semiconductor, Inc.
 */
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#define DEBUG 1
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/bitfield.h>
#include <linux/phy.h>
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <linux/crc-itu-t.h>
#include <linux/of_gpio.h>
#include "mtk_eth_soc.h"
#endif
#include "aquantia.h"

#define PHY_ID_AQ1202	0x03a1b445
#define PHY_ID_AQ2104	0x03a1b460
#define PHY_ID_AQR105	0x03a1b4a2
#define PHY_ID_AQR106	0x03a1b4d0
#define PHY_ID_AQR107	0x03a1b4e0
#define PHY_ID_AQCS109	0x03a1b5c2
#define PHY_ID_AQR405	0x03a1b4b0
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#define PHY_1_ID_AQR113C	0x31c31c12
#define PHY_2_ID_AQR113C	0x31c31c13
#define AQR113C_USE_1G 0
#define AQR113C_USE_5G 0
//Global Fault Message: Address 1E.C850
#define MDIO_GLOBAL_FAULT_MESSAGE 0xC850
//Global Thermal Status 1: Address 1E.C820
#define MDIO_GLOBAL_THERMAL_STATUS_1 0xC820

//Global Thermal Status 2: Address 1E.C821
#define MDIO_GLOBAL_THERMAL_STATUS_2 0xC821

#define NET_TAG "AQR113C PHY"
#define QLOGD(f,...)        do{\
            printk("[%s]%s():%d " f " ",NET_TAG,__FUNCTION__,__LINE__,## __VA_ARGS__);}while(0);

u8 ql_match_mac_id_from_phy_addr(int addr);
#endif

#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
#define PHY_PORT 6
extern int ql_get_netlink_status(void);
extern int ql_update_link_table(u32 type, int port,int status);
extern int ql_phy_send_msg(void *pbuf);
#endif

#define MDIO_PHYXS_VEND_IF_STATUS		0xe812
#define MDIO_PHYXS_VEND_IF_STATUS_TYPE_MASK	GENMASK(7, 3)
#define MDIO_PHYXS_VEND_IF_STATUS_TYPE_KR	0
#define MDIO_PHYXS_VEND_IF_STATUS_TYPE_XFI	2
#define MDIO_PHYXS_VEND_IF_STATUS_TYPE_USXGMII	3
#define MDIO_PHYXS_VEND_IF_STATUS_TYPE_SGMII	6
#define MDIO_PHYXS_VEND_IF_STATUS_TYPE_OCSGMII	10

#define MDIO_AN_VEND_PROV			0xc400
#define MDIO_AN_VEND_PROV_1000BASET_FULL	BIT(15)
#define MDIO_AN_VEND_PROV_1000BASET_HALF	BIT(14)
#define MDIO_AN_VEND_PROV_5000BASET_FULL	BIT(11)
#define MDIO_AN_VEND_PROV_2500BASET_FULL	BIT(10)
#define MDIO_AN_VEND_PROV_DOWNSHIFT_EN		BIT(4)
#define MDIO_AN_VEND_PROV_DOWNSHIFT_MASK	GENMASK(3, 0)
#define MDIO_AN_VEND_PROV_DOWNSHIFT_DFLT	4

#define MDIO_AN_TX_VEND_STATUS1			0xc800
#define MDIO_AN_TX_VEND_STATUS1_RATE_MASK	GENMASK(3, 1)
#define MDIO_AN_TX_VEND_STATUS1_10BASET		0
#define MDIO_AN_TX_VEND_STATUS1_100BASETX	1
#define MDIO_AN_TX_VEND_STATUS1_1000BASET	2
#define MDIO_AN_TX_VEND_STATUS1_10GBASET	3
#define MDIO_AN_TX_VEND_STATUS1_2500BASET	4
#define MDIO_AN_TX_VEND_STATUS1_5000BASET	5
#define MDIO_AN_TX_VEND_STATUS1_FULL_DUPLEX	BIT(0)

#define MDIO_AN_TX_VEND_INT_STATUS1		0xcc00
#define MDIO_AN_TX_VEND_INT_STATUS1_DOWNSHIFT	BIT(1)

#define MDIO_AN_TX_VEND_INT_STATUS2		0xcc01

#define MDIO_AN_TX_VEND_INT_MASK2		0xd401
#define MDIO_AN_TX_VEND_INT_MASK2_LINK		BIT(0)

#define MDIO_AN_RX_LP_STAT1			0xe820
#define MDIO_AN_RX_LP_STAT1_1000BASET_FULL	BIT(15)
#define MDIO_AN_RX_LP_STAT1_1000BASET_HALF	BIT(14)
#define MDIO_AN_RX_LP_STAT1_SHORT_REACH		BIT(13)
#define MDIO_AN_RX_LP_STAT1_AQRATE_DOWNSHIFT	BIT(12)
#define MDIO_AN_RX_LP_STAT1_AQ_PHY		BIT(2)

#define MDIO_AN_RX_LP_STAT4			0xe823
#define MDIO_AN_RX_LP_STAT4_FW_MAJOR		GENMASK(15, 8)
#define MDIO_AN_RX_LP_STAT4_FW_MINOR		GENMASK(7, 0)

#define MDIO_AN_RX_VEND_STAT3			0xe832
#define MDIO_AN_RX_VEND_STAT3_AFR		BIT(0)

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#define MDIO_AN_RSVD_VEND_PROV		0xC410
#define MDIO_AN_RSVD_VEND_PROV_WOL_MODE	BIT(7)
#define MDIO_AN_RSVD_VEND_PROV_WOL_ENABLE	BIT(6)

/* MDIO_MMD_C22EXT */
#define MDIO_C22EXT_MAGIC_FRAME_WORD0		0xC339
#define MDIO_C22EXT_MAGIC_FRAME_WORD1		0xC33A
#define MDIO_C22EXT_MAGIC_FRAME_WORD2		0xC33B
#endif

/* MDIO_MMD_C22EXT */
#define MDIO_C22EXT_STAT_SGMII_RX_GOOD_FRAMES		0xd292
#define MDIO_C22EXT_STAT_SGMII_RX_BAD_FRAMES		0xd294
#define MDIO_C22EXT_STAT_SGMII_RX_FALSE_CARRIER		0xd297
#define MDIO_C22EXT_STAT_SGMII_TX_GOOD_FRAMES		0xd313
#define MDIO_C22EXT_STAT_SGMII_TX_BAD_FRAMES		0xd315
#define MDIO_C22EXT_STAT_SGMII_TX_FALSE_CARRIER		0xd317
#define MDIO_C22EXT_STAT_SGMII_TX_COLLISIONS		0xd318
#define MDIO_C22EXT_STAT_SGMII_TX_LINE_COLLISIONS	0xd319
#define MDIO_C22EXT_STAT_SGMII_TX_FRAME_ALIGN_ERR	0xd31a
#define MDIO_C22EXT_STAT_SGMII_TX_RUNT_FRAMES		0xd31b

/* Vendor specific 1, MDIO_MMD_VEND1 */
#define VEND1_GLOBAL_FW_ID			0x0020
#define VEND1_GLOBAL_FW_ID_MAJOR		GENMASK(15, 8)
#define VEND1_GLOBAL_FW_ID_MINOR		GENMASK(7, 0)

#define VEND1_GLOBAL_RSVD_STAT1			0xc885
#define VEND1_GLOBAL_RSVD_STAT1_FW_BUILD_ID	GENMASK(7, 4)
#define VEND1_GLOBAL_RSVD_STAT1_PROV_ID		GENMASK(3, 0)

#define VEND1_GLOBAL_RSVD_STAT9			0xc88d
#define VEND1_GLOBAL_RSVD_STAT9_MODE		GENMASK(7, 0)
#define VEND1_GLOBAL_RSVD_STAT9_1000BT2		0x23

#define VEND1_GLOBAL_INT_STD_STATUS		0xfc00
#define VEND1_GLOBAL_INT_VEND_STATUS		0xfc01

#define VEND1_GLOBAL_INT_STD_MASK		0xff00
#define VEND1_GLOBAL_INT_STD_MASK_PMA1		BIT(15)
#define VEND1_GLOBAL_INT_STD_MASK_PMA2		BIT(14)
#define VEND1_GLOBAL_INT_STD_MASK_PCS1		BIT(13)
#define VEND1_GLOBAL_INT_STD_MASK_PCS2		BIT(12)
#define VEND1_GLOBAL_INT_STD_MASK_PCS3		BIT(11)
#define VEND1_GLOBAL_INT_STD_MASK_PHY_XS1	BIT(10)
#define VEND1_GLOBAL_INT_STD_MASK_PHY_XS2	BIT(9)
#define VEND1_GLOBAL_INT_STD_MASK_AN1		BIT(8)
#define VEND1_GLOBAL_INT_STD_MASK_AN2		BIT(7)
#define VEND1_GLOBAL_INT_STD_MASK_GBE		BIT(6)
#define VEND1_GLOBAL_INT_STD_MASK_ALL		BIT(0)

#define VEND1_GLOBAL_INT_VEND_MASK		0xff01
#define VEND1_GLOBAL_INT_VEND_MASK_PMA		BIT(15)
#define VEND1_GLOBAL_INT_VEND_MASK_PCS		BIT(14)
#define VEND1_GLOBAL_INT_VEND_MASK_PHY_XS	BIT(13)
#define VEND1_GLOBAL_INT_VEND_MASK_AN		BIT(12)
#define VEND1_GLOBAL_INT_VEND_MASK_GBE		BIT(11)
#define VEND1_GLOBAL_INT_VEND_MASK_GLOBAL1	BIT(2)
#define VEND1_GLOBAL_INT_VEND_MASK_GLOBAL2	BIT(1)
#define VEND1_GLOBAL_INT_VEND_MASK_GLOBAL3	BIT(0)

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
//PHY XS Transmit
#define  PHY_XS_Transmit_Page 0x4
#define  PHY_XS_Transmit_Addr 0xC441

/* MDIO FW load */
#define GLOBAL_FIRMWARE_ID 0x20
#define GLOBAL_FAULT 0xc850
//#define GLOBAL_RSTATUS_1 0xc885

#define GLOBAL_STANDARD_CONTROL 0x0
#define SOFT_RESET BIT(15)
#define LOW_POWER BIT(11)

#define MAILBOX_CONTROL 0x0200
#define MAILBOX_EXECUTE BIT(15)
#define MAILBOX_WRITE BIT(14)
#define MAILBOX_RESET_CRC BIT(12)
#define MAILBOX_BUSY BIT(8)

#define MAILBOX_CRC 0x0201

#define MAILBOX_ADDR_MSW 0x0202
#define MAILBOX_ADDR_LSW 0x0203

#define MAILBOX_DATA_MSW 0x0204
#define MAILBOX_DATA_LSW 0x0205

#define UP_CONTROL 0xc001
#define UP_RESET BIT(15)
#define UP_RUN_STALL_OVERRIDE BIT(6)
#define UP_RUN_STALL BIT(0)

/* addresses of memory segments in the phy */
#define DRAM_BASE_ADDR 0x3FFE0000
#define IRAM_BASE_ADDR 0x40000000

/* firmware image format constants */
//  /lib/firmware/aqr113.cld
#define VERSION_STRING_SIZE 0x40
#define VERSION_STRING_OFFSET 0x0200
#define HEADER_OFFSET 0x300

#pragma pack(1)
struct fw_header {
	u8 padding[4];
	u8 iram_offset[3];
	u8 iram_size[3];
	u8 dram_offset[3];
	u8 dram_size[3];
};
#pragma pack()
#endif

struct aqr107_hw_stat {
	const char *name;
	int reg;
	int size;
};


#define SGMII_STAT(n, r, s) { n, MDIO_C22EXT_STAT_SGMII_ ## r, s }
static const struct aqr107_hw_stat aqr107_hw_stats[] = {
	SGMII_STAT("sgmii_rx_good_frames",	    RX_GOOD_FRAMES,	26),
	SGMII_STAT("sgmii_rx_bad_frames",	    RX_BAD_FRAMES,	26),
	SGMII_STAT("sgmii_rx_false_carrier_events", RX_FALSE_CARRIER,	 8),
	SGMII_STAT("sgmii_tx_good_frames",	    TX_GOOD_FRAMES,	26),
	SGMII_STAT("sgmii_tx_bad_frames",	    TX_BAD_FRAMES,	26),
	SGMII_STAT("sgmii_tx_false_carrier_events", TX_FALSE_CARRIER,	 8),
	SGMII_STAT("sgmii_tx_collisions",	    TX_COLLISIONS,	 8),
	SGMII_STAT("sgmii_tx_line_collisions",	    TX_LINE_COLLISIONS,	 8),
	SGMII_STAT("sgmii_tx_frame_alignment_err",  TX_FRAME_ALIGN_ERR,	16),
	SGMII_STAT("sgmii_tx_runt_frames",	    TX_RUNT_FRAMES,	22),
};
#define AQR107_SGMII_STAT_SZ ARRAY_SIZE(aqr107_hw_stats)

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
extern struct mtk_eth *ext_eth;
u32 _mtk_mdio_read_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 devad);
u32 _mtk_mdio_write_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data, u32 devad);
#endif

struct aqr107_priv {
	u64 sgmii_stats[AQR107_SGMII_STAT_SZ];
};

static int aqr107_get_sset_count(struct phy_device *phydev)
{
	return AQR107_SGMII_STAT_SZ;
}

static void aqr107_get_strings(struct phy_device *phydev, u8 *data)
{
	int i;

	for (i = 0; i < AQR107_SGMII_STAT_SZ; i++)
		strscpy(data + i * ETH_GSTRING_LEN, aqr107_hw_stats[i].name,
			ETH_GSTRING_LEN);
}

static u64 aqr107_get_stat(struct phy_device *phydev, int index)
{
	const struct aqr107_hw_stat *stat = aqr107_hw_stats + index;
	int len_l = min(stat->size, 16);
	int len_h = stat->size - len_l;
	u64 ret;
	int val;

	val = phy_read_mmd(phydev, MDIO_MMD_C22EXT, stat->reg);
	if (val < 0)
		return U64_MAX;

	ret = val & GENMASK(len_l - 1, 0);
	if (len_h) {
		val = phy_read_mmd(phydev, MDIO_MMD_C22EXT, stat->reg + 1);
		if (val < 0)
			return U64_MAX;

		ret += (val & GENMASK(len_h - 1, 0)) << 16;
	}

	return ret;
}

static void aqr107_get_stats(struct phy_device *phydev,
			     struct ethtool_stats *stats, u64 *data)
{
	struct aqr107_priv *priv = phydev->priv;
	u64 val;
	int i;

	for (i = 0; i < AQR107_SGMII_STAT_SZ; i++) {
		val = aqr107_get_stat(phydev, i);
		if (val == U64_MAX)
			phydev_err(phydev, "Reading HW Statistics failed for %s\n",
				   aqr107_hw_stats[i].name);
		else
			priv->sgmii_stats[i] += val;

		data[i] = priv->sgmii_stats[i];
	}
}

static int aqr_config_aneg(struct phy_device *phydev)
{
	bool changed = false;
	u16 reg;
	int ret;

	if (phydev->autoneg == AUTONEG_DISABLE)
		return genphy_c45_pma_setup_forced(phydev);

	ret = genphy_c45_an_config_aneg(phydev);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	/* Clause 45 has no standardized support for 1000BaseT, therefore
	 * use vendor registers for this mode.
	 */
	reg = 0;
	if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
			      phydev->advertising))
		reg |= MDIO_AN_VEND_PROV_1000BASET_FULL;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
			      phydev->advertising))
		reg |= MDIO_AN_VEND_PROV_1000BASET_HALF;

	/* Handle the case when the 2.5G and 5G speeds are not advertised */
	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
			      phydev->advertising))
		reg |= MDIO_AN_VEND_PROV_2500BASET_FULL;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_5000baseT_Full_BIT,
			      phydev->advertising))
		reg |= MDIO_AN_VEND_PROV_5000BASET_FULL;

	ret = phy_modify_mmd_changed(phydev, MDIO_MMD_AN, MDIO_AN_VEND_PROV,
				     MDIO_AN_VEND_PROV_1000BASET_HALF |
				     MDIO_AN_VEND_PROV_1000BASET_FULL |
				     MDIO_AN_VEND_PROV_2500BASET_FULL |
				     MDIO_AN_VEND_PROV_5000BASET_FULL, reg);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	return genphy_c45_check_and_restart_aneg(phydev, changed);
}

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static int aqr113c_config_aneg(struct phy_device *phydev)
{
	bool changed = false;
	u16 reg;
	int ret;

	//Enable serdes AN mode.
	if(1)
	{
		reg = phy_read_mmd(phydev, PHY_XS_Transmit_Page, PHY_XS_Transmit_Addr);
		reg |= 1 << 3;
		phy_write_mmd(phydev, PHY_XS_Transmit_Page, PHY_XS_Transmit_Addr, reg);
	}

	//switch sgmii mode
	if(AQR113C_USE_1G)
	{
		//PHY XS System Interface Connection Status: Address 4.E812
		reg = phy_read_mmd(phydev, 0x04, 0xe812);
		reg &= ~(0x1f<<3); reg |= 0x6 << 3;
		phy_write_mmd(phydev, 0x04, 0xe812, reg);

		//Global System Configuration For 1G: Address 1E.31C
		reg = phy_read_mmd(phydev, 0x1e, 0x31c);
		reg &= ~(0x7); reg |= 0x3 << 0;
		phy_write_mmd(phydev, 0x1e, 0x31c, reg);

		//Global System Configuration For 2.5G: Address 1E.31D
		reg = phy_read_mmd(phydev, 0x1e, 0x31d);
		reg &= ~(0x7); reg |= 0x3 << 0;
		phy_write_mmd(phydev, 0x1e, 0x31d, reg);
	}
	if(AQR113C_USE_5G)
	{
		//PHY XS System Interface Connection Status: Address 4.E812
		reg = phy_read_mmd(phydev, 0x04, 0xe812);
		reg &= ~(0x1f<<3); reg |= 0x6 << 3;
		phy_write_mmd(phydev, 0x04, 0xe812, reg);

		//PCS Standard Control 2: Address 3.7
		reg = phy_read_mmd(phydev, 0x03, 0x7);
		reg &= ~(0x7<<0); reg |= 0xb << 0;
		phy_write_mmd(phydev, 0x03, 0x7, reg);

		//Global System Configuration For 1G: Address 1E.31C
		reg = phy_read_mmd(phydev, 0x1e, 0x31c);
		reg &= ~(0x7); reg |= 0x6 << 0;
		phy_write_mmd(phydev, 0x1e, 0x31c, reg);

		//Global System Configuration For 2.5G: Address 1E.31D
		reg = phy_read_mmd(phydev, 0x1e, 0x31d);
		reg &= ~(0x7); reg |= 0x6 << 0;
		phy_write_mmd(phydev, 0x1e, 0x31d, reg);

		//Global System Configuration For 5G: Address 1E.31E
		reg = phy_read_mmd(phydev, 0x1e, 0x31e);
		reg &= ~(0x7); reg |= 0x6 << 0;
		phy_write_mmd(phydev, 0x1e, 0x31e, reg);

		//Global System Configuration For 10G: Address 1E.31F
		reg = phy_read_mmd(phydev, 0x1e, 0x31f);
		reg &= ~(0x7); reg |= 0x6 << 0;
		phy_write_mmd(phydev, 0x1e, 0x31f, reg);
	}

	//close eee mode

	//set led configure
	//led0 100M/1G link
	phy_write_mmd(phydev, 0x1e, 0xc430, 0x2063);
	//led1 2.5G/5G link
	phy_write_mmd(phydev, 0x1e, 0xc431, 0xc003);
	//led1 10G link
	phy_write_mmd(phydev, 0x1e, 0xc432, 0x0083);

	phydev_info(phydev, "%s.%d addr:0x%x\n", \
			__func__, __LINE__, phydev->mdio.addr);
	if (phydev->autoneg == AUTONEG_DISABLE)
		return genphy_c45_pma_setup_forced(phydev);

	ret = genphy_c45_an_config_aneg(phydev);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	/* Clause 45 has no standardized support for 1000BaseT, therefore
	 * use vendor registers for this mode.
	 */
	reg = 0;
	if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
			      phydev->advertising))
		reg |= MDIO_AN_VEND_PROV_1000BASET_FULL;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
			      phydev->advertising))
		reg |= MDIO_AN_VEND_PROV_1000BASET_HALF;

	ret = phy_modify_mmd_changed(phydev, MDIO_MMD_AN, MDIO_AN_VEND_PROV,
				     MDIO_AN_VEND_PROV_1000BASET_HALF |
				     MDIO_AN_VEND_PROV_1000BASET_FULL, reg);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	return genphy_c45_check_and_restart_aneg(phydev, changed);
}
#endif
static int aqr_config_intr(struct phy_device *phydev)
{
	bool en = phydev->interrupts == PHY_INTERRUPT_ENABLED;
	int err;

	err = phy_write_mmd(phydev, MDIO_MMD_AN, MDIO_AN_TX_VEND_INT_MASK2,
			    en ? MDIO_AN_TX_VEND_INT_MASK2_LINK : 0);
	if (err < 0)
		return err;

	err = phy_write_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_INT_STD_MASK,
			    en ? VEND1_GLOBAL_INT_STD_MASK_ALL : 0);
	if (err < 0)
		return err;

	return phy_write_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_INT_VEND_MASK,
			     en ? VEND1_GLOBAL_INT_VEND_MASK_GLOBAL3 |
			     VEND1_GLOBAL_INT_VEND_MASK_AN : 0);
}

static int aqr_ack_interrupt(struct phy_device *phydev)
{
	int reg;

	reg = phy_read_mmd(phydev, MDIO_MMD_AN,
			   MDIO_AN_TX_VEND_INT_STATUS2);
	return (reg < 0) ? reg : 0;
}

static int aqr_read_status(struct phy_device *phydev)
{
	int val;

	if (phydev->autoneg == AUTONEG_ENABLE) {
		val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RX_LP_STAT1);
		if (val < 0)
			return val;

		linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
				 phydev->lp_advertising,
				 val & MDIO_AN_RX_LP_STAT1_1000BASET_FULL);
		linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
				 phydev->lp_advertising,
				 val & MDIO_AN_RX_LP_STAT1_1000BASET_HALF);
	}

	return genphy_c45_read_status(phydev);
}

static int aqr107_read_downshift_event(struct phy_device *phydev)
{
	int val;

	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_TX_VEND_INT_STATUS1);
	if (val < 0)
		return val;

	return !!(val & MDIO_AN_TX_VEND_INT_STATUS1_DOWNSHIFT);
}

static int aqr107_read_rate(struct phy_device *phydev)
{
	int val;

	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_TX_VEND_STATUS1);
	if (val < 0)
		return val;

	switch (FIELD_GET(MDIO_AN_TX_VEND_STATUS1_RATE_MASK, val)) {
	case MDIO_AN_TX_VEND_STATUS1_10BASET:
		phydev->speed = SPEED_10;
		break;
	case MDIO_AN_TX_VEND_STATUS1_100BASETX:
		phydev->speed = SPEED_100;
		break;
	case MDIO_AN_TX_VEND_STATUS1_1000BASET:
		phydev->speed = SPEED_1000;
		break;
	case MDIO_AN_TX_VEND_STATUS1_2500BASET:
		phydev->speed = SPEED_2500;
		break;
	case MDIO_AN_TX_VEND_STATUS1_5000BASET:
		phydev->speed = SPEED_5000;
		break;
	case MDIO_AN_TX_VEND_STATUS1_10GBASET:
		phydev->speed = SPEED_10000;
		break;
	default:
		phydev->speed = SPEED_UNKNOWN;
		break;
	}

	if (val & MDIO_AN_TX_VEND_STATUS1_FULL_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	return 0;
}

static int aqr107_read_status(struct phy_device *phydev)
{
	int val, ret;
	val = phy_read_mmd(phydev, MDIO_MMD_PHYXS, 0xC850);

	ret = aqr_read_status(phydev);
	if (ret)
		return ret;

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	int message_code, gThermal_status1, gThermal_status2;
	message_code = phy_read_mmd(phydev, MDIO_MMD_VEND1, MDIO_GLOBAL_FAULT_MESSAGE);
	gThermal_status1 = phy_read_mmd(phydev, MDIO_MMD_VEND1, MDIO_GLOBAL_THERMAL_STATUS_1);
	gThermal_status2 = phy_read_mmd(phydev, MDIO_MMD_VEND1, MDIO_GLOBAL_THERMAL_STATUS_2);
	static int link_old = 0;
	int link_change = (link_old != phydev->link);

#ifdef CONFIG_QL_WAN_LAN_AUTO_ADAPT_SUPPORT
	int mac_id = ql_match_mac_id_from_phy_addr(phydev->mdio.addr);
	if (ql_get_netlink_status() && link_change)
	{
		QLOGD("send to app\n");
		ql_update_link_table(PHY_1_ID_AQR113C, mac_id==0?PHY_PORT:PHY_PORT+1, phydev->link);
		ql_phy_send_msg(NULL);
	}
#endif
	if(link_change) {
		//clear Receive/Transmit Activity
		phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc430, BIT(2)|BIT(3));
		phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc431, BIT(2)|BIT(3));
		phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc432, BIT(2)|BIT(3));
		switch (phydev->speed)
		{
			case SPEED_100:
			case SPEED_1000:
				phy_set_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc430, BIT(2)|BIT(3));
			break;
			case SPEED_2500:
			case SPEED_5000:
				phy_set_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc431, BIT(2)|BIT(3));
			break;
			case SPEED_10000:
				phy_set_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc432, BIT(2)|BIT(3));
			break;
			default:
			break;
		}
		QLOGD("addr:%x interface = %d link:%d message_code:%d {thermal valid:%d val:%d}", \
				phydev->mdio.addr, phydev->interface, phydev->link,\
				message_code, gThermal_status2, gThermal_status1>>8);
	}
	link_old = phydev->link;
#endif
	if (!phydev->link || phydev->autoneg == AUTONEG_DISABLE)
		return 0;

	val = phy_read_mmd(phydev, MDIO_MMD_PHYXS, MDIO_PHYXS_VEND_IF_STATUS);
	if (val < 0)
		return val;

	switch (FIELD_GET(MDIO_PHYXS_VEND_IF_STATUS_TYPE_MASK, val)) {
	case MDIO_PHYXS_VEND_IF_STATUS_TYPE_KR:
	case MDIO_PHYXS_VEND_IF_STATUS_TYPE_XFI:
		phydev->interface = PHY_INTERFACE_MODE_10GKR;
		break;
	case MDIO_PHYXS_VEND_IF_STATUS_TYPE_USXGMII:
		phydev->interface = PHY_INTERFACE_MODE_USXGMII;
		break;
	case MDIO_PHYXS_VEND_IF_STATUS_TYPE_SGMII:
		phydev->interface = PHY_INTERFACE_MODE_SGMII;
		break;
	case MDIO_PHYXS_VEND_IF_STATUS_TYPE_OCSGMII:
		phydev->interface = PHY_INTERFACE_MODE_2500BASEX;
		break;
	default:
		phydev->interface = PHY_INTERFACE_MODE_NA;
		break;
	}

	val = aqr107_read_downshift_event(phydev);
	if (val <= 0)
		return val;

	phydev_warn(phydev, "Downshift occurred! Cabling may be defective.\n");

	/* Read downshifted rate from vendor register */
	return aqr107_read_rate(phydev);
}

static int aqr107_get_downshift(struct phy_device *phydev, u8 *data)
{
	int val, cnt, enable;

	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_VEND_PROV);
	if (val < 0)
		return val;

	enable = FIELD_GET(MDIO_AN_VEND_PROV_DOWNSHIFT_EN, val);
	cnt = FIELD_GET(MDIO_AN_VEND_PROV_DOWNSHIFT_MASK, val);

	*data = enable && cnt ? cnt : DOWNSHIFT_DEV_DISABLE;

	return 0;
}

static int aqr107_set_downshift(struct phy_device *phydev, u8 cnt)
{
	int val = 0;

	if (!FIELD_FIT(MDIO_AN_VEND_PROV_DOWNSHIFT_MASK, cnt))
		return -E2BIG;

	if (cnt != DOWNSHIFT_DEV_DISABLE) {
		val = MDIO_AN_VEND_PROV_DOWNSHIFT_EN;
		val |= FIELD_PREP(MDIO_AN_VEND_PROV_DOWNSHIFT_MASK, cnt);
	}

	return phy_modify_mmd(phydev, MDIO_MMD_AN, MDIO_AN_VEND_PROV,
			      MDIO_AN_VEND_PROV_DOWNSHIFT_EN |
			      MDIO_AN_VEND_PROV_DOWNSHIFT_MASK, val);
}

static int aqr107_get_tunable(struct phy_device *phydev,
			      struct ethtool_tunable *tuna, void *data)
{
	switch (tuna->id) {
	case ETHTOOL_PHY_DOWNSHIFT:
		return aqr107_get_downshift(phydev, data);
	default:
		return -EOPNOTSUPP;
	}
}

static int aqr107_set_tunable(struct phy_device *phydev,
			      struct ethtool_tunable *tuna, const void *data)
{
	switch (tuna->id) {
	case ETHTOOL_PHY_DOWNSHIFT:
		return aqr107_set_downshift(phydev, *(const u8 *)data);
	default:
		return -EOPNOTSUPP;
	}
}

/* If we configure settings whilst firmware is still initializing the chip,
 * then these settings may be overwritten. Therefore make sure chip
 * initialization has completed. Use presence of the firmware ID as
 * indicator for initialization having completed.
 * The chip also provides a "reset completed" bit, but it's cleared after
 * read. Therefore function would time out if called again.
 */
static int aqr107_wait_reset_complete(struct phy_device *phydev)
{
	int val, retries = 100;

	do {
		val = phy_read_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_FW_ID);
		if (val < 0)
			return val;
		msleep(20);
	} while (!val && --retries);

	return val ? 0 : -ETIMEDOUT;
}

static void aqr107_chip_info(struct phy_device *phydev)
{
	u8 fw_major, fw_minor, build_id, prov_id;
	int val;

	val = phy_read_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_FW_ID);
	if (val < 0)
		return;

	fw_major = FIELD_GET(VEND1_GLOBAL_FW_ID_MAJOR, val);
	fw_minor = FIELD_GET(VEND1_GLOBAL_FW_ID_MINOR, val);

	val = phy_read_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_RSVD_STAT1);
	if (val < 0)
		return;

	build_id = FIELD_GET(VEND1_GLOBAL_RSVD_STAT1_FW_BUILD_ID, val);
	prov_id = FIELD_GET(VEND1_GLOBAL_RSVD_STAT1_PROV_ID, val);

	phydev_dbg(phydev, "FW %u.%u, Build %u, Provisioning %u\n",
		   fw_major, fw_minor, build_id, prov_id);
}

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static int aqr107_check_chip_info(struct phy_device *phydev)
{
	u8 fw_major, fw_minor;
	int val;

	val = phy_read_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_FW_ID);
	if (val < 0)
		return -1;

	fw_major = FIELD_GET(VEND1_GLOBAL_FW_ID_MAJOR, val);
	fw_minor = FIELD_GET(VEND1_GLOBAL_FW_ID_MINOR, val);

	phydev_dbg(phydev, "FW %u.%u\n",
		   fw_major, fw_minor);
	if(fw_major == 0 || fw_minor == 0)
	{
		return -1;
	}
	return 0;
}
#endif

static int aqr107_config_init(struct phy_device *phydev)
{
	int ret;

	/* Check that the PHY interface type is compatible */
	if (phydev->interface != PHY_INTERFACE_MODE_SGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_2500BASEX &&
	    phydev->interface != PHY_INTERFACE_MODE_XGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_USXGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_10GKR)
		return -ENODEV;

	WARN(phydev->interface == PHY_INTERFACE_MODE_XGMII,
	     "Your devicetree is out of date, please update it. The AQR107 family doesn't support XGMII, maybe you mean USXGMII.\n");

	ret = aqr107_wait_reset_complete(phydev);
	if (!ret)
		aqr107_chip_info(phydev);

	/* ensure that a latched downshift event is cleared */
	aqr107_read_downshift_event(phydev);

	return aqr107_set_downshift(phydev, MDIO_AN_VEND_PROV_DOWNSHIFT_DFLT);
}

static int aqcs109_config_init(struct phy_device *phydev)
{
	int ret;

	/* Check that the PHY interface type is compatible */
	if (phydev->interface != PHY_INTERFACE_MODE_SGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_2500BASEX)
		return -ENODEV;

	ret = aqr107_wait_reset_complete(phydev);
	if (!ret)
		aqr107_chip_info(phydev);

	/* AQCS109 belongs to a chip family partially supporting 10G and 5G.
	 * PMA speed ability bits are the same for all members of the family,
	 * AQCS109 however supports speeds up to 2.5G only.
	 */
	ret = phy_set_max_speed(phydev, SPEED_2500);
	if (ret)
		return ret;

	/* ensure that a latched downshift event is cleared */
	aqr107_read_downshift_event(phydev);

	return aqr107_set_downshift(phydev, MDIO_AN_VEND_PROV_DOWNSHIFT_DFLT);
}

static void aqr107_link_change_notify(struct phy_device *phydev)
{
	u8 fw_major, fw_minor;
	bool downshift, short_reach, afr;
	int mode, val;

	if (phydev->state != PHY_RUNNING || phydev->autoneg == AUTONEG_DISABLE)
		return;

	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RX_LP_STAT1);
	/* call failed or link partner is no Aquantia PHY */
	if (val < 0 || !(val & MDIO_AN_RX_LP_STAT1_AQ_PHY))
		return;

	short_reach = val & MDIO_AN_RX_LP_STAT1_SHORT_REACH;
	downshift = val & MDIO_AN_RX_LP_STAT1_AQRATE_DOWNSHIFT;

	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RX_LP_STAT4);
	if (val < 0)
		return;

	fw_major = FIELD_GET(MDIO_AN_RX_LP_STAT4_FW_MAJOR, val);
	fw_minor = FIELD_GET(MDIO_AN_RX_LP_STAT4_FW_MINOR, val);

	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RX_VEND_STAT3);
	if (val < 0)
		return;

	afr = val & MDIO_AN_RX_VEND_STAT3_AFR;

	phydev_dbg(phydev, "Link partner is Aquantia PHY, FW %u.%u%s%s%s\n",
		   fw_major, fw_minor,
		   short_reach ? ", short reach mode" : "",
		   downshift ? ", fast-retrain downshift advertised" : "",
		   afr ? ", fast reframe advertised" : "");

	val = phy_read_mmd(phydev, MDIO_MMD_VEND1, VEND1_GLOBAL_RSVD_STAT9);
	if (val < 0)
		return;

	mode = FIELD_GET(VEND1_GLOBAL_RSVD_STAT9_MODE, val);
	if (mode == VEND1_GLOBAL_RSVD_STAT9_1000BT2)
		phydev_info(phydev, "Aquantia 1000Base-T2 mode active\n");
}

static int aqr107_suspend(struct phy_device *phydev)
{
	return phy_set_bits_mmd(phydev, MDIO_MMD_VEND1, MDIO_CTRL1,
				MDIO_CTRL1_LPOWER);
}

static int aqr107_resume(struct phy_device *phydev)
{
	return phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1, MDIO_CTRL1,
				  MDIO_CTRL1_LPOWER);
}

static int aqr107_probe(struct phy_device *phydev)
{
	phydev->priv = devm_kzalloc(&phydev->mdio.dev,
				    sizeof(struct aqr107_priv), GFP_KERNEL);
	if (!phydev->priv)
		return -ENOMEM;

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	phydev->autoneg = AUTONEG_ENABLE;
	// phydev->interrupts = PHY_INTERRUPT_ENABLED;
	//Aqr113c, close mdio broadcast respond
	phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1, 0xC441, BIT(14));
	phydev_info(phydev, "aqr107_probe addr0x:%x MDIO_EN_BIT:0x%x\n", \
				phydev->mdio.addr, phy_read_mmd(phydev, MDIO_MMD_VEND1, 0xC441) & BIT(14));
#endif

	return aqr_hwmon_probe(phydev);
}

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static int aqr113_ack_interrupt(struct phy_device *phydev)
{
	int reg;

#if IS_ENABLED(CONFIG_AQUANTIA_MACSEC)
	aqr_check_txsa_expiration(phydev);
#endif
	pr_info("%s.%d\n", __func__, __LINE__);

	reg = phy_read_mmd(phydev, MDIO_MMD_AN,
			   MDIO_AN_TX_VEND_INT_STATUS2);
	return (reg < 0) ? reg : 0;
}

static int aqr113_fix_provisioning(struct phy_device *phydev)
{
	int config_regs[] = {0x31B, 0x31C, 0x31D, 0x31E, 0x31F};
	int i, val = 0;

	for (i = 0; i < ARRAY_SIZE(config_regs); i++) {
	    val = phy_read_mmd(phydev, MDIO_MMD_VEND1, config_regs[i]);
	    printk(KERN_INFO"aqr113_fix_provisioning: set provisioning for reg:%x old value=%x\n", config_regs[i], val);
#if IS_ENABLED(CONFIG_AQUANTIA_MACSEC)
	    /* Enabling MACSEC provisioning */
	    val |= BIT(9);
#endif
	    /* Enabling EEE provisioning */
	    val |= BIT(11);

	    printk(KERN_INFO"aqr113_fix_provisioning: set provisioning for reg:%x new value=%x\n", config_regs[i], val);
	    phy_write_mmd(phydev, MDIO_MMD_VEND1, config_regs[i], val);
	}

	return 0;
}
/* Enable PHY load via MDIO */
// #define MDIO_LOAD
/* number of gangloaded ports to image load */
#define NUM_OF_PORTS 1
/* Firmware filename for MDIO loading */
#define AQR113_FW "aqr113.cld"

#ifdef MDIO_LOAD

/* load data into the phy's memory */
static int aquantia_load_memory(struct phy_device *phydev, u32 addr,
				const u8 *data, size_t len)
{
	size_t pos;
	u16 crc = 0, up_crc;

	phy_write_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_CONTROL, MAILBOX_RESET_CRC);
	phy_write_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_ADDR_MSW, addr >> 16);
	phy_write_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_ADDR_LSW, addr & 0xfffc);

	for (pos = 0; pos < len; pos += min(sizeof(u32), len - pos)) {
		u32 word = 0;

		memcpy(&word, &data[pos], min(sizeof(u32), len - pos));

		phy_write_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_DATA_MSW,
			  (word >> 16));
		phy_write_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_DATA_LSW,
			  word & 0xffff);

		phy_write_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_CONTROL,
			  MAILBOX_EXECUTE | MAILBOX_WRITE);

		/* keep a big endian CRC to match the phy processor */
		word = cpu_to_be32(word);
		crc = crc_itu_t(crc, (u8 *)&word, sizeof(word));
	}

	up_crc = phy_read_mmd(phydev, MDIO_MMD_VEND1, MAILBOX_CRC);
	if (crc != up_crc) {
		phydev_info(phydev, "%s crc mismatch: calculated 0x%04hx phy 0x%04hx\n",
		       __func__, crc, up_crc);
		return -EINVAL;
	}
	return 0;
}

static u32 unpack_u24(const u8 *data)
{
	return (data[2] << 16) + (data[1] << 8) + data[0];
}

static int aquantia_upload_firmware(struct phy_device *phydev)
{
	int ret;

	size_t fw_length = 0;

	const struct firmware *fw;
	u16 calculated_crc, read_crc;
	char version[VERSION_STRING_SIZE];
	u32 primary_offset, iram_offset, iram_size, dram_offset, dram_size;
	const struct fw_header *header;

	phydev_info(phydev, "loading addr0x:%x\n", phydev->mdio.addr);
	/* Load AQR113 firmware image */
	ret = request_firmware(&fw, AQR113_FW, &phydev->mdio.dev);

	if (ret) {
		phydev_err(phydev, "Failed to load firmware %s, ret: %d\n",
			AQR113_FW, ret);
		return ret;
	}

	fw_length = fw->size;

	read_crc = (fw->data[fw_length - 2] << 8)  | fw->data[fw_length - 1];
	calculated_crc = crc_itu_t(0, fw->data, fw_length - 2);
	if (read_crc != calculated_crc) {
		phydev_err(phydev, "bad firmware crc: file 0x%04x calculated 0x%04x\n",
		       read_crc, calculated_crc);
		ret = -EINVAL;
		goto done;
	}

	/* Find the DRAM and IRAM sections within the firmware file. */
	primary_offset = ((fw->data[9] & 0xf) << 8 | fw->data[8]) << 12;

	header = (struct fw_header *)&fw->data[primary_offset + HEADER_OFFSET];

	iram_offset = primary_offset + unpack_u24(header->iram_offset);
	iram_size = unpack_u24(header->iram_size);

	dram_offset = primary_offset + unpack_u24(header->dram_offset);
	dram_size = unpack_u24(header->dram_size);

	phydev_info(phydev, "primary %d iram offset=%d size=%d dram offset=%d size=%d\n",
	      primary_offset, iram_offset, iram_size, dram_offset, dram_size);

	strlcpy(version, (char *)&fw->data[dram_offset + VERSION_STRING_OFFSET],
		VERSION_STRING_SIZE);
	phydev_info(phydev, "loading firmare version '%s'\n", version);

	/* stall the microcprocessor */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, UP_CONTROL,
		  UP_RUN_STALL | UP_RUN_STALL_OVERRIDE);

	phydev_info(phydev, "loading dram 0x%08x from offset=%d size=%d\n",
	      DRAM_BASE_ADDR, dram_offset, dram_size);
	ret = aquantia_load_memory(phydev, DRAM_BASE_ADDR, &fw->data[dram_offset],
				   dram_size);
	if (ret != 0){
		phydev_info(phydev, "firmare loading addr:0x%x failed.\n", phydev->mdio.addr);
		goto done;
	}

	phydev_info(phydev, "loading iram 0x%08x from offset=%d size=%d\n",
	      IRAM_BASE_ADDR, iram_offset, iram_size);
	ret = aquantia_load_memory(phydev, IRAM_BASE_ADDR, &fw->data[iram_offset],
				   iram_size);
	if (ret != 0) {
		phydev_info(phydev, "firmare loading addr:0x%x failed.111\n", phydev->mdio.addr);
		goto done;
	}

	/* make sure soft reset and low power mode are clear */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, GLOBAL_STANDARD_CONTROL, 0);

	/* Release the microprocessor. UP_RESET must be held for 100 usec. */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, UP_CONTROL,
		  UP_RUN_STALL | UP_RUN_STALL_OVERRIDE | UP_RESET);

	udelay(100);

	phy_write_mmd(phydev, MDIO_MMD_VEND1, UP_CONTROL, UP_RUN_STALL_OVERRIDE);

	phydev_info(phydev, "firmare loading addr:0x%x done.\n", phydev->mdio.addr);
done:
	release_firmware(fw);

	return ret;
}

#endif

#if IS_ENABLED(CONFIG_AQUANTIA_MACSEC)
static int aqr_mdio_write(void *priv, unsigned int mmd, unsigned int address, unsigned int data) {
	return phy_write_mmd((struct phy_device *)priv, mmd, address, data);
}

static int aqr_mdio_read(void *priv, unsigned int mmd, unsigned int address) {
	return phy_read_mmd((struct phy_device *)priv, mmd, address);
}
#endif

static u32 MmdPhyRead(struct mtk_eth *eth, int phy_addr, uint16_t page, uint16_t reg, uint16_t *val)
{
    int ret = 0;
	*val = _mtk_mdio_read_C45(eth, phy_addr, reg, page);
    return ret;
}

static u32 MmdPhyWrite(struct mtk_eth *eth, int phy_addr, uint16_t page, uint16_t reg, uint16_t val)
{
    int ret = 0;
	_mtk_mdio_write_C45(eth, phy_addr, reg, val, page);
    return ret;
}

int aqr_phy_write_mmd(struct phy_device *phydev, int devnum, u16 regnum, u16 val)
{
	if(ext_eth)
		return MmdPhyWrite(ext_eth, phydev->mdio.addr, devnum, regnum, val);
	else
		return 0;
}

int aqr_phy_read_mmd(struct phy_device *phydev, int devnum, u16 regnum)
{
	uint16_t tmp = 0;
	if(ext_eth)
		MmdPhyRead(ext_eth, phydev->mdio.addr, devnum, regnum, &tmp);
	return tmp;
}

static int aqr113c_phy_status_cb(struct phy_device *phydev)
{
    int ret = 0;
    u32 tmp1 = 0, tmp2 = 0, phy_id = 0;
	char *__check = "failed";

    phy_write_mmd(phydev, 0x1e, 0x300, 0x55ff);
    tmp1 = phy_read_mmd(phydev, 0x1e, 0x300);
    if(tmp1 == 0x55ff) {
        tmp1 = phy_read_mmd(phydev, 0x1, 0x2);
        tmp2 = phy_read_mmd(phydev, 0x1, 0x3);
        phy_id = tmp1 << 16 | tmp2;

		if(phy_id == PHY_1_ID_AQR113C || phy_id == PHY_2_ID_AQR113C)
		{
			__check = "success";
		}
        printk("addr:0x%x Test phy_id:0x%x aqr mdio scratchpad register %s\n", \
                            phydev->mdio.addr, phy_id, __check);
        ret = 0;
    }
    else {
        printk("addr:0x%x Test aqr mdio scratchpad register failed tmp1:0x%x\n", phydev->mdio.addr, tmp1);
        ret = -1;
    }
    return ret;
}

static int aqr113_config_init(struct phy_device *phydev)
{
	int ret;

#if IS_ENABLED(CONFIG_AQUANTIA_MACSEC)
	struct aqr107_priv *priv = phydev->priv;
	struct aqr_port *port = &priv->port;
#endif
	pr_info("%s.%d addr:%x \n", __func__, __LINE__, phydev->mdio.addr);

	if(aqr107_check_chip_info(phydev) == 0)
		return 0;

	/* Check that the PHY interface type is compatible */
	if (phydev->interface != PHY_INTERFACE_MODE_SGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_2500BASEX &&
	    phydev->interface != PHY_INTERFACE_MODE_XGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_USXGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_10GKR)
		return -ENODEV;

	WARN(phydev->interface == PHY_INTERFACE_MODE_XGMII,
	     "Your devicetree is out of date, please update it. The AQR107 family doesn't support XGMII, maybe you mean USXGMII.\n");

	phydev->is_c45 = true;

	aqr113c_phy_status_cb(phydev);

	aqr113_fix_provisioning(phydev);

#if IS_ENABLED(CONFIG_AQUANTIA_MACSEC)
	port->device = aqr_gen_4;
	port->priv = phydev;
	port->mdio_ops.aqr_mdio_write = &aqr_mdio_write;
	port->mdio_ops.aqr_mdio_read = &aqr_mdio_read;

	phydev->macsec_ops = &aqr_macsec_ops;
#endif

#ifdef MDIO_LOAD
		aquantia_upload_firmware(phydev);
#endif

	ret = aqr107_wait_reset_complete(phydev);
	if (!ret)
		aqr107_chip_info(phydev);

	linkmode_copy(phydev->advertising, phydev->supported);
	/* ensure that a latched downshift event is cleared */
	aqr107_read_downshift_event(phydev);

	return aqr107_set_downshift(phydev, MDIO_AN_VEND_PROV_DOWNSHIFT_DFLT);
}

static void aqr113_get_wol(struct phy_device *phydev, struct ethtool_wolinfo *wol)
{
	int val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RSVD_VEND_PROV);

	wol->supported = WAKE_MAGIC | WAKE_PHY;

	wol->wolopts = (val & MDIO_AN_RSVD_VEND_PROV_WOL_ENABLE) ? WAKE_PHY : 0;
}

static int aqr113_set_wol(struct phy_device *phydev, struct ethtool_wolinfo *wol)
{
	struct net_device *ndev = phydev->attached_dev;
	const u8 *addr;
	uint16_t i;
	int err = 0;

	if (!ndev)
		return -ENODEV;

	if (wol->wolopts & WAKE_PHY) {
		phy_set_bits_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RSVD_VEND_PROV,
				MDIO_AN_RSVD_VEND_PROV_WOL_ENABLE);
		/* Set 100BASE-TX WoL mode. For 1000BASE-T set WOL_MODE bit to 1. */
		phy_clear_bits_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RSVD_VEND_PROV,
				MDIO_AN_RSVD_VEND_PROV_WOL_MODE);
	} else {
		phy_clear_bits_mmd(phydev, MDIO_MMD_AN, MDIO_AN_RSVD_VEND_PROV,
				MDIO_AN_RSVD_VEND_PROV_WOL_ENABLE);
	}

	if (wol->wolopts & WAKE_MAGIC)
	{
		addr = (const u8*)ndev->dev_addr;

		if (!is_valid_ether_addr(addr))
			return -EINVAL;

		/* Write mac address by reversed word */
		i = (addr[0] << 8) | addr[1];
		err = phy_write_mmd(phydev, MDIO_MMD_C22EXT, MDIO_C22EXT_MAGIC_FRAME_WORD0, i);

		if (err < 0)
			return err;

		i = (addr[2] << 8) | addr[3];
		phy_write_mmd(phydev, MDIO_MMD_C22EXT, MDIO_C22EXT_MAGIC_FRAME_WORD1, i);

		if (err < 0)
			return err;

		i = (addr[4] << 8) | addr[5];
		err = phy_write_mmd(phydev, MDIO_MMD_C22EXT, MDIO_C22EXT_MAGIC_FRAME_WORD2, i);
	}

	return err;
}
#endif

static struct phy_driver aqr_driver[] = {
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQ1202),
	.name		= "Aquantia AQ1202",
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr_read_status,
},
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQ2104),
	.name		= "Aquantia AQ2104",
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr_read_status,
},
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQR105),
	.name		= "Aquantia AQR105",
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr_read_status,
	.suspend	= aqr107_suspend,
	.resume		= aqr107_resume,
},
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQR106),
	.name		= "Aquantia AQR106",
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr_read_status,
},
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQR107),
	.name		= "Aquantia AQR107",
	.probe		= aqr107_probe,
	.config_init	= aqr107_config_init,
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr107_read_status,
	.get_tunable    = aqr107_get_tunable,
	.set_tunable    = aqr107_set_tunable,
	.suspend	= aqr107_suspend,
	.resume		= aqr107_resume,
	.get_sset_count	= aqr107_get_sset_count,
	.get_strings	= aqr107_get_strings,
	.get_stats	= aqr107_get_stats,
	.link_change_notify = aqr107_link_change_notify,
},
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQCS109),
	.name		= "Aquantia AQCS109",
	.probe		= aqr107_probe,
	.config_init	= aqcs109_config_init,
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr107_read_status,
	.get_tunable    = aqr107_get_tunable,
	.set_tunable    = aqr107_set_tunable,
	.suspend	= aqr107_suspend,
	.resume		= aqr107_resume,
	.get_sset_count	= aqr107_get_sset_count,
	.get_strings	= aqr107_get_strings,
	.get_stats	= aqr107_get_stats,
	.link_change_notify = aqr107_link_change_notify,
},
{
	PHY_ID_MATCH_MODEL(PHY_ID_AQR405),
	.name		= "Aquantia AQR405",
	.config_aneg    = aqr_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr_ack_interrupt,
	.read_status	= aqr_read_status,
},
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
{
	PHY_ID_MATCH_MODEL(PHY_1_ID_AQR113C),
	.name		= "Aquantia AQR113C",
	.probe		= aqr107_probe,
	.config_init	= aqr113_config_init,
	.config_aneg    = aqr113c_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr113_ack_interrupt,
	.read_status	= aqr107_read_status,
	.get_tunable    = aqr107_get_tunable,
	.set_tunable    = aqr107_set_tunable,
	.suspend	= aqr107_suspend,
	.resume		= aqr107_resume,
	.get_sset_count	= aqr107_get_sset_count,
	.get_strings	= aqr107_get_strings,
	.get_stats	= aqr107_get_stats,
	.link_change_notify = aqr107_link_change_notify,
	.read_mmd = aqr_phy_read_mmd,
	.write_mmd = aqr_phy_write_mmd,
},
{
	PHY_ID_MATCH_MODEL(PHY_2_ID_AQR113C),
	.name		= "Aquantia AQR113C 11",
	.probe		= aqr107_probe,
	.config_init	= aqr113_config_init,
	.config_aneg    = aqr113c_config_aneg,
	.config_intr	= aqr_config_intr,
	.ack_interrupt	= aqr113_ack_interrupt,
	.read_status	= aqr107_read_status,
	.get_tunable    = aqr107_get_tunable,
	.set_tunable    = aqr107_set_tunable,
	.suspend	= aqr107_suspend,
	.resume		= aqr107_resume,
	.get_sset_count	= aqr107_get_sset_count,
	.get_strings	= aqr107_get_strings,
	.get_stats	= aqr107_get_stats,
	.link_change_notify = aqr107_link_change_notify,
	.read_mmd = aqr_phy_read_mmd,
	.write_mmd = aqr_phy_write_mmd,
},
#endif
};

module_phy_driver(aqr_driver);

static struct mdio_device_id __maybe_unused aqr_tbl[] = {
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQ1202) },
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQ2104) },
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQR105) },
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQR106) },
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQR107) },
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQCS109) },
	{ PHY_ID_MATCH_MODEL(PHY_ID_AQR405) },
	{ }
};

MODULE_DEVICE_TABLE(mdio, aqr_tbl);

MODULE_DESCRIPTION("Aquantia PHY driver");
MODULE_AUTHOR("Shaohui Xie <Shaohui.Xie@freescale.com>");
MODULE_LICENSE("GPL v2");
