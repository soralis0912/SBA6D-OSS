// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author:	Guochun Mao	<guochun.mao@mediatek.com>
 *		Xiaolei Li	<xiaolei.li@mediatek.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <asm/div64.h>

#define MAX_PARTITION_COUNT 128
#define MAX_PARTITION_NAME_LEN 64
#define PT_SIG      0x50547633    //"PTv3"
#define MPT_SIG     0x4D505433    //"MPT3"
#define PT_SIG_SIZE 8

#define is_valid_mpt(buf) ((*(uint32_t *)(buf)) == MPT_SIG)
#define is_valid_pt(buf) ((*(uint32_t *)(buf)) == PT_SIG)
#define is_valid_pt_v1(buf) ((*(uint32_t *)(buf)) == PT_SIG_V1)

typedef struct
{
	unsigned char name[MAX_PARTITION_NAME_LEN];	/* partition name */
	unsigned long long size;			/* partition size */
	unsigned long long part_id;			/* partition region */
	unsigned long long offset;			/* partition start */
	unsigned long long mask_flags;			/* partition flags */
#ifdef CONFIG_NAND_BBM
	unsigned long bbm_policy;                       /* partition bbm_policy */
#endif
} pt_resident;

static void pmt_add_part(struct mtd_partition *part, char *name,
			 u64 offset, uint32_t mask_flags, uint64_t size)
{
	part->name = kstrdup_const(name, GFP_KERNEL);
	part->offset = offset;
	part->mask_flags = mask_flags;
	part->size = size;
}

static int pmt_parse(struct mtd_info *master,
		     const struct mtd_partition **pparts,
		     struct mtd_part_parser_data *data)
{
	struct mtd_partition *parts;
	int err, i;
	u_char *buf;
	u32 pmt_size = sizeof(pt_resident) * MAX_PARTITION_COUNT;
	u8 pmt[PT_SIG_SIZE] = {0};
	pt_resident *pt;
	size_t bytes_read = 0;
	loff_t pmt_start_addr = (loff_t)master->size - 8 * (loff_t)master->erasesize;
	loff_t pmt_addr;
	u8 max_partition_count = 0;

	dev_dbg(&master->dev, "PMT: enter pmt parser...\n");

	buf = kzalloc(pmt_size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	for (i = 3; i >= 0; i--) {
		pmt_addr = pmt_start_addr + i * (loff_t)master->erasesize;

		err = mtd_block_isbad(master, pmt_addr);
		if (err) {
			dev_err(&master->dev,
				"PMT: PMT block %d is bad\n", i);
			err = -EFAULT;
			continue;
		}

		err = mtd_read(master, pmt_addr, sizeof(pmt),
			       &bytes_read, pmt);
		if (err < 0) {
			dev_err(&master->dev,
				"PMT: read PMT block %d fail\n", i);
			continue;
		}

		if (is_valid_mpt(pmt) == 0 && is_valid_pt(pmt) == 0) {
			dev_err(&master->dev,
				"PMT: not find sig in PMT block %d\n", i);
			err = -EFAULT;
			continue;
		}

		err = mtd_read(master, pmt_addr + PT_SIG_SIZE, pmt_size,
			       &bytes_read, buf);
		if (err < 0) {
			dev_err(&master->dev,
				"PMT: read PMT block %d fail\n", i);
			continue;
		}

		break;
	}

	if (err < 0)
		goto freebuf;

	pt = (pt_resident *)buf;
	for (i = 0 ; i < MAX_PARTITION_COUNT ; i++) {
		if (strlen(pt[i].name) == 0)
			break;
	}
	max_partition_count = i;
	dev_dbg(&master->dev, "pmt there are <%d> parititons.\n",
				max_partition_count);

	parts = kcalloc(max_partition_count,
			sizeof(struct mtd_partition), GFP_KERNEL);
	if (parts == NULL)
		goto freebuf;

	for (i = 0 ; i < max_partition_count ; i++) {
		pmt_add_part(&parts[i], pt[i].name,
				pt[i].offset, 0, pt[i].size);

	}

parsedone:
	*pparts = parts;
	kfree(buf);
	return max_partition_count;

freebuf:
	kfree(buf);
	return 0;
};

static void pmt_cleanup(const struct mtd_partition *pparts, int nr_parts)
{
	int i;

	for (i = 0; i < nr_parts; i++)
		kfree_const(pparts[i].name);

	kfree(pparts);
}

static struct mtd_part_parser pmt_parser = {
	.owner = THIS_MODULE,
	.parse_fn = pmt_parse,
	.cleanup = pmt_cleanup,
	.name = "pmtpart",
};

static int __init pmtpart_init(void)
{
	return register_mtd_parser(&pmt_parser);
}

static void __exit pmtpart_exit(void)
{
	deregister_mtd_parser(&pmt_parser);
}

module_init(pmtpart_init);
module_exit(pmtpart_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMT partitioning for flash memories");
