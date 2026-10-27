/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __BBM_CORE_H__
#define __BBM_CORE_H__

#include "bbm_util.h"

#define BBMT_SCAN_MAX_BLOCKS     2
#define BBMPOOL_DEFAULT_RATIO    20

struct bbm_nand {
	u32 blocks;
	u32 page_size;
	u32 oob_size;
	u32 bs_shift;
	u32 bbt_blocks;
	u32 part_blocks;
	int (*read)(u8 *data, u8 *oob, u64 offset, size_t len);
	int (*write)(u8 *data, u8 *oob, u64 offset, size_t len);
	int (*erase)(u64 offset, size_t len);
	int (*is_bad)(u64 offset);
	int (*mark_bad)(u64 offset);
	u8  *page_buf;
};

struct block_map {
	u16 bad;
	u16 map;
};

struct bbm_mapping {
	u16 version;
	u16 start_block;
	u16 end_block;
	u16 top_block;
	u16 map_length;
	struct block_map *map_entry;
};

struct bbm {
	struct bbm_mapping mapping;
	u64 bbmt_addr[BBMT_SCAN_MAX_BLOCKS];
	const struct bbm_nand *device;
};

struct bbmt {
	u8 magic[4];
	u16 crc;
	u16 version;
	u16 start_block;
	u16 top_block;
	u16 map_length;
	struct block_map map[];
};

int bbm_core_init(const struct bbm_nand *device, struct bbm *bbm, u16 pool_start, u16 pool_end);
int bbm_core_initialize_table(struct bbm *bbm, u16 pool_start, u16 pool_end);
int bbm_core_get_map(const struct bbm *bbm, u32 bad_block, u32 *map_block);
int bbm_core_map_block(struct bbm *bbm, u32 bad_block, u32 *map_block);
int bbm_core_sync(struct bbm *bbm);
int bbm_core_corrcet_bbmt(const struct bbm *bbm);
void bbm_core_exit(struct bbm *bbm);

#endif /*__BBM_CORE_H__*/
