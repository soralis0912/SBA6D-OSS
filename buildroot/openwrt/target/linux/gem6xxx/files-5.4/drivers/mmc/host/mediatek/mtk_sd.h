/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MT_SD_H
#define MT_SD_H

#ifdef CONFIG_FPGA_EARLY_PORTING
#ifndef FPGA_PLATFORM
#define FPGA_PLATFORM
#endif
#else
/* #define MTK_MSDC_BRINGUP_DEBUG */
#define MSDC_AUTOK
#define BUILD_BYPASS 0
#define EMMC_RUNTIME_AUTOK_MERGE
#define SD_RUNTIME_AUTOK_MERGE
#endif

#include <linux/bitops.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/pm_qos.h>
#include <linux/reset.h>

#include "autok.h"
#include "autok_dvfs.h"
#include "msdc_reg.h"
#include "dbg.h"
#define TUNE_NONE		(0)        /* No need tune */
#define TUNE_CMD_ERR		(0x1 << 0) /* transfer cmd crc */
#define TUNE_DATA_WRITE		(0x1 << 1) /* transfer data crc */
#define TUNE_DATA_READ		(0x1 << 2) /* transfer data crc */

// These types are defined in both mtk-sd.c and msdc_type.h. They should be the same.
struct msdc_dma;
struct msdc_save_para;
struct mtk_mmc_compatible;
struct msdc_tune_para;
struct msdc_saved_para;
struct msdc_host;

struct msdc_hw_driving {
	unsigned char cmd_drv;  /* command pad driving */
	unsigned char dat_drv;  /* data pad driving */
	unsigned char clk_drv;  /* clock pad driving */
	unsigned char rst_drv;  /* RST-N pad driving */
	unsigned char ds_drv;   /* eMMC5.0 DS pad driving */
};

struct msdc_hw {
	unsigned char clk_src;  /* host clock source */
	unsigned char cmd_edge; /* command latch edge */
	unsigned char rdata_edge;       /* read data latch edge */
	unsigned char wdata_edge;       /* write data latch edge */
	struct msdc_hw_driving *driving_applied;
	struct msdc_hw_driving driving;
	struct msdc_hw_driving driving_sdr104;
	struct msdc_hw_driving driving_sdr50;
	struct msdc_hw_driving driving_ddr50;
	struct msdc_hw_driving driving_hs400;
	struct msdc_hw_driving driving_hs200;

	unsigned long flags;            /* hardware capability flags */

	unsigned char boot;             /* define boot host */
	unsigned char host_function;    /* define host function */
	unsigned char cd_level;         /* card detection level */
};

#define MSDC_EMMC               (0)
#define MSDC_SD                 (1)
#define MSDC_SDIO				(2)

/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/
#define REG_ADDR(x)             (base + OFFSET_##x)
#define REG_ADDR_TOP(x)         (top_base + OFFSET_##x)

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (64)    /* (128) */
#define MSDC_NUM                (4)

/* No memory stick mode, 0 use to gate clock */
#define MSDC_MS                 (0)
#define MSDC_SDMMC              (1)

#define MSDC_MODE_UNKNOWN       (0)
#define MSDC_MODE_PIO           (1)
#define MSDC_MODE_DMA_BASIC     (2)
#define MSDC_MODE_DMA_DESC      (3)

#define MSDC_BUS_1BITS          (0)
#define MSDC_BUS_4BITS          (1)
#define MSDC_BUS_8BITS          (2)

#define MSDC_BRUST_8B           (3)
#define MSDC_BRUST_16B          (4)
#define MSDC_BRUST_32B          (5)
#define MSDC_BRUST_64B          (6)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)
#define MSDC_AUTOCMD53          (0x0004)

enum {
	RESP_NONE = 0,
	RESP_R1,
	RESP_R2,
	RESP_R3,
	RESP_R4,
	RESP_R5,
	RESP_R6,
	RESP_R7,
	RESP_R1B
};

enum {
	OPER_TYPE_READ,
	OPER_TYPE_WRITE,
	OPER_TYPE_NUM
};

struct dma_addr {
	u32 start_address;
	u32 size;
	u8 end;
	struct dma_addr *next;
};

static inline unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}
#include <linux/io.h>

#define mt_reg_sync_writeb(v, a) \
	do {    \
		__raw_writeb((v), (void __force __iomem *)((a)));   \
		mb();  /*make sure register access in order */ \
	} while (0)

#define mt_reg_sync_writew(v, a) \
	do {    \
		__raw_writew((v), (void __force __iomem *)((a)));   \
		mb();  /*make sure register access in order */ \
	} while (0)

#define mt_reg_sync_writel(v, a) \
	do {    \
		__raw_writel((v), (void __force __iomem *)((a)));   \
		mb();  /*make sure register access in order */ \
	} while (0)

#define MSDC_READ8(reg)           __raw_readb(reg)
#define MSDC_READ16(reg)          __raw_readw(reg)
#define MSDC_READ32(reg)          __raw_readl(reg)
#define MSDC_WRITE8(reg, val)     mt_reg_sync_writeb(val, reg)
#define MSDC_WRITE16(reg, val)    mt_reg_sync_writew(val, reg)
#define MSDC_WRITE32(reg, val)    mt_reg_sync_writel(val, reg)

#define UNSTUFF_BITS(resp, start, size) \
({ \
	const int __size = size; \
	const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; \
	const int __off = 3 - ((start) / 32); \
	const int __shft = (start) & 31; \
	u32 __res; \
	__res = resp[__off] >> __shft; \
	if (__size + __shft > 32) \
		__res |= resp[__off-1] << ((32 - __shft) % 32); \
	__res & __mask; \
})

#define MSDC_SET_BIT32(reg, bs) \
	do { \
		unsigned int tv = MSDC_READ32(reg);\
		tv |= (u32)(bs); \
		MSDC_WRITE32(reg, tv); \
	} while (0)

#define MSDC_CLR_BIT32(reg, bs) \
	do { \
		unsigned int tv = MSDC_READ32(reg);\
		tv &= ~((u32)(bs)); \
		MSDC_WRITE32(reg, tv); \
	} while (0)

#define MSDC_SET_FIELD(reg, field, val) \
	do { \
		unsigned int tv = MSDC_READ32(reg); \
		tv &= ~(field); \
		tv |= ((val) << (uffs((unsigned int)field) - 1)); \
		MSDC_WRITE32(reg, tv); \
	} while (0)

#define MSDC_GET_FIELD(reg, field, val) \
	do { \
		unsigned int tv = MSDC_READ32(reg); \
		val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
	} while (0)

#define GET_FIELD(reg, field_shift, field_mask, val) \
	(val = (reg >> field_shift) & field_mask)

#define sdc_is_busy()           (MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()       (MSDC_READ32(SDC_STS) & SDC_STS_CMDBUSY)

int msdc_error_tuning(struct mmc_host *mmc,  struct mmc_request *mrq);
/* Function provided by msdc_tune.c */
void msdc_init_tune_setting(struct msdc_host *host);
void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios);
void msdc_init_tune_path(struct msdc_host *host, unsigned char timing);
void msdc_restore_timing_setting(struct msdc_host *host);
void msdc_save_timing_setting(struct msdc_host *host);

/* SD card, bad card handling settings */

/* if continuous data timeout reach the limit */
/* driver will force remove card */
#define MSDC_MAX_DATA_TIMEOUT_CONTINUOUS (100)

/* if continuous power cycle fail reach the limit */
/* driver will force remove card */
#define MSDC_MAX_POWER_CYCLE_FAIL_CONTINUOUS (3)

/**************************************************************/
/* Section 6: BBChip-depenent Tunnig Parameter                */
/**************************************************************/
#define EMMC_MAX_FREQ_DIV               4 /* lower frequence to 12.5M */
#define MSDC_CLKTXDLY                   0

#define MSDC0_DDR50_DDRCKD              1 /* FIX ME: may be removed */

#define VOL_CHG_CNT_DEFAULT_VAL         0x1F4 /* =500 */

#define MSDC_PB0_DEFAULT_VAL            0x403C0007
#define MSDC_PB1_DEFAULT_VAL            0xFFE64309

#define MSDC_PB2_DEFAULT_RESPWAITCNT    0x3
#define MSDC_PB2_DEFAULT_RESPSTENSEL    0x1
#define MSDC_PB2_DEFAULT_CRCSTSENSEL    0x1

#endif /* end of  MT_SD_H */
