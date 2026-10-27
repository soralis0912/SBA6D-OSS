// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <linux/bitfield.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/iopoll.h>
#include <linux/soc/mediatek/mtk_wed.h>
#include "mtk_wed.h"
#include "mtk_wed_regs.h"
#include "mtk_wed_ccif.h"
#include "mtk_wed_wo.h"
#include "mtk_wed_mcu.h"

struct wed_wo_profile_stat profile_total[6] = {
	{1001, 0},
	{1501, 0},
	{3001, 0},
	{5001, 0},
	{10001, 0},
	{0xffffffff, 0}
};

struct wed_wo_profile_stat profiling_mod[6] = {
	{1001, 0},
	{1501, 0},
	{3001, 0},
	{5001, 0},
	{10001, 0},
	{0xffffffff, 0}
};

struct wed_wo_profile_stat profiling_rro[6] = {
	{1001, 0},
	{1501, 0},
	{3001, 0},
	{5001, 0},
	{10001, 0},
	{0xffffffff, 0}
};

static void
woif_q_sync_idx(struct mtk_wed_wo *wo, struct wed_wo_queue *q)
{
	woccif_w32(wo, q->regs->desc_base, q->desc_dma);
	woccif_w32(wo, q->regs->ring_size, q->ndesc);

}

static void
woif_q_reset(struct mtk_wed_wo *dev, struct wed_wo_queue *q)
{

	if (!q || !q->ndesc)
		return;

	woccif_w32(dev, q->regs->cpu_idx, 0);

	woif_q_sync_idx(dev, q);
}

static void
woif_q_kick(struct mtk_wed_wo *wo, struct wed_wo_queue *q, int offset)
{
	wmb();
	woccif_w32(wo, q->regs->cpu_idx, q->head + offset);
}

static int
woif_q_rx_fill(struct mtk_wed_wo *wo, struct wed_wo_queue *q, bool rx)
{
	int len = q->buf_size, frames = 0;
	struct wed_wo_queue_entry *entry;
	struct page_frag_cache *page = &q->tx_page;
	struct wed_wo_desc *desc;
	dma_addr_t addr;
	u32 ctrl = 0;
	void *buf;

	if (!q->ndesc)
		return 0;

	spin_lock_bh(&q->lock);

	if(rx)
		page = &q->rx_page;

	while (q->queued < q->ndesc) {
		buf = page_frag_alloc(page, len, GFP_ATOMIC);
		if (!buf)
			break;

		addr = dma_map_single(wo->hw->dev, buf, len, DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(wo->hw->dev, addr))) {
			skb_free_frag(buf);
			break;
		}

		q->head = (q->head + 1) % q->ndesc;

		desc = &q->desc[q->head];
		entry = &q->entry[q->head];

		entry->dma_addr = addr;
		entry->dma_len = len;

		if (rx) {
			ctrl = FIELD_PREP(WED_CTL_SD_LEN0, entry->dma_len);
			ctrl |= WED_CTL_LAST_SEC0;

			WRITE_ONCE(desc->buf0, cpu_to_le32(addr));
			WRITE_ONCE(desc->ctrl, cpu_to_le32(ctrl));
		}
		q->queued++;
		q->entry[q->head].buf = buf;

		frames++;
	}

	spin_unlock_bh(&q->lock);

	return frames;
}

static void
woif_q_rx_fill_process(struct mtk_wed_wo *wo, struct wed_wo_queue *q)
{
	if(woif_q_rx_fill(wo, q, true))
		woif_q_kick(wo, q, -1);
}

static int
woif_q_alloc(struct mtk_wed_wo *dev, struct wed_wo_queue *q,
		     int n_desc, int bufsize, int idx,
		     struct wed_wo_queue_regs *regs)
{
	struct wed_wo_queue_regs *q_regs;
	int size;

	spin_lock_init(&q->lock);
	spin_lock_init(&q->cleanup_lock);

	q_regs = devm_kzalloc(dev->hw->dev, sizeof(*q_regs), GFP_KERNEL);

	q_regs->desc_base = regs->desc_base;
	q_regs->ring_size = regs->ring_size;
	q_regs->cpu_idx = regs->cpu_idx;
	q_regs->dma_idx = regs->dma_idx;

	q->regs = q_regs;
	q->ndesc = n_desc;
	q->buf_size = bufsize;

	size = q->ndesc * sizeof(struct wed_wo_desc);

	q->desc = dmam_alloc_coherent(dev->hw->dev, size,
				      &q->desc_dma, GFP_KERNEL);
	if (!q->desc)
		return -ENOMEM;

	size = q->ndesc * sizeof(*q->entry);
	q->entry = devm_kzalloc(dev->hw->dev, size, GFP_KERNEL);
	if (!q->entry)
		return -ENOMEM;

	if (idx == 0) {
		/* alloc tx buf */
		woif_q_rx_fill(dev, &dev->q_tx, false);
		woif_q_reset(dev, &dev->q_tx);
	}

	return 0;
}

static void
woif_q_free(struct mtk_wed_wo *dev, struct wed_wo_queue *q)
{
	int size;

	if (!q)
		return;

	if (!q->desc)
		return;

	woccif_w32(dev, q->regs->cpu_idx, 0);

	size = q->ndesc * sizeof(struct wed_wo_desc);
	dma_free_coherent(dev->hw->dev, size, q->desc, q->desc_dma);
}

static void
woif_q_tx_clean(struct mtk_wed_wo *wo, struct wed_wo_queue *q)
{
	struct page *page;
	int i = 0;

	if (!q || !q->ndesc)
		return;

	spin_lock_bh(&q->lock);
	while (i < q->ndesc) {
		struct wed_wo_queue_entry *e;

		e = &q->entry[i];
		i++;

		if (!e)
			continue;
		dma_unmap_single(wo->hw->dev, e->dma_addr, e->dma_len,
				 DMA_TO_DEVICE);

		skb_free_frag(e->buf);
	}
	spin_unlock_bh(&q->lock);

	if (!q->tx_page.va)
		return;

	page = virt_to_page(q->tx_page.va);
	__page_frag_cache_drain(page, q->tx_page.pagecnt_bias);
	memset(&q->tx_page, 0, sizeof(q->tx_page));
}

static void *
woif_q_deq(struct mtk_wed_wo *wo, struct wed_wo_queue *q, bool flush,
		 int *len, u32 *info, bool *more)
{
	int buf_len = SKB_WITH_OVERHEAD(q->buf_size);
	struct wed_wo_queue_entry *e;
	struct wed_wo_desc *desc;
	int idx = (q->tail + 1) % q->ndesc;;
	void *buf;

	*more = false;
	if (!q->queued) {
		pr_err("%s: q->queued is null\n", __func__);
		return NULL;
	}

	if (flush)
		q->desc[idx].ctrl |= cpu_to_le32(WED_CTL_DMA_DONE);
	else if (!(q->desc[idx].ctrl & cpu_to_le32(WED_CTL_DMA_DONE)))
		return NULL;

	q->tail = idx;
	q->queued--;

	desc = &q->desc[idx];
	e = &q->entry[idx];

	buf = e->buf;
	if (len) {
		u32 ctl = le32_to_cpu(READ_ONCE(desc->ctrl));
		*len = FIELD_GET(WED_CTL_SD_LEN0, ctl);
		*more = !(ctl & WED_CTL_LAST_SEC0);
	}

	if (info)
		*info = le32_to_cpu(desc->info);
	if(buf)
		dma_unmap_single(wo->hw->dev, e->dma_addr, buf_len,
				 DMA_FROM_DEVICE);
	e->skb = NULL;

	return buf;
}

static void
woif_q_rx_clean(struct mtk_wed_wo *wo, struct wed_wo_queue *q)
{
	struct page *page;
	int *buf;
	bool more;

	if (!q->ndesc)
		return;

	spin_lock_bh(&q->lock);
	do {
		buf = woif_q_deq(wo, q, true, NULL, NULL, &more);
		if (!buf)
			break;

		skb_free_frag(buf);
	} while (1);
	spin_unlock_bh(&q->lock);

	if (!q->rx_page.va)
		return;

	page = virt_to_page(q->rx_page.va);
	__page_frag_cache_drain(page, q->rx_page.pagecnt_bias);
	memset(&q->rx_page, 0, sizeof(q->rx_page));

}

static int
woif_q_init(struct mtk_wed_wo *dev,
	       int (*poll)(struct napi_struct *napi, int budget))
{
	init_dummy_netdev(&dev->napi_dev);
	snprintf(dev->napi_dev.name, sizeof(dev->napi_dev.name), "%s",
		 "woif_q");

	if (dev->q_rx.ndesc) {
		netif_napi_add(&dev->napi_dev, &dev->napi, poll, 64);
		woif_q_rx_fill(dev, &dev->q_rx, true);
		woif_q_reset(dev, &dev->q_rx);
		napi_enable(&dev->napi);
	}

	return 0;
}

void woif_q_rx_skb(struct mtk_wed_wo *wo, struct sk_buff *skb)
{
	struct wed_cmd_hdr *hdr = (struct wed_cmd_hdr *)skb->data;
	int ret;

	ret = mtk_wed_mcu_cmd_sanity_check(wo, skb);
	if (ret)
		goto free_skb;

	if (WED_WO_CMD_FLAG_IS_RSP(hdr))
		mtk_wed_mcu_rx_event(wo, skb);
	else
		mtk_wed_mcu_rx_unsolicited_event(wo, skb);

	return;
free_skb:
	dev_kfree_skb(skb);
}

static int
woif_q_tx_skb(struct mtk_wed_wo *wo, struct wed_wo_queue *q,
		      struct sk_buff *skb)
{
	struct wed_wo_queue_entry *entry;
	struct wed_wo_desc *desc;
	int len, ret = 0, idx = -1;
	dma_addr_t addr;
	u32 ctrl = 0;

	len = skb->len;
	spin_lock_bh(&q->lock);

	q->tail = woccif_r32(wo, q->regs->dma_idx);
	q->head = (q->head + 1) % q->ndesc;
	if (q->tail == q->head) {
		ret = -ENOMEM;
		goto error;
	}

	idx = q->head;
	desc = &q->desc[idx];
	entry = &q->entry[idx];

	if (len > entry->dma_len) {
		ret = -ENOMEM;
		goto error;
	}
	addr = entry->dma_addr;

	dma_sync_single_for_cpu(wo->hw->dev, addr, len, DMA_TO_DEVICE);
	memcpy(entry->buf, skb->data, len);
	dma_sync_single_for_device(wo->hw->dev, addr, len, DMA_TO_DEVICE);

	ctrl = FIELD_PREP(WED_CTL_SD_LEN0, len);
	ctrl |= WED_CTL_LAST_SEC0;
	ctrl |= WED_CTL_DMA_DONE;

	WRITE_ONCE(desc->buf0, cpu_to_le32(addr));
	WRITE_ONCE(desc->ctrl, cpu_to_le32(ctrl));

	woif_q_kick(wo, q, 0);
	wo->drv_ops->kickout(wo);

	spin_unlock_bh(&q->lock);
	return 0;

error:
	dev_kfree_skb(skb);
	return -ENOMEM;
}

static const struct wed_wo_queue_ops wo_queue_ops = {
	.init = woif_q_init,
	.alloc = woif_q_alloc,
	.free = woif_q_free,
	.reset = woif_q_reset,
	.tx_skb = woif_q_tx_skb,
	.tx_clean = woif_q_tx_clean,
	.rx_clean = woif_q_rx_clean,
	.kick = woif_q_kick,
};

static int
mtk_wed_wo_rx_process(struct mtk_wed_wo *wo, struct wed_wo_queue *q, int budget)
{
	int len, done = 0;
	struct sk_buff *skb;
	unsigned char *data;
	bool more;

	while (done < budget) {
		u32 info;

		data = woif_q_deq(wo, q, false, &len, &info, &more);
		if (!data)
			break;

		skb = build_skb(data, q->buf_size);
		if (!skb) {
			skb_free_frag(data);
			continue;
		}

		__skb_put(skb, len);
		done++;

		woif_q_rx_skb(wo, skb);
	}

	woif_q_rx_fill_process(wo, q);

	return done;
}

void mtk_wed_wo_set_isr_mask(struct mtk_wed_wo *wo, bool set,
		       u32 clear, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&wo->ccif.irq_lock, flags);
	wo->ccif.irqmask &= ~clear;
	wo->ccif.irqmask |= val;
	if (set)
		wo->drv_ops->set_isr(wo, wo->ccif.irqmask);

	spin_unlock_irqrestore(&wo->ccif.irq_lock, flags);
}

static inline void mtk_wed_wo_set_ack_mask(struct mtk_wed_wo *wo, u32 mask)
{
	wo->drv_ops->set_ack(wo, mask);
}

static void mtk_wed_wo_poll_complete(struct mtk_wed_wo *wo)
{
	mtk_wed_wo_set_ack_mask(wo, wo->ccif.q_int_mask);
	mtk_wed_wo_isr_enable(wo, wo->ccif.q_int_mask);
}

int mtk_wed_wo_rx_poll(struct napi_struct *napi, int budget)
{
	struct mtk_wed_wo *wo;
	int done = 0, cur;

	wo = container_of(napi->dev, struct mtk_wed_wo, napi_dev);

	rcu_read_lock();

	do {
		cur = mtk_wed_wo_rx_process(wo, &wo->q_rx, budget - done);
		/* rx packet handle */
		done += cur;
	} while (cur && done < budget);

	rcu_read_unlock();

	if (done < budget && napi_complete(napi))
		mtk_wed_wo_poll_complete(wo);

	return done;
}

static void mtk_wed_wo_isr_tasklet(unsigned long data)
{
	struct mtk_wed_wo *wo = (struct mtk_wed_wo *)data;
	u32 intr, mask;

	/* disable isr */
	wo->drv_ops->set_isr(wo, 0);

	intr = wo->drv_ops->get_csr(wo);
	intr &= wo->ccif.irqmask;

	mask = intr & (wo->ccif.q_int_mask | wo->ccif.q_exep_mask);
	mtk_wed_wo_isr_disable(wo, mask);

	if (intr & wo->ccif.q_int_mask)
		napi_schedule(&wo->napi);

	if (intr & wo->ccif.q_exep_mask) {
		/* todo */
	}
}

static irqreturn_t mtk_wed_wo_isr_handler(int irq, void *wo_instance)
{
	struct mtk_wed_wo *wo = wo_instance;

	wo->drv_ops->set_isr(wo, 0);

	tasklet_schedule(&wo->irq_tasklet);

	return IRQ_HANDLED;
}

int mtk_wed_wo_init(struct mtk_wed_hw *hw)
{
	struct mtk_wed_wo *wo;
	int ret = 0;

	wo = kzalloc(sizeof(struct mtk_wed_wo), GFP_KERNEL);
	if (!wo)
		return -ENOMEM;

	wo->hw = hw;
	wo->queue_ops = &wo_queue_ops;
	hw->wed_wo = wo;

	tasklet_init(&wo->irq_tasklet, mtk_wed_wo_isr_tasklet,
		     (unsigned long)wo);

	skb_queue_head_init(&wo->mcu.res_q);
	init_waitqueue_head(&wo->mcu.wait);
	mutex_init(&wo->mcu.mutex);

	ret = wed_wo_hardware_init(wo, mtk_wed_wo_isr_handler);
	if (ret)
		goto error;

	/* fw download */
	ret = wed_wo_mcu_init(wo);
	if (ret)
		goto error;

	ret = mtk_wed_exception_init(wo);
	if (ret)
		goto error;

	return ret;

error:
	kfree(wo);

	return ret;
}

void mtk_wed_wo_exit(struct mtk_wed_hw *hw)
{
	struct mtk_wed_wo *wo = hw->wed_wo;

	wed_wo_hardware_exit(wo);

	if (wo->exp.log) {
		dma_unmap_single(wo->hw->dev, wo->exp.phys, wo->exp.log_size, DMA_FROM_DEVICE);
		kfree(wo->exp.log);
	}

	wo->hw = NULL;
	memset(wo, 0, sizeof(*wo));
	kfree(wo);
}
