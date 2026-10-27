/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MSDC_TYPE_H
#define MSDC_TYPE_H

#include "mtk_sd.h"

struct msdc_dma {
	struct scatterlist *sg;	/* I/O scatter list */
	struct mt_gpdma_desc *gpd;		/* pointer to gpd array */
	struct mt_bdma_desc *bd;		/* pointer to bd array */
	dma_addr_t gpd_addr;	/* the physical address of gpd array */
	dma_addr_t bd_addr;	/* the physical address of bd array */
};

struct msdc_save_para {
	u32 msdc_cfg;
	u32 iocon;
	u32 sdc_cfg;
	u32 pad_tune;
	u32 patch_bit0;
	u32 patch_bit1;
	u32 patch_bit2;
	u32 pad_ds_tune;
	u32 pad_cmd_tune;
	u32 emmc50_cfg0;
	u32 emmc50_cfg3;
	u32 sdc_fifo_cfg;
	u32 emmc_top_control;
	u32 emmc_top_cmd;
	u32 emmc50_pad_ds_tune;
};

struct mtk_mmc_compatible {
	u8 clk_div_bits;
	bool hs400_tune; /* only used for MT8173 */
	u32 pad_tune_reg;
	bool async_fifo;
	bool data_tune;
	bool busy_check;
	bool stop_clk_fix;
	bool enhance_rx;
	bool support_64g;
	bool use_internal_cd;
};

struct msdc_tune_para {
	u32 iocon;
	u32 pad_tune;
	u32 pad_cmd_tune;
	u32 emmc_top_control;
	u32 emmc_top_cmd;
};

struct msdc_saved_para {
	u32 pad_tune0;
	u32 pad_tune1;
	u8 suspend_flag;
	u32 msdc_cfg;
	u32 sdc_cfg;
	u32 iocon;
	u8 timing;
	u32 hz;
	u8 inten_sdio_irq;
	u8 ds_dly1;
	u8 ds_dly3;
	u32 emmc50_pad_cmd_tune;
	u32 emmc50_dat01;
	u32 emmc50_dat23;
	u32 emmc50_dat45;
	u32 emmc50_dat67;
	u32 emmc50_cfg0;
	u32 pb0;
	u32 pb1;
	u32 pb2;
	u32 sdc_fifo_cfg;
	u32 sdc_adv_cfg0;

	/* msdc top reg  */
	u32 emmc_top_control;
	u32 emmc_top_cmd;
	u32 top_emmc50_pad_ctl0;
	u32 top_emmc50_pad_ds_tune;
	u32 top_emmc50_pad_dat_tune[8];
};

struct msdc_host {
	struct device *dev;
	const struct mtk_mmc_compatible *dev_comp;
	struct mmc_host *mmc;	/* mmc structure */
	int cmd_rsp;

	spinlock_t lock;

	/* prevent dbg_host_cnt competition between msdc_irq and msdc_ops_request*/
	spinlock_t log_lock;

	struct mmc_request *mrq;
	struct mmc_command *cmd;
	struct mmc_data *data;
	int error;

	void __iomem *base;		/* host base address */
	void __iomem *top_base;		/* host top register base address */
	int	id;

	struct msdc_dma dma;	/* dma channel */
	u64 dma_mask;

	u32 timeout_ns;		/* data timeout ns */
	u32 timeout_clks;	/* data timeout clks */

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_uhs;
	struct delayed_work req_timeout;
	int irq;		/* host interrupt */
	struct reset_control *reset;

	struct clk *p_clk;		/* msdc power clock */
	struct clk *axi_clk;	/* msdc axi clock*/
	struct clk *ahb_clk;	/* msdc ahb2axi_brg_clk clock*/
	struct clk *src_clk;	/* msdc source clock */
	struct clk *h_clk;      /* msdc h_clk */
	struct clk *bus_clk;	/* bus clock which used to access register */
	struct clk *src_clk_cg; /* msdc source clock control gate */

	u32 mclk;		/* mmc subsystem clock frequency */
	u32 src_clk_freq;	/* source clock frequency */
	unsigned char timing;
	bool vqmmc_enabled;
	struct gpio_desc *mmc_power_gpio;
	u32 latch_ck;
	u32 hs400_ds_delay;
	u32 hs200_cmd_int_delay; /* cmd internal delay for HS200/SDR104 */
	u32 hs400_cmd_int_delay; /* cmd internal delay for HS400 */
	bool hs400_cmd_resp_sel_rising;
				 /* cmd response sample selection for HS400 */
	bool hs400_mode;	/* current eMMC will run at hs400 mode */
	bool internal_cd;	/* Use internal card-detect logic */
	bool cqhci;		/* support eMMC hw cmdq */
	struct msdc_save_para save_para; /* used when gate HCLK */
	struct msdc_tune_para def_tune_para; /* default tune setting */
	struct msdc_tune_para saved_tune_para; /* tune result of CMD21/CMD19 */
	struct cqhci_host *cq_host;
	struct msdc_hw	*hw;
	u32		tune_latch_ck_cnt;
	struct msdc_saved_para saved_para;
	/************************ +1 for merge ****************************************/
	u8      autok_res[AUTOK_VCORE_NUM+1][TUNING_PARA_SCAN_COUNT];
	bool	tuning_in_progress;
	u32     need_tune;
	int		autok_error;
	bool	is_autok_done;
	u8		use_hw_dvfs;
	u32    	data_timeout_cont; /* data continuous timeout */
};

#endif /* end of  MSDC_TYPE_H */
