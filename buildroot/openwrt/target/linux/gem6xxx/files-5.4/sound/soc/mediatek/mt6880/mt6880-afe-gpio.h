/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Eason Yen <eason.yen@mediatek.com>
 */

#ifndef _MT6880_AFE_GPIO_H_
#define _MT6880_AFE_GPIO_H_

struct mtk_base_afe;

int mt6880_afe_gpio_init(struct mtk_base_afe *afe);

int mt6880_afe_gpio_request(struct mtk_base_afe *afe, bool enable,
			    int dai, int uplink);

#endif
