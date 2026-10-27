define Device/evb6990_cpe_mt7990_emmc
  DEVICE_MODEL := MT6990 EMMC EVB for CPE
  DEVICE_PACKAGES := $(MT6990_DEFAULT_PACKAGES)
endef
TARGET_DEVICES += evb6990_cpe_mt7990_emmc

define Device/evb6990_cpe_mt7990_emmc_debug
  DEVICE_MODEL := MT6990 EMMC EVB for CPE (debug)
  DEVICE_PACKAGES := $(MT6990_DEFAULT_PACKAGES) $(DEVICE_PACKAGES_DEBUG)
endef
TARGET_DEVICES += evb6990_cpe_mt7990_emmc_debug

#Remove these files from upgrade image of this platform
TMP_SYSUPGRADE_FILES := $(SYSUPGRADE_FILES)
SYSUPGRADE_FILES = $(filter-out root_ro.sig, $(TMP_SYSUPGRADE_FILES))
TMP_SYSUPGRADE_FILES := $(SYSUPGRADE_FILES)
SYSUPGRADE_FILES = $(filter-out loader_ext.img, $(TMP_SYSUPGRADE_FILES))
