//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "nfiecc_regs.h"
#include "nfiecc.h"

#define NFIECC_IDLE_REG(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCIDLE : NFIECC_DECIDLE)
#define         IDLE_MASK       1
#define NFIECC_CTL_REG(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCCON : NFIECC_DECCON)
#define NFIECC_IRQ_REG(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCIRQEN : NFIECC_DECIRQEN)
#define NFIECC_ADDR(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCDIADDR : NFIECC_DECDIADDR)

#define ECC_TIMEOUT     500000

/* ecc strength that each IP supports */
static const int ecc_strength_mt6880[] = {
	4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24
};

static int nfiecc_irq_handler(void *data)
{
	struct nfiecc *ecc = data;
	void *regs = ecc->res.regs;
	u32 status;

	nandx_irq_disable(ecc->res.irq_id);
	status = readl(regs + NFIECC_DECIRQSTA) & ECC_IRQEN;
	if (status) {
		ecc->irq_status = status;
		/*status = readl(regs + NFIECC_DECDONE);
		if (!(status & ecc->config.sectors))
			return NAND_IRQ_NONE;*/

		pr_debug("%s ECC DEC irq complete status:%x\n",
			 __func__, status);
		/*
		 * Clear decode IRQ status once again to ensure that
		 * there will be no extra IRQ.
		 */
		readl(regs + NFIECC_DECIRQSTA);
		ecc->config.sectors = 0;
		nandx_event_complete(ecc->done);
	} else {
		status = readl(regs + NFIECC_ENCIRQSTA) & ECC_IRQEN;
		if (!status)
			return NAND_IRQ_NONE;
		
		ecc->irq_status = status;

		pr_debug("%s ECC ENC irq complete status:%x\n",
			 __func__, status);

		readl(regs + NFIECC_ENCIRQSTA);
		nandx_event_complete(ecc->done);
	}

	nandx_irq_enable(ecc->res.irq_id);
	return NAND_IRQ_HANDLED;
}

static inline int nfiecc_wait_idle(struct nfiecc *ecc)
{
	int op = ecc->config.op;
	int ret, val;

	ret = readl_poll_timeout_atomic(ecc->res.regs + NFIECC_IDLE_REG(op),
					val, val & IDLE_MASK,
					10, ECC_TIMEOUT);
	if (ret)
		pr_warn("%s not idle\n",
			op == ECC_ENCODE ? "encoder" : "decoder");

	return ret;
}

static inline int nfiecc_wait_irq(struct nfiecc *ecc, u32 timeout)
{
	u32 i = 0;
	int ret = 0;

	while (i++ < timeout) {
		if (ecc->irq_status) {
			pr_debug("%s IRQ done status:%x time:%duS\n",
				 __func__, ecc->irq_status, i);
			break;
		}
		udelay(1);
	}
	if (i > timeout) {
		pr_err("%s IRQ timeout error:ecc_irq_en:%d nfi_irq_rb_en:%d\n",
		       __func__, ecc->irq_en, ecc->page_irq_en);
		ret = -EIO;
	}

	return ret;
}

static int nfiecc_wait_encode_done(struct nfiecc *ecc)
{
	int ret, val;

	if (ecc->irq_en) {
		nfiecc_wait_irq(ecc, 0x10000);
		/* poll one time to avoid missing irq event */
		ret = readl_poll_timeout_atomic(ecc->res.regs + NFIECC_ENCSTA,
						val, val & ENC_FSM_IDLE, 1, 1);
		if (!ret)
			return 0;

		/* irq done, if not, we can go on to poll status for a while */
		ret = nandx_event_wait_complete(ecc->done, ECC_TIMEOUT);
		if (ret)
			return 0;
	}

	ret = readl_poll_timeout_atomic(ecc->res.regs + NFIECC_ENCSTA,
					val, val & ENC_FSM_IDLE,
					10, ECC_TIMEOUT);
	if (ret)
		pr_err("encode timeout\n");

	return ret;
}

static int nfiecc_wait_decode_done(struct nfiecc *ecc)
{
	u32 secbit = BIT(ecc->config.sectors - 1);
	void *regs = ecc->res.regs;
	int ret, val;

	if (ecc->irq_en) {
		ret = nfiecc_wait_irq(ecc, 0x10000);
		if (!ret)
			return 0;

		ret = nandx_event_wait_complete(ecc->done, ECC_TIMEOUT);
		if (ret)
			return 0;
	}

	ret = readl_poll_timeout_atomic(regs + NFIECC_DECDONE,
					val, val & secbit,
					10, ECC_TIMEOUT);
	if (ret) {
		pr_err("decode timeout\n");
		return ret;
	}

	/* decode done does not stands for ecc all work done.
	 * we need check syn, bma, chien, autoc all idle.
	 * just check it when ECC_DECCNFG[13:12] is 3,
	 * which means auto correct.
	 */
	ret = readl_poll_timeout_atomic(regs + NFIECC_DECFSM,
					val, (val & FSM_MASK) == FSM_IDLE,
					10, ECC_TIMEOUT);
	if (ret)
		pr_err("decode fsm(0x%x) is not idle\n",
			   readl(regs + NFIECC_DECFSM));

	return ret;
}

static int nfiecc_wait_done(struct nfiecc *ecc)
{
	if (ecc->config.op == ECC_ENCODE)
		return nfiecc_wait_encode_done(ecc);

	return nfiecc_wait_decode_done(ecc);
}

static void nfiecc_encode_config(struct nfiecc *ecc, u32 ecc_idx)
{
	struct nfiecc_config *config = &ecc->config;
	u32 val;

	val = ecc_idx | (config->mode << ecc->caps->ecc_mode_shift);

	if (config->mode == ECC_DMA_MODE)
		val |= ENC_BURST_EN;

	val |= (config->len << 3) << ENCCNFG_MS_SHIFT;
	writel(val, ecc->res.regs + NFIECC_ENCCNFG);
}

static void nfiecc_decode_config(struct nfiecc *ecc, u32 ecc_idx)
{
	struct nfiecc_config *config = &ecc->config;
	u32 dec_sz = (config->len << 3) +
		     config->strength * ecc->caps->parity_bits;
	u32 val;

	val = ecc_idx | (config->mode << ecc->caps->ecc_mode_shift);

	if (config->mode == ECC_DMA_MODE)
		val |= DEC_BURST_EN;

	val |= (dec_sz << DECCNFG_MS_SHIFT) |
	       (config->deccon << DEC_CON_SHIFT);
	val |= DEC_EMPTY_EN;
	writel(val, ecc->res.regs + NFIECC_DECCNFG);
}

static void nfiecc_config(struct nfiecc *ecc)
{
	u32 idx;

	if (ecc->config.mode == ECC_DMA_MODE) {
		if ((unsigned long)ecc->config.dma_addr & 0x3)
			pr_err("encode address is not 4B aligned: 0x%x\n",
			       (u32)(unsigned long)ecc->config.dma_addr);

		writel((unsigned long)ecc->config.dma_addr,
		       ecc->res.regs + NFIECC_ADDR(ecc->config.op));
	}

	for (idx = 0; idx < ecc->caps->ecc_strength_num; idx++) {
		if (ecc->config.strength == ecc->caps->ecc_strength[idx])
			break;
	}

	if (ecc->config.op == ECC_ENCODE)
		nfiecc_encode_config(ecc, idx);
	else
		nfiecc_decode_config(ecc, idx);
}

static int nfiecc_enable(struct nfiecc *ecc)
{
	enum nfiecc_operation op = ecc->config.op;
	void *regs = ecc->res.regs;

	nfiecc_config(ecc);

	writel(ECC_OP_EN, regs + NFIECC_CTL_REG(op));

	if (ecc->irq_en) {
		writel(ECC_IRQEN, regs + NFIECC_IRQ_REG(op));

		if (ecc->page_irq_en)
			writel(ECC_IRQEN | ECC_PG_IRQ_SEL,
			       regs + NFIECC_IRQ_REG(op));
		ecc->irq_status = 0;
		nandx_event_init(ecc->done);
	}

	return 0;
}

static int nfiecc_disable(struct nfiecc *ecc)
{
	enum nfiecc_operation op = ecc->config.op;
	void *regs = ecc->res.regs;

	nfiecc_wait_idle(ecc);

	writel(0, regs + NFIECC_IRQ_REG(op));
	writel(~ECC_OP_EN, regs + NFIECC_CTL_REG(op));

	return 0;
}

static int nfiecc_correct_data(struct nfiecc *ecc,
							   struct nfiecc_status *status,			
							   u8 *data, u32 sector)
{
	u32 err, offset, i;
	u32 loc, byteloc, bitloc;

	status->corrected = 0;
	status->failed = 0;

	offset = (sector >> 2);
	err = readl(ecc->res.regs + NFIECC_DECENUM(offset));
	err >>= (sector % 4) * 8;
	err &= ecc->caps->err_mask;

	if (err == ecc->caps->err_mask) {
		status->failed++;
		return -ENANDREAD;
	}

	status->corrected += err;
	status->bitflips = max(status->bitflips, err);

	for (i = 0; i < err; i++) {
		loc = readl(ecc->res.regs + NFIECC_DECEL(i >> 1));
		loc >>= ((i & 0x1) << 4);
		loc &= 0xFFFF;
		byteloc = loc >> 3;
		bitloc = loc & 0x7;
		data[byteloc] ^= (1 << bitloc);
	}

	return 0;
}

static int nfiecc_fill_data(struct nfiecc *ecc, u8 *data)
{
	struct nfiecc_config *config = &ecc->config;
	void *regs = ecc->res.regs;
	int size, ret, i;
	u32 val;

	if (config->mode != ECC_PIO_MODE)
		return 0;

	if (config->op == ECC_ENCODE)
		size = (config->len + 3) >> 2;
	else {
		size = config->strength * ecc->caps->parity_bits;
		size = (size + 7) >> 3;
		size += config->len;
		size = (size + 3) >> 2;
	}

	for (i = 0; i < size; i++) {
		ret = readl_poll_timeout_atomic(regs + NFIECC_PIO_DIRDY,
						val, val & PIO_DI_RDY,
						10, ECC_TIMEOUT);
		if (ret)
			return ret;

		writel(*((u32 *)data + i), regs + NFIECC_PIO_DI);
	}

	return 0;
}

static int nfiecc_encode(struct nfiecc *ecc, u8 *data)
{
	struct nfiecc_config *config = &ecc->config;
	u32 len, i, val = 0;
	u8 *p;
	int ret;

	/* Under NFI mode, nothing need to do */
	if (config->mode == ECC_NFI_MODE)
		return 0;

	ret = nfiecc_fill_data(ecc, data);
	if (ret)
		return ret;

	ret = nfiecc_wait_encode_done(ecc);
	if (ret)
		return ret;

	ret = nfiecc_wait_idle(ecc);
	if (ret)
		return ret;

	/* Program ECC bytes to OOB: per sector oob = FDM + ECC + SPARE */
	len = (config->strength * ecc->caps->parity_bits + 7) >> 3;
	p = data + config->len;

	/* Write the parity bytes generated by the ECC back to the OOB region */
	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			val = readl(ecc->res.regs + NFIECC_ENCPAR(i / 4));

		p[i] = (val >> ((i % 4) * 8)) & 0xff;
	}

	return 0;
}

static int nfiecc_decode(struct nfiecc *ecc, u8 *data)
{
	int ret;

	/* Under NFI mode, nothing need to do */
	if (ecc->config.mode == ECC_NFI_MODE)
		return 0;

	ret = nfiecc_fill_data(ecc, data);
	if (ret)
		return ret;

	return nfiecc_wait_decode_done(ecc);
}

static int nfiecc_decode_status(struct nfiecc *ecc, u32 start_sector,
				u32 sectors)
{
	void *regs = ecc->res.regs;
	struct nfiecc_status *status = &ecc->ecc_status;
	u32 i, val = 0, err;
	u32 bitflips = 0;

	status->corrected = 0;
	status->failed = 0;

	for (i = start_sector; i < start_sector + sectors; i++) {
		if (ecc->config.deccon == ECC_DEC_FER) {
			if (readl(regs + NFIECC_DECFER) & (1 << i)) {
				bitflips = 1;
				/* $i is not larger than $MAX_SEC_NUMS */
				ecc->last_decode_status[i] = bitflips;
			}
			continue;
		}

		if ((i % 4) == 0)
			val = readl(regs + NFIECC_DECENUM(i / 4));

		err = val >> ((i % 4) * 8);
		err &= ecc->caps->err_mask;
		ecc->last_decode_status[i] = err;

		if (err == ecc->caps->err_mask) {
			status->failed++;
			pr_debug("sector %d is uncorrect\n", i);
		} else {
			status->corrected += err;
		}

		bitflips = max(bitflips, err);
		/* if (bitflips)
			pr_debug("Corrected bitflips:%d sectors:%d start_sector:%d\n",
			bitflips, sectors, start_sector); */
	}

	if (bitflips == ecc->caps->err_mask)
		return -ENANDREAD;

	status->bitflips = bitflips;

	return bitflips;
}

static int nfiecc_adjust_strength(struct nfiecc *ecc, int strength)
{
	struct nfiecc_caps *caps = ecc->caps;
	int i, count = caps->ecc_strength_num;

	if (strength >= caps->ecc_strength[count - 1])
		return caps->ecc_strength[count - 1];

	if (strength < caps->ecc_strength[0])
		return -EINVAL;

	for (i = 1; i < count; i++) {
		if (strength < caps->ecc_strength[i])
			return caps->ecc_strength[i - 1];
	}

	return -EINVAL;
}

static int nfiecc_ctrl(struct nfiecc *ecc, int cmd, void *args)
{
	int ret = 0;

	switch (cmd) {
	case NFI_CTRL_ECC_IRQ:
		ecc->irq_en = *(bool *)args;
		pr_debug("ecc_irq_en :%d\n", ecc->irq_en);
		break;

	case NFI_CTRL_ECC_PAGE_IRQ:
		ecc->page_irq_en = *(bool *)args;
		pr_debug("ecc_page_irq_en:%d\n",
			 ecc->page_irq_en);
		break;

	case NFI_CTRL_ECC_ERRNUM0:
		*(u32 *)args = ecc->last_decode_status[0];
		break;

	case NFI_CTRL_ECC_GET_STATUS:
		memcpy(args, &ecc->ecc_status, sizeof(struct nfiecc_status));
		break;

	default:
		pr_err("cmd(%d) args(%d) not support.\n",
		       cmd, *(u32 *)args);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int nfiecc_hw_init(struct nfiecc *ecc)
{
	int ret;

	ret = nfiecc_wait_idle(ecc);
	if (ret)
		return ret;

	writel(~ECC_OP_EN, ecc->res.regs + NFIECC_ENCCON);

	ret = nfiecc_wait_idle(ecc);
	if (ret)
		return ret;

	writel(~ECC_OP_EN, ecc->res.regs + NFIECC_DECCON);

	writel(BIT(0), ecc->res.regs + NFIECC_DEBUG2);

	return 0;
}

static struct nfiecc_caps nfiecc_caps_mt6880 = {
	.err_mask = 0x1f,
	.ecc_mode_shift = 5,
	.parity_bits = 14,
	.ecc_strength = ecc_strength_mt6880,
	.ecc_strength_num = 11,
};

static struct nfiecc_caps *nfiecc_get_match_data(enum mtk_ic_version ic)
{
	/* NOTE: add other IC's data */
	return &nfiecc_caps_mt6880;
}

struct nfiecc *nfiecc_init(struct nfiecc_resource *res)
{
	struct nfiecc *ecc;
	int ret;

	ecc = mem_alloc(1, sizeof(struct nfiecc));
	if (!ecc)
		return NULL;

	ecc->res = *res;
	ecc->irq_en = false;
	ecc->page_irq_en = false;

#if 0
	ret = nandx_irq_register(res->dev, res->irq_id, nfiecc_irq_handler,
				 "mtk_ecc", ecc);
	if (ret) {
		pr_err("ecc irq register failed!\n");
		goto error;
	}
#endif

	ecc->done = nandx_event_create();
	ecc->caps = nfiecc_get_match_data(res->ic_ver);

	ecc->adjust_strength = nfiecc_adjust_strength;
	ecc->enable = nfiecc_enable;
	ecc->disable = nfiecc_disable;
	ecc->decode = nfiecc_decode;
	ecc->encode = nfiecc_encode;
	ecc->wait_done = nfiecc_wait_done;
	ecc->decode_status = nfiecc_decode_status;
	ecc->correct_data = nfiecc_correct_data;
	ecc->nfiecc_ctrl = nfiecc_ctrl;

	ret = nfiecc_hw_init(ecc);
	if (ret)
		goto error;

	return ecc;

error:
	mem_free(ecc);

	return NULL;
}

void nfiecc_exit(struct nfiecc *ecc)
{
	nandx_irq_unregister(ecc->res.irq_id);
	nandx_event_destroy(ecc->done);
	mem_free(ecc);
}

