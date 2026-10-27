// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/misc/mediatek/ethernet/rtl8251/rtl8251.c
 *
 * Driver for Realtek PHYs
 *
 * Author: Joe Huang
 *
 * Copyright (c) 2023 Quectel, Inc.
 */
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#define DEBUG 1
#include "mtk_eth_soc.h"
#endif
#include "rtl8251.h"
#include "nic_rtl8251b.h"
#include "nic_rtl8251b_init.h"

#include <linux/bitops.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of_net.h>

#define RTL_SUPPORTS_5000FULL			BIT(14)
#define RTL_SUPPORTS_2500FULL			BIT(13)
#define RTL_SUPPORTS_10000FULL			BIT(0)
#define RTL_ADV_2500FULL			BIT(7)
#define RTL_LPADV_10000FULL			BIT(11)
#define RTL_LPADV_5000FULL			BIT(6)
#define RTL_LPADV_2500FULL			BIT(5)

#define PHY_ID_RTL8251	0x1CC86A
#define NET_TAG "RTL8251 PHY"

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
extern struct mtk_eth *ext_eth;
u32 _mtk_mdio_read_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 devad);
u32 _mtk_mdio_write_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data, u32 devad);
#endif

BOOLEAN __MmdPhyRead(HANDLE eth,uint16_t page, uint16_t reg, uint16_t *val)
{
    int ret = TRUE;
	*val = _mtk_mdio_read_C45(eth->eth, eth->phy_addr, reg, page);
    return ret;
}

BOOLEAN __MmdPhyWrite(HANDLE eth, uint16_t page, uint16_t reg, uint16_t val)
{
    int ret = TRUE;
	_mtk_mdio_write_C45(eth->eth, eth->phy_addr, reg, val, page);
    return ret;
}

int rtl_phy_write_mmd(struct phy_device *phydev, int devnum, u16 regnum, u16 val)
{
	int ret;
    struct _mtk_eth Device;
	struct _mtk_eth *hDevice = &Device;
	Device.eth = ext_eth;
	Device.phy_addr = phydev->mdio.addr;
	QLOGD("--");
	if(ext_eth) {
		ret = __MmdPhyWrite(hDevice, devnum, regnum, val);
		QLOGD("++");
		return ret;
	}
	else
		return 0;
}

int rtl_phy_read_mmd(struct phy_device *phydev, int devnum, u16 regnum)
{
	uint16_t tmp = 0;
    struct _mtk_eth Device;
	struct _mtk_eth *hDevice = &Device;
	Device.eth = ext_eth;
	Device.phy_addr = phydev->mdio.addr;
	QLOGD("--");
	if(ext_eth) {
		__MmdPhyRead(hDevice, devnum, regnum, &tmp);
		QLOGD("++");
	}
	return tmp;
}

static void __init_phy(HANDLE hDevice)
{
	PHY_EEE_ENABLE EeeEnable;
	EeeEnable.EEE_5G = 0;
    EeeEnable.EEE_2_5G = 0;
	EeeEnable.EEE_1000 = 0;
	EeeEnable.EEE_100 = 0;
	Rtl8251b_ThermalSensorEnable_set(hDevice, 1);
	Rtl8251b_eeeEnable_set(hDevice, &EeeEnable);
	Rtl8251b_greenEnable_set(hDevice, 0);

}

static int rtl8251_config_init(struct phy_device *phydev)
{
	int ret = 0;
    struct _mtk_eth Device;
	struct _mtk_eth *hDevice = &Device;
	Device.eth = ext_eth;
	Device.phy_addr = phydev->mdio.addr;
	QLOGD("interface:%d\n", phydev->interface);
	/* Check that the PHY interface type is compatible */
	if (phydev->interface != PHY_INTERFACE_MODE_SGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_2500BASEX &&
	    phydev->interface != PHY_INTERFACE_MODE_XGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_USXGMII &&
	    phydev->interface != PHY_INTERFACE_MODE_10GKR){
			QLOGD("please set currect phydev->interface\n");
			return -ENODEV;
	}

    if(ext_eth)
        ret = Rtl8251b_phy_init(hDevice, NULL, 1);
	QLOGD("addr:%d status:%s\n", \
			phydev->mdio.addr, ret?"false":"true");
    __init_phy(hDevice);
	return 0;
}

static int rtl8251_config_aneg(struct phy_device *phydev)
{
    struct _mtk_eth Device;
	struct _mtk_eth *hDevice = &Device;
	Device.eth = ext_eth;
	Device.phy_addr = phydev->mdio.addr;
    PHY_LINK_ABILITY LINK_ABILITY;
    LINK_ABILITY.Half_10 = 1;
    LINK_ABILITY.Full_10 = 1;
    LINK_ABILITY.Half_100 = 1;
    LINK_ABILITY.Full_100 = 1;
    LINK_ABILITY.Full_1000 = 1;
    LINK_ABILITY.adv_2_5G = 1;
    LINK_ABILITY.adv_5G = 1;
    LINK_ABILITY.FC = 1;
    LINK_ABILITY.AsyFC = 1;
	QLOGD("..");
    Rtl8251b_autoNegoEnable_set(hDevice, 1);
    Rtl8251b_autoNegoAbility_set(hDevice, &LINK_ABILITY);
    return 0;
}

static int rtl8251_read_status(struct phy_device *phydev)
{
    struct _mtk_eth Device;
	struct _mtk_eth *hDevice = &Device;
	Device.eth = ext_eth;
	Device.phy_addr = phydev->mdio.addr;
    PHY_SERDES_MODE serdes_mode;
    bool serdesLink;
	int ret = genphy_read_status(phydev);
    Rtl8251b_serdes_link_get(hDevice, &serdesLink, &serdes_mode);
    QLOGD("addr:%d serdesLink:%d serdes_mode:%d speed:%d duplex:%d\n", \
			phydev->mdio.addr, serdesLink, serdes_mode, \
            phydev->speed, phydev->duplex);
    return ret;
}

static int rtl8251_probe(struct phy_device *phydev)
{

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	phydev->autoneg = AUTONEG_ENABLE;
	// phydev->interrupts = PHY_INTERRUPT_ENABLED;
	QLOGD("rtl8251_probe addr:0x%x\n", phydev->mdio.addr);
#endif

	return 0;
}

static struct phy_driver realtek_drvs[] = {
    {
		PHY_ID_MATCH_EXACT(PHY_ID_RTL8251),
		.name		= "RTL8251B Gigabit Ethernet",
		.probe		= rtl8251_probe,
		.config_init	= rtl8251_config_init,
        .read_status  = rtl8251_read_status,
        .config_aneg    = rtl8251_config_aneg,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
        .read_mmd = rtl_phy_read_mmd,
	    .write_mmd = rtl_phy_write_mmd,
	},
};

module_phy_driver(realtek_drvs);

static struct mdio_device_id __maybe_unused rtl_tbl[] = {
	{ PHY_ID_MATCH_EXACT(PHY_ID_RTL8251) },
	{ }
};

MODULE_DEVICE_TABLE(mdio, rtl_tbl);

MODULE_DESCRIPTION("Realtek PHY driver");
MODULE_AUTHOR("Joe Huang");
MODULE_LICENSE("GPL");