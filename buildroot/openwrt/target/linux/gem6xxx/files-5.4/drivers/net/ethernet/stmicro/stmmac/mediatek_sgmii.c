// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#include <linux/bitfield.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/regmap.h>
#include <linux/stmmac.h>

#include "stmmac.h"
#include "stmmac_platform.h"

/* update gen1/gen2 to V2 */
void mediatek_sgmii_phy_gen1(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	pr_err("%s: v2 in\n", __func__);
	regmap_update_bits(ss->regmap[id], 0x24, GENMASK(31, 0), 0x1000fa);
	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);

	//0x00F4[0] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(0), BIT(0));
	//0x00F4[5] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(5), BIT(5));

	//0x0030[11:10] = 2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x0030, GENMASK(31, 10), 0x3 << 10);

	//0x00F8[8:0] =9'h0FA
	regmap_update_bits(ss->regmap_phy[id], 0x00F8, GENMASK(8, 0), 0x0FA);
	//0x00F8[25:16] =10'h0FA
	regmap_update_bits(ss->regmap_phy[id], 0x00F8, GENMASK(25, 16), 0x0FA << 16);

	//0x0070[26:24] =3'h2
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(26, 24), 0x2 << 24);
	//0x3080[5:4] =2'h2
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x2 << 4);

	//0x3028[17:0] =18'h08A01
	regmap_update_bits(ss->regmap_phy[id], 0x3028, GENMASK(17, 0), 0x08A01);
	//0x302C[17:0] =18'h0A884
	regmap_update_bits(ss->regmap_phy[id], 0x302C, GENMASK(17, 0), 0x0A884);
	//0x3024[20:8] =13'h0830
	regmap_update_bits(ss->regmap_phy[id], 0x3024, GENMASK(20, 8), 0x0830 << 8);
	//0x50EC[12:8] =5'h06
	regmap_update_bits(ss->regmap_phy[id], 0x50EC, GENMASK(12, 8), 0x06 << 8);

	//0x3014[23:16] =32'h08
	regmap_update_bits(ss->regmap_phy[id], 0x3014, GENMASK(15, 8), 0x08 << 8);
	//0x3018[23:16] =32'h04
	regmap_update_bits(ss->regmap_phy[id], 0x3018, GENMASK(15, 8), 0x04 << 8);
	//0x301C[23:16] =32'h08
	regmap_update_bits(ss->regmap_phy[id], 0x301C, GENMASK(15, 8), 0x08 << 8);


	//0x00EC[28:17] =12'h249
	regmap_update_bits(ss->regmap_phy[id], 0x00EC, GENMASK(28, 17), 0x249 << 17);
	//0x2230[23:20] =4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x2230, GENMASK(23, 20), 0x0);
	//0x223C[31:0] = 32'h20000000
	regmap_update_bits(ss->regmap_phy[id], 0x223C, GENMASK(31, 0), 0x20000000);
	//0x2240[31:0] =32'h20000000
	regmap_update_bits(ss->regmap_phy[id], 0x2240, GENMASK(31, 0), 0x20000000);
	//0x2244[31:0] =32'h20000000
	regmap_update_bits(ss->regmap_phy[id], 0x2244, GENMASK(31, 0), 0x20000000);
	//0x2248[31:0] =32'h20000000
	regmap_update_bits(ss->regmap_phy[id], 0x2248, GENMASK(31, 0), 0x20000000);
	//0x50F8[31:16] =16'h00FF
	regmap_update_bits(ss->regmap_phy[id], 0x50F8, GENMASK(31, 16), 0x00FF << 16);
	//0x30F8[31:20] =12'hFFF
	regmap_update_bits(ss->regmap_phy[id], 0x30F8, GENMASK(31, 20), 0xFFF << 20);
	//0x90D0[3:0] =4'h9
	regmap_update_bits(ss->regmap_phy[id], 0x90D0, GENMASK(3, 0), 0x9);
	//0x0070[15] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(15), BIT(15));
	//0x0070[14] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(14), BIT(14));
	udelay(150);
	//0x0070[0] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(0), BIT(0));


	//0x0070[8] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(8), BIT(8));

	//0x0070[13:10] =4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(13, 10), 0x0);
	//0x0070[4] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	udelay(10);

	//0x0070[4] = 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(10);

	//0x0070[17:16] = 2'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(17, 16), 0x1 << 16);
	//0x0070[4] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));

	udelay(10);

	//0x0070[4] = 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(100);
	//0x3080[5:4] =2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x3 << 4);
	//0x00F4[5] =1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(5), 0x0);
	//0x3040[29:28] = 2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x3040, GENMASK(29, 28), 0x3 << 28);
}

void mediatek_sgmii_phy_gen2(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	pr_err("%s: v2 in\n", __func__);

	regmap_update_bits(ss->regmap[id], 0x24, GENMASK(31, 0), 0x1000fa);

	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	pr_err("%s:SGMSYS_QPHY_PWR_STATE_CTRL %x\n", __func__, val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);

	//0x00F4[0] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(0), BIT(0));
	//0x00F4[5] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(5), BIT(5));

	//0x0030[11:10] = 2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x0030, GENMASK(31, 10), 0x3 << 10);

	//0x00F8[8:0] =9'h09C
	regmap_update_bits(ss->regmap_phy[id], 0x00F8, GENMASK(8, 0), 0x09C);
	//0x00F8[25:16] =10'h09C
	regmap_update_bits(ss->regmap_phy[id], 0x00F8, GENMASK(25, 16), 0x09C << 16);


	//0x0070[26:24] =3'h2
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(26, 24), 0x2 << 24);


	//0x3080[5:4] =2'h2
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x2 << 4);


	//0x3028[17:0] =18'h08A01
	regmap_update_bits(ss->regmap_phy[id], 0x3028, GENMASK(17, 0), 0x08A01);


	//0x302C[17:0] =18'h0A884
	regmap_update_bits(ss->regmap_phy[id], 0x302C, GENMASK(17, 0), 0x0A884);


	//0x3024[20:8] =13'h0830
	regmap_update_bits(ss->regmap_phy[id], 0x3024, GENMASK(20, 8), 0x0830 << 8);


	//0x50EC[12:8] =5'h06 delete

	//0x3014[23:16] =32'h50
	regmap_update_bits(ss->regmap_phy[id], 0x3014, GENMASK(15, 8), 0x50 << 8);
	//0x3018[23:16] =32'h28
	regmap_update_bits(ss->regmap_phy[id], 0x3018, GENMASK(15, 8), 0x28 << 8);
	//0x301C[23:16] =32'h50
	regmap_update_bits(ss->regmap_phy[id], 0x301C, GENMASK(15, 8), 0x50 << 8);

	//0x00EC[28:17] =12'h249
	regmap_update_bits(ss->regmap_phy[id], 0x00EC, GENMASK(28, 17), 0x249 << 17);
	//0x2230[23:20] =4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x2230, GENMASK(23, 20), 0x0);

	//0x223C[31:0] = 32'h14000000
	regmap_update_bits(ss->regmap_phy[id], 0x223C, GENMASK(31, 0), 0x14000000);
	//0x2240[31:0] =32'h14000000
	regmap_update_bits(ss->regmap_phy[id], 0x2240, GENMASK(31, 0), 0x14000000);
	//0x2244[31:0] =32'h14000000
	regmap_update_bits(ss->regmap_phy[id], 0x2244, GENMASK(31, 0), 0x14000000);
	//0x2248[31:0] =32'h14000000
	regmap_update_bits(ss->regmap_phy[id], 0x2248, GENMASK(31, 0), 0x14000000);
	//0x50F8[31:16] =16'h0055
	regmap_update_bits(ss->regmap_phy[id], 0x50F8, GENMASK(31, 16), 0x0055 << 16);
	//0x30F8[31:20] =12'hFFF
	regmap_update_bits(ss->regmap_phy[id], 0x30F8, GENMASK(31, 20), 0xFFF << 20);
	//0x90D0[3:0] =4'h7
	regmap_update_bits(ss->regmap_phy[id], 0x90D0, GENMASK(3, 0), 0x7);
	//0x0070[15] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(15), BIT(15));
	//0x0070[14] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(14), BIT(14));
	udelay(150);
	//0x0070[0] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(0), BIT(0));

	//0x0070[8] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(8), BIT(8));

	//0x0070[13:10] =4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(13, 10), 0x0);
	//0x0070[4] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	udelay(10);

	//0x0070[4] = 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(10);

	//0x0070[17:16] = 2'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(17, 16), 0x1 << 16);
	//0x0070[4] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	udelay(10);
	//0x0070[4] = 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(1000);
	//0x3080[5:4] =2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x3 << 4);
	//0x00F4[5] =1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(5), 0x0);
	//0x3040[29:28] = 2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x3040, GENMASK(29, 28), 0x3 << 28);
}

int mediatek_sgmii_setup_mode_an(struct mtk_sgmii *ss, int id)
{
	unsigned int val = 0;

	if (!ss->regmap[id])
		return -EINVAL;

	/* Setup the link timer and QPHY power up inside SGMIISYS */
	regmap_write(ss->regmap[id], SGMSYS_PCS_LINK_TIMER,
		  SGMII_LINK_TIMER_DEFAULT);

	regmap_read(ss->regmap[id], SGMSYS_SGMII_MODE, &val);
	val |= SGMII_REMOTE_FAULT_DIS;
	regmap_write(ss->regmap[id], SGMSYS_SGMII_MODE, val);

	regmap_read(ss->regmap[id], SGMSYS_PCS_CONTROL_1, &val);
	val |= SGMII_AN_RESTART;
	regmap_write(ss->regmap[id], SGMSYS_PCS_CONTROL_1, val);

	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);

	regmap_read(ss->regmap[id], SGMSYS_UTIF_CTRL, &val);
	val |= SGMII_MAC_SEL;
	regmap_write(ss->regmap[id], SGMSYS_UTIF_CTRL, val);

	return 0;
}

int mediatek_sgmii_setup_mode_force(struct mtk_sgmii *ss, int id)
{
	unsigned int val = 0;
	int mode;

	if (!ss->regmap[id])
		return -EINVAL;

	/* set SGMII PLL */
	regmap_read(ss->remap_pll, MT6990_XFI_PLL_OFFSET, &val);
	pr_err("MT6990_XFI_PLL_OFFSET before= %x \n", val);
	val |= MT6990_XFI_PLL_EN;
	pr_err("MT6990_XFI_PLL_OFFSET after= %x \n", val);
	regmap_write(ss->remap_pll, MT6990_XFI_PLL_OFFSET, val);
	udelay(1000);

	regmap_read(ss->regmap[id], ss->ana_rgc3, &val);
	val &= ~GENMASK(3, 2);
	mode = ss->flags[id] & MTK_SGMII_PHYSPEED_MASK;
	val |= (mode == MTK_SGMII_PHYSPEED_1000) ? 0 : BIT(2);
	regmap_write(ss->regmap[id], ss->ana_rgc3, val);

	/* disable SGMII AN */
	regmap_read(ss->regmap[id], SGMSYS_PCS_CONTROL_1, &val);
	val &= ~BIT(12);
	regmap_write(ss->regmap[id], SGMSYS_PCS_CONTROL_1, val);

	/* SGMII force mode setting */
	val = 0x31120009;
	regmap_write(ss->regmap[id], SGMSYS_SGMII_MODE, val);

	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);

	/* switch SGMII0 to SNPS MAC */
	regmap_read(ss->regmap[id], SGMSYS_UTIF_CTRL, &val);
	val |= SGMII_MAC_SEL;
	regmap_write(ss->regmap[id], SGMSYS_UTIF_CTRL, val);

	/*SET SGMII PHY parameter*/
	if(mode == MTK_SGMII_PHYSPEED_2500)
		mediatek_sgmii_phy_gen2(ss, id);
	else
		mediatek_sgmii_phy_gen1(ss, id);
	return 0;
}

int mediatek_sgmii_path_setup(struct mtk_sgmii *ss, int mac_id)
{
	int sid, err;

	sid = mac_id;
	
	/* Setup SGMIISYS with the determined property */
	if (MTK_HAS_FLAGS(ss->flags[sid], MTK_SGMII_PHYSPEED_AN))
		err = mediatek_sgmii_setup_mode_an(ss, sid);
	else
		err = mediatek_sgmii_setup_mode_force(ss, sid);

	if (err)
		return err;
	return 0;
}

int mediatek_sgmii_init(struct mtk_sgmii *ss, struct device_node *r, struct device *dev, u32 ana_rgc3)
{
	struct device_node *np, *np_phy;
	const char *str;
	int err;

	ss->ana_rgc3 = ana_rgc3;

	np = of_parse_phandle(r, "mediatek,sgmiisys", 0);
	if (!np)
		return -ENOMEM;

	ss->regmap[0] = syscon_node_to_regmap(np);
	if (IS_ERR(ss->regmap[0]))
		return PTR_ERR(ss->regmap[0]);

	err = of_property_read_string(np, "mediatek,physpeed", &str);
	if (err)
		return err;

	if (!strcmp(str, "2500")) {
		ss->flags[0] |= MTK_SGMII_PHYSPEED_2500;
	}
	else if (!strcmp(str, "1000"))
		ss->flags[0] |= MTK_SGMII_PHYSPEED_1000;
	else if (!strcmp(str, "auto"))
		ss->flags[0] |= MTK_SGMII_PHYSPEED_AN;
	else
		return -EINVAL;

	/* SGMII PHY init */
	np_phy = of_parse_phandle(r, "mediatek,sgmiisys_phy", 0);
	if (!np_phy)
		return -ENOMEM;

	ss->regmap_phy[0] = syscon_node_to_regmap(np_phy);
	if (IS_ERR(ss->regmap_phy[0]))
		return PTR_ERR(ss->regmap_phy[0]);

	/* SGMII PLL init */
	np_phy = of_parse_phandle(r, "mediatek,sgmii_pll", 0);
	if (!np_phy)
		return -1;

	ss->remap_pll = syscon_node_to_regmap(np_phy);
	if (IS_ERR(ss->remap_pll))
		return PTR_ERR(ss->remap_pll);

	pr_info("%s: ss->remap_pll %x \n",
		__func__, ss->remap_pll);

	return 0;
}


