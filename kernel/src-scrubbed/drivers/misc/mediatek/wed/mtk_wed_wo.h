// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2021 Felix Fietkau <nbd@nbd.name> */

#ifndef __MTK_WED_WO_H
#define __MTK_WED_WO_H

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include "mtk_wed.h"

#define WED_CTL_SD_LEN1		GENMASK(13, 0)
#define WED_CTL_LAST_SEC1	BIT(14)
#define WED_CTL_BURST		BIT(15)
#define WED_CTL_SD_LEN0_SHIFT	16
#define WED_CTL_SD_LEN0		GENMASK(29, 16)
#define WED_CTL_LAST_SEC0	BIT(30)
#define WED_CTL_DMA_DONE	BIT(31)
#define WED_INFO_WINFO		GENMASK(15, 0)

#define MTK_WED_WO_TXQ_FREE_THR		10

#define WED_WO_PROFILE_MAX_LVL		6


enum mtk_wed_fw_region_id {
	WO_REGION_EMI = 0,
	WO_REGION_ILM,
	WO_REGION_DATA,
	WO_REGION_BOOT,
	__WO_REGION_MAX
};

struct wed_wo_profile_stat {
	u32 bound;
	u32 record;
};

#define PROFILE_STAT(record, val) do {			\
		u8 lvl = 0;				\
		while (lvl < WED_WO_PROFILE_MAX_LVL) {	\
			if (val < record[lvl].bound) {	\
				record[lvl].record++;	\
				break;			\
			}				\
			lvl++;				\
		}					\
	} while (0)

/* align with wo report structure */
struct wed_wo_log {
	u32 sn;
	u32 total;
	u32 rro;
	u32 mod;
};

struct wed_wo_rxcnt {
	u16 wlan_idx;
	u16 tid;
	u32 rx_pkt_cnt;
	u32 rx_byte_cnt;
	u32 rx_err_cnt;
	u32 rx_drop_cnt;
};

struct wed_wo_queue {
	struct wed_wo_queue_regs *regs;

	spinlock_t lock;
	spinlock_t cleanup_lock;
	struct wed_wo_queue_entry *entry;
	struct wed_wo_desc *desc;

	u16 first;
	u16 head;
	u16 tail;
	int ndesc;
	int queued;
	int buf_size;

	u8 hw_idx;
	u8 qid;
	u8 flags;

	dma_addr_t desc_dma;
	struct page_frag_cache rx_page;
	struct page_frag_cache tx_page;
};


struct wed_wo_mmio {
	struct regmap *regs;

	spinlock_t irq_lock;
	u8 irq;
	u32 irqmask;

	u32 q_int_mask;
	u32 q_exep_mask;
};

struct wed_wo_mcu {
	struct mutex mutex;
	u32 msg_seq;
	int timeout;

	struct sk_buff_head res_q;
	wait_queue_head_t wait;
};

struct wed_wo_exception {
	void* log;
	int log_size;
	dma_addr_t phys;
};

struct wed_wo_queue_regs {
	u32 desc_base;
	u32 ring_size;
	u32 cpu_idx;
	u32 dma_idx;
};

struct wed_wo_desc {
	__le32 buf0;
	__le32 ctrl;
	__le32 buf1;
	__le32 info;
	__le32 reserved[4];
} __packed __aligned(32);

struct wed_wo_queue_entry {
	union {
		void *buf;
		struct sk_buff *skb;
	};

	u32 dma_addr;
	u16 dma_len;
	u16 wcid;
	bool skip_buf0:1;
	bool skip_buf1:1;
	bool done:1;
};

struct wo_cmd_query {
	u32 query0;
	u32 query1;
};

struct wed_cmd_hdr {
	/*DW0*/
	u8 ver;
	u8 cmd_id;
	u16 length;

	/*DW1*/
	u16 uni_id;
	u16 flag;

	/*DW2*/
	int status;

	/*DW3*/
	u8 reserved[20];
};

struct mtk_wed_fw_region {
	void *addr;
	u32 addr_pa;
	u32 size;
	u32 shared;
};

struct wed_wo_queue_ops;
struct wed_wo_drv_ops;
struct wed_wo_mcu_ops;

struct wo_rx_total_cnt {
	u64 rx_pkt_cnt;
	u64 rx_byte_cnt;
	u64 rx_err_cnt;
	u64 rx_drop_cnt;
};

struct mtk_wed_wo {
	struct mtk_wed_hw *hw;

	struct wed_wo_mmio ccif;
	struct wed_wo_mcu mcu;
	struct wed_wo_exception exp;

	const struct wed_wo_drv_ops *drv_ops;
	const struct wed_wo_mcu_ops *mcu_ops;
	const struct wed_wo_queue_ops *queue_ops;

	struct net_device napi_dev;
	spinlock_t rx_lock;
	struct napi_struct napi;
	struct sk_buff_head rx_skb;
	struct wed_wo_queue q_rx;
	struct tasklet_struct irq_tasklet;

	struct wed_wo_queue q_tx;

	struct mtk_wed_fw_region region[__WO_REGION_MAX];

	struct wed_wo_profile_stat total[WED_WO_PROFILE_MAX_LVL];
	struct wed_wo_profile_stat mod[WED_WO_PROFILE_MAX_LVL];
	struct wed_wo_profile_stat rro[WED_WO_PROFILE_MAX_LVL];
	char dirname[4];
	struct wo_rx_total_cnt wo_rxcnt[8][544];
};

struct wed_wo_queue_ops {
	int (*init)(struct mtk_wed_wo *wo,
		    int (*poll)(struct napi_struct *napi, int budget));

	int (*alloc)(struct mtk_wed_wo *wo, struct wed_wo_queue *q,
		     int idx, int n_desc, int bufsize,
		     struct wed_wo_queue_regs *regs);
	void (*free)(struct mtk_wed_wo *wo, struct wed_wo_queue *q);
	void (*reset)(struct mtk_wed_wo *wo, struct wed_wo_queue *q);

	int (*tx_skb)(struct mtk_wed_wo *wo, struct wed_wo_queue *q,
		      struct sk_buff *skb);
	void (*tx_clean)(struct mtk_wed_wo *wo, struct wed_wo_queue *q);

	void (*rx_clean)(struct mtk_wed_wo *wo, struct wed_wo_queue *q);

	void (*kick)(struct mtk_wed_wo *wo, struct wed_wo_queue *q, int offset);
};

struct wed_wo_drv_ops {
	void (*kickout)(struct mtk_wed_wo *wo);
	void (*set_ack)(struct mtk_wed_wo *wo, u32 mask);
	void (*set_isr)(struct mtk_wed_wo *wo, u32 mask);
	u32 (*get_csr)(struct mtk_wed_wo *wo);
	int (*tx_prepare_skb)(struct mtk_wed_wo *wo);
	bool (*check_excpetion)(struct mtk_wed_wo *wo);
	void (*clear_int)(struct mtk_wed_wo *wo, u32 mask);
};

struct wed_wo_mcu_ops {
	u32 headroom;

	int (*mcu_skb_send_msg)(struct mtk_wed_wo *wo, int to_id,
				int cmd, struct sk_buff *skb,
				int *seq, bool wait_resp);

	int (*mcu_parse_response)(struct mtk_wed_wo *wo, int cmd,
				  struct sk_buff *skb, int seq);

	int (*mcu_restart)(struct mtk_wed_wo *wo);
};

#define mtk_wed_wo_q_init(wo, ...)	(wo)->queue_ops->init((wo), __VA_ARGS__)
#define mtk_wed_wo_q_alloc(wo, ...)	(wo)->queue_ops->alloc((wo), __VA_ARGS__)
#define mtk_wed_wo_q_free(wo, ...)	(wo)->queue_ops->free((wo), __VA_ARGS__)
#define mtk_wed_wo_q_reset(wo, ...)	(wo)->queue_ops->reset((wo), __VA_ARGS__)
#define mtk_wed_wo_q_tx_skb(wo, ...)	(wo)->queue_ops->tx_skb((wo), __VA_ARGS__)
#define mtk_wed_wo_q_tx_clean(wo, ...)	(wo)->queue_ops->tx_clean((wo), __VA_ARGS__)
#define mtk_wed_wo_q_rx_clean(wo, ...)	(wo)->queue_ops->rx_clean((wo), __VA_ARGS__)
#define mtk_wed_wo_q_kick(wo, ...)	(wo)->queue_ops->kick((wo), __VA_ARGS__)

enum {
	WARP_CMD_FLAG_RSP		= 1 << 0, /* is responce*/
	WARP_CMD_FLAG_NEED_RSP		= 1 << 1, /* need responce */
	WARP_CMD_FLAG_FROM_TO_WO	= 1 << 2, /* send between host and wo */
};

#define WED_WO_CMD_FLAG_IS_RSP(_hdr)		((_hdr)->flag & (WARP_CMD_FLAG_RSP))
#define WED_WO_CMD_FLAG_SET_RSP(_hdr)		((_hdr)->flag |= (WARP_CMD_FLAG_RSP))
#define WED_WO_CMD_FLAG_IS_NEED_RSP(_hdr)	((_hdr)->flag & (WARP_CMD_FLAG_NEED_RSP))
#define WED_WO_CMD_FLAG_SET_NEED_RSP(_hdr)	((_hdr)->flag |= (WARP_CMD_FLAG_NEED_RSP))
#define WED_WO_CMD_FLAG_IS_FROM_TO_WO(_hdr)	((_hdr)->flag & (WARP_CMD_FLAG_FROM_TO_WO))
#define WED_WO_CMD_FLAG_SET_FROM_TO_WO(_hdr)	((_hdr)->flag |= (WARP_CMD_FLAG_FROM_TO_WO))

void mtk_wed_wo_set_isr_mask(struct mtk_wed_wo *wo, bool set,
			     u32 clear, u32 val);

static inline void mtk_wed_wo_isr_enable(struct mtk_wed_wo *wo, u32 mask)
{
	mtk_wed_wo_set_isr_mask(wo, false, 0, mask);

	tasklet_schedule(&wo->irq_tasklet);
}

static inline void mtk_wed_wo_isr_disable(struct mtk_wed_wo *wo, u32 mask)
{
	mtk_wed_wo_set_isr_mask(wo, true, mask, 0);
}

static inline void
wo_w32(struct mtk_wed_wo *dev, u32 reg, u32 val)
{
	writel(val, dev->region[WO_REGION_BOOT].addr + reg);
}

static inline u32
wo_r32(struct mtk_wed_wo *dev, u32 reg)
{
	return readl(dev->region[WO_REGION_BOOT].addr + reg);
}
static inline void
woccif_w32(struct mtk_wed_wo *dev, u32 reg, u32 val)
{
	regmap_write(dev->ccif.regs, reg, val);
}

static inline u32
woccif_r32(struct mtk_wed_wo *dev, u32 reg)
{
	unsigned int val;

	regmap_read(dev->ccif.regs, reg, &val);

	return val;
}

int mtk_wed_wo_init(struct mtk_wed_hw *hw);
void mtk_wed_wo_exit(struct mtk_wed_hw *hw);
#endif

