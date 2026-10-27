include $(TOPDIR)/rules.mk

SOURCE_DIR:=$(TOPDIR)/../mtk/package/utils/mrdump/mrdump_tool

PKG_NAME:=mrdump_tool
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=zlib
PKG_FILE_DEPENDS := $(SOURCE_DIR)

include $(INCLUDE_DIR)/package.mk

define Package/mrdump_tool
	SECTION:=MTK Properties
	CATEGORY:=MTK Properties
	TITLE:=mrdump tools
	SUBMENU:=Applications
	DEPENDS:=+zlib +libnandapi
	MENU:=1
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Package/mrdump_tool/description
	mrdump tool and host command -utils.
endef

TARGET_CFLAGS += \
  -I$(STAGING_DIR)/usr/include \
  -D_LINUX

TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib/
TARGET_LIBS = -Bdynamic -lnandapi -lz

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) OUT=$(PKG_BUILD_DIR) CC="$(TARGET_CC)" CONFIG_SUPPORT_OPENWRT=y CFLAGS="$(TARGET_CFLAGS) $(TARGET_LIBS)"
endef

define Package/mrdump_tool/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mrdump_tool $(1)/usr/bin
endef

$(eval $(call BuildPackage,mrdump_tool))
