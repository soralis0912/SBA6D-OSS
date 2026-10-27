// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek RSS
 *
 * Author: FangRu Wang <fangju.wang@mediatek.com>
 *
 */
#include <linux/of_device.h>
#include <linux/of_net.h>
#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"

static inline void mtk_rx_irq_disable0(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK0);
	mtk_w32(eth, val & ~mask, MTK_PDMA_INT_MASK0);
}

static inline void mtk_rx_irq_enable0(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK0);
	mtk_w32(eth, val | mask, MTK_PDMA_INT_MASK0);
}

static inline void mtk_rx_irq_disable1(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK1);
	mtk_w32(eth, val & ~mask, MTK_PDMA_INT_MASK1);
}

static inline void mtk_rx_irq_enable1(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK1);
	mtk_w32(eth, val | mask, MTK_PDMA_INT_MASK1);
}

static inline void mtk_rx_irq_disable2(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK2);
	mtk_w32(eth, val & ~mask, MTK_PDMA_INT_MASK2);
}

static inline void mtk_rx_irq_enable2(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK2);
	mtk_w32(eth, val | mask, MTK_PDMA_INT_MASK2);
}

static inline void mtk_rx_irq_disable3(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK3);
	mtk_w32(eth, val & ~mask, MTK_PDMA_INT_MASK3);
}


static inline void mtk_rx_irq_enable3(struct mtk_eth *eth, u32 mask)
{
	u32 val;

	val = mtk_r32(eth, MTK_PDMA_INT_MASK3);
	mtk_w32(eth, val | mask, MTK_PDMA_INT_MASK3);
}


struct mtk_rx_ring *mtk_get_rx_ring_rss(struct mtk_eth *eth, struct napi_struct *napi)
{
	struct rss_napi *rssnapi = container_of(napi, struct rss_napi, rx_napi);

	if (!eth->hwrss)
		return &eth->rx_ring[0];

	return &eth->rx_ring[rssnapi->idx];
}

bool mtk_is_rss_ring(struct mtk_eth *eth, struct napi_struct *napi)
{

#if defined(CONFIG_MTK_ADMAV2)
		struct rss_napi *rssnapi = container_of(napi, struct rss_napi, rx_napi);
#endif
	if (!eth->hwrss)
		return false;

#if defined(CONFIG_MTK_ADMAV2)
		if (rssnapi == NULL)
				return false;
		if (eth->hwrss) {
			if (rssnapi->idx > 0 && rssnapi->idx <= 3 )
				return true;
			else
				return false;
		} else
			return false;
#endif

	return true;
}

void mtk_update_rx_cpu_idx_rss(struct mtk_eth *eth, struct mtk_rx_ring *ring)
{
	if ((ring != NULL) && ring->calc_idx_update) {
		ring->calc_idx_update = false;
		mtk_w32(eth, ring->calc_idx, ring->crx_idx_reg);
	}
}

int mtk_napi_rx_ext(struct napi_struct *napi, int budget)
{
	struct rss_napi *rssnapi = container_of(napi, struct rss_napi, rx_napi);
	struct mtk_eth *eth;
	u32 status, mask;
	int rx_done = 0;
	int remain_budget = budget;

	u32 val_bit, status_reg, mask_reg, val_bit2;

	if (rssnapi == NULL) {
		pr_info("Error : %s idx: not found", __func__);
		return 0;
	}

	eth = rssnapi->eth;
	val_bit = rssnapi->val_bit;
	status_reg = rssnapi->status_reg;
	mask_reg = rssnapi->mask_reg;
	val_bit2 = rssnapi->val_bit2;

	mtk_handle_status_irq(eth);

poll_again:
	mtk_w32(eth, val_bit, status_reg);
	rx_done = mtk_poll_rx(napi, remain_budget, eth);
	if (unlikely(netif_msg_intr(eth))) {
		status = mtk_r32(eth, status_reg);
		mask = mtk_r32(eth, mask_reg);
		dev_info(eth->dev,
			 "done rx %d, intr 0x%08x/0x%x\n",
			 rx_done, status, mask);
	}
	if (rx_done == remain_budget)
		return budget;

	status = mtk_r32(eth, status_reg);
	if (status & (val_bit)) {
		remain_budget -= rx_done;
		goto poll_again;
	}
	napi_complete(napi);

	if (rssnapi->idx == 0)
		mtk_rx_irq_enable0(eth, val_bit2);
	else if (rssnapi->idx == 1)
		mtk_rx_irq_enable1(eth, val_bit2);
	else if (rssnapi->idx == 2)
		mtk_rx_irq_enable2(eth, val_bit2);
	else if (rssnapi->idx == 3)
		mtk_rx_irq_enable3(eth, val_bit2);
	else
		mtk_rx_irq_enable(eth, val_bit2);

	return rx_done + budget - remain_budget;
}

irqreturn_t mtk_handle_irq_rrs(int irq, void *_eth)
{
	struct mtk_eth *eth = _eth;

	if (irq == eth->irq[2]) {
		if (likely(napi_schedule_prep(&eth->rx_rss_napi[0].rx_napi))) {
			__napi_schedule(&eth->rx_rss_napi[0].rx_napi);
#if defined(CONFIG_MTK_ADMAV2)
			mtk_rx_irq_disable0(eth, MTK_RX_DONE_DLY0 | BIT(28) |BIT(29)|BIT(30)|BIT(31));
#else
			mtk_rx_irq_disable0(eth, MTK_RX_DONE_DLY);
#endif
		}
	} else if (irq == eth->irq[3]) {
		if (likely(napi_schedule_prep(&eth->rx_rss_napi[1].rx_napi))) {
			__napi_schedule(&eth->rx_rss_napi[1].rx_napi);
			mtk_rx_irq_disable1(eth, MTK_RX_DONE_DLY1);
		}
	} else if (irq == eth->irq[4]) {
		if (likely(napi_schedule_prep(&eth->rx_rss_napi[2].rx_napi))) {
			__napi_schedule(&eth->rx_rss_napi[2].rx_napi);
			mtk_rx_irq_disable2(eth, MTK_RX_DONE_DLY2);
		}
	} else if (irq == eth->irq[5]) {
		if (likely(napi_schedule_prep(&eth->rx_rss_napi[3].rx_napi))) {
			__napi_schedule(&eth->rx_rss_napi[3].rx_napi);
			mtk_rx_irq_disable3(eth, MTK_RX_DONE_DLY3);
		}
	} else
		pr_info("Error: %s = %d", __func__, irq);

	return IRQ_HANDLED;
}

void mtk_rx_irq_enable_rss_all(struct mtk_eth *eth)
{
#if defined(CONFIG_MTK_ADMAV2)
	mtk_rx_irq_enable0(eth, MTK_RX_DONE_DLY0 | BIT(28) |BIT(29)|BIT(30)|BIT(31));
#else
	mtk_rx_irq_enable0(eth, MTK_RX_DONE_DLY);
#endif
	mtk_rx_irq_enable1(eth, MTK_RX_DONE_DLY1);
	mtk_rx_irq_enable2(eth, MTK_RX_DONE_DLY2);
	mtk_rx_irq_enable3(eth, MTK_RX_DONE_DLY3);
}

void mtk_rx_irq_disable_rss_all(struct mtk_eth *eth)
{
#if defined(CONFIG_MTK_ADMAV2)
	mtk_rx_irq_disable0(eth, MTK_RX_DONE_DLY0 | BIT(28) |BIT(29)|BIT(30)|BIT(31));
#else
	mtk_rx_irq_disable0(eth, MTK_RX_DONE_DLY);
#endif
	mtk_rx_irq_disable1(eth, MTK_RX_DONE_DLY1);
	mtk_rx_irq_disable2(eth, MTK_RX_DONE_DLY2);
	mtk_rx_irq_disable3(eth, MTK_RX_DONE_DLY3);
}

int mtk_hwrss_rx_init(struct mtk_eth *eth)
{
	u32 reg_ctrl = 0;
	//ADMAv2 LRO is ring 4~7, RSS ring 0~3 no pse mode
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2)) {
	/* 1. Set RX ring1~3 to pse modes */
	//SET_PDMA_RXRING_MODE(ADMA_RX_RING1, PDMA_RX_PSE_MODE);
	//SET_PDMA_RXRING_MODE(ADMA_RX_RING2, PDMA_RX_PSE_MODE);
	//SET_PDMA_RXRING_MODE(ADMA_RX_RING3, PDMA_RX_PSE_MODE);
	reg_ctrl = mtk_r32(eth, LRO_RX_RING1_CTRL_DW2);
	reg_ctrl &= ~(0x3 << MTK_RX_MODE_OFFSET);
	mtk_w32(eth, reg_ctrl | MTK_RING_PSE_MODE, LRO_RX_RING1_CTRL_DW2);
	mtk_w32(eth, reg_ctrl | MTK_RING_PSE_MODE, LRO_RX_RING2_CTRL_DW2);
	mtk_w32(eth, reg_ctrl | MTK_RING_PSE_MODE, LRO_RX_RING3_CTRL_DW2);

	/* 2. Enable non-lro multiple rx */
	//SET_PDMA_NON_LRO_MULTI_EN(1);  /* MRX EN */
	reg_ctrl = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0);
	mtk_w32(eth, reg_ctrl | MTK_NON_LRO_MULTI_EN | MTK_LRO_DLY_INT_EN, MTK_PDMA_LRO_CTRL_DW0);
	}

	/*Hash Type*/
	//SET_PDMA_RSS_IPV4_TYPE(7);
	//SET_PDMA_RSS_IPV6_TYPE(7);

	reg_ctrl = mtk_r32(eth, MTK_RSS_GLO_CFG);
	reg_ctrl &= ~(MTK_RSS_IPV4_TYPE);
	reg_ctrl &= ~(MTK_RSS_IPV6_TYPE);
	reg_ctrl |= ((7) & 0x7) << MTK_RSS_IPV4_TYPE_OFFSET;
	reg_ctrl |= ((7) & 0x7) << MTK_RSS_IPV6_TYPE_OFFSET;
	mtk_w32(eth, reg_ctrl, MTK_RSS_GLO_CFG);

	/* 3. Select the size of indirection table */
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW0);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW1);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW2);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW3);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW4);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW5);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW6);
	mtk_w32(eth, 0x39393939, MTK_RSS_INDR_TABLE_DW7);
#if (0)
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW0);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW1);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW2);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW3);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW4);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW5);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW6);
	mtk_w32(eth, 0x44444444, MTK_RSS_INDR_TABLE_DW7);
#endif
#if (1)
	/* 4. Pause */
	//SET_PDMA_RSS_CFG_REQ(1);
	reg_ctrl = mtk_r32(eth, MTK_RSS_GLO_CFG);
	reg_ctrl &= ~(MTK_RSS_CFG_REQ);
	reg_ctrl |= ((1) & 0x1) << MTK_RSS_CFG_REQ_OFFSET;
	mtk_w32(eth, reg_ctrl, MTK_RSS_GLO_CFG);

	/* 5. Enable RSS */
	//SET_PDMA_RSS_EN(1);

	reg_ctrl = mtk_r32(eth, MTK_RSS_GLO_CFG);
	reg_ctrl &= ~(MTK_RSS_EN);
	reg_ctrl |= ((1) & 0x1) << MTK_RSS_EN_OFFSET;
	mtk_w32(eth, reg_ctrl, MTK_RSS_GLO_CFG);

	/* 6. Release pause */
	//SET_PDMA_RSS_CFG_REQ(0);
	reg_ctrl = mtk_r32(eth, MTK_RSS_GLO_CFG);
	reg_ctrl &= ~(MTK_RSS_CFG_REQ);
	reg_ctrl |= ((0) & 0x1) << MTK_RSS_CFG_REQ_OFFSET;
	mtk_w32(eth, reg_ctrl, MTK_RSS_GLO_CFG);
#endif

#if defined(CONFIG_MTK_ADMAV2)
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_ADMAV2_RSS_FLB)) {
		/* RSS LFB*/
		mtk_w32(eth,
		(mtk_r32(eth, ADMAV2_RSS2_LFB_TIMER) | ADMAV2_RSS_LFB_ENABLE |
			ADMAV2_RSS_LFB_TIMER_AGE | ADMAV2_RSS_LFB_TIMER_TICK),
			ADMAV2_RSS2_LFB_TIMER);

	}
#endif

	return 0;
}

void init_rss_napi(struct mtk_eth *eth)
{

#if defined(CONFIG_MTK_ADMAV2)
	eth->rx_rss_napi[0].val_bit =  MTK_RX_DONE_DLY0 | MTK_RX_DONE_INT0 | BIT(28) |BIT(29)|BIT(30)|BIT(31);
	eth->rx_rss_napi[0].val_bit2 =  MTK_RX_DONE_DLY0 | BIT(28) |BIT(29)|BIT(30)|BIT(31);
#else
	eth->rx_rss_napi[0].val_bit =  MTK_RX_DONE_DLY;
	eth->rx_rss_napi[0].val_bit2 =	MTK_RX_DONE_DLY;
#endif
	eth->rx_rss_napi[0].status_reg = MTK_PDMA_INT_STATUS0;
	eth->rx_rss_napi[0].mask_reg = MTK_PDMA_INT_MASK0;
	eth->rx_rss_napi[0].eth = eth;
	eth->rx_rss_napi[0].idx = 0;

	eth->rx_rss_napi[1].val_bit = MTK_RX_DONE_DLY1 | MTK_RX_DONE_INT1;
	eth->rx_rss_napi[1].status_reg = MTK_PDMA_INT_STATUS1;
	eth->rx_rss_napi[1].mask_reg = MTK_PDMA_INT_MASK1;
	eth->rx_rss_napi[1].val_bit2 = MTK_RX_DONE_DLY1;
	eth->rx_rss_napi[1].eth = eth;
	eth->rx_rss_napi[1].idx = 1;

	eth->rx_rss_napi[2].val_bit =  MTK_RX_DONE_DLY2 | MTK_RX_DONE_INT2;
	eth->rx_rss_napi[2].status_reg = MTK_PDMA_INT_STATUS2;
	eth->rx_rss_napi[2].mask_reg = MTK_PDMA_INT_MASK2;
	eth->rx_rss_napi[2].val_bit2 = MTK_RX_DONE_DLY2;
	eth->rx_rss_napi[2].eth = eth;
	eth->rx_rss_napi[2].idx = 2;

	eth->rx_rss_napi[3].val_bit = MTK_RX_DONE_DLY3 | MTK_RX_DONE_INT3;
	eth->rx_rss_napi[3].status_reg = MTK_PDMA_INT_STATUS3;
	eth->rx_rss_napi[3].mask_reg = MTK_PDMA_INT_MASK3;
	eth->rx_rss_napi[3].val_bit2 = MTK_RX_DONE_DLY3;
	eth->rx_rss_napi[3].eth = eth;
	eth->rx_rss_napi[3].idx = 3;
}


