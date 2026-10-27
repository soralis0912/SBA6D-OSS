// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2021 Felix Fietkau <nbd@nbd.name> */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/bitfield.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/mfd/syscon.h>
#include <linux/debugfs.h>
#include <linux/iopoll.h>
#include <linux/soc/mediatek/mtk_wed.h>
#include <net/rtnetlink.h>

#include "mtk_eth_soc.h"
#include "mtk_wed_regs.h"
#include "mtk_wed.h"
#ifdef CONFIG_MTK_PPE
#include "mtk_ppe.h"
#endif
#include "mtk_wed_mcu.h"
#include "mtk_wed_wo.h"

struct wo_cmd_ring {
	u32 q_base;
	u32 cnt;
	u32 unit;
};
static struct mtk_wed_hw *hw_list[WARP_WED_NUM];
static DEFINE_MUTEX(hw_lock);

static void
wed_m32(struct mtk_wed_device *dev, u32 reg, u32 mask, u32 val)
{
	regmap_update_bits(dev->hw->regs, reg, mask | val, val);
}

static void
wed_set(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	return wed_m32(dev, reg, 0, mask);
}

static void
wed_clr(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	return wed_m32(dev, reg, mask, 0);
}

static void
wdma_m32(struct mtk_wed_device *dev, u32 reg, u32 mask, u32 val)
{
	wdma_w32(dev, reg, (wdma_r32(dev, reg) & ~mask) | val);
}

static void
wdma_set(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	wdma_m32(dev, reg, 0, mask);
}

static void
wdma_clr(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	wdma_m32(dev, reg, mask, 0);
}

static u32
mtk_wdma_read_reset(struct mtk_wed_device *dev)
{
	return wdma_r32(dev, MTK_WDMA_GLO_CFG);
}

static int
mtk_wdma_rx_reset(struct mtk_wed_device *dev)
{
	u32 status;
	u32 mask = MTK_WDMA_GLO_CFG_RX_DMA_BUSY;
	int busy, i;

	wdma_clr(dev, MTK_WDMA_GLO_CFG, MTK_WDMA_GLO_CFG_RX_DMA_EN);
	busy = readx_poll_timeout(mtk_wdma_read_reset, dev, status,
			       !(status & mask), 0, 10000);

	wdma_w32(dev, MTK_WDMA_RESET_IDX, MTK_WDMA_RESET_IDX_RX);
	wdma_w32(dev, MTK_WDMA_RESET_IDX, 0);

	for (i = 0; i < ARRAY_SIZE(dev->rx_wdma); i++)
		if (!dev->rx_wdma[i].desc) {
			wdma_w32(dev, MTK_WDMA_RING_RX(i) +
				 MTK_WED_RING_OFS_CPU_IDX, 0);
	}

	return busy;
}

static void
mtk_wdma_tx_reset(struct mtk_wed_device *dev)
{
	u32 status;
	u32 mask = MTK_WDMA_GLO_CFG_TX_DMA_BUSY;
	int i;

	wdma_clr(dev, MTK_WDMA_GLO_CFG, MTK_WDMA_GLO_CFG_TX_DMA_EN);
	if (readx_poll_timeout(mtk_wdma_read_reset, dev, status,
			       !(status & mask), 0, 10000))
		WARN_ON_ONCE(1);

	wdma_w32(dev, MTK_WDMA_RESET_IDX, MTK_WDMA_RESET_IDX_TX);
	wdma_w32(dev, MTK_WDMA_RESET_IDX, 0);
	for (i = 0; i < ARRAY_SIZE(dev->tx_wdma); i++)
		wdma_w32(dev, MTK_WDMA_RING_TX(i) +
			 MTK_WED_RING_OFS_CPU_IDX, 0);
}

static u32
mtk_wed_read_reset(struct mtk_wed_device *dev)
{
	return wed_r32(dev, MTK_WED_RESET);
}

static void
mtk_wed_reset(struct mtk_wed_device *dev, u32 mask)
{
	u32 status;

	wed_w32(dev, MTK_WED_RESET, mask);
	if (readx_poll_timeout(mtk_wed_read_reset, dev, status,
			       !(status & mask), 0, 1000))
		WARN_ON_ONCE(1);
}

static void
mtk_wed_wo_reset(struct mtk_wed_device *dev)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;
	u8 state = WO_STATE_DISABLE;
	u8 state_done = WOIF_DISABLE_DONE;
	void __iomem *reg;
	u32 value;
	unsigned long timeout = jiffies + WOCPU_TIMEOUT;

	mtk_wdma_tx_reset(dev);

	mtk_wed_reset(dev, MTK_WED_RESET_WED);

	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_CHANGE_STATE,
			     &state, sizeof(state), false);

	do {
		value = wed_r32(dev, MTK_WED_SCR0 + 4 * WED_DUMMY_CR_WO_STATUS);
	} while (value != state_done && !time_after(jiffies, timeout));

	//for mt6990
	switch(dev->hw->index) {
	case WARP_WED1:
		reg = ioremap(WOCPU_WO0_MCU_RESET_ADDR, 4);
		value = readl((void *)reg);
		value |= WOCPU_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		value &= ~WOCPU_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		iounmap((void *)reg);
		break;
	case WARP_WED2:
		reg = ioremap(WOCPU_WO1_MCU_RESET_ADDR, 4);
		value = readl((void *)reg);
		value |= WOCPU_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		value &= ~WOCPU_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		iounmap((void *)reg);
		break;
	case WARP_WED3:
		reg = ioremap(WOCPU_WO2_MCU_RESET_ADDR, 4);
		value = readl((void *)reg);
		value |= WOCPU_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		value &= ~WOCPU_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		iounmap((void *)reg);
		break;

	default:
		dev_err(dev->hw->dev, "wrong mtk_wed%d\n",
			dev->hw->index);

		break;
	}

}

static void mtk_wed_wo_set_apsrc_idle(struct mtk_wed_hw *hw)
{
	unsigned long base_addr, addr_remap;
	u32 value = 0;
	u32 wed_idx = hw->index;

	pr_info("%s: start..\n",  __func__);

	base_addr = (WOCPU_MCUSYS_HS_BASE_ADDR + wed_idx * WOCPU_MCUSYS_OFFSET
		+ WOX_MCU_CFG_HS_WOX_APSRC_ADDR);
	addr_remap = (unsigned long)ioremap(base_addr, 4);
	value = readl((void *)addr_remap);
	value |= WOX_MCU_CFG_HS_WOX_APSRC_WOX_APSRC_DDREN_MASK;
	value |= WOX_MCU_CFG_HS_WOX_APSRC_WOX_APSRC_REQ_MASK;
	value |= WOX_MCU_CFG_HS_WOX_APSRC_WOX_APSRC_IDLE_MASK;
	writel(value, (void *)addr_remap);
	iounmap((void *)addr_remap);

	pr_info("%s: end..\n", __func__);

}

void mtk_wed_fe_reset(void)
{
	int i;

	mutex_lock(&hw_lock);

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw = hw_list[i];
		struct mtk_wed_device *dev = hw->wed_dev;
		int err;

		if (!dev || !dev->wlan.reset)
			continue;

		pr_info("%s: receive fe reset start event, trigger SER\n", __func__);

		/* reset callback blocks until WLAN reset is completed */
		err = dev->wlan.reset(dev);
		if (err)
			dev_err(dev->dev, "wlan reset failed: %d\n", err);
	}

	mutex_unlock(&hw_lock);
}

void mtk_wed_fe_reset_complete(void)
{
	int i;

	mutex_lock(&hw_lock);

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw = hw_list[i];
		struct mtk_wed_device *dev = hw->wed_dev;

		if (!dev || !dev->wlan.reset_complete)
			continue;

		pr_info("%s: receive fe reset done event, continue SER\n", __func__);
		dev->wlan.reset_complete(dev);
	}

	mutex_unlock(&hw_lock);
}

static struct mtk_wed_hw *
mtk_wed_assign(struct mtk_wed_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw = hw_list[i];

		if (!hw || hw->wed_dev)
			continue;

		hw->wed_dev = dev;
		return hw;
	}

	return NULL;
}

static int
mtk_wed_buffer_alloc(struct mtk_wed_device *dev)
{
	struct mtk_wdma_desc *desc;
	dma_addr_t desc_phys;
	void **page_list;
	u32 last_seg = MTK_WDMA_DESC_CTRL_LAST_SEG1;
	int token = dev->wlan.token_start;
	int ring_size, n_pages, page_idx;
	int i;


	if (dev->ver == MTK_WED_V1) {
		ring_size = dev->wlan.nbuf & ~(MTK_WED_BUF_PER_PAGE - 1);
	} else {
		ring_size = MTK_WED_VLD_GROUP_SIZE * MTK_WED_PER_GROUP_PKT +
			    MTK_WED_WDMA_RING_SIZE * 2;
		last_seg = MTK_WDMA_DESC_CTRL_LAST_SEG0;
	}

	n_pages = ring_size / MTK_WED_BUF_PER_PAGE;

	page_list = kcalloc(n_pages, sizeof(*page_list), GFP_KERNEL);
	if (!page_list)
		return -ENOMEM;

	dev->buf_ring.size = dev->wlan.nbuf & ~(MTK_WED_BUF_PER_PAGE - 1);
	dev->buf_ring.pages = page_list;

	desc = dma_alloc_coherent(dev->hw->dev, ring_size * sizeof(*desc),
				  &desc_phys, GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	dev->buf_ring.desc = desc;
	dev->buf_ring.desc_phys = desc_phys;

	for (i = 0, page_idx = 0; i < ring_size; i += MTK_WED_BUF_PER_PAGE) {
		dma_addr_t page_phys, buf_phys;
		struct page *page;
		void *buf;
		int s;

		page = __dev_alloc_pages(GFP_KERNEL, 0);
		if (!page)
			return -ENOMEM;

		page_phys = dma_map_page(dev->hw->dev, page, 0, PAGE_SIZE,
					 DMA_BIDIRECTIONAL);
		if (dma_mapping_error(dev->hw->dev, page_phys)) {
			__free_page(page);
			return -ENOMEM;
		}

		page_list[page_idx++] = page;
		dma_sync_single_for_cpu(dev->hw->dev, page_phys, PAGE_SIZE,
					DMA_BIDIRECTIONAL);

		buf = page_to_virt(page);
		buf_phys = page_phys;

		for (s = 0; s < MTK_WED_BUF_PER_PAGE; s++) {
			u32 txd_size;

			txd_size = dev->wlan.init_buf(buf, buf_phys, token++);

			desc->buf0 = buf_phys; //sdp1, pkt_pa
			desc->buf1 = buf_phys + txd_size; //sdp0, fdesc_pa
			desc->ctrl = FIELD_PREP(MTK_WDMA_DESC_CTRL_LEN0, //ctrl
						txd_size) |
				     FIELD_PREP(MTK_WDMA_DESC_CTRL_LEN1,
						MTK_WED_BUF_SIZE - txd_size) |
						last_seg;
			desc->info = 0;

			//support 33 bit
			//desc->info =  ((((buf_phys + txd_size) >> 32) & 0xf) |
			//		((buf_phys >> 32) & 0xf) >> 16);

			desc++;

			buf += MTK_WED_BUF_SIZE;
			buf_phys += MTK_WED_BUF_SIZE;
		}

		dma_sync_single_for_device(dev->hw->dev, page_phys, PAGE_SIZE,
					   DMA_BIDIRECTIONAL);
	}

	return 0;
}

static void
mtk_wed_free_buffer(struct mtk_wed_device *dev)
{
	struct mtk_wdma_desc *desc = dev->buf_ring.desc;
	void **page_list = dev->buf_ring.pages;
	int ring_size, page_idx;
	int i;

	if (!page_list)
		return;

	if (!desc)
		goto free_pagelist;

	if (dev->ver == MTK_WED_V1) {
		ring_size = dev->wlan.nbuf & ~(MTK_WED_BUF_PER_PAGE - 1);
	} else {
		ring_size = MTK_WED_VLD_GROUP_SIZE * MTK_WED_PER_GROUP_PKT +
			    MTK_WED_WDMA_RING_SIZE * 2;
	}

	for (i = 0, page_idx = 0; i < ring_size; i += MTK_WED_BUF_PER_PAGE) {
		void *page = page_list[page_idx++];

		if (!page)
			break;

		dma_unmap_page(dev->hw->dev, desc[i].buf0,
			       PAGE_SIZE, DMA_BIDIRECTIONAL);
		__free_page(page);
	}

	dma_free_coherent(dev->hw->dev, ring_size * sizeof(*desc),
			  desc, dev->buf_ring.desc_phys);

free_pagelist:
	kfree(page_list);
}

static int
mtk_wed_rx_bm_alloc(struct mtk_wed_device *dev)
{
	struct mtk_rxbm_desc *desc;
	dma_addr_t desc_phys;
	int ring_size;

	ring_size = dev->wlan.rx_nbuf;
	dev->rx_buf_ring.size = ring_size;
	desc = dma_alloc_coherent(dev->hw->dev, ring_size * sizeof(*desc),
				  &desc_phys, GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	dev->rx_buf_ring.desc = desc;
	dev->rx_buf_ring.desc_phys = desc_phys;

	dev->wlan.init_rx_buf(dev, dev->wlan.rx_npkt);
	return 0;
}

static void
mtk_wed_free_rx_bm(struct mtk_wed_device *dev)
{
	struct mtk_rxbm_desc *desc = dev->rx_buf_ring.desc;
	int ring_size = dev->rx_buf_ring.size;

	if (!desc)
		return;

	dev->wlan.release_rx_buf(dev);

	dma_free_coherent(dev->hw->dev, ring_size * sizeof(*desc),
			  desc, dev->rx_buf_ring.desc_phys);
}

static void
mtk_wed_free_ring(struct mtk_wed_device *dev, struct mtk_wed_ring *ring, int scale)
{
	if (!ring->desc)
		return;

	dma_free_coherent(dev->hw->dev, ring->size * sizeof(*ring->desc) * scale,
			  ring->desc, ring->desc_phys);
}

static void
mtk_wed_free_tx_rings(struct mtk_wed_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dev->tx_ring); i++)
		mtk_wed_free_ring(dev, &dev->tx_ring[i], 1);
	for (i = 0; i < ARRAY_SIZE(dev->tx_wdma); i++)
		mtk_wed_free_ring(dev, &dev->tx_wdma[i], dev->ver);
}

static void
mtk_wed_free_rx_rings(struct mtk_wed_device *dev)
{
	mtk_wed_free_rx_bm(dev);
	mtk_wed_free_ring(dev, &dev->rro.rro_ring, 1);
}

static void
mtk_wed_set_int(struct mtk_wed_device *dev, u32 irq_mask)
{
	u32 wdma_mask;

	wdma_mask = FIELD_PREP(MTK_WDMA_INT_MASK_RX_DONE, GENMASK(1, 0));
	if (dev->ver > MTK_WED_V1)
		wdma_mask |= FIELD_PREP(MTK_WDMA_INT_MASK_TX_DONE,
					GENMASK(1, 0));
	/* wed control cr set */
	wed_set(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_WDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WPDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WED_TX_BM_EN |
		MTK_WED_CTRL_WED_TX_FREE_AGENT_EN);

	if (dev->ver == MTK_WED_V1) {
		wed_w32(dev, MTK_WED_PCIE_INT_TRIGGER,
			MTK_WED_PCIE_INT_TRIGGER_STATUS);

		wed_w32(dev, MTK_WED_WPDMA_INT_TRIGGER,
			MTK_WED_WPDMA_INT_TRIGGER_RX_DONE |
			MTK_WED_WPDMA_INT_TRIGGER_TX_DONE);

		wed_set(dev, MTK_WED_WPDMA_INT_CTRL,
			MTK_WED_WPDMA_INT_CTRL_FLD_SUBRT_ADV);
	} else {

		wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_TX,
			MTK_WED_WPDMA_INT_CTRL_TX0_DONE_EN |
			MTK_WED_WPDMA_INT_CTRL_TX0_DONE_CLR |
			MTK_WED_WPDMA_INT_CTRL_TX1_DONE_EN |
			MTK_WED_WPDMA_INT_CTRL_TX1_DONE_CLR |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_TX0_DONE_TRIG,
				   dev->wlan.tx_tbit[0]) |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_TX1_DONE_TRIG,
				   dev->wlan.tx_tbit[1]));

		wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_TX_FREE,
			MTK_WED_WPDMA_INT_CTRL_TX_FREE_DONE_EN |
			MTK_WED_WPDMA_INT_CTRL_TX_FREE_DONE_CLR |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_TX_FREE_DONE_TRIG,
				    dev->wlan.txfree_tbit));

		wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_RX,
			MTK_WED_WPDMA_INT_CTRL_RX0_EN |
			MTK_WED_WPDMA_INT_CTRL_RX0_CLR |
			MTK_WED_WPDMA_INT_CTRL_RX1_EN |
			MTK_WED_WPDMA_INT_CTRL_RX1_CLR |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RX0_DONE_TRIG,
				   dev->wlan.rx_tbit[0]) |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RX1_DONE_TRIG,
				   dev->wlan.rx_tbit[1]));
	}
	wed_w32(dev, MTK_WED_WDMA_INT_TRIGGER, wdma_mask);
	if (dev->ver == MTK_WED_V1) {
		wed_clr(dev, MTK_WED_WDMA_INT_CTRL, wdma_mask);
	} else {
		wed_w32(dev, MTK_WED_WDMA_INT_CLR, wdma_mask);
		wed_set(dev, MTK_WED_WDMA_INT_CTRL,
			FIELD_PREP(MTK_WED_WDMA_INT_POLL_SRC_SEL, dev->wdma_idx)); //for mt6990
	}

	wdma_w32(dev, MTK_WDMA_INT_MASK, wdma_mask);
	wdma_w32(dev, MTK_WDMA_INT_GRP2, wdma_mask);
	wed_w32(dev, MTK_WED_WPDMA_INT_MASK, irq_mask);
	wed_w32(dev, MTK_WED_INT_MASK, irq_mask);
}

static void
mtk_wed_set_ext_int(struct mtk_wed_device *dev, bool en)
{
	u32 mask = MTK_WED_EXT_INT_STATUS_ERROR_MASK;

	if (!dev->hw->num_flows)
		mask &= ~MTK_WED_EXT_INT_STATUS_TKID_WO_PYLD;

	wed_w32(dev, MTK_WED_EXT_INT_MASK, en ? mask : 0);
	wed_r32(dev, MTK_WED_EXT_INT_MASK);
}

static void
mtk_wed_set_512_support(struct mtk_wed_device *dev, bool en)
{
	if (en) {
		wed_w32(dev, MTK_WED_TXDP_CTRL, MTK_WED_TXDP_DW9_OVERWR);
		wed_w32(dev, MTK_WED_TXP_DW1,
			FIELD_PREP(MTK_WED_WPDMA_WRITE_TXP, 0x0103));
	} else {
		wed_w32(dev, MTK_WED_TXP_DW1,
			FIELD_PREP(MTK_WED_WPDMA_WRITE_TXP, 0x0100));
		wed_clr(dev, MTK_WED_TXDP_CTRL, MTK_WED_TXDP_DW9_OVERWR);
	}
}

static void
mtk_wed_check_wfdma_rx_fill(struct mtk_wed_device *dev, int idx)
{
#define MTK_WFMDA_RX_DMA_EN 	BIT(2)

	int timeout = 3;
	u32 cur_idx, regs;

	do {
		regs = MTK_WED_WPDMA_RING_RX_DATA(idx) +
		       MTK_WED_RING_OFS_CPU_IDX;
		cur_idx = wed_r32(dev, regs);
		if (cur_idx == MTK_WED_RX_RING_SIZE - 1)
			break;

		usleep_range(100000, 200000);
		timeout--;
	} while (timeout > 0);

	if (timeout) {
		unsigned int val;

		val = wifi_r32(dev, dev->wlan.wpdma_rx_glo -
			       dev->wlan.phy_base);
		val |= MTK_WFMDA_RX_DMA_EN;

		wifi_w32(dev, dev->wlan.wpdma_rx_glo -
			 dev->wlan.phy_base, val);
	} else {
		dev_err(dev->hw->dev, "mtk_wed%d: rx(%d) dma enable failed!\n",
			       dev->hw->index, idx);
	}
}

static void
mtk_wed_dma_enable(struct mtk_wed_device *dev)
{
	wed_set(dev, MTK_WED_GLO_CFG,
		MTK_WED_GLO_CFG_TX_DMA_EN |
		MTK_WED_GLO_CFG_RX_DMA_EN);

	wed_set(dev, MTK_WED_WPDMA_GLO_CFG,
		MTK_WED_WPDMA_GLO_CFG_TX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DRV_EN);

	wed_set(dev, MTK_WED_WDMA_GLO_CFG,
		MTK_WED_WDMA_GLO_CFG_RX_DRV_EN);

	wdma_clr(dev, MTK_WDMA_GLO_CFG, MTK_WDMA_GLO_CFG_FLD_RESV_BUF);//for CDM FIFO wrokaround

	wdma_set(dev, MTK_WDMA_GLO_CFG,
		MTK_WDMA_GLO_CFG_TX_DMA_EN |
		MTK_WDMA_GLO_CFG_RX_INFO1_PRERES |
		FIELD_PREP(MTK_WDMA_GLO_CFG_FLD_RESV_BUF, 0x80)); //add CDM FIFO workaround
		//MTK_WDMA_GLO_CFG_RX_INFO1_PRERES |
		//MTK_WDMA_GLO_CFG_RX_INFO2_PRERES);//no MTK_WDMA_GLO_CFG_RX_INFO2_PRERES for collgin

	if (dev->ver == MTK_WED_V1) {
		wdma_set(dev, MTK_WDMA_GLO_CFG,
			 MTK_WDMA_GLO_CFG_RX_INFO3_PRERES);

		wed_set(dev, MTK_WED_WPDMA_INT_CTRL,
			MTK_WED_WPDMA_INT_CTRL_FLD_SUBRT_ADV);
	} else {
		int idx = 0;

		wed_set(dev, MTK_WED_WPDMA_CTRL,
			MTK_WED_WPDMA_CTRL_SDL1_FIXED);

		wed_set(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_TX_DRV_EN |
			MTK_WED_WDMA_GLO_CFG_TX_DDONE_CHK);

		//WED_WPDMA_TX

		/* For support BMAC/HWIFI, we use DDONE2 to check free done event */
		wed_set(dev, MTK_WED_WPDMA_GLO_CFG, MTK_WED_WPDMA_GLO_CFG_RX_DDONE2_WR);

		/*Bollinger TXD_VER should be 1 for 33bit*/
		//wed_set(dev, MTK_WED_WPDMA_GLO_CFG, MTK_WED_WPDMA_GLO_CFG_FLD_TXD_VER);

		/*new txbm, not required to keep token id*/
		wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_TX_TKID_KEEP |
			MTK_WED_WPDMA_GLO_CFG_TX_DMAD_DW3_PREV);

		//WED_WPDMA_RX

		wed_set(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_PKT_PROC |
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_CRX_SYNC);

		//w->tx_free_done_ver <= 0x5
		wed_set(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_UEVENT_PKT_FMT_CHK |
			MTK_WED_WPDMA_GLO_CFG_FLD_RX_DRV_UNS_VER_FORCE_4);

		wed_set(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RX_DRV_EN |
			FIELD_PREP(MTK_WED_WPDMA_RX_D_RXD_READ_LEN, 0x18) |
			FIELD_PREP(MTK_WED_WPDMA_RX_D_INIT_PHASE_RXEN_SEL,
				   0x2));

		for (idx = 0; idx < dev->hw->ring_num; idx++)
			mtk_wed_check_wfdma_rx_fill(dev, idx);
	}
}

static void
mtk_wed_dma_disable(struct mtk_wed_device *dev)
{
	wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
		MTK_WED_WPDMA_GLO_CFG_TX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DRV_EN);

	wed_clr(dev, MTK_WED_WDMA_GLO_CFG,
		MTK_WED_WDMA_GLO_CFG_RX_DRV_EN);

	wed_clr(dev, MTK_WED_GLO_CFG,
		MTK_WED_GLO_CFG_TX_DMA_EN |
		MTK_WED_GLO_CFG_RX_DMA_EN);

	wdma_clr(dev, MTK_WDMA_GLO_CFG,
		 MTK_WDMA_GLO_CFG_TX_DMA_EN |
		 MTK_WDMA_GLO_CFG_RX_INFO1_PRERES |
		 MTK_WDMA_GLO_CFG_RX_INFO2_PRERES);

	if (dev->ver == MTK_WED_V1) {
		regmap_write(dev->hw->mirror, dev->hw->index * 4, 0);
		wdma_clr(dev, MTK_WDMA_GLO_CFG,
			 MTK_WDMA_GLO_CFG_RX_INFO3_PRERES);
	} else {
		wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_PKT_PROC |
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_CRX_SYNC);
		wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RX_DRV_EN);
		wed_clr(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_TX_DDONE_CHK);
	}

	mtk_wed_set_512_support(dev, false);
}

static void
mtk_wed_stop(struct mtk_wed_device *dev)
{
	if (dev->ver > MTK_WED_V1) {
		wed_w32(dev, MTK_WED_EXT_INT_MASK1, 0);
		wed_w32(dev, MTK_WED_EXT_INT_MASK2, 0);
	}
	mtk_wed_set_ext_int(dev, false);

	wed_w32(dev, MTK_WED_WPDMA_INT_TRIGGER, 0);
	wed_w32(dev, MTK_WED_WDMA_INT_TRIGGER, 0);
	wdma_w32(dev, MTK_WDMA_INT_MASK, 0);
	wdma_w32(dev, MTK_WDMA_INT_GRP2, 0);
	wed_w32(dev, MTK_WED_WPDMA_INT_MASK, 0);
}

static void
mtk_wed_deinit(struct mtk_wed_device *dev)
{
	mtk_wed_stop(dev);
	mtk_wed_dma_disable(dev);

	wed_clr(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_WDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WPDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WED_TX_BM_EN |
		MTK_WED_CTRL_WED_TX_FREE_AGENT_EN);

	if (dev->hw->ver == 1)
		return;

	wed_clr(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_RX_ROUTE_QM_EN |
		MTK_WED_CTRL_WED_RX_BM_EN |
		MTK_WED_CTRL_RX_RRO_QM_EN);
}

/*
*  set MSI group# to WED
*/
static void
mtk_wed_pcie_msi_set(struct mtk_wed_device *dev, bool msi_enable)
{
        struct device_node *pcie_node = NULL;

        struct pcie_dts {
                unsigned long base_addr;
        }pcie;

	u32 msi_group0_mask = PCIE_MSI_GROUP0_MASK;
	u32 msi_group1_mask = PCIE_MSI_GROUP1_MASK;
        u32 value = 0;
        u32 domain = 0;
	u32 idx = dev->hw->index;

	dev_info(dev->hw->dev, "%s(): msi_enable is %d\n", __func__, msi_enable);

        do {
		pcie_node = of_find_node_by_name(pcie_node, PCIE_NODE);
		of_property_read_u32(pcie_node, "linux,pci-domain",&domain);
		if (idx == domain)
			break;
        } while (pcie_node);

        if (pcie_node == NULL)
                dev_info(dev->hw->dev, "%s(): get device_node faill\n", __func__);

        pcie.base_addr = (unsigned long)of_iomap(pcie_node, 0);

//        if (msi_msg_base_addr && ((msi_msg_base_addr & 0xfffff000) >> 12 == (bus->pcie_base[idx] & 0xfffff000) >> 12)) {
//                msi_group0_mask = (msi_msg_base_addr & 0xff0) + (msi_group0_mask & 0x00f);
//                msi_group1_mask = (msi_msg_base_addr & 0xff0) + (msi_group1_mask & 0x00f);
//        }

        dev_info(dev->hw->dev, "%s(): msi_group0_mask is 0x%x\n", __func__, msi_group0_mask);
        dev_info(dev->hw->dev, "%s(): msi_group1_mask is 0x%x\n", __func__, msi_group1_mask);

	value = readl((void *)pcie.base_addr + msi_group0_mask);

        if (msi_enable)
                value &= ~(1 << 0);
        else
                value |= (1 << 0);

	writel(value, (void *)pcie.base_addr + msi_group0_mask);

	value = readl((void *)pcie.base_addr + msi_group1_mask);

        if (msi_enable)
                value |= (1 << 0);
        else
                value &= ~(1 << 0);

	writel(value, (void *)pcie.base_addr + msi_group1_mask);

	iounmap((void *)pcie.base_addr);
}

static void
mtk_wed_pcie_clk_set(struct mtk_wed_device *dev, bool enable)
{

	struct device_node *node;
	struct node_dts {
		unsigned long base_addr;
	}infra_clk;

	u32 value;

	if (dev->hw->index == WARP_WED3 || dev->hw->index == WARP_WED2) {
		node = of_find_compatible_node(NULL, NULL, PERICFG_AO_DEVICE_NODE);
	}
	else {
		node = of_find_compatible_node(NULL, NULL, VLPCFG_DEVICE_NODE);
	}

	if (node == NULL)
		dev_err(dev->hw->dev, "%s(): get device_node faill\n", __func__);

	infra_clk.base_addr = (unsigned long)of_iomap(node, 0);

	dev_info(dev->hw->dev, "%s(): node_cfg.base_addr= 0x%lx\n", __func__, infra_clk.base_addr);
	dev_info(dev->hw->dev, "%s(): dev index is %d\n", __func__, dev->hw->index);

	switch (dev->hw->index) {
	case WARP_WED1:
		value = readl((void *)infra_clk.base_addr + VLPCFG_INT_MASK);
		if (enable)
			value |= (1 << 11);
		else
			value &= ~(1 << 11);
		writel(value, (void *)infra_clk.base_addr + VLPCFG_INT_MASK);
		break;

	case WARP_WED2:
		value = readl((void *)infra_clk.base_addr + PERICFG_AO_INT_MASK_PCIE_0);
		if (enable)
			value |= (1 << 3);
		else
			value |= ~(1 << 3);
		writel(value, (void *)infra_clk.base_addr + PERICFG_AO_INT_MASK_PCIE_0);
		break;

	case WARP_WED3:
		value = readl((void *)infra_clk.base_addr + PERICFG_AO_INT_MASK_PCIE_1);
		if (enable)
			value |= (1 << 3);
		else
			value |= ~(1 << 3);
		writel(value, (void *)infra_clk.base_addr + PERICFG_AO_INT_MASK_PCIE_1);
		break;

	default:
		break;

	}

	iounmap((void *)infra_clk.base_addr);
}

static void
mtk_wed_detach(struct mtk_wed_device *dev)
{
        struct device_node *wlan_node;
        struct mtk_wed_hw *hw = dev->hw;

        mutex_lock(&hw_lock);

        mtk_wed_deinit(dev);

        mtk_wdma_rx_reset(dev);

        mtk_wed_reset(dev, MTK_WED_RESET_WED);

        mtk_wdma_tx_reset(dev);

        mtk_wed_free_buffer(dev);
        mtk_wed_free_tx_rings(dev);
        if (dev->ver > MTK_WED_V1) {
                mtk_wed_wo_reset(dev);
		mtk_wed_wo_set_apsrc_idle(hw);
                mtk_wed_free_rx_rings(dev);
                mtk_wed_wo_exit(hw);
        }

        if (dev->wlan.bus_type == MTK_WED_BUS_PCIE) {
                wlan_node = dev->wlan.pci_dev->dev.of_node;
                if (of_dma_is_coherent(wlan_node))
                        regmap_update_bits(hw->hifsys, HIFSYS_DMA_AG_MAP,
                                           BIT(hw->index), BIT(hw->index));

                if (dev->ver > MTK_WED_V1) {
                        mtk_wed_pcie_msi_set(dev, 0);
                        mtk_wed_pcie_clk_set(dev, 0);
                }
        }

//      There is no mtk_eth_set_dma_device() and eth->dma_dev in ethernet's code base
//      if (!hw_list[!hw->index]->wed_dev &&
//          hw->eth->dma_dev != hw->eth->dev)
//              mtk_eth_set_dma_device(hw->eth, hw->eth->dev);

        memset(dev, 0, sizeof(*dev));
        module_put(THIS_MODULE);

        hw->wed_dev = NULL;
        mutex_unlock(&hw_lock);
}



static void
mtk_wed_bus_init(struct mtk_wed_device *dev, bool msi_enable)
{
	u32 pcie_base_addr, value = 0;

	if (dev->wlan.bus_type == MTK_WED_BUS_PCIE) {

		mtk_wed_pcie_msi_set(dev, msi_enable);
		mtk_wed_pcie_clk_set(dev, 1);

		pcie_base_addr = MTK_PCIE_BASE(dev->hw->index);

		//for wed workaround INT POLL
		wed_w32(dev, MTK_WED_PCIE_INT_CTRL,
			FIELD_PREP(MTK_WED_PCIE_INT_CTRL_POLL_EN, 2));
		
		if (msi_enable) {
			/* Use legacy WED INTx design to instead MSI design. */
			value = wed_r32(dev, MTK_WED_PCIE_INT_CTRL);
			value &= ~MTK_WED_PCIE_INT_CTRL_FLD_MSI_EN;
			value |= MTK_WED_PCIE_INT_CTRL_FLD_IRQ_MSI_SEL;
			value |= MTK_WED_PCIE_INT_CTRL_FLD_CLR_PCIE_BF_WPDMA;
			wed_set(dev, MTK_WED_PCIE_INT_CTRL, value);

			//pcie interrupt status register
			wed_w32(dev, MTK_WED_PCIE_CFG_INTM, pcie_base_addr | 0xC0C);
			wed_w32(dev, MTK_WED_PCIE_CFG_BASE, pcie_base_addr | 0xC04);
			wed_w32(dev, MTK_WED_PCIE_INT_TRIGGER, 0xff);

			value = wed_r32(dev, MTK_WED_PCIE_INTS_CLR);
			value |= 0xff;
			wed_set(dev, MTK_WED_PCIE_INTS_CLR, value);

		} else {
			//pcie interrupt status register, only support INTx here
			wed_w32(dev, MTK_WED_PCIE_CFG_INTM, pcie_base_addr | 0x180);
			wed_w32(dev, MTK_WED_PCIE_CFG_BASE, pcie_base_addr | 0x184);
			wed_w32(dev, MTK_WED_PCIE_INT_TRIGGER, BIT(24));
		}

		//pcie interrupt control: pola/source selection
		value = wed_r32(dev, MTK_WED_PCIE_INT_CTRL);
		value |= MTK_WED_PCIE_INT_CTRL_MSK_EN_POLA;
		value |= FIELD_PREP(MTK_WED_PCIE_INT_CTRL_SRC_SEL, dev->hw->index);
		wed_set(dev, MTK_WED_PCIE_INT_CTRL, value);

	} else if (dev->wlan.bus_type == MTK_WED_BUS_AXI) {
		wed_set(dev, MTK_WED_WPDMA_INT_CTRL,
			MTK_WED_WPDMA_INT_CTRL_FLD_SIG_SRC |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_FLD_SIG_SRC, 0));
	}

	return;
}

static void
mtk_wed_set_wpdma(struct mtk_wed_device *dev)
{
	if (dev->ver > MTK_WED_V1) {
		wed_w32(dev, MTK_WED_WPDMA_CFG_BASE,  dev->wlan.wpdma_int);
		wed_w32(dev, MTK_WED_WPDMA_CFG_INT_MASK,  dev->wlan.wpdma_mask);
		wed_w32(dev, MTK_WED_WPDMA_CFG_TX,  dev->wlan.wpdma_tx);
		wed_w32(dev, MTK_WED_WPDMA_CFG_TX_FREE,  dev->wlan.wpdma_txfree);

		wed_w32(dev, MTK_WED_WPDMA_RX_GLO_CFG,  dev->wlan.wpdma_rx_glo);
		wed_w32(dev, MTK_WED_WPDMA_RX_RING,  dev->wlan.wpdma_rx);
	} else {
		wed_w32(dev, MTK_WED_WPDMA_CFG_BASE,  dev->wlan.wpdma_phys);
	}
}

static void
mtk_wed_hw_init_early(struct mtk_wed_device *dev)
{
	u32 mask, set, msi_enable = 1;//default, it can be passed by mt76 driver

	mtk_wed_deinit(dev);
	mtk_wed_reset(dev, MTK_WED_RESET_WED);

	if (dev->ver > MTK_WED_V1)
		mtk_wed_bus_init(dev, msi_enable);

	mtk_wed_set_wpdma(dev);

	mask = MTK_WED_WDMA_GLO_CFG_BT_SIZE |
	       MTK_WED_WDMA_GLO_CFG_DYNAMIC_DMAD_RECYCLE | //bit 25
	       MTK_WED_WDMA_GLO_CFG_RX_DIS_FSM_AUTO_IDLE;
	set = FIELD_PREP(MTK_WED_WDMA_GLO_CFG_BT_SIZE, 2) |
	      MTK_WED_WDMA_GLO_CFG_DYNAMIC_SKIP_DMAD_PREP | //bit 24
	      MTK_WED_WDMA_GLO_CFG_IDLE_DMAD_SUPPLY; //bit 23

	wed_m32(dev, MTK_WED_WDMA_GLO_CFG, mask, set);

	if (dev->ver == MTK_WED_V1) {
		u32 offset;
		offset = dev->hw->index ? 0x04000400 : 0;
		wed_w32(dev, MTK_WED_WDMA_OFFSET0, 0x2a042a20 + offset);
		wed_w32(dev, MTK_WED_WDMA_OFFSET1, 0x29002800 + offset);

		wed_w32(dev, MTK_WED_PCIE_CFG_BASE, MTK_PCIE_BASE(dev->hw->index));
	} else {
		wed_w32(dev, MTK_WED_WDMA_CFG_BASE, dev->hw->wdma_phy);
		wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_ETH_DMAD_FMT);
		wed_w32(dev, MTK_WED_WDMA_OFFSET0,
			FIELD_PREP(MTK_WED_WDMA_OFST0_GLO_INTS,
				   MTK_WDMA_INT_STATUS) |
			FIELD_PREP(MTK_WED_WDMA_OFST0_GLO_CFG,
				   MTK_WDMA_GLO_CFG));

		wed_w32(dev, MTK_WED_WDMA_OFFSET1,
			FIELD_PREP(MTK_WED_WDMA_OFST1_TX_CTRL,
				   MTK_WDMA_RING_TX(0)) |
			FIELD_PREP(MTK_WED_WDMA_OFST1_RX_CTRL,
				   MTK_WDMA_RING_RX(0)));
	}
}

static void
mtk_wed_rx_bm_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_RX_BM_RX_DMAD,
		FIELD_PREP(MTK_WED_RX_BM_RX_DMAD_SDL0,  dev->wlan.rx_size));

	wed_w32(dev, MTK_WED_RX_BM_BASE, dev->rx_buf_ring.desc_phys);

	wed_w32(dev, MTK_WED_RX_BM_INIT_PTR, MTK_WED_RX_BM_INIT_SW_TAIL |
		FIELD_PREP(MTK_WED_RX_BM_SW_TAIL, dev->wlan.rx_npkt));

	wed_w32(dev, MTK_WED_RX_BM_DYN_ALLOC_TH,
		FIELD_PREP(MTK_WED_RX_BM_DYN_ALLOC_TH_H, 0xffff));

	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_BM_EN);
}

static void
mtk_wed_rro_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_RROQM_MIOD_CFG,
		FIELD_PREP(MTK_WED_RROQM_MIOD_MID_DW, 0x70 >> 2) | //mid_size = mod_size + max_rxd_size
		FIELD_PREP(MTK_WED_RROQM_MIOD_MOD_DW, 0x10 >> 2) | //mod_size
		FIELD_PREP(MTK_WED_RROQM_MIOD_ENTRY_DW,  //miod_entry_size
			   MTK_WED_MIOD_ENTRY_CNT >> 2));

	wed_w32(dev, MTK_WED_RROQM_MIOD_CTRL0, dev->rro.miod_desc_phys);

	wed_w32(dev, MTK_WED_RROQM_MIOD_CTRL1,
		FIELD_PREP(MTK_WED_RROQM_MIOD_CNT, MTK_WED_MIOD_CNT));

	wed_w32(dev, MTK_WED_RROQM_FDBK_CTRL0, dev->rro.fdbk_desc_phys);

	wed_w32(dev, MTK_WED_RROQM_FDBK_CTRL1,
		FIELD_PREP(MTK_WED_RROQM_FDBK_CNT, MTK_WED_FB_CMD_CNT));

	wed_w32(dev, MTK_WED_RROQM_FDBK_CTRL2, 0);

	wed_w32(dev, MTK_WED_RROQ_BASE_L, dev->rro.rro_ring.desc_phys);

	wed_set(dev, MTK_WED_RROQM_RST_IDX,
		MTK_WED_RROQM_RST_IDX_MIOD |
		MTK_WED_RROQM_RST_IDX_FDBK);

	wed_w32(dev, MTK_WED_RROQM_RST_IDX, 0);

	wed_w32(dev, MTK_WED_RROQM_MIOD_CTRL2, MTK_WED_MIOD_CNT -1);

	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_RRO_QM_EN);
}

static void
mtk_wed_route_qm_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_RESET, MTK_WED_RESET_RX_ROUTE_QM);

	do {
		udelay(100);

		if (!(wed_r32(dev, MTK_WED_RESET) & MTK_WED_RESET_RX_ROUTE_QM))
			break;
	} while (1);

	/* configure RX_ROUTE_QM */
	wed_clr(dev, MTK_WED_RTQM_GLO_CFG, MTK_WED_RTQM_Q_RST);
	wed_clr(dev, MTK_WED_RTQM_GLO_CFG, MTK_WED_RTQM_TXDMAD_FPORT);
	wed_set(dev, MTK_WED_RTQM_GLO_CFG,
		FIELD_PREP(MTK_WED_RTQM_TXDMAD_FPORT, 0x3 + dev->hw->index));
	wed_clr(dev, MTK_WED_RTQM_GLO_CFG, MTK_WED_RTQM_Q_RST);

	/* enable RX_ROUTE_QM */
	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_ROUTE_QM_EN);
}

static void
mtk_wed_tx_hw_init(struct mtk_wed_device *dev)
{
	int size = dev->buf_ring.size;
	int rev_size = MTK_WED_TX_RING_SIZE / 2;
	int thr = 1;

	if (dev->ver > MTK_WED_V1) {
		size = MTK_WED_WDMA_RING_SIZE * ARRAY_SIZE(dev->tx_wdma) +
		       dev->buf_ring.size;
		rev_size = size;
		thr = 0;
	}

	wed_w32(dev, MTK_WED_TX_BM_CTRL,
		MTK_WED_TX_BM_CTRL_PAUSE |
		FIELD_PREP(MTK_WED_TX_BM_CTRL_VLD_GRP_NUM, size / 128) |
		FIELD_PREP(MTK_WED_TX_BM_CTRL_RSV_GRP_NUM, rev_size / 128));

	wed_w32(dev, MTK_WED_TX_BM_BASE, dev->buf_ring.desc_phys);

	wed_w32(dev, MTK_WED_TX_BM_TKID,
		FIELD_PREP(MTK_WED_TX_BM_TKID_START,
			   dev->wlan.token_start) |
		FIELD_PREP(MTK_WED_TX_BM_TKID_END,
			   dev->wlan.token_start + dev->wlan.nbuf - 1));

	wed_w32(dev, MTK_WED_TX_BM_BUF_LEN, MTK_WED_PKT_SIZE);

	wed_w32(dev, MTK_WED_TX_BM_DYN_THR,
		FIELD_PREP(MTK_WED_TX_BM_DYN_THR_LO, thr) |
		MTK_WED_TX_BM_DYN_THR_HI);

	if (dev->ver > MTK_WED_V1) {
		wed_w32(dev, MTK_WED_TX_TKID_CTRL,
			MTK_WED_TX_TKID_CTRL_PAUSE |
			FIELD_PREP(MTK_WED_TX_TKID_CTRL_VLD_GRP_NUM,
				   dev->buf_ring.size / 128) |
			FIELD_PREP(MTK_WED_TX_TKID_CTRL_RSV_GRP_NUM,
				   dev->buf_ring.size / 128));
		wed_w32(dev, MTK_WED_TX_TKID_DYN_THR,
			FIELD_PREP(MTK_WED_TX_TKID_DYN_THR_LO, 0) |
			MTK_WED_TX_TKID_DYN_THR_HI);
	}
	mtk_wed_reset(dev, MTK_WED_RESET_TX_BM);

	wed_clr(dev, MTK_WED_TX_BM_CTRL, MTK_WED_TX_BM_CTRL_PAUSE);
	if (dev->ver > MTK_WED_V1)
		wed_clr(dev, MTK_WED_TX_TKID_CTRL, MTK_WED_TX_TKID_CTRL_PAUSE);

	//Enable agent for BM about MTK_WED_CTRL_WED_TX_BM_EN and MTK_WED_CTRL_WED_TX_FREE_AGENT_EN
        //is moved to mtk_wed_set_int()
}

static void
mtk_wed_rx_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX,
		MTK_WED_WPDMA_RX_D_RST_CRX_IDX |
		MTK_WED_WPDMA_RX_D_RST_DRV_IDX);

	wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX, 0);

	mtk_wed_rx_bm_hw_init(dev);
	mtk_wed_rro_hw_init(dev);
	mtk_wed_route_qm_hw_init(dev);
}

static void
mtk_wed_hw_init(struct mtk_wed_device *dev)
{
	if (dev->init_done)
		return;

	dev->init_done = true;
	mtk_wed_set_ext_int(dev, false);
	mtk_wed_tx_hw_init(dev);
	if (dev->ver > MTK_WED_V1)
		mtk_wed_rx_hw_init(dev);
}

static void
mtk_wed_ring_reset(struct mtk_wdma_desc *desc, int size, int scale, bool tx)
{
	__le32 ctrl;
	int i;

	if (tx)
		ctrl = cpu_to_le32(MTK_WDMA_DESC_CTRL_DMA_DONE);
	else
		ctrl = cpu_to_le32(MTK_WFDMA_DESC_CTRL_TO_HOST);

	for (i = 0; i < size; i++) {
		desc->buf0 = 0;
		desc->ctrl = ctrl;
		desc->buf1 = 0;
		desc->info = 0;
		desc += scale;
	}
}

static u32
mtk_wed_check_busy(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
 	if (wed_r32(dev, reg) & mask)
		return true;

	return false;
}

static int
mtk_wed_poll_busy(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	int sleep = 1000;
	int timeout = 100 * sleep;
	u32 val;

	return read_poll_timeout(mtk_wed_check_busy, val, !val, sleep,
				 timeout, false, dev, reg, mask);
}

static void
mtk_wed_rx_reset(struct mtk_wed_device *dev)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;
	u8 state = WO_STATE_SER_RESET;
	bool busy = false;
	int i;

	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_CHANGE_STATE,
			     &state, sizeof(state), true);

	wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG, MTK_WED_WPDMA_RX_D_RX_DRV_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
				 MTK_WED_WPDMA_RX_D_RX_DRV_BUSY);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_INT_AGENT);
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_RX_D_DRV);
	} else {
		wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX,
			MTK_WED_WPDMA_RX_D_RST_CRX_IDX |
			MTK_WED_WPDMA_RX_D_RST_DRV_IDX);

		wed_set(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RST_INIT_COMPLETE |
			MTK_WED_WPDMA_RX_D_FSM_RETURN_IDLE);
		wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RST_INIT_COMPLETE |
			MTK_WED_WPDMA_RX_D_FSM_RETURN_IDLE);

		wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX, 0);
	}

	/* reset rro qm */
	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_RRO_QM_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_CTRL,
				 MTK_WED_CTRL_RX_RRO_QM_BUSY);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_RX_RRO_QM);
	} else {
		wed_set(dev, MTK_WED_RROQM_RST_IDX,
			MTK_WED_RROQM_RST_IDX_MIOD |
			MTK_WED_RROQM_RST_IDX_FDBK);
		wed_w32(dev, MTK_WED_RROQM_RST_IDX, 0);
	}

	/* reset route qm */
	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_ROUTE_QM_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_CTRL,
				 MTK_WED_CTRL_RX_ROUTE_QM_BUSY);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_RX_ROUTE_QM);
	} else {
		wed_set(dev, MTK_WED_RTQM_GLO_CFG,
			MTK_WED_RTQM_Q_RST);
	}

	/* reset tx wdma */
	mtk_wdma_tx_reset(dev);

	/* reset tx wdma drv */
	wed_clr(dev, MTK_WED_WDMA_GLO_CFG, MTK_WED_WDMA_GLO_CFG_TX_DRV_EN);
	mtk_wed_poll_busy(dev, MTK_WED_CTRL,
			  MTK_WED_CTRL_WDMA_INT_AGENT_BUSY);
	mtk_wed_reset(dev, MTK_WED_RESET_WDMA_TX_DRV);

	/* reset wed rx dma */
	busy = mtk_wed_poll_busy(dev, MTK_WED_GLO_CFG,
				 MTK_WED_GLO_CFG_RX_DMA_BUSY);
	wed_clr(dev, MTK_WED_GLO_CFG, MTK_WED_GLO_CFG_RX_DMA_EN);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WED_RX_DMA);
	} else {
		wed_set(dev, MTK_WED_RESET_IDX,
			MTK_WED_RESET_IDX_RX);
		wed_w32(dev, MTK_WED_RESET_IDX, 0);
	}

	/* reset rx bm */
	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_BM_EN);
	mtk_wed_poll_busy(dev, MTK_WED_CTRL,
			   MTK_WED_CTRL_WED_RX_BM_BUSY);
	mtk_wed_reset(dev, MTK_WED_RESET_RX_BM);

	/* wo change to enable state */
	state = WO_STATE_ENABLE;
	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_CHANGE_STATE,
			     &state, sizeof(state), true);

	/* wed_rx_ring_reset */
	for (i = 0; i < ARRAY_SIZE(dev->rx_ring); i++) {
		struct mtk_wdma_desc *desc = dev->rx_ring[i].desc;

		if (!desc)
			continue;

		mtk_wed_ring_reset(desc, MTK_WED_RX_RING_SIZE, 1, false);
	}

	mtk_wed_free_rx_bm(dev);
}


static void
mtk_wed_reset_dma(struct mtk_wed_device *dev)
{
	bool busy = false;
	u32 val;
	int i;

	for (i = 0; i < ARRAY_SIZE(dev->tx_ring); i++) {
		struct mtk_wdma_desc *desc = dev->tx_ring[i].desc;

		if (!desc)
			continue;

		mtk_wed_ring_reset(desc, MTK_WED_TX_RING_SIZE, 1, true);
	}

	/* 1.Reset WED Tx DMA */
	wed_clr(dev, MTK_WED_GLO_CFG, MTK_WED_GLO_CFG_TX_DMA_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_GLO_CFG, MTK_WED_GLO_CFG_TX_DMA_BUSY);

	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WED_TX_DMA);
	} else {
		wed_w32(dev, MTK_WED_RESET_IDX,
			MTK_WED_RESET_IDX_TX);
		wed_w32(dev, MTK_WED_RESET_IDX, 0);
	}

	/* 2. Reset WDMA Rx DMA/Driver_Engine */
	busy = !!mtk_wdma_rx_reset(dev);

	wed_clr(dev, MTK_WED_WDMA_GLO_CFG, MTK_WED_WDMA_GLO_CFG_RX_DRV_EN);
	busy = !!(busy ||
		  mtk_wed_poll_busy(dev, MTK_WED_WDMA_GLO_CFG,
					 MTK_WED_WDMA_GLO_CFG_RX_DRV_BUSY));

	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WDMA_INT_AGENT);
		mtk_wed_reset(dev, MTK_WED_RESET_WDMA_RX_DRV);
	} else {
		wed_w32(dev, MTK_WED_WDMA_RESET_IDX,
			MTK_WED_WDMA_RESET_IDX_RX | MTK_WED_WDMA_RESET_IDX_DRV);
		wed_w32(dev, MTK_WED_WDMA_RESET_IDX, 0);

		wed_set(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_RST_INIT_COMPLETE);

		wed_clr(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_RST_INIT_COMPLETE);
	}

	/* 3. Reset WED WPDMA Tx Driver Engine */
	wed_clr(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_WED_TX_FREE_AGENT_EN);

	//MTK_WED_CTRL_WPDMA_INT_AGENT_EN is cleared in propietary warp driver for mt6990.

	for (i = 0; i < 100; i++) {
		val = wed_r32(dev, MTK_WED_TX_BM_INTF);
		if (FIELD_GET(MTK_WED_TX_BM_INTF_TKFIFO_FDEP, val) == 0x40)
			break;
	}
	mtk_wed_reset(dev, MTK_WED_RESET_TX_FREE_AGENT);

	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_TX_BM_EN);
	mtk_wed_reset(dev, MTK_WED_RESET_TX_BM);

	/* 4. Reset WED WPDMA Tx Driver Engine */
	busy = mtk_wed_poll_busy(dev, MTK_WED_WPDMA_GLO_CFG,
				      MTK_WED_WPDMA_GLO_CFG_TX_DRV_BUSY);
	wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
		MTK_WED_WPDMA_GLO_CFG_TX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DRV_EN);

	busy = !!(busy ||
		  mtk_wed_poll_busy(dev, MTK_WED_WPDMA_GLO_CFG,
					 MTK_WED_WPDMA_GLO_CFG_RX_DRV_BUSY));
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_INT_AGENT);
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_TX_DRV);
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_RX_DRV);
	} else {
		wed_w32(dev, MTK_WED_WPDMA_RESET_IDX,
			MTK_WED_WPDMA_RESET_IDX_TX |
			MTK_WED_WPDMA_RESET_IDX_RX);
		//WED_WPDMA_RST_IDX_FLD_CTX_IDX0 and WED_WPDMA_RST_IDX_FLD_CTX_IDX1 will be written
                //in propietary warp driver for mt6990.

		wed_w32(dev, MTK_WED_WPDMA_RESET_IDX, 0);
		if (dev->ver > MTK_WED_V1) {
			wed_w32(dev, MTK_WED_RESET_IDX,
				MTK_WED_RESET_WPDMA_IDX_RX);
			wed_w32(dev, MTK_WED_RESET_IDX, 0);
		}
	}

	if (dev->ver > MTK_WED_V1) {
		dev->init_done = false;
		mtk_wed_rx_reset(dev);
	}

}

static int
mtk_wed_rro_ring_alloc(struct mtk_wed_device *dev, struct mtk_wed_ring *ring,
		   int size)
{
	ring->desc = dma_alloc_coherent(dev->hw->dev,
					size * sizeof(*ring->desc),
					&ring->desc_phys, GFP_KERNEL);
	if (!ring->desc)
		return -ENOMEM;

	ring->size = size;
	memset(ring->desc, 0, size);
	return 0;
}

static int
mtk_wed_ring_alloc(struct mtk_wed_device *dev, struct mtk_wed_ring *ring,
		   int size, int scale, bool tx)
{
	ring->desc = dma_alloc_coherent(dev->hw->dev,
					size * sizeof(*ring->desc) * scale,
					&ring->desc_phys, GFP_KERNEL);
	if (!ring->desc)
		return -ENOMEM;

	ring->size = size;
	mtk_wed_ring_reset(ring->desc, size, scale, tx);

	return 0;
}

static int
mtk_wed_wdma_rx_ring_setup(struct mtk_wed_device *dev,
		int idx, int size, bool reset)
{
	struct mtk_wed_ring *wdma = &dev->tx_wdma[idx];

	if(!reset)
		if (mtk_wed_ring_alloc(dev, wdma, MTK_WED_WDMA_RING_SIZE,
				       dev->ver, true))
			return -ENOMEM;

	wdma_w32(dev, MTK_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_BASE,
		 wdma->desc_phys);
	wdma_w32(dev, MTK_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_COUNT,
		 size);
	wdma_w32(dev, MTK_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_CPU_IDX, 0);

	wed_w32(dev, MTK_WED_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_BASE,
		wdma->desc_phys);
	wed_w32(dev, MTK_WED_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_COUNT,
		size);

	return 0;
}

static int
mtk_wed_wdma_tx_ring_setup(struct mtk_wed_device *dev,
	int idx, int size, bool reset)
{
	struct mtk_wed_ring *wdma = &dev->rx_wdma[idx];

	if (!reset)
		if (mtk_wed_ring_alloc(dev, wdma, MTK_WED_WDMA_RING_SIZE,
				       dev->ver, true))
			return -ENOMEM;

	wdma_w32(dev, MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_BASE,
		 wdma->desc_phys);
	wdma_w32(dev, MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_COUNT,
		 size);
	wdma_w32(dev,
		 MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_CPU_IDX, 0);
	wdma_w32(dev,
		 MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_DMA_IDX, 0);
	if (reset)
		mtk_wed_ring_reset(wdma->desc, MTK_WED_WDMA_RING_SIZE,
				   dev->ver, true);
	if (idx == 0)  {
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_BASE, wdma->desc_phys);
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_COUNT, size);
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_CPU_IDX, 0);
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_DMA_IDX, 0);
	}

	return 0;
}

static int
mtk_wed_rro_alloc(struct mtk_wed_device *dev)
{
	struct device_node *np, *node = dev->hw->node;
	struct mtk_wed_ring *ring;
	struct resource res;
	int ret;

	np = of_parse_phandle(node, "mediatek,wocpu_dlm", 0);
	if (!np)
		return -ENODEV;

	ret = of_address_to_resource(np, 0, &res);
	if (ret)
		return ret;

	dev->rro.rro_desc = ioremap(res.start, resource_size(&res));

	ring = &dev->rro.rro_ring;

	dev->rro.miod_desc_phys = res.start;

	dev->rro.mcu_view_miod = MTK_WED_WOCPU_VIEW_MIOD_BASE;
	dev->rro.fdbk_desc_phys = MTK_WED_MIOD_ENTRY_CNT * MTK_WED_MIOD_CNT
				  + dev->rro.miod_desc_phys;

	if (mtk_wed_rro_ring_alloc(dev, ring, MTK_WED_RRO_QUE_CNT))
		return -ENOMEM;

	return 0;
}

static int
mtk_wed_rro_cfg(struct mtk_wed_device *dev)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;
	struct {
		struct wo_cmd_ring ring[2];

		u32 wed;
		u8 ver;
	} req = {
		.ring = {
			[0] = {
				.q_base = dev->rro.mcu_view_miod,
				.cnt = MTK_WED_MIOD_CNT,
				.unit = MTK_WED_MIOD_ENTRY_CNT,
			},
			[1] = {
				.q_base = dev->rro.mcu_view_miod +
					  MTK_WED_MIOD_ENTRY_CNT *
					  MTK_WED_MIOD_CNT,
				.cnt = MTK_WED_FB_CMD_CNT,
				.unit = 4,
			},
		},
		.wed = 0,
	};

	return mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_WED_CFG,
				    &req, sizeof(req), true);
}

static int
mtk_wed_send_msg(struct mtk_wed_device *dev, int cmd_id, void *data, int len)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;

	if (dev->ver == MTK_WED_V1)
		return 0;

	return mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, cmd_id, data, len, true);
}

#ifdef CONFIG_MTK_PPE
static void
mtk_wed_ppe_check(struct mtk_wed_device *dev, struct sk_buff *skb,
			u32 reason, u32 hash)
{
	int idx = dev->hw->index;
	struct mtk_eth *eth = dev->hw->eth;
	struct ethhdr *eh;

	if (reason == MTK_PPE_CPU_REASON_HIT_UNBIND_RATE_REACHED) {
		if (!skb)
			return;

		skb_set_mac_header(skb, 0);
		eh = eth_hdr(skb);
		skb->protocol = eh->h_proto;
		mtk_ppe_check_skb(nf_ppe.ppe[idx], skb, hash);
	}
}
#endif

static void
mtk_wed_start(struct mtk_wed_device *dev, u32 irq_mask)
{
	int i, ret;

	if (dev->ver > MTK_WED_V1)
		ret = mtk_wed_rx_bm_alloc(dev);

	for (i = 0; i < ARRAY_SIZE(dev->tx_wdma); i++)
		if (!dev->tx_wdma[i].desc)
			mtk_wed_wdma_rx_ring_setup(dev, i, 16, false);

	mtk_wed_hw_init(dev);

	mtk_wed_set_int(dev, irq_mask);
	mtk_wed_set_ext_int(dev, true);

	if (dev->ver == MTK_WED_V1) {
		u32 val;

		val = dev->wlan.wpdma_phys |
		      MTK_PCIE_MIRROR_MAP_EN |
		      FIELD_PREP(MTK_PCIE_MIRROR_MAP_WED_ID, dev->hw->index);

		if (dev->hw->index)
			val |= BIT(1);
		val |= BIT(0);
		regmap_write(dev->hw->mirror, dev->hw->index * 4, val);
	} else {
		/* driver set mid ready and only once */
		wed_w32(dev, MTK_WED_EXT_INT_MASK1,
			MTK_WED_EXT_INT_STATUS_WPDMA_MID_RDY);
		wed_w32(dev, MTK_WED_EXT_INT_MASK2,
			MTK_WED_EXT_INT_STATUS_WPDMA_MID_RDY);
		wed_w32(dev, MTK_WED_EXT_INT_MASK3,
			MTK_WED_EXT_INT_STATUS_WPDMA_MID_RDY);

		wed_r32(dev, MTK_WED_EXT_INT_MASK1);
		wed_r32(dev, MTK_WED_EXT_INT_MASK2);
		wed_r32(dev, MTK_WED_EXT_INT_MASK3);

		ret = mtk_wed_rro_cfg(dev);
		if (ret)
			return;
	}
	mtk_wed_set_512_support(dev, dev->wlan.wcid_512);

	mtk_wed_dma_enable(dev);
	dev->running = true;
}

static int
mtk_wed_attach(struct mtk_wed_device *dev)
	__releases(RCU)
{
	struct mtk_wed_hw *hw;
	struct device *device;
	u16 ver;
	int ret = 0;

	RCU_LOCKDEP_WARN(!rcu_read_lock_held(),
			 "mtk_wed_attach without holding the RCU read lock");

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	rcu_read_unlock();

	mutex_lock(&hw_lock);

	hw = mtk_wed_assign(dev);
	if (!hw) {
		module_put(THIS_MODULE);
		ret = -ENODEV;
		goto out;
	}

	device = dev->wlan.bus_type == MTK_WED_BUS_PCIE
	? &dev->wlan.pci_dev->dev
	: &dev->wlan.platform_dev->dev;
	dev_info(device, "attaching wed device %d version %d\n",
		 hw->index, hw->ver);

	dev->hw = hw;
	dev->dev = hw->dev;
	dev->irq = hw->irq;
	dev->wdma_idx = hw->index;

//	there is no mtk_eth_set_dma_device() in ethernet code base
//	if (hw->eth->dma_dev == hw->eth->dev &&
//	    of_dma_is_coherent(hw->eth->dev->of_node))
//		mtk_eth_set_dma_device(hw->eth, hw->dev);

	dev->ver = FIELD_GET(MTK_WED_REV_ID_MAJOR,
			    wed_r32(dev, MTK_WED_REV_ID));
	if (dev->ver > MTK_WED_V1) {
		ver = FIELD_GET(MTK_WED_REV_ID_MINOR,
			    wed_r32(dev, MTK_WED_REV_ID));
		mtk_wed_wo_set_apsrc_idle(dev->hw);
	}

	dev->rev_id = ((dev->ver << 28) | ver << 16);

	ret = mtk_wed_buffer_alloc(dev);
	if (ret)
		goto error;

	if (dev->ver > MTK_WED_V1) {
		ret = mtk_wed_rro_alloc(dev);
		if (ret)
			goto error;
	}

	mtk_wed_hw_init_early(dev);

	init_completion(&dev->fe_reset_done);
	init_completion(&dev->wlan_reset_done);
	atomic_set(&dev->fe_reset, 0);

	if (dev->ver == MTK_WED_V1)
		regmap_update_bits(hw->hifsys, HIFSYS_DMA_AG_MAP,
				   BIT(hw->index), 0);
	else
		ret = mtk_wed_wo_init(hw);

error:
	if (ret)
		mtk_wed_detach(dev);
out:
	mutex_unlock(&hw_lock);

	return ret;
}

static int
mtk_wed_tx_ring_setup(struct mtk_wed_device *dev, int idx,
		void __iomem *regs, bool reset)
{
	struct mtk_wed_ring *ring = &dev->tx_ring[idx];

	/*
	 * Tx ring redirection:
	 * Instead of configuring the WLAN PDMA TX ring directly, the WLAN
	 * driver allocated DMA ring gets configured into WED MTK_WED_RING_TX(n)
	 * registers.
	 *
	 * WED driver posts its own DMA ring as WLAN PDMA TX and configures it
	 * into MTK_WED_WPDMA_RING_TX(n) registers.
	 * It gets filled with packets picked up from WED TX ring and from
	 * WDMA RX.
	 */

	BUG_ON(idx > ARRAY_SIZE(dev->tx_ring));

	if (!reset)
		if (mtk_wed_ring_alloc(dev, ring, MTK_WED_TX_RING_SIZE,
				       1, true))
			return -ENOMEM;

	if (mtk_wed_wdma_rx_ring_setup(dev, idx, MTK_WED_WDMA_RING_SIZE, reset))
		return -ENOMEM;

	ring->reg_base = MTK_WED_RING_TX(idx);
	ring->wpdma = regs;

	/* WED -> WPDMA */
	wpdma_tx_w32(dev, idx, MTK_WED_RING_OFS_BASE, ring->desc_phys);
	wpdma_tx_w32(dev, idx, MTK_WED_RING_OFS_COUNT, MTK_WED_TX_RING_SIZE);
	wpdma_tx_w32(dev, idx, MTK_WED_RING_OFS_CPU_IDX, 0);

	wed_w32(dev, MTK_WED_WPDMA_RING_TX(idx) + MTK_WED_RING_OFS_BASE,
		ring->desc_phys);
	wed_w32(dev, MTK_WED_WPDMA_RING_TX(idx) + MTK_WED_RING_OFS_COUNT,
		MTK_WED_TX_RING_SIZE);
	wed_w32(dev, MTK_WED_WPDMA_RING_TX(idx) + MTK_WED_RING_OFS_CPU_IDX, 0);

	return 0;
}

static int
mtk_wed_txfree_ring_setup(struct mtk_wed_device *dev, void __iomem *regs)
{
	struct mtk_wed_ring *ring = &dev->txfree_ring;
	int i, idx = 1;

	if(dev->ver > MTK_WED_V1)
		idx = 0;

	/*
	 * For txfree event handling, the same DMA ring is shared between WED
	 * and WLAN. The WLAN driver accesses the ring index registers through
	 * WED
	 */
	ring->reg_base = MTK_WED_RING_RX(idx);
	ring->wpdma = regs;

	for (i = 0; i < 12; i += 4) {
		u32 val = readl(regs + i);

		wed_w32(dev, MTK_WED_RING_RX(idx) + i, val);
		wed_w32(dev, MTK_WED_WPDMA_RING_RX(idx) + i, val);
	}

	return 0;
}

static int
mtk_wed_rx_ring_setup(struct mtk_wed_device *dev,
		int idx, void __iomem *regs, bool reset)
{
	struct mtk_wed_ring *ring = &dev->rx_ring[idx];

	BUG_ON(idx > ARRAY_SIZE(dev->rx_ring));

	if (!reset)
		if (mtk_wed_ring_alloc(dev, ring, MTK_WED_RX_RING_SIZE,
				       1, false))
			return -ENOMEM;

	if (mtk_wed_wdma_tx_ring_setup(dev, idx, MTK_WED_WDMA_RING_SIZE, reset))
		return -ENOMEM;

	ring->reg_base = MTK_WED_RING_RX_DATA(idx);
	ring->wpdma = regs;
	dev->hw->ring_num = idx + 1;

	/* WPDMA ->  WED */
	wpdma_rx_w32(dev, idx, MTK_WED_RING_OFS_BASE, ring->desc_phys);
	wpdma_rx_w32(dev, idx, MTK_WED_RING_OFS_COUNT, MTK_WED_RX_RING_SIZE);
	//wpdma_rx cpu idx is mapped for propietary warp driver in mt6990

	wed_w32(dev, MTK_WED_WPDMA_RING_RX_DATA(idx) + MTK_WED_RING_OFS_BASE,
		ring->desc_phys);
	wed_w32(dev, MTK_WED_WPDMA_RING_RX_DATA(idx) + MTK_WED_RING_OFS_COUNT,
		MTK_WED_RX_RING_SIZE);

	return 0;
}

static u32
mtk_wed_irq_get(struct mtk_wed_device *dev, u32 mask)
{
	u32 val;

	val = wed_r32(dev, MTK_WED_EXT_INT_STATUS);
	wed_w32(dev, MTK_WED_EXT_INT_STATUS, val);
	val &= MTK_WED_EXT_INT_STATUS_ERROR_MASK;
	WARN_RATELIMIT(val, "mtk_wed%d: error status=%08x\n",
		       dev->hw->index, val);

	val = wed_r32(dev, MTK_WED_INT_STATUS);
	val &= mask;
	wed_w32(dev, MTK_WED_INT_STATUS, val); /* ACK */

	return val;
}

static void
mtk_wed_irq_set_mask(struct mtk_wed_device *dev, u32 mask)
{
	if (!dev->running)
		return;

	mtk_wed_set_ext_int(dev, !!mask);
	wed_w32(dev, MTK_WED_INT_MASK, mask);
}

int mtk_wed_flow_add(int index)
{
	struct mtk_wed_hw *hw = hw_list[index];
	int ret;

	if (!hw || !hw->wed_dev)
		return -ENODEV;

	if (hw->num_flows) {
		hw->num_flows++;
		return 0;
	}

	mutex_lock(&hw_lock);
	if (!hw->wed_dev) {
		ret = -ENODEV;
		goto out;
	}

	ret = hw->wed_dev->wlan.offload_enable(hw->wed_dev);
	if (!ret)
		hw->num_flows++;
	mtk_wed_set_ext_int(hw->wed_dev, true);

out:
	mutex_unlock(&hw_lock);

	return ret;
}

void mtk_wed_flow_remove(int index)
{
	struct mtk_wed_hw *hw = hw_list[index];

	if (!hw)
		return;

	if (--hw->num_flows)
		return;

	mutex_lock(&hw_lock);
	if (!hw->wed_dev)
		goto out;

	hw->wed_dev->wlan.offload_disable(hw->wed_dev);
	mtk_wed_set_ext_int(hw->wed_dev, true);

out:
	mutex_unlock(&hw_lock);
}

#ifdef CONFIG_MTK_PPE
static int mtk_wed_eth_setup_tc(struct mtk_wed_device *wed, struct net_device *dev,
		int type, void *type_data)
{
	return mtk_netdev_setup_tc(dev, type, type_data);
}
#endif

void mtk_wed_add_hw(struct device_node *np, struct mtk_eth *eth,
			void __iomem *wdma, u32 wdma_phy, int index)

{
	static const struct mtk_wed_ops wed_ops = {
		.attach = mtk_wed_attach,
		.tx_ring_setup = mtk_wed_tx_ring_setup,
		.txfree_ring_setup = mtk_wed_txfree_ring_setup,
		.rx_ring_setup = mtk_wed_rx_ring_setup,
		.msg_update = mtk_wed_send_msg,
		.start = mtk_wed_start,
		.stop = mtk_wed_stop,
		.reset_dma = mtk_wed_reset_dma,
		.reg_read = wed_r32,
		.reg_write = wed_w32,
		.irq_get = mtk_wed_irq_get,
		.irq_set_mask = mtk_wed_irq_set_mask,
		.detach = mtk_wed_detach,
#ifdef CONFIG_MTK_PPE
		.setup_tc = mtk_wed_eth_setup_tc,
		.ppe_check = mtk_wed_ppe_check,
#endif
	};
	struct device_node *eth_np = eth->dev->of_node;
	struct platform_device *pdev;
	struct mtk_wed_hw *hw;
	struct regmap *regs;
	int irq;


	if (!np)
		return;

	pdev = of_find_device_by_node(np);
	if (!pdev)
		return;

	get_device(&pdev->dev);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return;

	regs = syscon_regmap_lookup_by_phandle(np, NULL);
	if (!regs)
		return;

	rcu_assign_pointer(mtk_soc_wed_ops, &wed_ops);

	mutex_lock(&hw_lock);

	if (WARN_ON(hw_list[index]))
		goto unlock;

	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	hw->node = np;
	hw->regs = regs;
	hw->eth = eth;
	hw->dev = &pdev->dev;
	hw->wdma = wdma;
	hw->wdma_phy = wdma_phy;
	hw->index = index;
	hw->irq = irq;

	if (!CONFIG_MEDIATEK_WED_V2) {
		hw->mirror = syscon_regmap_lookup_by_phandle(eth_np,
							     "mediatek,pcie-mirror");
		hw->hifsys = syscon_regmap_lookup_by_phandle(eth_np,
							     "mediatek,hifsys");

		if (IS_ERR(hw->mirror) || IS_ERR(hw->hifsys)) {
			kfree(hw);
			goto unlock;
		}

		if (!index) {
			regmap_write(hw->mirror, 0, 0);
			regmap_write(hw->mirror, 4, 0);
		}

		hw->ver = MTK_WED_V1;
	} else {
		hw->ver = MTK_WED_V2;
	}

	mtk_wed_hw_add_debugfs(hw);

	hw_list[index] = hw;

unlock:
	mutex_unlock(&hw_lock);
}

void mtk_wed_exit(void)
{
	int i;

	rcu_assign_pointer(mtk_soc_wed_ops, NULL);

	synchronize_rcu();

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw;

		hw = hw_list[i];
		if (!hw)
			continue;

		hw_list[i] = NULL;
		debugfs_remove(hw->debugfs_dir);
		put_device(hw->dev);
		kfree(hw);
	}
}
