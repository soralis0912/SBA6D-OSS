# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE := y
endif

obj-$(CONFIG_MTK_DEVAPC) := devapc.o

# Core
devapc-$(CONFIG_DEVAPC_ARCH_MULTI) += devapc-mtk-multi-ao.o
devapc-$(CONFIG_DEVAPC_COMMON_V1) += devapc-common_v1.o

# Platform
devapc-$(CONFIG_DEVAPC_MT6880) += devapc-mt6880.o
devapc-$(CONFIG_DEVAPC_MT69XX) += devapc-mt69xx.o