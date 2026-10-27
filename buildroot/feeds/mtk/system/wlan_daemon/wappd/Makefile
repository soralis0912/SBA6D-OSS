#
# hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

SRC_DIR:=$(TOPDIR)/../mtk/package/system/wlan_daemon/wappd

PKG_NAME:=wappd
PKG_RELEASE:=1.0
PKG_SOURCE:=$(PKG_NAME).tar.gz
PKG_FILE_DEPENDS:=$(SRC_DIR)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

TARGET_CFLAGS += -DCONFIG_SUPPORT_OPENWRT
MAKE_FLAGS += CONFIG_SUPPORT_OPENWRT=y
MAKE_FLAGS += UAPD_INCLUDE_PATH=$(LINUX_DIR)/include/uapi/linux/wapp
MAKE_FLAGS += NL80211_INCLUDE_PATH=$(LINUX_DIR)/include/uapi/linux/mtk_nl80211_inc
TARGET_CFLAGS += -DWIFI_MD_COEX_SUPPORT

define Package/wapp
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=wapp daemon
  SUBMENU:=Applications
  DEPENDS:= +USE_UCLIBC:libpthread +USE_UCLIBC:librt +libkvcutil +libopenssl +libuci +libnl-tiny +WAPP_WPACTRL_SUPPORT:libwpactrl
endef

define Package/wapp/description
  wifi app daemon
endef

define Package/wapp/config
	source "$(SOURCE)/Config.in"
endef


TARGET_CFLAGS += \
	-D_GNU_SOURCE \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-I$(PKG_BUILD_DIR) \


ifeq ($(CONFIG_WAPP_WPACTRL_SUPPORT), y)
MAKE_FLAGS += CONFIG_WPACTRL_SUPPORT=y
TARGET_CFLAGS += \
	-DWPACTRL_SUPPORT \
	-I$(STAGING_DIR)/usr/include/wpactrl
endif
define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	$(if $(wildcard $(SRC_DIR)/*),,@echo "Error: SRC_DIR=$(SRC_DIR) path not found"; false)
	cp -rf $(SRC_DIR) $(PKG_BUILD_DIR)
	rm -rf $(PKG_BUILD_DIR)/.git
	$(Build/Patch)
	touch $(PKG_BUILD_DIR)/.source_dir
	touch $(PKG_BUILD_DIR)/.no_autorebuild
endef

define Package/wapp/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wappctrl $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wapp $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wapp_openwrt.sh $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wifi_config_save.lua $(1)/usr/bin/wifi_config_save
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mbo_nr.sh $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc
	cp $(PKG_BUILD_DIR)/config_and_icon_files/* $(1)/etc
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) files/wapp.init $(1)/etc/init.d/wapp
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) files/wapp.config $(1)/etc/config/wapp
ifeq ($(CONFIG_MTK_WIFI7_MAP_HOSTAPD_SUPPORT), y)
	$(INSTALL_DATA) files/1905d_cfg $(1)/etc/config/1905d_cfg
	$(INSTALL_DATA) files/mapd $(1)/etc/config/mapd
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_BIN) files/uci2map.lua $(1)/etc/uci2map.lua
	$(INSTALL_DATA) files/mapd.uci $(1)/etc/mapd.uci
endif
endef

$(eval $(call BuildPackage,wapp))

