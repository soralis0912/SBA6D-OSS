# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2016 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.
#
ccflags-y += -I$(srctree)/drivers/misc/mediatek/ethernet/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/ppe/

obj-$(CONFIG_NET_MEDIATEK_SOC_WED)	+= mtk_wed.o \
					   mtk_wed_debugfs.o \
					   mtk_wed_ops.o \
					   mtk_wed_wo.o \
					   mtk_wed_mcu.o \
					   mtk_wed_ccif.o


