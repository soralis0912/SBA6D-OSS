//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 * Author SkyLake Huang <SkyLake.Huang@mediatek.com>
 */

#ifndef __NANDX_PLATFORM_H__
#define __NANDX_PLATFORM_H__

/* #define NANDX_RANDOM_SUPPORT */


#ifndef NANDX_DTS_SUPPORT
/****** controller resource defines *******/
#define NAND_NFI_BASE        NFI_BASE
#define NAND_NFIECC_BASE     NFIECC_BASE
#define NAND_NFI_IRQ         119
#define NAND_ECC_IRQ    118

/***** other module resource defines *****/
#define NAND_CLK_BASE        CKSYS_BASE

#define NAND_CLK_SEL         (CKSYS_BASE + 0x4)
#define SNAND_CLK_SEL        (CKSYS_BASE + 0xDC)

#define NAND_GPIO_BASE       (IO_PHYS+0x5000)

/* for SLC */
#define NAND_GPIO_MODE1      (NAND_GPIO_BASE + 0x300)
#define NAND_GPIO_MODE2      (NAND_GPIO_BASE + 0x310)
#define NAND_GPIO_MODE3      (NAND_GPIO_BASE + 0x320)
#define NAND_GPIO_PUPD_CTRL0 (NAND_GPIO_BASE + 0xE00)
#define NAND_GPIO_PUPD_CTRL1 (NAND_GPIO_BASE + 0xE10)
#define NAND_GPIO_PUPD_CTRL2 (NAND_GPIO_BASE + 0xE20)
#define NAND_GPIO_PUPD_CTRL6 (NAND_GPIO_BASE + 0xE60)
#define NAND_GPIO_DRV_MODE0  (NAND_GPIO_BASE + 0xD00)
#define NAND_GPIO_DRV_MODE5  (NAND_GPIO_BASE + 0xD50)
#define NAND_GPIO_DRV_MODE6  (NAND_GPIO_BASE + 0xD60)
#define NAND_GPIO_DRV_MODE7  (NAND_GPIO_BASE + 0xD70)
#define NAND_GPIO_TDSEL6_EN  (NAND_GPIO_BASE + 0xB60)
#define NAND_GPIO_TDSEL7_EN  (NAND_GPIO_BASE + 0xB70)
#define NAND_GPIO_RDSEL1_EN  (NAND_GPIO_BASE + 0xC10)
#define NAND_GPIO_RDSELE_EN  (NAND_GPIO_BASE + 0xCE0)
#define NAND_GPIO_RDSELF_EN  (NAND_GPIO_BASE + 0xCF0)

/* SPI nand */
#define NAND_GPIO_MODE17     (NAND_GPIO_BASE + 0x460)
#define NAND_GPIO_MODE18     (NAND_GPIO_BASE + 0x470)
#define NAND_GPIO_DRV_MODE5  (NAND_GPIO_BASE + 0xD50)
#define NAND_GPIO_DRV_MODE6  (NAND_GPIO_BASE + 0xD60)
#define NAND_GPIO_RDSELC_EN  (NAND_GPIO_BASE + 0xCC0)
#define NAND_GPIO_RDSELD_EN  (NAND_GPIO_BASE + 0xCD0)

/********** reserved buffer for test ******/
#define NAND_BUF             0x5FA00000
#define NAND_BUF_LEN         0x500000
#define NAND_DATA            0x5FA00000
#define NAND_DATA_R          0x5FB00000
#define NAND_DATA_W          0x5FC00000
#define NAND_DATA_LEN        0x400000
#define NAND_OOB             0x5FE00000
#define NAND_OOB_R           0x5FE00000
#define NAND_OOB_W           0x5FE80000
#define NAND_OOB_LEN         0x100000

void nand_gpio_init(int nand_type, int pinmux_group);
void nand_clock_init(int nand_type, int sel, int *rate);
#endif
void nand_get_resource(struct nfi_resource *res);
int nand_types_support(void);

#endif /* end of __NANDX_PLATFORM_H__ */

