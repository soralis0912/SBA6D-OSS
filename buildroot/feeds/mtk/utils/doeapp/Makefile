include $(TOPDIR)/rules.mk
# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=$(TOPDIR)/../mtk/package/utils/doeapp/dconfig
# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=dconfig
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=
PKG_FILE_DEPENDS := $(SOURCE_DIR)

#include $(INCLUDE_DIR)/uclibc++.mk
include $(INCLUDE_DIR)/package.mk


define Package/dconfig
	SECTION:=MTK Properties
	CATEGORY:=MTK Properties
	TITLE:=dconfig
	SUBMENU:=Applications
	DEPENDS:=+libstdcpp +libblkid +libxml2 +dtc
	MENU:=1
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Package/dconfig/description
	doeapp/dconfig -utils.
endef

TARGET_CFLAGS += \
  -I$(STAGING_DIR)/usr/include \
  -I$(STAGING_DIR)/usr/include/libxml2 \
  -D_LINUX

TARGET_LDFLAGS+= -L$(STAGING_DIR)/usr/lib/
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LIBS="$(TARGET_LIBS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/dconfig/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dconfig $(1)/usr/bin
endef

$(eval $(call BuildPackage,dconfig))
