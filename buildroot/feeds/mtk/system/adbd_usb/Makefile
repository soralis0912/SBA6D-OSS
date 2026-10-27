include $(TOPDIR)/rules.mk

SRC_DIR:=$(TOPDIR)/../mtk/package/system/adb

PKG_NAME:=adbd_usb
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_FILE_DEPENDS := $(SRC_DIR)
export OPENWRT_BUILD_ADBD := 1
export ADB_OVER_USB := 1
export UCI_PROPERTY_SUPPORT := 1

include $(INCLUDE_DIR)/package.mk

define Package/adbd_usb
  SECTION:=sys
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
  TITLE:=adbd (USB)
  MAINTAINER:=Jeremy Chou <jeremy.chou@mediatek.com>
  DEPENDS:=+uci +libuci +ubox +libubox
endef

define Package/adbd_usb/config
	source "$(SOURCE)/Config.in"
endef

define Package/adbd_usb/description
	This package is for adbd (USB)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -rf $(SRC_DIR)/* $(PKG_BUILD_DIR)
endef

ifeq (y,$(CONFIG_MTK_ADB_ENABLE))
    export ADB_SERVICE_ENABLE := 1
endif

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include

TARGET_LIBS += \
	-luci \
	-lubox \
	-lpthread \
	-lrt

TARGET_LDFLAGS += \
	-L$(STAGING_DIR)/usr/lib

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
			CFLAGS="$(TARGET_CFLAGS)" \
			CC=$(TARGET_CC) \
			CXX=$(TARGET_CXX) \
			LD=$(TARGET_LD) \
			AR=$(TARGET_AR) \
			HOST=${ARCH} all \
			LIBS="$(TARGET_LIBS)" \
			LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/adbd_usb/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_BUILD_DIR)/adbd $(1)/sbin/adbd_usb
endef

$(eval $(call BuildPackage,adbd_usb))
