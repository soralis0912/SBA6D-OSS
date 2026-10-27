MFD_TOP = $(srctree)/drivers/mfd

#Old GKI
PLAT_TOP = $(srctree)/drivers/misc/mediatek/include/mt-plat

ifeq ($(CONFIG_MTK_CLKBUF_BBLPM),y)
    ccflags-y += -D BBLPM_SUPPORT=1
else
    ccflags-y += -D BBLPM_SUPPORT=0
endif

ifeq ($(CONFIG_MTK_CLKBUF_NFC),y)
    ccflags-y += -D NFC_CLKBUF_SUPPORT=1
else
    ccflags-y += -D NFC_CLKBUF_SUPPORT=0
endif

ifneq (,$(filter y m, $(CONFIG_MTK_CLK_BUFFER)))
    ccflags-y += -I$(src)/src \
	     -I$(srctree)/include/ \
	     -I$(MFD_TOP)/ \
	     -I$(PLAT_TOP)/

    obj-$(CONFIG_MTK_CLK_BUFFER) += clkbuf.o

    clkbuf-$(CONFIG_MTK_CLK_BUFFER) += mtk-clkbuf-bridge.o \
        src/mtk-clkbuf-pmic.o \
        src/mtk-clkbuf-ctl.o \
        src/mtk-clkbuf-hw.o

endif


# New GKI
# common clkbuf driver
obj-$(CONFIG_MTK_CLKBUF) += clkbuf.o
ifneq ($(CONFIG_MTK_CLKBUF),)
	clkbuf-y += v1/src/mtk_clkbuf_common.o
	ccflags-y += -I$(srctree)/$(src)/v1/inc
endif

clkbuf-$(CONFIG_MTK_CLKBUF_V1) += v1/src/mtk_clkbuf_ctl.o
clkbuf-$(CONFIG_MTK_CLKBUF_V1) += v1/src/mtk-clkbuf-pmif.o

clkbuf-$(CONFIG_MTK_CLKBUF_V1) += v1/src/mtk-clkbuf-dcxo.o
clkbuf-$(CONFIG_MTK_CLKBUF_V1) += v1/src/mtk-clkbuf-dcxo-6359p.o
clkbuf-$(CONFIG_MTK_CLKBUF_V1) += v1/src/mtk-clkbuf-dcxo-6685.o
clkbuf-$(CONFIG_MTK_CLKBUF_V1) += v1/src/mtk-clkbuf-dcxo-6366.o

clkbuf-$(CONFIG_MTK_SRCLKEN_RC_V1) += v1/src/mtk-srclken-rc-hw.o
clkbuf-$(CONFIG_MTK_SRCLKEN_RC_V1) += v1/src/mtk-srclken-rc-hw-v1.o
ccflags-$(CONFIG_MTK_SRCLKEN_RC_V1) += -D SRCLKEN_RC_SUPPORT
