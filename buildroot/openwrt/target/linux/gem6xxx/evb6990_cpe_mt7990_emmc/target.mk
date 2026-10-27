#
# Copyright (C) 2009 OpenWrt.org
#
ARCH:=aarch64
SUBTARGET:=evb6990_cpe_mt7990_emmc
BOARDNAME:=MT6990 CPE EMMC EVB development (64 bits)
FEATURES+=squashfs
CPU_TYPE:=cortex-a55
CPU_SUBTYPE:=neon-vfpv4

KERNELNAME:=Image.gz dtbs

define Target/Description
	Build firmware images for MT6990 CPE EMMC EVB development
endef

DEVICE_TYPE:=router
