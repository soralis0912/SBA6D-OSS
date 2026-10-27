# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for kernel SPMI framework.
#
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
obj-$(CONFIG_SPMI)	+= spmi.o

obj-$(CONFIG_SPMI_MSM_PMIC_ARB)	+= spmi-pmic-arb.o
obj-$(CONFIG_SPMI_MTK_PMIF)	+= spmi-mtk-pmif.o spmi-mtk-dbg.o
obj-$(CONFIG_SPMI_MTK_PMIF_V2)	+= spmi-mtk-pmif_v2.o spmi-mtk-dbg_v2.o
