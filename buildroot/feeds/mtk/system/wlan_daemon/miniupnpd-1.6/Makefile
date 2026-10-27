#
# hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

SRC_DIR:=$(TOPDIR)/../mtk/package/system/wlan_daemon/miniupnpd-1.6

PKG_NAME:=mtk_miniupnpd
PKG_RELEASE:=1.0
PKG_SOURCE:=$(PKG_NAME).tar.gz
PKG_FILE_DEPENDS:=$(SRC_DIR)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=RALINK_MT7620 RALINK_MT7621 RALINK_MT7628
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/mtk_miniupnpd
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=Miniupnpd Daemon
  SUBMENU:=Applications
  DEPENDS:= +libip4tc +libpthread +librt
endef

define Package/mtk_miniupnpd/description
  Miniupnpd Daemon.
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
		
TARGET_CFLAGS += \
	$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),-DCONFIG_$(c)=$(CONFIG_$c)))

MAKE_FLAGS += \
	$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)=$(CONFIG_$c))) 

TARGET_CFLAGS += -DCONFIG_SUPPORT_OPENWRT
MAKE_FLAGS += CONFIG_SUPPORT_OPENWRT=y
ifeq ($(CONFIG_LINUX_4_4),y)
MAKE_FLAGS += CONFIG_DEFAULTS_KERNEL_4_4=y
endif

MAKE_FLAGS += MASK_PARTIAL_MACADDR=y

define Package/mtk_miniupnpd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/miniupnpd $(1)/usr/bin
endef


$(eval $(call BuildPackage,mtk_miniupnpd))

