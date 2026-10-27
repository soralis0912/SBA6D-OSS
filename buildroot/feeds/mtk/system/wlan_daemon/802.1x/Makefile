#
# hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

SRC_DIR:=$(TOPDIR)/../mtk/package/system/wlan_daemon/802.1x

PKG_NAME:=8021xd
PKG_RELEASE:=1.0
PKG_SOURCE:=$(PKG_NAME).tar.gz
PKG_FILE_DEPENDS:=$(SRC_DIR)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=RALINK_MT7620 RALINK_MT7621 RALINK_MT7628
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/8021xd
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=802.1X Daemon
  SUBMENU:=Applications
endef

define Package/8021xd/description
  802.1X Daemon
endef

MTK_RELEASE_VERSION:=trunk
ifeq ($(MTK_RELEASE_VERSION),trunk)
define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	$(if $(wildcard $(SRC_DIR)/*),,@echo "Error: SRC_DIR=$(SRC_DIR) path not found"; false)
	cp -rf $(SRC_DIR) $(PKG_BUILD_DIR)
	rm -rf $(PKG_BUILD_DIR)/.git
	$(Build/Patch)
endef
endif

TARGET_CFLAGS += -DMASK_PARTIAL_MACADDR
TARGET_CFLAGS += -DCONFIG_SUPPORT_OPENWRT
MAKE_FLAGS += CONFIG_SUPPORT_OPENWRT=y
MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Package/8021xd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/8021xd $(1)/usr/bin
endef


$(eval $(call BuildPackage,8021xd))

