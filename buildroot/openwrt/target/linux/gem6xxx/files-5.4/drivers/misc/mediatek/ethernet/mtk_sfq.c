// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek SFQ
 *
 * Author: Stella Yang <stella.yang@mediatek.com>
 *
 */


#include <linux/of_device.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/if_vlan.h>
#include <linux/reset.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <net/ipv6.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/in.h>
#include <linux/ppp_defs.h>
#include <linux/if_pppox.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/mdio.h>
#include <linux/regulator/consumer.h>

#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"
#include "mtk_sfq.h"

static int sfq_status = SFQ_STATUS_DEINIT;

static bool sfq_init(struct mtk_eth *eth)
{
	unsigned int reg_val;
	dma_addr_t sfq_phy0;
	dma_addr_t sfq_phy1;
	dma_addr_t sfq_phy2;
	dma_addr_t sfq_phy3;
	struct SFQ_table *sfq0 = NULL;
	struct SFQ_table *sfq1 = NULL;
	struct SFQ_table *sfq2 = NULL;
	struct SFQ_table *sfq3 = NULL;

	dma_addr_t sfq_phy4;
	dma_addr_t sfq_phy5;
	dma_addr_t sfq_phy6;
	dma_addr_t sfq_phy7;
	struct SFQ_table *sfq4 = NULL;
	struct SFQ_table *sfq5 = NULL;
	struct SFQ_table *sfq6 = NULL;
	struct SFQ_table *sfq7 = NULL;

	int i = 0;

	reg_val = mtk_r32(eth, VQTX_GLO);
	reg_val = reg_val | VQTX_MIB_EN;
	/* Virtual table extends to 32bytes */
	mtk_w32(eth, reg_val, VQTX_GLO);
	reg_val = mtk_r32(eth, VQTX_GLO);

	mtk_w32(eth, (VQTX_NUM_0) | (VQTX_NUM_1) | (VQTX_NUM_2) |
		     (VQTX_NUM_3) | (VQTX_NUM_4) | (VQTX_NUM_5) |
		     (VQTX_NUM_6) | (VQTX_NUM_7), VQTX_NUM);

	/* 10 s change hash algorithm */
	mtk_w32(eth, 0xF002710, VQTX_HASH_CFG);

	/* one group has two slices (64 queues/slice, totally 128 queues) */
	mtk_w32(eth, 0xeca86420, VQTX_VLD_CFG);

	mtk_w32(eth, 0x0D, VQTX_HASH_SD);

	mtk_w32(eth, 0, QDMA_HRED1);
	mtk_w32(eth, 0, QDMA_HRED2);
	mtk_w32(eth, 0, QDMA_SRED1);
	mtk_w32(eth, 0, QDMA_SRED2);

	/* assign 8 physical queues */
	mtk_w32(eth, (VQTX_0_BIND_QID) | (VQTX_1_BIND_QID) |
		     (VQTX_2_BIND_QID) | (VQTX_3_BIND_QID), VQTX_0_3_BIND_QID);
	mtk_w32(eth, (VQTX_4_BIND_QID) | (VQTX_5_BIND_QID) |
		     (VQTX_6_BIND_QID) | (VQTX_7_BIND_QID), VQTX_4_7_BIND_QID);

	pr_info("%s, VQTX_0_3_BIND_QID =%x\n", __func__, mtk_r32(eth, VQTX_0_3_BIND_QID));
	pr_info("%s, VQTX_4_7_BIND_QID =%x\n", __func__, mtk_r32(eth, VQTX_4_7_BIND_QID));


	sfq0 = dma_alloc_coherent(eth->dev,
				  VQ_NUM0 * sizeof(struct SFQ_table), &sfq_phy0,
				  GFP_KERNEL);

	memset(sfq0, 0x0, VQ_NUM0 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM0; i++) {
		sfq0[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq0[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	sfq1 = dma_alloc_coherent(eth->dev,
				  VQ_NUM1 * sizeof(struct SFQ_table), &sfq_phy1,
				  GFP_KERNEL);
	memset(sfq1, 0x0, VQ_NUM1 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM1; i++) {
		sfq1[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq1[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}

	sfq2 = dma_alloc_coherent(eth->dev,
				  VQ_NUM2 * sizeof(struct SFQ_table), &sfq_phy2,
				  GFP_KERNEL);
	memset(sfq2, 0x0, VQ_NUM2 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM2; i++) {
		sfq2[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq2[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}

	sfq3 = dma_alloc_coherent(eth->dev,
				  VQ_NUM3 * sizeof(struct SFQ_table), &sfq_phy3,
				  GFP_KERNEL);
	memset(sfq3, 0x0, VQ_NUM3 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM3; i++) {
		sfq3[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq3[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	if (unlikely((!sfq0)) || unlikely((!sfq1)) ||
	    unlikely((!sfq2)) || unlikely((!sfq3))) {
		pr_err("%s, QDMA SFQ0~3 VQ not available...\n", __func__);
		return 1;
	}

	sfq4 =
	    dma_alloc_coherent(eth->dev,
			       VQ_NUM4 * sizeof(struct SFQ_table),
			       &sfq_phy4, GFP_KERNEL);
	memset(sfq4, 0x0, VQ_NUM4 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM4; i++) {
		sfq4[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq4[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	sfq5 =
	    dma_alloc_coherent(eth->dev,
			       VQ_NUM5 * sizeof(struct SFQ_table),
			       &sfq_phy5, GFP_KERNEL);
	memset(sfq5, 0x0, VQ_NUM5 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM5; i++) {
		sfq5[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq5[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	sfq6 =
	    dma_alloc_coherent(eth->dev,
			       VQ_NUM6 * sizeof(struct SFQ_table),
			       &sfq_phy6, GFP_KERNEL);
	memset(sfq6, 0x0, VQ_NUM6 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM6; i++) {
		sfq6[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq6[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	sfq7 =
	    dma_alloc_coherent(eth->dev,
			       VQ_NUM7 * sizeof(struct SFQ_table),
			       &sfq_phy7, GFP_KERNEL);
	memset(sfq7, 0x0, VQ_NUM7 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM7; i++) {
		sfq7[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq7[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	if (unlikely((!sfq4)) || unlikely((!sfq5)) ||
	    unlikely((!sfq6)) || unlikely((!sfq7))) {
		pr_err("%s, QDMA SFQ4~7 VQ not available...\n", __func__);
		return 1;
	}


	pr_info("*****sfq_phy0 is 0x%p!!!*******\n", (void *)sfq_phy0);
	pr_info("*****sfq_phy1 is 0x%p!!!*******\n", (void *)sfq_phy1);
	pr_info("*****sfq_phy2 is 0x%p!!!*******\n", (void *)sfq_phy2);
	pr_info("*****sfq_phy3 is 0x%p!!!*******\n", (void *)sfq_phy3);
	pr_info("*****sfq_phy4 is 0x%p!!!*******\n", (void *)sfq_phy4);
	pr_info("*****sfq_phy5 is 0x%p!!!*******\n", (void *)sfq_phy5);
	pr_info("*****sfq_phy6 is 0x%p!!!*******\n", (void *)sfq_phy6);
	pr_info("*****sfq_phy7 is 0x%p!!!*******\n", (void *)sfq_phy7);

	pr_info("*****sfq_virt0 is 0x%p!!!*******\n", sfq0);
	pr_info("*****sfq_virt1 is 0x%p!!!*******\n", sfq1);
	pr_info("*****sfq_virt2 is 0x%p!!!*******\n", sfq2);
	pr_info("*****sfq_virt3 is 0x%p!!!*******\n", sfq3);
	pr_info("*****sfq_virt4 is 0x%p!!!*******\n", sfq4);
	pr_info("*****sfq_virt5 is 0x%p!!!*******\n", sfq5);
	pr_info("*****sfq_virt6 is 0x%p!!!*******\n", sfq6);
	pr_info("*****sfq_virt7 is 0x%p!!!*******\n", sfq7);

	mtk_w32(eth, (u32)sfq_phy0, VQTX_TB_BASE0);
	mtk_w32(eth, (u32)sfq_phy1, VQTX_TB_BASE1);
	mtk_w32(eth, (u32)sfq_phy2, VQTX_TB_BASE2);
	mtk_w32(eth, (u32)sfq_phy3, VQTX_TB_BASE3);
	mtk_w32(eth, (u32)sfq_phy4, VQTX_TB_BASE4);
	mtk_w32(eth, (u32)sfq_phy5, VQTX_TB_BASE5);
	mtk_w32(eth, (u32)sfq_phy6, VQTX_TB_BASE6);
	mtk_w32(eth, (u32)sfq_phy7, VQTX_TB_BASE7);

	return 0;
}


static int sfq_parse_layer_info(struct sk_buff *skb, struct sfq_parse_result *sfq_parse)
{
	struct vlan_hdr *vh_sfq = NULL;
	struct ethhdr *eth_sfq = NULL;
	struct iphdr *iph_sfq = NULL;
	struct ipv6hdr *ip6h_sfq = NULL;
	struct tcphdr *th_sfq = NULL;
	struct udphdr *uh_sfq = NULL;

	memset(sfq_parse, 0, sizeof(struct sfq_parse_result));
	eth_sfq = (struct ethhdr *)skb->data;
	ether_addr_copy(sfq_parse->dmac, eth_sfq->h_dest);
	ether_addr_copy(sfq_parse->smac, eth_sfq->h_source);
	/* memcpy(sfq_parse_result.dmac, eth_sfq->h_dest, ETH_ALEN); */
	/* memcpy(sfq_parse_result.smac, eth_sfq->h_source, ETH_ALEN); */
	sfq_parse->eth_type = eth_sfq->h_proto;

	if (sfq_parse->eth_type == htons(ETH_P_8021Q)) {
		sfq_parse->vlan1_gap = VLAN_HLEN;
		vh_sfq = (struct vlan_hdr *)(skb->data + ETH_HLEN);
		sfq_parse->eth_type = vh_sfq->h_vlan_encapsulated_proto;
	} else {
		sfq_parse->vlan1_gap = 0;
	}

	/* set layer4 start addr */
	if ((sfq_parse->eth_type == htons(ETH_P_IP)) ||
	    (sfq_parse->eth_type == htons(ETH_P_PPP_SES) &&
	     sfq_parse->ppp_tag == htons(PPP_IP))) {
		iph_sfq =
		    (struct iphdr *)(skb->data + ETH_HLEN +
				     (sfq_parse->vlan1_gap));

		/* prepare layer3/layer4 info */
		memcpy(&sfq_parse->iph, iph_sfq, sizeof(struct iphdr));
		if (iph_sfq->protocol == IPPROTO_TCP) {
			th_sfq =
			    (struct tcphdr *)(skb->data + ETH_HLEN +
					      (sfq_parse->vlan1_gap) +
					      (iph_sfq->ihl * 4));
			memcpy(&sfq_parse->th, th_sfq,
			       sizeof(struct tcphdr));

			sfq_parse->tcp_source_port = ntohs(sfq_parse->th.source);
			sfq_parse->udp_source_port = 0;
			/* tcp ack packet */
			if (ntohl(sfq_parse->iph.saddr) == 0xa0a0a04)
				sfq_parse->ack_packt = 1;
			else
				sfq_parse->ack_packt = 0;

			sfq_parse->sfq_prot = 2;	/* IPV4_HNAPT */
			if (iph_sfq->frag_off & htons(IP_MF | IP_OFFSET))
				return 1;
		} else if (iph_sfq->protocol == IPPROTO_UDP) {
			uh_sfq =
			    (struct udphdr *)(skb->data + ETH_HLEN +
					      (sfq_parse->vlan1_gap) +
					      iph_sfq->ihl * 4);
			memcpy(&sfq_parse->uh, uh_sfq,
			       sizeof(struct udphdr));

			sfq_parse->udp_source_port = ntohs(sfq_parse->uh.source);
			sfq_parse->tcp_source_port = 0;
			sfq_parse->ack_packt = 0;

			sfq_parse->sfq_prot = 2;	/* IPV4_HNAPT */
			if (iph_sfq->frag_off & htons(IP_MF | IP_OFFSET))
				return 1;
		} else {
			sfq_parse->sfq_prot = 1;
		}
	} else if (sfq_parse->eth_type == htons(ETH_P_IPV6) ||
		   (sfq_parse->eth_type == htons(ETH_P_PPP_SES) &&
		    sfq_parse->ppp_tag == htons(PPP_IPV6))) {
		ip6h_sfq =
		    (struct ipv6hdr *)(skb->data + ETH_HLEN +
				       (sfq_parse->vlan1_gap));
		if (ip6h_sfq->nexthdr == NEXTHDR_TCP) {
			sfq_parse->sfq_prot = 4;	/* IPV6_5T */

			if (ntohl(sfq_parse->ip6h.saddr.s6_addr32[3]) ==
			    8)
				sfq_parse->ack_packt = 1;
			else
				sfq_parse->ack_packt = 0;

		} else if (ip6h_sfq->nexthdr == NEXTHDR_UDP) {

			sfq_parse->ack_packt = 0;

			sfq_parse->sfq_prot = 4;	/* IPV6_5T */

		} else {
			sfq_parse->sfq_prot = 3;	/* IPV6_3T */
		}
	}
	return 0;
}

/* Exported APIs */
void sfq_fill_txd5(struct sk_buff *skb, u32 *txd5)
{

	struct sfq_parse_result sfq_parse;

	if (sfq_status != SFQ_STATUS_ENABLE)
		return;

	sfq_parse_layer_info(skb, &sfq_parse);

	pr_info("%s, sfq_prot=%d vlan1_gap=%d\n", __func__,
		sfq_parse.sfq_prot, sfq_parse.vlan1_gap);

	*txd5 |= TX_DMA_VQID0; /* 1:HW hash 0:CPU */
	*txd5 |= TX_DMA_PROT(sfq_parse.sfq_prot);
	*txd5 |= TX_DMA_IPOFST(14 + sfq_parse.vlan1_gap); /* no vlan */

}

void sfq_enable(struct mtk_eth *eth)
{
	if (sfq_status == SFQ_STATUS_DEINIT)
		sfq_init(eth);
	sfq_status = SFQ_STATUS_ENABLE;
	pr_info("%s, sfq_status=%d\n", __func__, sfq_status);
}

void sfq_disable(struct mtk_eth *eth)
{
	if (sfq_status == SFQ_STATUS_ENABLE)
		sfq_status = SFQ_STATUS_DISABLE;

	pr_info("%s, sfq_status=%d\n", __func__, sfq_status);
}

void sfq_print_status(struct mtk_eth *eth)
{
	pr_info("%s, sfq_status=%d\n", __func__, sfq_status);
}



