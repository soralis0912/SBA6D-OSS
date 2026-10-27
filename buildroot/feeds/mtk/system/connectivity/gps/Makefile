#
# Copyright (C) 2011-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mnld
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/connectivity/gps
PKG_FILE_DEPENDS:=$(SOURCE_DIR)

include $(INCLUDE_DIR)/package.mk

MTK_PLATFORM := $(call qstrip,$(CONFIG_MTK_PLATFORM))
  $(warning "platform is $(MTK_PLATFORM)")

ifeq ($(CONFIG_MTK_GPS_DBG_SOCKET_SUPPORT),y)
MAKE_FLAGS += \
	CONFIG_MTK_GPS_DBG_SOCKET_SUPPORT=y
endif

ifeq ($(CONFIG_MTK_GPS_OFL_SUPPORT),y)
MAKE_FLAGS += \
	CONFIG_MTK_GPS_OFL_SUPPORT=y
endif

ifeq ($(CONFIG_MTK_GNSS_TMD_SUPPORT),y)
LIBMNL_PLAT := MT6980D
MAKE_FLAGS += \
	CONFIG_MTK_GNSS_TMD_SUPPORT=y
else
LIBMNL_PLAT := MT6980
endif

ifeq (mt6880,$(MTK_PLATFORM))
  MAKE_FLAGS += PLATFORM_6880_6890=y
else ifeq (mt6890,$(MTK_PLATFORM))
  MAKE_FLAGS += PLATFORM_6880_6890=y
endif

ifeq (mt6980,$(MTK_PLATFORM))
  MAKE_FLAGS += PLATFORM_6980_6990=y
else ifeq (mt6990,$(MTK_PLATFORM))
  MAKE_FLAGS += PLATFORM_6980_6990=y
endif

define Package/mnld
  SECTION  := MTK Properties
  CATEGORY := MTK Properties
  SUBMENU  := Applications
  TITLE    := mnld
  DEPENDS  := +libc +librt +libpthread +zlib +libcurl +libopenssl +libmnl_gnss +mipc +MTK_GNSS_TMD_SUPPORT:libDR
endef

define Package/mnld/description
 mnld is the Deamon for MediaTek Navigation Library.
endef

define Package/mnld/config
 source "$(SOURCE)/Config.in"
endef

define Package/libmnl_gnss
  SECTION  := MTK Properties
  CATEGORY := MTK Properties
  SUBMENU  := Libraries
  TITLE    := libmnl_gnss
  HIDDEN   := 1
endef

define Package/libmnl_gnss/description
 libmnl_gnss is the algorithm library of MediaTek GNSS.
endef

define Package/libDR
  SECTION  := MTK Properties
  CATEGORY := MTK Properties
  SUBMENU  := Libraries
  TITLE    := libDR
  DEPENDS  := +libstdcpp
  HIDDEN   := 1
endef

define Package/libDR/description
 libDR is the algorithm library of MediaTek GNSS MPE.
endef

define Package/mnld_test
  SECTION  := MTK Properties
  CATEGORY := MTK Properties
  SUBMENU  := Applications
  TITLE    := mnld_test
  DEPENDS  := +libgnsshal
  HIDDEN   := 1
endef

define Package/mnld_test/description
 mnld_test is a process for mnld interface test.
endef

define Package/fm_gnss
  SECTION  := MTK Properties
  CATEGORY := MTK Properties
  SUBMENU  := Applications
  TITLE    := fm_gnss
  DEPENDS  := +libgnsshal
  HIDDEN   := 1
endef

define Package/fm_gnss/description
 fm_gnss is a process for GNSS factory mode test.
endef

define Package/libgnsshal
  SECTION  := MTK Properties
  CATEGORY := MTK Properties
  SUBMENU  := Libraries
  TITLE    := libgnsshal
  HIDDEN   := 1
endef

define Package/libgnsshal/description
 libgnsshal is a share library for MediaTek GNSS driver interface.
endef

# Package preparation instructions; create the build directory and copy the source code.
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	test -d $(PKG_BUILD_DIR)/mtk_mnld/files/$(SUBTARGET) && cp -r $(PKG_BUILD_DIR)/mtk_mnld/files/$(SUBTARGET)/* $(PKG_BUILD_DIR)/mtk_mnld/files/ || echo "No project config files"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_DIR) $(1)/usr/bin
	mkdir -p $(1)/usr/include
	cp -rf $(PKG_BUILD_DIR)/gps_hal/inc/* $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/mtk_mnld/mnl/openwrt/$(LIBMNL_PLAT)/$(ARCH)/libmnl_gnss.so $(1)/usr/lib/libmnl_gnss.so.0
	ln -s libmnl_gnss.so.0 $(1)/usr/lib/libmnl_gnss.so
ifeq ($(CONFIG_MTK_GNSS_TMD_SUPPORT),y)
	$(CP) $(PKG_BUILD_DIR)/mtk_mnld/mnl/openwrt/$(LIBMNL_PLAT)/$(ARCH)/libDR.so $(1)/usr/lib/libDR.so.0
	ln -s libDR.so.0 $(1)/usr/lib/libDR.so
endif
	$(CP) $(PKG_BUILD_DIR)/gps_hal/libgnsshal.so $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/mtk_mnld/mnld $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gnss_test/mnld_test $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gnss_test/fm_gnss $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gpslog/gpslog $(1)/usr/bin/
endef

define Package/mnld/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/gnss
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtk_mnld/files/mnld.init $(1)/etc/init.d/mnld
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtk_mnld/files/MNL_Config.xml $(1)/etc/MNL_Config.xml
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gps_hal/libgnsshal.so $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/mtk_mnld/mnld $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gnss_test/mnld_test $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gnss_test/fm_gnss $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gpslog/gpslog $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/gps_hal/libgnsshal.so $(1)/usr/lib/
endef

define Package/libmnl_gnss/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(SOURCE_DIR)/mtk_mnld/mnl/openwrt/$(LIBMNL_PLAT)/$(ARCH)/libmnl_gnss.so $(1)/usr/lib/libmnl_gnss.so.0
	ln -s libmnl_gnss.so.0 $(1)/usr/lib/libmnl_gnss.so
endef

define Package/libDR/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(SOURCE_DIR)/mtk_mnld/mnl/openwrt/$(LIBMNL_PLAT)/$(ARCH)/libDR.so $(1)/usr/lib/libDR.so.0
	ln -s libDR.so.0 $(1)/usr/lib/libDR.so
endef

$(eval $(call BuildPackage,mnld))
$(eval $(call BuildPackage,libmnl_gnss))
$(eval $(call BuildPackage,libDR))
$(eval $(call BuildPackage,mnld_test))
$(eval $(call BuildPackage,fm_gnss))
$(eval $(call BuildPackage,libgnsshal))
