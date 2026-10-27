/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2019 MediaTek Inc.
 * Copyright (C) 2009-2016 John Crispin <blogic@openwrt.org>
 * Copyright (C) 2009-2016 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2013-2016 Michael Lee <igvtee@gmail.com>
 */

#ifndef MTK_ETH_DBG_H
#define MTK_ETH_DBG_H

#ifdef CONFIG_MTK_ETHERNET_DBG
#define MTKETH_MII_READ                  0x89F3
#define MTKETH_MII_WRITE                 0x89F4
#define MTKETH_ESW_REG_READ              0x89F1
#define MTKETH_ESW_REG_WRITE             0x89F2
#define MTKETH_MII_READ_CL45             0x89FC
#define MTKETH_MII_WRITE_CL45            0x89FD
#define MTKETH_QDMA_IOCTL				 0x89F8

#define REG_ESW_MAX                     0xFC

#define REG_ATA1_ADDR			(0x0074)
#define REG_ATA2_ADDR			(0x0078)
#define REG_ATC_ADDR			(0x0080)
#define REG_ATRD_ADDR			(0x008C)

struct mtk_esw_reg {
	unsigned int off;
	unsigned int val;
};

struct mtk_mii_ioctl_data {
	u16 phy_id;
	u16 reg_num;
	unsigned int val_in;
	unsigned int val_out;
	unsigned int port_num;
	unsigned int dev_addr;
	unsigned int reg_addr;
};

/*****************QDMA IOCTL DATA*************/
#define MTKETH_QDMA_SCH_READ				0x0000
#define MTKETH_QDMA_SCH_WRITE				0x0001
#define MTKETH_QDMA_QUEUE_MAPPING        	0x0002
#define MTKETH_QDMA_SCH_POLICY_WRITE      	0x0003
#define MTKETH_QDMA_SCH_RATE_WRITE      	0x0004


#define REG_HQOS_MAX			0x1BFF


struct mtk_qdma_ioctl_data {
	unsigned int cmd;
	unsigned int queue_id;
	unsigned int val;
	unsigned int mask;
};



#if defined(CONFIG_NET_DSA_MT7530) || defined(CONFIG_MT753X_GSW)
static inline bool mt7530_exist(struct mtk_eth *eth)
{
	return true;
}
#else
static inline bool mt7530_exist(struct mtk_eth *eth)
{
	return false;
}
#endif



int debug_proc_init(struct mtk_eth *eth);
void debug_proc_exit(void);

int mtketh_debugfs_init(struct mtk_eth *eth);
void mtketh_debugfs_exit(struct mtk_eth *eth);
int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

void mii_mgr_write_combine(struct mtk_eth *eth, u32 phy_addr, u32 phy_register,
	u32 write_data);
void mii_mgr_read_combine(struct mtk_eth *eth, u32 phy_addr, u32 phy_register,
	u32 *read_data);

u32 mt7530_mdio_r32(struct mtk_eth *eth, u32 reg);

#else
int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	return 0;
}

int mtketh_debugfs_init(struct mtk_eth *eth)
{
	return 0;
}

void mtketh_debugfs_exit(struct mtk_eth *eth)
{
}

int debug_proc_init(struct mtk_eth *eth)
{
	return 0;
}

void debug_proc_exit(void)
{
}
#endif

#endif /* MTK_ETH_DBG_H */
