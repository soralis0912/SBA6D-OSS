include $(TOPDIR)/rules.mk

SRC_DIR := $(TOPDIR)/../mtk/package/boot/preloader

PKG_NAME    := preloader
PKG_RELEASE := 1
PKG_FILE_DEPENDS := $(SRC_DIR)

include $(INCLUDE_DIR)/package.mk

SRC_OUT_DIR := $(PKG_BUILD_DIR)/out
PRELOADER_BIN:=$(SRC_OUT_DIR)/bin
PRELOADER_BIN_FILE := preloader_$(CONFIG_TARGET_SUBTARGET).bin
PRELOADER_ELF_FILE := preloader_$(CONFIG_TARGET_SUBTARGET).elf
VENDOR_INFO_PATH := $(TOPDIR)/../mtk/package/system/vendor_info
DA_CROSS_COMPILE_PATH := $(TOPDIR)/../mtk/prebuilt/openwrt-toolchain/arm-a7_neon-gcc-9.3.0_musl/bin
EMIGEN_TOOL_PATH := $(TOPDIR)/../mtk/tools/emigen

TARGET_PRODUCT := $(CONFIG_TARGET_SUBTARGET)

define Package/preloader
	SECTION  := boot
	CATEGORY := MTK Properties
	TITLE    := preloader
	HIDDEN   := 1
endef

define Package/preloader/description
	MT6880/MT6890 preloader
endef

define Build/Compile/preloader
	$(INSTALL_DIR) $(SRC_OUT_DIR)
	$(MAKE) -C $(SRC_DIR) -f Makefile PRELOADER_OUT=$(SRC_OUT_DIR) MTK_PROJECT=$(CONFIG_TARGET_SUBTARGET) \
		TARGET_PROFILE=$(if $(filter %_debug,$(call qstrip,$(CONFIG_TARGET_PROFILE))),"Debug","Default") \
		TOOL_PATH=$(TOOL_PATH) ROOTDIR=$(SRC_DIR) EMIGEN_TOOL_PATH=$(EMIGEN_TOOL_PATH) \
		QUECTEL_PRELOADER_ATAG=$(if $(filter y,$(CONFIG_QUECTEL_ATAG)),yes,no) \
		QUECTEL_PRELOADER_RAWDATA_NAND=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_NAND)),yes,no) \
		QUECTEL_PRELOADER_RAWDATA_EMMC=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_EMMC)),yes,no)
endef

DA_SRC_DIR := $(TOPDIR)/../mtk/package/boot/preloader
DA_OUT_DIR := $(PKG_BUILD_DIR)/da_out/$(CONFIG_TARGET_SUBTARGET)

FPGA_PROJECT :=$(findstring fpga,$(CONFIG_TARGET_SUBTARGET))
ifeq ($(FPGA_PROJECT),fpga)
DA_BR_BIN_TAG:=DA_BR_FPGA
else
DA_BR_BIN_TAG:=DA_BR
endif

DA_BR_BIN:=$(DA_OUT_DIR)/$(DA_BR_BIN_TAG)_OBJ/bin

# Flashless need no DA
ifneq (y,$(CONFIG_MTK_FLASHLESS_SUPPORT))
define DA_install
	mkdir -p $(STAGING_DIR_IMAGE)/download_agent
	$(CP) $(DA_BR_BIN)/$(DA_BR_BIN_TAG).bin $(STAGING_DIR_IMAGE)/download_agent/$(DA_BR_BIN_TAG).bin
	$(CP) $(DA_BR_BIN)/flash.xml $(STAGING_DIR_IMAGE)/download_agent/flash.xml
	$(CP) $(DA_BR_BIN)/flash.xsd $(STAGING_DIR_IMAGE)/download_agent/flash.xsd
endef

define DA_clean
	rm -rf $(DA_OUT_DIR)
endef

define DA_compile
	$(INSTALL_DIR) $(DA_OUT_DIR)
	$(MAKE) -C $(DA_SRC_DIR) -f Makefile_DA DA MTK_PROJECT=$(CONFIG_TARGET_SUBTARGET) \
		TARGET_PROFILE=$(if $(filter %_debug,$(call qstrip,$(CONFIG_TARGET_PROFILE))),"Debug","Default") \
		DA_OUT=$(DA_OUT_DIR) PLATFORM=$(call qstrip,$(CONFIG_MTK_PLATFORM)) FPGA_PROJECT=$(FPGA_PROJECT) \
		VENDOR_INFO_PATH=$(VENDOR_INFO_PATH) DA_CROSS_COMPILE_PATH=$(DA_CROSS_COMPILE_PATH) \
		EMIGEN_TOOL_PATH=$(EMIGEN_TOOL_PATH) \
		QUECTEL_PRELOADER_RAWDATA_NAND=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_NAND)),yes,no) \
		QUECTEL_PRELOADER_RAWDATA_EMMC=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_EMMC)),yes,no)
endef
endif

define Package/preloader/install
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PRELOADER_BIN)/$(PRELOADER_BIN_FILE) $(STAGING_DIR_IMAGE)
	$(CP) $(PRELOADER_BIN)/$(PRELOADER_ELF_FILE) $(STAGING_DIR_IMAGE)
	@echo "=================loader_ext.img======================="
	if [ -f "$(PRELOADER_BIN)/loader_ext.img" ]; then \
		$(CP) $(PRELOADER_BIN)/loader_ext.img $(STAGING_DIR_IMAGE); \
	fi
	@echo "=================efuse.img======================="
	if [ -f "$(PRELOADER_BIN)/efuse.img" ]; then \
		$(CP) $(PRELOADER_BIN)/efuse.img $(STAGING_DIR_IMAGE); \
	fi
	$(CP) $(PRELOADER_BIN)/pi_img.img $(STAGING_DIR_IMAGE)
	$(DA_install)
endef

define Build/Clean
	rm -rf $(SRC_OUT_DIR)
	$(DA_clean)
endef

define Build/Compile
	$(Build/Compile/preloader)
	$(DA_compile)
endef

$(eval $(call BuildPackage,preloader))
