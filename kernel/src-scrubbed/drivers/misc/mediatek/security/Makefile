# SPDX-License-Identifier: GPL-2.0

ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/

#ifeq ($(CONFIG_MTK_SECURE_DATA_PROTECTION), y)
obj-$(CONFIG_MTK_SECURE_DATA_PROTECTION) += secure_dp.o
#endif

#ifeq ($(CONFIG_MTK_SECURE_STATUS_QUERY), y)
obj-$(CONFIG_MTK_SECURE_STATUS_QUERY) += secure_status.o
#endif

