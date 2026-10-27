
include $(TOPDIR)/rules.mk

PKG_NAME:=libaudioparamparser
PKG_RELEASE := 1

include $(INCLUDE_DIR)/package.mk

SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/audio/audio-xml-parser
SRC_OUT_DIR := $(PKG_BUILD_DIR)/out
USE_SOURCE_DIR := $(SOURCE_DIR)

define Package/libaudioparamparser
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=audio-xml-parser
  DEPENDS:=+libstdcpp +libxml2 +uci +libuci
  MENU:=1
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Package/libaudioparamparser/description
  An program to query info from audio mixer control
endef

define Build/Compile
	$(MAKE_FLAGS) $(MAKE) -C $(PKG_BUILD_DIR)
endef

define Package/libaudioparamparser/conffiles
/etc/config/speech_property
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/audio_big_sw/libaudioparamparser.so $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/audio_big_sw/libsystemlib.so $(1)/usr/lib/
endef

define Package/libaudioparamparser/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_BUILD_DIR)/audio_big_sw/libaudioparamparser.so \
		$(1)/usr/lib/
	$(INSTALL_DIR) $(1)/system/etc/audio_param
	$(CP) \
		$(PKG_BUILD_DIR)/audio_big_sw/$(call qstrip,$(CONFIG_MTK_PLATFORM))/audio_param/* \
		$(1)/system/etc/audio_param/
	$(INSTALL_DIR) $(1)/home/root/.audio_param/
	$(CP) $(PKG_BUILD_DIR)/audio_big_sw/libaudioparamparser.so $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/audio_big_sw/libsystemlib.so $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) \
		$(PKG_BUILD_DIR)/files/speech_property.config \
		$(1)/etc/config/speech_property
endef

$(eval $(call BuildPackage,libaudioparamparser))

