#
# Copyright (C) 2017 MediaTek Inc.
# Licensed under either
#     BSD Licence, (see NOTICE for more details)
#     GNU General Public License, version 2.0, (see NOTICE for more details)
#

nandx-y += platform/$(NANDX_IC_VERSION)/nandx_platform.c

nandx-$(NANDX_SIMULATOR_SUPPORT) += simulator/driver.c

nandx-$(NANDX_CTP_SUPPORT) += ctp/ts_nand.c

nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_generic.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_misc.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_nfi_spi.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_ecc.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_base.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_slc.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_spi.c
nandx-$(NANDX_VERIFY_SUPPORT) += libnt/nt_ops.c
nandx-header-$(NANDX_VERIFY_SUPPORT) += libnt/nt_ops.h
nandx-header-$(NANDX_VERIFY_SUPPORT) += libnt/nt_generic.h

nandx-$(NANDX_BBT_SUPPORT) += bbt/bbt.c
nandx-$(NANDX_BROM_SUPPORT) += brom/driver.c
nandx-$(NANDX_KERNEL_SUPPORT) += kernel/driver.c
nandx-$(NANDX_LK_SUPPORT) += lk/driver.c
nandx-$(NANDX_UBOOT_SUPPORT) += uboot/driver.c
nandx-$(NANDX_AOS_SUPPORT) += aos/driver.c
nandx-$(NANDX_AOS_SUPPORT) += aos/ftl.c

nandx-$(NANDX_BBM_SUPPORT) += bbm/bbm.c
nandx-$(NANDX_BBM_SUPPORT) += bbm/bbm_core.c
nandx-$(NANDX_BBM_SUPPORT) += bbm/bbm_policy.c
nandx-$(NANDX_BBM_SUPPORT) += bbm/policy_parse.c