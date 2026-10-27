include $(TOPDIR)/rules.mk

SRC_DIR := $(TOPDIR)/../mtk/package/firmware/trustzone/atf

PKG_NAME    := atf
PKG_RELEASE := 1
PKG_FILE_DEPENDS := $(SRC_DIR)
PKG_BUILD_DEPENDS := PACKAGE_optee:optee

include $(INCLUDE_DIR)/package.mk

SRC_OUT_DIR := $(PKG_BUILD_DIR)/out

ATF_BIN_FILENAME := tee.img
ATF_ELF_FILE := $(SRC_OUT_DIR)/trustzone/ATF_OBJ/debug/bl31/bl31.elf

define Package/atf
	SECTION  := firmware
	CATEGORY := MTK Properties
	TITLE    := atf
	HIDDEN   := 1
endef

define Package/atf/description
  MT6880/MT6890 ATF (ARM Trusted Firmware)
endef

define Build/Compile/atf
	$(INSTALL_DIR) $(SRC_OUT_DIR)
	$(MAKE) -C $(SRC_DIR)/custom/build all \
		LOCAL_PATH=$(TOPDIR)/../mtk/package/firmware/trustzone/atf/custom/build PRODUCT_OUT=$(ATF_OUT) \
		CROSS_COMPILE=$(TOPDIR)/../mtk/prebuilt/openwrt-toolchain/arm64-a55_neon-gcc-9.3.0_musl/bin/aarch64-openwrt-linux- \
		PRODUCT_OUT=$(SRC_OUT_DIR) \
		MTK_PLATFORM=$(call qstrip,$(CONFIG_MTK_PLATFORM)) \
		MTK_BASE_PROJECT=$(SUBTARGET) \
		MTK_TARGET_PROJECT=$(SUBTARGET) \
		CONFIG_MTK_GPS_OFL_SUPPORT=$(CONFIG_MTK_GPS_OFL_SUPPORT)
endef

define Package/atf/install
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(SRC_OUT_DIR)/$(ATF_BIN_FILENAME) $(STAGING_DIR_IMAGE)
	$(CP) $(ATF_ELF_FILE) $(STAGING_DIR_IMAGE)/bl31.elf
endef

define Build/Clean
	rm -rf $(SRC_OUT_DIR)
endef

define Build/Compile
	$(Build/Compile/atf)
endef

$(eval $(call BuildPackage,atf))
