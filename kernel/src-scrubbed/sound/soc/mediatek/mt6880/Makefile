# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE := y
endif

# platform driver
snd-soc-mt6880-afe-objs := \
	mt6880-afe-pcm.o \
	mt6880-afe-clk.o \
	mt6880-afe-gpio.o \
	mt6880-dai-adda.o \
	mt6880-afe-control.o \
	mt6880-dai-i2s.o \
	mt6880-dai-hw-gain.o \
	mt6880-dai-src.o \
	mt6880-dai-pcm.o \
	mt6880-dai-tdm.o \
	mt6880-dai-hostless.o \
	mt6880-misc-control.o \
	mt6880-dai-etdm.o

obj-$(CONFIG_SND_SOC_MT6880) += snd-soc-mt6880-afe.o

# machine driver
obj-$(CONFIG_SND_SOC_MT6880_MT6359) += mt6880-mt6359.o

