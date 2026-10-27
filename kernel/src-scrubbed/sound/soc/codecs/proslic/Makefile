# Release Version 1.0.4
# Add config define "CONFIG_SND_SOC_PROSLIC" to "y"
# Add to Makefile of prented folder: obj-$(CONFIG_SND_SOC_PROSLIC) += proslic/

# Platform
subdir-ccflags-y += -DSI3218X
#subdir-ccflags-y += -DSI3217X
subdir-ccflags-y += -DPROSLIC_LINUX_KERNEL
subdir-ccflags-y += -Iinc
subdir-ccflags-y += -Iconfig_inc
subdir-ccflags-y += -I../le9643_api/inc

# Basic
snd-soc-proslic-objs := ../quectel_slic_app.o ../quectel_si321xx_api.o si321xx.o proslic_sys_timer.o proslic_spi_api.o proslic_sys_main.o proslic_sys_spi.o proslic_timer_intf_linux.o \
						src/quectel_tone_presets.o src/proslic.o src/proslic_tstin.o src/si_voice.o src/si_voice_version.o src/vdaa.o src/vdaa_constants.o \
						si3218x_2.o proslic_spi_api2.o proslic_sys_main2.o proslic_sys_spi2.o
snd-soc-proslic-objs += src/si3218x_LCCB_constants.o src/si3218x_intf.o \
						patch_files/si3218x_patch_A_2017MAY25.o
snd-soc-proslic-objs += src/si3217x_LCQC_MP_constants.o src/si3217x_intf.o src/si3217x_revb_intf.o src/si3217x_revc_intf.o \
						patch_files/si3217x_patch_B_FB_2017MAY25.o patch_files/si3217x_patch_C_FB_2017MAY25.o
obj-$(CONFIG_SND_SOC_PROSLIC) += ../quectel_le9643_api.o ../le9643_api/src/profiles.o ../le9643_api/src/sys_service.o ../le9643_api/src/vp_api_common.o \
								../le9643_api/src/vp_api_config.o ../le9643_api/src/vp_api_control.o ../le9643_api/src/vp_api_cslac_seq.o ../le9643_api/src/vp_api_init.o \
								../le9643_api/src/vp_api_query.o ../le9643_api/src/vp_api_test.o ../le9643_api/src/vp_debug.o ../le9643_api/src/vp_dtmf_detect.o ../le9643_api/src/vp_hal.o \
								../le9643_api/src/vp_pulse_decode.o ../le9643_api/src/vp_timer_queue.o ../le9643_api/src/vp886_adaptive_ringing.o ../le9643_api/src/vp886_calibration_common.o \
								../le9643_api/src/vp886_common.o ../le9643_api/src/vp886_control.o ../le9643_api/src/vp886_events.o ../le9643_api/src/vp886_init.o ../le9643_api/src/vp886_query.o \
								../le9643_api/src/vp886_seq.o ../le9643_api/src/vp886_slac.o  ../le9643_api/src/vp886_timers.o
obj-$(CONFIG_SND_SOC_PROSLIC) += snd-soc-proslic.o