#
# Copyright (C) 2010-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lvm2
PKG_VERSION:=2.02.181
PKG_HASH:=400fead33b3abc2d82bd631b63f644b646e83040699f2e8f91ff5779119bb89e
PKG_RELEASE:=1

PKG_SOURCE:=LVM2.$(PKG_VERSION).tgz
PKG_SOURCE_URL:=ftp://sources.redhat.com/pub/lvm2/old

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_ARGS += \
        --enable-write_install \
        --enable-pkgconfig \
        --disable-cmdlib \
        --disable-dmeventd \
        --disable-applib \
        --disable-fsadm \
        --disable-readline \
        --disable-selinux

# lvm2 unpacks in the wrong folder
define Host/Prepare
	$(call Host/Prepare/Default)
	mv $(HOST_BUILD_DIR)/../LVM2.$(PKG_VERSION)/* $(HOST_BUILD_DIR)/
	rmdir $(HOST_BUILD_DIR)/../LVM2.$(PKG_VERSION)
endef

define Host/Compile
	$(call Host/Compile/Default,device-mapper)
endef

define Host/Install
	$(call Host/Compile/Default,install_device-mapper)
endef

$(eval $(call HostBuild))
