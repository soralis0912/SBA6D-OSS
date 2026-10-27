/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#ifndef __NANDX_UTIL_H__
#define __NANDX_UTIL_H__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

enum nand_irq_return {
	NAND_IRQ_NONE,
	NAND_IRQ_HANDLED,
};

enum nand_dma_operation {
	NDMA_FROM_DEV,
	NDMA_TO_DEV,
};

#include "nandx_errno.h"

/*
 * Compatible function
 * used for preloader/lk/kernel environment
 */
#include "nandx_os.h"

#ifndef BIT
#define BIT(a)                  (1 << (a))
#endif

#ifndef min
#define min(a, b)   (((a) > (b)) ? (b) : (a))
#define max(a, b)   (((a) < (b)) ? (b) : (a))
#endif

#ifndef GENMASK
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> ((sizeof(unsigned long) * 8) - 1 - (h))))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#ifndef __weak
#define __weak __attribute__((__weak__))
#endif

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef KB
#define KB(x)   ((x) << 10)
#define MB(x)   (KB(x) << 10)
#define GB(x)   (MB(x) << 10)
#endif

#ifndef offsetof
#define offsetof(type, member) ((size_t)&((type *)0)->member)
#endif

#ifndef NULL
#define NULL (void *)0
#endif
static inline u32 nandx_popcount(u32 x)
{
	x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
	x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
	x = (x & 0x0000FFFF) + ((x >> 16) & 0x0000FFFF);

	return x;
}

#ifndef zero_popcount
#define zero_popcount(x) (32 - nandx_popcount(x))
#endif

#ifndef do_div
#define do_div(n, base) \
	({ \
		u32 __base = (base); \
		u32 __rem; \
		__rem = ((u64)(n)) % __base; \
		(n) = ((u64)(n)) / __base; \
		__rem; \
	})
#endif

#define div_up(x, y) \
	({ \
		u64 __temp = ((x) + (y) - 1); \
		do_div(__temp, (y)); \
		__temp; \
	})

#define div_down(x, y) \
	({ \
		u64 __temp = (x); \
		do_div(__temp, (y)); \
		__temp; \
	})

#define div_round_up(x, y)      (div_up(x, y) * (y))
#define div_round_down(x, y)    (div_down(x, y) * (y))

#define reminder(x, y) \
	({ \
		u64 __temp = (x); \
		do_div(__temp, (y)); \
	})

#ifndef round_up
#define round_up(x, y)          ((((x) - 1) | ((y) - 1)) + 1)
#define round_down(x, y)        ((x) & ~((y) - 1))
#endif

#ifndef readx_poll_timeout_atomic
#define readx_poll_timeout_atomic(op, addr, val, cond, delay_us, timeout_us) \
	({ \
		u64 end = get_current_time_us() + timeout_us; \
		for (;;) { \
			u64 now = get_current_time_us(); \
			(val) = op(addr); \
			if (cond) \
				break; \
			if (now > end) { \
				(val) = op(addr); \
				break; \
			} \
		} \
		(cond) ? 0 : -ETIMEDOUT; \
	})

#define readl_poll_timeout_atomic(addr, val, cond, delay_us, timeout_us) \
	readx_poll_timeout_atomic(readl, addr, val, cond, delay_us, timeout_us)
#define readw_poll_timeout_atomic(addr, val, cond, delay_us, timeout_us) \
	readx_poll_timeout_atomic(readw, addr, val, cond, delay_us, timeout_us)
#define readb_poll_timeout_atomic(addr, val, cond, delay_us, timeout_us) \
	readx_poll_timeout_atomic(readb, addr, val, cond, delay_us, timeout_us)
#endif

struct nandx_split64 {
	u64 head;
	size_t head_len;
	u64 body;
	size_t body_len;
	u64 tail;
	size_t tail_len;
};

struct nandx_split32 {
	u32 head;
	u32 head_len;
	u32 body;
	u32 body_len;
	u32 tail;
	u32 tail_len;
};

#define nandx_split(split, offset, len, val, align) \
	do { \
		(split)->head = (offset); \
		(val) = div_round_down((offset), (align)); \
		(val) = (align) - ((offset) - (val)); \
		if ((val) == (align)) \
			(split)->head_len = 0; \
		else if ((val) > (len)) \
			(split)->head_len = len; \
		else \
			(split)->head_len = val; \
		(split)->body = (offset) + (split)->head_len; \
		(split)->body_len = div_round_down((len) - \
						   (split)->head_len,\
						   (align)); \
		(split)->tail = (split)->body + (split)->body_len; \
		(split)->tail_len = (len) - (split)->head_len - \
				    (split)->body_len; \
	} while (0)

#ifndef container_of
#define container_of(ptr, type, member) \
	({const __typeof__(((type *)0)->member) * __mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); })
#endif

static inline u32 nandx_cpu_to_be32(u32 val)
{
	u32 temp = 1;
	u8 *p_temp = (u8 *)&temp;

	if (*p_temp)
		return (((val & 0xff) << 24) | ((val & 0xff00) << 8) |
				((val >> 8) & 0xff00) | ((val >> 24) & 0xff));

	return val;
}

static inline void nandx_set_bits32(unsigned long addr, u32 mask,
				    u32 val)
{
	u32 temp = readl((void *)addr);

	temp &= ~(mask);
	temp |= val;
	writel(temp, (void *)addr);
}

#endif /* __NANDX_UTIL_H__ */
