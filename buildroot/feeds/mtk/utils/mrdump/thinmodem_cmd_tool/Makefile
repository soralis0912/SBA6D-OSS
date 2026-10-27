include $(TOPDIR)/rules.mk

SOURCE_DIR:=$(TOPDIR)/../mtk/package/utils/mrdump/thinmodem_cmd_tool

PKG_NAME:=thinmodem_cmd_tool
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=
PKG_FILE_DEPENDS := $(SOURCE_DIR)

HOST_BUILD_PARALLEL:=1
HOST_CONFIGURE_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/thinmodem_cmd_tool
	SECTION:=MTK Properties
	CATEGORY:=MTK Properties
	TITLE:=thinmodem host cmd utils
	SUBMENU:=Applications
	DEPENDS:=
	MENU:=1
endef

define Package/thinmodem_cmd_tool/description
	thinmodem host cmd utils.
endef

define Host/Prepare
	mkdir -p $(HOST_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(HOST_BUILD_DIR)
endef

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) CC="$(HOSTCC)"
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOSTPKG)/bin
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/out/thinmodem_cmd_tool $(STAGING_DIR_HOSTPKG)/bin
endef

define Host/Clean
	rm -rf $(HOST_BUILD_DIR)
endef

$(eval $(call BuildPackage,thinmodem_cmd_tool))
$(eval $(call HostBuild))
