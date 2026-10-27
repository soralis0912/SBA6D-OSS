# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

SOURCE_DIR:=$(TOPDIR)/../mtk/package/kernel/netsys_driver/nat/hw_nat
PKG_NAME:=hw_nat
PKG_FILE_DEPENDS := $(SOURCE_DIR)
include $(INCLUDE_DIR)/package.mk

define KernelPackage/hw_nat
  CATEGORY:=MTK Properties
  TITLE:=MTK hw_nat driver
  FILES:=$(PKG_BUILD_DIR)/hw_nat.ko
  SUBMENU:=Drivers
  MENU:=1
  AUTOLOAD:=$(call AutoLoad,70,hw_nat)
endef

define KernelPackage/hw_nat/config
        source "$(SOURCE)/config.in"
endef

EXTRA_KCONFIG:= \
	CONFIG_RA_NAT_HW=y \
	CONFIG_RA_HW_NAT=m \
	CONFIG_RA_HW_NAT_TBL_32K=y \
	CONFIG_RA_HW_NAT_HASH1=y \
	CONFIG_HW_NAT_AUTO_MODE=y \
	CONFIG_RA_HW_NAT_WIFI=y \
	CONFIG_RA_HW_NAT_WIFI_NEW_ARCH=y \
	CONFIG_ARCH_MT7622_WIFI_HW_NAT=y \
	CONFIG_PPE_MIB=y \
	CONFIG_HW_NAT_AUTO_MODE=y \
	CONFIG_ARCH_COLGIN=y \
	CONFIG_RA_HW_NAT_IPV6=y \
	CONFIG_SUPPORT_OPENWRT=y \
	CONFIG_QDMA_SUPPORT_QOS=y \
	CONFIG_HW_NAT_SW_DVFS=y

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

ifeq ($(CONFIG_NET_MEDIATEK_HSETH),y)
    EXTRA_CFLAGS += -DCONFIG_MEDIATEK_HSETH
endif

ifeq ($(CONFIG_MTK_TINYSYS_MEDMCU_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_MTK_TINYSYS_MEDMCU_SUPPORT
endif

ifeq ($(CONFIG_MTK_WIFI7_CHIP_MT7990),y)
	EXTRA_CFLAGS += -DCONFIG_MTK_WIFI7_CHIP_MT7990
endif

ifeq ($(CONFIG_MTK_USB_NET_HWNAT_SUPPORT),y)
    EXTRA_CFLAGS += -DCONFIG_MTK_HNAT_FAST_SUPPORT
else ifeq ($(CONFIG_MTK_WIFI_SW_FAST_SUPPORT),y)
    EXTRA_CFLAGS += -DCONFIG_MTK_HNAT_FAST_SUPPORT
endif

ifeq ($(CONFIG_MTK_NAT46_UTIL),y)
	EXTRA_CFLAGS += -DCONFIG_MTK_NAT46_UTIL
endif

ifeq ($(CONFIG_HNAT_V2),y)
	EXTRA_CFLAGS += -DCONFIG_HNAT_V2
else ifeq ($(CONFIG_HNAT_V1),y)
	EXTRA_CFLAGS += -DCONFIG_HNAT_V1
endif

ifeq ($(CONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP),0)
EXTRA_CFLAGS+=-DCONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP=0
else ifeq ($(CONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP),1)
EXTRA_CFLAGS+=-DCONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP=1
else ifeq ($(CONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP),2)
EXTRA_CFLAGS+=-DCONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP=2
else ifeq ($(CONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP),3)
EXTRA_CFLAGS+=-DCONFIG_HW_NAT_SW_DVFS_DEFAULT_OPP=3
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define Package/hw_nat/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/ $(1)/etc
endef

$(eval $(call KernelPackage,hw_nat))
