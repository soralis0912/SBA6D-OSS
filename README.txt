RG620TSBK241106 GPL/LGPL Disclosure Package
===========================================

This package contains the source code, configuration, patches and
build-related materials for the open source software used in this
product, provided in fulfilment of the GPL/LGPL license obligations.

Product baseline
----------------
  Product revision : RG620TSBK00AAR03A01G4G_OCPU_01.001.01.001
  OpenWrt version  : 21.02.7  (r16847-f8282da11e)
  Linux kernel     : 5.4.238
  OpenWrt target   : gem6xxx / evb6990_cpe_mt7990_emmc
  OpenWrt profile  : DEVICE_evb6990_cpe_mt7990_emmc_debug

Package contents
----------------
  kernel/
      src-scrubbed/       Linux 5.4.238 kernel source tree
      .config             kernel configuration as built
  buildroot/
      openwrt/            OpenWrt 21.02.7 build system
      feeds/              OpenWrt package feeds
                            packages, luci, routing  (upstream OSS)
                            mtk                      (MediaTek, per OSS statement)
  configs/
      openwrt.config          OpenWrt target configuration
      openwrt.target.config   target profile configuration
      kernel.config           kernel configuration
      kernel.config.target    target kernel configuration
      kernel.config-5.4.fragment
  dts/                    Device Tree Sources for the selected target
  patches/                Kernel and package patches
      openwrt/target/linux/gem6xxx/patches-5.4/
                          136 kernel patches, applied in numeric order
  sources/
      upstream-archives/  pinned Git submodules and extracted source trees
                          for GPL/LGPL components
      source-tree-candidates/
      package-recipes/
  package-info/
      openwrt-packages-package-details.tsv
                          OSS inventory: all 372 installed packages with
                          version, license, and disclosure status
      openwrt-packages-summary.tsv   build identification
      built-package-inventory.tsv    build metadata
      gpl-lgpl-package-inventory.tsv GPL/LGPL component inventory
      config.buildinfo, feeds.buildinfo, target.mk, device.mk
  OSS_STATEMENT.txt          scope of this disclosure
  BUILD_INSTRUCTIONS.txt     how to build from these materials
  EXCLUSIONS_JUSTIFICATION.tsv  material not included, with rationale

OSS inventory and license information
-------------------------------------
package-info/openwrt-packages-package-details.tsv lists every package
installed on the product (372 entries) with its version, architecture,
license, category and description.

The "Disclose" column indicates whether the component's source is
included in this package:
    Yes (282)  open source component, source provided here
    No  (90)   proprietary component, not subject to source disclosure

Full license texts are provided at:
    kernel/src-scrubbed/COPYING          GPL-2.0 (Linux kernel)
    kernel/src-scrubbed/LICENSES/        SPDX license texts
    buildroot/openwrt/LICENSES/          OpenWrt license texts

Scope
-----
This is a source disclosure package covering the open source software
used in the product. It is not the complete vendor SDK. Proprietary
components, signing material and modem binaries are not included; see
EXCLUSIONS_JUSTIFICATION.tsv.

Keyonet modifications to the Linux kernel are supplied as a patch:
    patches/openwrt/target/linux/gem6xxx/patches-5.4/0990-knos-patch.patch

See BUILD_INSTRUCTIONS.txt to build from these materials.
