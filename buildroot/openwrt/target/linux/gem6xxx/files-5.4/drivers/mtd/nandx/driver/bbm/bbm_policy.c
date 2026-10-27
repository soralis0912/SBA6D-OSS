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

static inline bool is_empty_data(const u8 *data, u32 len)
{
	u32 i = 0;

	while (i < len) {
		if (data[i] != 0xFF)
			return false;
		i++;
	}

	return true;
}

static int bbmp_get_part(const struct bbm_policy *bbmp, u64 offset, struct part_policy **ppart)
{
	struct part_policy *part;
	u32 begin = 0, end = bbmp->part_count, mid;

	while (end > begin) {
		mid = (begin + end) / 2;
		part = &bbmp->parts[mid];

		if (offset >= (part->start_addr + part->length)) {
			begin = mid + 1;
		} else if (offset < part->start_addr) {
			end = mid;
		} else {
			*ppart = part;
			return 0;
		}
	}

	pr_err("get part fail, offset:%lld, part_count:%d\n", offset, bbmp->part_count);

	return -EINVAL;
}

static inline bool bbmp_need_skip(const struct bbm_policy *bbmp, const struct part_policy *part)
{
	u32 index = div_down((u32)(part - bbmp->parts), sizeof(struct part_policy));

#if BBM_SKIP_ENABLE
	if (part->policy & POLICY_SKIP)
		return true;

	/* For FS partition, when update it, use skip policy */
	if ((bbmp->upgrade_mode == UPGRADE_ALL) || (bbmp->upgrade_mode == index))
		if (part->policy & POLICY_REPORT)
			return true;
#endif
	return false;
}

static inline bool bbmp_base_need_map(const struct bbm_policy *bbmp, const struct part_policy *part)
{
	/* only partition with map policy, map bad block */
	return (part->policy & POLICY_MAP) ? true : false;
}

static int bbmp_base_map(struct bbm_policy *bbmp, u64 offset, u64 *map_offset)
{
	u32 map_block;
	int ret;

	ret = bbm_core_map_block(bbmp->bbm, BBMP_ADDR2BLOCK(offset, bbmp), &map_block);
	if (ret)
		return ret;

	*map_offset = BBMP_BLOCK2ADDR(map_block, bbmp) + BBMP_ADDR2BLOCKOFFS(offset, bbmp);

	return 0;
}

static void bbmp_base_get_map(const struct bbm_policy *bbmp, u64 offset, u64 *map_offset)
{
	u32 map_block;
	int ret;

	ret = bbm_core_get_map(bbmp->bbm, BBMP_ADDR2BLOCK(offset, bbmp), &map_block);
	if (!ret)
		*map_offset = BBMP_BLOCK2ADDR(map_block, bbmp) + BBMP_ADDR2BLOCKOFFS(offset, bbmp);
	else
		*map_offset = offset;
}

static int bbmp_base_move_worn_bad(const struct bbm_policy *bbmp, u64 offset, size_t len, u64 map_offset)
{
	const struct bbm_nand *nand = bbmp->device;
	u64 src = offset & ~(GENMASK(nand->bs_shift - 1, 0)), boundary_addr;
	u64 dest = map_offset & ~(GENMASK(nand->bs_shift - 1, 0));
	u8 *buf;
	int ret = 0;

	buf = nand->page_buf;
	boundary_addr = src + (1ull << nand->bs_shift);
	for (; src < boundary_addr; src += nand->page_size, dest += nand->page_size) {
		/* skip the new write*/
		if (src >= div_round_down(offset, nand->page_size) &&
			src < div_round_up(offset + len, nand->page_size))
			continue;

		ret = nand->read(buf, buf + nand->page_size, src, nand->page_size);
		if (ret) {
			pr_err("read 0x%llx fail, while copyback, ret:%d\n", src, ret);
			continue;
		}

		if (is_empty_data(buf, nand->page_size + nand->oob_size)) {
			pr_info("empty_page, not write\n");
			continue;
		}

		ret = nand->write(buf, buf + nand->page_size, dest, nand->page_size);
		if (ret) {
			pr_err("write 0x%llx fail, while copyback, ret:%d\n", dest, ret);
			break;
		}
	}

	return ret;
}

static bool bbmp_base_is_bad_block(const struct bbm_policy *bbmp, u64 offset)
{
	u32 map_block;
	int ret;

	ret = bbm_core_get_map(bbmp->bbm, BBMP_ADDR2BLOCK(offset, bbmp), &map_block);
	if (!ret)
		return bbmp->device->is_bad(BBMP_BLOCK2ADDR(map_block, bbmp));

	return bbmp->device->is_bad(offset);
}

static int bbmp_base_mark_bad(const struct bbm_policy *bbmp, u64 offset)
{
	u64 map_offset;

	bbmp_base_get_map(bbmp, offset, &map_offset);

	if (bbmp->device->is_bad(map_offset))
		return 0;

	return bbmp->device->mark_bad(map_offset);
}

static int bbmp_base_read(const struct bbm_policy *bbmp, u8 *data, u8 *oob, u64 offset, size_t len)
{
	u64 map_offset;

	bbmp_base_get_map(bbmp, offset, &map_offset);

	if (bbmp->device->is_bad(map_offset))
		return -E_BAD;

	return bbmp->device->read(data, oob, map_offset, len);
}

static int bbmp_base_erase(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset, size_t len)
{
	struct bbm_nand *nand = bbmp->device;
	u64 map_offset, old_offset;
	bool bad;
	int ret;

	bbmp_base_get_map(bbmp, offset, &map_offset);
	bad = nand->is_bad(map_offset);
	/* bad && no need map, directly return */
	if (bad && !bbmp_base_need_map(bbmp, part))
		return -E_BAD;

	/* not bad, execute erase */
	if (!bad) {
		ret = nand->erase(map_offset, len);
		if (ret == 0)
			return 0;

		if (!bbmp_base_need_map(bbmp, part))
			return ret;
	}

	/* process bad block and erase fail case */
	old_offset = map_offset;
	do {
		ret = bbmp_base_map(bbmp, offset, &map_offset);
		if (ret)
			return ret;

		ret = nand->erase(map_offset, len);
		if (ret) {
			/* do not process mark bad error */
			if (nand->mark_bad(map_offset))
				pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, map_offset);
		}
	} while (ret);

	ret = bbm_core_sync(bbmp->bbm);
	if (ret) {
		pr_err("bbm sync err %d\n", ret);
		return ret;
	}

	/* do not process mark bad error */
	if (nand->mark_bad(old_offset))
		pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, old_offset);

	return ret;
}

static int bbmp_base_write(const struct bbm_policy *bbmp, const struct part_policy *part, u8 *data, u8 *oob, u64 offset, size_t len)
{
	const struct bbm_nand *nand = bbmp->device;
	u64 map_offset, old_offset;
	bool bad;
	int ret;

	bbmp_base_get_map(bbmp, offset, &map_offset);
	bad = nand->is_bad(map_offset);
	/* bad && no need map, directly return */
	if (bad && !bbmp_base_need_map(bbmp, part))
		return -E_BAD;

	/* not bad, execute write */
	if (!bad) {
		ret = nand->write(data, oob, map_offset, len);
		if (ret == 0)
			return 0;

		if (!bbmp_base_need_map(bbmp, part))
			return ret;
	}

	/* process bad block or write fail */
	old_offset = map_offset;
	do {
		ret = bbmp_base_map(bbmp, offset, &map_offset);
		if (ret)
			return ret;

		ret = nand->erase(map_offset & ~(GENMASK(nand->bs_shift - 1, 0)), 1 << nand->bs_shift);
		if (!ret) {
			ret = nand->write(data, oob, map_offset, len);
			if (!ret)
				ret = bbmp_base_move_worn_bad(bbmp, offset, len, map_offset);
		}

		if (ret) {
			if (nand->mark_bad(map_offset))
				pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, map_offset);
		}
	} while (ret);

	ret = bbm_core_sync(bbmp->bbm);
	if (ret) {
		pr_err("bbm sync err %d\n", ret);
		return ret;
	}

	/* do not process mark bad error */
	if (nand->mark_bad(old_offset))
		pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, old_offset);

	return ret;
}

static int bbmp_skip_get_map(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset, u64 *map_offset)
{
	const struct bbm_nand *nand = bbmp->device;
	u64 remind;
	u32 block;

	remind = offset & GENMASK(nand->bs_shift - 1, 0);
	block = BBMP_ADDR2BLOCK((offset - part->start_addr), bbmp) + 1;

	for(offset = part->start_addr; offset < (part->start_addr + part->length);
	    offset += (1ull << nand->bs_shift)) {
		if (bbmp_base_is_bad_block(bbmp, offset))
			continue;

		if (--block == 0) {
			*map_offset = offset + remind;
			return 0;
		}
	}

	return -E_OUT_OF_RANGE;
}

static bool bbmp_skip_is_bad_block(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset)
{
	const struct bbm_nand *nand = bbmp->device;
	u64 remind;
	u32 block;

	remind = offset & GENMASK(nand->bs_shift - 1, 0);
	block = BBMP_ADDR2BLOCK((offset - part->start_addr), bbmp) + 1;

	for(offset = part->start_addr; offset < (part->start_addr + part->length);
	    offset += (1ull << nand->bs_shift)) {
		if (bbmp_base_is_bad_block(bbmp, offset))
			continue;

		if (--block == 0)
			return false;
	}

	return true;
}

static int bbmp_skip_find_next_good(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset, u64 *n_offset)
{
	u64 end;

	offset = offset & ~(GENMASK(bbmp->device->bs_shift - 1, 0));
	offset += (1ull << bbmp->device->bs_shift);
	end = part->start_addr + part->length;
	for (; offset < end; offset += (1ull << bbmp->device->bs_shift)) {
		if (!bbmp_base_is_bad_block(bbmp, offset))
			break;
	}

	if (offset < end) {
		*n_offset = offset;
		return 0;
	}

	return -E_OUT_OF_RANGE;
}

static int bbmp_skip_move_worn_bad(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset, size_t len, u64 map_offset)
{
	const struct bbm_nand *nand = bbmp->device;
	u64 src = offset & ~(GENMASK(nand->bs_shift - 1, 0)), boundary_addr;
	u64 dest = map_offset & ~(GENMASK(nand->bs_shift - 1, 0));
	u8 *buf;
	int ret = 0;

	buf = nand->page_buf;
	boundary_addr = src + (1ull << nand->bs_shift);
	for (; src < boundary_addr; src += nand->page_size, dest += nand->page_size) {
		/* skip the new write*/
		if (src >= div_round_down(offset, nand->page_size) &&
			src < div_round_up(offset + len, nand->page_size))
			continue;

		ret = bbmp_base_read(bbmp, buf, buf + nand->page_size, src, nand->page_size);
		if (ret) {
			pr_err("read 0x%llx fail, while copyback, ret:%d\n", src, ret);
			continue;
		}

		if (is_empty_data(buf, nand->page_size + nand->oob_size)) {
			pr_info("empty_page, not write\n");
			continue;
		}

		ret = bbmp_base_write(bbmp, part, buf, buf + nand->page_size, dest, nand->page_size);
		if (ret) {
			pr_err("write 0x%llx fail, while copyback, ret:%d\n", dest, ret);
			break;
		}
	}

	return ret;
}

static int bbmp_skip_read(const struct bbm_policy *bbmp, const struct part_policy *part, u8 *data, u8 *oob, u64 offset, size_t len)
{
	u64 map_offset;
	int ret;

	ret = bbmp_skip_get_map(bbmp, part, offset, &map_offset);
	if (ret)
		return ret;

	return bbmp_base_read(bbmp, data, oob, map_offset, len);
}

static int bbmp_skip_erase(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset, size_t len)
{
	u64 map_offset, old_offset;
	int ret;

	ret = bbmp_skip_get_map(bbmp, part, offset, &map_offset);
	if (ret)
		return ret;

	ret = bbmp_base_erase(bbmp, part, map_offset, len);
	if (ret == 0 || bbmp_base_need_map(bbmp, part))
		return ret;

	old_offset = map_offset;
	/* process erase fail */
	do {
		ret = bbmp_skip_find_next_good(bbmp, part, map_offset, &map_offset);
		if (ret)
			return ret;

		ret = bbmp_base_erase(bbmp, part, map_offset, len);
		if (ret) {
			/* do not process mark bad error */
			if (bbmp_base_mark_bad(bbmp, map_offset))
				pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, map_offset);
		}
	} while (ret);

	/* do not process mark bad error */
	if (bbmp_base_mark_bad(bbmp, old_offset))
		pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, old_offset);

	return ret;
}

static int bbmp_skip_mark_bad(const struct bbm_policy *bbmp, const struct part_policy *part, u64 offset)
{
	u64 map_offset;
	int ret;

	ret = bbmp_skip_get_map(bbmp, part, offset, &map_offset);
	if (ret)
		return ret;

	return bbmp_base_mark_bad(bbmp, map_offset);
}

static int bbmp_skip_write(const struct bbm_policy *bbmp, const struct part_policy *part, u8 *data, u8 *oob, u64 offset, size_t len)
{
	const struct bbm_nand *nand = bbmp->device;
	u64 map_offset, old_offset;
	int ret;

	ret = bbmp_skip_get_map(bbmp, part, offset, &map_offset);
	if (ret)
		return ret;

	ret = bbmp_base_write(bbmp, part, data, oob, map_offset, len);
	if (ret == 0 || bbmp_base_need_map(bbmp, part))
		return ret;

	old_offset = map_offset;
	/* process write fail */
	do {
		ret = bbmp_skip_find_next_good(bbmp, part, map_offset, &map_offset);
		if (ret)
			return ret;

		ret = bbmp_base_erase(bbmp, part, map_offset & ~(GENMASK(nand->bs_shift - 1, 0)), 1 << nand->bs_shift);

		if (!ret) {
			ret = bbmp_base_write(bbmp, part, data, oob, map_offset, len);
			if (!ret)
				ret = bbmp_skip_move_worn_bad(bbmp, part, offset, len, map_offset);
		}

		if (ret) {
			if (bbmp_base_mark_bad(bbmp, map_offset))
				pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, map_offset);
		}

	} while (ret);

	/* do not process mark bad error */
	if (bbmp_base_mark_bad(bbmp, old_offset))
		pr_err("mark bad: 0x%llx, 0x:%llx\n", offset, old_offset);

	return ret;
}

int bbm_policy_read(const struct bbm_policy *bbmp, u8 *data, u8 *oob, u64 offset, size_t len)
{
	struct part_policy *part;
	int ret;

	ret = bbmp_get_part(bbmp, offset, &part);
	if (ret)
		return ret;

	if (bbmp_need_skip(bbmp, part))
		return bbmp_skip_read(bbmp, part, data, oob, offset, len);

	return bbmp_base_read(bbmp, data, oob, offset, len);
}

int bbm_policy_write(const struct bbm_policy *bbmp, u8 *data, u8 *oob, u64 offset, size_t len)
{
	struct part_policy *part;
	int ret;

	ret = bbmp_get_part(bbmp, offset, &part);
	if (ret)
		return ret;

	if (bbmp_need_skip(bbmp, part))
		return bbmp_skip_write(bbmp, part, data, oob, offset, len);

	return bbmp_base_write(bbmp, part, data, oob, offset, len);
}

int bbm_policy_erase(const struct bbm_policy *bbmp, u64 offset, size_t len)
{
	struct part_policy *part;
	int ret;

	ret = bbmp_get_part(bbmp, offset, &part);
	if (ret)
		return ret;

	if (bbmp_need_skip(bbmp, part))
		return bbmp_skip_erase(bbmp, part, offset, len);

	return bbmp_base_erase(bbmp, part, offset, len);
}

bool bbm_policy_is_bad_block(const struct bbm_policy *bbmp, u64 offset)
{
	struct part_policy *part;
	int ret;

	ret = bbmp_get_part(bbmp, offset, &part);
	if (ret)
		return ret;

	if (bbmp_need_skip(bbmp, part))
		return bbmp_skip_is_bad_block(bbmp, part, offset);

	return bbmp_base_is_bad_block(bbmp, offset);
}

int bbm_policy_mark_bad(const struct bbm_policy *bbmp, u64 offset)
{
	struct part_policy *part;
	int ret;

	ret = bbmp_get_part(bbmp, offset, &part);
	if (ret)
		return ret;

	if (bbmp_need_skip(bbmp, part))
		return bbmp_skip_mark_bad(bbmp, part, offset);

	return bbmp_base_mark_bad(bbmp, offset);
}


