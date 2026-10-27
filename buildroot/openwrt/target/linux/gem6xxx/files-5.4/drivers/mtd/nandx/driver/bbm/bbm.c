/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */
#include "bbm_util.h"
#include "bbm_error.h"
#include "bbm_core.h"
#include "bbm_policy.h"
#include "policy_parse.h"

struct bbm_instance {
	struct bbm_policy bbmp;
	struct bbm bbm;
	struct bbm_nand nand;
};

static struct bbm_instance bbm_inst;

int bbm_read(u8 *data, u8 *oob, u64 offset, size_t len)
{
	return bbm_policy_read(&bbm_inst.bbmp, data, oob, offset, len);
}

int bbm_write(u8 *data, u8 *oob, u64 offset, size_t len)
{
	return bbm_policy_write(&bbm_inst.bbmp, data, oob, offset, len);
}

int bbm_erase(u64 offset, size_t len)
{
	return bbm_policy_erase(&bbm_inst.bbmp, offset, len);
}

int bbm_is_bad(u64 offset)
{
	return bbm_policy_is_bad_block(&bbm_inst.bbmp, offset);
}

int bbm_mark_bad(u64 offset)
{
	return bbm_policy_mark_bad(&bbm_inst.bbmp, offset);
}

/**
* bbm_partition_policy_parse - parse partition policy
*
* The function is only called in DA or other download agent
*/
int bbm_partition_policy_parse(void)
{
	/* the function must be called after bbm_init where bbm_io.bbmp.parts has been allocated memory*/
	return partition_policy_parse(&bbm_inst.nand, bbm_inst.bbmp.parts, &bbm_inst.bbmp.part_count);
}

int bbm_reset_map_table(void)
{
	const struct bbm_nand *nand = &bbm_inst.nand;
	int ret;
	u16 pool_start, pool_end;

	pool_end = nand->blocks - nand->bbt_blocks - nand->part_blocks - 1;
	pool_start = pool_end + 1 - div_down((bbm_inst.nand.blocks * BBMPOOL_DEFAULT_RATIO), 1024);

	return bbm_core_initialize_table(&bbm_inst.bbm, pool_start, pool_end);
}

u64 bbm_default_pool_size(void)
{
	return (u64)(((u64)bbm_inst.nand.blocks * BBMPOOL_DEFAULT_RATIO) / 1024) << bbm_inst.nand.bs_shift;
}

u32 bbm_parse_part_policy(char *policy_str)
{
	u32 policy = 0;

	if (strstr(policy_str, "MAP"))
		policy |= POLICY_MAP;

	if (strstr(policy_str, "REPORT"))
		policy |= POLICY_REPORT;

	if (strstr(policy_str, "SKIP"))
		policy |= POLICY_SKIP;

	return policy;
}

void bbm_set_upgrade_all(struct bbm_policy *bbmp)
{
	bbmp->upgrade_mode = UPGRADE_ALL;
}

void bbm_set_upgrade_partition(struct bbm_policy *bbmp, u32 part_index)
{
	bbmp->upgrade_mode = part_index;
}

void bbm_cancel_upgrade(struct bbm_policy *bbmp)
{
	bbmp->upgrade_mode = UPGRADE_NONE;
}

/**
* bbm_init - init bbm
* @nand: input argument, bbm_nand instance
* @is_da: the function is called by DA or not
* @is_preloader: the function is called by preloader or not
*/
int bbm_init(const struct bbm_nand *nand, bool is_da, bool is_preloader)
{
	struct bbm_nand *bbm_nand = &bbm_inst.nand;
	struct part_policy *parts;
	u32 part_count;
	int ret;
	u16 pool_start, pool_end;

	bbm_inst.nand = *nand;
	bbm_nand->page_buf = mem_alloc(1, nand->page_size + nand->oob_size);
	if (bbm_nand->page_buf == NULL) {
		pr_err("%s alloc page buf fail\n", __func__);
		return -ENOMEM;
	}

	parts = mem_alloc(PARTITION_MAX_COUNT, sizeof(struct part_policy));
	if (!parts) {
		pr_err("%s, %d, buf alloc fail!\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto free_page_buf;
	}

	ret = partition_policy_parse(bbm_nand, parts, &part_count);
	/* partition table may be not exit in DA, but must be exit in other phase */
	if (ret && !is_da)
		goto free_page_buf;

	pool_end = nand->blocks - nand->bbt_blocks - nand->part_blocks - 1;
	pool_start = pool_end + 1 - div_down((bbm_inst.nand.blocks * BBMPOOL_DEFAULT_RATIO), 1024);
	ret = bbm_core_init(bbm_nand, &bbm_inst.bbm, pool_start, pool_end);
	if (ret < 0 && ret != -E_NO_BBMT)
		goto free_parts;

	/* ret == 0 or ret == -E_NO_BBMT*/
	if (ret == -E_NO_BBMT) {
		/* BBMT may be not exit in DA and preloader, but must be exit in other phase */
		if (!is_da && !is_preloader)
			goto free_bbm;

		ret = bbm_core_initialize_table(&bbm_inst.bbm, pool_start, pool_end);
		if (ret < 0)
			goto free_bbm;
	} else if (is_da || is_preloader) {
		bbm_core_corrcet_bbmt(&bbm_inst.bbm);
	}

	bbm_inst.bbmp.parts = parts;
	bbm_inst.bbmp.part_count = part_count;
	bbm_inst.bbmp.bbm = &bbm_inst.bbm;
	bbm_inst.bbmp.device = bbm_nand;
	/* set upgrade all mode in DA */
	if (is_da)
		bbm_inst.bbmp.upgrade_mode = UPGRADE_ALL;
	else
		bbm_inst.bbmp.upgrade_mode = UPGRADE_NONE;

	return 0;

free_bbm:
	bbm_core_exit(&bbm_inst.bbm);

free_parts:
	mem_free(parts);

free_page_buf:
	mem_free(bbm_nand->page_buf);

	return ret;
}
