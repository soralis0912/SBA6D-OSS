/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MFD_MT6330_CORE_H__
#define __MFD_MT6330_CORE_H__

#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spmi.h>
#include <linux/regmap.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>

#define MT6330_REG_WIDTH 8

enum chip_id {
	MT6330_CHIP_ID = 0x30,
};

enum mt6330_irq_top_status_shift {
	MT6330_BUCK_TOP = 8,
	MT6330_LDO_TOP = 9,
	MT6330_PSC_TOP = 10,
	MT6330_MISC_TOP = 11,
	MT6330_SCK_TOP = 13,
};

enum mt6330_irq_top_mask_shift {
	MT6330_BUCK_TOP_MASK = 0,
	MT6330_LDO_TOP_MASK = 1,
	MT6330_PSC_TOP_MASK = 2,
	MT6330_MISC_TOP_MASK = 3,
	MT6330_SCK_TOP_MASK = 5,
};

enum mt6330_irq_numbers {
	MT6330_IRQ_VCORE_OC = 0,
	MT6330_IRQ_VMD11_OC = 1,
	MT6330_IRQ_VMD12_OC = 2,
	MT6330_IRQ_VSRAM_MD_OC = 3,
	MT6330_IRQ_VRFDIG_OC = 4,
	MT6330_IRQ_VS1_OC = 5,
	MT6330_IRQ_VS2_OC = 6,
	MT6330_IRQ_VS3_OC = 7,

	MT6330_IRQ_VMC_OC = 8,
	MT6330_IRQ_VXO22_OC = 9,
	MT6330_IRQ_VRF18_OC = 10,
	MT6330_IRQ_VRF13_OC = 11,
	MT6330_IRQ_VEFUSE_OC = 12,
	MT6330_IRQ_VA12_1_OC = 13,
	MT6330_IRQ_VA12_2_OC = 14,
	MT6330_IRQ_VCN18_OC = 15,
	MT6330_IRQ_VMDDR_OC = 16,
	MT6330_IRQ_VMDDQ_OC = 17,
	MT6330_IRQ_VMDD2_OC = 18,
	MT6330_IRQ_VRF09_OC = 19,
	MT6330_IRQ_VAUX18_OC = 20,
	MT6330_IRQ_VIO18_1_OC = 21,
	MT6330_IRQ_VIO18_2_OC = 22,
	MT6330_IRQ_VSRAM_PROC_OC = 23,
	MT6330_IRQ_VSRAM_CORE_OC = 24,
	MT6330_IRQ_VSRAM_RFDIG_OC = 25,
	MT6330_IRQ_VBBCK_OC = 26,
	MT6330_IRQ_VEMC_OC = 27,
	MT6330_IRQ_VSIM1_OC = 28,
	MT6330_IRQ_VSIM2_OC = 29,
	MT6330_IRQ_VUSB_OC = 30,
	MT6330_IRQ_VRFCK_OC = 31,

	MT6330_IRQ_PWRKEY = 32,
	MT6330_IRQ_HOMEKEY = 33,
	MT6330_IRQ_PWRKEY_R = 34,
	MT6330_IRQ_HOMEKEY_R = 35,
	MT6330_IRQ_NI_LBAT_INT = 36,
	MT6330_IRQ_CHRDET_EDGE = 37,
	MT6330_IRQ_CHRDET_LEVEL = 38,

	MT6330_IRQ_RTC = 40,
	MT6330_IRQ_RCS0 = 72,
	MT6330_IRQ_RCS1 = 73,
	MT6330_IRQ_RCS2 = 74,
	MT6330_IRQ_VRC_PROTREG = 79,
	MT6330_IRQ_BUCK_PROTREG = 80,
	MT6330_IRQ_LDO_PROTREG = 81,
	MT6330_IRQ_PSC_PROTREG = 82,
	MT6330_IRQ_PLT_PROTREG = 83,
	MT6330_IRQ_HK_PROTREG = 84,
	MT6330_IRQ_SCK_PROTREG = 85,
	MT6330_IRQ_XPP_PROTREG = 86,
	MT6330_IRQ_TOP_PROTREG = 87,
	MT6330_IRQ_NR = 88,
};

#define MT6330_IRQ_BUCK_BASE MT6330_IRQ_VCORE_OC
#define MT6330_IRQ_LDO_BASE MT6330_IRQ_VMC_OC
#define MT6330_IRQ_PSC_BASE MT6330_IRQ_PWRKEY
#define MT6330_IRQ_SCK_BASE MT6330_IRQ_RTC
#define MT6330_IRQ_MISC_BASE MT6330_IRQ_RCS0

#define MT6330_IRQ_BUCK_BITS \
	(MT6330_IRQ_VS3_OC - MT6330_IRQ_BUCK_BASE)
#define MT6330_IRQ_LDO_BITS \
	(MT6330_IRQ_VRFCK_OC - MT6330_IRQ_LDO_BASE)
#define MT6330_IRQ_PSC_BITS \
	(MT6330_IRQ_CHRDET_LEVEL - MT6330_IRQ_PSC_BASE)
#define MT6330_IRQ_SCK_BITS \
	(MT6330_IRQ_RTC - MT6330_IRQ_SCK_BASE)
#define MT6330_IRQ_MISC_BITS \
	(MT6330_IRQ_TOP_PROTREG - MT6330_IRQ_MISC_BASE)

#define MT6330_TOP_GEN(sp)	\
{	\
	.hwirq_base = MT6330_IRQ_##sp##_BASE,	\
	.num_int_regs =	\
		(MT6330_IRQ_##sp##_BITS / MT6330_REG_WIDTH) + 1,	\
	.en_reg = MT6330_##sp##_TOP_INT_CON0,		\
	.en_reg_shift = 0x3,	\
	.sta_reg = MT6330_##sp##_TOP_INT_STATUS0,		\
	.sta_reg_shift = 0x1,	\
	.top_offset = MT6330_##sp##_TOP,	\
	.top_mask_offset = MT6330_##sp##_TOP_MASK,	\
}

struct mt6330_chip {
	struct device *dev;
	struct regmap *regmap;
	struct notifier_block pm_nb;
	int irq;
	struct irq_domain *irq_domain;
	struct mutex irqlock;
	u16 wake_mask[2];
	u16 irq_masks_cur[2];
	u16 irq_masks_cache[2];
	u16 int_con[2];
	u16 int_status[2];
	u16 chip_id;
	void *irq_data;
	__iomem void *spmimst_base;
	struct spmi_device *sdev;
};

int mt6330_irq_init(struct mt6330_chip *chip);

#endif /* __MFD_MT6330_CORE_H__ */
