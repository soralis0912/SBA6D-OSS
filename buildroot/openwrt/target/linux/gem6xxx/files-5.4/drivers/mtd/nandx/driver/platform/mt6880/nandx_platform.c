//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 * Author SkyLake Huang <SkyLake.Huang@mediatek.com>
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "nandx_platform.h"

int nand_types_support(void)
{
	return BIT(NAND_SLC);
}

#ifdef NANDX_DTS_SUPPORT
void nand_get_resource(struct nfi_resource *res)
{
	res->ic_ver = NANDX_MT6880;

	res->min_oob_req = NAND_MIN_OOB_REQ;

	res->clock_1x = 156000000;
	res->clock_2x = NULL;
	res->clock_2x_num = 8;

	/* Sector size force to 512Byte for MT8512 */
	res->force_sector_size = 512;
}

#else
void nand_get_resource(struct nfi_resource *res)
{
	res->ic_ver = NANDX_MT8512;
	res->dev = NULL;

	res->ecc_regs = NAND_NFIECC_BASE;
	res->ecc_irq_id = NAND_ECC_IRQ;

	res->nfi_regs = NAND_NFI_BASE;
	res->nfi_irq_id = NAND_NFI_IRQ;

	res->clock_1x = 26000000;
	res->clock_2x = NULL;
	res->clock_2x_num = 0;

	res->min_oob_req = NAND_MIN_OOB_REQ;

	/* Sector size force to 512Byte for MT8512 */
	res->force_sector_size = 512;
	res->force_spare_size = 0;
}

void nand_high_clock_sel(void)
{
	nandx_set_bits32(NAND_CLK_BASE + 0xb8, 0x7 << 8, 0x7 << 8);
	nandx_set_bits32(NAND_CLK_BASE + 0xb4, 0x7 << 8, 0x5 << 8);
	nandx_set_bits32(NAND_CLK_BASE + 0x4, 0x1 << 29, 0x1 << 29);

	nandx_set_bits32(NAND_CLK_BASE + 0xb8, 0x7, 0x7);
	nandx_set_bits32(NAND_CLK_BASE + 0xb4, 0x7, 0x2);
	nandx_set_bits32(NAND_CLK_BASE + 0x4, 0x1 << 28, 0x1 << 28);
}

void nand_hard_reset(void)
{
	u32 val;

	val = readl(INFRACFG_AO_BASE + 0x130);
	val |= BIT(15);
	writel(val, INFRACFG_AO_BASE + 0x130);

	udelay(5);

	val = readl(INFRACFG_AO_BASE + 0x134);
	val |= BIT(15);
	writel(val, INFRACFG_AO_BASE + 0x134);
}



void nand_gpio_init(int nand_type, int pinmux_group)
{
	pr_debug("gpio init, nand type:0x%x, pinmux group:0x%x.\n",
		 nand_type, pinmux_group);

	if (nand_type == NAND_SPI) {
		switch (pinmux_group) {
		case 0:
			pr_info("Try to setting nand_spi gpio pinmux 0.\n");
			nandx_set_bits32(GPIO_MODE7_ADDR,
					 0xFFF << 18, 0x6DB << 18);
			nandx_set_bits32(GPIO_MODE8_ADDR,
					 (0x7 << 6) | 0x7, (0x3 << 6) | 0x3);
			nandx_set_bits32(GPIO_DRV1_ADDR, 0xF << 16, 3 << 16);
			nandx_set_bits32(GPIO_DRV2_ADDR, 0xF, 3);
			nandx_set_bits32(GPIO_DRV2_ADDR, 0xF << 8, 3 << 8);
			break;
		case 1:
			pr_info("Try to setting nand_spi gpio pinmux 1.\n");
			nandx_set_bits32(GPIO_MODE1_ADDR,
					 0x7fff << 15, 0x2492 << 15);
			nandx_set_bits32(GPIO_MODE2_ADDR, 0x7 << 0, 0x2);
			break;
		default:
			break;
		}
	} else if (nand_type == NAND_SLC)
		pr_warn("SLC NOT support for MT8512 !\n");
}

/*
 * The CLK of MT8512 SNAND is clk_spinfi_bclk_sel[2:0] at 0x1000_00b0[10:8]
 * The range of spi nand clock is 26MHz to 124.8MHz.
 * 0: top_ap_clk_ctrl_f26m_ck, 26MHz
 * 1: univpll2_d8, 52MHz
 * 2: univpll3_d4, 62.4MHz
 * 3: syspll1_d8, 68.25MHz
 * 4: syspll4_d2: 78MHz
 * 5: syspll2_d4: 91MHz
 * 6: univpll2_d4: 104MHz
 * 7: univpll3_d2: 124.8MHz
 * The CLK of MT8512 NAND is clk_nfi2x_sel[2:0] at 0x1000_00b0[2:0]
 * But 8512 haven't support P-NAND
 */

void nand_clock_init(int nand_type, int sel, int *rate)
{
	if ((nand_type != NAND_SPI) && (nand_type != NAND_SLC))
		pr_err("nand type not support!\n");

	pr_debug("%s sel:%d!\n", __func__, sel);

	if (nand_type == NAND_SPI) {
		nandx_set_bits32(NAND_CLK_BASE + 0xB8, 0x700, 0x7 << 8);
		nandx_set_bits32(NAND_CLK_BASE + 0xB4, 0x700, sel << 8);
		nandx_set_bits32(NAND_CLK_BASE + 0x4, BIT(29), BIT(29));

		if (rate == NULL)
			return;

		switch (sel) {
		case 0:
			*rate = 26 * 1000 * 1000;
			break;
		case 1:
			*rate = 52 * 1000 * 1000;
			break;
		case 2:
			*rate = 62 * 1000 * 1000;
			break;
		case 3:
			*rate = 68 * 1000 * 1000;
			break;
		case 4:
			*rate = 78 * 1000 * 1000;
			break;
		case 5:
			*rate = 91 * 1000 * 1000;
			break;
		case 6:
			*rate = 104 * 1000 * 1000;
			break;
		case 7:
			*rate = 124 * 1000 * 1000;
			break;
		default:
			break;
		}
	}

	/* TODO: setting for slc */
}

void nand_cg_switch(int nand_type, int gate, bool enable)
{
	/* NOTE: all cg should be enabled in nand_clock_init. */
}

#endif
