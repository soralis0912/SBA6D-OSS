// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek HWLRO
 *
 * Author: Stella Yang <stella.yang@mediatek.com>
 *
 */


#include <linux/trace_seq.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include "mtk_eth_soc.h"
#include "mtk_hwlro_dbg.h"

static struct mtk_eth *g_eth;

typedef int (*HWLRO_DBG_FUNC) (int par1, int par2);
unsigned int hw_lro_agg_num_cnt[MTK_MAX_RX_RING_NUM][MTK_HW_LRO_MAX_AGG_CNT + 1];
unsigned int hw_lro_agg_size_cnt[MTK_MAX_RX_RING_NUM][16];
unsigned int hw_lro_tot_agg_cnt[MTK_MAX_RX_RING_NUM];
unsigned int hw_lro_tot_flush_cnt[MTK_MAX_RX_RING_NUM];

/* HW LRO flush reason proc */
#define HW_LRO_AGG_FLUSH        (1)
#define HW_LRO_AGE_FLUSH        (2)
#define HW_LRO_NOT_IN_SEQ_FLUSH (3)
#define HW_LRO_TIMESTAMP_FLUSH  (4)
#define HW_LRO_NON_RULE_FLUSH   (5)

unsigned int hw_lro_agg_flush_cnt[MTK_MAX_RX_RING_NUM];
unsigned int hw_lro_age_flush_cnt[MTK_MAX_RX_RING_NUM];
unsigned int hw_lro_seq_flush_cnt[MTK_MAX_RX_RING_NUM];
unsigned int hw_lro_timestamp_flush_cnt[MTK_MAX_RX_RING_NUM];
unsigned int hw_lro_norule_flush_cnt[MTK_MAX_RX_RING_NUM];

#define PROCREG_RXRING1		"rx_ring1"
#define PROCREG_RXRING2		"rx_ring2"
#define PROCREG_RXRING3		"rx_ring3"
#if defined(CONFIG_MTK_ADMAV2)
#define PROCREG_RXRING4		"rx_ring4"
#define PROCREG_RXRING5		"rx_ring5"
#define PROCREG_RXRING6		"rx_ring6"
#define PROCREG_RXRING7		"rx_ring7"
#endif

#define PROCREG_HW_LRO_STATS	"hw_lro_stats"
#define PROCREG_HW_LRO_AUTO_TLB	"hw_lro_auto_tlb"

static struct proc_dir_entry *proc_rx_ring1, *proc_rx_ring2, *proc_rx_ring3;
#if defined(CONFIG_MTK_ADMAV2)
static struct proc_dir_entry *proc_rx_ring4, *proc_rx_ring5, *proc_rx_ring6,*proc_rx_ring7;
#endif

static struct proc_dir_entry *proc_hw_lro_stats, *proc_hw_lro_auto_tlb;

int rx_lro_ring_read(struct seq_file *seq, void *v,
		     struct mtk_rx_dma *rx_dma_p)
{
	struct mtk_rx_dma *rx_dma;
	int i = 0;

	rx_dma =
	    vmalloc(sizeof(struct mtk_rx_dma) * MTK_HW_LRO_DMA_SIZE);
	if (!rx_dma) {
		seq_puts(seq, " allocate temp rx_dma fail.\n");
		return 0;
	}

	memcpy(rx_dma, rx_dma_p, sizeof(struct mtk_rx_dma) * MTK_HW_LRO_DMA_SIZE);

#if defined(CONFIG_MTK_ADMAV2)
	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x %08x %08x\n", i,
			   rx_dma[i].rxd1, rx_dma[i].rxd2, rx_dma[i].rxd3, rx_dma[i].rxd4,
			   rx_dma[i].rxd5, rx_dma[i].rxd6);
	}
#else
	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   rx_dma[i].rxd1,
			   rx_dma[i].rxd2,
			   rx_dma[i].rxd3,
			   rx_dma[i].rxd4);
	}
#endif
	vfree(rx_dma);
	return 0;
}

int rx_ring1_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[1].dma);

	return 0;
}

int rx_ring2_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[2].dma);

	return 0;
}

int rx_ring3_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[3].dma);

	return 0;
}

#if defined(CONFIG_MTK_ADMAV2)
int rx_ring4_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[4].dma);

	return 0;
}
int rx_ring5_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[5].dma);

	return 0;
}
int rx_ring6_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[6].dma);

	return 0;
}
int rx_ring7_read(struct seq_file *seq, void *v)
{
	rx_lro_ring_read(seq, v, g_eth->rx_ring[7].dma);

	return 0;
}
#endif




static int rx_ring1_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring1_read, NULL);
}

static int rx_ring2_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring2_read, NULL);
}

static int rx_ring3_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring3_read, NULL);
}

#if defined(CONFIG_MTK_ADMAV2)
static int rx_ring4_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring4_read, NULL);
}

static int rx_ring5_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring5_read, NULL);
}

static int rx_ring6_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring6_read, NULL);
}

static int rx_ring7_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring7_read, NULL);
}
#endif

static const struct file_operations rx_ring1_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring2_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring3_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#if defined(CONFIG_MTK_ADMAV2)
static const struct file_operations rx_ring4_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring4_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring5_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring5_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring6_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring6_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring7_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring7_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif



static int hw_lro_len_update(unsigned int agg_size)
{
	int len_idx;

	if (agg_size > 65000)
		len_idx = 13;
	else if (agg_size > 60000)
		len_idx = 12;
	else if (agg_size > 55000)
		len_idx = 11;
	else if (agg_size > 50000)
		len_idx = 10;
	else if (agg_size > 45000)
		len_idx = 9;
	else if (agg_size > 40000)
		len_idx = 8;
	else if (agg_size > 35000)
		len_idx = 7;
	else if (agg_size > 30000)
		len_idx = 6;
	else if (agg_size > 25000)
		len_idx = 5;
	else if (agg_size > 20000)
		len_idx = 4;
	else if (agg_size > 15000)
		len_idx = 3;
	else if (agg_size > 10000)
		len_idx = 2;
	else if (agg_size > 5000)
		len_idx = 1;
	else
		len_idx = 0;

	return len_idx;
}

static unsigned int hw_lro_get_ring_idx(struct mtk_rx_ring *ring) {

	int i;

#if defined(CONFIG_MTK_ADMAV2)
	for (i = MTK_LRO_RX_RING_BASE;
		i < MTK_MAX_RX_RING_NUM; i++) {
		if (ring == &g_eth->rx_ring[i])
			return i;
	}
#else
	for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
		if (ring == &g_eth->rx_ring[i])
			return i;

	}
#endif
	return 0;
}

bool isLroRing(int ring_num) {

#if defined(CONFIG_MTK_ADMAV2)
	if ((ring_num >= MTK_LRO_RX_RING_BASE) &&
		(ring_num < MTK_MAX_RX_RING_NUM))
		return true;
#else

	if ((ring_num > 0) && (ring_num < 4))
		return true;
#endif
	return false;
}

void hw_lro_stats_update(struct mtk_rx_ring *ring, struct mtk_rx_dma *dma)
{

#if defined(CONFIG_MTK_ADMAV2)
	unsigned int agg_cnt = RX_DMA_LRO_AGG_CNT(dma->rxd6);
	unsigned int agg_size = RX_DMA_GET_PLEN0(dma->rxd2);
#else
	unsigned int agg_cnt = RX_DMA_LRO_AGG_CNT(dma->rxd2);
	unsigned int agg_size = (RX_DMA_PLEN1(dma->rxd2) << 14) |
			 RX_DMA_GET_PLEN0(dma->rxd2);
#endif

	unsigned int ring_num = hw_lro_get_ring_idx(ring);

	if (isLroRing(ring_num)) {
		hw_lro_agg_size_cnt[ring_num]
				   [hw_lro_len_update(agg_size)]++;
		hw_lro_agg_num_cnt[ring_num][agg_cnt]++;
		hw_lro_tot_flush_cnt[ring_num]++;
		hw_lro_tot_agg_cnt[ring_num] += agg_cnt;
	}
}

void hw_lro_flush_stats_update(struct mtk_rx_ring *ring, struct mtk_rx_dma *dma)
{

#if defined(CONFIG_MTK_ADMAV2)
	unsigned int flush_reason = RX_DMA_REV(dma->rxd6);
#else
	unsigned int flush_reason = RX_DMA_REV(dma->rxd2);
#endif
	unsigned int ring_num = hw_lro_get_ring_idx(ring);

	if (isLroRing(ring_num)) {
		if ((flush_reason & 0x7) == HW_LRO_AGG_FLUSH)
			hw_lro_agg_flush_cnt[ring_num]++;
		else if ((flush_reason & 0x7) == HW_LRO_AGE_FLUSH)
			hw_lro_age_flush_cnt[ring_num]++;
		else if ((flush_reason & 0x7) == HW_LRO_NOT_IN_SEQ_FLUSH)
			hw_lro_seq_flush_cnt[ring_num]++;
		else if ((flush_reason & 0x7) == HW_LRO_TIMESTAMP_FLUSH)
			hw_lro_timestamp_flush_cnt[ring_num]++;
		else if ((flush_reason & 0x7) == HW_LRO_NON_RULE_FLUSH)
			hw_lro_norule_flush_cnt[ring_num]++;
	}
}

ssize_t hw_lro_stats_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *data)
{
	memset(hw_lro_agg_num_cnt, 0, sizeof(hw_lro_agg_num_cnt));
	memset(hw_lro_agg_size_cnt, 0, sizeof(hw_lro_agg_size_cnt));
	memset(hw_lro_tot_agg_cnt, 0, sizeof(hw_lro_tot_agg_cnt));
	memset(hw_lro_tot_flush_cnt, 0, sizeof(hw_lro_tot_flush_cnt));
	memset(hw_lro_agg_flush_cnt, 0, sizeof(hw_lro_agg_flush_cnt));
	memset(hw_lro_age_flush_cnt, 0, sizeof(hw_lro_age_flush_cnt));
	memset(hw_lro_seq_flush_cnt, 0, sizeof(hw_lro_seq_flush_cnt));
	memset(hw_lro_timestamp_flush_cnt, 0,
	       sizeof(hw_lro_timestamp_flush_cnt));
	memset(hw_lro_norule_flush_cnt, 0, sizeof(hw_lro_norule_flush_cnt));

	pr_info("clear hw lro cnt table\n");

	return count;
}

#if defined(CONFIG_MTK_ADMAV2)
int hw_lro_stats_read(struct seq_file *seq, void *v)
{
	int i;

	seq_puts(seq, "HW LRO statistic dump:\n");

	/* Agg number count */
	seq_puts(seq, "Cnt:\tRING4\tRING5\tRING6\tRING7\tTotal\n");
	for (i = 0; i <= MTK_HW_LRO_MAX_AGG_CNT; i++) {
		seq_printf(seq, "%d:\t%d\t%d\t%d\t%d\t%d\n",
			   i, hw_lro_agg_num_cnt[4][i],
			   hw_lro_agg_num_cnt[5][i], hw_lro_agg_num_cnt[6][i],
			   hw_lro_agg_num_cnt[7][i],
			   hw_lro_agg_num_cnt[4][i] + hw_lro_agg_num_cnt[5][i] +
			   hw_lro_agg_num_cnt[6][i] + hw_lro_agg_num_cnt[7][i]);
	}

	/* Total agg count */
	seq_puts(seq, "Total agg:\tRING4\tRING5\tRING6\tRING7\tTotal\n");
	seq_printf(seq, "\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_tot_agg_cnt[4], hw_lro_tot_agg_cnt[5],
		   hw_lro_tot_agg_cnt[6], hw_lro_tot_agg_cnt[7],
		   hw_lro_tot_agg_cnt[4] + hw_lro_tot_agg_cnt[5] +
		   hw_lro_tot_agg_cnt[6] + hw_lro_tot_agg_cnt[7]);

	/* Total flush count */
	seq_puts(seq, "Total flush:\tRING4\tRING5\tRING6\tRING7\tTotal\n");
	seq_printf(seq, "\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_tot_flush_cnt[4], hw_lro_tot_flush_cnt[5],
		   hw_lro_tot_flush_cnt[6], hw_lro_tot_flush_cnt[7],
		   hw_lro_tot_flush_cnt[4] + hw_lro_tot_flush_cnt[5] +
		   hw_lro_tot_flush_cnt[6] + hw_lro_tot_flush_cnt[7]);

	/* Avg agg count */
	seq_puts(seq, "Avg agg:\tRING4\tRING5\tRING6\tRING7\tTotal\n");
	seq_printf(seq, "\t%d\t%d\t%d\t%d\t%d\n",
		   (hw_lro_tot_flush_cnt[4]) ? hw_lro_tot_agg_cnt[4] /
		   hw_lro_tot_flush_cnt[4] : 0,
		   (hw_lro_tot_flush_cnt[5]) ? hw_lro_tot_agg_cnt[5] /
		   hw_lro_tot_flush_cnt[5] : 0,
		   (hw_lro_tot_flush_cnt[6]) ? hw_lro_tot_agg_cnt[6] /
		   hw_lro_tot_flush_cnt[6] : 0,
			(hw_lro_tot_flush_cnt[7]) ? hw_lro_tot_agg_cnt[7] /
			hw_lro_tot_flush_cnt[7] : 0,
		   (hw_lro_tot_flush_cnt[4] + hw_lro_tot_flush_cnt[5] +
		    hw_lro_tot_flush_cnt[6] + hw_lro_tot_flush_cnt[7]) ?
		    ((hw_lro_tot_agg_cnt[4] + hw_lro_tot_agg_cnt[5] +
			hw_lro_tot_agg_cnt[6] + hw_lro_tot_agg_cnt[7]) /
						(hw_lro_tot_flush_cnt[4] +
						 hw_lro_tot_flush_cnt[5] +
						 hw_lro_tot_flush_cnt[6] +
						 hw_lro_tot_flush_cnt[7])) : 0);

	/*  Statistics of aggregation size counts */
	seq_puts(seq, "HW LRO flush pkt len:\n");
	seq_puts(seq, " Length\tRING4\tRING5\tRING6\tRING7\tTotal\n");
	for (i = 0; i < 15; i++) {
		seq_printf(seq, "%d~%d:\t%d\t%d\t%d\t%d\t%d\n", i * 5000,
			   (i + 1) * 5000, hw_lro_agg_size_cnt[4][i],
			   hw_lro_agg_size_cnt[5][i], hw_lro_agg_size_cnt[6][i],
			   hw_lro_agg_size_cnt[7][i],
			   hw_lro_agg_size_cnt[4][i] +
			   hw_lro_agg_size_cnt[5][i] +
			   hw_lro_agg_size_cnt[6][i] + hw_lro_agg_size_cnt[7][i]);
	}

	seq_puts(seq, "Flush reason:\tRING4\tRING5\tRING6\tRING7\tTotal\n");
	seq_printf(seq, "AGG timeout:\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_agg_flush_cnt[4], hw_lro_agg_flush_cnt[5],
		   hw_lro_agg_flush_cnt[6], hw_lro_agg_flush_cnt[7],
		   (hw_lro_agg_flush_cnt[4] + hw_lro_agg_flush_cnt[5] +
		    hw_lro_agg_flush_cnt[6]+ hw_lro_agg_flush_cnt[7])
	    );
	seq_printf(seq, "AGE timeout:\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_age_flush_cnt[4], hw_lro_age_flush_cnt[5],
		   hw_lro_age_flush_cnt[6], hw_lro_age_flush_cnt[7],
		   (hw_lro_age_flush_cnt[4] + hw_lro_age_flush_cnt[5] +
		    hw_lro_age_flush_cnt[6] + hw_lro_age_flush_cnt[7])
	    );
	seq_printf(seq, "Not in-sequence:\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_seq_flush_cnt[4], hw_lro_seq_flush_cnt[5],
		   hw_lro_seq_flush_cnt[6],hw_lro_seq_flush_cnt[7],
		   (hw_lro_seq_flush_cnt[4] + hw_lro_seq_flush_cnt[5] +
		    hw_lro_seq_flush_cnt[6] + hw_lro_seq_flush_cnt[7])
	    );
	seq_printf(seq, "Timestamp:\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_timestamp_flush_cnt[4],
		   hw_lro_timestamp_flush_cnt[5],
		   hw_lro_timestamp_flush_cnt[6],
		hw_lro_timestamp_flush_cnt[7],
		   (hw_lro_timestamp_flush_cnt[4] +
		    hw_lro_timestamp_flush_cnt[5] +
		    hw_lro_timestamp_flush_cnt[6] + hw_lro_timestamp_flush_cnt[7])
	    );
	seq_printf(seq, "No LRO rule:\t%d\t%d\t%d\t%d\t%d\n",
		   hw_lro_norule_flush_cnt[4],
		   hw_lro_norule_flush_cnt[5],
		   hw_lro_norule_flush_cnt[6],
		   hw_lro_norule_flush_cnt[7],
		   (hw_lro_norule_flush_cnt[4] +
		    hw_lro_norule_flush_cnt[5] +
		    hw_lro_norule_flush_cnt[6] + hw_lro_norule_flush_cnt[7])
	    );

	return 0;
}


#else
int hw_lro_stats_read(struct seq_file *seq, void *v)
{
	int i;

	seq_puts(seq, "HW LRO statistic dump:\n");

	/* Agg number count */
	seq_puts(seq, "Cnt:   RING1 | RING2 | RING3 | Total\n");
	for (i = 0; i <= MTK_HW_LRO_MAX_AGG_CNT; i++) {
		seq_printf(seq, " %d :      %d        %d        %d        %d\n",
			   i, hw_lro_agg_num_cnt[0][i],
			   hw_lro_agg_num_cnt[1][i], hw_lro_agg_num_cnt[2][i],
			   hw_lro_agg_num_cnt[0][i] + hw_lro_agg_num_cnt[1][i] +
			   hw_lro_agg_num_cnt[2][i]);
	}

	/* Total agg count */
	seq_puts(seq, "Total agg:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   hw_lro_tot_agg_cnt[0], hw_lro_tot_agg_cnt[1],
		   hw_lro_tot_agg_cnt[2],
		   hw_lro_tot_agg_cnt[0] + hw_lro_tot_agg_cnt[1] +
		   hw_lro_tot_agg_cnt[2]);

	/* Total flush count */
	seq_puts(seq, "Total flush:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   hw_lro_tot_flush_cnt[0], hw_lro_tot_flush_cnt[1],
		   hw_lro_tot_flush_cnt[2],
		   hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		   hw_lro_tot_flush_cnt[2]);

	/* Avg agg count */
	seq_puts(seq, "Avg agg:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   (hw_lro_tot_flush_cnt[0]) ? hw_lro_tot_agg_cnt[0] /
		   hw_lro_tot_flush_cnt[0] : 0,
		   (hw_lro_tot_flush_cnt[1]) ? hw_lro_tot_agg_cnt[1] /
		   hw_lro_tot_flush_cnt[1] : 0,
		   (hw_lro_tot_flush_cnt[2]) ? hw_lro_tot_agg_cnt[2] /
		   hw_lro_tot_flush_cnt[2] : 0,
		   (hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		    hw_lro_tot_flush_cnt[2]) ? ((hw_lro_tot_agg_cnt[0] +
						 hw_lro_tot_agg_cnt[1] +
						 hw_lro_tot_agg_cnt[2]) /
						(hw_lro_tot_flush_cnt[0] +
						 hw_lro_tot_flush_cnt[1] +
						 hw_lro_tot_flush_cnt[2])) : 0);

	/*  Statistics of aggregation size counts */
	seq_puts(seq, "HW LRO flush pkt len:\n");
	seq_puts(seq, " Length  | RING1  | RING2  | RING3  | Total\n");
	for (i = 0; i < 15; i++) {
		seq_printf(seq, "%d~%d: %d      %d      %d      %d\n", i * 5000,
			   (i + 1) * 5000, hw_lro_agg_size_cnt[0][i],
			   hw_lro_agg_size_cnt[1][i], hw_lro_agg_size_cnt[2][i],
			   hw_lro_agg_size_cnt[0][i] +
			   hw_lro_agg_size_cnt[1][i] +
			   hw_lro_agg_size_cnt[2][i]);
	}

	seq_puts(seq, "Flush reason:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "AGG timeout:      %d      %d      %d      %d\n",
		   hw_lro_agg_flush_cnt[0], hw_lro_agg_flush_cnt[1],
		   hw_lro_agg_flush_cnt[2],
		   (hw_lro_agg_flush_cnt[0] + hw_lro_agg_flush_cnt[1] +
		    hw_lro_agg_flush_cnt[2])
	    );
	seq_printf(seq, "AGE timeout:      %d      %d      %d      %d\n",
		   hw_lro_age_flush_cnt[0], hw_lro_age_flush_cnt[1],
		   hw_lro_age_flush_cnt[2],
		   (hw_lro_age_flush_cnt[0] + hw_lro_age_flush_cnt[1] +
		    hw_lro_age_flush_cnt[2])
	    );
	seq_printf(seq, "Not in-sequence:  %d      %d      %d      %d\n",
		   hw_lro_seq_flush_cnt[0], hw_lro_seq_flush_cnt[1],
		   hw_lro_seq_flush_cnt[2],
		   (hw_lro_seq_flush_cnt[0] + hw_lro_seq_flush_cnt[1] +
		    hw_lro_seq_flush_cnt[2])
	    );
	seq_printf(seq, "Timestamp:        %d      %d      %d      %d\n",
		   hw_lro_timestamp_flush_cnt[0],
		   hw_lro_timestamp_flush_cnt[1],
		   hw_lro_timestamp_flush_cnt[2],
		   (hw_lro_timestamp_flush_cnt[0] +
		    hw_lro_timestamp_flush_cnt[1] +
		    hw_lro_timestamp_flush_cnt[2])
	    );
	seq_printf(seq, "No LRO rule:      %d      %d      %d      %d\n",
		   hw_lro_norule_flush_cnt[0],
		   hw_lro_norule_flush_cnt[1],
		   hw_lro_norule_flush_cnt[2],
		   (hw_lro_norule_flush_cnt[0] +
		    hw_lro_norule_flush_cnt[1] +
		    hw_lro_norule_flush_cnt[2])
	    );

	return 0;
}
#endif

static int hw_lro_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_lro_stats_read, NULL);
}

static const struct file_operations hw_lro_stats_fops = {
	.owner = THIS_MODULE,
	.open = hw_lro_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hw_lro_stats_write,
	.release = single_release
};

int hwlro_agg_cnt_ctrl(int par1, int par2)
{
	/* SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING1, par2) */

	int i;
	unsigned int reg_val1, reg_val2;
	//LRO_RX_RING1_CTRL_DW2 MAX_AGGREGATED_CNT_L bit26~bit 31
	//RO2_RING1_CTRL_DW3 MAX_AGGREGATED_CNT_H bit 0~ bit1

	pr_info("[hw_lro_auto_tlb_write] hwlro_agg_cnt_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_LRO_RING_NUM; i++) {

		reg_val1 = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));
		reg_val2 = mtk_r32(g_eth, MTK_LRO_CTRL_DW3_CFG(i));
		reg_val1 &= ~MTK_LRO_RING_AGG_CNT1;
		reg_val2 &= ~MTK_LRO_RING_AGG_CNT2;
		reg_val1 |= ((par2) & 0x3f) << MTK_LRO_RING_AGG_CNT1_OFFSET;
		reg_val2 |= (((par2) >> MTK_LRO_AGG_CNT_H_OFFSET) & 0x03) <<
			     MTK_LRO_RING_AGG_CNT2_OFFSET;
		mtk_w32(g_eth, reg_val1, MTK_LRO_CTRL_DW2_CFG(i));
		mtk_w32(g_eth, reg_val2, MTK_LRO_CTRL_DW3_CFG(i));

		reg_val1 = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));
		reg_val2 = mtk_r32(g_eth, MTK_LRO_CTRL_DW3_CFG(i));

		pr_info("[hw_lro_auto_tlb_write] agg_cnt %d = %x %x\n\r", i, reg_val1,reg_val2);
	}
	return 0;

}

int hwlro_agg_time_ctrl(int par1, int par2)
{
	/* SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING1, par2); */

	int i;
	unsigned int reg_val;

	pr_info("[hw_lro_auto_tlb_write] hwlro_agg_time_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_LRO_RING_NUM; i++) {
		reg_val = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));
		reg_val &= ~MTK_LRO_RING_AGG;
		reg_val |= (par2 & 0xffff) << MTK_LRO_RING_AGG_OFFSET;
		mtk_w32(g_eth, reg_val, MTK_LRO_CTRL_DW2_CFG(i));

		reg_val = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));

		pr_info("[hw_lro_auto_tlb_write] agg_time result %d = %x\n\r", i, reg_val);
	}

	return 0;
}

int hwlro_age_time_ctrl(int par1, int par2)
{
	/* SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING1, par2); */
	int i;
	unsigned int reg_val1, reg_val2;

	pr_info("[hw_lro_auto_tlb_write] hwlro_age_time_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_LRO_RING_NUM; i++) {
		reg_val1 = mtk_r32(g_eth, MTK_LRO_CTRL_DW1_CFG(i));
		reg_val2 = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));
		reg_val1 &= ~MTK_LRO_RING_AGE1;
		reg_val2 &= ~MTK_LRO_RING_AGE2;
		reg_val1 |= (par2 & 0x3ff) << MTK_LRO_RING_AGE1_OFFSET;
		reg_val2 |= ((par2 >> MTK_LRO_AGE_H_OFFSET) & 0x03f) << MTK_LRO_RING_AGE2_OFFSET;
		mtk_w32(g_eth, reg_val1, MTK_LRO_CTRL_DW1_CFG(i));
		mtk_w32(g_eth, reg_val2, MTK_LRO_CTRL_DW2_CFG(i));

		reg_val1 = mtk_r32(g_eth, MTK_LRO_CTRL_DW1_CFG(i));
		reg_val2 = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));

		pr_info("[hw_lro_auto_tlb_write] age %d = %x %x\n\r", i, reg_val1,reg_val2);
	}

	return 0;
}

int hwlro_threshold_ctrl(int par1, int par2)
{
	/* bandwidth threshold setting: SET_PDMA_LRO_BW_THRESHOLD(par2) */

	unsigned int reg_val = mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW2);

	pr_info("[hw_lro_auto_tlb_write] hwlro_threshold_ctrl par2 = %d\n\r", par2);

	reg_val = par2;
	mtk_w32(g_eth, reg_val, MTK_PDMA_LRO_CTRL_DW2);

	return 0;
}

int hwlro_ring_enable_ctrl(int par1, int par2)
{
	/* par2: 1 (enable), 0 (disable) */

	int i;
	unsigned int reg_val;

	pr_info("[hw_lro_auto_tlb_write] hwlro_ring_enable_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_LRO_RING_NUM; i++) {
		reg_val = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));
		reg_val &= ~(0x1 << MTK_RX_PORT_VALID_OFFSET);
		reg_val |= (par2 & 0x1) << MTK_RX_PORT_VALID_OFFSET;
		mtk_w32(g_eth, reg_val,MTK_LRO_CTRL_DW2_CFG(i));
	}

	return 0;
}

int hwlro_ring_mode_ctrl(int par1, int par2) {

	/* SET_PDMA_RXRING_MODE(par2) 0: normal, 1: PSE, 2: forced_port, 3: auto_learn */

	int i;
	unsigned int reg_val;


	pr_info("[hw_lro_auto_tlb_write] hwlro_ring_enable_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_LRO_RING_NUM; i++) {
		reg_val = mtk_r32(g_eth, MTK_LRO_CTRL_DW2_CFG(i));
		reg_val &= ~(0x3 << MTK_RX_MODE_OFFSET);
		reg_val |= (par2 & 0x3) << MTK_RX_MODE_OFFSET;
		mtk_w32(g_eth, reg_val, MTK_LRO_CTRL_DW2_CFG(i));
	}
	return 0;
}

int hwlro_tcp_src_port_ctrl(int par1, int par2) {

	/* SET_PDMA_RXRING_TCP_SRC_PORT(par2) */

	int i;
	unsigned int reg_val;

	pr_info("[hw_lro_auto_tlb_write] hwlro_tcp_src_port_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_LRO_RING_NUM; i++) {
		reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING1_STP_DTP_DW + ((i - 1) << 6));
		reg_val &= ~(0xffff << MTK_LRO_RX_TCP_SRC_PORT_OFFSET);
		reg_val |= (par2 & 0xffff) << MTK_LRO_RX_TCP_SRC_PORT_OFFSET;
		mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING1_STP_DTP_DW + ((i - 1) << 6));
	}
	return 0;
}


int hwlro_tcp_dst_port_ctrl(int par1, int par2) {

	/* SET_PDMA_RXRING_TCP_DEST_PORT(par2) */

	int i;
	unsigned int reg_val;

	pr_info("[hw_lro_auto_tlb_write] hwlro_tcp_dst_port_ctrl par2 = %d\n\r", par2);

	for (i = 1; i < MTK_MAX_RX_RING_NUM; i++) {
		reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING1_STP_DTP_DW + ((i - 1) << 6));
		reg_val &= ~(0xffff << MTK_LRO_RX_TCP_DEST_PORT_OFFSET);
		reg_val |= (par2 & 0xffff) << MTK_LRO_RX_TCP_DEST_PORT_OFFSET;
		mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING1_STP_DTP_DW + ((i - 1) << 6));
	}
	return 0;
}

int hwlro_myip_valid_ctrl(int par1, int par2) {

	/* MTK_RX_MYIP_VALID_OFFSET */
	int i;
	unsigned int reg_val;

	pr_info("[hw_lro_auto_tlb_write] hwlro_myip_valid_ctrl par2 = %d\n\r", par2);

	for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
		reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + (i << 6));
		reg_val &= ~(0x1 << MTK_RX_MYIP_VALID_OFFSET);
		reg_val |= (par2 & 0x1) << MTK_RX_MYIP_VALID_OFFSET;
		mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING0_CTRL_DW2 + (i << 6));
	}
	return 0;
}


int hwlro_force_aggre_dvt(int par1, int par2)
{
	unsigned int regVal = 0;
	unsigned int ip;
	unsigned int reg_val;

	pr_info("pdma_lro_force_aggre_dvt()\n");

/* pdma rx ring1 */
	/* 1. Set RX ring mode to force port */
	//SET_PDMA_RXRING_MODE(ADMA_RX_RING1, PDMA_RX_FORCE_PORT);
	reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6));
	reg_val &= ~(0x3 << MTK_RX_MODE_OFFSET);
	reg_val |= (2) << MTK_RX_MODE_OFFSET;
	mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6));


	/* 2. Configure lro ring */
	/* 2.1 set src/destination TCP ports */
	//SET_PDMA_RXRING_TCP_SRC_PORT(ADMA_RX_RING1, 3423);
	reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING1_STP_DTP_DW +
				    (((1) - 1) << 6));
	reg_val &= ~(0xffff << MTK_LRO_RX_TCP_SRC_PORT_OFFSET);
	reg_val |= (3423) << MTK_LRO_RX_TCP_SRC_PORT_OFFSET;
	mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING1_STP_DTP_DW + (((1) - 1) << 6));

	//SET_PDMA_RXRING_TCP_DEST_PORT(ADMA_RX_RING1, 2301);
	reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING1_STP_DTP_DW +
				    (((1) - 1) << 6));
	reg_val &= ~(0xffff << MTK_LRO_RX_TCP_DEST_PORT_OFFSET);
	reg_val |= (2301) << MTK_LRO_RX_TCP_DEST_PORT_OFFSET;
	mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING1_STP_DTP_DW + (((1) - 1) << 6));

	/* 2.2 set src/destination IPs */
	ip = 0x0a0a0a03;
	//str_to_ip(&ip, "10.10.10.3");
	mtk_w32(g_eth, ip, MTK_LRO_RX_RING1_SIP_DW0);
	mtk_w32(g_eth, 0, MTK_LRO_RX_RING1_SIP_DW1);
	mtk_w32(g_eth, 0, MTK_LRO_RX_RING1_SIP_DW2);
	mtk_w32(g_eth, 0, MTK_LRO_RX_RING1_SIP_DW3);
	//str_to_ip(&ip, "10.10.10.100");
	ip = 0x0a0a0a64;
	mtk_w32(g_eth, ip, MTK_LRO_RX_RING1_DIP_DW0);
	mtk_w32(g_eth, 0, MTK_LRO_RX_RING1_DIP_DW1);
	mtk_w32(g_eth, 0, MTK_LRO_RX_RING1_DIP_DW2);
	mtk_w32(g_eth, 0, MTK_LRO_RX_RING1_DIP_DW3);

	//SET_PDMA_RXRING_MYIP_VALID(ADMA_RX_RING1, 1);
	reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6));
	reg_val &= ~(0x1 << MTK_RX_MYIP_VALID_OFFSET);
	reg_val |= ((1) & 0x1) << MTK_RX_MYIP_VALID_OFFSET;
	mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6));

	/* 2.3 Valid LRO ring */
	//SET_PDMA_RXRING_VALID(ADMA_RX_RING1, 1);
	reg_val = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6));
	reg_val &= ~(0x1 << MTK_RX_PORT_VALID_OFFSET);
	reg_val |= ((1) & 0x1) << MTK_RX_PORT_VALID_OFFSET;
	mtk_w32(g_eth, reg_val, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6));

	/* 2.4 Set AGE timer */
	//SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING1, 0);
	hwlro_age_time_ctrl(0, 0);

	/* 2.5 Set max AGG timer */
	//SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING1, 0);
	hwlro_agg_time_ctrl(0, 0);

	/* 2.6 Set max LRO agg count */
	//SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING1, HW_LRO_MAX_AGG_CNT);
	hwlro_agg_cnt_ctrl(0, 64);

	/* 3. IPv4 checksum update enable */
	//SET_PDMA_LRO_IPV4_CSUM_UPDATE_EN(1);
	reg_val = mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW0);
	reg_val &= ~PDMA_LRO_IPV4_CSUM_UPDATE_EN;
	reg_val |= ((1) & 0x1) << PDMA_LRO_IPV4_CSUM_UPDATE_EN_OFFSET;
	mtk_w32(g_eth, reg_val, MTK_PDMA_LRO_CTRL_DW0);

	/* 4. Polling relinguish */
	while (mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW0) & PDMA_LRO_RELINGUISH) {;
	}

	/* 5. Enable LRO */
	regVal = mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW0);
	regVal |= PDMA_LRO_EN;
	mtk_w32(g_eth, regVal, MTK_PDMA_LRO_CTRL_DW0);

	// print

	pr_info("pdma_lro_force_aggre_dvt(), 2.4|, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING0_CTRL_DW1 + ((1) << 6),
		mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW1 + ((1) << 6)));
	pr_info("pdma_lro_force_aggre_dvt(), 1|2.2|2.3|2.4|2.5|2.6, 0x%08x=0x%08x\n",
		MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6),
		mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + ((1) << 6)));
	pr_info("pdma_lro_force_aggre_dvt(), 2.6, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING0_CTRL_DW3 + ((1) << 6),
		mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW3 + ((1) << 6)));

	pr_info("pdma_lro_force_aggre_dvt(), 2.1, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_STP_DTP_DW + (((1) - 1) << 6),
		mtk_r32(g_eth, MTK_LRO_RX_RING1_STP_DTP_DW + (((1) - 1) << 6)));

	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_SIP_DW0,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_SIP_DW0));

	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_SIP_DW1,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_SIP_DW1));
	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_SIP_DW2,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_SIP_DW2));

	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_SIP_DW3,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_SIP_DW3));
	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_DIP_DW0,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_DIP_DW0));
	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_DIP_DW1,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_DIP_DW1));
	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_DIP_DW2,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_DIP_DW2));
	pr_info("pdma_lro_force_aggre_dvt(), 2.2, 0x%08x=0x%x\n",
		MTK_LRO_RX_RING1_DIP_DW3,
		mtk_r32(g_eth, MTK_LRO_RX_RING1_DIP_DW3));


	pr_info("pdma_lro_force_aggre_dvt(), 3|4|5, 0x%08x=0x%x\n",
		MTK_PDMA_LRO_CTRL_DW0,
		mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW0));

	pr_info("pdma_lro_force_aggre_dvt() end\n");
	return 0;
}


static const HWLRO_DBG_FUNC hw_lro_dbg_func[] = {
	[0] = hwlro_agg_cnt_ctrl,
	[1] = hwlro_agg_time_ctrl,
	[2] = hwlro_age_time_ctrl,
	[3] = hwlro_threshold_ctrl,
	[4] = hwlro_ring_enable_ctrl,
	[5] = hwlro_ring_mode_ctrl,
	[6] = hwlro_tcp_src_port_ctrl,
	[7] = hwlro_tcp_dst_port_ctrl,
	[8] = hwlro_myip_valid_ctrl,
	[9] = hwlro_force_aggre_dvt,
};

ssize_t hw_lro_auto_tlb_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	u32 len = count;
	long x = 0, y = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	pr_info("[hw_lro_auto_tlb_write]write parameter len = %d\n\r",
		(int)len);
	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("[hw_lro_auto_tlb_write]write parameter data = %s\n\r", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		x = 0;
	else
		ret = kstrtol(p_token, 10, &x);

	p_token = strsep(&p_buf, "\t\n ");
	if (p_token) {
		ret = kstrtol(p_token, 10, &y);
		pr_info("y = %ld\n\r", y);
	}

	if (hw_lro_dbg_func[x] &&
	    (ARRAY_SIZE(hw_lro_dbg_func) > x)) {
		(*hw_lro_dbg_func[x]) (x, y);
	}

	return count;
}

void hw_lro_auto_tlb_dump(struct seq_file *seq, unsigned int index)
{
	int i;
	struct PDMA_LRO_AUTO_TLB_INFO pdma_lro_auto_tlb;
	unsigned int tlb_info[9];
	unsigned int dw_len, cnt, priority;
	unsigned int entry;

	if (index > 4)
		index = index - 1;
	entry = (index * 9) + 1;

	/* read valid entries of the auto-learn table */
	mtk_w32(g_eth, entry, MTK_FE_ALT_CF8);

	/* seq_printf(seq, "\nEntry = %d\n", entry); */
	for (i = 0; i < 9; i++) {
		tlb_info[i] = mtk_r32(g_eth, MTK_FE_ALT_SEQ_CFC);
		seq_printf(seq, "tlb_info[%d] = 0x%x\n", i, tlb_info[i]);
	}
	memcpy(&pdma_lro_auto_tlb, tlb_info,
	       sizeof(struct PDMA_LRO_AUTO_TLB_INFO));

	dw_len = pdma_lro_auto_tlb.auto_tlb_info7.DW_LEN;
	cnt = pdma_lro_auto_tlb.auto_tlb_info6.CNT;

	if (mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW0) & MTK_LRO_ALT_SCORE_MODE)
		priority = cnt;		/* packet count */
	else
		priority = dw_len;	/* byte count */

	/* dump valid entries of the auto-learn table */
	if (index >= 4)
		seq_printf(seq, "\n===== TABLE Entry: %d (Act) =====\n", index);
	else
		seq_printf(seq, "\n===== TABLE Entry: %d (LRU) =====\n", index);
	if (pdma_lro_auto_tlb.auto_tlb_info8.IPV4) {
		seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv4)\n",
			   pdma_lro_auto_tlb.auto_tlb_info4.SIP3,
			   pdma_lro_auto_tlb.auto_tlb_info3.SIP2,
			   pdma_lro_auto_tlb.auto_tlb_info2.SIP1,
			   pdma_lro_auto_tlb.auto_tlb_info1.SIP0);
	} else {
		seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",
			   pdma_lro_auto_tlb.auto_tlb_info4.SIP3,
			   pdma_lro_auto_tlb.auto_tlb_info3.SIP2,
			   pdma_lro_auto_tlb.auto_tlb_info2.SIP1,
			   pdma_lro_auto_tlb.auto_tlb_info1.SIP0);
	}
	seq_printf(seq, "DIP_ID = %d\n",
		   pdma_lro_auto_tlb.auto_tlb_info8.DIP_ID);
	seq_printf(seq, "TCP SPORT = %d | TCP DPORT = %d\n",
		   pdma_lro_auto_tlb.auto_tlb_info0.STP,
		   pdma_lro_auto_tlb.auto_tlb_info0.DTP);
	seq_printf(seq, "VLAN_VID_VLD = %d\n",
		   pdma_lro_auto_tlb.auto_tlb_info6.VLAN_VID_VLD);
	seq_printf(seq, "VLAN1 = %d | VLAN2 = %d | VLAN3 = %d | VLAN4 =%d\n",
		   (pdma_lro_auto_tlb.auto_tlb_info5.VLAN_VID0 & 0xfff),
		   ((pdma_lro_auto_tlb.auto_tlb_info5.VLAN_VID0 >> 12) & 0xfff),
		   ((pdma_lro_auto_tlb.auto_tlb_info6.VLAN_VID1 << 8) |
		   ((pdma_lro_auto_tlb.auto_tlb_info5.VLAN_VID0 >> 24)
		     & 0xfff)),
		   ((pdma_lro_auto_tlb.auto_tlb_info6.VLAN_VID1 >> 4) & 0xfff));
	seq_printf(seq, "TPUT = %d | FREQ = %d\n", dw_len, cnt);
	seq_printf(seq, "PRIORITY = %d\n", priority);
}

#if defined(CONFIG_MTK_ADMAV2)


void hw_lro_auto_tlb_dump_v2(struct seq_file *seq, unsigned int index)
{
	int i;
	struct PDMA_LRO_AUTO_TLB_INFO_V2 pdma_lro_auto_tlb;
	unsigned int tlb_info[12];
	unsigned int entry;
	unsigned int score = 0;
	unsigned int ipv4 = 0;
	unsigned int ipv6[4] = {0};

	/* read valid entries of the auto-learn table */
	entry = index << 8;
	mtk_w32(g_eth, entry, PDMA_LRO2_ALT_DBG);

	/* seq_printf(seq, "\nEntry = %d\n", entry); */
	for (i = 0; i < 11; i++) {
		tlb_info[i] = mtk_r32(g_eth, PDMA_LRO2_ALT_DATA);
	}
	memcpy(&pdma_lro_auto_tlb, tlb_info,
	       sizeof(struct PDMA_LRO_AUTO_TLB_INFO_V2));


	if (mtk_r32(g_eth,MTK_PDMA_LRO_CTRL_DW0) & MTK_LRO_ALT_SCORE_MODE)
		score = 1;		/* packet count */
	else
		score = 0;	/* byte count */

	seq_printf(seq, "index = %d	\n", index);
	seq_printf(seq, "score mode = %d	(1:pkt 0:byte)\n", score);
	seq_printf(seq, "table = 0x%x: 0x%x: 0x%x: 0x%x: 0x%x: 0x%x: 0x%x: 0x%x: 0x%x: 0x%x:  0x%x:\n", tlb_info[0],tlb_info[1],tlb_info[2],tlb_info[3],tlb_info[4],tlb_info[5],tlb_info[6],tlb_info[7],tlb_info[8],tlb_info[9],tlb_info[10]);

	/* dump valid entries of the auto-learn table */
	if (pdma_lro_auto_tlb.auto_tlb_info0.VALID) {
		if (index < 5)
			seq_printf(seq, "\n===== TABLE Entry: %d (onging) =====\n", index);
		else
			seq_printf(seq, "\n===== TABLE Entry: %d (candidate) =====\n", index);


		seq_printf(seq, "v0_vld:%d  v1_vld:%d  v2_vld:%d  v3_vld:%d  \n",
			   pdma_lro_auto_tlb.auto_tlb_info0.V0_VALID,pdma_lro_auto_tlb.auto_tlb_info0.V1_VALID,pdma_lro_auto_tlb.auto_tlb_info0.V2_VALID,pdma_lro_auto_tlb.auto_tlb_info0.V3_VALID);
		seq_printf(seq, "VLAN0 = %d  VLAN1 = %d  VLAN2 = %d  VLAN3 =%d\n",
			   (pdma_lro_auto_tlb.auto_tlb_info0.V0_ID),
			   (pdma_lro_auto_tlb.auto_tlb_info0.V1_ID),
			   ((pdma_lro_auto_tlb.auto_tlb_info0.V2_ID_H << 9) | (pdma_lro_auto_tlb.auto_tlb_info1.V2_ID_L)),
			   (pdma_lro_auto_tlb.auto_tlb_info1.V3_ID));



		if (pdma_lro_auto_tlb.auto_tlb_info1.V4_VALID) {

			seq_printf(seq, "SIP0_H = 0x%x: SIP0_L:0x%x\n", pdma_lro_auto_tlb.auto_tlb_info4.SIP0_H,pdma_lro_auto_tlb.auto_tlb_info5.SIP0_L);
			ipv4 = (pdma_lro_auto_tlb.auto_tlb_info4.SIP0_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info5.SIP0_L);
			seq_printf(seq, "SIP = 0x%x: (IPv4)\n", ipv4);

			ipv4 = (pdma_lro_auto_tlb.auto_tlb_info8.DIP0_H << 23) | pdma_lro_auto_tlb.auto_tlb_info9.DIP0_L;
			seq_printf(seq, "DIP = 0x%x: (IPv4)\n", ipv4);
		}

		if (pdma_lro_auto_tlb.auto_tlb_info1.V6_VALID) {
			ipv6[3] = (pdma_lro_auto_tlb.auto_tlb_info1.SIP3_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info2.SIP3_L << 9);
			ipv6[2] = (pdma_lro_auto_tlb.auto_tlb_info2.SIP2_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info3.SIP2_L << 9);
			ipv6[1] = (pdma_lro_auto_tlb.auto_tlb_info3.SIP1_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info4.SIP1_L << 9);
			ipv6[0] = (pdma_lro_auto_tlb.auto_tlb_info4.SIP0_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info5.SIP0_L << 9);
			seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",ipv6[3],ipv6[2],ipv6[1],ipv6[0]);

			ipv6[3] = (pdma_lro_auto_tlb.auto_tlb_info5.DIP3_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info6.DIP3_L << 9);
			ipv6[2] = (pdma_lro_auto_tlb.auto_tlb_info6.DIP2_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info7.DIP2_L << 9);
			ipv6[1] = (pdma_lro_auto_tlb.auto_tlb_info7.DIP1_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info8.DIP1_L << 9);
			ipv6[0] = (pdma_lro_auto_tlb.auto_tlb_info8.DIP0_H << 23) | (pdma_lro_auto_tlb.auto_tlb_info9.DIP0_L << 9);
			seq_printf(seq, "DIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",ipv6[3],ipv6[2],ipv6[1],ipv6[0]);
		}

		seq_printf(seq, "TCP SPORT = %d | TCP DPORT = %d\n",
			 ((pdma_lro_auto_tlb.auto_tlb_info9.SP_H << 7) | (pdma_lro_auto_tlb.auto_tlb_info10.SP_L)),
			   pdma_lro_auto_tlb.auto_tlb_info10.DP);
	}

}

#endif


int hw_lro_auto_tlb_read(struct seq_file *seq, void *v)
{
	int i;
	unsigned int reg_val;
	unsigned int reg_op1, reg_op2, reg_op3, reg_op4;
	unsigned int agg_cnt, agg_time, age_time;

	seq_puts(seq, "Usage of /proc/mtketh/hw_lro_auto_tlb:\n");
	seq_puts(seq, "echo [function] [setting] > /proc/mtketh/hw_lro_auto_tlb\n");
	seq_puts(seq, "Functions:\n");
	seq_puts(seq, "[0] = hwlro_agg_cnt_ctrl\n");
	seq_puts(seq, "[1] = hwlro_agg_time_ctrl\n");
	seq_puts(seq, "[2] = hwlro_age_time_ctrl\n");
	seq_puts(seq, "[3] = hwlro_threshold_ctrl\n");
	seq_puts(seq, "[4] = hwlro_ring_enable_ctrl\n\n");
	seq_puts(seq, "[5] = hwlro_ring_mode_ctrl\n\n");
	seq_puts(seq, "[6] = hwlro_tcp_src_port_ctrl\n\n");
	seq_puts(seq, "[7] = hwlro_tcp_dst_port_ctrl\n\n");

	/* Read valid entries of the auto-learn table */
	mtk_w32(g_eth, 0, MTK_FE_ALT_CF8);
	reg_val = mtk_r32(g_eth, MTK_FE_ALT_SEQ_CFC);

	seq_printf(seq,
		   "HW LRO Auto-learn Table: (MTK_FE_ALT_SEQ_CFC=0x%x)\n",
		   reg_val);

	for (i = 7; i >= 0; i--) {
		if (reg_val & (1 << i))
			hw_lro_auto_tlb_dump(seq, i);
	}

	/* Read the agg_time/age_time/agg_cnt of LRO rings */
	seq_puts(seq, "\nHW LRO Ring Settings\n");
	for (i = 1; i <= 3; i++) {
		reg_op1 = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW1+ (i * 0x40));
		reg_op2 = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + (i * 0x40));
		reg_op3 = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW3 + (i * 0x40));
		reg_op4 = mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW2);
		agg_cnt =
		    ((reg_op3 & 0x03) << MTK_LRO_AGG_CNT_H_OFFSET) |
		    ((reg_op2 >> MTK_LRO_RING_AGG_CNT1_OFFSET) & 0x3f);
		agg_time = (reg_op2 >> MTK_LRO_RING_AGG_OFFSET) & 0xffff;
		age_time =
		    ((reg_op2 & 0x03f) << MTK_LRO_AGE_H_OFFSET) |
		    ((reg_op1 >> MTK_LRO_RING_AGE1_OFFSET) & 0x3ff);
		seq_printf(seq,
			   "Ring[%d]: MAX_AGG_CNT=%d, AGG_TIME=%d, AGE_TIME=%d, Threshold=%d\n",
			   i, agg_cnt, agg_time, age_time, reg_op4);
	}
	seq_puts(seq, "\n");

	return 0;
}

#if defined(CONFIG_MTK_ADMAV2)

int hw_lro_auto_tlb_read_v2(struct seq_file *seq, void *v)
{
	int i;
	unsigned int reg_val;
	unsigned int reg_op1, reg_op2, reg_op3, reg_op4;
	unsigned int agg_cnt, agg_time, age_time;

	seq_puts(seq, "Usage of /proc/mtketh/hw_lro_auto_tlb:\n");
	seq_puts(seq, "echo [function] [setting] > /proc/mtketh/hw_lro_auto_tlb\n");
	seq_puts(seq, "Functions:\n");
	seq_puts(seq, "[0] = hwlro_agg_cnt_ctrl\n");
	seq_puts(seq, "[1] = hwlro_agg_time_ctrl\n");
	seq_puts(seq, "[2] = hwlro_age_time_ctrl\n");
	seq_puts(seq, "[3] = hwlro_threshold_ctrl\n");
	seq_puts(seq, "[4] = hwlro_ring_enable_ctrl\n\n");
	seq_puts(seq, "[5] = hwlro_ring_mode_ctrl\n\n");
	seq_puts(seq, "[6] = hwlro_tcp_src_port_ctrl\n\n");
	seq_puts(seq, "[7] = hwlro_tcp_dst_port_ctrl\n\n");

	/* Read valid entries of the auto-learn table */
	mtk_w32(g_eth, 0, MTK_FE_ALT_CF8);
	reg_val = mtk_r32(g_eth, MTK_FE_ALT_SEQ_CFC);

	seq_printf(seq,
		   "HW LRO Auto-learn Table: (MTK_FE_ALT_SEQ_CFC=0x%x)\n",
		   reg_val);
	for (i = 1; i <= 8; i++) {
		hw_lro_auto_tlb_dump_v2(seq, i);
	}

	/* Read the agg_time/age_time/agg_cnt of LRO rings */
	seq_puts(seq, "\nHW LRO Ring Settings\n");
	for (i = 0; i <= 3; i++) {
		reg_op1 = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW1+ (i * 0x40));
		reg_op2 = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW2 + (i * 0x40));
		reg_op3 = mtk_r32(g_eth, MTK_LRO_RX_RING0_CTRL_DW3 + (i * 0x40));
		reg_op4 = mtk_r32(g_eth, MTK_PDMA_LRO_CTRL_DW2);
		agg_cnt =
		    ((reg_op3 & 0x03) << MTK_LRO_AGG_CNT_H_OFFSET) |
		    ((reg_op2 >> MTK_LRO_RING_AGG_CNT1_OFFSET) & 0x3f);
		agg_time = (reg_op2 >> MTK_LRO_RING_AGG_OFFSET) & 0xffff;
		age_time =
		    ((reg_op2 & 0x03f) << MTK_LRO_AGE_H_OFFSET) |
		    ((reg_op1 >> MTK_LRO_RING_AGE1_OFFSET) & 0x3ff);
		seq_printf(seq,
			   "Ring[%d]: MAX_AGG_CNT=%d, AGG_TIME=%d, AGE_TIME=%d, Threshold=%d\n",
			   i+4, agg_cnt, agg_time, age_time, reg_op4);
	}
	seq_puts(seq, "\n");

	return 0;
}


#endif

static int hw_lro_auto_tlb_open(struct inode *inode, struct file *file)
{

#if defined(CONFIG_MTK_ADMAV2)
	return single_open(file, hw_lro_auto_tlb_read_v2, NULL);
#else
	return single_open(file, hw_lro_auto_tlb_read, NULL);
#endif

}

static const struct file_operations hw_lro_auto_tlb_fops = {
	.owner = THIS_MODULE,
	.open = hw_lro_auto_tlb_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hw_lro_auto_tlb_write,
	.release = single_release
};

int hwlro_debug_proc_init(struct proc_dir_entry *proc_reg_dir, struct mtk_eth *eth)
{
	g_eth = eth;

	proc_rx_ring1 =
	     proc_create(PROCREG_RXRING1, 0400, proc_reg_dir, &rx_ring1_fops);
	if (!proc_rx_ring1)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING1);

	proc_rx_ring2 =
	     proc_create(PROCREG_RXRING2, 0400, proc_reg_dir, &rx_ring2_fops);
	if (!proc_rx_ring2)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING2);

	proc_rx_ring3 =
	     proc_create(PROCREG_RXRING3, 0400, proc_reg_dir, &rx_ring3_fops);
	if (!proc_rx_ring3)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING3);

	proc_hw_lro_stats =
	     proc_create(PROCREG_HW_LRO_STATS, 0400, proc_reg_dir,
			 &hw_lro_stats_fops);
	if (!proc_hw_lro_stats)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_HW_LRO_STATS);

	proc_hw_lro_auto_tlb =
	     proc_create(PROCREG_HW_LRO_AUTO_TLB, 0400, proc_reg_dir,
			 &hw_lro_auto_tlb_fops);
	if (!proc_hw_lro_auto_tlb)
		pr_info("!! FAIL to create %s PROC !!\n",
			PROCREG_HW_LRO_AUTO_TLB);

	return 0;
}

#if defined(CONFIG_MTK_ADMAV2)
int hwlrov2_debug_proc_init(struct proc_dir_entry *proc_reg_dir, struct mtk_eth *eth)
{

	g_eth = eth;

	proc_rx_ring4 =
	     proc_create(PROCREG_RXRING4, 0400, proc_reg_dir, &rx_ring4_fops);
	if (!proc_rx_ring4)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING4);

	proc_rx_ring5 =
	     proc_create(PROCREG_RXRING5, 0400, proc_reg_dir, &rx_ring5_fops);
	if (!proc_rx_ring5)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING5);

	proc_rx_ring6 =
	     proc_create(PROCREG_RXRING6, 0400, proc_reg_dir, &rx_ring6_fops);
	if (!proc_rx_ring6)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING6);

	proc_rx_ring7 =
	     proc_create(PROCREG_RXRING7, 0400, proc_reg_dir, &rx_ring7_fops);
	if (!proc_rx_ring7)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING7);

	proc_hw_lro_stats =
	     proc_create(PROCREG_HW_LRO_STATS, 0400, proc_reg_dir,
			 &hw_lro_stats_fops);
	if (!proc_hw_lro_stats)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_HW_LRO_STATS);

	proc_hw_lro_auto_tlb =
	     proc_create(PROCREG_HW_LRO_AUTO_TLB, 0400, proc_reg_dir,
			 &hw_lro_auto_tlb_fops);
	if (!proc_hw_lro_auto_tlb)
		pr_info("!! FAIL to create %s PROC !!\n",
			PROCREG_HW_LRO_AUTO_TLB);

	return 0;
}

#endif

void hwlro_debug_proc_exit(struct proc_dir_entry *proc_reg_dir)
{
	if (proc_rx_ring1)
		remove_proc_entry(PROCREG_RXRING1, proc_reg_dir);
	if (proc_rx_ring2)
		remove_proc_entry(PROCREG_RXRING2, proc_reg_dir);
	if (proc_rx_ring3)
		remove_proc_entry(PROCREG_RXRING3, proc_reg_dir);

#if defined(CONFIG_MTK_ADMAV2)

	if (proc_rx_ring4)
		remove_proc_entry(PROCREG_RXRING4, proc_reg_dir);
	if (proc_rx_ring5)
		remove_proc_entry(PROCREG_RXRING5, proc_reg_dir);
	if (proc_rx_ring6)
		remove_proc_entry(PROCREG_RXRING6, proc_reg_dir);
	if (proc_rx_ring7)
		remove_proc_entry(PROCREG_RXRING7, proc_reg_dir);
#endif


	if (proc_hw_lro_stats)
		remove_proc_entry(PROCREG_HW_LRO_STATS, proc_reg_dir);
	if (proc_hw_lro_auto_tlb)
		remove_proc_entry(PROCREG_HW_LRO_AUTO_TLB, proc_reg_dir);
}

