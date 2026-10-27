//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "../nfi.h"
#include "nfiecc.h"
#include "nfi_regs.h"
#include "nfi_base.h"
#include "nfi_spi_regs.h"
#include "nfi_spi.h"

#define NFI_CMD_DUMMY_RD 0x00
#define NFI_CMD_DUMMY_WR 0x80

static struct nfi_spi_delay spi_delay[SPI_NAND_MAX_DELAY] = {
	/*
	 * tCLK_SAM_DLY, tCLK_OUT_DLY, tCS_DLY, tWR_EN_DLY,
	 * tIO_IN_DLY[4], tIO_OUT_DLY[4], tREAD_LATCH_LATENCY
	 */
	{0, 0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0},
	{23, 0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0},
	{47, 0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0},
	{0, 0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 1},
	{23, 0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 1},
	{47, 0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 1}
};

static inline struct nfi_spi *base_to_snfi(struct nfi_base *nb)
{
	return container_of(nb, struct nfi_spi, base);
}

static int spi_wait_done(struct nfi_base *nb, u32 timeout)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u64 now, end;
	u32 val;

	end = get_current_time_us() + timeout;

	do {
		val = readl(regs + SNF_STA_CTL1);
		val &= nfi_spi->snfi_status_mask;
		now = get_current_time_us();

		if (now > end)
			break;
	} while (!val);

	return !val ? -ETIMEDOUT : 0;
}

static void snfi_mac_enable(struct nfi_base *nb)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = readl(regs + SNF_MAC_CTL);
	if (nfi_spi->mac_qpi_mode)
		val |= MAC_XIO_SEL;
	else
		val &= ~MAC_XIO_SEL;
	val |= SF_MAC_EN;

	writel(val, regs + SNF_MAC_CTL);
}

static void snfi_mac_disable(struct nfi_base *nb)
{
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = readl(regs + SNF_MAC_CTL);
	val &= ~(SF_TRIG | SF_MAC_EN);
	writel(val, regs + SNF_MAC_CTL);
}

static int snfi_mac_trigger(struct nfi_base *nb)
{
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	val = readl(regs + SNF_MAC_CTL);
	val |= SF_TRIG;
	writel(val, regs + SNF_MAC_CTL);

	ret = readl_poll_timeout_atomic(regs + SNF_MAC_CTL, val,
					val & WIP_READY, 10,
					NFI_TIMEOUT);
	if (ret) {
		pr_err("polling wip ready for read timeout\n");
		return ret;
	}

	return readl_poll_timeout_atomic(regs + SNF_MAC_CTL, val,
					 !(val & WIP), 10,
					 NFI_TIMEOUT);
}

static int snfi_mac_op(struct nfi_base *nb)
{
	int ret;

	snfi_mac_enable(nb);
	ret = snfi_mac_trigger(nb);
	snfi_mac_disable(nb);

	return ret;
}

static void snfi_write_mac(struct nfi_spi *nfi_spi, u8 *data, int count)
{
	struct nandx_split32 split = {0};
	u32 reg_offset = round_down(nfi_spi->tx_count, 4);
	void *regs = nfi_spi->base.res.nfi_regs;
	u32 data_offset = 0, i, val;
	u8 *p_val = (u8 *)(&val);

	nandx_split(&split, nfi_spi->tx_count, count, val, 4);

	if (split.head_len) {
		val = readl(regs + SPI_GPRAM_ADDR + reg_offset);

		for (i = 0; i < split.head_len; i++)
			p_val[reminder(split.head, 4) + i] = data[i];

		writel(val, regs + SPI_GPRAM_ADDR + reg_offset);
	}

	if (split.body_len) {
		reg_offset = split.body;
		data_offset = split.head_len;

		for (i = 0; i < split.body_len; i++) {
			p_val[i & 3] = data[data_offset + i];

			if ((i & 3) == 3) {
				writel(val, regs + SPI_GPRAM_ADDR + reg_offset);
				reg_offset += 4;
			}
		}
	}

	if (split.tail_len) {
		reg_offset = split.tail;
		data_offset += split.body_len;

		for (i = 0; i < split.tail_len; i++) {
			p_val[i] = data[data_offset + i];

			if (i == split.tail_len - 1)
				writel(val, regs + SPI_GPRAM_ADDR + reg_offset);
		}
	}
}

static void snfi_read_mac(struct nfi_spi *nfi_spi, u8 *data, int count)
{
	void *regs = nfi_spi->base.res.nfi_regs;
	u32 reg_offset = round_down(nfi_spi->tx_count, 4);
	struct nandx_split32 split = {0};
	u32 data_offset = 0, i, val;
	u8 *p_val = (u8 *)&val;

	nandx_split(&split, nfi_spi->tx_count, count, val, 4);

	if (split.head_len) {
		val = readl(regs + SPI_GPRAM_ADDR + reg_offset);

		for (i = 0; i < split.head_len; i++)
			data[data_offset + i] = p_val[split.head + i];
	}

	if (split.body_len) {
		reg_offset = split.body;
		data_offset = split.head_len;

		for (i = 0; i < split.body_len; i++) {
			if ((i & 3) == 0) {
				val = readl(regs + SPI_GPRAM_ADDR + reg_offset);
				reg_offset += 4;
			}

			data[data_offset + i] = p_val[i % 4];
		}
	}

	if (split.tail_len) {
		reg_offset = split.tail;
		data_offset += split.body_len;
		val = readl(regs + SPI_GPRAM_ADDR + reg_offset);

		for (i = 0; i < split.tail_len; i++)
			data[data_offset + i] = p_val[i];
	}
}

static int snfi_auto_erase(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u32 val;
	int ret;

	nfi_spi->snfi_status_mask = AUTO_BLKER_DONE;

	val = readl(regs + SNF_ER_CTL);
	val &= ~ER_CMD_MASK;
	val |= nfi_spi->cmd[0] << ER_CMD_SHIFT;
	writel(val, regs + SNF_ER_CTL);

	val = nfi_spi->row_addr[0];
	writel(val, regs + SNF_ER_CTL2);

	val = readl(regs + SNF_ER_CTL);
	val |= AUTO_ER_TRIGGER;
	writel(val, regs + SNF_ER_CTL);

	ret = spi_wait_done(nb, NFI_TIMEOUT);
	if (ret)
		pr_warn("snfi wait done time out, status(0x%x) mask(0x%x)\n",
			readl(regs + SNF_STA_CTL1), nfi_spi->snfi_status_mask);

	nfi_spi->snfi_status_mask = 0;
	val = readl(regs + SNF_ER_CTL);
	val &= ~AUTO_ER_TRIGGER;
	writel(val, regs + SNF_ER_CTL);

	nfi_spi->tx_count = 0;
	nfi_spi->cur_cmd_idx = 0;
	nfi_spi->cur_addr_idx = 0;

	return ret;
}

static int snfi_send_command(struct nfi *nfi, short cmd)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);

	if (cmd == -1)
		return 0;

	if (nfi_spi->snfi_mode == SNFI_MAC_MODE) {
		snfi_write_mac(nfi_spi, (u8 *)&cmd, 1);
		nfi_spi->tx_count++;
		return 0;
	}

	nfi_spi->cmd[nfi_spi->cur_cmd_idx++] = cmd;

	/* for erase op */
	if ((cmd == 0xd8) && (nfi_spi->snfi_mode == SNFI_AUTO_MODE))
		nfi_spi->auto_erase = true;

	return 0;
}

static int snfi_send_address(struct nfi *nfi, int col, int row,
			     int col_cycle,
			     int row_cycle)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	u32 addr, cycle, temp;

	nb->col = col;
	nb->row = row;

	if (nfi_spi->snfi_mode == SNFI_MAC_MODE) {
		addr = row;
		cycle = row_cycle;

		if (!row_cycle) {
			addr = col;
			cycle = col_cycle;
		}

		/* for read, for col addr [15:0] + 8bits dummy */
		temp = nandx_cpu_to_be32(addr) >> ((4 - cycle) << 3);
		if (!row_cycle && (col_cycle == 3))
			temp = nandx_cpu_to_be32(addr) >>
			       ((4 - cycle + 1) << 3);
		snfi_write_mac(nfi_spi, (u8 *)&temp, cycle);
		nfi_spi->tx_count += cycle;
	}  else {
		nfi_spi->row_addr[nfi_spi->cur_addr_idx++] = row;
		nfi_spi->col_addr[nfi_spi->cur_addr_idx++] = col;
	}

	return 0;
}

static int snfi_trigger(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	int ret;

	if (nfi_spi->auto_erase) {
		nfi_spi->auto_erase = false;
		ret = snfi_auto_erase(nfi);
	} else {
		writel(nfi_spi->tx_count, regs + SNF_MAC_OUTL);
		writel(0, regs + SNF_MAC_INL);

		ret =  snfi_mac_op(nb);
	}

	if (!nfi_spi->cur_cmd_idx) {
		nfi_spi->tx_count = 0;
		nfi_spi->cur_cmd_idx = 0;
		nfi_spi->cur_addr_idx = 0;
	}

	return ret;
}

static int snfi_select_chip(struct nfi *nfi, int cs)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = readl(regs + SNF_MISC_CTL);

	if (cs == 0) {
		val &= ~SF2CS_SEL;
		val &= ~SF2CS_EN;
	} else if (cs == 1) {
		val |= SF2CS_SEL;
		val |= SF2CS_EN;
	} else
		return -EIO;

	writel(val, regs + SNF_MISC_CTL);

	return 0;
}

static int snfi_set_delay(struct nfi_base *nb, u8 delay_mode)
{
	void *regs = nb->res.nfi_regs;
	struct nfi_spi_delay *delay;
	u32 val;

	if (delay_mode < 0 || delay_mode >= SPI_NAND_MAX_DELAY)
		return -EINVAL;

	delay = &spi_delay[delay_mode];

	val = delay->tIO_OUT_DLY[0] | delay->tIO_OUT_DLY[1] << 8 |
	      delay->tIO_OUT_DLY[2] << 16 |
	      delay->tIO_OUT_DLY[3] << 24;
	writel(val, regs + SNF_DLY_CTL1);

	val = delay->tIO_IN_DLY[0] | (delay->tIO_IN_DLY[1] << 8) |
	      delay->tIO_IN_DLY[2] << 16 |
	      delay->tIO_IN_DLY[3] << 24;
	writel(val, regs + SNF_DLY_CTL2);

	val = delay->tCLK_SAM_DLY | delay->tCLK_OUT_DLY << 8 |
	      delay->tCS_DLY << 16 |
	      delay->tWR_EN_DLY << 24;
	writel(val, regs + SNF_DLY_CTL3);

	writel(delay->tCS_DLY, regs + SNF_DLY_CTL4);

	val = readl(regs + SNF_MISC_CTL);
	val |= (delay->tREAD_LATCH_LATENCY) <<
	       LATCH_LAT_SHIFT;
	writel(val, regs + SNF_MISC_CTL);

	return 0;
}

static int snfi_set_timing(struct nfi *nfi, void *timing, int type)
{
	/* Nothing need to do. */
	return 0;
}

static int snfi_wait_ready(struct nfi *nfi, int type, u32 timeout)
{
	/* Nothing need to do. */
	return 0;
}

static int snfi_ctrl(struct nfi *nfi, int cmd, void *args)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	int ret = 0;
	u32 val;

	switch (cmd) {
#if 0
	case NFI_CTRL_AUTO_READ_IRQ:
		nfi_spi->auto_read_irq = *(u8 *)args;
		break;

	case NFI_CTRL_AUTO_PROGRAM_IRQ:
		nfi_spi->auto_write_irq = *(u8 *)args;
		break;

	case NFI_CTRL_AUTO_ERASE_IRQ:
		nfi_spi->auto_erase_irq = *(u8 *)args;
		break;

	case NFI_CTRL_CUSTOM_READ_IRQ:
		nfi_spi->custom_read_irq = *(u8 *)args;
		break;

	case NFI_CTRL_CUSTOM_PROGRAM_IRQ:
		nfi_spi->custom_write_irq = *(u8 *)args;
		break;
#endif
	case SNFI_CTRL_BASE_INFO:
		pr_debug("snfi hw config as below:\n");
		pr_debug("snfi mode:%d\n", nfi_spi->snfi_mode);
		pr_debug("snfi read mode:%d\n", nfi_spi->read_cache_mode);
		pr_debug("snfi write mode:%d\n", nfi_spi->write_cache_mode);
		break;

	case SNFI_CTRL_OP_MODE:
		nfi_spi->snfi_mode = *(u8 *)args;
		break;

	case SNFI_CTRL_RX_MODE:
		nfi_spi->read_cache_mode = *(u8 *)args;
		break;

	case SNFI_CTRL_TX_MODE:
		nfi_spi->write_cache_mode = *(u8 *)args;
		break;

	case SNFI_CTRL_DELAY_MODE:
		ret = snfi_set_delay(nb, *(u8 *)args);
		break;

	case SNFI_CTRL_4FIFO_EN:
		val = readl(regs + SNF_MISC_CTL);
		if (*(u8 *)args)
			val |= FIFO4_EN;
		else
			val &= ~FIFO4_EN;

		writel(val, regs + SNF_MISC_CTL);
		break;

	case SNFI_CTRL_GF_CONFIG:
		val = *(u32 *)args;
		if (val)
			writel(val, regs + SNF_GF_CTL3);
		break;

	case SNFI_CTRL_SAMPLE_DELAY:
		writel(*(u8 *)args, regs + SNF_DLY_CTL3);
		break;

	case SNFI_CTRL_LATCH_LATENCY:
		val = readl(regs + SNF_MISC_CTL);
		val &= ~LATCH_LAT_MASK;
		val |= *(u8 *)args << LATCH_LAT_SHIFT;
		writel(val, regs + SNF_MISC_CTL);
		break;

	case SNFI_CTRL_MAC_QPI_MODE:
		nfi_spi->mac_qpi_mode = *(u8 *)args;
		break;

	default:
		ret = nfi_spi->parent->nfi.nfi_ctrl(&nfi_spi->base.nfi,
						    cmd, args);
		if (ret < 0) {
			pr_err("%s cmd(%d) args(%d) not support.\n",
			       __func__, cmd, *(u32 *)args);
			ret = -EOPNOTSUPP;
		}
		break;

	}

	return ret;
}

static int snfi_read_bytes(struct nfi *nfi, u8 *data, int count)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	int ret;

	writel(nfi_spi->tx_count, regs + SNF_MAC_OUTL);
	writel(count, regs + SNF_MAC_INL);

	ret = snfi_mac_op(nb);
	if (ret)
		return ret;

	snfi_read_mac(nfi_spi, data, count);

	nfi_spi->tx_count = 0;

	return 0;
}

static int snfi_write_bytes(struct nfi *nfi, u8 *data, int count)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;

	snfi_write_mac(nfi_spi, data, count);
	nfi_spi->tx_count += count;

	writel(0, regs + SNF_MAC_INL);
	writel(nfi_spi->tx_count, regs + SNF_MAC_OUTL);

	nfi_spi->tx_count = 0;

	return snfi_mac_op(nb);
}

static int snfi_reset(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u32 val;
	int ret;

	ret = nfi_spi->parent->nfi.reset(nfi);
	if (ret)
		return ret;

	val = readl(regs + SNF_MISC_CTL);
	val |= SW_RST;
	writel(val, regs + SNF_MISC_CTL);

	ret = readx_poll_timeout_atomic(readw, regs + SNF_STA_CTL1, val,
					!(val & SPI_STATE), 50,
					NFI_TIMEOUT);
	if (ret) {
		pr_warn("spi state active in reset [0x%x] = 0x%x\n",
			SNF_STA_CTL1, val);
		return ret;
	}

	val = readl(regs + SNF_MISC_CTL);
	val &= ~SW_RST;
	writel(val, regs + SNF_MISC_CTL);

	return 0;
}

static int snfi_config_for_write(struct nfi_base *nb, int count)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u32 val;

	nb->set_op_mode(regs, CNFG_CUSTOM_MODE);
#if 0
	val  = readl(regs + NFI_INTR_EN);
	if (nfi_spi->snfi_mode == SNFI_CUSTOM_MODE)
		val |= INTR_CUST_PROG_DONE;
	else if (nfi_spi->snfi_mode == SNFI_AUTO_MODE)
		val |= INTR_AUTO_PROG_DONE;
	writel(val, regs + NFI_INTR_EN);
#endif
	val = readl(regs + SNF_MISC_CTL);

	if (nfi_spi->write_cache_mode == SNFI_TX_114)
		val |= PG_LOAD_X4_EN;
	else
		val &= ~PG_LOAD_X4_EN;

	if (nfi_spi->snfi_mode == SNFI_CUSTOM_MODE) {
		val |= PG_LOAD_CUSTOM_EN;
		nfi_spi->snfi_status_mask = CUST_PROG_DONE;
	} else {
		val &= ~PG_LOAD_CUSTOM_EN;
		nfi_spi->snfi_status_mask = AUTO_PROG_DONE;
	}

	writel(val, regs + SNF_MISC_CTL);

	val = count * (nb->nfi.sector_size + nb->nfi.sector_spare_size);
	writel(val << PG_LOAD_SHIFT, regs + SNF_MISC_CTL2);

	val = readl(regs + SNF_PG_CTL1);
	val &= ~PG_LOAD_CMD_MASK;

	if (nfi_spi->snfi_mode == SNFI_CUSTOM_MODE)
		val |= nfi_spi->cmd[0] << PG_LOAD_CMD_SHIFT;
	else {
		val |= nfi_spi->cmd[0] << PG_LOAD_CMD_SHIFT;
		writel(nfi_spi->row_addr[0], regs + SNF_PG_CTL3);
	}

	writel(val, regs + SNF_PG_CTL1);
	writel(nfi_spi->col_addr[0], regs + SNF_PG_CTL2);

	writel(NFI_CMD_DUMMY_WR, regs + NFI_CMD);

	return 0;
}

static int snfi_config_for_read(struct nfi_base *nb, int count)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u32 val;
	int ret = 0;

	nb->set_op_mode(regs, CNFG_CUSTOM_MODE);

#if 0
	val  = readl(regs + NFI_INTR_EN);
	if (nfi_spi->snfi_mode == SNFI_CUSTOM_MODE)
		val |= INTR_CUST_READ_DONE;
	else if (nfi_spi->snfi_mode == SNFI_AUTO_MODE)
		val |= INTR_AUTO_READ_DONE;
	writel(val, regs + NFI_INTR_EN);
#endif
	val = readl(regs + SNF_MISC_CTL);
	val &= ~(DARA_READ_MODE_MASK | PG_LOAD_X4_EN);

	switch (nfi_spi->read_cache_mode) {

	case SNFI_RX_111:
		break;

	case SNFI_RX_112:
		val |= X2_DATA_MODE << READ_MODE_SHIFT;
		break;

	case SNFI_RX_114:
		val |= X4_DATA_MODE << READ_MODE_SHIFT;
		break;

	case SNFI_RX_122:
		val |= DUAL_IO_MODE << READ_MODE_SHIFT;
		break;

	case SNFI_RX_144:
		val |= QUAD_IO_MODE << READ_MODE_SHIFT;
		break;

	default:
		pr_err("Not support this read operarion: %d!\n",
		       nfi_spi->read_cache_mode);
		ret = -EINVAL;
		break;
	}

	if (nfi_spi->snfi_mode == SNFI_CUSTOM_MODE) {
		val |= DATARD_CUSTOM_EN;
		nfi_spi->snfi_status_mask = CUST_READ_DONE;
	} else {
		val &= ~DATARD_CUSTOM_EN;
		nfi_spi->snfi_status_mask = AUTO_READ_DONE;
	}

	writel(val, regs + SNF_MISC_CTL);

	val = count * (nb->nfi.sector_size + nb->nfi.sector_spare_size);
	writel(val, regs + SNF_MISC_CTL2);

	val = readl(regs + SNF_RD_CTL2);
	val &= ~DATA_READ_CMD_MASK;

	if (nfi_spi->snfi_mode == SNFI_CUSTOM_MODE) {
		val |= nfi_spi->cmd[0];
		writel(nfi_spi->col_addr[1], regs + SNF_RD_CTL3);
	} else {
		val |= nfi_spi->cmd[1];
		writel(nfi_spi->cmd[0] << PAGE_READ_CMD_SHIFT |
		       nfi_spi->row_addr[0], regs + SNF_RD_CTL1);
#if 0
		writel(nfi_spi->cmd[1] << GF_CMD_SHIFT |
		       nfi_spi->col_addr[1] << GF_ADDR_SHIFT,
		       regs + SNF_GF_CTL1);
#endif
		writel(nfi_spi->col_addr[0], regs + SNF_RD_CTL3);
	}

	writel(val, regs + SNF_RD_CTL2);

	writel(NFI_CMD_DUMMY_RD, regs + NFI_CMD);

	return ret;
}

static bool is_page_empty(struct nfi_base *nb, u8 *data, u8 *fdm,
			  int sectors)
{
	u32 *data32 = (u32 *)data;
	u32 *fdm32 = (u32 *)fdm;
	u32 i, count = 0;

	for (i = 0; i < nb->format.page_size >> 2; i++) {
		if (data32[i] != 0xffff) {
			count += zero_popcount(data32[i]);
			if (count > 10) {
				pr_debug("%d %d count:%d\n", __LINE__, i, count);
				return false;
			}
		}
	}

	if (fdm) {
		for (i = 0; i < (nb->nfi.fdm_size * sectors >> 2); i++)
			if (fdm32[i] != 0xffff) {
				count += zero_popcount(fdm32[i]);
				if (count > 10) {
					pr_debug("%d %d count:%d\n", __LINE__, i, count);
					return false;
				}
			}
	}

	pr_debug("page %d is empty\n", nb->row);

	return true;
}

static int rw_prepare(struct nfi_base *nb, int sectors, u8 *data,
		      u8 *fdm,
		      bool read)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	int ret;

	ret = nfi_spi->parent->rw_prepare(nb, sectors, data, fdm, read);
	if (ret)
		return ret;

	if (read)
		ret = snfi_config_for_read(nb, sectors);
	else
		ret = snfi_config_for_write(nb, sectors);

	return ret;
}

static int rw_wait_done(struct nfi_base *nb, int sectors, bool read)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	int ret;

	ret = nfi_spi->parent->rw_wait_done(nb, sectors, read);
	if (ret)
		return ret;

	ret = spi_wait_done(nb, NFI_TIMEOUT);
	if (ret)
		pr_warn("snfi wait done time out, status(0x%x) mask(0x%x)\n",
			readl(regs + SNF_STA_CTL1), nfi_spi->snfi_status_mask);

	return ret;
}

static void rw_complete(struct nfi_base *nb, u8 *data, u8 *fdm,
			bool read)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);
	void *regs = nb->res.nfi_regs;
	u32 val;

	nfi_spi->parent->rw_complete(nb, data, fdm, read);

	val = readl(regs + SNF_MISC_CTL);

	if (read)
		val &= ~DATARD_CUSTOM_EN;
	else
		val &= ~PG_LOAD_CUSTOM_EN;

	writel(val, regs + SNF_MISC_CTL);

	/* clear snfi status */
	val = readl(regs + SNF_STA_CTL1);
	val |= nfi_spi->snfi_status_mask;
	writel(val, regs + SNF_STA_CTL1);
	val &= ~nfi_spi->snfi_status_mask;
	writel(val, regs + SNF_STA_CTL1);

	nfi_spi->tx_count = 0;
	nfi_spi->cur_cmd_idx = 0;
	nfi_spi->cur_addr_idx = 0;
	nfi_spi->snfi_status_mask = 0;
	nfi_spi->mac_qpi_mode = 0;
}

static void set_nfi_base_funcs(struct nfi_base *nb)
{
	nb->nfi.reset = snfi_reset;
	nb->nfi.set_timing = snfi_set_timing;
	nb->nfi.wait_ready = snfi_wait_ready;

	nb->nfi.send_cmd = snfi_send_command;
	nb->nfi.send_addr = snfi_send_address;
	nb->nfi.trigger = snfi_trigger;
	nb->nfi.nfi_ctrl = snfi_ctrl;
	nb->nfi.select_chip = snfi_select_chip;

	nb->nfi.read_bytes = snfi_read_bytes;
	nb->nfi.write_bytes = snfi_write_bytes;

	nb->rw_prepare = rw_prepare;
	nb->rw_wait_done = rw_wait_done;
	nb->rw_complete = rw_complete;
	nb->is_page_empty = is_page_empty;
}

struct nfi *nfi_extend_spi_init(struct nfi_base *nb)
{
	struct nfi_spi *nfi_spi;

	nfi_spi = mem_alloc(1, sizeof(struct nfi_spi));
	if (!nfi_spi) {
		pr_err("snfi alloc memory fail @%s.\n", __func__);
		return NULL;
	}

	memcpy(&nfi_spi->base, nb, sizeof(struct nfi_base));
	nfi_spi->parent = nb;
	nfi_spi->base.nfi_irq_all = NFI_IRQ_SPI;

	nfi_spi->read_cache_mode = SNFI_RX_114;
	nfi_spi->write_cache_mode = SNFI_TX_114;
	nfi_spi->cur_cmd_idx = 0;

	set_nfi_base_funcs(&nfi_spi->base);

	/* Change nfi to spi mode */
	writel(SPI_MODE, nb->res.nfi_regs + SNF_SNF_CNFG);

	return &(nfi_spi->base.nfi);
}

void nfi_extend_spi_exit(struct nfi_base *nb)
{
	struct nfi_spi *nfi_spi = base_to_snfi(nb);

	mem_free(nfi_spi->parent);
	mem_free(nfi_spi);
}

