//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

/**
 * nfi_base.c - the base logic for nfi to access nand flash
 *
 * slc/mlc/tlc could use same code to access nand
 * of cause, there still some work need to do
 * even for spi nand, there should be a chance to integrate code together
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "../nfi.h"
#include "../nand_device.h"
#include "../nand_chip.h"
#include "nfi_regs.h"
#include "nfiecc.h"
#include "nfi_base.h"

static const int spare_size_v10[] = {
	16, 26, 27, 28, 32, 36, 40, 44, 48, 49, 50, 51,
	52, 62, 61, 63, 64, 67, 74
};

#ifdef NFI_RANDOM_SUPPORT
#define RAND_SEED_SHIFT(op) \
	((op) == RAND_ENCODE ? ENCODE_SEED_SHIFT : DECODE_SEED_SHIFT)
#define RAND_EN(op) \
	((op) == RAND_ENCODE ? RAN_ENCODE_EN : RAN_DECODE_EN)

#define SS_SEED_NUM     128
static u16 ss_randomizer_seed[SS_SEED_NUM] = {
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

static int nfi_enable_randomizer(struct nfi *nfi, u32 row, bool encode)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	enum randomizer_op op = RAND_ENCODE;
	void *regs = nb->res.nfi_regs;
	u32 val;

	if (!encode)
		op = RAND_DECODE;

	/* randomizer type and reseed type setup */
	val = readl(regs + NFI_CNFG);
	val |= CNFG_RAND_SEL | CNFG_RESEED_SEC_EN;
	writel(val, regs + NFI_CNFG);

	/* randomizer seed and type setup */
	val = ss_randomizer_seed[row % SS_SEED_NUM] & RAN_SEED_MASK;
	val <<= RAND_SEED_SHIFT(op);
	val |= RAND_EN(op);
	writel(val, regs + NFI_RANDOM_CNFG);

	return 0;
}

static int nfi_disable_randomizer(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);

	writel(0, nb->res.nfi_regs + NFI_RANDOM_CNFG);

	return 0;
}
#else
static int nfi_enable_randomizer(struct nfi *nfi, u32 row, bool encode)
{
	/* Not support */
	return 0;
}

static int nfi_disable_randomizer(struct nfi *nfi)
{
	/* Not support */
	return 0;
}
#endif

static inline void nfi_dump_register(struct nfi_base *nb)
{
	void *nfi_regs = nb->res.nfi_regs;
	void *ecc_regs = nb->ecc->res.regs;
	int i;

	pr_info("nfi registers:\n");
	for (i = 0; i < 0x600; i += 0x10) {
		pr_info("    address 0x%X : %X %X %X %X\n",
			(u32)((unsigned long)nfi_regs + i),
			(u32)readl(nfi_regs + i),
			(u32)readl(nfi_regs + i + 0x4),
			(u32)readl(nfi_regs + i + 0x8),
			(u32)readl(nfi_regs + i + 0xC));
	}

	pr_info("ecc registers:\n");
	for (i = 0; i < 0x600; i += 0x10) {
		pr_info("    address 0x%X : %X %X %X %X\n",
			(u32)((unsigned long)ecc_regs + i),
			(u32)readl(ecc_regs + i),
			(u32)readl(ecc_regs + i + 0x4),
			(u32)readl(ecc_regs + i + 0x8),
			(u32)readl(ecc_regs + i + 0xC));
	}
}

static int nfi_irq_handler(void *data)
{
	struct nfi_base *nb = nfi_to_base(data);
	void *regs = nb->res.nfi_regs;
	u16 status, en;
	int ret = 0;

	nandx_irq_disable(nb->res.nfi_irq_id);

	status = readl(regs + NFI_INTR_STA);
	en = readl(regs + NFI_INTR_EN);

	if (!(status & en)) {
		ret = NAND_IRQ_NONE;
		goto done;
	}

	nb->nfi_irq_status = status;
	nandx_event_complete(nb->done);

	/* Read clear irq status register */
	writel(0, regs + NFI_INTR_EN);

	writel(0, regs + NFI_INTR_STA);

	nandx_irq_enable(nb->res.nfi_irq_id);

	pr_debug("%s irq occur status:0x%x en:0x%x\n", __func__, status, en);

	/* Read clear irq status register */
	status =  readl(regs + NFI_INTR_STA);

	ret = NAND_IRQ_HANDLED;
done:
	return ret;
}

static int nfi_select_chip(struct nfi *nfi, int cs)
{
	struct nfi_base *nb = nfi_to_base(nfi);

	writel(cs, nb->res.nfi_regs + NFI_CSEL);

	return 0;
}

static inline void set_op_mode(void *regs, u32 mode)
{
	u32 val = readl(regs + NFI_CNFG);

	val &= ~CNFG_OP_MODE_MASK;
	val |= mode;

	writel(val, regs + NFI_CNFG);
}

static int nfi_reset(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret, val;

	/* Add IRQ Setting here, enable all slc/spi irq */
	if (nb->nfi_irq_en) {
		val  = readl(regs + NFI_INTR_EN);
		val |= NFI_IRQ_INTR;
		val |= nb->nfi_irq_all;
		writel(val, regs + NFI_INTR_EN);
		nb->nfi_irq_status = 0;
	}

	/* The NFI reset to reset all registers and force the NFI
	 * master be early terminated
	 */
	writel(CON_FIFO_FLUSH | CON_NFI_RST, regs + NFI_CON);

	/* check state of NFI internal FSM and NAND interface FSM */
	ret = readl_poll_timeout_atomic(regs + NFI_MASTER_STA, val,
					!(val & MASTER_BUS_BUSY),
					10, NFI_TIMEOUT);
	if (ret) {
		nfi_dump_register(nb);
		pr_warn("nfi reset timeout...\n");
	}

	writel(CON_FIFO_FLUSH | CON_NFI_RST, regs + NFI_CON);
	writew(STAR_DE, regs + NFI_STRDATA);

	return ret;
}

static void bad_mark_swap(struct nfi *nfi, u8 *buf, u8 *fdm)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	u32 start_sector = div_down(nb->col, nfi->sector_size);
	u32 data_mark_pos;
	u8 temp;

	/* raw access, no need to do swap. */
	if (!nb->ecc_en)
		return;

	if (!buf || !fdm)
		return;

	if (nb->bad_mark_ctrl.sector < start_sector ||
	    nb->bad_mark_ctrl.sector > start_sector + nb->rw_sectors)
		return;

	data_mark_pos = nb->bad_mark_ctrl.position +
			(nb->bad_mark_ctrl.sector - start_sector) *
			nfi->sector_size;

	temp = *fdm;
	*fdm = *(buf + data_mark_pos);
	*(buf + data_mark_pos) = temp;
}

static u8 *fdm_shift(struct nfi *nfi, u8 *fdm, int sector)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	u8 *pos;

	if (!fdm)
		return NULL;

	/* map the sector's FDM data to free oob:
	 * the beginning of the oob area stores the FDM data of bad mark sectors
	 */
	if (sector < nb->bad_mark_ctrl.sector)
		pos = fdm + (sector + 1) * nfi->fdm_size;
	else if (sector == nb->bad_mark_ctrl.sector)
		pos = fdm;
	else
		pos = fdm + sector * nfi->fdm_size;

	return pos;
}

static void set_bad_mark_ctrl(struct nfi_base *nb)
{
	int temp, page_size = nb->format.page_size;

	nb->bad_mark_ctrl.bad_mark_swap = bad_mark_swap;
	nb->bad_mark_ctrl.fdm_shift = fdm_shift;

	temp = nb->nfi.sector_size + nb->nfi.sector_spare_size;
	nb->bad_mark_ctrl.sector = div_down(page_size, temp);
	nb->bad_mark_ctrl.position = reminder(page_size, temp);
}


static void setup_spare_format(struct nfi_base *nb, int spare_idx)
{
	struct nfi *nfi = &nb->nfi;
	u32 val = readl(nb->res.nfi_regs + NFI_PAGEFMT);

	val &= ~PAGEFMT_SPARE_MASK;
	val &= ~PAGEFMT_FDM_MASK;
	val &= ~PAGEFMT_FDM_ECC_MASK;
	val |= spare_idx << PAGEFMT_SPARE_SHIFT;
	val |= nfi->fdm_size << PAGEFMT_FDM_SHIFT;
	val |= nfi->fdm_ecc_size << PAGEFMT_FDM_ECC_SHIFT;
	writel(val, nb->res.nfi_regs + NFI_PAGEFMT);

	if (nb->cus_sec_size_en) {
		/*TODO: check if right about custom sector setting*/
		val = nfi->sector_spare_size + nfi->sector_size;
		val |= SECCUS_SIZE_EN;
		writel(val, nb->res.nfi_regs + NFI_SECCUS_SIZE);
	}
}

static int setup_page_format(struct nfi_base *nb)
{
	struct nfi *nfi = &nb->nfi;
	u32 page_size = nb->format.page_size;
	u32 val, tmp;

	switch (page_size) {
	case 512:
		val = PAGEFMT_512_2K | PAGEFMT_SEC_SEL_512;
		break;

	case KB(2):
		if (nfi->sector_size == 512)
			val = PAGEFMT_2K_4K | PAGEFMT_SEC_SEL_512;
		else
			val = PAGEFMT_512_2K;

		break;

	case KB(4):
		if (nfi->sector_size == 512)
			val = PAGEFMT_4K_8K | PAGEFMT_SEC_SEL_512;
		else
			val = PAGEFMT_2K_4K;

		break;

	case KB(8):
		if (nfi->sector_size == 512)
			val = PAGEFMT_8K_16K | PAGEFMT_SEC_SEL_512;
		else
			val = PAGEFMT_4K_8K;

		break;

	case KB(16):
		val = PAGEFMT_8K_16K;
		break;

	default:
		pr_err("invalid page len: %d\n", page_size);
		return -EINVAL;
	}

	tmp = readl(nb->res.nfi_regs + NFI_PAGEFMT);
	tmp &= ~PAGEFMT_PAGE_MASK;
	tmp |= val;
	writel(tmp, nb->res.nfi_regs + NFI_PAGEFMT);

	return 0;
}

static int adjust_spare(struct nfi_base *nb, int *spare)
{
	int multi = nb->nfi.sector_size == 512 ? 1 : 2;
	int i, count = nb->caps->spare_size_num;

	if (*spare >= nb->caps->spare_size[count - 1] * multi) {
		*spare = nb->caps->spare_size[count - 1] * multi;
		return count - 1;
	}

	if (*spare < nb->caps->spare_size[0] * multi)
		return -EINVAL;

	for (i = 1; i < count; i++) {
		if (*spare < nb->caps->spare_size[i] * multi) {
			*spare = nb->caps->spare_size[i - 1] * multi;
			return i - 1;
		}
	}

	return -EINVAL;
}

static int nfi_set_format(struct nfi *nfi, struct nfi_format *format)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfiecc *ecc = nb->ecc;
	int ecc_strength = format->ecc_req;
	int min_fdm, min_ecc, max_ecc;
	u32 temp, page_sectors;
	int spare_idx = 0;

	if (!nb->buf) {
#if NANDX_BULK_IO_USE_DRAM
		nb->buf = (u8 *)NANDX_NFI_BUF_ADDR;
#else
		nb->buf = (u8 *)pmem_alloc(1, format->page_size + format->spare_size);
#endif
		if (!nb->buf)
			return -ENOMEM;
	}

#ifdef NANDX_TEST_BUF_ALIGN
	if (!nb->buf_align) {
#if NANDX_BULK_IO_USE_DRAM
		nb->buf_align = NANDX_NFI_BUF_ALIGN_ADDR;
#else
		nb->buf_align = mem_alloc(1,
					  format->page_size + format->spare_size + 0x100);
#endif
		if (!nb->buf_align)
			return -ENOMEM;
	}
#endif

	if (nb->res.force_spare_size) {
		format->spare_size = nb->res.force_spare_size;
		pr_debug("%s force spare_size to %d\n", __func__, format->spare_size);
	}

	nb->format = *format;

	/* setup sector_size according to the min oob required */
	if (nb->res.force_sector_size) {
		nfi->sector_size = nb->res.force_sector_size;
		if (nfi->sector_size == 512)
			ecc_strength >>= 1;
		pr_debug("%s force sector size to %d\n", __func__, nfi->sector_size);
	} else {
		if (nb->res.min_oob_req / nb->caps->max_fdm_size >=
		    format->page_size / 1024) {
			if (format->page_size / 512 <
			    nb->res.min_oob_req / nb->caps->max_fdm_size)
				return -EINVAL;

			nfi->sector_size = 512;
			/* format->ecc_req is the requirement per 1KB */
			ecc_strength >>= 1;
		} else
			nfi->sector_size = 1024;
	}

	page_sectors = div_down(format->page_size, nfi->sector_size);
	nfi->sector_spare_size = div_down(format->spare_size, page_sectors);

	if (!nb->cus_sec_size_en) {
		spare_idx = adjust_spare(nb, &nfi->sector_spare_size);
		if (spare_idx < 0)
			return -EINVAL;
	}

	/* calculate ecc strength and fdm size */
	temp = (nfi->sector_spare_size - nb->caps->max_fdm_size) * 8;
	min_ecc = div_down(temp, nb->caps->ecc_parity_bits);
	min_ecc = ecc->adjust_strength(ecc, min_ecc);
	if (min_ecc < 0)
		return -EINVAL;

	temp = div_up(nb->res.min_oob_req, page_sectors);
	temp = (nfi->sector_spare_size - temp) * 8;
	max_ecc = div_down(temp, nb->caps->ecc_parity_bits);
	max_ecc = ecc->adjust_strength(ecc, max_ecc);
	if (max_ecc < 0)
		return -EINVAL;

	temp = div_up(temp * nb->caps->ecc_parity_bits, 8);
	temp = nfi->sector_spare_size - temp;
	min_fdm = min(temp, (u32)nb->caps->max_fdm_size);

	if (ecc_strength > max_ecc) {
		pr_warn("required ecc strength %d, max supported %d\n",
			ecc_strength, max_ecc);
		nfi->ecc_strength = max_ecc;
		nfi->fdm_size = min_fdm;
	} else if (ecc_strength < min_ecc) {
		nfi->ecc_strength = min_ecc;
		nfi->fdm_size = nb->caps->max_fdm_size;
	} else {
		ecc_strength = ecc->adjust_strength(ecc, ecc_strength);
		if (ecc_strength < 0)
			return -EINVAL;

		nfi->ecc_strength = ecc_strength;
		temp = div_up((u32)ecc_strength * nb->caps->ecc_parity_bits, 8);
		nfi->fdm_size = nfi->sector_spare_size - temp;
	}

	nb->page_sectors = div_down(format->page_size, nfi->sector_size);

	/* some IC has fixed fdm_ecc_size, if not assigend, set to fdm_size */
	nfi->fdm_ecc_size = nb->caps->fdm_ecc_size ? : nfi->fdm_size;

	nfi->ecc_parity_size = div_up((u32)nfi->ecc_strength *
				      nb->caps->ecc_parity_bits,
				      8);
	set_bad_mark_ctrl(nb);

	pr_debug("sector_size: %d\n", nfi->sector_size);
	pr_debug("sector_spare_size: %d\n", nfi->sector_spare_size);
	pr_debug("fdm_size: %d\n", nfi->fdm_size);
	pr_debug("fdm_ecc_size: %d\n", nfi->fdm_ecc_size);
	pr_debug("ecc_strength: %d\n", nfi->ecc_strength);
	pr_debug("ecc_parity_size: %d\n", nfi->ecc_parity_size);

	setup_spare_format(nb, spare_idx);
	return setup_page_format(nb);
}

static int update_io_format(struct nfi_base *nb,
			    struct nfi_io_format *ioformat)
{
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 val, spare;
	int ret = 0, ecc_level, tmp;

	/* custom sector format setting */
	val = readl(regs + NFI_SECCUS_SIZE);
	if (ioformat->cus_sec_en) {
		val |= SECCUS_SIZE_EN;
		val &= ~SECCUS_SIZE_MASK;
		val |= ioformat->cus_sec_size & SECCUS_SIZE_MASK;
		nfi->sector_size = ioformat->cus_sec_size;
	} else
		val &= ~SECCUS_SIZE_EN;
	writel(val, regs + NFI_SECCUS_SIZE);

	/* if custom sector disable, just set normal sector format */
	if (!ioformat->cus_sec_en) {
		if (ioformat->sec_size == 1024)
			nfi->sector_size = 1024;
		else if (ioformat->sec_size == 512)
			nfi->sector_size = 512;
		else
			return -EINVAL;
		ret = setup_page_format(nb);
		if (ret)
			return ret;
	}

	/* spare format settting */
	spare = ioformat->sec_spare_size;
	tmp = adjust_spare(nb, &spare);
	if (tmp < 0)
		return -EINVAL;
	val = readl(regs + NFI_PAGEFMT);
	val &= ~PAGEFMT_SPARE_MASK;
	val &= ~PAGEFMT_FDM_MASK;
	val &= ~PAGEFMT_FDM_ECC_MASK;
	val |= tmp << PAGEFMT_SPARE_SHIFT;
	val |= ioformat->fdm_size << PAGEFMT_FDM_SHIFT;
	val |= ioformat->fdm_ecc_size << PAGEFMT_FDM_ECC_SHIFT;
	writel(val, regs + NFI_PAGEFMT);
	nfi->fdm_size = ioformat->fdm_size;
	nfi->fdm_ecc_size = ioformat->fdm_ecc_size;

	ecc_level = nb->ecc->adjust_strength(nb->ecc, ioformat->ecc_level_sel);
	if (ecc_level < 0)
		return -EINVAL;
	nfi->ecc_strength = ecc_level;
	nfi->ecc_parity_size = div_up((u32)nfi->ecc_strength *
				      nb->caps->ecc_parity_bits,
				      8);
	return ret;
}

static int nfi_ctrl(struct nfi *nfi, int cmd, void *args)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret = 0;
	u32 val;

	switch (cmd) {
	case NFI_CTRL_DMA:
		nb->dma_en = *(bool *)args;
		break;

	case NFI_BURST_EN:
		nb->dma_burst_en = *(bool *)args;
		break;

	case NFI_CTRL_NFI_IRQ:
		nb->nfi_irq_en = *(bool *)args;
		break;

	case NFI_CTRL_IO_FORMAT:
		ret = update_io_format(nb, args);
		break;

	case NFI_CTRL_IOCON:
		val = readl(regs + NFI_IOCON);
		val &= ~BRSTN_MASK;
		val |= *(u32 *)args << BRSTN_SHIFT;
		writel(val, regs + NFI_IOCON);
		break;

	case NFI_CTRL_ECC:
		nb->ecc_en = *(bool *)args;
		break;

	case NFI_CTRL_ECC_MODE:
		nb->ecc_mode = *(enum nfiecc_mode *)args;
		break;

	case NFI_CTRL_ECC_DECODE_MODE:
		nb->ecc_deccon = *(enum nfiecc_deccon *)args;
		break;

	case NFI_CTRL_BAD_MARK_SWAP:
		nb->bad_mark_swap_en = *(bool *)args;
		break;

#ifdef NANDX_TEST_BUF_ALIGN
	case NFI_ADDR_ALIGNMENT_EN:
		if (*(u8 *)args)
			nb->buf = nb->buf_align + *(u8 *)args;
		else
			nb->buf = nb->buf_align;
		break;
#endif

	case NFI_BYTE_RW_EN:
		nb->byte_rw_en = *(bool *)args;
		break;

	case NFI_CRC_EN:
		/* TODO */
		break;

	default:
		ret = nb->ecc->nfiecc_ctrl(nb->ecc, cmd, args);
		if (ret < 0) {
			pr_err("%s cmd(%d) args(%d) not support.\n",
			       __func__, cmd, *(u32 *)args);
		}
		break;
	}

	return ret;
}

static int nfi_send_cmd(struct nfi *nfi, short cmd)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	pr_debug("%s: cmd 0x%x\n", __func__, cmd);

	if (cmd < 0)
		return -EINVAL;

	set_op_mode(regs, nb->op_mode);

	writel(cmd, regs + NFI_CMD);

	ret = readl_poll_timeout_atomic(regs + NFI_STA,
					val, !(val & STA_CMD),
					5, NFI_TIMEOUT);
	if (ret)
		pr_err("send cmd 0x%x timeout\n", cmd);

	return ret;
}

static int nfi_send_addr(struct nfi *nfi, int col, int row,
			 int col_cycle, int row_cycle)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	pr_debug("%s: col 0x%x, row 0x%x, col_cycle 0x%x, row_cycle 0x%x\n",
		 __func__, col, row, col_cycle, row_cycle);

	nb->col = col;
	nb->row = row;

	writel(col, regs + NFI_COLADDR);
	writel(row, regs + NFI_ROWADDR);
	writel(col_cycle | (row_cycle << ROW_SHIFT), regs + NFI_ADDRNOB);

	ret = readl_poll_timeout_atomic(regs + NFI_STA,
					val, !(val & STA_ADDR),
					5, NFI_TIMEOUT);
	if (ret)
		pr_err("send address timeout\n");

	return ret;
}

static int nfi_trigger(struct nfi *nfi)
{
	/* Nothing need to do. */
	return 0;
}

static inline int wait_io_ready(void *regs)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(regs + NFI_PIO_DIRDY,
					val, val & PIO_DI_RDY,
					2, NFI_TIMEOUT);
	if (ret)
		pr_err("wait io ready timeout\n");

	return ret;
}

static int wait_ready_irq(struct nfi_base *nb, u32 timeout)
{
	void *regs = nb->res.nfi_regs;
	int ret = 0;
	u32 val, i = 0;

	val = readl(regs + NFI_INTR_EN);
	writel(0xf1, regs + NFI_CNRNB);
	nandx_event_init(nb->done);

	/**
	 * check if nand already bean ready,
	 * avoid issue that casued by missing irq-event.
	 * AHB and Busy can not be valid at same time.
	 */
	while (i++ < timeout) {
		if (nb->nfi_irq_status) {
			pr_debug("%s IRQ done en:%x, status:%x time:%duS\n",
				 __func__, val, nb->nfi_irq_status, i);
			break;
		}
		udelay(10);
	}

	if (i > timeout) {
		pr_err("IRQ timeout, irq_en: 0x%x\n", val);
		ret = -EIO;
	}

	ret = nandx_event_wait_complete(nb->done, timeout);

	writew(0, regs + NFI_CNRNB);
	return ret;
}

static int wait_ready_poll(struct nfi_base *nb, u32 timeout)
{
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	writel(0x21, regs + NFI_CNRNB);
	ret = readl_poll_timeout_atomic(regs + NFI_STA, val,
					val & STA_BUSY2READY,
					2, timeout);
	writew(0, regs + NFI_CNRNB);

	return ret;
}

static void wait_ready_time(struct nfi_base *nb, u32 timeout)
{
	udelay(timeout);
}

static int nfi_wait_ready(struct nfi *nfi, int type, u32 timeout)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	int ret;

	switch (type) {
	case NAND_WAIT_BUSY:
		if (nb->nfi_irq_en)
			ret = wait_ready_irq(nb, timeout);
		else
			ret = wait_ready_poll(nb, timeout);
		break;

	case NAND_WAIT_TIME:
		wait_ready_time(nb, timeout);
		ret = 0;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	if (ret) {
		pr_err("%s: type 0x%x, timeout 0x%x\n",
		       __func__, type, timeout);
		nfi_dump_register(nb);
	}

	return ret;
}

static int enable_ecc_decode(struct nfi_base *nb, int sectors)
{
	struct nfi *nfi = &nb->nfi;
	struct nfiecc *ecc = nb->ecc;

	ecc->config.op = ECC_DECODE;
	ecc->config.mode = nb->ecc_mode;
	ecc->config.deccon = nb->ecc_deccon;
	ecc->config.sectors = sectors;
	ecc->config.len = nfi->sector_size + nfi->fdm_ecc_size;
	ecc->config.strength = nfi->ecc_strength;

	return ecc->enable(ecc);
}

static int enable_ecc_encode(struct nfi_base *nb)
{
	struct nfiecc *ecc = nb->ecc;
	struct nfi *nfi = &nb->nfi;

	ecc->config.op = ECC_ENCODE;
	ecc->config.mode = nb->ecc_mode;
	ecc->config.len = nfi->sector_size + nfi->fdm_ecc_size;
	ecc->config.strength = nfi->ecc_strength;

	return ecc->enable(ecc);
}

static void read_fdm(struct nfi_base *nb, u8 *fdm, int start_sector,
		     int sectors)
{
	void *regs = nb->res.nfi_regs;
	int j, i;
	u32 vall, valm;
	u8 *buf = fdm;

	for (i = start_sector; i < start_sector + sectors; i++) {
		if (nb->bad_mark_swap_en)
			buf = nb->bad_mark_ctrl.fdm_shift(&nb->nfi, fdm, i);

		vall = readl(regs + NFI_FDML(i));
		valm = readl(regs + NFI_FDMM(i));

		for (j = 0; j < nb->nfi.fdm_size; j++)
			*buf++ = (j >= 4 ? valm : vall) >> ((j & 3) << 3);
	}
}

static void write_fdm(struct nfi_base *nb, u8 *fdm)
{
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 vall, valm;
	int i, j;
	u8 *buf = fdm;

	for (i = 0; i < nb->page_sectors; i++) {
		if (nb->bad_mark_swap_en)
			buf = nb->bad_mark_ctrl.fdm_shift(nfi, fdm, i);

		vall = 0;
		for (j = 0; j < 4; j++)
			vall |= (j < nfi->fdm_size ? *buf++ : 0xff) << (j * 8);
		writel(vall, regs + NFI_FDML(i));

		valm = 0;
		for (j = 0; j < 4; j++)
			valm |= (j < nfi->fdm_size ? *buf++ : 0xff) << (j * 8);
		writel(valm, regs + NFI_FDMM(i));
	}
}

static bool is_page_empty(struct nfi_base *nb, u8 *data, u8 *fdm,
			  int sectors)
{
	u32 empty = readl(nb->res.nfi_regs + NFI_STA) & STA_EMP_PAGE;

	if (empty) {
		pr_debug("empty page!\n");
		return true;
	}

	return false;
}

/* NOTE: pio not use auto format */
static int pio_rx_data(struct nfi_base *nb, u8 *data, u8 *fdm,
		       int sectors)
{
	struct nfiecc_status ecc_status;
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 val, bitflips = 0;
	int len, ret, i;
	u32 *buf32;
	u8 *buf;

	len = nfi->sector_size + nfi->sector_spare_size;
	len *= sectors;

	val = readl(regs + NFI_CNFG);
	if (nb->byte_rw_en) {
		val |= CNFG_BYTE_RW;
		writel(val, regs + NFI_CNFG);

		for (i = 0; i < len; i++) {
			ret = wait_io_ready(regs);
			if (ret)
				return ret;

			nb->buf[i] = readb(regs + NFI_DATAR);
		}
	} else {
		val &= ~CNFG_BYTE_RW;
		writel(val, regs + NFI_CNFG);

		buf32 = (u32 *)nb->buf;
		for (i = 0; i < (len >> 2); i++) {
			ret = wait_io_ready(regs);
			if (ret)
				return ret;

			buf32[i] = readl(regs + NFI_DATAR);
		}
	}

	/* TODO: do error handle for autoformat setting of pio */
	if (nb->ecc_en) {
		for (i = 0; i < sectors; i++) {
			buf = nb->buf + i * (nfi->sector_size +
					     nfi->sector_spare_size);
			
			if (nb->ecc_deccon == ECC_DEC_LOCATE) {
				/* Need to do manual corrrect data for PIO mode */
				nb->ecc->correct_data(nb->ecc,
						      &ecc_status,
						      buf, i);
				ret = nb->ecc->decode_status(nb->ecc, i, 1);
				if (ret < 0) {
					if (nb->is_page_empty(nb,
							      nb->buf,
							      fdm,
							      sectors))
						return 0;
					return ret;
				}
				bitflips = max((int)bitflips, ret);
			} else if (nb->ecc_deccon == ECC_DEC_FER) {
				ret = nb->ecc->decode_status(nb->ecc, i, 1);
				if (ret)
					return ret;
			}

			if (data)
				memcpy(data + i * nfi->sector_size,
				       buf, nfi->sector_size);
			if (fdm)
				memcpy(fdm + i * nfi->fdm_size,
				       buf + nfi->sector_size, nfi->fdm_size);
		}

		return bitflips;
	}

	/* raw read, only data not null, and its length should be $len */
	if (data)
		memcpy(data, nb->buf, len);

	return 0;
}

static int pio_tx_data(struct nfi_base *nb, u8 *data, u8 *fdm,
		       int sectors)
{
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 i, val, *buf32;
	int len, ret;

	len = nb->ecc_en ? nfi->sector_size :
	      nfi->sector_size + nfi->sector_spare_size;
	len *= sectors;

	/* raw read, only data not null, and its length should be $len */
	if (data)
		memcpy(nb->buf, data, len);

	val = readl(regs + NFI_CNFG);
	if (nb->byte_rw_en) {
		val |= CNFG_BYTE_RW;
		writel(val, regs + NFI_CNFG);

		for (i = 0; i < len; i++) {
			ret = wait_io_ready(regs);
			if (ret)
				return ret;
			writeb(nb->buf[i], regs + NFI_DATAW);
		}
	} else {
		val &= ~CNFG_BYTE_RW;
		writel(val, regs + NFI_CNFG);
		buf32 = (u32 *)nb->buf;
		for (i = 0; i < (len >> 2); i++) {
			ret = wait_io_ready(regs);
			if (ret)
				return ret;
			writel(buf32[i], regs + NFI_DATAW);
		}
	}

	return 0;
}

static int rw_prepare(struct nfi_base *nb, int sectors, u8 *data,
		      u8 *fdm, bool read)
{
	void *regs = nb->res.nfi_regs;
	u32 len = nb->nfi.sector_size * sectors;
	bool irq_en = nb->dma_en && nb->nfi_irq_en;
	void *dma_addr;
	u32 val;
	int ret;

	nb->rw_sectors = sectors;

	/* IRQ setting, enable all slc/spi irq */
	if (irq_en) {
		nandx_event_init(nb->done);
		val  = readl(regs + NFI_INTR_EN);
		val |= NFI_IRQ_INTR;
		val |= nb->nfi_irq_all;
		writel(val, regs + NFI_INTR_EN);
		nb->nfi_irq_status = 0;
	}

	val = readw(regs + NFI_CNFG);
	if (read)
		val |= CNFG_READ_EN;
	else
		val &= ~CNFG_READ_EN;

	/* as design, now, auto format enabled when ecc enabled */
	if (nb->ecc_en) {
		val |= CNFG_HW_ECC_EN | CNFG_AUTO_FMT_EN;

		if (read)
			ret = enable_ecc_decode(nb, sectors);
		else
			ret = enable_ecc_encode(nb);

		if (ret) {
			pr_warn("%s: ecc enable %s fail!\n", __func__,
				read ? "decode" : "encode");
			nfi_dump_register(nb);
			return ret;
		}
	}

	if (!read && nb->bad_mark_swap_en)
		nb->bad_mark_ctrl.bad_mark_swap(&nb->nfi, data, fdm);

	/* Need check with auto_fmt_en flag */
	if (!nb->ecc_en)
		len += sectors * nb->nfi.sector_spare_size;

	if (nb->dma_en) {
		val |= CNFG_AHB;
		if (nb->dma_burst_en)
			val |= CNFG_DMA_BURST_EN;
		else
			val &= ~CNFG_DMA_BURST_EN;

		if (read) {
			dma_addr = (void *)(unsigned long)nandx_dma_map(
					   nb->res.dev, nb->buf,
					   (u64)len, NDMA_FROM_DEV);
		} else {
			memcpy(nb->buf, data, len);
			dma_addr = (void *)(unsigned long)nandx_dma_map(
					   nb->res.dev, nb->buf,
					   (u64)len, NDMA_TO_DEV);
		}

		if (dma_mapping_error(nb->res.dev, (dma_addr_t)dma_addr)) {
			pr_err("unable to map nfi memory\n");
			return -ENOMEM;
		}

		writel((unsigned long)dma_addr, (void *)regs + NFI_STRADDR);

		nb->access_len = len;
		nb->dma_addr = dma_addr;
	} else {
		val &= ~CNFG_AHB;
		val &= ~CNFG_DMA_BURST_EN;
	}

	if (nb->ecc_en && !read && fdm)
		write_fdm(nb, fdm);

	writew(val, regs + NFI_CNFG);
	/* setup R/W sector number */
	writel(sectors << CON_SEC_SHIFT, regs + NFI_CON);

	return 0;
}

static void rw_trigger(struct nfi_base *nb, bool read)
{
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = readl(regs + NFI_CON);
	val |= read ? CON_BRD : CON_BWR;
	writel(val, regs + NFI_CON);

	writel(STAR_EN, regs + NFI_STRDATA);
}

static int rw_wait_done(struct nfi_base *nb, int sectors, bool read)
{
	void *regs = nb->res.nfi_regs;
	bool irq_en = nb->dma_en && nb->nfi_irq_en;
	int ret;
	u32 val;

	if (irq_en) {
		ret = wait_ready_irq(nb, NFI_TIMEOUT);
		if (ret) {
			pr_err("AHB IRQ error read:%d, sectors:%d\n",
			       read, sectors);
		}

		ret = nandx_event_wait_complete(nb->done, NFI_TIMEOUT);
		if (!ret)
			writew(0, regs + NFI_INTR_EN);
	}

	if (read) {
		ret = readl_poll_timeout(regs + NFI_BYTELEN, val,
						ADDRCNTR_SEC(val) >=
						(u32)sectors,
						2, NFI_TIMEOUT);
		/* HW issue: if not wait ahb done, need polling bus busy */
		if (!ret && !irq_en)
			ret = readl_poll_timeout(regs + NFI_MASTER_STA,
							val,
							!(val &
							  MASTER_BUS_BUSY),
							2, NFI_TIMEOUT);
	} else {
		ret = readl_poll_timeout(regs + NFI_ADDRCNTR, val,
						ADDRCNTR_SEC(val) >=
						(u32)sectors,
						2, NFI_TIMEOUT);
	}

	if (ret) {
		pr_warn("do page %s timeout\n", read ? "read" : "write");
		nfi_dump_register(nb);
		return ret;
	}

	if (read && nb->ecc_en) {
		ret = nb->ecc->wait_done(nb->ecc);
		if (ret) {
			nfi_dump_register(nb);
			return ret;
		}

		return nb->ecc->decode_status(nb->ecc, 0, sectors);
	}

	return 0;
}

static int rw_data(struct nfi_base *nb, u8 *data, u8 *fdm, int sectors,
		   bool read)
{
	if (read && nb->dma_en && nb->ecc_en && fdm)
		read_fdm(nb, fdm, 0, sectors);

	if (!nb->dma_en) {
		if (read)
			return pio_rx_data(nb, data, fdm, sectors);

		return pio_tx_data(nb, data, fdm, sectors);
	}

	return 0;
}

static void rw_complete(struct nfi_base *nb, u8 *data, u8 *fdm,
			bool read)
{
	bool is_empty;

	if (nb->dma_en) {
		if (read) {
			nandx_dma_unmap(nb->res.dev, nb->buf, nb->dma_addr,
					(u64)nb->access_len, NDMA_FROM_DEV);

			if (data)
				memcpy(data, nb->buf, nb->access_len);
		} else {
			nandx_dma_unmap(nb->res.dev, nb->buf, nb->dma_addr,
					(u64)nb->access_len, NDMA_TO_DEV);
		}
	}

	if (read) {
		is_empty = nb->is_page_empty(nb, data, fdm, nb->rw_sectors);
		if (is_empty) {
			nb->read_status = 0;

			/* try to memset 0xff for empty page */
			if (data)
				memset(data, 0xff, nb->access_len);
			if (nb->ecc_en && fdm)
				memset(fdm, 0xff, nb->nfi.fdm_size * nb->rw_sectors);
		}
	}

	/* whether it's reading or writing, we all check if nee swap
	 * for write, we need to restore data
	 */
	if (nb->bad_mark_swap_en)
		nb->bad_mark_ctrl.bad_mark_swap(&nb->nfi, data, fdm);

	if (nb->ecc_en)
		nb->ecc->disable(nb->ecc);

	writel(0, nb->res.nfi_regs + NFI_CNFG);
	writel(0, nb->res.nfi_regs + NFI_CON);
}

static int nfi_read_sectors(struct nfi *nfi, u8 *data, u8 *fdm,
			    int sectors)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	int bitflips = 0, ret;

	pr_debug("%s: read page#%d\n", __func__, nb->row);
	pr_debug("%s: data address 0x%x, fdm address 0x%x,sectors 0x%x, buf: 0x%x\n",
		 __func__, (u32)((unsigned long)data),
		 (u32)((unsigned long)fdm), sectors,
		 (u32)((unsigned long)nb->buf));
	nb->read_status = 0;

	ret = nb->rw_prepare(nb, sectors, data, fdm, true);
	if (ret)
		return ret;

	nb->rw_trigger(nb, true);

	if (nb->dma_en) {
		ret = nb->rw_wait_done(nb, sectors, true);
		if (ret > 0)
			bitflips = ret;
		else if (ret == -ENANDREAD)
			nb->read_status = -ENANDREAD;
		else if (ret < 0)
			goto complete;
	}

	ret = nb->rw_data(nb, data, fdm, sectors, true);
	if (ret >= 0)
		ret = max(ret, bitflips);

complete:
	nb->rw_complete(nb, data, fdm, true);

	if (nb->read_status == -ENANDREAD)
		return -ENANDREAD;

	return ret;
}

int nfi_write_page(struct nfi *nfi, u8 *data, u8 *fdm)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	u32 sectors = div_down(nb->format.page_size, nfi->sector_size);
	int ret;

	pr_debug("%s: write page#%d\n", __func__, nb->row);
	pr_debug("%s: data address 0x%x, fdm address 0x%x, buf 0x%x\n",
		 __func__, (int)((unsigned long)data),
		 (int)((unsigned long)fdm), (u32)((unsigned long)nb->buf));

	ret = nb->rw_prepare(nb, sectors, data, fdm, false);
	if (ret)
		return ret;

	nb->rw_trigger(nb, false);

	ret = nb->rw_data(nb, data, fdm, sectors, false);
	if (ret)
		return ret;

	ret = nb->rw_wait_done(nb, sectors, false);

	nb->rw_complete(nb, data, fdm, false);

	return ret;
}

static void nfi_enable_seccus(struct nfi *nfi, int count)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = readl(regs + NFI_SECCUS_SIZE);
	val &= ~SECCUS_SIZE_MASK;
	val |= ((count & SECCUS_SIZE_MASK) << SECCUS_SIZE_SHIFT);
	val |= SECCUS_SIZE_EN;
	writel(val, regs + NFI_SECCUS_SIZE);
}

static void nfi_disable_seccus(struct nfi *nfi, int count)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = readl(regs + NFI_SECCUS_SIZE);
	val &= ~(SECCUS_SIZE_MASK | SECCUS_SIZE_EN);
	writel(val, regs + NFI_SECCUS_SIZE);
}

static int nfi_rw_bytes(struct nfi *nfi, u8 *data, int count, bool read)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int i, ret;
	u32 val;

	for (i = 0; i < count; i++) {
		val = readl(regs + NFI_STA) & NFI_FSM_MASK;
		if (val != NFI_FSM_CUSTDATA) {
			val = readw(regs + NFI_CNFG) | CNFG_BYTE_RW;
			if (read)
				val |= CNFG_READ_EN;
			writew(val, regs + NFI_CNFG);

			if (count < nfi->sector_size) {
				nfi_enable_seccus(nfi, count);
				val = 1;
			} else {
				val = div_up(count, nfi->sector_size);
			}
			val = (val << CON_SEC_SHIFT) | CON_BRD | CON_BWR;
			writel(val, regs + NFI_CON);

			writew(STAR_EN, regs + NFI_STRDATA);
		}

		ret = wait_io_ready(regs);
		if (ret)
			break;

		if (read)
			data[i] = readb(regs + NFI_DATAR);
		else
			writeb(data[i], regs + NFI_DATAW);
	}

	if (count < nfi->sector_size) {
		nfi_disable_seccus(nfi, count);
	}

	writel(0, nb->res.nfi_regs + NFI_CNFG);

	return ret;
}

static int nfi_read_bytes(struct nfi *nfi, u8 *data, int count)
{
	return nfi_rw_bytes(nfi, data, count, true);
}

static int nfi_write_bytes(struct nfi *nfi, u8 *data, int count)
{
	return nfi_rw_bytes(nfi, data, count, false);
}

/* As register map says, only when flash macro is idle,
 * sw reset or nand interface change can be issued
 */
static inline int wait_flash_macro_idle(void *regs)
{
	u32 val;

	return readl_poll_timeout_atomic(regs + NFI_STA, val,
					 val & FLASH_MACRO_IDLE, 2,
					 NFI_TIMEOUT);
}

#define ACCTIMING(tpoecs, tprecs, tc2r, tw2r, twh, twst, trlt) \
	((tpoecs) << 28 | (tprecs) << 22 | (tc2r) << 16 | \
	 (tw2r) << 12 | (twh) << 8 | (twst) << 4 | (trlt))

static int nfi_set_sdr_timing(struct nfi *nfi, void *timing, u8 type)
{
	struct nand_sdr_timing *sdr = (struct nand_sdr_timing *) timing;
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	u32 tpoecs, tprecs, tc2r, tw2r, twh, twst, trlt, tstrobe;
	u32 rate, val;
	u16 thold;
	int ret;

	ret = wait_flash_macro_idle(regs);
	if (ret) {
		pr_err("wait_flash_macro_idle fail.\n");
		return ret;
	}

	/* turn clock rate into KHZ */
	rate = div_down(nb->res.clock_1x, 1000);

	tpoecs = max(sdr->tALH, sdr->tCLH);
	tpoecs = div_up(tpoecs * rate, 1000000);
	tpoecs &= 0xf;

	tprecs = max(sdr->tCLS, sdr->tALS);
	tprecs = div_up(tprecs * rate, 1000000) + 1;
	tprecs &= 0x3f;

	/* tc2r is in unit of 2T */
	tc2r = div_up(sdr->tCR * rate, 1000000);
	tc2r = div_down(tc2r, 2);
	tc2r &= 0x3f;

	tw2r = div_up(sdr->tWHR * rate, 1000000);
	tw2r = div_down(tw2r, 2);
	tw2r &= 0xf;

	twh = max(sdr->tREH, sdr->tWH);
	twh = div_up(twh * rate, 1000000) - 1;
	twh &= 0xf;

	twst = 0;
	thold = div_down((twh + 1) * 1000000, rate);
	if (thold < sdr->tWC)
		twst = sdr->tWC - thold;
	twst = max((u32)sdr->tWP, twst);
	twst = div_up(twst * rate, 1000000) - 1;
	twst &= 0xf;

	trlt = 0;
	if (thold < sdr->tRC)
		trlt = sdr->tRC - thold;
	trlt = max((u32)sdr->tRP, trlt);
	trlt = div_up(trlt * rate, 1000000) - 1;
	trlt &= 0xf;

	tstrobe = 0;
	/* If tREA is bigger than tRP, setup strobe sel here */
	if (div_down((trlt + 1) * 1000000, rate) < sdr->tREA) {
		tstrobe = div_up(sdr->tREA * rate, 1000000);
		tstrobe -= trlt + 1;
		val = readl(regs + NFI_DEBUG_CON1);
		val &= ~STROBE_MASK;
		val |= tstrobe << STROBE_SHIFT;
		writel(val, regs + NFI_DEBUG_CON1);
	}

	/*
	 * ACCON: access timing control register
	 * -------------------------------------
	 * 31:28: tpoecs, minimum required time for CS post pulling down after
	 *        accessing the device
	 * 27:22: tprecs, minimum required time for CS pre pulling down before
	 *        accessing the device
	 * 21:16: tc2r, minimum required time from NCEB low to NREB low
	 * 15:12: tw2r, minimum required time from NWEB high to NREB low.
	 * 11:08: twh, write enable hold time
	 * 07:04: twst, write wait states
	 * 03:00: trlt, read wait states
	 */
	val = ACCTIMING(tpoecs, tprecs, tc2r, tw2r, twh, twst, trlt);
	pr_debug("NAND acctiming: 0x%x, strobe_sel:0x%x\n", val, tstrobe);
	writel(val, regs + NFI_ACCCON);

	/* set NAND type */
	writel(NAND_TYPE_ASYNC, regs + NFI_NAND_TYPE_CNFG);

	return ret;
}

static int nfi_set_timing(struct nfi *nfi, void *timing, int type)
{
	switch (type) {
	case NAND_TIMING_SDR:
		return nfi_set_sdr_timing(nfi, timing, type);

	/* NOTE: for mlc/tlc */
	case NAND_TIMING_SYNC_DDR:
	case NAND_TIMING_TOGGLE_DDR:
	case NAND_TIMING_NVDDR2:
	default:
		return -EINVAL;
	}

	return 0;
}

static void set_nfi_funcs(struct nfi *nfi)
{
	nfi->select_chip = nfi_select_chip;
	nfi->set_format = nfi_set_format;
	nfi->nfi_ctrl = nfi_ctrl;
	nfi->set_timing = nfi_set_timing;

	nfi->reset = nfi_reset;
	nfi->send_cmd = nfi_send_cmd;
	nfi->send_addr = nfi_send_addr;
	nfi->trigger = nfi_trigger;

	nfi->write_page = nfi_write_page;
	nfi->write_bytes = nfi_write_bytes;
	nfi->read_sectors = nfi_read_sectors;
	nfi->read_bytes = nfi_read_bytes;

	nfi->wait_ready = nfi_wait_ready;

	nfi->enable_randomizer = nfi_enable_randomizer;
	nfi->disable_randomizer = nfi_disable_randomizer;
}

static struct nfi_caps nfi_caps_v10 = {
	.max_fdm_size = 8,
	.fdm_ecc_size = 8,
	.ecc_parity_bits = 14,
	.spare_size = spare_size_v10,
	.spare_size_num = 19,
};

static struct nfi_caps *nfi_get_match_data(enum mtk_ic_version ic)
{
	switch (ic) {
	case NANDX_MT6880:
	case NANDX_MT8518:
		return &nfi_caps_v10;

	/* NOTE: add other IC's data */
	default:
		return &nfi_caps_v10;
	}
	/* NOTE: add other IC's data */
}

static void set_nfi_base_params(struct nfi_base *nb)
{
	nb->ecc_en = true;
	nb->dma_en = true;
	nb->dma_burst_en = true;

	nb->nfi_irq_en = false;
	nb->nfi_irq_all = NFI_IRQ_SLC;

	nb->randomize_en = false;
	nb->crc_en = false;

	nb->cus_sec_size_en = false;
	nb->last_seccus_size_en = false;
	nb->bad_mark_swap_en = false;

	nb->op_mode = CNFG_CUSTOM_MODE;
	nb->ecc_deccon = ECC_DEC_CORRECT;
	nb->ecc_mode = ECC_NFI_MODE;

	nb->done = nandx_event_create();
	nb->caps = nfi_get_match_data(nb->res.ic_ver);

	nb->set_op_mode = set_op_mode;
	nb->is_page_empty = is_page_empty;

	nb->rw_prepare = rw_prepare;
	nb->rw_trigger = rw_trigger;
	nb->rw_wait_done = rw_wait_done;
	nb->rw_data = rw_data;
	nb->rw_complete = rw_complete;
}

struct nfi *nfi_extend_init(struct nfi_base *nb)
{
	return &nb->nfi;
}

void nfi_extend_exit(struct nfi_base *nb)
{
	mem_free(nb);
}

struct nfi *nfi_init(struct nfi_resource *res)
{
	struct nfiecc_resource ecc_res;
	struct nfi_base *nb;
	struct nfiecc *ecc;
	struct nfi *nfi;
	int ret;

	nb = mem_alloc(1, sizeof(struct nfi_base));
	if (!nb) {
		pr_err("nfi alloc memory fail @%s.\n", __func__);
		return NULL;
	}
	nb->buf = NULL;

	nb->res = *res;

	/* fill ecc paras and init ecc */
	ecc_res.ic_ver = nb->res.ic_ver;
	ecc_res.dev = nb->res.dev;
	ecc_res.irq_id = nb->res.ecc_irq_id;
	ecc_res.regs = nb->res.ecc_regs;
	ecc = nfiecc_init(&ecc_res);
	if (!ecc) {
		pr_err("nfiecc init fail.\n");
		goto error;
	}

	nb->ecc = ecc;

	set_nfi_base_params(nb);
	set_nfi_funcs(&nb->nfi);

	/* Assign a temp sector size for reading ID & para page.
	 * We may assign new value later.
	 */
	nb->nfi.sector_size = 512;

	/* give a default timing, and as discuss
	 * this is the only thing what we need do for nfi init
	 * if need do more, then we can add a function
	 */
	writel(0x30C77FFF, nb->res.nfi_regs + NFI_ACCCON);

	if (nb->res.nand_type == NAND_SPI)
		nfi = nfi_extend_spi_init(nb);
	else
		nfi = nfi_extend_init(nb);
	if (nfi) {
		ret = nandx_irq_register(res->dev, res->nfi_irq_id,
					 nfi_irq_handler,
					 "mtk_nfi", nfi);
		if (ret) {
			pr_err("nfi irq register failed!\n");
			goto error;
		}

		return nfi;
	}

error:
	mem_free(nb);
	return NULL;
}

void nfi_exit(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);

	nandx_event_destroy(nb->done);
	nandx_irq_unregister(nb->res.nfi_irq_id);
	nfiecc_exit(nb->ecc);
	pmem_free(nb->buf);
#ifdef NANDX_TEST_BUF_ALIGN
	pmem_free(nb->buf_align);
#endif
	if (nb->res.nand_type == NAND_SPI)
		nfi_extend_spi_exit(nb);
	else
		nfi_extend_exit(nb);
}
