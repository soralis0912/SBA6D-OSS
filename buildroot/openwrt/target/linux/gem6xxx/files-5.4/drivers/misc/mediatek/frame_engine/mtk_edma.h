/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2022 MediaTek Inc.
 *
 * Author: Tino Liou <tino.liou@mediatek.com>
 */

#include <net/mtk/mtk_frame_engine.h>
#include <net/mtk/mtk_fe_dma.h>

enum edma_hw_index {
	 EDMA_IDX0,
	 EDMA_IDX1,
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	 EDMA_IDX2,
#endif
	 EDMA_MODULE_NUM,
 };

 enum edma_fc_state {
	 EDMA_FC_SUSPEND,
	 EDMA_FC_WAIT_RESUME,
	 EDMA_FC_NUM,
 };

#define MTK_HW_EDMA_RX_DMA_SIZE		(512)

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
#define MTK_HW_EDMA_RX_DMA_SIZE_RING1		MTK_HW_EDMA_RX_DMA_SIZE
#else
#define MTK_HW_EDMA_RX_DMA_SIZE_RING1		(8)
#endif

#define EDMA_TX_DMA_SIZE		(2048)
#define EDMA_TX_DMA_BUF_LEN			(0xffff)

 //Kurt: actually we can get these numbers by querying 15103A00.
#define MTK_MAX_EDMA_RX_RING_NUM	2
#define MTK_MAX_EDMA_TX_RING_NUM	1

#define MTK_DMA_BUSY_TIMEOUT	HZ

#define	MTK_MAX_RX_LENGTH	1536
#define MTK_RX_ETH_HLEN		(VLAN_ETH_HLEN + VLAN_HLEN + ETH_FCS_LEN)
#define MTK_RX_HLEN		(NET_SKB_PAD + MTK_RX_ETH_HLEN + NET_IP_ALIGN)
#define VLAN_TAG_PRESENT	VLAN_CFI_MASK

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
#define EDMA_BASE 0x5C00
#else
#define EDMA_BASE 0x3800
#endif
 /*
  * Note: all HW registers below are index by @enum edma_hw_index
  * instead of the dma channel.
  */
#define EDMA_GLO_CFG(idx) (EDMA_BASE + 0x204 + idx*0x400)
#define MTK_RX_2B_OFFSET	BIT(31)
#define MTK_TX_WB_DDONE		BIT(6)
#define MTK_RX_DMA_EN		BIT(2)
#define MTK_TX_DMA_EN		BIT(0)
#define MTK_TX_DMA_BUSY		BIT(1)
#define MTK_RX_DMA_BUSY		BIT(3)

 /* Reset Index Register */
#define EDMA_RST_IDX(idx)	(EDMA_BASE + 0x208 + idx * 0x400)
#define EDMA_RST_DTX_IDX0	BIT(0)
#define EDMA_RST_DTX_IDX_CFG(ring)	(EDMA_RST_DTX_IDX0 << (ring))
#define EDMA_RST_DRX_IDX0	BIT(16)
#define EDMA_RST_DRX_IDX_CFG(ring)	(EDMA_RST_DRX_IDX0 << (ring))

#define EDMA_TX_DELAY_INT_CFG_0(idx)	(EDMA_BASE + 0x2B0 + idx*0x400)
#define EDMA_TX_DELAY_INT_CFG_1(idx)	(EDMA_BASE + 0x2B4 + idx*0x400)
#define EDMA_RX_DELAY_INT_CFG_0(idx)	(EDMA_BASE + 0x2C0 + idx*0x400)
#define EDMA_DELAY_INT_CFG(idx)	(EDMA_BASE + 0x20C + idx*0x400)
 /* Interrupt Status/Mask Register */
#define EDMA_INT_STATUS(idx) (EDMA_BASE + 0x220 + idx*0x400)
#define EDMA_INT_MASK(idx) (EDMA_BASE + 0x228 + idx*0x400)

#define EDMA_CDM_EG_CTRL(idx) (0x1624 + idx*0x10)
#define EDMA_VLAN_UNTAG_EN		BIT(0)

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
#define EDMA_G0_INT_STATUS(idx) (EDMA_BASE + 0x240 + idx*0x400)
#define EDMA_G1_INT_STATUS(idx) (EDMA_BASE + 0x244 + idx*0x400)
#define EDMA_G2_INT_STATUS(idx) (EDMA_BASE + 0x248 + idx*0x400)
#define EDMA_G3_INT_STATUS(idx) (EDMA_BASE + 0x24C + idx*0x400)
#define EDMA_G1_INT_CTRL(idx) (EDMA_BASE + 0x250 + idx*0x400)
#define EDMA_G2_INT_CTRL(idx) (EDMA_BASE + 0x254 + idx*0x400)
#define EDMA_G3_INT_CTRL(idx) (EDMA_BASE + 0x258 + idx*0x400)
#else
#define EDMA_G0_INT_STATUS(idx) (EDMA_BASE + 0x240 + idx*0x400)
#define EDMA_G1_INT_STATUS(idx) (EDMA_BASE + 0x244 + idx*0x400)
#define EDMA_G1_INT_CTRL(idx) (EDMA_BASE + 0x250 + idx*0x400)
#endif

#define EDMA_RX_DONE_DLY		BIT(30)
#define EDMA_TX_DONE_DLY		BIT(28)
#define EDMA_RX_DONE_DLY1		BIT(21)
#define EDMA_RX_DONE_DLY0		BIT(20)

 /* TX Base Pointer Register */
#define ETX_BASE_PTR_CFG(idx, ring)	((EDMA_BASE + (ring * 0x10)) + idx * 0x400)

 /* TX Maximum Count Register */
#define ETX_MAX_CNT_CFG(idx, ring)	((EDMA_BASE + 0x4 + (ring * 0x10)) + idx * 0x400)

 /* TX CPU Pointer Register */
#define ETX_CTX_IDX_CFG(idx, ring)	((EDMA_BASE + 0x8 + (ring * 0x10)) + idx * 0x400)

 /* TX DMA Pointer Register */
#define ETX_DTX_IDX_CFG(idx, ring)	((EDMA_BASE + 0xC + (ring * 0x10)) + idx * 0x400)

 /* RX Base Pointer Register */
#define ERX_BASE_PTR_CFG(idx, ring)	((EDMA_BASE + 0x100 + (ring * 0x10)) + idx * 0x400)

 /* RX Maximum Count Register */
#define ERX_MAX_CNT_CFG(idx, ring)	((EDMA_BASE + 0x104 + (ring * 0x10)) + idx * 0x400)

 /* RX CPU Pointer Register */
#define ERX_CRX_IDX_CFG(idx, ring)	((EDMA_BASE + 0x108 + (ring * 0x10)) + idx * 0x400)

 /* RX DMA Pointer Register */
#define ERX_DRX_IDX_CFG(idx, ring)	((EDMA_BASE + 0x10C + (ring * 0x10)) + idx * 0x400)

 struct EDMA_RXD2_T {
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	 unsigned int SDP_H:4;
	 unsigned int RSV1:4;
#else
	 unsigned int RSV0:6;
	 unsigned int RSV1:2;
#endif
	 unsigned int PLEN0:16;
	 unsigned int RSV2:6;
	 unsigned int LS0:1;
	 unsigned int DDONE_bit:1;
 };

 struct EDMA_RXD3_T {
	 unsigned int TAG:1;
	 unsigned int L4F:1;
	 unsigned int L4VLD:1;
	 unsigned int TACK:1;
	 unsigned int IP4F:1;
	 unsigned int IP4:1;
	 unsigned int IP6:1;
	 unsigned int RSV0:5;
	 unsigned int RSV1:20;
 };

 struct EDMA_RXD4_T {
	 unsigned int VID:16;
	 unsigned int VPID:16;
 };

 struct EDMA_RXD5_T {
	 unsigned int FOE_ENTRY:15;
	 unsigned int RSV0:3;
	 unsigned int CRSN:5;
	 unsigned int RSV1:3;
	 unsigned int SP:4;
	 unsigned int RSV2:2;
 };

 // struct EDMA_TXD1_T :unsigned int SDP0;

 struct EDMA_TXD2_T {
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	 unsigned int SDP_H:4;
	 unsigned int RSV1:4;
#else
	 unsigned int RSV0:6;
	 unsigned int RSV1:2;
#endif
	 unsigned int SDL0:16;
	 unsigned int RSV2:6;
	 unsigned int LS0:1;
	 unsigned int DDONE:1;
 };

 // struct EDMA_TXD3_T :unsigned int SDP1;

 struct EDMA_TXD4_T {
	 unsigned int RSV0:6;
	 unsigned int RSV1:2;
	 unsigned int SDL1:16;
	 unsigned int RSV2:6;
	 unsigned int LS1:1;
	 unsigned int BURST:1;
 };

 struct EDMA_TXD5_T {
	 unsigned int RSV0:16;
	 unsigned int FPORT:4;
	 unsigned int RSV1:2;
	 unsigned int RSV2:6;
	 unsigned int TUI_CO:3;
	 unsigned int TSO:1;
 };

 struct EDMA_TXD6_T {
	 unsigned int VLAN_TAG_1:16;
	 unsigned int INSV_1:1;
	 unsigned int RSV0:14;
	 unsigned int INSV_0:1;
 };

 struct edma_descriptor {
	 unsigned int desc1;
	 unsigned int desc2;
	 unsigned int desc3;
	 unsigned int desc4;
	 unsigned int desc5;
	 unsigned int desc6;
	 unsigned int desc7;
	 unsigned int desc8;
 } __packed __aligned(4);

 struct _dummy_rxd_t {
	 int desc[5];
 };


 /* struct edma_rx_ring -	 This struct holds info describing a RX ring
  * @dma:		 The descriptor ring
  * @data:		 The memory pointed at by the ring
  * @phys:		 The physical addr of rx_buf
  * @frag_size: 	 How big can each fragment be
  * @buf_size:		 The size of each packet buffer
  * @cpu_idx:		 The current head of ring
  * @fc_state:		 The state of buffer flow control
  * @suspend_cnt:		 The counter of flow control suspend
  * @resume_cnt:	 The counter of flow control resume
  */
 struct edma_rx_ring {
	 struct edma_descriptor *dma;
	 struct dma_rx_buf **bufs;
	 dma_addr_t phys;
	 u16 frag_size;
	 u16 buf_size;
	 u16 dma_size;
	 bool cpu_idx_update;
	 u16 cpu_idx;

	 unsigned long fc_state;
	 int suspend_cnt;
	 int resume_cnt;
	 int max_distance;

	 int free_buf_cnt;
	 int recycle_buf_cnt;
	 struct list_head *free_buf_pool;
	 struct list_head *recycle_pool;
	 struct list_head __buf_pool_1;
	 struct list_head __buf_pool_2;
	 spinlock_t pool_swap_lock;
	 spinlock_t suspend_lock;
 };

 enum edma_tx_flags {
	 /* EDMA descriptor can point at 1-2 segments. This enum allows us to
	  * track how memory was allocated so that it can be freed properly.
	  */
	 TX_FLAGS_SINGLE0	 = 0x01,
	 TX_FLAGS_PAGE0  = 0x02,
 };

 /* struct mtk_tx_buf -  This struct holds the pointers to the memory pointed at
  * 		 by the TX descriptors
  * @obj:	 The pointer of the packet being sent. (might be skb, or raw buf)
  * @len:	 the length of the buffer.
  * @flags:  the information of this buffer. @see enum mtk_tx_flags.
  * @dma_addr0:  The base addr of the first segment
  * @dma_len0:	 The length of the first segment
  * @dma_addr1:  The base addr of the second segment
  * @dma_len1:	 The length of the second segment
  */
 struct mtk_tx_buf {
	 void *obj;
	 int len;
	 u32 flags;
	 DEFINE_DMA_UNMAP_ADDR(dma_addr0);
	 DEFINE_DMA_UNMAP_LEN(dma_len0);
	 DEFINE_DMA_UNMAP_ADDR(dma_addr1);
	 DEFINE_DMA_UNMAP_LEN(dma_len1);
 };

 /* struct mtk_tx_ring - This struct holds info describing a TX ring
  * @dma:	 The descriptor ring
  * @buf:	 The memory pointed at by the ring
  * @phys:	 The physical addr of tx_buf
  * @thresh: The threshold of minimum amount of free descriptors
  * @free_count: Track how many free descriptors are present.
  * @dma_size:	 size of the ring.
  * @cpu_idx:	 Pointer to the latest unfinished tx desc/buf.
  */
 struct edma_tx_ring {
	 struct edma_descriptor *dma;
	 struct mtk_tx_buf *buf;
	 dma_addr_t phys;
	 u16 thresh;
	 atomic_t free_count;
	 int dma_size;
	 int cpu_idx;
 };

 /*
  * EDMA main datasructure for holding the state, contains tx/rx rings,
  * base address.
  *
  * @dev:	 The device pointer
  * @base:	 The mapped register i/o base
  * @rx_ring:	 Pointers to the memory holding info about the RX rings
  * @tx_ring:	 Pointers to the memory holding info about the TX rings
  * @irq_lock:	 Make sure that IRQ register operations are atomic
  * @fe_dma: Pointers to the coresponding objects. @see struct mtk_fe_dma
  */
 struct mtk_edma_module {
	 struct device		 *dev;
	 void __iomem		 *base;

	 struct edma_rx_ring rx_ring[EDMA_MODULE_NUM][MTK_MAX_EDMA_RX_RING_NUM];
	 struct edma_tx_ring tx_ring[EDMA_MODULE_NUM][MTK_MAX_EDMA_TX_RING_NUM];
	 spinlock_t 	 irq_lock[EDMA_MODULE_NUM];
	 struct mtk_fe_dma	 *fe_dma[EDMA_MODULE_NUM];

	 u32		 msg_enable; // used by netif_msg_intr(edma)
 };
 //static struct mtk_edma_module *g_edma;

 struct mtk_edma {
	 struct mtk_fe_dma	 fe_dma;
	 u32		 edma_idx;
	 struct mtk_edma_module  *module;
 };

#define mtk_edma_w32(edma, val, reg)	__raw_writel(val, edma->base + reg)
#define mtk_edma_r32(edma, reg)			__raw_readl(edma->base + reg)

