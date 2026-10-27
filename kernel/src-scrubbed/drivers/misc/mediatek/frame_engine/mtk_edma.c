// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A driver for MediaTek Netsys EDMA
 *
 * Author: Kurt Yang <kurt.yang@mediatek.com>
 *
 */
#include <linux/of_device.h>

#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <linux/interrupt.h>
#include <linux/bitmap.h>
#include <linux/ctype.h>

#include "mtk_edma.h"
#include "mtk_fe_proc.h"

extern int fe_dbg_level;

struct mtk_edma_module *g_edma;

int __CHANNEL_TO_IDX_SHIFT = EDMA_IDX0 - MTK_FE_EDMA_0;
#define channel_to_idx(channel) (channel+__CHANNEL_TO_IDX_SHIFT)

#define get_tx_ring_by_channel(channel, ring)  \
	(&g_edma->tx_ring[channel_to_idx(channel)][(ring)])

#define get_tx_ring(dma, ring) \
	get_tx_ring_by_channel((dma)->dma_channel, (ring))

#define get_tx_ring_by_idx(idx, ring) (&g_edma->tx_ring[(idx)][(ring)])

#define get_rx_ring_by_channel(channel, ring)  \
	(&g_edma->rx_ring[channel_to_idx(channel)][(ring)])

#define get_rx_ring(dma, ring) get_rx_ring_by_channel(dma->dma_channel, ring)

#define get_rx_ring_by_idx(idx, ring) (&g_edma->rx_ring[(idx)][(ring)])

static inline int atoi(char *s)
{
	int i = 0;

	while (isdigit(*s))
		i = i * 10 + *(s++) - '0';
	return i;
}

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

static inline void mtk_edma_rx_get_desc(struct edma_descriptor *rxd,
				   struct edma_descriptor *dma_rxd)
{
	struct _dummy_rxd_t *_rxd = (struct _dummy_rxd_t *)rxd;
	struct _dummy_rxd_t *_dma_rxd = (struct _dummy_rxd_t *)dma_rxd;
	*_rxd = READ_ONCE(*_dma_rxd);
}

static inline void edma_enable_irq(struct mtk_edma_module *edma,
		int idx, u32 mask)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&edma->irq_lock[idx], flags);
	val = mtk_edma_r32(edma, EDMA_INT_MASK(idx));
	mtk_edma_w32(edma, val | mask, EDMA_INT_MASK(idx));
	spin_unlock_irqrestore(&edma->irq_lock[idx], flags);
}

static inline void edma_disable_irq(struct mtk_edma_module *edma,
		int idx, u32 mask)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&edma->irq_lock[idx], flags);
	val = mtk_edma_r32(edma, EDMA_INT_MASK(idx));
	mtk_edma_w32(edma, val & ~mask, EDMA_INT_MASK(idx));
	spin_unlock_irqrestore(&edma->irq_lock[idx], flags);
}

static inline irqreturn_t edma_handle_rx_irq0(int irq, void *_idx)
{
	u64 idx = (u64)_idx;
	u32 status = mtk_edma_r32(g_edma, EDMA_INT_STATUS(idx));

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	if (status & (EDMA_RX_DONE_DLY0)) {
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	if (status & (EDMA_RX_DONE_DLY)) {
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
		if (fe_dbg_level >= 10)
			pr_notice("%s[%d], edma_idx = %d\n", __func__, __LINE__, idx);

		if (!fe_dma_schedule_rx0_poll(g_edma->fe_dma[idx]))
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			edma_disable_irq(g_edma, idx, EDMA_RX_DONE_DLY0);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
			edma_disable_irq(g_edma, idx, EDMA_RX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
		else {
			if (fe_dbg_level >= 1)
				pr_err("%s[%d], edma_handle_rx_irq0 fail, edma_idx = %d\n", __func__, __LINE__, idx);
		}
	}

	return IRQ_HANDLED;
}

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
static inline irqreturn_t edma_handle_rx_irq1(int irq, void *_idx)
{
	u64 idx = (u64)_idx;
	u32 status = mtk_edma_r32(g_edma, EDMA_INT_STATUS(idx));

	if (status & (EDMA_RX_DONE_DLY1)) {
		if (fe_dbg_level >= 10)
			pr_notice("%s[%d], edma_idx = %d\n", __func__, __LINE__, idx);

		if (!fe_dma_schedule_rx1_poll(g_edma->fe_dma[idx]))
			edma_disable_irq(g_edma, idx, EDMA_RX_DONE_DLY1);
		else {
			if (fe_dbg_level >= 1)
				pr_err("%s[%d], fe_dma_schedule_rx1_poll fail, edma_idx = %d\n", __func__, __LINE__, idx);
		}
	}

	return IRQ_HANDLED;
}
#endif

static inline irqreturn_t edma_handle_tx_irq(int irq, void *_idx)
{
	u64 idx = (u64)_idx;
	u32 status = mtk_edma_r32(g_edma, EDMA_INT_STATUS(idx));

	if (status & (EDMA_TX_DONE_DLY)) {
		if (fe_dbg_level >= 10)
			pr_notice("%s[%d], edma_idx = %d\n", __func__, __LINE__, idx);

		if (!fe_dma_schedule_tx_poll(g_edma->fe_dma[idx]))
			edma_disable_irq(g_edma, idx, EDMA_TX_DONE_DLY);
		else {
			if (fe_dbg_level >= 1)
				pr_err("%s[%d], fe_dma_schedule_tx_poll fail, edma_idx = %d\n", __func__, __LINE__, idx);
		}
	}

	return IRQ_HANDLED;
}

static inline int fe_edma_suspend_buf(
		struct mtk_fe_dma *dma, int ring_no)
{
	unsigned long flags;
	struct edma_rx_ring *ring = get_rx_ring_by_channel(
					dma->dma_channel, ring_no);

	spin_lock_irqsave(&ring->suspend_lock, flags);
	set_bit(EDMA_FC_SUSPEND, &ring->fc_state);
	ring->suspend_cnt++;
	spin_unlock_irqrestore(&ring->suspend_lock, flags);
	if (fe_dbg_level >= 7)
		pr_notice("[%s] Suspend: dma_channel=%x, ring_no=%x\n",
				__func__, dma->dma_channel, ring_no);

	return 0;
}

static inline int fe_edma_resume_buf(
		struct mtk_fe_dma *dma, int ring_no)
{
	unsigned long flags;
	struct edma_rx_ring *ring = get_rx_ring_by_channel(
					dma->dma_channel, ring_no);

	spin_lock_irqsave(&ring->suspend_lock, flags);
	clear_bit(EDMA_FC_SUSPEND, &ring->fc_state);
	if (test_and_clear_bit(EDMA_FC_WAIT_RESUME, &ring->fc_state)) {
		ring->resume_cnt++;
		spin_unlock_irqrestore(&ring->suspend_lock, flags);
		struct mtk_edma *fe_edma;

		fe_edma = container_of(dma, struct mtk_edma, fe_dma);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		edma_enable_irq(fe_edma->module, fe_edma->edma_idx,
			ring_no ? EDMA_RX_DONE_DLY1 : EDMA_RX_DONE_DLY0);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
		edma_enable_irq(fe_edma->module, fe_edma->edma_idx, EDMA_RX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
		if (fe_dbg_level >= 7)
			pr_notice("[%s] Resume: dma_channel=%x, ring_no=%x\n",
					__func__, dma->dma_channel, ring_no);

		return 0;
	}
	spin_unlock_irqrestore(&ring->suspend_lock, flags);

	return -1;
}

static void edma_tx_unmap(struct mtk_edma_module *edma,
		struct mtk_tx_buf *tx_buf, u32 idx)
{
	if (tx_buf->flags & TX_FLAGS_SINGLE0) {
		dma_unmap_single(edma->dev,
				 dma_unmap_addr(tx_buf, dma_addr0),
				 dma_unmap_len(tx_buf, dma_len0),
				 DMA_TO_DEVICE);
	}
	if (tx_buf->flags & TX_FLAGS_PAGE0) {
		if (!(tx_buf->flags & TX_FLAGS_SINGLE0)
				&& dma_unmap_len(tx_buf, dma_len0)) {
			dma_unmap_page(edma->dev,
				       dma_unmap_addr(tx_buf, dma_addr0),
				       dma_unmap_len(tx_buf, dma_len0),
				       DMA_TO_DEVICE);
		}
		if (dma_unmap_len(tx_buf, dma_len1)) {
			dma_unmap_page(edma->dev,
				       dma_unmap_addr(tx_buf, dma_addr1),
				       dma_unmap_len(tx_buf, dma_len1),
				       DMA_TO_DEVICE);
		}
	}
	tx_buf->flags = 0;
	if (tx_buf->obj)
		fe_dma_notify_txdone(edma->fe_dma[idx], tx_buf->obj);
	tx_buf->obj = NULL;
}

static int edma_poll_rx(struct mtk_edma_module *edma,
			u32 edma_idx, int budget, int ring_no)
{
	struct edma_rx_ring *ring = get_rx_ring_by_idx(edma_idx, ring_no);
	int idx, ret;
	struct dma_rx_buf *buf, *new_buf;
	struct edma_descriptor *rxd, trxd;
	int done = 0;

	struct EDMA_RXD2_T *rxd2_ptr;
	struct EDMA_RXD3_T *rxd3_ptr;
	struct EDMA_RXD4_T *rxd4_ptr;

	//update max_distance
	int tmp_distance = (mtk_edma_r32(edma, ERX_DRX_IDX_CFG(edma_idx, ring_no)) - ring->cpu_idx - 1)
						% (ring->dma_size);
	if (tmp_distance > ring->max_distance)
		ring->max_distance = tmp_distance;

	while (done < budget) {
		unsigned int pktlen;
		dma_addr_t dma_addr, dma_addr_old;

		idx = (ring->cpu_idx + 1) % (ring->dma_size);

		if (test_bit(EDMA_FC_SUSPEND, &ring->fc_state)) {
			if (fe_dbg_level >= 7)
				pr_notice("%s[%d], edma-%d rx_ring-%d fc_state is EDMA_FC_SUSPEND\n", __func__, __LINE__, edma_idx, ring_no);

			goto rx_done;
		}

		if (unlikely(!ring)) {
			pr_err("%s[%d], no edma-%d rx_ring-%d\n", __func__, __LINE__, edma_idx, ring_no);
			goto rx_done;
		}

		rxd = &ring->dma[idx];
		buf = ring->bufs[idx];
		mtk_edma_rx_get_desc(&trxd, rxd);

		if (fe_dbg_level >= 7)
			pr_notice("%s[%d], cidx = %d, didx = %d, edma_idx = %d, ring_no = %d\n", __func__, __LINE__,
				idx, mtk_edma_r32(edma, ERX_DRX_IDX_CFG(edma_idx, ring_no)), edma_idx, ring_no);

		rxd2_ptr = (struct EDMA_RXD2_T *) &trxd.desc2;
		if (likely(rxd2_ptr->DDONE_bit == 1))
			ring->cpu_idx_update = true;
		else {
			if (fe_dbg_level >= 7)
				pr_notice("%s[%d], DDONE_bit is 0\n", __func__, __LINE__);

			goto rx_done;
		}

		/* acquire new buffer */
		if (edma->fe_dma[edma_idx]->rx_recycle_mode) {
get_again:
			new_buf = list_first_entry_or_null(
					ring->free_buf_pool,
					struct dma_rx_buf,
					list);
			if (likely(new_buf)) {
				list_del_init(&new_buf->list);
				ring->free_buf_cnt--;
			} else {
				unsigned long flags;
				struct list_head *tmp;

				if (unlikely(list_empty(ring->recycle_pool))) {
					new_buf = napi_alloc_frag(
						sizeof(struct dma_rx_buf));
					if (unlikely(!new_buf)) {
						fe_dma_rxdrop_counting(
							edma->fe_dma[edma_idx],
							1);
						if (fe_dbg_level >= 7)
							pr_notice("%s[%d], fe_dma_rxdrop_counting\n", __func__, __LINE__);

						goto release_desc;
					}
					new_buf->skb = NULL;
					new_buf->dma = edma->fe_dma[edma_idx];
					new_buf->ring_no = ring_no;
				} else {
					spin_lock_irqsave(
							&ring->pool_swap_lock,
							flags);
					tmp = ring->free_buf_pool;
					ring->free_buf_pool =
						ring->recycle_pool;
					ring->recycle_pool = tmp;
					ring->free_buf_cnt =
						ring->recycle_buf_cnt;
					ring->recycle_buf_cnt = 0;
					spin_unlock_irqrestore(
							&ring->pool_swap_lock,
							flags);
					goto get_again;
				}
			}
		} else {
			new_buf = napi_alloc_frag(sizeof(struct dma_rx_buf));
			if (unlikely(!new_buf)) {
				fe_dma_rxdrop_counting(edma->fe_dma[edma_idx],
						1);
				if (fe_dbg_level >= 7)
					pr_notice("%s[%d], fe_dma_rxdrop_counting\n", __func__, __LINE__);

				goto release_desc;
			}
			new_buf->skb = NULL;
			new_buf->dma = edma->fe_dma[edma_idx];
			new_buf->ring_no = ring_no;
		}

		dma_addr = dma_map_single(edma->dev,
					  new_buf->data + NET_SKB_PAD,
					  ring->buf_size,
					  DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(edma->dev, dma_addr))) {
			skb_free_frag(new_buf);
			fe_dma_rxdrop_counting(edma->fe_dma[edma_idx], 1);
			if (fe_dbg_level >= 7)
				pr_notice("%s[%d], fe_dma_rxdrop_counting\n", __func__, __LINE__);

			goto release_desc;
		}

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		dma_addr_old = ((u64)rxd2_ptr->SDP_H << 32) | (u64)trxd.desc1;
#else
		dma_addr_old = trxd.desc1;
#endif
		dma_unmap_single(edma->dev, dma_addr_old,
				 ring->buf_size, DMA_FROM_DEVICE);
		pktlen = (trxd.desc2 >> 8) & 0xffff;
		rxd3_ptr = (struct EDMA_RXD3_T *) &trxd.desc3;
		rxd4_ptr = (struct EDMA_RXD4_T *) &trxd.desc4;
                if (fe_dbg_level >= 10)
                        pr_notice("[%s][%d], rxd3_ptr->TAG = 0x%x, rxd4_ptr->VID = 0x%x\n", __func__, __LINE__, rxd3_ptr->TAG, rxd4_ptr->VID);

		ret = fe_dma_deliver_rxdata(edma->fe_dma[edma_idx],
				buf->data,
				ring->frag_size,
				pktlen,
				rxd3_ptr->L4VLD,
				rxd3_ptr->TAG ? rxd4_ptr->VID : 0,
				*(u32 *)&trxd.desc5);
		if (unlikely(ret)) {
			fe_dma_rxdrop_counting(edma->fe_dma[edma_idx], 1);
			/* re-map again. */
			dma_addr_old = dma_map_single(edma->dev,
					  buf->data + NET_SKB_PAD,
					  ring->buf_size,
					  DMA_FROM_DEVICE);
			ring->dma[idx].desc1 = (unsigned int)dma_addr_old;
			if (fe_dbg_level >= 7)
				pr_notice("%s[%d], fe_dma_rxdrop_counting\n", __func__, __LINE__);

			goto release_desc;
		}

		ring->bufs[idx] = new_buf;
		ring->dma[idx].desc1 = (unsigned int)dma_addr;
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		ring->dma[idx].desc2 = (ring->buf_size << 8)
				| ((dma_addr >> 32) & 0xF);
#else
		ring->dma[idx].desc2 = (ring->buf_size << 8);
#endif

release_desc:
		//rxd2_ptr = (struct EDMA_RXD2_T *) &ring->dma[idx].desc2;
		//rxd2_ptr->DDONE_bit=0;
		//rxd2_ptr->LS0=0; rxd2_ptr->PLEN0=ring->buf_size;
		/* replace above two lines with this. */
		ring->cpu_idx = idx;
		done++;
	}

rx_done:
	if (done) {
		/* make sure that all changes to dma ring are flushed before
		 * we continue
		 */
		wmb();

		if (ring->cpu_idx_update) {
			ring->cpu_idx_update = false;
			mtk_edma_w32(edma, ring->cpu_idx,
					ERX_CRX_IDX_CFG(edma_idx, ring_no));
		}
	}

	return done;
}

static int edma_poll_tx(struct mtk_edma_module *edma, u32 idx,
		int budget, unsigned int *done, unsigned int *bytes)
{
	struct edma_tx_ring *ring = get_tx_ring_by_idx(idx, 0);
	void *buf_obj;
	struct mtk_tx_buf *tx_buf;
	u32 cpu, dma;

	cpu = ring->cpu_idx;
	dma = mtk_edma_r32(edma, ETX_DTX_IDX_CFG(idx, 0));
	if (fe_dbg_level >= 7)
		pr_notice("%s[%d], cidx = %d, didx = %d, edma_idx = %d\n", __func__, __LINE__, cpu, dma, idx);

	while ((cpu != dma) && budget) {
		tx_buf = &ring->buf[cpu];
		buf_obj = tx_buf->obj;

		if (buf_obj) {
			*bytes += tx_buf->len;
			*done += 1;
			budget--;
		}

		edma_tx_unmap(edma, tx_buf, idx);
		atomic_inc(&ring->free_count);
		cpu = (cpu + 1) % ring->dma_size;
	}

	ring->cpu_idx = cpu;
	return budget;
}

/* wait for DMA to finish whatever it is doing before we start using it again */
static int edma_busy_wait(struct mtk_edma_module *edma)
{
	unsigned long t_start = jiffies;

	while (1) {
		if (!(mtk_edma_r32(edma, EDMA_GLO_CFG(EDMA_IDX0)) &
					(MTK_TX_DMA_BUSY | MTK_RX_DMA_BUSY))
			&& !(mtk_edma_r32(edma, EDMA_GLO_CFG(EDMA_IDX1)) &
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
					(MTK_TX_DMA_BUSY | MTK_RX_DMA_BUSY))
			&& !(mtk_edma_r32(edma, EDMA_GLO_CFG(EDMA_IDX2)) &
#endif
				(MTK_TX_DMA_BUSY | MTK_RX_DMA_BUSY))) {
			return 0;
		}
		if (time_after(jiffies, t_start + MTK_DMA_BUSY_TIMEOUT))
			break;
	}

	return -1;
}

static void edma_start_dma(struct mtk_edma_module *edma, int idx)
{
	u32 rx_2b_offset = (NET_IP_ALIGN == 2) ? MTK_RX_2B_OFFSET : 0;

	//EDMA_GLO_CFG: TX_WB_DDONE | RX_DMA_EN |
	//	PDMA_BT_SIZE_16DWORDS | PDMA_DESC_32B_E |
	//   MULTI_EN | ADMA_RX_BT_SIZE_32DWORDS  RX_2B_OFFSET
	//mtk_edma_w32(edma, 0x80001C64, EDMA_GLO_CFG(channel));
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	u32 val = 0;

	val = mtk_edma_r32(edma, EDMA_GLO_CFG(idx));
	val |= rx_2b_offset | MTK_TX_WB_DDONE | MTK_RX_DMA_EN | MTK_TX_DMA_EN;
	/* Set EDMA_GLO_CFG field [23:16] to 0x80 since CDM FIFO overrun issue */
	val &= ~(0xFF << 16);
	val |= (0x80 << 16);
	mtk_edma_w32(edma, val, EDMA_GLO_CFG(idx));
#else
	mtk_edma_w32(edma, 0x804C65 | rx_2b_offset, EDMA_GLO_CFG(idx));
#endif
}

static void edma_stop_dma(struct mtk_edma_module *edma, int idx)
{
	u32 val;
	int i;

	val = mtk_edma_r32(edma, EDMA_GLO_CFG(idx));

	val &= ~(MTK_TX_WB_DDONE | MTK_RX_DMA_EN | MTK_TX_DMA_EN);
	mtk_edma_w32(edma, val, EDMA_GLO_CFG(idx));

	/* wait for dma stop */
	for (i = 0; i < 10; i++) {
		val = mtk_edma_r32(edma, EDMA_GLO_CFG(idx));
		if (val & (MTK_TX_DMA_BUSY | MTK_RX_DMA_BUSY)) {
			msleep(20);
			continue;
		}
		break;
	}
}

static int edma_tx_hw_init(struct mtk_edma_module *edma,
		struct edma_tx_ring *ring, int ring_no, int idx)
{
	mtk_edma_w32(edma, ring->phys, ETX_BASE_PTR_CFG(idx, ring_no));
	mtk_edma_w32(edma, EDMA_TX_DMA_SIZE, ETX_MAX_CNT_CFG(idx, ring_no));
	mtk_edma_w32(edma, ring->cpu_idx, ETX_CTX_IDX_CFG(idx, ring_no));
	mtk_edma_w32(edma, EDMA_RST_DTX_IDX_CFG(ring_no), EDMA_RST_IDX(idx));

	return 0;
}

static int edma_rx_hw_init(struct mtk_edma_module *edma,
		struct edma_rx_ring *ring, int ring_no, int idx)
{
	int rx_dma_size = MTK_HW_EDMA_RX_DMA_SIZE_RING1;

	if (ring_no == 0)
		rx_dma_size = MTK_HW_EDMA_RX_DMA_SIZE;
	else
		rx_dma_size = MTK_HW_EDMA_RX_DMA_SIZE_RING1;

	mtk_edma_w32(edma, ring->phys, ERX_BASE_PTR_CFG(idx, ring_no));
	mtk_edma_w32(edma, rx_dma_size, ERX_MAX_CNT_CFG(idx, ring_no));
	mtk_edma_w32(edma, ring->cpu_idx, ERX_CRX_IDX_CFG(idx, ring_no));
	mtk_edma_w32(edma, EDMA_RST_DRX_IDX_CFG(ring_no), EDMA_RST_IDX(idx));

	return 0;
}


static void edma_tx_clean(struct mtk_edma_module *edma,
		struct edma_tx_ring *ring, int idx)
{
	int i;

	if (ring->buf) {
		for (i = 0; i < ring->dma_size; i++)
			edma_tx_unmap(edma, &ring->buf[i], idx);
		kfree(ring->buf);
		ring->buf = NULL;
	}

	if (ring->dma) {
		dma_free_coherent(edma->dev,
				  ring->dma_size * sizeof(*ring->dma),
				  ring->dma,
				  ring->phys);
		ring->dma = NULL;
	}
}

static void edma_rx_clean(struct mtk_edma_module *edma,
		struct edma_rx_ring *ring)
{
	int i;
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	u64 addr64;
#endif

	struct dma_rx_buf *buf;
	if (fe_dbg_level >= 7) {
		pr_notice("[%s][%d] free_buf_cnt = %d, recycle_buf_cnt = %d\n", __func__, __LINE__, ring->free_buf_cnt, ring->recycle_buf_cnt);
	}
	/* clean free_buf_pool */
	for (i = 0; i < ring->free_buf_cnt; i++) {
		if (fe_dbg_level >= 10)
			pr_notice("[%s][%d] clean free_buf_pool, i = %d\n", __func__, __LINE__, i);

		buf = list_first_entry_or_null(ring->free_buf_pool, struct dma_rx_buf, list);
		if (buf) {
			list_del_init(&buf->list);
			skb_free_frag(buf);
			if (fe_dbg_level >= 10)
				pr_notice("[%s][%d] skb_free_frag(buf)\n", __func__, __LINE__);
		}
		else {
			if (fe_dbg_level >= 10)
				pr_notice("[%s][%d] buf is NULL, cannot free\n", __func__, __LINE__);

			break;
		}
	}
	/* clean recycle_pool */
	for (i = 0; i < ring->recycle_buf_cnt; i++) {
		buf = list_first_entry_or_null(ring->recycle_pool, struct dma_rx_buf, list);
		if (buf) {
			list_del_init(&buf->list);
			skb_free_frag(buf);
			if (fe_dbg_level >= 10)
				pr_notice("[%s][%d] skb_free_frag(buf)\n", __func__, __LINE__);
		}
		else {
			if (fe_dbg_level >= 10)
				pr_notice("[%s][%d] buf is NULL, cannot free\n", __func__, __LINE__);

			break;
		}
	}

	if (ring->bufs && ring->dma) {
		for (i = 0; i < ring->dma_size; i++) {
			if (!ring->bufs[i])
				continue;
			if (!ring->dma[i].desc1)
				continue;
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			addr64 = (u64)(ring->dma[i].desc2 & 0xF) << 32;
#endif
			dma_unmap_single(edma->dev,
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
					 addr64 | ((u64)ring->dma[i].desc1),
#else
					 ring->dma[i].desc1,
#endif
					 ring->buf_size,
					 DMA_FROM_DEVICE);
			skb_free_frag(ring->bufs[i]);
		}
		kfree(ring->bufs);
		ring->bufs = NULL;
	}

	if (ring->dma) {
		dma_free_coherent(edma->dev,
				  ring->dma_size * sizeof(*ring->dma),
				  ring->dma,
				  ring->phys);
		ring->dma = NULL;
	}
}

static int edma_tx_alloc(struct mtk_edma_module *edma, int ring_no, int idx)
{
	struct edma_tx_ring *ring = get_tx_ring_by_idx(idx, ring_no);
	int i;
	struct EDMA_TXD2_T *txd2;

	ring->buf = kcalloc(EDMA_TX_DMA_SIZE, sizeof(*ring->buf), GFP_KERNEL);
	if (!ring->buf)
		return -ENOMEM;

	ring->dma = dma_alloc_coherent(edma->dev,
				EDMA_TX_DMA_SIZE * sizeof(*ring->dma),
				&ring->phys, GFP_KERNEL  | __GFP_ZERO);
	if (!ring->dma)
		return -ENOMEM;

	for (i = 0; i < EDMA_TX_DMA_SIZE; i++) {
		txd2 = (struct EDMA_TXD2_T *) &ring->dma[i].desc2;
		txd2->LS0 = 1;
		txd2->DDONE = 1;
	}

	ring->dma_size = EDMA_TX_DMA_SIZE;
	ring->cpu_idx = 0;
	atomic_set(&ring->free_count, EDMA_TX_DMA_SIZE - 2);  // Kurt: why -2??
	//ring->thresh = MAX_SKB_FRAGS;  Kurt: ... to be implemented.

	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();

	edma_tx_hw_init(edma, ring, ring_no, idx);

	return 0;
}

static int edma_rx_alloc(struct mtk_edma_module *edma, int ring_no, u32 idx)
{
	struct edma_rx_ring *ring = get_rx_ring_by_idx(idx, ring_no);
	int rx_data_len = ETH_DATA_LEN;
	int rx_dma_size = MTK_HW_EDMA_RX_DMA_SIZE_RING1;
	int i;
	struct EDMA_RXD2_T *rxd2_ptr;

	if (ring_no == 0)
		rx_dma_size = MTK_HW_EDMA_RX_DMA_SIZE;
	else
		rx_dma_size = MTK_HW_EDMA_RX_DMA_SIZE_RING1;

	ring->frag_size = mtk_max_frag_size(rx_data_len);
	ring->buf_size = mtk_max_buf_size(ring->frag_size);

	spin_lock_init(&ring->suspend_lock);
	clear_bit(EDMA_FC_SUSPEND, &ring->fc_state);
	clear_bit(EDMA_FC_WAIT_RESUME, &ring->fc_state);
	ring->suspend_cnt = 0;
	ring->resume_cnt = 0;
	ring->max_distance = 0;

	INIT_LIST_HEAD(&ring->__buf_pool_1);
	INIT_LIST_HEAD(&ring->__buf_pool_2);
	ring->free_buf_pool = &ring->__buf_pool_1;
	ring->recycle_pool = &ring->__buf_pool_2;
	spin_lock_init(&ring->pool_swap_lock);

	if (edma->fe_dma[idx]->rx_recycle_mode)
		ring->free_buf_cnt = 100;
	else
		ring->free_buf_cnt = 0;

	ring->recycle_buf_cnt = 0;

	ring->bufs = kcalloc(rx_dma_size, sizeof(*ring->bufs),
			     GFP_KERNEL);
	if (!ring->bufs)
		return -ENOMEM;

	for (i = 0; i < rx_dma_size + ring->free_buf_cnt; i++) {
		struct dma_rx_buf *buf = (struct dma_rx_buf *)
				netdev_alloc_frag(sizeof(struct dma_rx_buf));
		if (!buf) {
			dev_err(edma->dev, "%s[%d]: mem error (%d) %d\n",
					__func__, __LINE__, i, (int) ring->frag_size);
			return -ENOMEM;
		}

		/* For buffer recycle */
		buf->skb = NULL;
		buf->dma = edma->fe_dma[idx];
		buf->ring_no = ring_no;

		if (i < rx_dma_size)
			ring->bufs[i] = buf;
		else
			list_add_tail(&buf->list, ring->free_buf_pool);
	}

	ring->dma = dma_alloc_coherent(edma->dev,
				       rx_dma_size * sizeof(*ring->dma),
				       &ring->phys,
				       GFP_KERNEL | __GFP_ZERO);
	if (!ring->dma) {
		dev_err(edma->dev, "%s[%d]: mem error (%d) %ld\n",
				__func__, __LINE__, rx_dma_size, sizeof(*ring->dma));
		return -ENOMEM;
	}

	for (i = 0; i < rx_dma_size; i++) {
		dma_addr_t dma_addr = dma_map_single(edma->dev,
				ring->bufs[i]->data + NET_SKB_PAD,
				ring->buf_size,
				DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(edma->dev, dma_addr))) {
			dev_err(edma->dev, "%s[%d]: mem error (%d) %d\n",
					__func__, __LINE__, i, ring->buf_size);
			return -ENOMEM;
		}

		ring->dma[i].desc1 = (unsigned int)dma_addr;
		rxd2_ptr = (struct EDMA_RXD2_T *)&ring->dma[i].desc2;
		rxd2_ptr->DDONE_bit = 0;
		rxd2_ptr->LS0 = 0;
		rxd2_ptr->PLEN0 = ring->buf_size;
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		rxd2_ptr->SDP_H = (dma_addr >> 32) & 0xF;
#endif
	}

	ring->dma_size = rx_dma_size;
	ring->cpu_idx_update = false;
	ring->cpu_idx = rx_dma_size - 1;
	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();

	edma_rx_hw_init(edma, ring, ring_no, idx);

	return 0;
}

static int edma_hw_init(struct mtk_edma_module *edma, int idx)
{
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	edma_disable_irq(edma, idx, EDMA_RX_DONE_DLY0 | EDMA_RX_DONE_DLY1
				| EDMA_TX_DONE_DLY);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	edma_disable_irq(edma, idx, EDMA_RX_DONE_DLY | EDMA_TX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */

	// Assign tx irq into group-1.
	mtk_edma_w32(edma, EDMA_TX_DONE_DLY, EDMA_G1_INT_CTRL(EDMA_IDX0));
	mtk_edma_w32(edma, EDMA_TX_DONE_DLY, EDMA_G1_INT_CTRL(EDMA_IDX1));
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	mtk_edma_w32(edma, EDMA_TX_DONE_DLY, EDMA_G1_INT_CTRL(EDMA_IDX2));
	/* Assign rx ring1 irq into group-2*/
	mtk_edma_w32(edma, EDMA_RX_DONE_DLY1, EDMA_G2_INT_CTRL(EDMA_IDX0));
	mtk_edma_w32(edma, EDMA_RX_DONE_DLY1, EDMA_G2_INT_CTRL(EDMA_IDX1));
	mtk_edma_w32(edma, EDMA_RX_DONE_DLY1, EDMA_G2_INT_CTRL(EDMA_IDX2));
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */

	mtk_edma_w32(edma, 0x8f0f8f0f, EDMA_DELAY_INT_CFG(EDMA_IDX0));
	mtk_edma_w32(edma, 0x8f0f8f0f, EDMA_DELAY_INT_CFG(EDMA_IDX1));
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	mtk_edma_w32(edma, 0x8f0f8f0f, EDMA_DELAY_INT_CFG(EDMA_IDX2));
	/* Configure RX DLY INT for rx ring0 & ring1*/
	mtk_edma_w32(edma, 0x8f0f8f0f, EDMA_RX_DELAY_INT_CFG_0(EDMA_IDX0));
	mtk_edma_w32(edma, 0x8f0f8f0f, EDMA_RX_DELAY_INT_CFG_0(EDMA_IDX1));
	mtk_edma_w32(edma, 0x8f0f8f0f, EDMA_RX_DELAY_INT_CFG_0(EDMA_IDX2));
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	return 0;
}

static int edma_request_irq(struct platform_device *pdev, char *irq_name,
				     irq_handler_t handler, void *irq_priv)
{
	int ret, irq;
	struct device *dev = &pdev->dev;

	irq = platform_get_irq_byname(pdev, irq_name);
	if (irq < 0) {
		dev_err(dev, "unable to get IRQ '%s' (err %d)\n",
			irq_name, irq);
		return irq;
	}

	ret = devm_request_irq(dev, irq, handler,
				IRQF_NO_SUSPEND | IRQF_TRIGGER_NONE,
				irq_name, irq_priv);
	if (ret) {
		dev_err(dev, "unable to request IRQ %d\n", irq);
		return ret;
	}

	return irq;
}

static int fe_edma_poll(struct mtk_fe_dma *dma, int budget, int ring_no)
{
	struct mtk_edma *fe_edma = container_of(dma, struct mtk_edma, fe_dma);
	struct mtk_edma_module *edma = fe_edma->module;
	int edma_idx = fe_edma->edma_idx;
	struct edma_rx_ring *ring = get_rx_ring_by_idx(edma_idx, ring_no);
	u32 status, mask, rx_int_mask;
	int rx_done = 0;
	int remain_budget = budget;
	unsigned long flags;

// Kurt: really need this?
	//mtk_handle_status_irq(edma);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	rx_int_mask = ring_no ? EDMA_RX_DONE_DLY1 : EDMA_RX_DONE_DLY0;
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	rx_int_mask = EDMA_RX_DONE_DLY;
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
poll_again:
	mtk_edma_w32(edma, rx_int_mask, EDMA_INT_STATUS(edma_idx));
	rx_done = edma_poll_rx(edma, edma_idx, remain_budget, ring_no);
	spin_lock_irqsave(&ring->suspend_lock, flags);
	if (test_bit(EDMA_FC_SUSPEND, &ring->fc_state)) {
		if (ring_no)
			fe_dma_complete_rx1_poll(dma);
		else
			fe_dma_complete_rx0_poll(dma);
		set_bit(EDMA_FC_WAIT_RESUME, &ring->fc_state);
		spin_unlock_irqrestore(&ring->suspend_lock, flags);
		if (fe_dbg_level >= 7)
			pr_notice("[%s] Wait Resume: dma_channel=%x\n",
					__func__, dma->dma_channel);

		return rx_done + budget - remain_budget;
	}
	spin_unlock_irqrestore(&ring->suspend_lock, flags);

	if (unlikely(netif_msg_intr(edma))) {  // Kurt: ??
		status = mtk_edma_r32(edma, EDMA_INT_STATUS(edma_idx));
		mask = mtk_edma_r32(edma, EDMA_INT_MASK(edma_idx));
		dev_info(edma->dev, "done rx %d, intr 0x%08x/0x%x\n",
				rx_done, status, mask);
	}
	if (rx_done == remain_budget)
		return budget;

	status = mtk_edma_r32(edma, EDMA_INT_STATUS(edma_idx));
	if (status & (rx_int_mask)) {
		remain_budget -= rx_done;

		if (fe_dbg_level >= 7)
			pr_notice("%s[%d], edma-%d rx_ring-%d poll_again\n", __func__, __LINE__, edma_idx, ring_no);

		goto poll_again;
	}

	if (ring_no)
		fe_dma_complete_rx1_poll(dma);
	else
		fe_dma_complete_rx0_poll(dma);
	edma_enable_irq(edma, edma_idx, rx_int_mask);
	return rx_done + budget - remain_budget;
}

static int fe_edma_txRring_check(
		struct mtk_fe_dma *dma, int tx_num)
{
	struct edma_tx_ring *ring = get_tx_ring(dma, 0);

	if ((int)atomic_read(&ring->free_count) < tx_num)
		return 1;
	return 0;
}

static int fe_edma_tx_housekeeping(struct mtk_fe_dma *dma, int budget)
{
	struct mtk_edma *fe_edma = container_of(dma, struct mtk_edma, fe_dma);
	struct mtk_edma_module *edma = fe_edma->module;
	u32 status, mask;
	int tx_done = 0, bytes = 0;

	mtk_edma_w32(edma, EDMA_TX_DONE_DLY,
			EDMA_INT_STATUS(fe_edma->edma_idx));
	edma_poll_tx(edma, fe_edma->edma_idx, budget, &tx_done, &bytes);

	if (unlikely(netif_msg_intr(edma))) {
		status = mtk_edma_r32(edma,
			EDMA_INT_STATUS(fe_edma->edma_idx));
		mask = mtk_edma_r32(edma, EDMA_INT_MASK(fe_edma->edma_idx));
		dev_info(edma->dev,
				"done tx %d, bytes = %d, intr 0x%08x/0x%x\n",
				tx_done, bytes, status, mask);
	}

	if (tx_done == budget)
		return budget;

	status = mtk_edma_r32(edma, EDMA_INT_STATUS(fe_edma->edma_idx));
	if (status & EDMA_TX_DONE_DLY)
		return budget;

	//netdev_completed_queue(eth->netdev[i], done[i], bytes[i]);
	//    ...Kurt: todo
	fe_dma_complete_tx_poll(dma);
	edma_enable_irq(edma, fe_edma->edma_idx, EDMA_TX_DONE_DLY);

	return tx_done;
}

static int fe_edma_xmit(struct mtk_fe_dma *dma,
		void *data, int len,
		skb_frag_t *frags, int nr_frags,
		u8 tso, u8 is_checksum_offload, u16 vlan_tci, void *buf_obj)
{
	struct mtk_edma *fe_edma = container_of(dma, struct mtk_edma, fe_dma);
	struct mtk_edma_module *edma = fe_edma->module;
	struct edma_descriptor *itxd;
	struct mtk_tx_buf *itx_buf;
	dma_addr_t mapped_addr;
	u32 txd2_tmp = 0, txd5_tmp = 0, txd4_tmp = 0, txd6_tmp = 0;
	u32 cpu, i, n_desc = 1;
	struct EDMA_TXD2_T *p_txd2_tmp = (struct EDMA_TXD2_T *)&txd2_tmp;
	struct EDMA_TXD4_T *p_txd4_tmp = (struct EDMA_TXD4_T *)&txd4_tmp;
	struct EDMA_TXD5_T *p_txd5_tmp = (struct EDMA_TXD5_T *)&txd5_tmp;
	struct EDMA_TXD6_T *p_txd6_tmp = (struct EDMA_TXD6_T *)&txd6_tmp;
	struct edma_tx_ring *ring = get_tx_ring(dma, 0);

	cpu = mtk_edma_r32(edma, ETX_CTX_IDX_CFG(fe_edma->edma_idx, 0));
	itxd = &ring->dma[cpu];
	itx_buf = &ring->buf[cpu];

	mapped_addr = dma_map_single(
					edma->dev, data, len, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(edma->dev, mapped_addr)))
		return -ENOMEM;

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	p_txd2_tmp->SDP_H = (mapped_addr >> 32) & 0xF;
#endif
	p_txd2_tmp->SDL0 = len;
	p_txd2_tmp->LS0 = nr_frags ? 0 : 1;
	p_txd2_tmp->DDONE = 0;
	p_txd5_tmp->FPORT = dma->dest_port;
	p_txd5_tmp->TSO = tso ? 1 : 0;
	p_txd5_tmp->TUI_CO = is_checksum_offload ? 7 : 0;
	if (vlan_tci & VLAN_TAG_PRESENT) {
		p_txd6_tmp->INSV_1 = 1;
		p_txd6_tmp->VLAN_TAG_1 = vlan_tci & (~VLAN_TAG_PRESENT);

		if (fe_dbg_level >= 10)
			pr_notice("[%s][%d], VLAN_TAG_1 = 0x%x\n", __func__, __LINE__, p_txd6_tmp->VLAN_TAG_1);
	} else {
		p_txd6_tmp->INSV_1 = 0;
		p_txd6_tmp->VLAN_TAG_1 = 0;
	}

	WRITE_ONCE(itxd->desc1, mapped_addr);
	WRITE_ONCE(itxd->desc2, txd2_tmp);
	if (!nr_frags) {
		WRITE_ONCE(itxd->desc3, 0);
		WRITE_ONCE(itxd->desc4, 0);
	}
	WRITE_ONCE(itxd->desc5, txd5_tmp);
	WRITE_ONCE(itxd->desc6, txd6_tmp);
	//WRITE_ONCE(itxd->desc7, 0);
	//WRITE_ONCE(itxd->desc8, 0);

	/* store buffer/skb to cleanup */
	memset(itx_buf, 0, sizeof(*itx_buf));
	itx_buf->len = len;
	itx_buf->obj = buf_obj;	// should buf_obj be belong to the last frags?
	itx_buf->flags |= TX_FLAGS_SINGLE0;
	dma_unmap_addr_set(itx_buf, dma_addr0, mapped_addr);
	dma_unmap_len_set(itx_buf, dma_len0, len);

	cpu = (cpu + 1) % ring->dma_size;

	/* TX SG offload */
	for (i = 0; i < nr_frags; i++) {
		skb_frag_t *frag = &frags[i];
		unsigned int offset = 0;
		int frag_size = skb_frag_size(frag);

		while (frag_size) {
			bool last_frag = false;
			unsigned int frag_map_size =
					min(frag_size, EDMA_TX_DMA_BUF_LEN);
			mapped_addr = skb_frag_dma_map(edma->dev, frag, offset,
							   frag_map_size,
							   DMA_TO_DEVICE);
			if (unlikely(dma_mapping_error(edma->dev,
					mapped_addr))) {
				pr_err("%s[%d], dma_mapping_error\n", __func__, __LINE__);
				goto err_dma;
			}

			if (i == nr_frags - 1 &&
				(frag_size - frag_map_size) == 0)
				last_frag = true;

			if (p_txd4_tmp->SDL1 != 0) {
				p_txd4_tmp->SDL1 = 0;

				itxd = &ring->dma[cpu];
				itx_buf = &ring->buf[cpu];

				p_txd2_tmp->SDL0 = frag_map_size;
				p_txd2_tmp->LS0 = last_frag ? 1 : 0;
				p_txd2_tmp->DDONE = 0;

				WRITE_ONCE(itxd->desc1, mapped_addr);
				WRITE_ONCE(itxd->desc2, txd2_tmp);
				if (last_frag) {
					WRITE_ONCE(itxd->desc3, 0);
					WRITE_ONCE(itxd->desc4, 0);
				}
				WRITE_ONCE(itxd->desc5, txd5_tmp);
				//WRITE_ONCE(itxd->desc6, 0);
				// for FE_HW_VLAN_TX. ToDo..
				//WRITE_ONCE(itxd->desc7, 0);
				//WRITE_ONCE(itxd->desc8, 0);

				memset(itx_buf, 0, sizeof(*itx_buf));
				itx_buf->obj = 0;
				itx_buf->flags |= TX_FLAGS_PAGE0;
				dma_unmap_addr_set(itx_buf, dma_addr0,
						mapped_addr);
				dma_unmap_len_set(itx_buf, dma_len0,
						frag_map_size);

				cpu = (cpu + 1) % ring->dma_size;
				n_desc++;
			} else {
				p_txd4_tmp->SDL1 = frag_map_size;
				p_txd4_tmp->LS1 = last_frag ? 1 : 0;
				WRITE_ONCE(itxd->desc3, mapped_addr);
				WRITE_ONCE(itxd->desc4, txd4_tmp);
				dma_unmap_addr_set(itx_buf, dma_addr1,
						mapped_addr);
				dma_unmap_len_set(itx_buf, dma_len1,
						frag_map_size);
			}
			frag_size -= frag_map_size;
			offset += frag_map_size;
		}
	}

	atomic_sub(n_desc, &ring->free_count);

	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();
	mtk_edma_w32(edma, cpu, ETX_CTX_IDX_CFG(fe_edma->edma_idx, 0));
	if (fe_dbg_level >= 7)
		pr_notice("%s[%d], cidx = %d, edma_idx = %d\n", __func__, __LINE__, cpu, fe_edma->edma_idx);

	return 0;

err_dma:

	return -ENOMEM;
/*
 *Kurt: need to implement with fragments together...
 *
 *err_dma:
 *	do {
 *		tx_buf = mtk_desc_to_tx_buf(ring, itxd);
 *		mtk_tx_unmap(eth, tx_buf); // unmap dma
 *
 *		itxd->txd3 = TX_DMA_LS0 | TX_DMA_OWNER_CPU;
 *		itxd = mtk_qdma_phys_to_virt(ring, itxd->txd2);
 *	} while (itxd != txd);
 *	return -ENOMEM;
 */
}

static int fe_edma_recycle_buf(
		struct mtk_fe_dma *dma, struct dma_rx_buf *buf)
{
	struct edma_rx_ring *ring = get_rx_ring_by_channel(
					dma->dma_channel, buf->ring_no);
	unsigned long flags;

	if (dma->rx_recycle_mode) {
		spin_lock_irqsave(&ring->pool_swap_lock, flags);
		list_add_tail(&buf->list, ring->recycle_pool);
		ring->recycle_buf_cnt++;
		spin_unlock_irqrestore(&ring->pool_swap_lock, flags);
	}
	else {
		dev_kfree_skb_any(buf->skb);
	}
	return 0;
}

static int fe_edma_pm_handle(int channel, bool suspend)
{
	int edma_idx = channel_to_idx(channel);
	struct edma_tx_ring *tx_ring;
	struct edma_rx_ring *rx_ring;

	if(edma_idx < 0) {
		//should not come here
		pr_err("[%s] edma_idx (%d) < 0\n", __func__, edma_idx);
		return -1;
	}

	if (g_edma->fe_dma[edma_idx]) {
		if (suspend) {
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			edma_disable_irq(g_edma, edma_idx,
				EDMA_RX_DONE_DLY0 | EDMA_RX_DONE_DLY1 | EDMA_TX_DONE_DLY);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
			edma_disable_irq(g_edma, edma_idx, EDMA_RX_DONE_DLY | EDMA_TX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
			edma_stop_dma(g_edma, edma_idx);
		} else {

			edma_hw_init(g_edma, edma_idx);
			/* edma_tx_ring*/
			tx_ring = get_tx_ring_by_idx(edma_idx, 0);
			/* edma_tx has only index: 0 */
			g_edma->tx_ring[edma_idx][0].cpu_idx = 0;
			/* make sure that all changes to the dma ring are flushed before we
			 * continue
			 */
			wmb();

			edma_tx_hw_init(g_edma, tx_ring, 0, edma_idx);

			/* edma_rx_ring*/
			/* edma_rx has two index: 0,1 */
			/* Currently use edma_rx0_ring0,  edma_rx0_ring1, edma_rx1_ring0*/
			if (edma_idx == 0) {
				/* edma_rx0_ring0 */
				rx_ring = get_rx_ring_by_idx(edma_idx, 0);
				g_edma->rx_ring[edma_idx][0].cpu_idx = MTK_HW_EDMA_RX_DMA_SIZE - 1;
				/* make sure that all changes to the dma ring are flushed before we
				 * continue
				 */
				wmb();
				edma_rx_hw_init(g_edma, rx_ring, 0, edma_idx);

				/* edma_rx0_ring1 */
				rx_ring = get_rx_ring_by_idx(edma_idx, 1);
				g_edma->rx_ring[edma_idx][1].cpu_idx = MTK_HW_EDMA_RX_DMA_SIZE - 1;
				/* make sure that all changes to the dma ring are flushed before we
				 * continue
				 */
				wmb();
				edma_rx_hw_init(g_edma, rx_ring, 1, edma_idx);
			} else if (edma_idx == 1) {
				/* edma_rx1_ring0 */
				rx_ring = get_rx_ring_by_idx(edma_idx, 0);
				g_edma->rx_ring[edma_idx][0].cpu_idx = MTK_HW_EDMA_RX_DMA_SIZE - 1;
				/* make sure that all changes to the dma ring are flushed before we
				 * continue
				 */
				wmb();
				edma_rx_hw_init(g_edma, rx_ring, 0, edma_idx);
			}

			edma_start_dma(g_edma, edma_idx);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			edma_enable_irq(g_edma, edma_idx,
				EDMA_RX_DONE_DLY0 | EDMA_RX_DONE_DLY1 | EDMA_TX_DONE_DLY);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
			edma_enable_irq(g_edma, edma_idx, EDMA_RX_DONE_DLY | EDMA_TX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
		}
	}
	return 0;
}

static void fe_edma_close(struct mtk_fe_dma *dma)
{
	struct mtk_edma *fe_edma = container_of(dma, struct mtk_edma, fe_dma);
	struct mtk_edma_module *edma = fe_edma->module;
	int i;

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	edma_disable_irq(edma, dma->dma_channel,
			EDMA_RX_DONE_DLY0 | EDMA_RX_DONE_DLY1 | EDMA_TX_DONE_DLY);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	edma_disable_irq(edma, dma->dma_channel, EDMA_RX_DONE_DLY | EDMA_TX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	edma_stop_dma(edma, dma->dma_channel);

	for (i = 0; i < MTK_MAX_EDMA_RX_RING_NUM; i++)
		edma_rx_clean(edma, get_rx_ring(dma, i));

	for (i = 0; i < MTK_MAX_EDMA_TX_RING_NUM; i++)
		edma_tx_clean(edma, get_tx_ring(dma, i), fe_edma->edma_idx);

	edma->fe_dma[fe_edma->edma_idx] = 0;
	kfree(fe_edma);
}

static void fe_edma_hw_conf(struct mtk_fe_dma *dma, u32 caps, bool on)
{
	struct mtk_edma *fe_edma = container_of(dma, struct mtk_edma, fe_dma);
	struct mtk_edma_module *edma = fe_edma->module;

	if(on)
		dma->caps |= caps;
	else
		dma->caps &= ~caps;

	if (MTK_FE_DMA_CAPS(dma->caps, MTK_FE_DMA_RX_VLAN_UNTAG))
		mtk_edma_w32(edma, EDMA_VLAN_UNTAG_EN,
				EDMA_CDM_EG_CTRL(fe_edma->edma_idx));
	else
		mtk_edma_w32(edma, 0,
				EDMA_CDM_EG_CTRL(fe_edma->edma_idx));
}

static int fe_edma_open(int channel, struct mtk_fe_dma **dma,
		u32 tx_ring_num, u32 rx_ring_num)
{
	int ret, i;
	struct mtk_edma *fe_edma;
	int edma_idx = channel_to_idx(channel);

	if(edma_idx < 0) {
		//should not come here
		pr_err("[%s][%d] edma_idx (%d) < 0\n", __func__, __LINE__, edma_idx);
		return -1;
	}

	if (g_edma->fe_dma[edma_idx])
		return -EBUSY;

	if (edma_busy_wait(g_edma))
		return -EBUSY;

	fe_edma = kzalloc(sizeof(struct mtk_edma), GFP_KERNEL);
	if (!fe_edma)
		return -ENOMEM;

	fe_edma->edma_idx = edma_idx;
	g_edma->fe_dma[fe_edma->edma_idx] = (struct mtk_fe_dma *)fe_edma;
	edma_hw_init(g_edma, fe_edma->edma_idx);

	/* Make sure tx_ring_num is valid. */
	if (tx_ring_num > MTK_MAX_EDMA_TX_RING_NUM)
		tx_ring_num = MTK_MAX_EDMA_TX_RING_NUM;

	/* Make sure rx_ring_num is valid. */
	if (rx_ring_num > MTK_MAX_EDMA_RX_RING_NUM)
		rx_ring_num = MTK_MAX_EDMA_RX_RING_NUM;

	for (i = 0; i < rx_ring_num; i++) {
		ret = edma_rx_alloc(g_edma, i, fe_edma->edma_idx);

		if (ret) {
			pr_err("[%s][%d] edma_rx_alloc fail\n", __func__, __LINE__);
			goto free_rx_desc;
		}
	}
	for (i = 0; i < tx_ring_num; i++) {
		ret = edma_tx_alloc(g_edma, i, fe_edma->edma_idx);

		if (ret) {
			pr_err("[%s][%d] edma_tx_alloc fail\n", __func__, __LINE__);
			goto free_tx_desc;
		}
	}

	edma_start_dma(g_edma, fe_edma->edma_idx);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	edma_enable_irq(g_edma,	fe_edma->edma_idx,
			EDMA_RX_DONE_DLY0 | EDMA_RX_DONE_DLY1 | EDMA_TX_DONE_DLY);
#else /* defined(CONFIG_MEDIATEK_NETSYS_V2) */
	edma_enable_irq(g_edma,	fe_edma->edma_idx, EDMA_RX_DONE_DLY | EDMA_TX_DONE_DLY);
#endif /* defined(CONFIG_MEDIATEK_NETSYS_V2) */

	fe_edma->module = g_edma;

	*dma = (struct mtk_fe_dma *)fe_edma;
	(*dma)->dma_channel = channel;
	(*dma)->do_xmit = fe_edma_xmit;
	(*dma)->is_tx_ring_full = fe_edma_txRring_check;
	(*dma)->do_tx_housekeeping = fe_edma_tx_housekeeping;
	(*dma)->do_poll = fe_edma_poll;
	(*dma)->do_close = fe_edma_close;
	(*dma)->recycle_buf = fe_edma_recycle_buf;
	(*dma)->suspend_buf = fe_edma_suspend_buf;
	(*dma)->resume_buf = fe_edma_resume_buf;
	(*dma)->do_hw_conf = fe_edma_hw_conf;
	(*dma)->max_tx_buf_len = EDMA_TX_DMA_BUF_LEN;
	(*dma)->do_pm_handle = fe_edma_pm_handle;

	return 0;

free_tx_desc:
	for (i = 0; i < MTK_MAX_EDMA_TX_RING_NUM; i++)
		edma_tx_clean(g_edma, get_tx_ring_by_channel(channel, i),
				fe_edma->edma_idx);

free_rx_desc:
	for (i = 0; i < MTK_MAX_EDMA_RX_RING_NUM; i++)
		edma_rx_clean(g_edma, get_rx_ring_by_channel(channel, i));

	kfree(fe_edma);

	return ret;
}

int fe_edma_dbg(char *argv[])
{
	if (!argv[0] || !strlen(argv[0]))
		return -1;

	if (!strcmp(argv[0], "help")) {
		pr_crit("\n\nEDMA dbg cmds usage:\n");
		pr_crit("1. echo edma dump free_count > /proc/%s/%s\n ==> dump tx ring's free count\n",
			PROCREG_DIR, PROCREG_DBG_DMA);
		pr_crit("2. echo edma dump recycle_mode > /proc/%s/%s\n ==> dump recycle mode related info\n",
			PROCREG_DIR, PROCREG_DBG_DMA);
		pr_crit("3. echo edma dump flow_ctrl > /proc/%s/%s\n ==> dump suspend_cnt and resume_cnt\n",
			PROCREG_DIR, PROCREG_DBG_DMA);
		pr_crit("4. echo edma dump ring_status > /proc/%s/%s\n ==> dump edma ring cidx and didx\n",
			PROCREG_DIR, PROCREG_DBG_DMA);
		pr_crit("5. echo edma dump rx_ring_max_dist > /proc/%s/%s\n ==> dump edma rx ring max distance\n",
			PROCREG_DIR, PROCREG_DBG_DMA);
		pr_crit("\n\n");
	} else if (!strcmp(argv[0], "dump")) {
		if (!argv[1] || !strlen(argv[1])) {
			pr_crit("EDMA dbg cmds: dump cmds without target\n");
			return -1;
		}
		if (!strcmp(argv[1], "free_count")) {
			int edma_idx, tx_ring;

			for (edma_idx = 0; edma_idx < EDMA_MODULE_NUM; edma_idx++) {
				for (tx_ring = 0; tx_ring < MTK_MAX_EDMA_TX_RING_NUM; tx_ring++) {
					pr_crit("edma-%d tx_ring-%d free_count: %d\n", edma_idx, tx_ring,
						atomic_read(&g_edma->tx_ring[edma_idx][tx_ring].free_count));
				}
			}
		} else if (!strcmp(argv[1], "recycle_mode")) {
			int edma_idx, ring_idx;

			for (edma_idx = 0; edma_idx < EDMA_MODULE_NUM; edma_idx++) {
				if (g_edma->fe_dma[edma_idx]) {
					pr_crit("edma-%d is%s in rx recycle mode\n",
                                        	edma_idx,
	                                        g_edma->fe_dma[edma_idx]->rx_recycle_mode ?
                                                "" : " not");
					for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_RX_RING_NUM; ring_idx++) {
						pr_crit("rx_ring-%d has %d free bufs, %d recycle bufs\n",
							ring_idx,
							g_edma->rx_ring[edma_idx][ring_idx].free_buf_cnt,
							g_edma->rx_ring[edma_idx][ring_idx].recycle_buf_cnt);
					}
				}
			}
		} else if (!strcmp(argv[1], "flow_ctrl")) {
			int edma_idx, ring_idx;

			for (edma_idx = 0; edma_idx < EDMA_MODULE_NUM; edma_idx++) {
				if (g_edma->fe_dma[edma_idx]) {
					for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_RX_RING_NUM; ring_idx++) {
						/* Currently rx1_ring1 is not used */
						if (edma_idx == EDMA_IDX1 && ring_idx == 1)
							continue;

						pr_crit(
						"emda-%d rx_ring-%d suspend_cnt:%d, resume_cnt:%d\n",
						edma_idx, ring_idx,
						g_edma->rx_ring[edma_idx][ring_idx].suspend_cnt,
						g_edma->rx_ring[edma_idx][ring_idx].resume_cnt
						);
						//clear to 0
						g_edma->rx_ring[edma_idx][ring_idx].suspend_cnt = 0;
						g_edma->rx_ring[edma_idx][ring_idx].resume_cnt = 0;
					}
				}
			}
		} else if (!strcmp(argv[1], "ring_status")) {
			int edma_idx, ring_idx, cidx, didx;

			pr_crit("edma tx ring status (ring size %d)\n", EDMA_TX_DMA_SIZE);
			/* Currently ony use tx0_ring0 and tx1_ring0 */
			for (edma_idx = 0; edma_idx <= EDMA_IDX1; edma_idx++) {
				for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_TX_RING_NUM; ring_idx++) {
					cidx = g_edma->tx_ring[edma_idx][ring_idx].cpu_idx;
					didx = mtk_edma_r32(g_edma, ETX_DTX_IDX_CFG(edma_idx, ring_idx));

					pr_crit("edma-%d tx ring-%d, cidx: %d, didx = %d, distance = %d\n",
						edma_idx, ring_idx, cidx, didx,
						(didx + EDMA_TX_DMA_SIZE - cidx) % EDMA_TX_DMA_SIZE);
				}
			}
			pr_crit("\n");
			pr_crit("edma rx ring status (ring size %d)\n", MTK_HW_EDMA_RX_DMA_SIZE_RING1);
			/* Currently ony use rx0_ring0, rx1_ring0 and rx0_ring1 */
			for (edma_idx = 0; edma_idx <= EDMA_IDX1; edma_idx++) {
				for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_RX_RING_NUM; ring_idx++) {
					/* Currently rx1_ring1 is not used */
					if (edma_idx == EDMA_IDX1 && ring_idx == 1)
						continue;

					cidx = g_edma->rx_ring[edma_idx][ring_idx].cpu_idx;
					didx = mtk_edma_r32(g_edma, ERX_DRX_IDX_CFG(edma_idx, ring_idx));

					pr_crit("edma-%d rx ring-%d, cidx: %d, didx = %d, distance = %d\n",
						edma_idx, ring_idx, cidx, didx,
						(didx + MTK_HW_EDMA_RX_DMA_SIZE_RING1 - cidx - 1) % MTK_HW_EDMA_RX_DMA_SIZE_RING1);
				}
			}
			pr_crit("\n");
		} else if (!strcmp(argv[1], "rx_ring_max_dist")) {
			int edma_idx, ring_idx;

			pr_crit("edma rx ring max distance (ring size %d)\n", MTK_HW_EDMA_RX_DMA_SIZE_RING1);
			/* Currently ony use rx0_ring0, rx1_ring0 and rx0_ring1 */
			for (edma_idx = 0; edma_idx <= EDMA_IDX1; edma_idx++) {
				for (ring_idx = 0; ring_idx < MTK_MAX_EDMA_RX_RING_NUM; ring_idx++) {
					/* Currently rx1_ring1 is not used */
					if (edma_idx == EDMA_IDX1 && ring_idx == 1)
						continue;

					pr_crit("edma-%d rx ring-%d, max_distance = %d\n",
						edma_idx, ring_idx, g_edma->rx_ring[edma_idx][ring_idx].max_distance);

					//clear to 0
					g_edma->rx_ring[edma_idx][ring_idx].max_distance = 0;
				}
			}
			pr_crit("\n");
		} else {
			pr_crit("EDMA dbg cmds: invalid dump target\n");
		}
	} else if (!strcmp(argv[0], "conf")) {
		if (!argv[1] || !strlen(argv[1])) {
			pr_crit("EDMA dbg cmds: conf cmds without target\n");
			return -1;
		}
	}

	return -1;
}

int mtk_fe_edma_probe(struct platform_device *pdev,
		fe_dma_open_fn *fn, fe_dma_pm_fn *pm_fn,
		fe_dma_dbg_fn *dbg_fn)
{
	if (!g_edma) {
		g_edma = devm_kzalloc(&pdev->dev, sizeof(*g_edma), GFP_KERNEL);
		if (!g_edma)
			return -ENOMEM;

		g_edma->dev = &pdev->dev;
		g_edma->base = platform_get_drvdata(pdev);
		spin_lock_init(&g_edma->irq_lock[EDMA_IDX0]);
		spin_lock_init(&g_edma->irq_lock[EDMA_IDX1]);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		spin_lock_init(&g_edma->irq_lock[EDMA_IDX2]);
#endif
		edma_request_irq(pdev, "netsys-edma0",
				edma_handle_rx_irq0, (void *)EDMA_IDX0);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		edma_request_irq(pdev, "netsys-edma0-r1",
				edma_handle_rx_irq1, (void *)EDMA_IDX0);
#endif
		edma_request_irq(pdev, "netsys-edma1",
				edma_handle_rx_irq0, (void *)EDMA_IDX1);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		edma_request_irq(pdev, "netsys-edma1-r1",
				edma_handle_rx_irq1, (void *)EDMA_IDX1);
#endif
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		edma_request_irq(pdev, "netsys-edma2",
				edma_handle_rx_irq0, (void *)EDMA_IDX2);
		edma_request_irq(pdev, "netsys-edma2-r1",
				edma_handle_rx_irq1, (void *)EDMA_IDX2);
#endif
		edma_request_irq(pdev, "netsys-edma0-g1",
				edma_handle_tx_irq, (void *)EDMA_IDX0);
		edma_request_irq(pdev, "netsys-edma1-g1",
				edma_handle_tx_irq, (void *)EDMA_IDX1);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		edma_request_irq(pdev, "netsys-edma2-g1",
				edma_handle_tx_irq, (void *)EDMA_IDX2);
#endif
	}

	*fn = fe_edma_open;
	*pm_fn = fe_edma_pm_handle;
	*dbg_fn = fe_edma_dbg;
	return 0;
}
