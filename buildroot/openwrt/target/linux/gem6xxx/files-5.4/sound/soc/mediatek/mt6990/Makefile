# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE := y
endif

# platform driver
snd-soc-mt6990-afe-objs := \
	mt6990-afe-pcm.o \
	mt6990-afe-clk.o \
	mt6990-afe-gpio.o \
	mt6990-dai-adda.o \
	mt6990-afe-control.o \
	mt6990-dai-i2s.o \
	mt6990-dai-hw-gain.o \
	mt6990-dai-src.o \
	mt6990-dai-pcm.o \
	mt6990-dai-tdm.o \
	mt6990-dai-hostless.o \
	mt6990-misc-control.o \
	mt6990-dai-etdm.o

obj-$(CONFIG_SND_SOC_MT6990) += snd-soc-mt6990-afe.o

# machine driver
obj-$(CONFIG_SND_SOC_MT6990_SLIC) += mt6990-slic.o
obj-$(CONFIG_SND_SOC_MT6990_AK4940) += mt6990-ak4940.o

