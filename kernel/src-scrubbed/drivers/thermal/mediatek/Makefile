ccflags-y += \
	-I$(srctree)/drivers/thermal/mediatek \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat \
	-I$(srctree)/include/dt-bindings \
	-I$(srctree)/drivers/misc/mediatek/sspm/v2/ \

obj-y += mtk_thermal_trace.o

obj-$(CONFIG_MTK_SOC_THERMAL)	+= soc_temp.o
obj-$(CONFIG_MTK_SOC_THERMAL_LVTS)	+= soc_temp_lvts.o

obj-$(CONFIG_MTK_BOARD_THERMAL) += board_temp.o
obj-$(CONFIG_MTK_PMIC_THERMAL) += pmic_temp.o

obj-$(CONFIG_MTK_MD_THERMAL)		+= md_cooling.o
obj-$(CONFIG_MTK_MD_THERMAL_MUTT)	+= md_cooling_mutt.o
obj-$(CONFIG_MTK_MD_THERMAL_TX_POWER)	+= md_cooling_tx_pwr.o
obj-$(CONFIG_MTK_MD_THERMAL_SCG_OFF)	+= md_cooling_scg_off.o
obj-$(CONFIG_MTK_MD_THERMAL_SYSFS)	+= md_cooling_sysfs.o
obj-$(CONFIG_MTK_MD_RF_THERMAL)		+= md_rf_temp.o

obj-$(CONFIG_MTK_THERMAL_IPI)		+= mtk_thermal_ipi.o
obj-$(CONFIG_MTK_THERMAL_TRM)		+= thermal_risk_monitor.o
