include $(TOPDIR)/rules.mk

PKG_NAME:=ccci_fsd
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_FLAGS:=nonshared

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/ccci_fsd/src
PKG_FILE_DEPENDS := $(SOURCE_DIR)

include $(INCLUDE_DIR)/package.mk

define Package/ccci_fsd
  SECTION:=sys
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
  TITLE:=ccci_fsd
  MAINTAINER:=Yanjie Jiang <yanjie.jiang@mediatek.com>
  DEPENDS:= +mtk_nvram +libubox
endef

define Package/ccci_fsd/description
 This package for md fsd access.
endef

define Build/Configure
endef

# Package preparation instructions; create the build directory and copy the source code.
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
		mkdir -p $(PKG_BUILD_DIR)
		cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/ccci_fsd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(TOPDIR)/../mtk/package/system/ccci_fsd/files/ccci_fsd.init $(1)/etc/init.d/2ccci_fsd
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ccci_fsd $(1)/usr/bin
endef

$(eval $(call BuildPackage,ccci_fsd))
