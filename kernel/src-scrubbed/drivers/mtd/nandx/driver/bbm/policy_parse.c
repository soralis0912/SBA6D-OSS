/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */
#include "bbm_core.h"
#include "bbm_policy.h"
#include "bbm_error.h"
#include "policy_parse.h"

#if PARTITION_FORMAT_PMT
#define PT_SIG 0x50547633  //"PTv3"
#define MPT_SIG 0x4D505433 //"MPT3"
#define PT_SIG_SIZE 8
#define is_valid_mpt(buf) ((*(uint32_t *)(buf)) == MPT_SIG)
#define is_valid_pt(buf) ((*(uint32_t *)(buf)) == PT_SIG)

/* 64bit */
struct pmt_entry
{
	char name[MAX_PARTITION_NAME_LEN];      /* partition name */
	unsigned long long size;		/* partition size */
	unsigned long long part_id;             /* partition region */
	unsigned long long offset;       	/* partition start */
	unsigned long long mask_flags;       	/* partition flags */
	unsigned long bbm_policy;               /* partition bbm_policy */
};

int partition_policy_parse(struct bbm_nand *device, struct part_policy *parts, u32 *part_count)
{
	int ret, j, i;
	u8 *buf;
	u64 offset;
	u32 pmt_entry_size, pos, start_block;
	struct pmt_entry pt = {0};

	start_block = device->blocks - device->bbt_blocks - device->part_blocks;
	buf = device->page_buf;
	pmt_entry_size = sizeof(struct pmt_entry);

	for (j = device->part_blocks - 1; j >= 0; j--) {
		*part_count = 0;
		offset = (u64)(start_block + (u32)j) << device->bs_shift;
		if (device->is_bad(offset)) {
			ret = -EFAULT;
			continue;
		}

		ret = device->read(buf, NULL, offset, device->page_size);
		if (ret < 0) {
			pr_err("PMT %d read , err(%d)\n", j, ret);
			continue;
		}

		if (!is_valid_mpt(buf) && !is_valid_pt(buf)) {
			pr_err("PMT: not invalid sig in PMT %d,sig:0x%X\n", j, *(uint32_t *)buf);
			ret = -EFAULT;
			continue;
		}

		pos = PT_SIG_SIZE;
		for (i = 0; i < PARTITION_MAX_COUNT; i++) {
			/* fill pmt entry data */
			if (pos + pmt_entry_size <= device->page_size) {
				memcpy((u8 *)&pt, buf + pos, pmt_entry_size);
				pos += pmt_entry_size;
			} else if (pos <= device->page_size) {
				/* copy the partial data of pmt entry in the previous page */
				if (device->page_size - pos)
					memcpy((u8 *)&pt, buf + pos, device->page_size - pos);

				/* read the next page*/
				offset += device->page_size;
				ret = device->read(buf, NULL, offset, device->page_size);
				if (ret < 0) {
					pr_err("PMT %d read , err(%d)\n", j, ret);
					break;
				}
				/* copy the partial data of pmt entry in the next page */
				memcpy(((u8 *)&pt) + device->page_size - pos, buf, pmt_entry_size - (device->page_size - pos));
				pos = pmt_entry_size - (device->page_size - pos);
			}

			if (strlen((const char *)pt.name) == 0)
				return 0;

			parts[*part_count].start_addr = pt.offset;
			parts[*part_count].length = (u32)pt.size;
			parts[*part_count].policy = (u32)pt.bbm_policy;
			*part_count = *part_count + 1;
		}
	}

	if (ret < 0)
		*part_count = 0;

	return ret;
}
#else
/* parser GPT partition */
int partition_policy_parse(struct bbm_nand *device, struct part_policy *parts, u32 *part_count)
{
	return 0;
}
#endif
