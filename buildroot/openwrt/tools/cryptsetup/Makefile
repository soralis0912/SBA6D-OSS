#
# Copyright (C) 2010-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cryptsetup
PKG_VERSION_MAJOR:=2.0
PKG_VERSION:=$(PKG_VERSION_MAJOR).6
PKG_HASH:=7c51fae0f0e7ea9af0f515b2ac77009fb2969a6619ebab47d097dca38b083d30
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@KERNEL/linux/utils/cryptsetup/v$(PKG_VERSION_MAJOR)/

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_ARGS += \
	--with-crypto-backend=openssl \
	--disable-kernel_crypto \
	--disable-blkid

$(eval $(call HostBuild))
