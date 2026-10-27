
include $(TOPDIR)/rules.mk

PKG_NAME:=libspeech_drv
PKG_RELEASE := 1
include $(INCLUDE_DIR)/package.mk

SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/audio/libspeech_drv

define Package/libspeech_drv
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=audio speech driver
  DEPENDS:=+libaudioparamparser
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	cp $(PKG_BUILD_DIR)/audio_big_sw/$(call qstrip,$(CONFIG_MTK_PLATFORM))/Makefile $(PKG_BUILD_DIR)
endef

define Package/libspeech_drv/description
  speech driver controled by audio ctrl service
endef

define Package/libspeech_drv/extra_provides
    echo "libc.so.6";
endef

define Build/Compile
	$(MAKE_FLAGS) $(MAKE) -C $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libspeech_drv.so $(1)/usr/lib/
endef

define Package/libspeech_drv/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_BUILD_DIR)/libspeech_drv.so \
		$(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_BUILD_DIR)/libspeech_dmn.so \
		$(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_BUILD_DIR)/libspeechparser_vendor.so \
		$(1)/usr/lib/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/files/speech_daemon.init \
		$(1)/etc/init.d/speech_daemon
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/speech_daemon \
		$(1)/usr/bin/
endef

$(eval $(call BuildPackage,libspeech_drv))
