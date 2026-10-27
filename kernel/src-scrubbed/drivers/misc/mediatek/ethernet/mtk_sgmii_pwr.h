// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.
/* A library for MediaTek HWLRO
 *
 * Author: Stella Yang <stella.yang@mediatek.com>
 *
 */

#ifndef MTK_SGMII_PWR_H
#define MTK_SGMII_PWR_H

/*=========================================
 *    SGMII power define
 *=========================================
 */
struct mtk_gmii {
	int	id;
	int	type;
	struct device			*dev;
	bool enable;
};

int sgmii_runtime_control(int id, bool enable);

#endif /* MTK_SGMII_PWR_H */
