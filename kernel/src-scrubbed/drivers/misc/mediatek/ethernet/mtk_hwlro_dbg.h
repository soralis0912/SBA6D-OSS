// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek HWLRO
 *
 * Author: Stella Yang <stella.yang@mediatek.com>
 *
 */


#ifndef MTK_HWLRO_DBG_H
#define MTK_HWLRO_DBG_H

#ifdef CONFIG_MTK_ETHERNET_DBG

#include "mtk_eth_soc.h"

#define MTK_LRO_RX_RING1_DIP_DW0 (LRO_BASE + 0x54)
#define MTK_LRO_RX_RING1_DIP_DW1 (LRO_BASE + 0x58)
#define MTK_LRO_RX_RING1_DIP_DW2 (LRO_BASE + 0x5C)
#define MTK_LRO_RX_RING1_DIP_DW3 (LRO_BASE + 0x60)
#define MTK_LRO_RX_RING1_SIP_DW0 (LRO_BASE + 0x64)
#define MTK_LRO_RX_RING1_SIP_DW1 (LRO_BASE + 0x68)
#define MTK_LRO_RX_RING1_SIP_DW2 (LRO_BASE + 0x6C)
#define MTK_LRO_RX_RING1_SIP_DW3 (LRO_BASE + 0x70)
#define PDMA_LRO_IPV4_CSUM_UPDATE_EN    BIT(7)
#define PDMA_LRO_IPV4_CSUM_UPDATE_EN_OFFSET (7)
#define PDMA_LRO_RELINGUISH     BITS(29, 31)
#define PDMA_LRO_EN             BIT(0)

#define MTK_LRO_RX_RING1_STP_DTP_DW	(LRO_BASE + 0x50)
#define MTK_LRO_RX_TCP_SRC_PORT_OFFSET     (16)
#define MTK_LRO_RX_TCP_DEST_PORT_OFFSET    (0)
#define MTK_LRO_RX_(_x)		(((_x) & 0x3) << 0)
#define MTK_LRO_RING_AGE1      BITS(22, 31)
#define MTK_LRO_AGE_H_OFFSET           (10)
#define MTK_LRO_RING_AGE1_OFFSET       (22)

#define MTK_RX_PORT_VALID_OFFSET       (8)
#define MTK_RX_MYIP_VALID_OFFSET       (9)
#define MTK_LRO_RING_AGE2_OFFSET       (0)

#define MTK_LRO_RING_AGG_OFFSET        (10)
#define MTK_LRO_RING_AGG               BITS(10, 25)
#define MTK_LRO_RING_AGG_CNT1          BITS(26, 31)
#define MTK_LRO_RING_AGG_CNT1_OFFSET   (26)
#define MTK_LRO_RING_AGE2      BITS(0, 5)

//#define LRO_RX_RING1_CTRL_DW3		(LRO_BASE + 0x70)
#define MTK_LRO_RING_AGG_CNT2          BITS(0, 1)
#define MTK_LRO_AGG_CNT_H_OFFSET       (6)
#define MTK_LRO_RING_AGG_CNT2_OFFSET   (0)

#define MTK_FE_ALT_CF8 0x300
#define MTK_FE_ALT_SEQ_CFC 0x308

#define MTK_LRO_ALT_SCORE_MODE         BIT(21)


struct PDMA_LRO_AUTO_TLB_INFO0_T {
	unsigned int DTP:16;
	unsigned int STP:16;
};

struct PDMA_LRO_AUTO_TLB_INFO1_T {
	unsigned int SIP0:32;
};

struct PDMA_LRO_AUTO_TLB_INFO2_T {
	unsigned int SIP1:32;
};

struct PDMA_LRO_AUTO_TLB_INFO3_T {
	unsigned int SIP2:32;
};

struct PDMA_LRO_AUTO_TLB_INFO4_T {
	unsigned int SIP3:32;
};

struct PDMA_LRO_AUTO_TLB_INFO5_T {
	unsigned int VLAN_VID0:32;
};

struct PDMA_LRO_AUTO_TLB_INFO6_T {
	unsigned int VLAN_VID1:16;
	unsigned int VLAN_VID_VLD:4;
	unsigned int CNT:12;
};

struct PDMA_LRO_AUTO_TLB_INFO7_T {
	unsigned int DW_LEN:32;
};

struct PDMA_LRO_AUTO_TLB_INFO8_T {
	unsigned int DIP_ID:2;
	unsigned int IPV6:1;
	unsigned int IPV4:1;
	unsigned int RESV:27;
	unsigned int VALID:1;
};

struct PDMA_LRO_AUTO_TLB_INFO {
	struct PDMA_LRO_AUTO_TLB_INFO0_T auto_tlb_info0;
	struct PDMA_LRO_AUTO_TLB_INFO1_T auto_tlb_info1;
	struct PDMA_LRO_AUTO_TLB_INFO2_T auto_tlb_info2;
	struct PDMA_LRO_AUTO_TLB_INFO3_T auto_tlb_info3;
	struct PDMA_LRO_AUTO_TLB_INFO4_T auto_tlb_info4;
	struct PDMA_LRO_AUTO_TLB_INFO5_T auto_tlb_info5;
	struct PDMA_LRO_AUTO_TLB_INFO6_T auto_tlb_info6;
	struct PDMA_LRO_AUTO_TLB_INFO7_T auto_tlb_info7;
	struct PDMA_LRO_AUTO_TLB_INFO8_T auto_tlb_info8;
};

#if defined(CONFIG_MTK_ADMAV2)
struct PDMA_LRO_AUTO_TLB_INFO_V2_0_T {
	unsigned int V2_ID_H:3;
	unsigned int V1_ID:12;
	unsigned int V0_ID:12;
	unsigned int V3_VALID:1;
	unsigned int V2_VALID:1;
	unsigned int V1_VALID:1;
	unsigned int V0_VALID:1;
	unsigned int VALID:1;
};

struct PDMA_LRO_AUTO_TLB_INFO_V2_1_T {
	unsigned int SIP3_H:9;
	unsigned int V6_VALID:1;
	unsigned int V4_VALID:1;
	unsigned int V3_ID:12;
	unsigned int V2_ID_L:9;
};

struct PDMA_LRO_AUTO_TLB_INFO_V2_2_T {
	unsigned int SIP2_H:9;
	unsigned int SIP3_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_3_T {
	unsigned int SIP1_H:9;
	unsigned int SIP2_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_4_T {   //0x181
	unsigned int SIP0_H:9;
	unsigned int SIP1_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_5_T {
	unsigned int DIP3_H:9;
	unsigned int SIP0_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_6_T {
	unsigned int DIP2_H:9;
	unsigned int DIP3_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_7_T {
	unsigned int DIP1_H:9;
	unsigned int DIP2_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_8_T {
	unsigned int DIP0_H:9;
	unsigned int DIP1_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_9_T {
	unsigned int SP_H:9;
	unsigned int DIP0_L:23;
};
struct PDMA_LRO_AUTO_TLB_INFO_V2_10_T {
	unsigned int RESV:9;
	unsigned int DP:16;
	unsigned int SP_L:7;
};

struct PDMA_LRO_AUTO_TLB_INFO_V2 {
	struct PDMA_LRO_AUTO_TLB_INFO_V2_0_T auto_tlb_info0;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_1_T auto_tlb_info1;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_2_T auto_tlb_info2;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_3_T auto_tlb_info3;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_4_T auto_tlb_info4;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_5_T auto_tlb_info5;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_6_T auto_tlb_info6;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_7_T auto_tlb_info7;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_8_T auto_tlb_info8;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_9_T auto_tlb_info9;
	struct PDMA_LRO_AUTO_TLB_INFO_V2_10_T auto_tlb_info10;
};

#endif
void hwlro_debug_proc_exit(struct proc_dir_entry *proc_reg_dir);
int hwlro_debug_proc_init(struct proc_dir_entry *proc_reg_dir, struct mtk_eth *eth);
int hwlrov2_debug_proc_init(struct proc_dir_entry *proc_reg_dir, struct mtk_eth *eth);

void hw_lro_stats_update(struct mtk_rx_ring *ring, struct mtk_rx_dma *dma);
void hw_lro_flush_stats_update(struct mtk_rx_ring *ring, struct mtk_rx_dma *dma);
#else /* CONFIG_MTK_ETHERNET_DBG */

/* empty function */
void hwlro_debug_proc_exit(struct proc_dir_entry *proc_reg_dir) {}
int hwlro_debug_proc_init(struct proc_dir_entry *proc_reg_dir, struct mtk_eth *eth){ return 0;}

void hw_lro_stats_update(struct mtk_rx_ring *ring, struct mtk_rx_dma *dma){}
void hw_lro_flush_stats_update(struct mtk_rx_ring *ring, struct mtk_rx_dma *dma){}

#endif /* CONFIG_MTK_ETHERNET_DBG */
#endif /* MTK_HWLRO_DBG_H */
