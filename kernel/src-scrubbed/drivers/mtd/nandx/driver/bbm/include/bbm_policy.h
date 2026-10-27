/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __BBM_POLICY_H__
#define __BBM_POLICY_H__

#include "bbm_core.h"

#define BBM_SKIP_ENABLE 1

#define BBMP_ADDR2BLOCK(offset, bbmp)		(ADDR2BLOCK(offset, bbmp->device->bs_shift))
#define BBMP_BLOCK2ADDR(block, bbmp)		(BLOCK2ADDR(block, bbmp->device->bs_shift))
#define BBMP_ADDR2BLOCKOFFS(offset, bbmp)	(ADDR2BLOCKOFFS(offset, bbmp->device->bs_shift))

#define POLICY_MAP	BIT(0)
#define POLICY_REPORT	BIT(1)
#define POLICY_SKIP	BIT(2)

struct part_policy {
	u64 start_addr;
	u32 length;
	u32 policy;
};

#define UPGRADE_ALL	(-2)
#define UPGRADE_NONE	(-1)

struct bbm_policy {
	u32 part_count;
	const struct part_policy *parts;
	int upgrade_mode;/* UPGRADE_ALL; UPGRADE_NONE; 0 or positive: upgraded partition index */
	struct bbm *bbm;
	const struct bbm_nand *device;
};

void bbm_policy_init(struct bbm_policy *bbmp, const struct bbm_nand *device, const struct part_policy *parts, u32 part_count);
int bbm_policy_read(const struct bbm_policy *bbmp, u8 *data, u8 *oob, u64 offset, size_t len);
int bbm_policy_write(const struct bbm_policy *bbmp, u8 *data, u8 *oob, u64 offset, size_t len);
int bbm_policy_erase(const struct bbm_policy *bbmp, u64 offset, size_t len);
bool bbm_policy_is_bad_block(const struct bbm_policy *bbmp, u64 offset);
int bbm_policy_mark_bad(const struct bbm_policy *bbmp, u64 offset);

#endif /*__BBM_POLICY_H__*/
