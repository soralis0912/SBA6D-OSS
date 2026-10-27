/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __BBM_UTIL_H__
#define __BBM_UTIL_H__

#include "nandx_util.h"

#define ADDR2BLOCK(addr, bs_shift)	(addr >> bs_shift)
#define BLOCK2ADDR(block, bs_shift)	(block << bs_shift)
#define ADDR2BLOCKOFFS(addr, bs_shift)	(addr & GENMASK((bs_shift - 1), 0))

static u16 crc16(const u8* data, u32 length)
{
	u8 x;
	u16 crc = 0xFFFF;

	while (length--) {
		x = crc >> 8 ^ *data++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((u8)(x << 12)) ^ ((u8)(x <<5)) ^ ((u8)x);
	}
	return crc;
}

static inline u32 log2(u64 n)
{
	int i = 0;

	while (n > 1) {
		n = n >> 1;
		i++;
	}

	return i;
}

#endif /*__BBM_UTIL_H__*/
