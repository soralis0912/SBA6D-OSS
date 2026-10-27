// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 MediaTek Inc.
/*
 * Author: Kurt Yang <kurt.yang@mediatek.com>
 *
 */
#include <linux/if_vlan.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/tcp.h>

#include <net/mtk/mtk_frame_engine.h>
#include <net/mtk/mtk_fe_dma.h>


#define MTK_FE_DMA_NAPI_WEIGHT		256

extern int fe_dbg_level;

struct fe_dma_moules {
	int (*probe)(struct platform_device *pdev,
			fe_dma_open_fn *fn,
			fe_dma_pm_fn *pm_fn,
			fe_dma_dbg_fn *dbg_fn);
	int (*open)(int channel, struct mtk_fe_dma **dma,
			u32 tx_ring_num, u32 rx_ring_num);
	int (*pm_handle)(int channel, bool suspend);
	int (*debug)(char *argv[]);
	struct mtk_fe_dma *fe_dma;
};

static struct fe_dma_moules _dma_modules[] = {
	[MTK_FE_EDMA_0] = {
		.probe = mtk_fe_edma_probe,
		.open = 0,
		.pm_handle = 0,
		.debug = 0,
		.fe_dma = NULL
	},
	[MTK_FE_EDMA_1] = {
		.probe = mtk_fe_edma_probe,
		.open = 0,
		.pm_handle = 0,
		.debug = 0,
		.fe_dma = NULL
	},
	[MTK_FE_EDMA_2] = {
		.probe = mtk_fe_edma_probe,
		.open = 0,
		.pm_handle = 0,
		.debug = 0,
		.fe_dma = NULL
	},
};

/*
 * Internal dummy net_device used for NAPI.
 */
static struct net_device dma_dummy_dev;

static int dma_napi_rx_0(struct napi_struct *napi, int budget)
{
	struct mtk_fe_dma *dma = container_of(napi, struct mtk_fe_dma, rx_napi_0);

	return dma->do_poll(dma, budget, 0);
}

static int dma_napi_rx_1(struct napi_struct *napi, int budget)
{
	struct mtk_fe_dma *dma = container_of(napi, struct mtk_fe_dma, rx_napi_1);

	return dma->do_poll(dma, budget, 1);
}

static int dma_napi_tx(struct napi_struct *napi, int budget)
{
	struct mtk_fe_dma *dma = container_of(napi, struct mtk_fe_dma, tx_napi);
	int tx_done = dma->do_tx_housekeeping(dma, budget);

	if (dma->netdev) {
		//netdev_completed_queue(eth->netdev[i], done[i], bytes[i]);
		if (netif_queue_stopped(dma->netdev)
				&& !dma->is_tx_ring_full(dma, 1)) {
			//netif_wake_queue(dma->netdev);
			netif_start_queue(dma->netdev);
		}
	}

	return tx_done;
}

static int dma_skb_txdone_handler(void *buf_obj)
{
	struct sk_buff *skb = (struct sk_buff *)buf_obj;

	dev_kfree_skb_any(skb);
	return 0;
}

static int dma_raw_txdone_handler(void *buf_obj)
{
	kfree(buf_obj);
	return 0;
}

static inline int dma_cal_txd_req(struct mtk_fe_dma *dma, struct sk_buff *skb)
{
	int i, nfrags, frag_size;
	skb_frag_t *frag;

	nfrags = 1;
	if (skb_is_gso(skb)) {
		for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
			frag = &skb_shinfo(skb)->frags[i];
			frag_size = skb_frag_size(frag);
			nfrags += DIV_ROUND_UP(frag_size, dma->max_tx_buf_len);
		}
	} else {
		nfrags += skb_shinfo(skb)->nr_frags;
	}

	return nfrags;
}

int recycle_fe_dma_skb(struct sk_buff *skb)
{
	struct dma_rx_buf *buf = container_of(
				(void *)skb->head, struct dma_rx_buf, data);

	return buf->dma->recycle_buf(buf->dma, buf);
}

int recycle_fe_dma_buf(void *_buf)
{
	struct dma_rx_buf *buf;

	_buf -= NET_SKB_PAD;
	_buf -= NET_IP_ALIGN;
	buf = container_of(_buf, struct dma_rx_buf, data);
	return buf->dma->recycle_buf(buf->dma, buf);
}
EXPORT_SYMBOL(recycle_fe_dma_buf);

int fe_dma_suspend_buf(u32 dma_channel, u32 ring_no)
{
	struct mtk_fe_dma *dma = _dma_modules[dma_channel].fe_dma;

	return dma->suspend_buf(dma, ring_no);
}
EXPORT_SYMBOL(fe_dma_suspend_buf);


int fe_dma_resume_buf(u32 dma_channel, u32 ring_no)
{
	struct mtk_fe_dma *dma = _dma_modules[dma_channel].fe_dma;

	return dma->resume_buf(dma, ring_no);
}
EXPORT_SYMBOL(fe_dma_resume_buf);

int fe_dma_xmit(struct mtk_fe_dma *dma, void *data, int len,
		u16 vlan_tci, void *buf_obj)
{
	if (unlikely(dma->is_tx_ring_full && dma->is_tx_ring_full(dma, 1))) {
		pr_err("[%s] Tx Ring full!\n", __func__);
		return NETDEV_TX_BUSY;
	}

	return dma->do_xmit(dma, data, len, 0, 0, 0 /* tso */,
		0 /* is_checksum_offload */, vlan_tci, buf_obj);
}
EXPORT_SYMBOL(fe_dma_xmit);

int fe_dma_xmit_skb(struct mtk_fe_dma *dma, struct sk_buff *skb,
		void *buf_obj, u16 vlan_tci)
{
	u8 is_checksum_offload, gso;
	int tx_num, ret;

	gso = 0;
	ret = 0;
	tx_num = dma_cal_txd_req(dma, skb);
	if (unlikely(dma->is_tx_ring_full
			&& dma->is_tx_ring_full(dma, tx_num))) {
		pr_err("[%s] Tx %d Ring full! tx_num = %d\n", __func__, dma->dma_channel, tx_num);
		return NETDEV_TX_BUSY;
	}

	/* TSO: fill MSS info in tcp checksum field */
	if (skb_is_gso(skb)) {
		if (skb_cow_head(skb, 0)) {
			pr_warn("[%s] GSO expand head fail.\n", __func__);
			goto drop;
		}

		if (skb_shinfo(skb)->gso_type &
				(SKB_GSO_TCPV4 | SKB_GSO_TCPV6)) {
			gso = true;
			tcp_hdr(skb)->check = htons(skb_shinfo(skb)->gso_size);
		}
	}

	is_checksum_offload = skb->ip_summed == CHECKSUM_PARTIAL;

	ret = dma->do_xmit(dma,
					skb->data,
					skb_headlen(skb),
					skb_shinfo(skb)->frags,
					skb_shinfo(skb)->nr_frags,
					gso,
					is_checksum_offload,
					vlan_tci ? vlan_tci : skb->vlan_tci,
					buf_obj);
	if (ret) {
		if (fe_dbg_level >= 5)
			pr_err("%s[%d], do_xmit fail\n", __func__, __LINE__);

		goto drop;
	}

	skb_tx_timestamp(skb);

/* Kurt: to be implemented....
 *		if (unlikely(atomic_read(&ring->free_count) <= ring->thresh))
 *			mtk_stop_queue(eth);
 *
 *		spin_unlock(&eth->page_lock);
 */
	return NETDEV_TX_OK;

drop:
//	spin_unlock(&eth->page_lock);
//	stats->tx_dropped++;
	dev_kfree_skb_any(skb);
	return ret;
}
EXPORT_SYMBOL(fe_dma_xmit_skb);

static int _netdev_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mtk_fe_dma **p = netdev_priv(dev);
	struct mtk_fe_dma *dma = *p;
	int ret;

	ret =  fe_dma_xmit_skb(dma, skb, (void *)skb, 0);
	if (likely(!ret)) {		//NETDEV_TX_OK
		netdev_sent_queue(dev, skb->len);
	} else if (ret == NETDEV_TX_BUSY) {
		netif_stop_queue(dev);
		netif_err(dma, tx_queued, dev,
				  "Tx Ring full when queue awake!\n");
		return NETDEV_TX_BUSY;
	}

	return NETDEV_TX_OK;
}

static const struct net_device_ops dma_netdev_ops = {
	.ndo_start_xmit		= _netdev_start_xmit,
};

static void _setup_dma_netdev(struct net_device *dev)
{
	dev->netdev_ops = &dma_netdev_ops;
	dev->reg_state = NETREG_DUMMY;

	dev->priv_flags |= IFF_NO_QUEUE;

	/* a dummy interface is started by default */
	set_bit(__LINK_STATE_PRESENT, &dev->state);
	set_bit(__LINK_STATE_START, &dev->state);
}

int fe_dma_set_skb_txmode(struct mtk_fe_dma *dma, bool on)
{
	if (on)
		fe_dma_set_notify_cb(dma, MTK_FE_TX_DONE,
				(void *)dma_skb_txdone_handler);
	else
		fe_dma_set_notify_cb(dma, MTK_FE_TX_DONE,
				(void *)dma_raw_txdone_handler);
	return 0;
}
EXPORT_SYMBOL(fe_dma_set_skb_txmode);

int fe_dma_set_rx_recycle(struct mtk_fe_dma *dma, bool on)
{
	if (!dma->recycle_buf)
		return -1; // Not support RX buffer recycle mode.

	dma->rx_recycle_mode = on;
	return 0;
}
EXPORT_SYMBOL(fe_dma_set_rx_recycle);

void fe_dma_hw_cap_conf(struct mtk_fe_dma *dma, u32 caps, bool on)
{
	dma->do_hw_conf(dma, caps, on);
}
EXPORT_SYMBOL(fe_dma_hw_cap_conf);

int fe_dma_setup_tx_netdev(struct mtk_fe_dma *dma,
		struct net_device **dma_netdev)
{
	struct mtk_fe_dma **ptr;

	if (!dma->netdev) {
		dma->netdev = alloc_netdev(
				sizeof(struct mtk_fe_dma *),
				"",
				NET_NAME_UNKNOWN,
				_setup_dma_netdev);

		if(dma->netdev == NULL) {
			pr_err("[error][%s] alloc_netdev fail!\n", __func__);
			return -ENOMEM;
		}

		SET_NETDEV_DEV(dma->netdev, dma->fe->dev);
		ptr = netdev_priv(dma->netdev);
		*ptr = dma;
	}

	*dma_netdev = dma->netdev;
	if (!dma->txdone_handler)
		fe_dma_set_notify_cb(dma, MTK_FE_TX_DONE,
				(void *)dma_skb_txdone_handler);
	return 0;
}

int fe_dma_set_rx_netdev(struct mtk_fe_dma *dma,
		struct net_device *netdev)
{
	dma->rx_netdev = netdev;
	return 0;
}

int fe_dma_set_dest(struct mtk_fe_dma *dma, int dst)
{
	dma->dest_port = dst;
	return 0;
}

int fe_dma_set_notify_cb(struct mtk_fe_dma *dma,
		int event, void *cb_fn, ...)
{
	va_list args;

	va_start(args, cb_fn);
	switch (event) {
	case MTK_FE_RX_DONE:
		dma->rxdone_handler = (int (*)(void*, int, u32, void*))cb_fn;
		dma->rx_handler_data = va_arg(args, void *);
		break;
	case MTK_FE_RX_DONE_SKB:
		dma->rxdone_skb_handler =
			(int (*)(struct sk_buff*, int, int, u32, void*))cb_fn;
		dma->rx_handler_data = va_arg(args, void *);
		break;
	case MTK_FE_TX_DONE:
		dma->txdone_handler = (int (*)(void *))cb_fn;
	default:
		break;
	}
	va_end(args);
	return 0;
}
EXPORT_SYMBOL(fe_dma_set_notify_cb);

int fe_dma_schedule_rx0_poll(struct mtk_fe_dma *dma)
{
	if (likely(napi_schedule_prep(&dma->rx_napi_0))) {
		__napi_schedule(&dma->rx_napi_0);
		return 0;
	}
	return 1;
}

int fe_dma_schedule_rx1_poll(struct mtk_fe_dma *dma)
{
	if (likely(napi_schedule_prep(&dma->rx_napi_1))) {
		__napi_schedule(&dma->rx_napi_1);
		return 0;
	}
	return 1;
}

int fe_dma_schedule_tx_poll(struct mtk_fe_dma *dma)
{
	if (likely(napi_schedule_prep(&dma->tx_napi))) {
		__napi_schedule(&dma->tx_napi);
		return 0;
	}
	return 1;
}

int fe_dma_complete_rx0_poll(struct mtk_fe_dma *dma)
{
	napi_complete(&dma->rx_napi_0);
	return 0;
}

int fe_dma_complete_rx1_poll(struct mtk_fe_dma *dma)
{
	napi_complete(&dma->rx_napi_1);
	return 0;
}

int fe_dma_complete_tx_poll(struct mtk_fe_dma *dma)
{
	napi_complete(&dma->tx_napi);
	return 0;
}

int fe_dma_notify_txdone(struct mtk_fe_dma *dma, void *buf_obj)
{
	if (likely(dma->txdone_handler))
		return dma->txdone_handler(buf_obj);

	pr_err("%s: there is no handler for the buf, it might have leak..\n",
			__func__);
	return 0;
}

//Kurt: should be refactord.
#if defined(CONFIG_HW_NAT)
#include <net/ra_nat.h>
#endif

extern int (*ppe_hook_rx_ext)(struct sk_buff *skb);

static int fe_dma_deliver_skb(struct mtk_fe_dma *dma,
		u8 *data, u16 frag_size, u16 pktlen,
		u8 is_checksum_offload, u16 vlan_tci, u32 foe_info)
{
	struct dma_rx_buf *buf = container_of(
			(void *)data, struct dma_rx_buf, data);
	struct sk_buff *skb = buf->skb;

	if (!dma->rx_recycle_mode) {
		skb = build_skb(data, frag_size);
		if (unlikely(!skb)) {
			if (fe_dbg_level >= 7)
				pr_notice("[%s][%d] build_skb FAIL\n", __func__, __LINE__);
			return -ENOMEM;
		}
		buf->skb = skb;
	} else if (!skb) {
		skb = build_skb(data, frag_size);
		if (unlikely(!skb)) {
			if (fe_dbg_level >= 7)
				pr_notice("[%s][%d] build_skb FAIL\n", __func__, __LINE__);
			return -ENOMEM;
		}
		buf->skb = skb;
	} else {
		skb->head = data;
		skb->data = data;
		skb->len = 0;
		skb->data_len = 0;
		skb_reset_tail_pointer(skb);
	}

	skb_reserve(skb, NET_SKB_PAD + NET_IP_ALIGN);
	skb_put(skb, pktlen);

	if (is_checksum_offload)
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	else
		skb_checksum_none_assert(skb);

	if (vlan_tci)
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vlan_tci);

	if (fe_dbg_level >= 10)
		pr_notice("[%s][%d], vlan_tci = 0x%x\, skb->vlan_tci = 0x%x\n", __func__, __LINE__, vlan_tci, skb->vlan_tci);

	if ((!dma->rxdone_skb_handler) ||
		dma->rxdone_skb_handler(skb, dma->dma_channel, buf->ring_no,
					foe_info, dma->rx_handler_data)) {
		skb->dev = dma->rx_netdev ? dma->rx_netdev : skb->dev;
		WARN_ON(!skb->dev);
		if (unlikely(!dma->rxdone_skb_handler))
			skb->protocol = eth_type_trans(skb, skb->dev);
		skb_record_rx_queue(skb, 0);

		/*
		 * Use same API (netif_rx_ni) as USB Tethering.
		 * if use napi_gro_receive will hit final length incorrect issue.
		 */
#if 1
		netif_rx_ni(skb);
#else
		if (buf->ring_no)
			napi_gro_receive(&dma->rx_napi_1, skb);
		else
			napi_gro_receive(&dma->rx_napi_0, skb);
#endif
	}

	return 0;
}

static int fe_dma_deliver_raw(struct mtk_fe_dma *dma,
		u8 *data, u16 pktlen, u32 foe_info)
{
	data += NET_SKB_PAD;
	data += NET_IP_ALIGN;
	return dma->rxdone_handler(data, pktlen, foe_info,
			dma->rx_handler_data);
}

int fe_dma_deliver_rxdata(struct mtk_fe_dma *dma,
		u8 *data, u16 frag_size, u16 pktlen, u8 is_checksum_offload,
		u16 vlan_tci, u32 foe_info)
{
	if (dma->rxdone_handler)
		return fe_dma_deliver_raw(dma, data, pktlen, foe_info);
	else
		return fe_dma_deliver_skb(dma,
				data, frag_size, pktlen, is_checksum_offload,
				vlan_tci, foe_info);
}

int fe_dma_rxdrop_counting(struct mtk_fe_dma *dma, int drop_num)
{
	if (dma->rx_netdev)
		dma->rx_netdev->stats.rx_dropped += drop_num;
	//Kurt: else ??
	return 0;
}

int fe_dma_napi_enable(struct mtk_fe_dma **dma)
{
	napi_enable(&(*dma)->rx_napi_0);
	napi_enable(&(*dma)->rx_napi_1);
	napi_enable(&(*dma)->tx_napi);

	return 0;
}
EXPORT_SYMBOL(fe_dma_napi_enable);

int fe_dma_napi_disable(struct mtk_fe_dma *dma)
{
	dev_info(dma->fe->dev, "%s: dma user is %s\n", __func__, dma->user);
	napi_disable(&dma->rx_napi_0);
	napi_disable(&dma->rx_napi_1);
	napi_disable(&dma->tx_napi);

	return 0;
}
EXPORT_SYMBOL(fe_dma_napi_disable);

int fe_dma_probe(struct mtk_fe_token *fe, u32 channel, struct mtk_fe_dma **dma,
		u32 tx_ring_num, u32 rx_ring_num)
{
	int ret;

	if (!_dma_modules[channel].open)
		return -1;


	ret = _dma_modules[channel].open(channel, dma, tx_ring_num, rx_ring_num);
	if (ret || !*dma) {
		dev_info(fe->dev, "[%s] open failed at channel-%d, ret=%d\n",
				__func__, channel, ret);
		return ret;
	}
	_dma_modules[channel].fe_dma = *dma;

	(*dma)->user = &fe->user[0];
	(*dma)->fe = fe;
	(*dma)->dest_port = MTK_FE_PPE0;
	dev_info(fe->dev, "%s:, channel=%d, dma user is %s\n",
			__func__, channel, (*dma)->user);

	netif_napi_add(&dma_dummy_dev,
					&(*dma)->rx_napi_0,
					dma_napi_rx_0,
					MTK_FE_DMA_NAPI_WEIGHT);
	netif_napi_add(&dma_dummy_dev,
					&(*dma)->rx_napi_1,
					dma_napi_rx_1,
					MTK_FE_DMA_NAPI_WEIGHT);
	netif_napi_add(&dma_dummy_dev,
					&(*dma)->tx_napi,
					dma_napi_tx,
					MTK_FE_DMA_NAPI_WEIGHT);
	return 0;
}
EXPORT_SYMBOL(fe_dma_probe);

int fe_dma_pm_resume(struct mtk_fe_dma *dma)
{
	dev_info(dma->fe->dev, "%s: dma user is %s\n", __func__, dma->user);

	dma->do_pm_handle(dma->dma_channel, 0);

	return 0;
}
EXPORT_SYMBOL(fe_dma_pm_resume);


int fe_dma_cleanup(struct mtk_fe_dma *dma)
{
	dev_info(dma->fe->dev, "%s: dma user is %s\n", __func__, dma->user);
	netif_napi_del(&dma->rx_napi_0);
	netif_napi_del(&dma->rx_napi_1);
	netif_napi_del(&dma->tx_napi);

	_dma_modules[dma->dma_channel].fe_dma = NULL;
	dma->do_close(dma);

	return 0;
}
EXPORT_SYMBOL(fe_dma_cleanup);

int fe_dma_pm_suspend(struct mtk_fe_dma *dma)
{
	dev_info(dma->fe->dev, "%s: dma user is %s\n", __func__, dma->user);

	dma->do_pm_handle(dma->dma_channel, 1);

	return 0;
}
EXPORT_SYMBOL(fe_dma_pm_suspend);

int mtk_fe_dma_dbg(char *dma, char *argv[])
{
	if (!strcmp(dma, "edma"))
		return _dma_modules[MTK_FE_EDMA_0].debug(argv);
	return -1;
}

int mtk_fe_dma_probe(struct platform_device *pdev)
{
	int i, ret;
	ret = 0;

	dev_info(&pdev->dev, "[%s] in\n", __func__);

	for (i = 0; i < MTK_FE_DMA_NUM; i++) {
		if (_dma_modules[i].probe)
			ret = _dma_modules[i].probe(pdev,
				&_dma_modules[i].open, &_dma_modules[i].pm_handle,
				&_dma_modules[i].debug);

		if (ret)
			return ret;
	}

	init_dummy_netdev(&dma_dummy_dev);
	return 0;
}

int mtk_fe_dma_pm_handle(bool suspend)
{
	int i;

	pr_notice("[%s] suspend:%d\n", __func__, suspend ? 1 : 0);

	for (i = 0; i < MTK_FE_DMA_NUM; i++) {
		if (_dma_modules[i].open)
			_dma_modules[i].pm_handle(i, suspend);
	}

	return 0;
}
