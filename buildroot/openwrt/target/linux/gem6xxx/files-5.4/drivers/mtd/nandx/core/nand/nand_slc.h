/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#ifndef __NAND_SLC_H__
#define __NAND_SLC_H__

/*
 * slc nand handler
 * @base: slc nand base functions
 * @parent: common parent nand base functions
 * @cache: nand cache operation flag
 * @multi: nand multi-plane operation flag
 * @timing_mode: current timing mode on nand device
 * @read_multi: slc nand advanced base function for multi-plane read
 * @program_multi: slc nand advanced base function for multi-plane write
 * @erase_multi: slc nand advanced base function for multi-plane erase
 * @read_status_enhanced: slc nand advanced base function for enhanced
 *    status read
 */
struct nand_slc {
	struct nand_base base;
	struct nand_base *parent;

	bool cache;
	bool multi;
	int timing_mode;

	/* slc advanced base function  */
	int (*read_multi)(struct nand_base *nand, int row);
	int (*program_multi)(struct nand_base *nand);
	int (*erase_multi)(struct nand_base *nand, int row);
	int (*read_status_enhanced)(struct nand_base *nand, int row);

	/* record chip base function */
	int (*read_page)(struct nand_chip *chip, struct nand_ops *ops,
			 int count);
	int (*write_page)(struct nand_chip *chip, struct nand_ops *ops,
			  int count);
	int (*erase_block)(struct nand_chip *chip, struct nand_ops *ops,
			   int count);

	/* slc advanced function */
	int (*cache_read_page)(struct nand_chip *chip, struct nand_ops *ops,
			       int count);
	int (*multi_read_page)(struct nand_chip *chip, struct nand_ops *ops,
			       int count);
	int (*cache_write_page)(struct nand_chip *chip, struct nand_ops *ops,
				int count);
	int (*multi_write_page)(struct nand_chip *chip, struct nand_ops *ops,
				int count);
	int (*multi_erase_block)(struct nand_chip *chip, struct nand_ops *ops,
				 int count);
};

static inline struct nand_slc *base_to_slc(struct nand_base *base)
{
	return container_of(base, struct nand_slc, base);
}

#endif /* __NAND_SLC_H__ */
