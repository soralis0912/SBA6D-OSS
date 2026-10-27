// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2021 Felix Fietkau <nbd@nbd.name> */

#ifndef __MTK_WED_PRIV_H
#define __MTK_WED_PRIV_H

#include <linux/soc/mediatek/mtk_wed.h>
#include <linux/debugfs.h>
#include <linux/regmap.h>
#include <linux/netdevice.h>

//About PCIE setting
#ifdef CONFIG_MEDIATEK_WED_V2
#define MTK_PCIE_BASE(n)                (0x11280000 + (n) * 0x8000)
#define PCIE_NODE "pcie"
#define VLPCFG_DEVICE_NODE "mediatek,mt6990-vlpcfg"
#define PERICFG_AO_DEVICE_NODE "mediatek,mt6990-pericfg_ao"
#define INFRAM_DEVICE_NODE "mediatek,infracfg_ao"
#define PCIE_MSI_GROUP0_MASK 0xC08
#define PCIE_MSI_GROUP1_MASK 0xC0C
#define VLPCFG_INT_MASK 0x118 //rg_pcie_bridge_irq_dis
#define PERICFG_AO_INT_MASK_PCIE_0 0xB04
#define PERICFG_AO_INT_MASK_PCIE_1 0xB08
#else
#define MTK_PCIE_BASE(n)		(0x1a143000 + (n) * 0x2000)
#endif

#define MTK_WED_PKT_SIZE		1920 //by logan driver
#define MTK_WED_BUF_SIZE		2048
#define MTK_WED_BUF_PER_PAGE		(PAGE_SIZE / 2048)
#define MTK_WED_RX_RING_SIZE		1536 //by logan driver

#define MTK_WED_TX_RING_SIZE		2048 //by logan driver
#define MTK_WED_WDMA_RING_SIZE		512
#define MTK_WED_MAX_GROUP_SIZE		0x100 //no use
#define MTK_WED_VLD_GROUP_SIZE		0x40
#define MTK_WED_PER_GROUP_PKT		128

#define MTK_WED_FBUF_SIZE		128 //by logan driver
#define MTK_WED_MIOD_CNT		16
#define MTK_WED_FB_CMD_CNT		1024
#define MTK_WED_RRO_QUE_CNT		8192
#define MTK_WED_MIOD_ENTRY_CNT		128 //rro_ctrl->miod_entry_size

#define MODULE_ID_WO		1

struct mtk_eth;
struct mtk_wed_wo;

struct mtk_wed_hw {
	struct device_node *node;
	struct mtk_eth *eth;
	struct regmap *regs;
	struct regmap *hifsys;
	struct device *dev;
	void __iomem *wdma;
	struct regmap *mirror;
	struct dentry *debugfs_dir;
	struct mtk_wed_device *wed_dev;
	struct mtk_wed_wo *wed_wo;
	u32 debugfs_reg;
	u32 num_flows;
	u32 wdma_phy;
	char dirname[5];
	int ring_num;
	int irq;
	int index;
	u32 ver;
};

struct mtk_wdma_info {
	u8 wdma_idx;
	u8 queue;
	u16 wcid;
	u8 bss;
};

enum {
        WARP_WED1 = 0,
        WARP_WED2 = 1,
        WARP_WED3 = 2,
        WARP_WED_NUM
};


#ifdef CONFIG_NET_MEDIATEK_SOC_WED
static inline void
wed_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	regmap_write(dev->hw->regs, reg, val);
}

static inline u32
wed_r32(struct mtk_wed_device *dev, u32 reg)
{
	unsigned int val;

	regmap_read(dev->hw->regs, reg, &val);

	return val;
}

static inline u32
wifi_r32(struct mtk_wed_device *dev, u32 reg)
{
	return readl(dev->wlan.base + reg);
}

static inline void
wifi_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	writel(val, dev->wlan.base + reg);
}

static inline void
wdma_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	writel(val, dev->hw->wdma + reg);
}

static inline u32
wdma_r32(struct mtk_wed_device *dev, u32 reg)
{
	return readl(dev->hw->wdma + reg);
}

static inline u32
wpdma_tx_r32(struct mtk_wed_device *dev, int ring, u32 reg)
{
	if (!dev->tx_ring[ring].wpdma)
		return 0;

	return readl(dev->tx_ring[ring].wpdma + reg);
}

static inline void
wpdma_tx_w32(struct mtk_wed_device *dev, int ring, u32 reg, u32 val)
{
	if (!dev->tx_ring[ring].wpdma)
		return;

	writel(val, dev->tx_ring[ring].wpdma + reg);
}

static inline u32
wpdma_txfree_r32(struct mtk_wed_device *dev, u32 reg)
{
	if (!dev->txfree_ring.wpdma)
		return 0;

	return readl(dev->txfree_ring.wpdma + reg);
}

static inline void
wpdma_txfree_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	if (!dev->txfree_ring.wpdma)
		return;

	writel(val, dev->txfree_ring.wpdma + reg);
}

static inline u32
wpdma_rx_r32(struct mtk_wed_device *dev, int ring, u32 reg)
{
	if (!dev->rx_ring[ring].wpdma)
		return 0;

	return readl(dev->rx_ring[ring].wpdma + reg);
}

static inline void
wpdma_rx_w32(struct mtk_wed_device *dev, int ring, u32 reg, u32 val)
{
	if (!dev->rx_ring[ring].wpdma)
		return;

	writel(val, dev->rx_ring[ring].wpdma + reg);
}
void mtk_wed_add_hw(struct device_node *np, struct mtk_eth *eth,
		    void __iomem *wdma, u32 wdma_phy, int index);
void mtk_wed_exit(void);
int mtk_wed_flow_add(int index);
void mtk_wed_flow_remove(int index);
void mtk_wed_fe_reset(void);
void mtk_wed_fe_reset_complete(void);

#else
static inline void
mtk_wed_add_hw(struct device_node *np, struct mtk_eth *eth,
	       void __iomem *wdma, u32 wdma_phy, int index)
{
}
static inline void
mtk_wed_exit(void)
{
}
static inline int mtk_wed_flow_add(int index)
{
	return -EINVAL;
}
static inline void mtk_wed_flow_remove(int index)
{
}
static inline void mtk_wed_fe_reset(void)
{
}

static inline void mtk_wed_fe_reset_complete(void)
{
}
#endif

#ifdef CONFIG_DEBUG_FS
void mtk_wed_hw_add_debugfs(struct mtk_wed_hw *hw);
#else
static inline void mtk_wed_hw_add_debugfs(struct mtk_wed_hw *hw)
{
}
#endif

int wed_wo_hardware_init(struct mtk_wed_wo *wo, irq_handler_t isr);
void wed_wo_hardware_exit(struct mtk_wed_wo *wo);
int wed_wo_mcu_init(struct mtk_wed_wo *wo);
int mtk_wed_exception_init(struct mtk_wed_wo *wo);
void mtk_wed_mcu_rx_unsolicited_event(struct mtk_wed_wo *wo, struct sk_buff *skb);
int mtk_wed_mcu_cmd_sanity_check(struct mtk_wed_wo *wo, struct sk_buff *skb);
void wed_wo_mcu_debugfs(struct mtk_wed_hw *hw, struct dentry *dir);
void mtk_wed_mcu_rx_event(struct mtk_wed_wo *wo, struct sk_buff *skb);
int mtk_wed_mcu_send_msg(struct mtk_wed_wo *wo,int to_id, int cmd,
			const void *data, int len, bool wait_resp);
int mtk_wed_wo_rx_poll(struct napi_struct *napi, int budget);
void get_wo_heartbeat(struct mtk_wed_wo *wo);
#endif
