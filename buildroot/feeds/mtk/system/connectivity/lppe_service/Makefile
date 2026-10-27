#
# Hugo.Chang@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lppe_service
PKG_RELEASE:=1

SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/connectivity/lppe_service/src

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/lppe_service
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=LPPe service daemon
  SUBMENU:=Applications
  DEPENDS:=+libstdcpp +mipc
endef

define Package/lppe_service/config
	source "$(SOURCE)/Config.in"
endef

ifeq (y,$(CONFIG_MTK_LPPE_NLP_SUPPORT))
    export MTK_LPPE_NLP_SUPPORT := 1
endif

define Package/lppe_service/description
	This is LPPe Service daemon
endef

define Build/Prepare
		mkdir -p $(PKG_BUILD_DIR)
		cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef


define Package/lppe_service/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(TOPDIR)/../mtk/package/system/connectivity/lppe_service/files/lppe_service.init $(1)/etc/init.d/lppe_service
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nlp_impl/libmock_nlp_impl.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lppe_service $(1)/usr/bin/lppe_service
endef

$(eval $(call BuildPackage,lppe_service))

