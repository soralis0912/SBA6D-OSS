/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */
#include "bbm_error.h"
#include "bbm_core.h"

static const u8 bbmt_pattern[] = {'b', 'b', 'm', 't' };

static u16 cal_bbmt_crc(const struct bbmt *bbmt)
{
	u8 *buf;
	u32 not_crc_len, length;

	not_crc_len = sizeof(bbmt->magic) + sizeof(bbmt->crc);
	/* move to the address of item that will be caculated crc */
	buf = (u8 *)bbmt + not_crc_len;
	length = sizeof(struct bbmt) - not_crc_len + bbmt->map_length * sizeof(struct block_map);

	return crc16(buf, length);
}

static bool is_valid_bbmt(const struct bbmt *bbmt)
{
	if (strncmp((u8 *)bbmt, bbmt_pattern, sizeof(bbmt_pattern)))
		return false;

	/* do crc check */
	if (bbmt->crc != cal_bbmt_crc(bbmt))
		return false;

	return true;
}

static inline u32 get_pool_size(const struct bbm_mapping *mapping)
{
	return mapping->end_block - mapping->start_block - BBMT_SCAN_MAX_BLOCKS + 1;
}

static void pack_bbmt(const struct bbm *bbm, struct bbmt *bbmt)
{
	struct bbm_mapping *mapping = &bbm->mapping;
	u32 i;

	bbmt->version = mapping->version;
	memcpy(bbmt->magic, bbmt_pattern, sizeof(bbmt_pattern));
	bbmt->start_block = mapping->start_block;
	bbmt->top_block = mapping->top_block;

	bbmt->map_length = mapping->map_length;
	memcpy(bbmt->map, mapping->map_entry, sizeof(struct block_map) * bbmt->map_length);

	bbmt->crc = cal_bbmt_crc(bbmt);

	for (i = 0; i < bbmt->map_length; i++)
		pr_info("bbmt map bad: %d map: %d!\n", bbmt->map[i].bad, bbmt->map[i].map);
}

static int write_bbmt(const struct bbm *bbm, u64 reserve_addr, u64 *write_addr)
{
	const struct bbm_nand *device = bbm->device;
	struct bbmt *bbmt;
	u32 i;
	u64 offset;
	int ret;

	memset(device->page_buf, 0, device->page_size);
	bbmt = (struct bbmt *)device->page_buf;
	pack_bbmt(bbm, bbmt);

	for (i = bbm->mapping.end_block; i >= bbm->mapping.top_block; i--) {
		offset = BLOCK2ADDR(i, device->bs_shift);
		if (device->is_bad(offset) || offset == reserve_addr)
			continue;

		ret = device->erase(offset, 1 << device->bs_shift);
		if (ret >= 0) {
			ret = device->write(device->page_buf, NULL, offset, device->page_size);
			if (ret >= 0) {
				*write_addr = offset;
				return 0;
			}
		}

		ret = device->mark_bad(offset);
		if (ret < 0)
			return ret;
	}

	return -ENOSPC;
}

static int set_bbm_mapping(struct bbm *bbm, const struct bbmt *bbmt)
{
	struct bbm_mapping *mapping = &bbm->mapping;
	int i;

	mapping->version = bbmt->version;
	mapping->start_block = bbmt->start_block;
	mapping->top_block = bbmt->top_block;
	mapping->map_length = bbmt->map_length;

	mapping->map_entry = mem_alloc(get_pool_size(mapping), sizeof(struct block_map));
	if (!mapping->map_entry) {
		pr_err("%s, %d, map_entry alloc fail!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	memcpy(mapping->map_entry, bbmt->map, sizeof(struct block_map) * bbmt->map_length);

	for (i = 0; i < mapping->map_length; i++)
		pr_info("bbm map init bad: %d map: %d!\n", mapping->map_entry[i].bad, mapping->map_entry[i].map);

	return 0;
}

/**
 * scan_read_bbmt - scan the pool to search and read bbmt
 * @bbm: bbm instance
 * @bbmt: the bbmt instance where will store the bbmt read from device
 * @bbmt_size: the bbmt max length
 *
 * This function return the bbmt count of the biggest version
 */
static u32 scan_read_bbmt(struct bbm *bbm, struct bbmt *bbmt, u32 bbmt_size)
{
	const struct bbm_nand *device = bbm->device;
	struct bbmt *bbmt_tmp;
	u64 offset;
	u32 i, good = 0, valid_bbmt = 0;
	int ret;

	bbmt_tmp = (struct bbmt *)device->page_buf;
	/* The bbmt data reside in the last two good block of BBM pool */
	for (i = bbm->mapping.end_block; i >= bbm->mapping.start_block; i--) {
		offset = (u64)i << device->bs_shift;

		/* stop to search bbmt if the last two good blocks of BBM pool have been checked */
		if ((good == BBMT_SCAN_MAX_BLOCKS) || (valid_bbmt == BBMT_SCAN_MAX_BLOCKS))
			break;

		if (device->is_bad(offset))
			continue;

		good++;
		ret = device->read(device->page_buf, NULL, offset, device->page_size);
		if (ret < 0) {
			pr_err("read bbmt at 0x%llx fail, ret:%d\n", offset, ret);
			continue;
		}

		/* check valid or not */
		if (!is_valid_bbmt(bbmt_tmp))
			continue;

		/**
		 * bbmt is always the highest version bbmt data
		 * bbm->bbmt_addr[0] is always the highest version bbmt address
		 */
		if ((valid_bbmt == 0) || (bbmt->version < bbmt_tmp->version)) {
			memcpy(bbmt, device->page_buf, bbmt_size);
			bbm->bbmt_addr[0] = offset;
			valid_bbmt = 1;
		} else if (bbmt->version == bbmt_tmp->version) {
			/* only if the version is equal to the highest version, preserve the address*/
			bbm->bbmt_addr[valid_bbmt] = offset;
			valid_bbmt++;
		}
	}

	return valid_bbmt;
}

static int get_next_good_block(struct bbm *bbm, u32 *good_block)
{
	struct bbm_mapping *mapping = &bbm->mapping;
	const struct bbm_nand *device = bbm->device;
	u32 block;
	u64 offset, end;

	end = min_t(u64, bbm->bbmt_addr[0], bbm->bbmt_addr[1]);
	end = ADDR2BLOCK(end, device->bs_shift);

	for (block = mapping->top_block; block < end; block++) {
		offset = BLOCK2ADDR(block, device->bs_shift);
		if (!device->is_bad(offset)) {
			*good_block = block;
			mapping->top_block = block + 1;
			return 0;
		}
	}

	return -1;
}

int bbm_core_init(const struct bbm_nand *device, struct bbm *bbm, u16 pool_start, u16 pool_end)
{
	struct bbmt *bbmt;
	u32 bbmt_size, bbmt_count;
	int ret;

	bbm->device = device;
	bbm->mapping.start_block = pool_start;
	bbm->mapping.end_block = pool_end;

	/* prepare bbmt buf to store bbmt data */
	bbmt_size = sizeof(struct bbmt) + get_pool_size(&bbm->mapping) * sizeof(struct block_map);
	bbmt = mem_alloc(1, bbmt_size);
	if (!bbmt) {
		pr_err("%s, %d, bbmt alloc fail!\n", __func__, __LINE__);
		return -ENOMEM;
	}
	memset(bbmt, 0, bbmt_size);

	/* read bbmt */
	bbmt_count = scan_read_bbmt(bbm, bbmt, bbmt_size);
	if (bbmt_count == 0) {
		ret = -E_NO_BBMT;
		goto OUT;
	}

	ret = set_bbm_mapping(bbm, bbmt);
	if (ret)
		goto OUT;

	/* recover another bbmt */
	if (bbmt_count < BBMT_SCAN_MAX_BLOCKS)
		ret = write_bbmt(bbm, bbm->bbmt_addr[0], &bbm->bbmt_addr[1]);

OUT:
	mem_free(bbmt);
	return ret;
}

int bbm_core_initialize_table(struct bbm *bbm, u16 pool_start, u16 pool_end)
{
	struct bbm_mapping *mapping = &bbm->mapping;
	int ret;

	mapping->version = 1;
	mapping->start_block = pool_start;
	mapping->end_block = pool_end;
	mapping->top_block = pool_start;
	mapping->map_length = 0;

	if (mapping->map_entry)
		mem_free(mapping->map_entry);
	mapping->map_entry = mem_alloc(get_pool_size(mapping), sizeof(struct block_map));
	if (!mapping->map_entry) {
		pr_err("%s, %d, map_entry alloc fail!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ret = write_bbmt(bbm, ~(0llu), &bbm->bbmt_addr[0]);
	if (ret < 0)
		goto ERROR;

	ret = write_bbmt(bbm, bbm->bbmt_addr[0], &bbm->bbmt_addr[1]);
	if (ret < 0)
		goto ERROR;

	return 0;

ERROR:
	mem_free(mapping->map_entry);

	return ret;
}

int bbm_core_get_map(const struct bbm *bbm, u32 bad_block, u32 *map_block)
{
	struct bbm_mapping *mapping = &bbm->mapping;
	u32 begin = 0, end = mapping->map_length, mid;
	struct block_map *map_entry;

	if ((bad_block >= mapping->start_block) && (bad_block <= mapping->end_block)) {
		pr_err("%s, %d, bad_block %d illegal \n",  __func__, __LINE__, bad_block);
		return -EINVAL;
	}

	if (!map_block) {
		pr_err("%s, %d, map_block is NULL \n", __func__, __LINE__, bad_block);
		return -EINVAL;
	}

	while (end > begin) {
		mid = (begin + end) / 2;
		map_entry = mapping->map_entry + mid;

		if (bad_block > map_entry->bad) {
			begin = mid + 1;
		} else if (bad_block < map_entry->bad) {
			end = mid;
		} else {
			*map_block = map_entry->map;
			pr_info("%s bad: %d map: %d!\n", __func__, bad_block, *map_block);
			return 0;
		}
	}

	return -E_NO_MAP;
}

int bbm_core_map_block(struct bbm *bbm, u32 bad_block, u32 *map_block)
{
	int ret, i;
	struct bbm_mapping *mapping = &bbm->mapping;
	u32 begin = 0, end = mapping->map_length, mid;
	struct block_map *map_entry;
	bool found = false;

	if ((bad_block >= mapping->start_block) && (bad_block <= mapping->end_block)) {
		pr_err("%s, %d, bad_block %d illegal \n",
		       __func__, __LINE__, bad_block);
		return -EINVAL;
	}

	if (!map_block) {
		pr_err("%s, %d, map_block is NULL \n",
		       __func__, __LINE__, bad_block);
		return -EINVAL;
	}

	/* get a good block from pool */
	ret = get_next_good_block(bbm, map_block);
	if (ret < 0)
		return ret;

	/**
	 * search the bad block or its position in map table,
	 * map table is sorted by bad in ascending order
	 */
	while (end > begin) {
		mid = (begin + end) / 2;
		map_entry = mapping->map_entry + mid;

		if (bad_block > map_entry->bad) {
			begin = mid + 1;
		} else if (bad_block < map_entry->bad) {
			end = mid;
		} else {
			found = true;
			break;
		}
	}

	if (!found) {
		map_entry = mapping->map_entry;
		/* spare a space for new map*/
		for (i = mapping->map_length; i > end; i--) {
			map_entry[i].bad = map_entry[i - 1].bad;
			map_entry[i].map = map_entry[i - 1].map;
		}

		map_entry = mapping->map_entry + end;
		map_entry->bad = bad_block;
		mapping->map_length++;
	}

	map_entry->map = *map_block;

	return 0;
}

int bbm_core_sync(struct bbm *bbm)
{
	int ret;

	bbm->mapping.version++;

	ret = write_bbmt(bbm, ~(0llu), &bbm->bbmt_addr[0]);
	if (ret < 0) {
		pr_err("write first bbmt fail!!!\n");
		return ret;
	}

	ret = write_bbmt(bbm, bbm->bbmt_addr[0], &bbm->bbmt_addr[1]);
	if (ret < 0)
		pr_err("write second bbmt fail!!!\n");

	return ret;
}

/**
* bbm_core_corrcet_bbmt - correct the BBMT if the bad block in BBMT is not bad
*
* We need correct BBMT because power loss may cause BBMT is written to flash,
* but marking the block as bad is not done.
*/
int bbm_core_corrcet_bbmt(const struct bbm *bbm)
{
	struct bbm_mapping *mapping = &bbm->mapping;
	const struct bbm_nand *device = bbm->device;
	u64 offset;
	int i, ret;

	for (i = 0; i < mapping->map_length; i++){
		offset = (u64)BLOCK2ADDR(mapping->map_entry[i].bad, device->bs_shift);
		if (device->is_bad(offset))
			continue;

		pr_info("power cut corrcet mark block %d!\n", mapping->map_entry[i].bad);
		ret = device->mark_bad(offset);
		if (ret)
			return ret;
	}

	return 0;
}

void bbm_core_exit(struct bbm *bbm)
{
	if (bbm->mapping.map_entry)
		mem_free(bbm->mapping.map_entry);
}
