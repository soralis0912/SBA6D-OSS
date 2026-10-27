include $(TOPDIR)/rules.mk

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=$(TOPDIR)/../mtk/package/libs/libnandapi

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=libnandapi
PKG_VERSION:=1.0
#PKG_RELEASE:=1
PKG_FILE_DEPENDS := $(SOURCE_DIR)

include $(INCLUDE_DIR)/uclibc++.mk
include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/libnandapi
	SECTION:=libs
	CATEGORY:=MTK Properties
	SUBMENU:=Libraries
	TITLE:=NAND API library
	DEPENDS:=$(CXX_DEPENDS) +libc
endef

# Package description; a more verbose description on what our package does
define Package/libnandapi/description
	NAND API library
endef

# Package preparation instructions; create the build directory and copy the source code.
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -af $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

# Package build instructions; invoke the target-specific compiler to first compile the source file, and then to link the file into the final executable
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) OPENWRT=1 CFLAGS="$(TARGET_CFLAGS) -fPIC -D_LARGEFILE64_SOURCE" $(TARGET_CONFIGURE_OPTS)
endef

define Build/InstallDev
	mkdir -p $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/inc/* $(1)/usr/include/
	mkdir -p $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libnandapi.so $(1)/usr/lib/
endef

# Package install instructions; create a directory inside the package to hold our executable, and then copy the executable we built previously into the folder
define Package/libnandapi/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libnandapi.so $(1)/usr/lib
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,libnandapi))

