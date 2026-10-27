include $(TOPDIR)/rules.mk

MEDMCU_SRC_DIR := $(TOPDIR)/../mtk/package/firmware/tinysys/medmcu

PKG_NAME    := medmcu
PKG_RELEASE := 1
PKG_FILE_DEPENDS := $(MEDMCU_SRC_DIR)

include $(INCLUDE_DIR)/package.mk

MEDMCU_OUT := $(PKG_BUILD_DIR)
MEDMCU_IMG_FILENAME := medmcu.img
MEDMCU_SYMBOL_FOLDER := RV33_A/medmcu
MEDMCU_SYMBOL_FILENAME := tinysys-medmcu-RV33_A.elf

define Package/medmcu
  CATEGORY := MTK Properties
  TITLE    := medmcu
endef

define Package/medmcu/description
  M80 MED-MCU is a CPU offload SW module to handle packet forwarding between Frame Engine and DPMAIF
endef

define Package/medmcu/install
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(MEDMCU_OUT)/$(MEDMCU_IMG_FILENAME) $(STAGING_DIR_IMAGE)/
	$(CP) $(MEDMCU_OUT)/$(MEDMCU_SYMBOL_FOLDER)/$(MEDMCU_SYMBOL_FILENAME) $(STAGING_DIR_IMAGE)/
endef

define Build/Compile
	mkdir -p $(MEDMCU_OUT)
	+$(MAKE) -C $(MEDMCU_SRC_DIR) PROJECT=$(SUBTARGET) O=$(MEDMCU_OUT)
endef

$(eval $(call BuildPackage,medmcu))
