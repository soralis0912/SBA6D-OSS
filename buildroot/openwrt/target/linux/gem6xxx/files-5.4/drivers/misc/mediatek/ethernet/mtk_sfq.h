// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek HWLRO
 *
 * Author: Stella Yang <stella.yang@mediatek.com>
 *
 */

#ifndef MTK_SFQ_H
#define MTK_SFQ_H

#include <linux/ip.h>
#include <linux/ipv6.h>

/*=========================================
 *    SFQ define
 *=========================================
 */
#define PQ0	0
#define PQ1	1
#define PQ2	15
#define PQ3	16
#define PQ4	30
#define PQ5	31
#define PQ6	43
#define PQ7	63

#define VQ_NUM0	128
#define VQ_NUM1	128
#define VQ_NUM2	128
#define VQ_NUM3	128
#define VQ_NUM4	128
#define VQ_NUM5	128
#define VQ_NUM6	128
#define VQ_NUM7	128

#define  QDMA_HRED1	(QDMA_BASE + 0x240)
#define  QDMA_HRED2	(QDMA_BASE + 0x244)
#define  QDMA_SRED1	(QDMA_BASE + 0x248)
#define  QDMA_SRED2	(QDMA_BASE + 0x24C)

#define VQTX_TB_BASE0 (QDMA_BASE + 0x180)
#define VQTX_TB_BASE1 (QDMA_BASE + 0x184)
#define VQTX_TB_BASE2 (QDMA_BASE + 0x188)
#define VQTX_TB_BASE3 (QDMA_BASE + 0x18C)
#define VQTX_TB_BASE4 (QDMA_BASE + 0x190)
#define VQTX_TB_BASE5 (QDMA_BASE + 0x194)
#define VQTX_TB_BASE6 (QDMA_BASE + 0x198)
#define VQTX_TB_BASE7 (QDMA_BASE + 0x19C)

#define VQTX_GLO 	(QDMA_BASE + 0x280)
#define VQTX_INVLD_PTR 	(QDMA_BASE + 0x28C)
#define VQTX_NUM 	(QDMA_BASE + 0x290)
#define VQTX_SCH 	(QDMA_BASE + 0x298)
#define VQTX_HASH_CFG 	(QDMA_BASE + 0x2A0)
#define VQTX_HASH_SD 	(QDMA_BASE + 0x2A4)
#define VQTX_VLD_CFG 	(QDMA_BASE + 0x2B0)
#define VQTX_MIB_IF 	(QDMA_BASE + 0x2BC)
#define VQTX_MIB_PCNT 	(QDMA_BASE + 0x2C0)
#define VQTX_MIB_BCNT0 	(QDMA_BASE + 0x2C4)
#define VQTX_MIB_BCNT1 	(QDMA_BASE + 0x2C8)
#define VQTX_0_BIND_QID	(PQ0 << 0)
#define VQTX_1_BIND_QID (PQ1 << 8)
#define VQTX_2_BIND_QID (PQ2 << 16)
#define VQTX_3_BIND_QID (PQ3 << 24)
#define VQTX_4_BIND_QID (PQ4 << 0)
#define VQTX_5_BIND_QID (PQ5 << 8)
#define VQTX_6_BIND_QID (PQ6 << 16)
#define VQTX_7_BIND_QID (PQ7 << 24)

#define VQTX_0_3_BIND_QID (QDMA_BASE + 0x3C0)
#define VQTX_4_7_BIND_QID (QDMA_BASE + 0x3C4)

#define VQTX_NUM_0  (3 << 0)
#define VQTX_NUM_1  (3 << 4)
#define VQTX_NUM_2  (3 << 8)
#define VQTX_NUM_3  (3 << 12)
#define VQTX_NUM_4  (3 << 16)
#define VQTX_NUM_5  (3 << 20)
#define VQTX_NUM_6  (3 << 24)
#define VQTX_NUM_7  (3 << 28)


#define VQTX_MIB_EN BIT(17)

/*=========================================
 *    QDMA DMAD descriptor txd5 - SFQ
 *=========================================
*/
#define TX_DMA_VQID0		BIT(26)
#define TX_DMA_PROT(_x)		(((_x) & 0x3) << 0)
#define TX_DMA_IPOFST(_x)	(((_x) & 0x7f) << 5)

/*=========================================
 *    SFQ Status define
 *=========================================
 */
#define SFQ_STATUS_DEINIT (-1)
#define SFQ_STATUS_DISABLE (0)
#define SFQ_STATUS_ENABLE (1)

/*=========================================
 *    SFQ Table Format define
 *=========================================
 */
struct SFQ_INFO1_T {
	unsigned int VQHPTR;
};

struct SFQ_INFO2_T {
	unsigned int VQTPTR;
};

struct SFQ_INFO3_T {
	unsigned int QUE_DEPTH:16;
	unsigned int DEFICIT_CNT:16;
};

struct SFQ_INFO4_T {
	unsigned int RESV;
};

struct SFQ_INFO5_T {
	unsigned int PKT_CNT;
};

struct SFQ_INFO6_T {
	unsigned int BYTE_CNT;
};

struct SFQ_INFO7_T {
	unsigned int BYTE_CNT;
};

struct SFQ_INFO8_T {
	unsigned int RESV;
};

struct SFQ_table {
	struct SFQ_INFO1_T sfq_info1;
	struct SFQ_INFO2_T sfq_info2;
	struct SFQ_INFO3_T sfq_info3;
	struct SFQ_INFO4_T sfq_info4;
	struct SFQ_INFO5_T sfq_info5;
	struct SFQ_INFO6_T sfq_info6;
	struct SFQ_INFO7_T sfq_info7;
	struct SFQ_INFO8_T sfq_info8;
};

struct sfq_parse_result {
	/* layer2 header */
	u8 dmac[6];
	u8 smac[6];

	/* vlan header */
	u16 vlan_tag;
	u16 vlan1_gap;
	u16 vlan1;
	u16 vlan2_gap;
	u16 vlan2;
	u16 vlan_layer;

	/* pppoe header */
	u32 pppoe_gap;
	u16 ppp_tag;
	u16 pppoe_sid;

	/* layer3 header */
	u16 eth_type;
	struct iphdr iph;
	struct ipv6hdr ip6h;

	/* layer4 header */
	struct tcphdr th;
	struct udphdr uh;

	u32 pkt_type;
	u8 is_mcast;

	int sfq_prot;
	/* debug */
	int udp_source_port;
	int tcp_source_port;
	int ack_packt;
};

void sfq_enable(struct mtk_eth *eth);
void sfq_disable(struct mtk_eth *eth);
void sfq_print_status(struct mtk_eth *eth);
void sfq_fill_txd5(struct sk_buff *skb, u32 *txd5);
#endif /* MTK_SFQ_H */
