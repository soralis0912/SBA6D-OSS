# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the Mediatek SoCs built-in ethernet macs
#

ccflags-y				+= -I$(srctree)/drivers/quectel-drivers/rawdata/api
ccflags-y				+= -I$(srctree)/drivers/misc/mediatek/ethernet
ccflags-y				+= -I$(srctree)/drivers/net/phy/

ifdef CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT
ccflags-y				+= -I$(srctree)/drivers/misc/mediatek/ethernet/of_overlay/phy_reg_rw

obj-y		+= mtk_sgmii_pwr.o of_overlay/mtk_eth_soc_overlay.o of_overlay/phy_reg_rw/ql_phy_rw_en8801.o of_overlay/phy_reg_rw/ql_phy_rw_yt8821.o

obj-$(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)	+= of_overlay/ql_overlay_of_mac0_phy_2p5g.dtb.o \
								of_overlay/ql_overlay_of_mac1_phy_2p5g.dtb.o \
								of_overlay/ql_overlay_of_mac0_phy_10g.dtb.o \
								of_overlay/ql_overlay_of_mac1_phy_10g.dtb.o \
								of_overlay/ql_overlay_of_mac0_phy_en8801_1g.dtb.o \
								of_overlay/ql_overlay_of_mac1_phy_en8801_1g.dtb.o \
								of_overlay/ql_overlay_of_mac0_phy_en8811_2p5g.dtb.o \
								of_overlay/ql_overlay_of_mac1_phy_en8811_2p5g.dtb.o \
								of_overlay/ql_overlay_of_mac0_phy_yt8821_2p5g.dtb.o \
								of_overlay/ql_overlay_of_mac1_phy_yt8821_2p5g.dtb.o

#enable creation of __symbols__ node
DTC_FLAGS_ql_overlay_of_mac0_phy_2p5g += -@
DTC_FLAGS_ql_overlay_of_mac1_phy_2p5g += -@
DTC_FLAGS_ql_overlay_of_mac0_phy_10g += -@
DTC_FLAGS_ql_overlay_of_mac1_phy_10g += -@
DTC_FLAGS_ql_overlay_of_mac0_phy_en8801_1g += -@
DTC_FLAGS_ql_overlay_of_mac1_phy_en8801_1g += -@
DTC_FLAGS_ql_overlay_of_mac0_phy_en8811_2p5g += -@
DTC_FLAGS_ql_overlay_of_mac1_phy_en8811_2p5g += -@
DTC_FLAGS_ql_overlay_of_mac0_phy_yt8821_2p5g += -@
DTC_FLAGS_ql_overlay_of_mac1_phy_yt8821_2p5g += -@

obj-$(CONFIG_MTK_ETHERNET_SOC)			+= \
							mtk_eth_soc.o mtk_sgmii.o \
							mtk_eth_path.o \
							mtk_common.o \
							mtk_rss.o \
							mtk_sfq.o
else

obj-$(CONFIG_MTK_ETHERNET_SOC)			+= mtk_sgmii_pwr.o\
							mtk_eth_soc.o mtk_sgmii.o \
							mtk_eth_path.o \
							mtk_common.o \
							mtk_rss.o \
							mtk_sfq.o
endif

ifdef CONFIG_QUECTEL_ETHERNET_PHY
obj-$(CONFIG_MTK_ETHERNET_SOC)			+= ql_eth_phy_hal.o

obj-y		+= ./rtl8221/realtek_822x.o ./rtl8221/nic_rtl8226b_init.o \
											 ./rtl8221/nic_rtl8226b.o
obj-y		+= ./aqr113/aqr113c.o ./aqr113/aqr_load_firmware.o

endif



obj-$(CONFIG_MTK_ETHERNET_DBG)			+= mtk_eth_dbg.o \
						   mtk_hwlro_dbg.o

obj-m			+= scan_phy/quec_mdio_scan.o	#default driver for scan phys
