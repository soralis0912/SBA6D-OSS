include $(TOPDIR)/rules.mk

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=$(TOPDIR)/../mtk/package/system/netagent/src

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=mtk_netagent
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_FILE_DEPENDS:=$(SOURCE_DIR)
PKG_BUILD_DEPENDS:=mipc
PKG_BUILD_DEPENDS:=libubox
PKG_BUILD_DEPENDS:=libnetagent

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/mtk_netagent
	SECTION:=MTK Properties
	CATEGORY:=MTK Properties
	SUBMENU:=Applications
	DEPENDS:=+libstdcpp +mipc +libubox +libubus +uci +libnetagent
	TITLE:=mtk_netagent
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include/mipc/api \
	-I$(STAGING_DIR)/usr/include/mipc/common \
	-I$(STAGING_DIR)/usr/include/mipc/msg \
	-I$(STAGING_DIR)/usr/include \
	-D_LINUX

TARGET_CFLAGS += $(if $(CONFIG_NO_IP_ASSIGNMENT),-DCONFIG_NO_IP_ASSIGNMENT,)
TARGET_CFLAGS += $(if $(CONFIG_ETHERNET_PDU), -DCONFIG_ETHERNET_PDU,)

TARGET_LIBS = -Bdynamic -lubox -lmipc_msg -luci -lnetagent
TARGET_LDFLAGS+= -L$(STAGING_DIR)/usr/lib/

# Package description; a more verbose description on what our package does
define Package/mtk_netagent/description
  NetAgent for network interface config
endef

# Package preparation instructions; create the build directory and copy the source code.
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
# define Build/Prepare
# 	mkdir -p $(PKG_BUILD_DIR)
# 	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
# 	$(Build/Patch)
# endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Package/mtk_netagent/config
	source "$(SOURCE)/Config.in"
endef

# Package build instructions; invoke the target-specific compiler to first compile the source file, and then to link the file into the final executable
#define Build/Compile
#	CC="$(TARGET_CC)" \
#	CFLAGS="$(TARGET_CFLAGS)" \
#	LDFLAGS="$(TARGET_LDFLAGS)" \
#	LIBS="$(TARGET_LIBS)" \
#	$(MAKE) -C $(PKG_BUILD_DIR)
#endef
#define Build/Compile
#	$(MAKE) -C$(PKG_BUILD_DIR) CFLAGS="$(TARGET_CFLAGS)" CC=$(TARGET_CC) CXX=$(TARGET_CXX) LD=$(TARGET_LD) AR=$(TARGET_AR) STRIP=$(STRIP) all
#endef

# Package install instructions; create a directory inside the package to hold our executable, and then copy the executable we built previously into the folder
define Package/mtk_netagent/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(TOPDIR)/../mtk/package/system/netagent/files/mtk_netagent.init $(1)/etc/init.d/mtk_netagent
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtk_netagent $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtk_ia $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtk_act_data_call $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtk_deact_data_call $(1)/usr/bin
	$(INSTALL_BIN) $(TOPDIR)/../mtk/package/system/netagent/files/nettest.sh $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) \
		$(TOPDIR)/../mtk/package/system/netagent/files/netagent.config \
		$(1)/etc/config/netagent
	$(if $(CONFIG_NO_IP_ASSIGNMENT),$(INSTALL_BIN) $(TOPDIR)/../mtk/package/system/netagent/files/mtk_bridge.init $(1)/etc/init.d/mtk_bridge)

endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,mtk_netagent))

