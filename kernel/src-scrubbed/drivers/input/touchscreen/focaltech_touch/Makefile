# Makefile for the focaltech touchscreen drivers.

ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/alsps/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/alsps/inc/
obj-$(CONFIG_TOUCHSCREEN_FTS) += focaltech_touch.o
focaltech_touch-y := \
	focaltech_core.o \
	focaltech_ex_fun.o \
	focaltech_ex_mode.o \
	focaltech_gesture.o \
	focaltech_esdcheck.o \
	focaltech_point_report_check.o \
	focaltech_proximity.o \
	focaltech_i2c.o \
	focaltech_flash.o \
	focaltech_upgrade_ft6336g.o

#focaltech_touch-y += \
#	tpd_control.o \
#	tpd_button.o \
#	tpd_calibrate.o \
#	tpd_debug.o \
#	tpd_default.o \
#	tpd_misc.o \
#	tpd_setting.o

obj-y   +=  tpd_control.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o
#focaltech_touch-y += focaltech_flash/
