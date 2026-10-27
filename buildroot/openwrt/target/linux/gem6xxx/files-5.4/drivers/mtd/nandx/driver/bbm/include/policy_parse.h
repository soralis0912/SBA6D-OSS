/*
 * Copyright (C) 2022 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __POLICY_PARSE__
#define __POLICY_PARSE__

#define PARTITION_FORMAT_PMT 1

#define PARTITION_MAX_COUNT 64
#define MAX_PARTITION_NAME_LEN 64

int partition_policy_parse(struct bbm_nand *device, struct part_policy *parts, u32 *part_count);
#endif /*__POLICY_PARSE__*/
