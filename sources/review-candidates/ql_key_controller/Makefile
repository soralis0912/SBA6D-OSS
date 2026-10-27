include $(TOPDIR)/rules.mk

ifeq (,$(wildcard $(TOPDIR)/../quectel/package/ql_key_controller/src))

SOURCE_DIR:=$(TOPDIR)/../quectel/package/utils/ql_key_controller

PKG_NAME:=ql_key_controller
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_FILE_DEPENDS := 
PKG_LICENSE:=GPL-2.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

# Where to deploy the built binaries or headers
PKG_PREBUILT_DIR:=$(PREBUILT_QL_DIR)/utils/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ql_key_controller
	SECTION:=Quectel Properties
	CATEGORY:=Quectel Properties
	SUBMENU:=Applications
	DEPENDS:=+ql_wifi_api
	TITLE:= Quectel key management daemon  
endef

define Package/ql_key_controller/description
	Quectel key management daemon  
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -ar $(SOURCE_DIR)/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	LIBS="$(TARGET_LIBS)" \
	$(MAKE) -C $(PKG_BUILD_DIR)/custom
endef

define Package/ql_key_controller/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/custom/libql_custom_key_handler.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_PREBUILT_DIR)/ql_key_controller $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(SOURCE_DIR)/files/ql_key_controller.init $(1)/etc/init.d/ql_key_controller
	$(INSTALL_DIR) $(1)/etc/ql_config
	$(INSTALL_BIN) $(SOURCE_DIR)/files/ql_keyctrl_cfg $(1)/etc/ql_config/ql_keyctrl_cfg
endef

$(eval $(call BuildPackage,ql_key_controller))
endif
