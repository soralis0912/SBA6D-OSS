include $(TOPDIR)/rules.mk

ifneq ($(CONFIG_LK_LEGACY),)
	SRC_DIR := $(TOPDIR)/../mtk/package/boot/lk
else
	SRC_DIR := $(TOPDIR)/../mtk/package/boot/lk
endif

PKG_NAME    := lk
PKG_RELEASE := 1
PKG_FILE_DEPENDS := $(SRC_DIR)
PKG_CONFIG_DEPENDS := CONFIG_TARGET_ROOTFS_HASHED

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

LK_BUILD_DIR := $(PKG_BUILD_DIR)/build-$(SUBTARGET)
LK_CROSS_COMPILE:=\
	$(TOPDIR)/../mtk/prebuilt/openwrt-toolchain/arm-a7_neon-gcc-9.3.0_musl/bin/arm-openwrt-linux-
LK_64_CROSS_COMPILE:=\
	$(TOPDIR)/../mtk/prebuilt/openwrt-toolchain/arm64-a55_neon-gcc-9.3.0_musl/bin/aarch64-openwrt-linux-

LK_MKIMAGE_TOOL := $(TOPDIR)/../mtk/tools/common/mkimage
LK_CFG_FILE := $(LK_BUILD_DIR)/img_hdr_lk.cfg
LK_DTB_CFG_FILE := $(LK_BUILD_DIR)/img_hdr_lk_dtb.cfg
LK_BUILD_TARGET := $(LK_BUILD_DIR)/lk.bin
DTB_FILE = $(LINUX_DIR)/arch/$(LINUX_KARCH)/boot/dts/mediatek/$(SUBTARGET).dtb

LK_MAIN_DTB_TARGET := $(LK_BUILD_DIR)/lk_main_dtb.img
LK_RAW_TARGET := $(LK_BUILD_DIR)/lk_raw.img
LK_INSTALLED_TARGET := $(LK_BUILD_DIR)/lk.img

# ifeq ($(CONFIG_TARGET_PROFILE),"Debug")
# Enter debug mode if CONFIG_TARGET_PROFILE ends with _debug
ifneq (,$(filter %_debug,$(call qstrip,$(CONFIG_TARGET_PROFILE))))
DEBUG := 2
TARGET_PROFILE := "Debug"
else
DEBUG := 0
TARGET_PROFILE := "Default"
endif

# For AEE LK
AEE_LK_BUILD_DIR := $(BUILD_DIR)/aee_lk/build-$(SUBTARGET)
AEE_LK_CFG_FILE := $(AEE_LK_BUILD_DIR)/img_hdr_lk.cfg
AEE_LK_BUILD_TARGET := $(AEE_LK_BUILD_DIR)/lk.bin
AEE_LK_TARGET := $(AEE_LK_BUILD_DIR)/lk.img
ifneq ($(CONFIG_LK_LEGACY),)
	LK_AS := LK_AS_AEE=1
else
	LK_AS := LK_AS=AEE
endif

# For SBOOT_DIS LK
LK_SBOOT_DIS_BUILD_DIR := $(BUILD_DIR)/lk_sboot_dis/build-$(SUBTARGET)
LK_SBOOT_DIS_CFG_FILE := $(LK_SBOOT_DIS_BUILD_DIR)/img_hdr_lk.cfg
LK_SBOOT_DIS_BUILD_TARGET := $(LK_SBOOT_DIS_BUILD_DIR)/lk.bin
LK_SBOOT_DIS_TARGET := $(LK_SBOOT_DIS_BUILD_DIR)/lk.img

define Package/lk
	SECTION  := boot
	CATEGORY := MTK Properties
	TITLE    := Little Kernel(lk as bootloader)
endef

define Package/lk/description
MT6880/MT6890 lk
Legacy lk based on gem-mp-1907
Common lk2 based on generic-trunk
endef

define Package/lk/config
choice
	prompt "lk codebase"
	default LK_COMMON

	config LK_LEGACY
		bool "Legacy LK(Not used now)"

	config LK_COMMON
		bool "Common LK2"
endchoice
endef

define Build/Compile/lk
	$(INSTALL_DIR) $(LK_BUILD_DIR)
	$(info TARGET_PROFILE=$(TARGET_PROFILE))
	$(info DEBUG=$(DEBUG))
	$(MAKE) -C $(SRC_DIR) -f makefile $(SUBTARGET) \
		TARGET_PROFILE=$(TARGET_PROFILE) \
		DEBUG=$(DEBUG) \
		ARCH_arm_TOOLCHAIN_PREFIX=$(LK_CROSS_COMPILE) \
		ARCH_arm64_TOOLCHAIN_PREFIX=$(LK_64_CROSS_COMPILE) \
		CLANG_BINDIR=$(TOPDIR)/../mtk/prebuilt/clang/host/linux-x86/clang-r433403b/bin \
		CLANG_ARM_TARGET_SYS=openwrt-linux \
		CLANG_ARM_TARGET_ABI=muslgnueabi \
		CLANG_ARM64_TARGET_SYS=openwrt-linux \
		CLANG_ARM64_TARGET_ABI=musl \
		BUILDROOT=$(PKG_BUILD_DIR) \
		BUILDID=$$$$(sed -nr 's/^VERNO=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		BUILDBRANCH=$$$$(sed -nr 's/^BRANCH_INFO=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		BUILDTYPE=$$$$(sed -nr 's/^TYPE=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		BUILDVIRSUB=$$$$(sed -nr 's/^VIRTUAL_SUBTARGET=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		MTK_SECURITY_DM_VERITY=$(if $(filter y,$(CONFIG_TARGET_ROOTFS_HASHED)),yes,no) \
		QUECTEL_LK_ALLPIN=$(if $(filter y,$(CONFIG_QL_LK_ALLPIN)),yes,no) \
		QUECTEL_LK_ATAG=$(if $(filter y,$(CONFIG_QUECTEL_ATAG)),yes,no) \
		QUECTEL_LK_RAWDATA_NAND=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_NAND)),yes,no) \
		QUECTEL_LK_RAWDATA_EMMC=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_EMMC)),yes,no)

	$(MAKE) -C $(SRC_DIR) -f makefile $(SUBTARGET) \
		TARGET_PROFILE=$(TARGET_PROFILE) \
		DEBUG=$(DEBUG) \
		ARCH_arm_TOOLCHAIN_PREFIX=$(LK_CROSS_COMPILE) \
		ARCH_arm64_TOOLCHAIN_PREFIX=$(LK_64_CROSS_COMPILE) \
		CLANG_BINDIR=$(TOPDIR)/../mtk/prebuilt/clang/host/linux-x86/clang-r433403b/bin \
		CLANG_ARM_TARGET_SYS=openwrt-linux \
		CLANG_ARM_TARGET_ABI=muslgnueabi \
		CLANG_ARM64_TARGET_SYS=openwrt-linux \
		CLANG_ARM64_TARGET_ABI=musl \
		BUILDROOT=$(PKG_BUILD_DIR)/../aee_lk $(LK_AS) \
		BUILDID=$$$$(sed -nr 's/^VERNO=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		BUILDBRANCH=$$$$(sed -nr 's/^BRANCH_INFO=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		BUILDTYPE=$$$$(sed -nr 's/^TYPE=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		BUILDVIRSUB=$$$$(sed -nr 's/^VIRTUAL_SUBTARGET=(.*)/\1/gp' $(TOPDIR)/../mtk/package/system/vendor_info/static_vendor_info) \
		MTK_SECURITY_DM_VERITY=$(if $(filter y,$(CONFIG_TARGET_ROOTFS_HASHED)),yes,no) \
		QUECTEL_LK_ALLPIN=$(if $(filter y,$(CONFIG_QL_LK_ALLPIN)),yes,no) \
		QUECTEL_LK_ATAG=$(if $(filter y,$(CONFIG_QUECTEL_ATAG)),yes,no) \
		QUECTEL_LK_RAWDATA_NAND=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_NAND)),yes,no) \
		QUECTEL_LK_RAWDATA_EMMC=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_EMMC)),yes,no)

	$(MAKE) -C $(SRC_DIR) -f makefile $(SUBTARGET) \
		TARGET_PROFILE=$(TARGET_PROFILE) \
		DEBUG=$(DEBUG) \
		ARCH_arm_TOOLCHAIN_PREFIX=$(LK_CROSS_COMPILE) \
		ARCH_arm64_TOOLCHAIN_PREFIX=$(LK_64_CROSS_COMPILE) \
		CLANG_BINDIR=$(TOPDIR)/../mtk/prebuilt/clang/host/linux-x86/clang-r433403b/bin \
		CLANG_ARM_TARGET_SYS=openwrt-linux \
		CLANG_ARM_TARGET_ABI=muslgnueabi \
		CLANG_ARM64_TARGET_SYS=openwrt-linux \
		CLANG_ARM64_TARGET_ABI=musl \
		BUILDROOT=$(PKG_BUILD_DIR)/../lk_sboot_dis \
		MTK_SECURITY_SEC_USDL=ATTR_SBOOT_DISABLE \
		MTK_SECURITY_SEC_BOOT=ATTR_SBOOT_DISABLE \
		MTK_SECURITY_DM_VERITY=$(if $(filter y,$(CONFIG_TARGET_ROOTFS_HASHED)),yes,no) \
		QUECTEL_LK_ALLPIN=$(if $(filter y,$(CONFIG_QL_LK_ALLPIN)),yes,no) \
		QUECTEL_LK_ATAG=$(if $(filter y,$(CONFIG_QUECTEL_ATAG)),yes,no) \
		QUECTEL_LK_RAWDATA_NAND=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_NAND)),yes,no) \
		QUECTEL_LK_RAWDATA_EMMC=$(if $(filter y,$(CONFIG_QUECTEL_RAWDATA_EMMC)),yes,no)
endef

define Package/lk/install
	echo "NAME = lk" > $(LK_CFG_FILE)
	$(LK_MKIMAGE_TOOL) $(LK_BUILD_TARGET) $(LK_CFG_FILE) > $(LK_RAW_TARGET)

	echo "NAME = aee_lk" > $(AEE_LK_CFG_FILE)
	$(LK_MKIMAGE_TOOL) $(AEE_LK_BUILD_TARGET) $(AEE_LK_CFG_FILE) > $(AEE_LK_TARGET)

	echo "NAME = lk" > $(LK_SBOOT_DIS_CFG_FILE)
	$(LK_MKIMAGE_TOOL) $(LK_SBOOT_DIS_BUILD_TARGET) $(LK_SBOOT_DIS_CFG_FILE) > $(LK_SBOOT_DIS_TARGET)

	echo "NAME = lk_main_dtb" > $(LK_DTB_CFG_FILE)

	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(LK_BUILD_DIR)/lk.elf $(STAGING_DIR_IMAGE)
	$(CP) $(AEE_LK_BUILD_DIR)/lk.elf $(STAGING_DIR_IMAGE)/aee_lk.elf
	$(CP) $(LK_SBOOT_DIS_BUILD_DIR)/lk.elf $(STAGING_DIR_IMAGE)/lk_SBOOT_DIS.elf
	$(CP) $(LK_RAW_TARGET) $(TMP_DIR)
	$(CP) $(AEE_LK_TARGET) $(TMP_DIR)
	$(CP) $(LK_SBOOT_DIS_TARGET) $(TMP_DIR)/lk_SBOOT_DIS_raw.img
	$(CP) $(LK_DTB_CFG_FILE) $(TMP_DIR)
endef

define Build/Clean
	rm -rf $(LK_BUILD_DIR)
	rm -rf $(AEE_LK_BUILD_DIR)
endef

define Build/Compile
	$(Build/Compile/lk)
endef

$(eval $(call BuildPackage,lk))
