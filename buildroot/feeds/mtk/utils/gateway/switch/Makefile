# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
SOURCE_DIR:=$(TOPDIR)/../mtk/package/utils/gateway/switch
PKG_NAME:=switch
PKG_RELEASE := 1
PKG_FILE_DEPENDS := $(SRC_DIR)
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

USE_SOURCE_DIR := $(SOURCE_DIR)

define Package/switch
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  DEPENDS:=+libnl-tiny
  TITLE:=Switch cmd
  SUBMENU:=Applications
  MENU:=1
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Package/hwnat/description
  An program to query info from ethernet driver
endef
TARGET_CPPFLAGS := \
	-D_GNU_SOURCE \
	-I$(LINUX_DIR)/user_headers/include \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \

TARGET_CFLAGS += -I$(TOPDIR)/../mtk/ext_kernel-4.19/drivers/net/phy/mtk/mt753x
TARGET_CFLAGS += -I$(TOPDIR)/../mtk/package/kernel/netsys_driver/raeth/raeth
TARGET_CFLAGS += -DCONFIG_SUPPORT_OPENWRT
TARGET_CFLAGS += -DCONFIG_MT753X_GSW

define Build/Compile
	CFLAGS="$(TARGET_CPPFLAGS) $(TARGET_CFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) OUT=$(PKG_BUILD_DIR) CC="$(TARGET_CC)" \
	LIBS="$(TARGET_LDFLAGS) -lnl-tiny -lm" \
	CONFIG_SUPPORT_OPENWRT=y CONFIG_MT753X_GSW=y
endef

define Package/switch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/switch $(1)/usr/bin
endef

$(eval $(call BuildPackage,switch))
