/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __BBM_H__
#define __BBM_H__

#include "bbm_core.h"

int bbm_init(const struct bbm_nand *nand, bool is_da, bool is_preloader);
int bbm_read(u8 *data, u8 *oob, u64 offset, size_t len);
int bbm_write(u8 *data, u8 *oob, u64 offset, size_t len);
int bbm_erase(u64 offset, size_t len);
int bbm_is_bad(u64 offset);
int bbm_mark_bad(u64 offset);

#endif /*__BBM_H__*/
