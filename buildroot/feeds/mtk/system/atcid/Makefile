include $(TOPDIR)/rules.mk

SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/atcid/atci/src
QL_ATCMD_INC:=$(TOPDIR)/../quectel/package/libs/libql_atcmd/inc

PKG_NAME:=atcid
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_FILE_DEPENDS := $(SOURCE_DIR)

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
CXX:=$(TARGET_CROSS)g++

define Package/atcid
  SECTION:=sys
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
  TITLE:=atcid daemon
  DEPENDS:=+mipc +libstdcpp +libuci +libql_atcmd +libql_qlog
endef

define Package/atcid/description
 This package for USB at command.
endef

define Package/atcid/config
config MTK_DATACARD_ATCI
	bool "Datacard ATCI support"
	default n
	depends on PACKAGE_atcid
endef

define Build/Configure
endef

# Package preparation instructions; create the build directory and copy the source code.
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
		mkdir -p $(PKG_BUILD_DIR)
		cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include/mipc/api \
	-I$(STAGING_DIR)/usr/include/mipc/common \
	-I$(STAGING_DIR)/usr/include/mipc/msg \
	-I$(STAGING_DIR)/usr/include/mipc/external \
	-I$(STAGING_DIR)/usr/include \
	-I$(QL_ATCMD_INC) 

TARGET_CFLAGS += -Wall -D_LINUX -fPIC
TARGET_CFLAGS += -DENABLE_TRM
TARGET_LIBS = -lmipc_msg -luci -pthread  -lql_atcmd -lql_qlog

ifneq (y,$(CONFIG_PACKAGE_apmeta_only))
EXTRA_BUILD_FLAGS := $(EXTRA_BUILD_FLAGS) -DMTK_CHECK_META_MODE
endif

ifeq (y,$(CONFIG_MTK_DATACARD_ATCI))
EXTRA_BUILD_FLAGS := $(EXTRA_BUILD_FLAGS) -DMTK_DATACARD_ATCI
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall $(EXTRA_BUILD_FLAGS)" \
		LIBS="$(TARGET_LIBS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef


define Package/atcid/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(TOPDIR)/../mtk/package/system/atcid/atci/files/atcid.init $(1)/etc/init.d/atcid
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/atcid $(1)/usr/bin
endef

$(eval $(call BuildPackage,atcid))
