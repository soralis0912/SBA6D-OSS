// SPDX-License-Identifier: GPL-2.0-only

#include <linux/soc/mediatek/mtk_wed.h>
#include <linux/of_address.h>
#include <linux/mfd/syscon.h>
#include <linux/of_irq.h>
#include "mtk_wed_ccif.h"
#include "mtk_wed_regs.h"
#include "mtk_wed_wo.h"

static inline void woif_set_isr(struct mtk_wed_wo *wo, u32 mask)
{
	woccif_w32(wo, MTK_WED_WO_CCIF_IRQ0_MASK, mask);
}

static inline u32 woif_get_csr(struct mtk_wed_wo *wo)
{
	u32 val;

	val = woccif_r32(wo, MTK_WED_WO_CCIF_RCHNUM);

	return  val & MTK_WED_WO_CCIF_RCHNUM_MASK;
}

static inline void woif_set_ack(struct mtk_wed_wo *wo, u32 mask)
{
	woccif_w32(wo, MTK_WED_WO_CCIF_ACK, mask);
}

static inline void woif_kickout(struct mtk_wed_wo *wo)
{
	woccif_w32(wo, MTK_WED_WO_CCIF_BUSY, 1 << MTK_WED_WO_TXCH_NUM);
	woccif_w32(wo, MTK_WED_WO_CCIF_TCHNUM, MTK_WED_WO_TXCH_NUM);
}

static inline void woif_clear_int(struct mtk_wed_wo *wo, u32 mask)
{
	woccif_w32(wo, MTK_WED_WO_CCIF_ACK, mask);
	woccif_r32(wo, MTK_WED_WO_CCIF_RCHNUM);
}

int wed_wo_hardware_init(struct mtk_wed_wo *wo, irq_handler_t isr)
{
	static const struct wed_wo_drv_ops wo_drv_ops = {
		.kickout = woif_kickout,
		.set_ack = woif_set_ack,
		.set_isr = woif_set_isr,
		.get_csr = woif_get_csr,
		.clear_int = woif_clear_int,
	};
	struct device_node *np, *node = wo->hw->node;
	struct wed_wo_queue_regs queues;
	struct regmap *regs;
	int ret;

	np = of_parse_phandle(node, "mediatek,ap2woccif", 0);
	if (!np)
		return -ENODEV;

	regs = syscon_regmap_lookup_by_phandle(np, NULL);
	if (!regs)
		return -ENODEV;

	wo->drv_ops = &wo_drv_ops;

	wo->ccif.regs = regs;
	wo->ccif.irq = irq_of_parse_and_map(np, 0);

	spin_lock_init(&wo->ccif.irq_lock);

	ret = request_irq(wo->ccif.irq, isr, IRQF_TRIGGER_HIGH,
			  "wo_ccif_isr", wo);
	if (ret)
		goto free_irq;

	queues.desc_base = MTK_WED_WO_CCIF_DUMMY1;
	queues.ring_size = MTK_WED_WO_CCIF_DUMMY2;
	queues.cpu_idx = MTK_WED_WO_CCIF_DUMMY3;
	queues.dma_idx = MTK_WED_WO_CCIF_SHADOW4;

	ret = mtk_wed_wo_q_alloc(wo, &wo->q_tx, MTK_WED_WO_RING_SIZE,
				 MTK_WED_WO_CMD_LEN, MTK_WED_WO_TXCH_NUM,
				 &queues);

	if (ret)
		goto free_irq;

	queues.desc_base = MTK_WED_WO_CCIF_DUMMY5;
	queues.ring_size = MTK_WED_WO_CCIF_DUMMY6;
	queues.cpu_idx = MTK_WED_WO_CCIF_DUMMY7;
	queues.dma_idx = MTK_WED_WO_CCIF_SHADOW8;

	ret = mtk_wed_wo_q_alloc(wo, &wo->q_rx, MTK_WED_WO_RING_SIZE,
				 MTK_WED_WO_CMD_LEN, MTK_WED_WO_RXCH_NUM,
				 &queues);
	if (ret)
		goto free_irq;

	wo->ccif.q_int_mask = MTK_WED_WO_RXCH_INT_MASK;

	ret = mtk_wed_wo_q_init(wo, mtk_wed_wo_rx_poll);
	if (ret)
		goto free_irq;

	wo->ccif.q_exep_mask = MTK_WED_WO_EXCEPTION_INT_MASK;
	wo->ccif.irqmask = MTK_WED_WO_ALL_INT_MASK;

	/* rx queue irqmask */
	wo->drv_ops->set_isr(wo, wo->ccif.irqmask);

	return 0;

free_irq:
	free_irq(wo->ccif.irq, wo);

	return ret;
}

void wed_wo_hardware_exit(struct mtk_wed_wo *wo)
{
	wo->drv_ops->set_isr(wo, 0);

	disable_irq(wo->ccif.irq);
	free_irq(wo->ccif.irq, wo);

	tasklet_disable(&wo->irq_tasklet);
	netif_napi_del(&wo->napi);

	mtk_wed_wo_q_tx_clean(wo, &wo->q_tx);
	mtk_wed_wo_q_rx_clean(wo, &wo->q_rx);
	mtk_wed_wo_q_free(wo, &wo->q_tx);
	mtk_wed_wo_q_free(wo, &wo->q_rx);
}
