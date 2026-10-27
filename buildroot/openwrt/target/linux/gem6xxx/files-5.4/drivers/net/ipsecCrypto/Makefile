
PWD := drivers/net/ipsecCrypto

SRC_ADAPTER = Integration/SLAD_Adapter
SRC_DRV_FWK = Integration/DriverFramework
SRC_KIT     = Kit

INCLUDE_FLAGS_ALL:= \
 -I$(PWD)/DriverConfig \
 -I$(PWD)/$(SRC_ADAPTER)/src \
 -I$(PWD)/$(SRC_ADAPTER)/incl \
 -I$(PWD)/$(SRC_DRV_FWK)/src \
 -I$(PWD)/$(SRC_KIT)/DriverFramework/Basic_Defs_API/incl \
 -I$(PWD)/$(SRC_KIT)/DriverFramework/CLib_Abstraction_API/incl \
 -I$(PWD)/$(SRC_KIT)/DriverFramework/Device_API/incl \
 -I$(PWD)/$(SRC_KIT)/DriverFramework/DMAResource_API/incl \
 -I$(PWD)/$(SRC_KIT)/EIP97/GlobalControl/incl \
 -I$(PWD)/$(SRC_KIT)/EIP97/GlobalControl/src \
 -I$(PWD)/$(SRC_KIT)/EIP97/FlowControl/incl \
 -I$(PWD)/$(SRC_KIT)/EIP97/FlowControl/src \
 -I$(PWD)/$(SRC_KIT)/EIP97/RingControl/incl \
 -I$(PWD)/$(SRC_KIT)/EIP97/RingControl/src \
 -I$(PWD)/$(SRC_KIT)/EIP201/incl \
 -I$(PWD)/$(SRC_KIT)/EIP201/src \
 -I$(PWD)/$(SRC_KIT)/IOToken/incl \
 -I$(PWD)/$(SRC_KIT)/IOToken/src \
 -I$(PWD)/$(SRC_KIT)/Log/incl \
 -I$(PWD)/$(SRC_KIT)/Log/src/printk \
 -I$(PWD)/$(SRC_KIT)/RingHelper/incl \
 -I$(PWD)/$(SRC_KIT)/SABuilder/incl \
 -I$(PWD)/$(SRC_KIT)/SABuilder/src \
 -I$(PWD)/$(SRC_KIT)/TokenBuilder/incl \
 -I$(PWD)/$(SRC_KIT)/TokenBuilder/src \

ccflags-y += -DDRIVER_64BIT_HOST
ccflags-y += -DDRIVER_64BIT_DEVICE
ccflags-y += -DEIP97_BUS_VERSION_AXI3
ccflags-y += -DARCH_ARM64
ccflags-y += $(INCLUDE_FLAGS_ALL)

obj-$(CONFIG_IPSEC_HWCRYPTO) += ipsec_crypto_eip97.o

ipsec_crypto_eip97-objs:= \
  mtk_crypto.o \
  $(SRC_ADAPTER)/src/adapter_driver97_init.o \
  $(SRC_ADAPTER)/src/adapter_init.o \
  $(SRC_ADAPTER)/src/adapter_interrupts.o \
  $(SRC_ADAPTER)/src/adapter_sleep.o \
  $(SRC_ADAPTER)/src/adapter_lock.o \
  $(SRC_ADAPTER)/src/adapter_global_init.o \
  $(SRC_ADAPTER)/src/adapter_global_eip97.o \
  $(SRC_ADAPTER)/src/adapter_pec_dma.o \
  $(SRC_ADAPTER)/src/adapter_ring_eip202.o \
  $(SRC_ADAPTER)/src/adapter_sglist.o \
  $(SRC_ADAPTER)/src/adapter_dmabuf.o \
  $(SRC_ADAPTER)/src/adapter_lock_internal.o \
  $(SRC_DRV_FWK)/src/lkm.o \
  $(SRC_DRV_FWK)/src/device_lkm.o \
  $(SRC_DRV_FWK)/src/dmares_lkm.o \
  $(SRC_DRV_FWK)/src/dmares_gen.o \
  $(SRC_DRV_FWK)/src/device_generic.o \
  $(SRC_KIT)/EIP97/GlobalControl/src/eip97_global_init.o \
  $(SRC_KIT)/EIP97/GlobalControl/src/eip202_global_init.o \
  $(SRC_KIT)/EIP97/GlobalControl/src/eip97_global_event.o \
  $(SRC_KIT)/EIP97/GlobalControl/src/eip97_global_prng.o \
  $(SRC_KIT)/EIP97/GlobalControl/src/eip97_global_fsm.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_cdr_init.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_cdr_dscr.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_cdr_event.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_cd_format.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_cdr_fsm.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_rdr_init.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_rdr_dscr.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_rdr_event.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_rd_format.o \
  $(SRC_KIT)/EIP97/RingControl/src/eip202_rdr_fsm.o \
  $(SRC_KIT)/EIP201/src/eip201.o \
  $(SRC_KIT)/IOToken/src/iotoken.o \
  $(SRC_KIT)/Log/src/log.o \
  $(SRC_KIT)/RingHelper/src/ringhelper.o \
  $(SRC_KIT)/SABuilder/src/sa_builder.o \
  $(SRC_KIT)/SABuilder/src/sa_builder_basic.o \
  $(SRC_KIT)/SABuilder/src/sa_builder_ipsec.o \
  $(SRC_KIT)/TokenBuilder/src/token_builder_context.o \
  $(SRC_KIT)/TokenBuilder/src/token_builder_core.o \

