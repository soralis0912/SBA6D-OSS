#
# Hugo.Chang@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lbs_em
PKG_RELEASE:=1

SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/connectivity/lbs_em/src

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=
PKG_CONFIG_DEPENDS:=
PKG_FILE_DEPENDS := $(SOURCE_DIR)

include $(INCLUDE_DIR)/uclibc++.mk
include $(INCLUDE_DIR)/package.mk

define Package/lbs_em
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=MTK LBS Engineer Tool
  SUBMENU:=Applications
  DEPENDS:=+uclibcxx
#  DEPENDS:=+libstdcpp
endef

define Package/MTK LBS Engineer Tool/description
  This is a MTK LBS Engineer Tool for testing MTK LBS solution
endef

define Build/Prepare
		mkdir -p $(PKG_BUILD_DIR)
		cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef

define Build/InstallDev
	mkdir -p $(1)/usr/include/lbs_em/
	mkdir -p $(1)/usr/include/lbs_em/agps_interface/
	$(CP) $(PKG_BUILD_DIR)/agps_interface/inc/agps_interface.h $(1)/usr/include/lbs_em/agps_interface/
	mkdir -p $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/agps_interface/libagps_interface.so $(1)/usr/lib/
endef

define Package/lbs_em/install
	$(INSTALL_DIR) $(1)/etc/lbs/certs
	$(INSTALL_DATA) $(TOPDIR)/../mtk/package/system/connectivity/lbs_em/src/certs/UbuntuOne-Go_Daddy_CA.pem $(1)/etc/lbs/certs/UbuntuOne-Go_Daddy_CA.pem
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/agps_interface/libagps_interface.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lbs_em $(1)/usr/bin/lbs_em
endef

$(eval $(call BuildPackage,lbs_em))

