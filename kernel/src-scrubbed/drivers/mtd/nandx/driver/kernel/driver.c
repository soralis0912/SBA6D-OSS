//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 * Author SkyLake Huang <SkyLake.Huang@mediatek.com>
 */
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include "nandx_core.h"
#include "nandx_util.h"
#include "bbt.h"
#include "nandx_platform.h"
#include "../../core/nand_chip.h"
#ifdef CONFIG_NAND_BBM
#include "bbm.h"
#include "bbm_error.h"
#endif

#define NAND_PMT_SCAN_MAXBLOCKS 4

typedef int (*func_nandx_operation)(u8 *, u8 *, u64, size_t);
static u8 *nandx_write_buffer, *nandx_read_buffer;

struct oob_page {
	struct list_head node;
	loff_t page;
};
static LIST_HEAD(oob_page_list);

#define OOB_SIZE 10
static u8 oob_buf[OOB_SIZE];
//for jffs2 only

struct nandx_clk {
	/* top_clock */
	struct clk *nfi_clk_sel;
	struct clk *snfi_clk_sel;
	struct clk *nfi_clk_parent;
	struct clk *snfi_clk_parent;
	/* infra sys*/
	struct clk *nfi_cg;
	struct clk *snfi_cg;
	struct clk *dma_cg;
};

struct nandx_nfc {
	struct nandx_info info;
	struct nandx_clk clk;
	struct nfi_resource *res;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_drive_high;

	struct mutex lock;
};

#ifdef NANDX_STORE_ATTRS_EN
	static bool nandx_store_attrs_en;
#endif

#ifdef CONFIG_MTD_NAND_MTK_WORN_BAD
static struct nandx_worn_bad_test g_worn_bad_test = {0};
#endif

static void nandx_get_device(struct mtd_info *mtd)
{
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;

	pm_runtime_get_sync(mtd->dev.parent);

	mutex_lock(&nfc->lock);
}

static void nandx_release_device(struct mtd_info *mtd)
{
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;

	mutex_unlock(&nfc->lock);

	pm_runtime_mark_last_busy(mtd->dev.parent);
	pm_runtime_put_autosuspend(mtd->dev.parent);
}

static int nandx_enable_clk(struct nandx_clk *clk, enum nand_type type)
{
	int ret;

	ret = clk_prepare_enable(clk->dma_cg);
	if (ret) {
		pr_err("failed to enable dma cg\n");
		return ret;
	}

	ret = clk_prepare_enable(clk->nfi_cg);
	if (ret) {
		pr_err("failed to enable nfi cg\n");
		return ret;
	}

	ret = clk_prepare_enable(clk->nfi_clk_sel);
	if (ret) {
		pr_err("failed to enable nfi clk sel\n");
		return ret;
	}

	if (type == NAND_SPI) {
		ret = clk_prepare_enable(clk->snfi_cg);
        if (ret) {
                pr_err("failed to enable snfi cg\n");
                return ret;
        }

        ret = clk_prepare_enable(clk->snfi_clk_sel);
        if (ret) {
                pr_err("failed to enable snfi clk sel\n");
                return ret;
        }
	}

	clk_set_parent(clk->nfi_clk_sel, clk->nfi_clk_parent);

	clk_disable_unprepare(clk->nfi_clk_sel);

	return 0;
}

static void nandx_disable_clk(struct nandx_clk *clk, enum nand_type type)
{
	clk_disable_unprepare(clk->dma_cg);
	clk_disable_unprepare(clk->nfi_cg);

	if (type == NAND_SPI)
		clk_disable_unprepare(clk->snfi_cg);
}

#ifdef CONFIG_MTD_NAND_MTK_WORN_BAD
/**
 * mtk_nfc_check_worn_bad_test - check the page addr is simulate worn
 *
 * This applies to check the page addr is simulate worn out or not.
 */
int mtk_nfc_check_worn_bad_test(void)
{
	if (g_worn_bad_test.sim_op_mode == WRITE_MODE ||
		g_worn_bad_test.sim_op_mode == ERASE_MODE)
		return g_worn_bad_test.sim_worn_page_addr;
	else
		return 0;
}
#else
int mtk_nfc_check_worn_bad_test(void)
{
	return 0;
}
#endif

#ifdef CONFIG_MTD_NAND_MTK_WORN_BAD
/**
 * mtk_nfc_set_worn_op - set worn out page and worn out mode
 * @mtd: MTD device structure
 * @ofs: offset in a partition.
 * @op_mode: worn out operation mode which is write mode or erase mode.
 *
 * This applies to simulate worn bad test for erase and program only.
 */
static int mtk_nfc_set_worn_op(struct mtd_info *mtd, loff_t ofs,
							   u32 op_mode)
{
	struct mtd_info *master_mtd = mtd_get_part_master(mtd);
	struct nandx_nfc *nfc = (struct nandx_nfc *)master_mtd->priv;
	u32 page, page_shift;
	u64 offset;

	if (ofs < 0 || ofs >= mtd->size)
		return -EINVAL;

	page_shift = ffs((u32)nfc->info.page_size) - 1;
	offset = mtd_get_part_offset(mtd);
	page = (offset >> page_shift) + (ofs >> page_shift);

	g_worn_bad_test.sim_worn_page_addr = page;
	g_worn_bad_test.sim_op_mode = op_mode;

	return 0;
}

int mtk_nfc_set_write_worn_page(struct mtd_info *mtd, loff_t ofs)
{
	return mtk_nfc_set_worn_op(mtd, ofs, WRITE_MODE);
}
EXPORT_SYMBOL_GPL(mtk_nfc_set_write_worn_page);

int mtk_nfc_set_erase_worn_block(struct mtd_info *mtd, loff_t ofs)
{
	return mtk_nfc_set_worn_op(mtd, ofs, ERASE_MODE);
}
EXPORT_SYMBOL_GPL(mtk_nfc_set_erase_worn_block);
#endif

static int nand_mark_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct nandx_nfc *nfc;
	int ret;

	nfc = (struct nandx_nfc *)mtd->priv;
	nandx_get_device(mtd);
#ifdef CONFIG_NAND_BBM
	ret = bbm_mark_bad(ofs);
#else
	ret = bbt_mark_bad(&nfc->info, ofs);
#endif
	nandx_release_device(mtd);

	return ret;
}

static int nand_unmark_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct nandx_nfc *nfc;
	int ret;

	nfc = (struct nandx_nfc *)mtd->priv;
	nandx_get_device(mtd);
	ret = nand_unmarkbad_bbt(&nfc->info, ofs);
	(void)nandx_erase(ofs, nfc->info.block_size);
	nandx_release_device(mtd);

	return ret;
}

static int mtk_nfc_ooblayout_free(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *oob_region)
{
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;
	u32 eccsteps;

	eccsteps = div_down(mtd->writesize, mtd->ecc_step_size);

	if (section >= eccsteps)
		return -EINVAL;

	oob_region->length = mtd->oobavail / eccsteps;
	oob_region->offset =
		(section + 1) * nfc->info.fdm_reg_size - oob_region->length;

	return 0;
}

static int mtk_nfc_ooblayout_ecc(struct mtd_info *mtd, int section,
				 struct mtd_oob_region *oob_region)
{
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;
	u32 eccsteps;

	if (section)
		return -EINVAL;

	eccsteps = div_down(mtd->writesize, mtd->ecc_step_size);
	oob_region->offset = nfc->info.fdm_reg_size * eccsteps;
	oob_region->length = mtd->oobsize - oob_region->offset;

	return 0;
}

static const struct mtd_ooblayout_ops mtk_nfc_ooblayout_ops = {
	.free = mtk_nfc_ooblayout_free,
	.ecc = mtk_nfc_ooblayout_ecc,
};

struct nfc_compatible {
	enum mtk_ic_version ic_ver;
};

static const struct nfc_compatible nfc_compats_mt6880 = {
	.ic_ver = NANDX_MT6880,
};

static const struct of_device_id ic_of_match[] = {
	{.compatible = "mediatek,mt6880-nfc", .data = &nfc_compats_mt6880},
	{}
};

static const char *const part_types[] = {"pmtpart", "ofpart", NULL};

static int nand_operation(struct mtd_info *mtd, loff_t addr, size_t len,
			  size_t *retlen, uint8_t *data, uint8_t *oob, bool read)
{
	struct nandx_split64 split = {0};
	func_nandx_operation operation;
	struct chip_status status;
	u64 block_oobs, val, align;
	uint8_t *databuf, *oobbuf;
	struct nandx_nfc *nfc;
	bool readoob;
	int ret = 0, ret_min = 0, ret_max = 0;

	nfc = (struct nandx_nfc *)mtd->priv;

	databuf = data;
	oobbuf = oob;

	readoob = data ? false : true;
	block_oobs = div_up(mtd->erasesize, mtd->writesize) * mtd->oobavail;
	align = readoob ? block_oobs : mtd->erasesize;

#ifdef CONFIG_NAND_BBM
	operation = read ? bbm_read : bbm_write;
#else
	operation = read ? nandx_read : nandx_write;
#endif

	nandx_split(&split, addr, len, val, align);

	if (split.head_len) {
		ret = operation((u8 *) databuf, oobbuf, addr, split.head_len);
		if (ret < 0) {
			pr_err("%s %d: read %d, ret %d\n", __func__,
			       __LINE__, read, ret);
			if (!read) {
				if (ret == -ENANDWRITE)
					ret = -EIO;

				return ret;
			}

			if (read && (ret == -ENANDREAD))
				ret = -EBADMSG;
		}

		if (read) {
			nandx_ioctl(CHIP_CTRL_GET_STATUS, &status);
			mtd->ecc_stats.corrected += status.corrected;
			mtd->ecc_stats.failed += status.failed;
		}

		ret_min = min_t(int, ret_min, ret);
		ret_max = max_t(int, ret_max, ret);

		if (databuf)
			databuf += split.head_len;

		if (oobbuf)
			oobbuf += split.head_len;

		addr += split.head_len;
		*retlen += split.head_len;
	}

	if (split.body_len) {
		while (div_up(split.body_len, align)) {
			ret = operation((u8 *) databuf, oobbuf, addr, align);
			if (ret < 0) {
				pr_err("%s %d: read %d, ret %d\n", __func__,
				       __LINE__, read, ret);
				if (!read) {
					if (ret == -ENANDWRITE)
						ret = -EIO;

					return ret;
				}

				if (read && (ret == -ENANDREAD))
					ret = -EBADMSG;
			}

			if (read) {
				nandx_ioctl(CHIP_CTRL_GET_STATUS, &status);
				mtd->ecc_stats.corrected += status.corrected;
				mtd->ecc_stats.failed += status.failed;
			}

			ret_min = min_t(int, ret_min, ret);
			ret_max = max_t(int, ret_max, ret);

			if (databuf) {
				databuf += mtd->erasesize;
				split.body_len -= mtd->erasesize;
				*retlen += mtd->erasesize;
			}

			if (oobbuf) {
				oobbuf += block_oobs;
				split.body_len -= block_oobs;
			}

			addr += mtd->erasesize;
		}

	}

	if (split.tail_len) {
		ret = operation((u8 *) databuf, oobbuf, addr, split.tail_len);
		if (ret < 0) {
			pr_err("%s %d: read %d, ret %d\n", __func__,
			       __LINE__, read, ret);
			if (!read) {
				if (ret == -ENANDWRITE)
					ret = -EIO;

				return ret;
			}

			if (read && (ret == -ENANDREAD))
				ret = -EBADMSG;
		}

		if (read) {
			nandx_ioctl(CHIP_CTRL_GET_STATUS, &status);
			mtd->ecc_stats.corrected += status.corrected;
			mtd->ecc_stats.failed += status.failed;
		}

		ret_min = min_t(int, ret_min, ret);
		ret_max = max_t(int, ret_max, ret);
		*retlen += split.tail_len;
	}

	return ret_min < 0 ? ret_min : ret_max;
}

static int nand_read(struct mtd_info *mtd, loff_t from, size_t len,
					 size_t *retlen, uint8_t *buf)
{
	int ret;

	nandx_get_device(mtd);
	ret = nand_operation(mtd, from, len, retlen, buf, NULL, true);
	if (ret >= (int)mtd->bitflip_threshold)
		pr_err("read from: 0x%llx, len: %ld, bitflips: %d exceed threshold %d\n",
			from, len, ret, mtd->bitflip_threshold);

	nandx_release_device(mtd);
	return ret;
}

static int nand_write(struct mtd_info *mtd, loff_t to, size_t len,
					  size_t *retlen, const uint8_t *buf)
{
	int ret;

	nandx_get_device(mtd);
	ret = nand_operation(mtd, to, len, retlen, (uint8_t *)buf,
						 NULL, false);
	nandx_release_device(mtd);
	return ret;
}

static void nand_copy_from_oob(struct mtd_info *mtd, u8 *dst, u8 *src, u8 len)
{
	u32 eccsteps, gap;
	u32 oobavail_per_step, oobsize_per_step;
	int i;

	eccsteps = div_down(mtd->writesize, mtd->ecc_step_size);
	gap = (mtd->oobsize - mtd->oobavail) / eccsteps;
	oobsize_per_step = mtd->oobsize / eccsteps;
	oobavail_per_step = mtd->oobavail / eccsteps;

	pr_debug("from eccsteps %d, gap %d, oobsize_per_step %d, oobavail_per_step %d, len %d\n",
			eccsteps, gap, oobsize_per_step, oobavail_per_step, len);
	memset(dst, 0xFF, len);

	for (i = 0; i < eccsteps ; i++) {
		int copylen = (len < oobavail_per_step) ? len : oobavail_per_step;
		memcpy(dst + i * oobavail_per_step , src + i * oobsize_per_step + gap, copylen);
		len -= copylen;
		if(len == 0)
			break;
	}
}

static void nand_copy_to_oob(struct mtd_info *mtd, u8 *dst, u8 *src, u8 len)
{
	u32 eccsteps, gap;
	u32 oobavail_per_step, oobsize_per_step;
	int i;

	eccsteps = div_down(mtd->writesize, mtd->ecc_step_size);
	gap = (mtd->oobsize - mtd->oobavail) / eccsteps;
	oobsize_per_step = mtd->oobsize / eccsteps;
	oobavail_per_step = mtd->oobavail / eccsteps;

	pr_debug("to eccsteps %d, gap %d, oobsize_per_step %d, oobavail_per_step %d, len %d\n",
			eccsteps, gap, oobsize_per_step, oobavail_per_step, len);
	memset(dst, 0xFF, mtd->oobsize);

	for (i = 0; i < eccsteps ; i++) {
		int copylen = (len < oobavail_per_step) ? len : oobavail_per_step;
		memcpy(dst + i * oobsize_per_step + gap, src + i * oobavail_per_step, copylen);
		len -= copylen;
		if(len == 0)
			break;
	}
}

int nand_read_oob(struct mtd_info *mtd, loff_t from,
				  struct mtd_oob_ops *ops)
{
	int ret, i;
	size_t retlen = 0;
	u32 loop = div_up(ops->ooblen, mtd->oobavail);

	if (ops->oobbuf == NULL)
		return nand_read(mtd, from, ops->len, &ops->retlen, ops->datbuf);

	if (nandx_read_buffer == NULL)
		nandx_read_buffer = kmalloc(2*mtd->writesize, GFP_KERNEL);
	if (nandx_read_buffer == NULL)
		return -ENOMEM;

	pr_debug("from: 0x%llX read datbuf %p oobbuf %p len %ld, ooblen %ld\n",
		from, ops->datbuf, ops->oobbuf, ops->len, ops->ooblen);

	nandx_get_device(mtd);

	if (ops->datbuf == NULL) {
		int len = ops->ooblen;

		for (i = 0 ;i < loop; i++) {
			int copylen = (len < mtd->oobavail)? len : mtd->oobavail;
			ret = nand_operation(mtd, from, mtd->writesize, &retlen, nandx_read_buffer,
				nandx_read_buffer + mtd->writesize, true);
			if (ret < 0) {
				pr_err("retlen %ld, ret %d\n", retlen, ret);
				goto read_oob_end;
			}
			pr_debug("%d len %d, OOB [%08x, %08x, %08x, %08x]\n",
				i, copylen,
				*(u32 *)nandx_read_buffer+mtd->writesize + 0,
				*(u32 *)nandx_read_buffer+mtd->writesize + 4,
				*(u32 *)nandx_read_buffer+mtd->writesize + 8,
				*(u32 *)nandx_read_buffer+mtd->writesize + 12);
			nand_copy_from_oob(mtd, ops->oobbuf + i * mtd->oobavail,
					   nandx_read_buffer + mtd->writesize, copylen);
			len -= copylen;
			ops->oobretlen += copylen;
		}
		goto read_oob_end;
	} else {
		memset(nandx_read_buffer + mtd->writesize, 0xFF, mtd->oobsize);
		ret = nand_operation(mtd, from, mtd->writesize, &ops->retlen, ops->datbuf,
			nandx_read_buffer + mtd->writesize, true);
		if (ret < 0) {
			pr_err("retlen %ld, ret %d\n", retlen, ret);
			goto read_oob_end;
		}
		nand_copy_from_oob(mtd, ops->oobbuf, nandx_read_buffer + mtd->writesize, ops->ooblen);
		ops->oobretlen += ops->ooblen;
	}
	dump_stack();

read_oob_end:
	nandx_release_device(mtd);
	return ret;
}

int nand_write_oob(struct mtd_info *mtd, loff_t to,
				   struct mtd_oob_ops *ops)
{
	struct oob_page *opage, *n;
	bool found = false;
	int ret;
	u8 *oobbuf = ops->oobbuf;

	nandx_get_device(mtd);

	if (nandx_write_buffer == NULL) {
		nandx_write_buffer = kmalloc(2*mtd->writesize, GFP_KERNEL);
		if (nandx_write_buffer == NULL)
			return -ENOMEM;
	}

	if (ops->datbuf) {
		if (oobbuf) {
			nand_copy_to_oob(mtd, nandx_write_buffer, ops->oobbuf, ops->ooblen);
			oobbuf = nandx_write_buffer;
		} else {
			list_for_each_entry_safe(opage, n, &oob_page_list, node) {
				if (opage->page == to) {
					nand_copy_to_oob(mtd, nandx_write_buffer,
							 &oob_buf[0], OOB_SIZE);
					oobbuf = nandx_write_buffer;
					list_del_init(&opage->node);
					mem_free(opage);
					break;
				}
			}
		}

		ret = nand_operation(mtd, to,  ops->len, &ops->retlen, ops->datbuf, oobbuf, false);
	} else if (ops->oobbuf) {
		pr_debug("write datbuf %p oobbuf %p ooboffs %d, len %ld, ooblen %ld @0x%llX\n",
			ops->datbuf, ops->oobbuf, ops->ooboffs, ops->len, ops->ooblen, to);

		if(ops->ooblen > OOB_SIZE)
			BUG();

		memset(&oob_buf[0], 0xFF, OOB_SIZE);
		memcpy(&oob_buf[0], ops->oobbuf, ops->ooblen);

		list_for_each_entry(opage, &oob_page_list, node) {
			if (opage->page == to) {
				found = true;
				break;
			}
		}

		if (!found) {
			opage = mem_alloc(1, sizeof(struct oob_page));
			if (!opage)
				return -ENOMEM;
			opage->page = to;
			list_add_tail(&opage->node, &oob_page_list);
		}

		ops->oobretlen = ops->ooblen;
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	nandx_release_device(mtd);
	return ret;
}


#ifdef CONFIG_NAND_BBM
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct nandx_nfc *nfc;
	u32 block_size;
	int ret = 0;

	nandx_get_device(mtd);

	nfc = (struct nandx_nfc *)mtd->priv;
	block_size = nfc->info.block_size;

	while (instr->len) {
		if (bbm_is_bad(instr->addr)) {
			pr_err("block(0x%llx) is bad, not erase\n",
				instr->addr);
			ret = -EIO;
			goto erase_exit;
		} else {
			ret = bbm_erase(instr->addr, block_size);
			if (ret == -E_OUT_OF_RANGE)
				break;

			if (ret < 0) {
				ret = -EIO;
				goto erase_exit;
				pr_err("erase fail at blk %llu, ret:%d\n",
					instr->addr, ret);
			}
		}
		instr->addr += block_size;
		instr->len -= block_size;
	}

	ret = 0;

erase_exit:
	nandx_release_device(mtd);

	return ret;
}
#else
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct nandx_nfc *nfc;
	u32 block_size;
	int ret = 0;

	nandx_get_device(mtd);

	nfc = (struct nandx_nfc *)mtd->priv;
	block_size = nfc->info.block_size;

	while (instr->len) {
		if (bbt_is_bad(&nfc->info, instr->addr)) {
			pr_err("block(0x%llx) is bad, not erase\n",
				instr->addr);
			ret = -EIO;
			goto erase_exit;
		} else {
			ret = nandx_erase(instr->addr, block_size);
			if (ret < 0) {
				ret = -EIO;
				goto erase_exit;
				pr_err("erase fail at blk %llu, ret:%d\n",
					instr->addr, ret);
			}
		}
		instr->addr += block_size;
		instr->len -= block_size;
	}

	ret = 0;

erase_exit:
	nandx_release_device(mtd);

	return ret;
}
#endif

int nand_is_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct nandx_nfc *nfc;
	int ret;

	nfc = (struct nandx_nfc *)mtd->priv;
	nandx_get_device(mtd);
#ifdef CONFIG_NAND_BBM
	ret = bbm_is_bad(ofs);
#else
	ret = bbt_is_bad(&nfc->info, ofs);
#endif
	nandx_release_device(mtd);

	return ret;
}

void nand_sync(struct mtd_info *mtd)
{
	nandx_get_device(mtd);
	nandx_sync();
	nandx_release_device(mtd);
}

static struct mtd_info *mtd_info_create(struct platform_device *pdev,
				struct nandx_nfc *nfc)
{
	struct mtd_info *mtd;
	int ret;

	mtd = mem_alloc(1, sizeof(struct mtd_info));
	if (!mtd)
		return NULL;

	ret = nandx_ioctl(CORE_CTRL_NAND_INFO, &nfc->info);
	if (ret) {
		pr_err("fail to get nand info (%d)!\n", ret);
		mem_free(mtd);
		return NULL;
	}

	mtd->priv = nfc;
	mtd->owner = THIS_MODULE;
	mtd->dev.parent = &pdev->dev;
	mtd->name = "MTK-Nand";
	mtd->writesize = nfc->info.page_size;
	mtd->erasesize = nfc->info.block_size;
	mtd->oobsize = nfc->info.oob_size;
	mtd->size = nfc->info.total_size;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->_erase = nand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read_oob = nand_read_oob;
	mtd->_write_oob = nand_write_oob;
	mtd->_sync = nand_sync;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_block_isbad = nand_is_bad;
	mtd->_block_markbad = nand_mark_bad;
	mtd->writebufsize = mtd->writesize;
	mtd->oobavail = 2 * (mtd->writesize / nfc->info.sector_size);

	mtd_set_ooblayout(mtd, &mtk_nfc_ooblayout_ops);

	mtd->ecc_strength = nfc->info.ecc_strength;
	mtd->ecc_step_size = nfc->info.sector_size;

	if (!mtd->bitflip_threshold)
		mtd->bitflip_threshold = (mtd->ecc_strength * 3) / 4;

	return mtd;
}

static int get_platform_res(struct platform_device *pdev,
			struct nandx_nfc *nfc, enum nand_type type)
{
	void __iomem *nfi_base, *ecc_base;
	const struct of_device_id *of_id;
	struct nfc_compatible *compat;
	struct nfi_resource *res;
	u32 nfi_irq, ecc_irq;
	struct device *dev;
	int ret = 0;

	res = mem_alloc(1, sizeof(struct nfi_resource));
	if (!res)
		return -ENOMEM;

	nfc->res = res;
	dev = &pdev->dev;

	nfi_base = of_iomap(dev->of_node, 0);
	ecc_base = of_iomap(dev->of_node, 1);
	nfi_irq = irq_of_parse_and_map(dev->of_node, 0);
	ecc_irq = irq_of_parse_and_map(dev->of_node, 1);

	of_id = of_match_node(ic_of_match, pdev->dev.of_node);
	if (!of_id) {
		ret = -EINVAL;
		goto freeres;
	}
	compat = (struct nfc_compatible *)of_id->data;

#if 0
	nfc->pinctrl = devm_pinctrl_get(dev);
	nfc->pins_drive_high = pinctrl_lookup_state(nfc->pinctrl,
						    "state_drive_high");
#endif

	nfc->clk.dma_cg = devm_clk_get(dev, "dma_cg");
	if (IS_ERR(nfc->clk.dma_cg)) {
		pr_err("no dma cg\n");
		ret = -EINVAL;
		goto freeres;
	}

	nfc->clk.nfi_cg = devm_clk_get(dev, "nfi_cg");
	if (IS_ERR(nfc->clk.nfi_cg)) {
		pr_err("no nfi cg\n");
		ret = -EINVAL;
		goto freeres;
	}

	if (type == NAND_SPI) {
		nfc->clk.snfi_cg = devm_clk_get(dev, "snfi_cg");
		if (IS_ERR(nfc->clk.snfi_cg)) {
			 pr_err("no snfi cg\n");
			 ret = -EINVAL;
			goto freeres;
		}
		nfc->clk.snfi_clk_sel = devm_clk_get(dev, "snfi_clk_sel");
		if (IS_ERR(nfc->clk.snfi_clk_sel)) {
			pr_err("no snfi clk sel\n");
			ret = -EINVAL;
			goto freeres;
		}
	}

	nfc->clk.nfi_clk_sel = devm_clk_get(dev, "nfi_clk_sel");
	if (IS_ERR(nfc->clk.nfi_clk_sel)) {
		pr_err("no nfi clk sel\n");
		ret = -EINVAL;
		goto freeres;
	}

	if (type == NAND_SPI)
		nfc->clk.nfi_clk_parent = devm_clk_get(dev, "snfi_parent78m");
	else
		nfc->clk.nfi_clk_parent = devm_clk_get(dev, "nfi_parent156m");
	if (IS_ERR(nfc->clk.nfi_clk_parent)) {
		pr_err("no nfi_clk_parent\n");
		ret = -EINVAL;
		goto freeres;
	}

	nand_get_resource(res);

	res->ic_ver = (enum mtk_ic_version)(compat->ic_ver);
	res->nfi_regs = (void *)nfi_base;
	res->nfi_irq_id = nfi_irq;
	res->ecc_regs = (void *)ecc_base;
	res->ecc_irq_id = ecc_irq;
	res->dev = dev;

	return ret;

freeres:
	mem_free(res);

	return ret;
}

static ssize_t nand_ids_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;

	nfc = (struct nandx_nfc *)mtd->priv;

	return snprintf(buf, PAGE_SIZE, "%llx\n", nfc->info.ids);
}
static DEVICE_ATTR(nand_ids, 0444, nand_ids_show, NULL);

static ssize_t nand_name_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;

	nfc = (struct nandx_nfc *)mtd->priv;
	return snprintf(buf, strlen(nfc->info.nand_name) + 2, "%s\n", nfc->info.nand_name);
}
static DEVICE_ATTR(nand_name, 0444, nand_name_show, NULL);

static ssize_t bbt_goodblocks_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;

	nfc = (struct nandx_nfc *)mtd->priv;

	return snprintf(buf, PAGE_SIZE, "%x\n", nfc->info.bbt_goodblocks);
}
static DEVICE_ATTR(bbt_goodblocks, 0444, bbt_goodblocks_show, NULL);

static ssize_t bb_total_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;
	u32 bb_worn = 0, bb_factory = 0;

	nfc = (struct nandx_nfc *)mtd->priv;

	get_bad_block(&nfc->info, &bb_worn, &bb_factory, NULL);

	return snprintf(buf, PAGE_SIZE, "%d\n", bb_worn + bb_factory);
}
static DEVICE_ATTR(bb_total, 0444, bb_total_show, NULL);

static ssize_t bbt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;

	nfc = (struct nandx_nfc *)mtd->priv;

	return get_bad_block(&nfc->info, NULL, NULL, buf);
}
static DEVICE_ATTR(bbtshow, 0444, bbt_show, NULL);

static ssize_t bb_worn_total_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;
	u32 bb_worn = 0;

	nfc = (struct nandx_nfc *)mtd->priv;

	get_bad_block(&nfc->info, &bb_worn, NULL, NULL);

	return snprintf(buf, PAGE_SIZE, "%d\n", bb_worn);
}
static DEVICE_ATTR(bb_worn_total, 0444, bb_worn_total_show, NULL);

static ssize_t bb_factory_total_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;
	u32 bb_factory = 0;

	nfc = (struct nandx_nfc *)mtd->priv;

	get_bad_block(&nfc->info, NULL, &bb_factory, NULL);

	return snprintf(buf, PAGE_SIZE, "%d\n", bb_factory);
}
static DEVICE_ATTR(bb_factory_total, 0444, bb_factory_total_show, NULL);

/* Currently, nand unique id's max lenghth is 16 bytes,
 * the valid length is 8 bytes, but this is not fixed invariance.
 */
#define NAND_UNIQUE_ID_MAX_LEN 16

/*
 * read nand's unique id
 *
 * $uid, the buffer to store unique id.
 * $uid_len, the value of $uid_len should be less than 16.
 *
 * return 0 if sucess, or reuturn error code.
 */
int nand_read_unique_id(u8 *uid, int uid_len)
{
	struct nand_unique_id nuid;
	int ret, i;

	if (uid_len > NAND_UNIQUE_ID_MAX_LEN)
		return -EINVAL;

	nuid.uid_len = 2 * NAND_UNIQUE_ID_MAX_LEN;
	nuid.uid = mem_alloc(1, nuid.uid_len);
	if (nuid.uid == NULL)
		return -ENOMEM;

	ret = nandx_ioctl(CHIP_CTRL_READ_UNIQUE_ID, &nuid);
	if (ret < 0)
		goto out;

	for (i = 0; i < NAND_UNIQUE_ID_MAX_LEN; i++) {
		if ((nuid.uid[i] ^ nuid.uid[NAND_UNIQUE_ID_MAX_LEN + i])
			!= 0xFF) {
			pr_err("index: %d <0x%x - 0x%x>\n", i, nuid.uid[i],
				nuid.uid[NAND_UNIQUE_ID_MAX_LEN]);
			ret = -EIO;
			goto out;
		}
	}

	memcpy(uid, nuid.uid, uid_len);

out:
	mem_free(nuid.uid);
	return ret;
}

static ssize_t nand_unique_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	u8 nuid[NAND_UNIQUE_ID_MAX_LEN];
	ssize_t len = 0;
	int i;
	int ret;

	nandx_get_device(mtd);
	ret = nand_read_unique_id(nuid, NAND_UNIQUE_ID_MAX_LEN);
	nandx_release_device(mtd);

	if (ret >= 0) {
		for (i = 0; i < NAND_UNIQUE_ID_MAX_LEN; i++)
			len += snprintf(buf + len, PAGE_SIZE - len, " 0x%x", nuid[i]);
	} else {
		pr_err("nand read unique id fail with ret(%d).\n", ret);
	}

	return len;
}

static DEVICE_ATTR(nand_unique_id, 0444, nand_unique_id_show, NULL);

static ssize_t nand_unit_size_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;
	ssize_t len = 0;

	len += snprintf(buf, PAGE_SIZE, "chip size\t: 0x%llx\n",
		       nfc->info.total_size);
	len += snprintf(buf + len, PAGE_SIZE - len, "block size\t: 0x%x\n",
			nfc->info.block_size);
	len += snprintf(buf + len, PAGE_SIZE - len, "page size\t: 0x%x\n",
			nfc->info.page_size);
	len += snprintf(buf + len, PAGE_SIZE - len, "sector size\t: 0x%x\n",
			nfc->info.sector_size);
	len += snprintf(buf + len, PAGE_SIZE - len, "oob size\t: 0x%x\n",
			nfc->info.oob_size);
	len += snprintf(buf + len, PAGE_SIZE - len, "ecc level\t: 0x%x\n",
			nfc->info.ecc_strength);

	return len;
}

static DEVICE_ATTR_RO(nand_unit_size);

#ifdef NANDX_STORE_ATTRS_EN
static ssize_t bb_mark_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf,
			size_t count)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;
	unsigned int block;
	int ret;

	if (!nandx_store_attrs_en)
		return -EOPNOTSUPP;

	ret = kstrtouint(buf, 0, &block);
	if (ret) {
		pr_err("error: invalid argument!!!\n");
		return count;
	}

	(void)nand_mark_bad(mtd, (loff_t)block * nfc->info.block_size);

	return count;
}
static DEVICE_ATTR_WO(bb_mark);

static ssize_t bb_unmark_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf,
			 size_t count)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;
	unsigned int block;
	int ret;

	if (!nandx_store_attrs_en)
		return -EOPNOTSUPP;

	ret = kstrtouint(buf, 0, &block);
	if (ret) {
		pr_err("error: invalid argument!!!\n");
		return count;
	}

	(void)nand_unmark_bad(mtd, (loff_t)block * nfc->info.block_size);

	return count;
}
static DEVICE_ATTR_WO(bb_unmark);

static ssize_t nandx_store_attrs_en_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf,
			 size_t count)
{
	const char *pword = "6D6763";

	if (!strncmp(buf, pword, strlen(pword)))
		nandx_store_attrs_en = !nandx_store_attrs_en;

	pr_info("nandx store attrs enable: %s\n",
		nandx_store_attrs_en ? "true" : "false");

	return count;
}
static DEVICE_ATTR_WO(nandx_store_attrs_en);
#endif /* end of NANDX_STORE_ATTRS_EN */

static struct attribute *mtk_nand_attrs[] = {
	&dev_attr_nand_name.attr,
	&dev_attr_nand_ids.attr,
	&dev_attr_bbt_goodblocks.attr,
	&dev_attr_bb_total.attr,
	&dev_attr_bbtshow.attr,
	&dev_attr_bb_worn_total.attr,
	&dev_attr_bb_factory_total.attr,
	&dev_attr_nand_unique_id.attr,
	&dev_attr_nand_unit_size.attr,
#ifdef NANDX_STORE_ATTRS_EN
	&dev_attr_bb_mark.attr,
	&dev_attr_bb_unmark.attr,
	&dev_attr_nandx_store_attrs_en.attr,
#endif
	NULL,
};

static const struct attribute_group mtk_nand_attr_group = {
	.attrs = mtk_nand_attrs,
};

static int get_nand_type_by_trap(struct platform_device *pdev, enum nand_type *type)
{
	struct device_node *node;
	u32 trap_base, trap_mask;
	u32 trap_list[2], trap, trap_count;
	const char *trap_name_list[2];
	int i;
	int ret;

	node = of_get_child_by_name(pdev->dev.of_node, "nand-trap");
	if (node == NULL) {
		/* if no nand-trap information, set NAND_SLC as default */
		*type = NAND_SLC;
		return 0;
	}

	ret = of_property_read_u32(node, "trap-base", &(trap_base));
	if (ret)
		return ret;

	ret = of_property_read_u32(node, "trap-mask", &(trap_mask));
	if(ret)
		return ret;

	ret = of_property_read_u32_array(node, "trap-list", trap_list, 2);
	if(ret)
		return ret;

	trap_count = of_property_read_string_array(node, "trap-name-list", trap_name_list, 2);
	if (trap_count < 2)
		return trap_count;

	trap = readl(ioremap(trap_base, 4)) & trap_mask;
	dev_info(&pdev->dev, "nand trap :%d\n", trap);
	for (i = 0; i < 2; i++) {
			if (trap == (trap_list[i]))
				break;
	}

	if (!strncmp(trap_name_list[i], "snand", 5)) {
		*type = NAND_SPI;
		dev_info(&pdev->dev, "NAND_SPI\n");
	} else {
		*type = NAND_SLC;
		dev_info(&pdev->dev, "NAND_SLC\n");
	}

	return ret;
}

#ifdef CONFIG_NAND_BBM
struct nandx_info g_nandi;
static int bbm_nand_block_is_bad(u64 offset)
{
        return bbt_is_bad(&g_nandi, offset);
}

static int bbm_nand_mark_bad(u64 offset)
{
        return bbt_mark_bad(&g_nandi, offset);
}

struct bbm_nand bbm_nand_dev = {
	.read = nandx_read,
	.write = nandx_write,
	.erase = nandx_erase,
	.is_bad = bbm_nand_block_is_bad,
	.mark_bad = bbm_nand_mark_bad,
};
#endif

static int nand_probe(struct platform_device *pdev)
{
	struct mtd_info *mtd;
	struct nandx_nfc *nfc;
	enum nand_type type;
	int arg;
	int ret;

	nfc = mem_alloc(1, sizeof(struct nandx_nfc));
	if (!nfc)
		return -ENOMEM;

	ret = get_nand_type_by_trap(pdev, &type);
	if (ret)
		goto release_nfc;

	ret = get_platform_res(pdev, nfc, type);
	if (ret)
		goto release_nfc;

	nfc->res->min_oob_req = 32;
	nfc->res->nand_type = type;

	ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
	if(ret) {
		pr_err("fail to set dma mask (%d)!\n", ret);
		goto release_res;
	}

	ret = nandx_enable_clk(&nfc->clk, nfc->res->nand_type);
	if (ret)
		goto release_res;

	ret = nandx_init(nfc->res);
	if (ret) {
		pr_err("nandx init error (%d)!\n", ret);
		goto disable_clk;
	}

	/*if (!IS_ERR(nfc->pinctrl) && !IS_ERR(nfc->pins_drive_high)) {
		clk_set_parent(nfc->clk.snfi_clk_sel, nfc->clk.snfi_parent_52m);
		clk_set_parent(nfc->clk.nfi2x_clk_sel,
			       nfc->clk.nfi2x_clk_parent);
		pinctrl_select_state(nfc->pinctrl, nfc->pins_drive_high);

		arg = 3;
		ret = nandx_ioctl(SNFI_CTRL_DELAY_MODE, &arg);
		if (ret)
			goto release_res;
	}*/
	arg = 0xf;
	nandx_ioctl(NFI_CTRL_IOCON, &arg);

	arg = 1;
	nandx_ioctl(NFI_CTRL_DMA, &arg);
	nandx_ioctl(NFI_CTRL_ECC, &arg);
	nandx_ioctl(NFI_CTRL_BAD_MARK_SWAP, &arg);

	mtd = mtd_info_create(pdev, nfc);
	if (!mtd) {
		ret = -ENOMEM;
		goto disable_clk;
	}

	mutex_init(&nfc->lock);

	ret = scan_bbt(&nfc->info);
	if (ret) {
		pr_err("bbt init error (%d)!\n", ret);
		goto release_mtd;
	}

	platform_set_drvdata(pdev, mtd);
	mtd->priv = nfc;

#ifdef CONFIG_NAND_BBM
	g_nandi = nfc->info;
	bbm_nand_dev.blocks = div_down(nfc->info.total_size, nfc->info.block_size);
	bbm_nand_dev.page_size = nfc->info.page_size;
	bbm_nand_dev.oob_size = nfc->info.oob_size;
	bbm_nand_dev.bs_shift = log2(nfc->info.block_size);
	bbm_nand_dev.bbt_blocks = NAND_BBT_SCAN_MAXBLOCKS;
	bbm_nand_dev.part_blocks = NAND_PMT_SCAN_MAXBLOCKS;
	ret = bbm_init(&bbm_nand_dev, false, false);

	if (ret) {
		pr_err("bbm init error (%d)!\n", ret);
		goto release_mtd;
	}
#endif

	ret = mtd_device_parse_register(mtd, part_types, NULL, NULL, 0);
	if (ret) {
		pr_err("mtd parse partition error! ret:%d\n", ret);
		mtd_device_unregister(mtd);
		goto release_mtd;
	}

	get_bbt_goodblocks_num(&nfc->info);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	/* Add device attribute groups */
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_nand_attr_group);
	if (ret) {
		pr_info("failed to create attribute group\n");
		goto release_mtd;
	}

#if 0
	buf = mem_alloc(MAX_OOB_KEEP, OOB_SIZE);
	if(buf == NULL) {
		int i;

		for(i=0;i<MAX_OOB_KEEP;i++)
			oob_buf[i] = buf + (i*OOB_SIZE);
	} else {
		pr_err("alloc oob_buf fail\n");
		return -ENOMEM;
	}
#endif

	return 0;

release_mtd:
	mem_free(mtd);
disable_clk:
	pm_runtime_disable(&pdev->dev);
	nandx_disable_clk(&nfc->clk, nfc->res->nand_type);
release_res:
	mem_free(nfc->res);
release_nfc:
	mem_free(nfc);

	pr_err("%s: probe err %d\n", __func__, ret);
	return ret;
}

static int nand_remove(struct platform_device *pdev)
{
	struct mtd_info *mtd = platform_get_drvdata(pdev);
	struct nandx_nfc *nfc;

	mtd_device_unregister(mtd);
	nfc = (struct nandx_nfc *)mtd->priv;
	nandx_disable_clk(&nfc->clk, nfc->res->nand_type);

	pm_runtime_get_sync(&pdev->dev);

	mem_free(nfc->res);
	mem_free(nfc);
	mem_free(mtd);
	return 0;
}

#ifdef CONFIG_PM
static int nandx_runtime_suspend(struct device *dev)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc = (struct nandx_nfc *)mtd->priv;
	int ret;

	ret = nandx_suspend();
	nandx_disable_clk(&nfc->clk, nfc->res->nand_type);

	return ret;
}

static int nandx_runtime_resume(struct device *dev)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct nandx_nfc *nfc;
	int ret;

	nfc = (struct nandx_nfc *)mtd->priv;

	ret = nandx_enable_clk(&nfc->clk, nfc->res->nand_type);
	if (ret)
		return ret;

	ret = nandx_resume();
	return ret;
}
#endif

static const struct dev_pm_ops nfc_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend, pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(nandx_runtime_suspend, nandx_runtime_resume, NULL)
};

static struct platform_driver nand_driver = {
	.probe = nand_probe,
	.remove = nand_remove,
	.driver = {
		   .name = "mtk-nand",
		   .owner = THIS_MODULE,
		   .of_match_table = ic_of_match,
		   .pm = &nfc_dev_pm_ops,
	},
};
MODULE_DEVICE_TABLE(of, mtk_nfc_id_table);

static int __init nand_init(void)
{
	return platform_driver_register(&nand_driver);
}

static void __exit nand_exit(void)
{
	platform_driver_unregister(&nand_driver);
}
module_init(nand_init);
module_exit(nand_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MTK Nand Flash Controller Driver");
MODULE_AUTHOR("MediaTek");
