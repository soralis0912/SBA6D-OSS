// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2020 Felix Fietkau <nbd@nbd.name>
 */

#include <linux/if_ether.h>
#include <linux/rhashtable.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/flow_offload.h>
#include <net/pkt_cls.h>
#include "mtk_eth_soc.h"
#include "mtk_wed.h"
#include "mtk_ppe.h"

/* Ethernet export API */
extern u32 mtk_eth_search_port_by_mac_table(const u8 *ptr_mac, u32 vid, int *port);

#define ETH_QOS_QCOUNT (64)
u8 eth_qos_enable = 1;
u8 fqos = 0;

struct mtk_flow_data {
	struct ethhdr eth;

	union {
		struct {
			__be32 src_addr;
			__be32 dst_addr;
		} v4;

		struct {
			struct in6_addr src_addr;
			struct in6_addr dst_addr;
		} v6;
	};

	__be16 src_port;
	__be16 dst_port;

	u16 vlan_in;

	struct {
		u16 id;
		__be16 proto;
		u8 num;
	} vlan;
	struct {
		u16 sid;
		u8 num;
	} pppoe;
};

static const struct rhashtable_params mtk_flow_ht_params = {
	.head_offset = offsetof(struct mtk_flow_entry, node),
	.key_offset = offsetof(struct mtk_flow_entry, cookie),
	.key_len = sizeof(unsigned long),
	.automatic_shrinking = true,
};

static int
mtk_flow_set_ipv4_addr(struct mtk_foe_entry *foe, struct mtk_flow_data *data,
		       bool egress)
{
	return mtk_foe_entry_set_ipv4_tuple(foe, egress,
					    data->v4.src_addr, data->src_port,
					    data->v4.dst_addr, data->dst_port);
}

static int
mtk_flow_set_ipv6_addr(struct mtk_foe_entry *foe, struct mtk_flow_data *data)
{
	return mtk_foe_entry_set_ipv6_tuple(foe,
					    data->v6.src_addr.s6_addr32, data->src_port,
					    data->v6.dst_addr.s6_addr32, data->dst_port);
}

static void
mtk_flow_offload_mangle_eth(const struct flow_action_entry *act, void *eth)
{
	void *dest = eth + act->mangle.offset;
	const void *src = &act->mangle.val;

	if (act->mangle.offset > 8)
		return;

	if (act->mangle.mask == 0xffff) {
		src += 2;
		dest += 2;
	}

	memcpy(dest, src, act->mangle.mask ? 2 : 4);
}

static int
mtk_flow_get_dma_info(struct net_device *dev, const u8 *addr, struct mtk_dma_info *info)
{
	struct net_device_path_ctx ctx = {
		.dev = dev,
	};
	struct net_device_path path = {};

	if (!dev || !dev->netdev_ops || !dev->netdev_ops->ndo_fill_forward_path) {
		if (debug_level >= 5)
			pr_notice("[%s] no ndo_fill_forward_path\n", __func__);
		return -1;
	}

	memcpy(ctx.daddr, addr, sizeof(ctx.daddr));
	if (dev->netdev_ops->ndo_fill_forward_path(&ctx, &path)) {
		if (debug_level >= 5)
			pr_notice("[%s] ERR! ndo_fill_forward_path returns fail\n", __func__);
		return -1;
	}

	info->path_type = path.type;

	switch (path.type) {
		case DEV_PATH_MTK_WDMA:
			if (!IS_ENABLED(CONFIG_NET_MEDIATEK_SOC_WED)) {
				pr_notice("[%s] ERR! CONFIG_NET_MEDIATEK_SOC_WED isn't enabled\n", __func__);
				return -1;
			}

			info->fe_wdma.wdma_idx = path.mtk_wdma.wdma_idx;
			info->fe_wdma.queue = path.mtk_wdma.queue;
			info->fe_wdma.bss = path.mtk_wdma.bss;
			info->fe_wdma.wcid = path.mtk_wdma.wcid;

			break;

		case DEV_PATH_MTK_MDMA:

			info->fe_mdma.ntype = path.mtk_mdma.ntype;
			info->fe_mdma.chid = path.mtk_mdma.chid;

			break;

		default:

			pr_notice("[%s] ERR! path.type:%d\n", __func__, path.type);
			return -1;

	}

	return 0;
}


static int
mtk_flow_mangle_ports(const struct flow_action_entry *act,
		      struct mtk_flow_data *data)
{
	u32 val = ntohl(act->mangle.val);

	switch (act->mangle.offset) {
	case 0:
		if (act->mangle.mask == ~htonl(0xffff))
			data->dst_port = cpu_to_be16(val);
		else
			data->src_port = cpu_to_be16(val >> 16);
		break;
	case 2:
		data->dst_port = cpu_to_be16(val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int
mtk_flow_mangle_ipv4(const struct flow_action_entry *act,
		     struct mtk_flow_data *data)
{
	__be32 *dest;

	switch (act->mangle.offset) {
	case offsetof(struct iphdr, saddr):
		dest = &data->v4.src_addr;
		break;
	case offsetof(struct iphdr, daddr):
		dest = &data->v4.dst_addr;
		break;
	default:
		return -EINVAL;
	}

	memcpy(dest, &act->mangle.val, sizeof(u32));

	return 0;
}


static int
mtk_flow_set_output_device(struct mtk_eth *eth, struct mtk_foe_entry *foe,
			   struct net_device *dev, const u8 *dest_mac,
			   int *wed_index)
{
	struct mtk_dma_info info = {};
	int pse_port, ret, switch_port, qidx;


	/* set qos & qid field */
	mtk_foe_entry_set_qid(foe, 0, fqos);

	if (mtk_flow_get_dma_info(dev, dest_mac, &info) == 0) {

		mtk_foe_entry_set_dma(foe, &info);

		switch (info.path_type) {

		case DEV_PATH_MTK_WDMA:

			if (info.fe_wdma.wdma_idx == 0)
				pse_port = WDMA0_PSE_PORT;
			else if (info.fe_wdma.wdma_idx == 1)
				pse_port = WDMA1_PSE_PORT;
			else {
				pr_notice("[%s] ERR! wdma_idx: %d is wrong\n", __func__, info.fe_wdma.wdma_idx);
				return -EOPNOTSUPP;
			}

			*wed_index = info.fe_wdma.wdma_idx;

			break;
		case DEV_PATH_MTK_MDMA:

			pse_port = MDMA_PSE_PORT;
			break;
		default:

			pr_notice("[%s] ERR! path_type:%d\n", __func__, info.path_type);

		} /* switch - end */

		goto out;
	}

	if (dev == eth->netdev[0])
		pse_port = GDMA1_PSE_PORT;
	else if (dev == eth->netdev[1])
		pse_port = GDMA2_PSE_PORT;
	else {
		pr_notice("[%s] ERR! dev is wrong\n", __func__);
		return -EOPNOTSUPP;
	}

	/* auto qos: pause frame enhancement */
	if (eth_qos_enable == 1) {
	#if defined(CONFIG_HNAT_V2)
		/* QDMA */
		if (FIELD_GET(MTK_FOE_IB1_BIND_SRC_PORT, foe->ib1) == 5)
			goto out;
	#endif /* CONFIG_HNAT_V2 */

		if (pse_port == GDMA2_PSE_PORT) {
			ret = mtk_eth_search_port_by_mac_table(dest_mac, 1, &switch_port);

			if (debug_level >= 1)
				pr_notice("[%s] dmac=%02x:%02x:%02x:%02x:%02x:%02x, ret:%d, switch_port:%d\n",
					__func__, dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5],
					ret, switch_port);

			if (ret == 0 && switch_port >= 0 && switch_port < 4) {
				qidx = ETH_QOS_QCOUNT - 1 - switch_port;
				mtk_foe_entry_set_qid(foe, qidx, 1);
			}
		} else {
			/* erouter0: fixed queue id 47 */
			mtk_foe_entry_set_qid(foe, ETH_QOS_QCOUNT * 3 / 4 - 1, 1);
		}

	}

out:
	mtk_foe_entry_set_pse_port(foe, pse_port);

	return 0;
}

static int
mtk_flow_offload_replace(struct mtk_eth *eth, struct flow_cls_offload *f)
{
	struct flow_rule *rule = flow_cls_offload_flow_rule(f);
	struct flow_action_entry *act;
	struct mtk_flow_data data = {};
	struct mtk_foe_entry foe;
	struct net_device *idev = NULL, *odev = NULL;
	struct mtk_flow_entry *entry;
	struct net_device_path_ctx ctx = {};
	struct net_device_path path = {};
	int offload_type = 0;
	int wed_index = -1;
	u16 addr_type = 0;
	u8 l4proto = 0;
	int err = 0;
	int i;

	if (rhashtable_lookup(&nf_ppe.flow_table, &f->cookie, mtk_flow_ht_params)) {
		if (debug_level >= 5)
			pr_notice("[%s]-, rhashtable_lookup EEXIST\n");
		return -EEXIST;
	}

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_META)) {
		struct flow_match_meta match;

		flow_rule_match_meta(rule, &match);
		idev = __dev_get_by_index(&init_net, match.key->ingress_ifindex);

		if (!idev)
			pr_err("[%s] idev doesn't exist !\n", __func__);

	} else {
		pr_err("[%s]-, FLOW_DISSECTOR_KEY_META EOPNOTSUPP\n", __func__);
		return -EOPNOTSUPP;
	}

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_CONTROL)) {
		struct flow_match_control match;

		flow_rule_match_control(rule, &match);
		addr_type = match.key->addr_type;

	} else {
		pr_err("[%s]-, FLOW_DISSECTOR_KEY_CONTROL EOPNOTSUPP\n", __func__);
		return -EOPNOTSUPP;
	}

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_BASIC)) {
		struct flow_match_basic match;

		flow_rule_match_basic(rule, &match);
		l4proto = match.key->ip_proto;
	} else {
		pr_err("[%s]-, FLOW_DISSECTOR_KEY_BASIC EOPNOTSUPP\n", __func__);
		return -EOPNOTSUPP;
	}

	switch (addr_type) {
	case 0:
		offload_type = MTK_PPE_PKT_TYPE_BRIDGE;
		if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ETH_ADDRS)) {
			struct flow_match_eth_addrs match;

			flow_rule_match_eth_addrs(rule, &match);
			memcpy(data.eth.h_dest, match.key->dst, ETH_ALEN);
			memcpy(data.eth.h_source, match.key->src, ETH_ALEN);

		} else {
			pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
			return -EOPNOTSUPP;
		}

		if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_VLAN)) {
			struct flow_match_vlan match;

			flow_rule_match_vlan(rule, &match);

			if (match.key->vlan_tpid != cpu_to_be16(ETH_P_8021Q)) {
				pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
				return -EOPNOTSUPP;
			}

			data.vlan_in = match.key->vlan_id;
		}
		break;
	case FLOW_DISSECTOR_KEY_IPV4_ADDRS:
		offload_type = MTK_PPE_PKT_TYPE_IPV4_HNAPT;
		break;
	case FLOW_DISSECTOR_KEY_IPV6_ADDRS:
		offload_type = MTK_PPE_PKT_TYPE_IPV6_ROUTE_5T;
		break;
	default:
		pr_notice("[%s][%d]-EOPNOTSUPP addr_type:%d\n", __func__, __LINE__, addr_type);
		return -EOPNOTSUPP;
	}

	flow_action_for_each(i, act, &rule->action) {
		switch (act->id) {
		case FLOW_ACTION_MANGLE:
			if (offload_type == MTK_PPE_PKT_TYPE_BRIDGE) {
				pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
				return -EOPNOTSUPP;
			}
			if (act->mangle.htype == FLOW_ACT_MANGLE_HDR_TYPE_ETH)
				mtk_flow_offload_mangle_eth(act, &data.eth);
			break;
		case FLOW_ACTION_REDIRECT:
			odev = act->dev;

			break;
		case FLOW_ACTION_CSUM:
			break;
		case FLOW_ACTION_VLAN_PUSH:
			if (data.vlan.num == 1 ||
			    act->vlan.proto != htons(ETH_P_8021Q)) {
				pr_err("[%s]-, FLOW_ACTION_VLAN_PUSH EOPNOTSUPP\n", __func__);
				return -EOPNOTSUPP;
			}
			data.vlan.id = act->vlan.vid;
			data.vlan.proto = act->vlan.proto;
			data.vlan.num++;
			break;
		case FLOW_ACTION_VLAN_POP:
			break;
		case FLOW_ACTION_PPPOE_PUSH:
			if (data.pppoe.num == 1) {
				pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
				return -EOPNOTSUPP;
			}

			data.pppoe.sid = act->pppoe.sid;
			data.pppoe.num++;
			break;
		default:
			pr_notice("[%s][%d]-EOPNOTSUPP act->id:%d\n", __func__, __LINE__, act->id);
			return -EOPNOTSUPP;
		}
	}

	if (debug_level >= 5)
		pr_notice("[%s] %s->%s, %pM -> %pM, type:%d, l4pro:%d, cookie:%u\n", __func__,
			idev->name, odev->name, data.eth.h_source, data.eth.h_dest, offload_type, l4proto, f->cookie);

	if (!is_valid_ether_addr(data.eth.h_source) ||
	    !is_valid_ether_addr(data.eth.h_dest)) {

		/* Modem TX packet is IP packet and no need to fill mac header */
		if (strncmp(odev->name, "ccmni", 5) != 0) {

	    		printk(KERN_ERR "[%s] -, EINVAL! h_source:%pM, h_dest:%pM\n", __func__, data.eth.h_source, data.eth.h_dest);

			err = -EINVAL;
			goto free;
		}
	}

	err = mtk_foe_entry_prepare(&foe, offload_type, l4proto, 0,
				    data.eth.h_source,
				    data.eth.h_dest);
	if (err) {
		pr_notice("[%s]-, prepare err\n", __func__, err);
		return err;
	}

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_PORTS)) {
		struct flow_match_ports ports;

		if (offload_type == MTK_PPE_PKT_TYPE_BRIDGE) {
			pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
			return -EOPNOTSUPP;
		}

		flow_rule_match_ports(rule, &ports);
		data.src_port = ports.key->src;
		data.dst_port = ports.key->dst;
	} else if (offload_type != MTK_PPE_PKT_TYPE_BRIDGE) {
		pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
		return -EOPNOTSUPP;
	}

	if (addr_type == FLOW_DISSECTOR_KEY_IPV4_ADDRS) {
		struct flow_match_ipv4_addrs addrs;

		flow_rule_match_ipv4_addrs(rule, &addrs);

		data.v4.src_addr = addrs.key->src;
		data.v4.dst_addr = addrs.key->dst;

		mtk_flow_set_ipv4_addr(&foe, &data, false);
	}

	if (addr_type == FLOW_DISSECTOR_KEY_IPV6_ADDRS) {
		struct flow_match_ipv6_addrs addrs;

		flow_rule_match_ipv6_addrs(rule, &addrs);

		data.v6.src_addr = addrs.key->src;
		data.v6.dst_addr = addrs.key->dst;

		mtk_flow_set_ipv6_addr(&foe, &data);
	}

	flow_action_for_each(i, act, &rule->action) {
		if (act->id != FLOW_ACTION_MANGLE)
			continue;

		if (offload_type == MTK_PPE_PKT_TYPE_BRIDGE) {
			pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
			return -EOPNOTSUPP;
		}

		switch (act->mangle.htype) {
		case FLOW_ACT_MANGLE_HDR_TYPE_TCP:
		case FLOW_ACT_MANGLE_HDR_TYPE_UDP:
			err = mtk_flow_mangle_ports(act, &data);
			break;
		case FLOW_ACT_MANGLE_HDR_TYPE_IP4:
			err = mtk_flow_mangle_ipv4(act, &data);
			break;
		case FLOW_ACT_MANGLE_HDR_TYPE_ETH:
			/* handled earlier */
			break;
		default:
			pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
			return -EOPNOTSUPP;
		}

		if (err)
			return err;
	}

	if (addr_type == FLOW_DISSECTOR_KEY_IPV4_ADDRS) {
		err = mtk_flow_set_ipv4_addr(&foe, &data, true);
		if (err) {
			pr_notice("[%s] set ipv4 addr, err:%d\n", __func__, err);
			return err;
		}
	}

	if (offload_type == MTK_PPE_PKT_TYPE_BRIDGE)
		foe.bridge.vlan = data.vlan_in;

	if (data.vlan.num == 1) {
		if (data.vlan.proto != htons(ETH_P_8021Q)) {
			pr_notice("[%s][%d]-EOPNOTSUPP\n", __func__, __LINE__);
			return -EOPNOTSUPP;
		}

		mtk_foe_entry_set_vlan(&foe, data.vlan.id);
	}

	if (data.pppoe.num == 1)
		mtk_foe_entry_set_pppoe(&foe, data.pppoe.sid);

	err = mtk_flow_set_output_device(eth, &foe, odev, data.eth.h_dest,
					 &wed_index);
	if (err) {
		pr_err("[%s]- set_output_device err=%d\n", __func__, err);
		return err;
	}

	if (wed_index >= 0 && (err = mtk_wed_flow_add(wed_index)) < 0) {
		pr_err("[%s]- wed add fail\n", __func__);
		return err;
	}

	i = 0;
	if (idev && idev->netdev_ops && idev->netdev_ops->ndo_fill_receive_path) {
		ctx.dev = idev;
		idev->netdev_ops->ndo_fill_receive_path(&ctx, &path);
		switch (path.type) {
			case DEV_PATH_MTK_MDMA:
				i = path.mtk_mdma.ppe_idx;

				if (debug_level >= 5)
					pr_notice("[%s] fill_receive_path (mdma), ppe%d\n", __func__, i);
				break;
			case DEV_PATH_MTK_WDMA:
				i = path.mtk_wdma.wdma_idx;

				if (debug_level >= 5)
					pr_notice("[%s] fill_receive_path (wdma), ppe%d\n", __func__, i);
				break;
			default:

				if (debug_level >= 5)
					pr_notice("[%s] fill_receive_path (wrong type:%d), ppe0\n", __func__, path.type);
				break;
		}

		if (i >= nf_ppe.ppe_num) {
			pr_notice("[%s] PPE%d doesn't exist\n", __func__, i);

			return -EINVAL;
		}
	} else {
		if (debug_level >= 5)
			pr_notice("[%s] fill_receive_path is empty\n", __func__);
	}

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		pr_notice("[%s]-ENOMEM\n", __func__);
		return -ENOMEM;
	}

	entry->cookie = f->cookie;
	memcpy(&entry->data, &foe, sizeof(entry->data));
	entry->ppe_index = i;
	entry->wed_index = wed_index;

	if (mtk_foe_entry_commit(nf_ppe.ppe[i], entry) < 0) {
		pr_notice("[%s] commit err\n", __func__);
		goto free;
	}

	err = rhashtable_insert_fast(&nf_ppe.flow_table, &entry->node,
				     mtk_flow_ht_params);
	if (err < 0) {
		pr_notice("[%s] insert err: %d\n", __func__, err);
		goto clear;
	}

	return 0;

clear:
	mtk_foe_entry_clear(nf_ppe.ppe[i], entry);
free:
	kfree(entry);
	if (wed_index >= 0)
	    mtk_wed_flow_remove(wed_index);
	return err;
}

static int
mtk_flow_offload_destroy(struct mtk_eth *eth, struct flow_cls_offload *f)
{
	struct mtk_flow_entry *entry;
	int i;

	entry = rhashtable_lookup(&nf_ppe.flow_table, &f->cookie,
				  mtk_flow_ht_params);
	if (!entry)
		return -ENOENT;

	if (debug_level >= 5)
		pr_notice("[%s] hash:%d, cookie:%u\n", __func__, entry->hash, f->cookie);

	i = entry->ppe_index;
	mtk_foe_entry_clear(nf_ppe.ppe[i], entry);
	rhashtable_remove_fast(&nf_ppe.flow_table, &entry->node,
			       mtk_flow_ht_params);
	if (entry->wed_index >= 0)
		mtk_wed_flow_remove(entry->wed_index);
	kfree(entry);

	return 0;
}

static int
mtk_flow_offload_stats(struct mtk_eth *eth, struct flow_cls_offload *f)
{
	struct mtk_flow_entry *entry;
	struct mtk_foe_accounting diff ={0};
	u32 idle;
	int i;

	entry = rhashtable_lookup(&nf_ppe.flow_table, &f->cookie,
				  mtk_flow_ht_params);
	if (!entry)
		return -ENOENT;

	i = entry->ppe_index;
	idle = mtk_foe_entry_idle_time(nf_ppe.ppe[i], entry);
	f->stats.lastused = jiffies - idle * HZ;

	if (entry->hash != 0xFFFF) {
		mtk_foe_entry_get_mib(nf_ppe.ppe[i], entry->hash, &diff);
		f->stats.pkts += diff.packets;
		f->stats.bytes += diff.bytes;
	}

	if (debug_level >= 5)
		pr_notice("[%s] cookie:%u, hash:%d, pkt:%llu (+%llu), byte:%llu (+%llu)\n", __func__,
				f->cookie, entry->hash, f->stats.pkts, diff.packets, f->stats.bytes, diff.bytes);

	return 0;
}

static DEFINE_MUTEX(mtk_flow_offload_mutex);

static int
mtk_netdev_setup_tc_block_cb(enum tc_setup_type type, void *type_data, void *cb_priv)
{
	struct flow_cls_offload *cls = type_data;
	struct net_device *dev = cb_priv;
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	int err;

	if (debug_level >= 5)
		pr_notice("[%s] type=%d, cmd=%d, dev:%s, cookie:%u\n", __func__,
				type, cls->command, dev->name, cls->cookie);

	if (!tc_can_offload(dev)) {
		pr_notice("[%s]- dev(%s) can't hw offload, dev->features:0x%llu\n", __func__, dev->name, dev->features);
		return -EOPNOTSUPP;
	}

	if (type != TC_SETUP_CLSFLOWER)
		return -EOPNOTSUPP;

	mutex_lock(&mtk_flow_offload_mutex);
	switch (cls->command) {
	case FLOW_CLS_REPLACE:
		err = mtk_flow_offload_replace(eth, cls);
		break;
	case FLOW_CLS_DESTROY:
		err = mtk_flow_offload_destroy(eth, cls);
		break;
	case FLOW_CLS_STATS:
		err = mtk_flow_offload_stats(eth, cls);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}
	mutex_unlock(&mtk_flow_offload_mutex);

	return err;
}

static int
mtk_netdev_setup_tc_block(struct net_device *dev, struct flow_block_offload *f)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	static LIST_HEAD(block_cb_list);
	struct flow_block_cb *block_cb;
	flow_setup_cb_t *cb;
	int i;

	for (i = 0; i < nf_ppe.ppe_num; i++) {
		if (!nf_ppe.ppe[i] || !nf_ppe.ppe[i]->foe_table)
			return -EOPNOTSUPP;
	}

	if (debug_level >= 5)
		pr_notice("[%s] dev=%s cmd=%d, binder_type=%d\n", __func__,
				dev->name, f->command, f->binder_type);

	if (f->binder_type != FLOW_BLOCK_BINDER_TYPE_CLSACT_INGRESS)
		return -EOPNOTSUPP;

	cb = mtk_netdev_setup_tc_block_cb;
	f->driver_block_list = &block_cb_list;

	switch (f->command) {
	case FLOW_BLOCK_BIND:
		block_cb = flow_block_cb_lookup(f->block, cb, dev);
		if (block_cb) {
			if (debug_level >= 5)
				pr_notice("lookup block_cb dev found\n",
						((struct net_device *)(block_cb->cb_ident))->name);
			flow_block_cb_incref(block_cb);
			return 0;
		}
		block_cb = flow_block_cb_alloc(cb, dev, dev, NULL);
		if (IS_ERR(block_cb))
			return PTR_ERR(block_cb);

			if (debug_level >= 5)
				pr_notice("alloc block_cb->cb_ident->name %s\n",
						((struct net_device *)(block_cb->cb_ident))->name);
		flow_block_cb_add(block_cb, f);
		list_add_tail(&block_cb->driver_list, &block_cb_list);
		return 0;
	case FLOW_BLOCK_UNBIND:
		block_cb = flow_block_cb_lookup(f->block, cb, dev);
		if (!block_cb)
			return -ENOENT;

		if (flow_block_cb_decref(block_cb)) {
			flow_block_cb_remove(block_cb, f);
			list_del(&block_cb->driver_list);
		}
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

int mtk_netdev_setup_tc(struct net_device *dev, enum tc_setup_type type,
		     void *type_data)
{
	switch (type) {
	case TC_SETUP_BLOCK:
	case TC_SETUP_FT:
		return mtk_netdev_setup_tc_block(dev, type_data);
	default:
		return -EOPNOTSUPP;
	}
}

int mtk_eth_offload_init(struct mtk_eth *eth, int id)
{
	if (!nf_ppe.ppe[id] || !nf_ppe.ppe[id]->foe_table)
		return 0;

	pr_notice("[%s]\n", __func__);

	return rhashtable_init(&nf_ppe.flow_table, &mtk_flow_ht_params);
}
