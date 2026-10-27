// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.

/* A library for MediaTek SGMII circuit
 *
 * Author: Sean Wang <sean.wang@mediatek.com>
 *
 */

#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/regmap.h>

#include "mtk_eth_soc.h"
#include "mtk_sgmii_pwr.h"

static struct mtk_eth *g_eth;

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
void mtk_sgmii_phy_gen1(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	dev_err(g_eth->dev, "%s: v2 in\n", __func__);

	if (id < 0 || !ss->regmap[id] || !ss->regmap_phy[id])
		return ;

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

void mtk_sgmii_phy_gen2(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	dev_err(g_eth->dev, "%s: v2 in\n", __func__);

	if (id < 0 || !ss->regmap[id] || !ss->regmap_phy[id])
		return ;

	regmap_update_bits(ss->regmap[id], 0x24, GENMASK(31, 0), 0x1000fa);

	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	dev_err(g_eth->dev, "%s:SGMSYS_QPHY_PWR_STATE_CTRL %x\n", __func__, val);
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


#else
void mtk_sgmii_phy_gen1(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	dev_err(g_eth->dev, "%s: in\n", __func__);

	regmap_update_bits(ss->regmap[id], 0x24, GENMASK(31, 0), 0x1000fa);
	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);
	//0xb44[3] = 1'b0
	regmap_update_bits(ss->regmap_phy[id], 0xb44, BIT(3), (~(1<<3)) & 0xffffffff);
	//0x720[15] = 1'b1
	regmap_update_bits(ss->regmap_phy[id], 0x720, BIT(15), BIT(15));
	//0xc00[13:12] = 2'h2
	regmap_update_bits(ss->regmap_phy[id], 0xc00, GENMASK(13, 12), 0x2 << 12);
	//0xc00[17:16]=2'h2
	regmap_update_bits(ss->regmap_phy[id], 0xc00, GENMASK(17, 16), 0x2 << 16);
	//0xa00[10]=1'b1
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(10), BIT(10));
	//0xa00[16] = 1'b1
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(16), BIT(16));
	//0xc28[31:0]=32'hc04ec4ec
	//0xc2c[31:0]=32'hc04ec4ec
	//0xc30[31:0]=32'hc04ec4ec
	//0xc34[31:0]=32'hc04ec4ec
	regmap_update_bits(ss->regmap_phy[id], 0xc28, GENMASK(31, 0), 0xc04ec4ec);
	regmap_update_bits(ss->regmap_phy[id], 0xc2c, GENMASK(31, 0), 0xc04ec4ec);
	regmap_update_bits(ss->regmap_phy[id], 0xc30, GENMASK(31, 0), 0xc04ec4ec);
	regmap_update_bits(ss->regmap_phy[id], 0xc34, GENMASK(31, 0), 0xc04ec4ec);
	//0xc38[31:16]=16'h0221
	//0xc3c[15:0]=16'h0221
	//0xc3c[31:16]=16'h0221
	//0xc40[15:0]=16'h0221
	regmap_update_bits(ss->regmap_phy[id], 0xc38, GENMASK(31, 16), (0x0221) << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc3c, GENMASK(15, 0), 0x0221);
	regmap_update_bits(ss->regmap_phy[id], 0xc3c, GENMASK(31, 16), (0x0221) << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc40, GENMASK(15, 0), 0x0221);
	//0xc44[15:0]=16'h0213
	//0xc44[31:16]=16'h0213
	//0xc48[15:0]=16'h0213
	//0xc48[31:16]=16'h0213
	regmap_update_bits(ss->regmap_phy[id], 0xc44, GENMASK(15, 0), 0x0213);
	regmap_update_bits(ss->regmap_phy[id], 0xc44, GENMASK(31, 16), (0x0213) << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc48, GENMASK(15, 0), 0x0213);
	regmap_update_bits(ss->regmap_phy[id], 0xc48, GENMASK(31, 16), (0x0213) << 16);
	//0x97c[27:24]=4'h1
	//0x97c[31:28]=4'h1
	regmap_update_bits(ss->regmap_phy[id], 0x97c, GENMASK(27, 24), 0x1 << 24);
	regmap_update_bits(ss->regmap_phy[id], 0x97c, GENMASK(31, 28), 0x1 << 28);
	//0xc04[0]=1'h1
	//0xc04[1]=1'h1
	//0xc04[2]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc04, BIT(0), BIT(0));
	regmap_update_bits(ss->regmap_phy[id], 0xc04, BIT(1), BIT(1));
	regmap_update_bits(ss->regmap_phy[id], 0xc04, BIT(2), BIT(2));
	//0xc00[20]=1'h1
	//0xc00[21]=1'h0
	//0xc00[22]=1'h0
	regmap_update_bits(ss->regmap_phy[id], 0xc00, BIT(20), BIT(20));
	regmap_update_bits(ss->regmap_phy[id], 0xc00, BIT(21), (~(1<<21)) & 0xffffffff);
	regmap_update_bits(ss->regmap_phy[id], 0xc00, BIT(22), (~(1<<22)) & 0xffffffff);
	//0xc64[28]=1'h1
	//0xc64[29]=1'h1
	//0xc64[30]=1'h1
	//0xc64[31]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(28), BIT(28));
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(29), BIT(29));
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(30), BIT(30));
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(31), BIT(31));
	/*
	0xc80[0]=1'h1
	0xc80[2]=1'h1
	0xc80[4]=1'h1
	0xc80[6]=1'h1
	0xc80[12:11]=2'h2
	0xc80[15:14]=2'h2
	0xc80[18:17]=2'h2
	0xc80[21:20]=2'h2
	0xc80[27:26]=2'h0
	0xc80[30:29]=2'h0*/
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(0), BIT(0));
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(2), BIT(2));
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(4), BIT(4));
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(6), BIT(6));
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(12, 11), 0x2 << 11);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(15, 14), 0x2 << 14);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(18, 17), 0x2 << 17);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(21, 20), 0x2 << 20);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(27, 26), 0x0 << 26);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(30, 29), 0x0 << 29);
	//0xc84[1:0]=2'h0
	//0xc84[4:3]=2'h0
	regmap_update_bits(ss->regmap_phy[id], 0xc84, GENMASK(1, 0), 0x0);
	regmap_update_bits(ss->regmap_phy[id], 0xc84, GENMASK(4, 3), 0x0 <<3);
	//0x928[31]=1'h1
	//0x928[30]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x928, BIT(31), BIT(31));
	regmap_update_bits(ss->regmap_phy[id], 0x928, BIT(30), BIT(30));
	/*
	0xa78[4:0]=5'h04
	0xa78[10:6]=5'h01
	0xa78[16:12]=5'h04
	0xa78[22:18]=5'h04
	0xa78[28:24]=5'h01*/
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(4, 0), 0x4);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(10, 6), 0x1 << 6);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(16, 12), 0x4 << 12);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(22, 18), 0x4 << 18);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(28, 24), 0x1 << 24);
	//0xa7c[4:0] = 0x04
	regmap_update_bits(ss->regmap_phy[id], 0xa7c, GENMASK(4, 0), 0x4);
	//0xc88[7:4] = 0
	//0xc88[11:8]= 0
	//0xc88[15:12] = 0
	//0xc88[19:16] = 4'h0
	//0xc88[25:23]=3'h4
	//0xc88[28:26]=3'h4
	//0xc88[31:29]=3'h4
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(7, 4), 0x0 << 4);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(11, 8), 0x0 << 8);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(15, 12), 0x0 << 12);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(19, 16), 0x0 << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(25, 23), 0x4 << 23);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(28, 26), 0x4 << 26);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(31, 29), 0x4 << 29);
	//0xc8c[2:0]=3'h4
	regmap_update_bits(ss->regmap_phy[id], 0xc8c, GENMASK(2, 0), 0x4);
	//0xb20[31:28]=4'h3
	regmap_update_bits(ss->regmap_phy[id], 0xb20, GENMASK(31, 28), 0x3 << 28);
	//0x9e8[19:15]=5'h14
	//0x9e8[14]=1'h1
	//0x9e8[25:21]=5'h00
	//0x9e8[20]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, GENMASK(19, 15), 0x14 << 15);
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, BIT(14), BIT(14));
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, GENMASK(25, 21), 0x0 << 21);
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, BIT(20), BIT(20));
	//0x948[29:24]=6'h3f
	regmap_update_bits(ss->regmap_phy[id], 0x948, GENMASK(29, 24), 0x3f << 24);
	//0x994[18:16]=3'h0
	regmap_update_bits(ss->regmap_phy[id], 0x994, GENMASK(18, 16), 0x0 << 16);
	//0x938[15]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x938, BIT(15), BIT(15));
	//0x940[30]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x940, BIT(30), BIT(30));
	//0x960[24:15]=10'h0
	regmap_update_bits(ss->regmap_phy[id], 0x960, GENMASK(24, 15), 0x0 << 15);
	//0x720[13]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x720, BIT(13), BIT(13));
	//0x810[24]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x810, BIT(24), BIT(24));
	//usleep_range(200, 100);
	//0x940[11]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x940, BIT(11), BIT(11));
	//usleep_range(200, 100);
	//0xa00[6]=1'h1
	//0xa00[5]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(6), BIT(6));
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(5), BIT(5));
	//usleep_range(200, 100);
	//0x720[13]=1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x720, BIT(13), (~(1<<13)) & 0xffffffff);
	//usleep_range(200, 100);
	//0x934[20]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x934, BIT(20), 0x1 << 20);

}

void mtk_sgmii_phy_gen2(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	dev_err(g_eth->dev, "%s: in\n", __func__);

	regmap_update_bits(ss->regmap[id], 0x24, GENMASK(31, 0), 0x1000fa);

	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);
	//0xb44[3] = 1'b0
	regmap_update_bits(ss->regmap_phy[id], 0xb44, BIT(3), (~(1<<3)) & 0xffffffff);
	//0x720[15] = 1'b1
	//regmap_update_bits(ss->regmap_phy[id], 0x720, BIT(15), 1);
	regmap_read(ss->regmap_phy[id], 0x720, &val);
	val &= ~BIT(15);
	val |= BIT(15);
	regmap_write(ss->regmap_phy[id], 0x720, val);
	//0xc00[13:12] = 2'h2
	//0xc00[17:16] = 2'h2
	regmap_read(ss->regmap_phy[id], 0xc00, &val);
	//regmap_update_bits(ss->regmap_phy[id], 0xc00, GENMASK(13, 12), 0x2);
	//regmap_update_bits(ss->regmap_phy[id], 0xc00, GENMASK(17, 16), 0x2);
	regmap_read(ss->regmap_phy[id], 0xc00, &val);
	val &= ~GENMASK(13, 12);
	val |= (0x2 << 12);
	regmap_write(ss->regmap_phy[id], 0xc00, val);
	regmap_read(ss->regmap_phy[id], 0xc00, &val);
	val &= ~GENMASK(17, 16);
	val |= (0x2 << 16);
	regmap_write(ss->regmap_phy[id], 0xc00, val);
	//0xa00[10]=1'b1
	//0xa00[16]=1'b1
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(10), BIT(10));
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(16), BIT(16));
	//0xc28[31:0]=32'hf0627627
	//0xc2c[31:0]=32'hf0627627
	//0xc30[31:0]=32'hf0627627
	//0xc34[31:0]=32'hf0627627
	regmap_update_bits(ss->regmap_phy[id], 0xc28, GENMASK(31, 0), 0xf0627627);
	regmap_update_bits(ss->regmap_phy[id], 0xc2c, GENMASK(31, 0), 0xf0627627);
	regmap_update_bits(ss->regmap_phy[id], 0xc30, GENMASK(31, 0), 0xf0627627);
	regmap_update_bits(ss->regmap_phy[id], 0xc34, GENMASK(31, 0), 0xf0627627);
	//0xc38[31:16]=16'h02aa
	//0xc3c[15:0]=16'h02aa
	//0xc3c[31:16]=16'h02aa
	//0xc40[15:0]=16'h02aa
	regmap_update_bits(ss->regmap_phy[id], 0xc38, GENMASK(31, 16), 0x02aa << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc3c, GENMASK(15, 0), 0x02aa);
	regmap_update_bits(ss->regmap_phy[id], 0xc3c, GENMASK(31, 16), 0x02aa << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc40, GENMASK(15, 0), 0x02aa);
	//0xc44[15:0]= 16'h0297
	//0xc44[31:16]=16'h0297
	//0xc48[15:0]= 16'h0297
	//0xc48[31:16]=16'h0297
	regmap_update_bits(ss->regmap_phy[id], 0xc44, GENMASK(15, 0), 0x0297);
	regmap_update_bits(ss->regmap_phy[id], 0xc44, GENMASK(31, 16), 0x0297<<16);
	regmap_update_bits(ss->regmap_phy[id], 0xc48, GENMASK(15, 0), 0x0297);
	regmap_update_bits(ss->regmap_phy[id], 0xc48, GENMASK(31, 16), 0x0297<<16);
	//0x97c[27:24]=4'h3
	//0x97c[31:28]=4'h3
	regmap_update_bits(ss->regmap_phy[id], 0x97c, GENMASK(27, 24), 0x3<<24);
	regmap_update_bits(ss->regmap_phy[id], 0x97c, GENMASK(31, 28), 0x3<<28);
	//0xc04[0]= 1'h1
	//0xc04[1]= 1'h1
	//0xc04[2]= 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc04, BIT(0), BIT(0));
	regmap_update_bits(ss->regmap_phy[id], 0xc04, BIT(1), BIT(1));
	regmap_update_bits(ss->regmap_phy[id], 0xc04, BIT(2), BIT(2));
	//0xc00[20]=1'h1
	//0xc00[21]=1'h0
	//0xc00[22]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc00, BIT(20), BIT(20));
	regmap_update_bits(ss->regmap_phy[id], 0xc00, BIT(21),(~(1<<21)) & 0xffffffff);
	regmap_update_bits(ss->regmap_phy[id], 0xc00, BIT(22), BIT(22));
	//0xc64[28]=1'h1
	//0xc64[29]=1'h1
	//0xc64[30]=1'h1
	//0xc64[31]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(28), BIT(28));
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(29), BIT(29));
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(30), BIT(30));
	regmap_update_bits(ss->regmap_phy[id], 0xc64, BIT(31), BIT(31));
	//0xc80[0]= 1'h0
	//0xc80[2]= 1'h0
	//0xc80[4]= 1'h0
	//0xc80[6]= 1'h0
	//0xc80[12:11]=2'h1
	//0xc80[15:14]=2'h1
	//0xc80[18:17]=2'h1
	//0xc80[21:20]=2'h1
	//0xc80[27:26]=2'h1
	//0xc80[30:29]=2'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(0), (~(1<<0)) & 0xffffffff);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(2), (~(1<<2)) & 0xffffffff);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(4), (~(1<<4)) & 0xffffffff);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, BIT(6), (~(1<<6)) & 0xffffffff);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(12, 11), 0x1 << 11);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(15, 14), 0x1 << 14);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(18, 17), 0x1<< 17);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(21, 20), 0x1 << 20);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(27, 26), 0x1<< 26);
	regmap_update_bits(ss->regmap_phy[id], 0xc80, GENMASK(30, 29), 0x1<< 29);
	//0xc84[1:0]=  2'h1
	//0xc84[4:3]=  2'h1
	regmap_update_bits(ss->regmap_phy[id], 0xc84, GENMASK(1, 0), 0x1);
	regmap_update_bits(ss->regmap_phy[id], 0xc84, GENMASK(4, 3), 0x1 <<3);
	//0xa78[4:0]=  5'h09
	//0xa78[10:6]= 5'h02
	//0xa78[16:12]=5'h09
	//0xa78[22:18]=5'h09
	//0xa78[28:24]=5'h02
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(4, 0), 0x9);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(10, 6), 0x2 << 6);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(16, 12), 0x9 <<12);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(22, 18), 0x9<<18);
	regmap_update_bits(ss->regmap_phy[id], 0xa78, GENMASK(28, 24), 0x2<<24);
	//0xa7c[4:0]=  5'h09
	regmap_update_bits(ss->regmap_phy[id], 0xa7c, GENMASK(4, 0), 0x9);
	//0xc88[7:4]=  4'h2
	//0xc88[11:8]= 4'h2
	//0xc88[15:12]=4'h2
	//0xc88[19:16]=4'h2
	//0xc88[25:23]=3'h3
	//0xc88[28:26]=3'h3
	//0xc88[31:29]=3'h3
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(7, 4), 0x2 << 4);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(11, 8), 0x2 << 8);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(15, 12), 0x2 << 12);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(19, 16), 0x2 << 16);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(25, 23), 0x3 << 23);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(28, 26), 0x3 << 26);
	regmap_update_bits(ss->regmap_phy[id], 0xc88, GENMASK(31, 29), 0x3 << 29);
	//0xc8c[2:0]=3'h3
	regmap_update_bits(ss->regmap_phy[id], 0xc8c, GENMASK(2, 0), 0x3);
	//0xb20[31:28]=4'h1
	regmap_update_bits(ss->regmap_phy[id], 0xb20, GENMASK(31, 28), 0x1 << 28);
	//0x9e8[19:15]=5'h15
	//0x9e8[14]=1'h1
	//0x9e8[25:21]=5'h10
	//0x9e8[20]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, GENMASK(19, 15), 0x15 <<15);
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, BIT(14), BIT(14));
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, GENMASK(25, 21), 0x10 << 21);
	regmap_update_bits(ss->regmap_phy[id], 0x9e8, BIT(20), BIT(20));
	//0x9ec[23:20]=4'h6
	//0x9ec[30:26]=5'h19
	regmap_update_bits(ss->regmap_phy[id], 0x9ec, GENMASK(23, 20), 0x6 << 20);
	regmap_update_bits(ss->regmap_phy[id], 0x9ec, GENMASK(30, 26), 0x19 << 26);
	//0x948[29:24]=6'h3f
	regmap_update_bits(ss->regmap_phy[id], 0x948, GENMASK(29, 24), 0x3f << 24);
	//0x994[18:16]=3'h1
	regmap_update_bits(ss->regmap_phy[id], 0x994, GENMASK(18, 16), 0x1 <<16);
	//0x938[15]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x938, BIT(15), BIT(15));
	//0x940[30]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x940, BIT(30), BIT(30));
	//0x720[13]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x720, BIT(13), BIT(13));
	//0x810[24]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x810, BIT(24), BIT(24));
	udelay(1000);
	//0x940[11]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x940, BIT(11), BIT(11));
	//0xa00[6]= 1'h1
	//0xa00[5]= 1'h1
	udelay(100);
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(6), BIT(6));
	regmap_update_bits(ss->regmap_phy[id], 0xa00, BIT(5), BIT(5));
	udelay(1000);
	//0x720[13]=1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x720, BIT(13), (~(1<<13)) & 0xffffffff);
	udelay(1000);
	//0x934[20= 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x934, BIT(20), BIT(20));
}

#endif


int mtk_usxgmii_phy_init(struct mtk_sgmii *ss,struct device_node *r,
	struct mtk_eth *eth)
{
	struct device_node *np_phy;
	int i;

	g_eth = eth;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		np_phy = of_parse_phandle(r, "mediatek,sgmiisys_uxphy", i);
		if (!np_phy)
			break;

		ss->regmap_uxphy[i] = syscon_node_to_regmap(np_phy);
		if (IS_ERR(ss->regmap_uxphy[i])) {
			pr_info("%s: [ERR] ss->regmap_uxphy[%d]: %x \n",
				__func__, i, ss->regmap_uxphy[i]);
			return PTR_ERR(ss->regmap_uxphy[i]);
		}
	}

	return 0;
}

int mtk_sgmii_pll_init(struct mtk_sgmii *ss,struct device_node *r,
	struct mtk_eth *eth)
{
	struct device_node *np_phy;
	int i;

	np_phy = of_parse_phandle(r, "mediatek,sgmii_pll", 0);
	if (!np_phy)
		return -1;

	ss->remap_pll = syscon_node_to_regmap(np_phy);
	if (IS_ERR(ss->remap_pll)) {
		pr_info("%s: [ERR] ss->remap_pll %x \n", __func__, ss->remap_pll);
		return PTR_ERR(ss->remap_pll);
	}

	return 0;
}


int mtk_sgmii_phy_init(struct mtk_sgmii *ss, struct device_node *r, struct mtk_eth *eth)
{
	struct device_node *np_phy;
	int i;
	unsigned int val;

	g_eth = eth;


	for (i = 0; i < MTK_MAX_DEVS; i++) {
		np_phy = of_parse_phandle(r, "mediatek,sgmiisys_phy", i);
		if (!np_phy)
			break;

		ss->regmap_phy[i] = syscon_node_to_regmap(np_phy);
		if (IS_ERR(ss->regmap_phy[i])) {
			pr_info("%s: [ERR] ss->regmap_phy[%d]: %x \n",
				__func__, i, ss->regmap_phy[i]);
			return PTR_ERR(ss->regmap_phy[i]);
		}

	}
	return 0;
}


int mtk_sgmii_init(struct mtk_sgmii *ss, struct device_node *r, u32 ana_rgc3)
{
	struct device_node *np;
	int i, err;
	const char *str;

	ss->ana_rgc3 = ana_rgc3;


	for (i = 0; i < MTK_MAX_DEVS; i++) {
		np = of_parse_phandle(r, "mediatek,sgmiisys", i);
		if (!np)
			break;

		ss->regmap[i] = syscon_node_to_regmap(np);
		if (IS_ERR(ss->regmap[i])) {
			pr_info("%s: ss->regmap %d = %x\n",
				__func__, i, ss->regmap[i]);
			return PTR_ERR(ss->regmap[i]);
		}

	}
	return 0;
}

void dump_sgmii(struct mtk_sgmii *ss, u32 id) {

	unsigned int val = 0, val2 = 0, val3 = 0;

	if (!ss->regmap[id])
		return ;

	if (IS_ERR(ss->regmap[id]))
		dev_err(g_eth->dev,"IS_ERR(ss->regmap[id] Fail\n");

	//debug info
	regmap_read(ss->regmap[id], 0x128, &val);
	regmap_read(ss->regmap[id], 0x20, &val2);
	regmap_read(ss->regmap[id], SGMSYS_PCS_CONTROL_1, &val3);

	dev_err(g_eth->dev, "[%d] 0x128= %x 0x020 = %x PCS_control 0x000 = %x\n",
		id, val, val2, val3);
}


int mtk_sgmii_setup_mode_an(struct mtk_sgmii *ss, int id,
	struct mtk_eth *eth, int speed)
{
	unsigned int val;
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	unsigned int sgmii_phy_addr, ppsc;
#endif
	if (!ss->regmap[id])
		return -EINVAL;

	pr_info("mtk_sgmii_setup_mode_an mac_id:%d speed:%d\n", id, speed);
	mtk_sgmii_setup_mode_force(ss, id, speed);
	regmap_update_bits(ss->regmap[id], 0x128, GENMASK(3, 2), 0x0 << 2);
//	mtk_w32(eth, 0x21057300, MTK_MAC_MCR(id));
	regmap_update_bits(ss->regmap[id],SGMSYS_PCS_CONTROL_1 , GENMASK(31,0), 0x41140);
	regmap_update_bits(ss->regmap[id], 0x20, GENMASK(31,0), 0x3112000B);
	/*	TODO: MTK_MAC_PPSC phy id should move to dts
	*/
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	sgmii_phy_addr = (eth->mac_data[0].sgmii_phy_addr<<0) | (eth->mac_data[1].sgmii_phy_addr<<8);
	ppsc = 0xc5000000 | sgmii_phy_addr;
	mtk_w32(eth, ppsc, MTK_MAC_PPSC);	//bit:12-8 is map sgmii1  bit:4-0 is map sgmii0
	pr_info("sgmii%d phy addr = 0x%x ppsc:%0x\n", \
				id, sgmii_phy_addr, ppsc);
#else
	mtk_w32(eth, 0xc5000808, MTK_MAC_PPSC);
#endif
	regmap_read(ss->regmap[id], SGMSYS_PCS_CONTROL_1, &val);
	pr_info("AN PCS_control[%d] = %x \n", id, val);
	return 0;
}

void dump_usxgmii(struct mtk_sgmii *ss, int id) {

	unsigned int val = 0, val2 = 0;

	if (id < 0)
		return ;

	if (!ss->regmap_uxphy[id])
		return;

	regmap_read(ss->regmap_uxphy[id], MT6890_RG_PCS_AN_CTRL_0, &val);
	regmap_read(ss->regmap_uxphy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, &val2);

	dev_err(g_eth->dev, "[%d] 0x810= %x 0x81C = %x\n",
		id, val, val2);
}

int mtk_sgmii_setup_mode_force(struct mtk_sgmii *ss, int id, int speed)
{
	unsigned int val;

	if (!ss->regmap[id])
		return -EINVAL;

	if (MTK_HAS_CAPS(g_eth->soc->caps, MTK_NETSYS2)) {
		regmap_read(ss->remap_pll, MT6990_XFI_PLL_OFFSET, &val);
		val |= MT6990_XFI_PLL_EN;
		regmap_write(ss->remap_pll, MT6990_XFI_PLL_OFFSET, val);

	}

	regmap_read(ss->regmap[id], ss->ana_rgc3, &val);
	val &= ~RG_PHY_SPEED_MASK;
	if (speed == SPEED_2500)
		val |= RG_PHY_SPEED_3_125G;
	regmap_write(ss->regmap[id], ss->ana_rgc3, val);

	dev_err(g_eth->dev,"ana_rgc3= %x val =%x speed:%d\n", ss->ana_rgc3, val, speed);

	/* Disable SGMII AN */
	regmap_read(ss->regmap[id], SGMSYS_PCS_CONTROL_1, &val);
	val &= ~SGMII_AN_ENABLE;
	regmap_write(ss->regmap[id], SGMSYS_PCS_CONTROL_1, val);

	/* SGMII force mode setting */
/*	regmap_read(ss->regmap[id], SGMSYS_SGMII_MODE, &val);
	val &= ~SGMII_IF_MODE_MASK;

	switch (speed) {
	case SPEED_10:
		val |= SGMII_SPEED_10;
		break;
	case SPEED_100:
		val |= SGMII_SPEED_100;
		break;
	case SPEED_2500:
	case SPEED_1000:
		val |= SGMII_SPEED_1000;
		break;
	};

	if (state->duplex != DUPLEX_FULL)
		val |= SGMII_DUPLEX_FULL;*/

	/* SGMII force mode setting */
	val = 0x31120009;
	regmap_write(ss->regmap[id], SGMSYS_SGMII_MODE, val);

	/* Release PHYA power down state */
	regmap_read(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, &val);
	val &= ~SGMII_PHYA_PWD;
	regmap_write(ss->regmap[id], SGMSYS_QPHY_PWR_STATE_CTRL, val);

	if (MTK_HAS_CAPS(g_eth->soc->caps, MTK_SOC_MT6890)) {

		/*SET SGMII PHY parameter*/
		if(speed == SPEED_2500)
			mtk_sgmii_phy_gen2(ss, id);
		else
			mtk_sgmii_phy_gen1(ss, id);

		//debug info
		dump_sgmii(ss, id);
	}

	return 0;
}

void mtk_sgmii_restart_an(struct mtk_eth *eth, int mac_id)
{
	struct mtk_sgmii *ss = eth->sgmii;
	unsigned int val, sid;

	/* Decide how GMAC and SGMIISYS be mapped */
	sid = (MTK_HAS_CAPS(eth->soc->caps, MTK_SHARED_SGMII)) ?
	       0 : mac_id;


	dev_err(g_eth->dev,"%x id: %d\n",
			__func__, mac_id);

	if (!ss->regmap[sid])
		return;

	regmap_read(ss->regmap[sid], SGMSYS_PCS_CONTROL_1, &val);
	val |= SGMII_AN_RESTART;
	regmap_write(ss->regmap[sid], SGMSYS_PCS_CONTROL_1, val);
}



static void mtk_usxgmii_setup_phya_an_5000(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	dev_err(g_eth->dev,"%s id: %d\n",
		__func__, id);

	if (id < 0 || !ss->regmap_phy[id])
		return ;

	//0x00f4[0]=1'h0
	regmap_update_bits(ss->regmap_phy[id], 0xf4, BIT(0), 0x0);
	//0x00f4[5]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xf4, BIT(5), BIT(5));
	//0x0030[11:10]=2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x30, GENMASK(11, 10), 0x3 << 10);
	//0x00F8[8:0]= 9'h0A1
	regmap_update_bits(ss->regmap_phy[id], 0xf8, GENMASK(8, 0), 0x0A1);
	//0x00F8[25:16]= 10'h0A1
	regmap_update_bits(ss->regmap_phy[id], 0xf8, GENMASK(25, 16), 0x0A1 << 16);
	//0x5064[1:0] = 2'h1
	regmap_update_bits(ss->regmap_phy[id], 0x5064, GENMASK(1, 0), 0x1);
	//0x50B4[21:20] = 2'h1
	regmap_update_bits(ss->regmap_phy[id], 0x50b4, GENMASK(21, 20), 0x1 << 20);
	//0x0070[26:24] = 3'h5
	regmap_update_bits(ss->regmap_phy[id], 0x70, GENMASK(26, 24), 0x5 << 24);
	//0x3080[5:4] = 2'h2
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x2 << 4);
	//0x3028[17:0] =18'h08A01
	regmap_update_bits(ss->regmap_phy[id], 0x3028, GENMASK(17, 0), 0x8A01);
	//0x302C[17:0] =18'h0A884
	regmap_update_bits(ss->regmap_phy[id], 0x302C, GENMASK(17, 0), 0xA884);
	//0x3024[20:8] =13'h0830
	regmap_update_bits(ss->regmap_phy[id], 0x3024, GENMASK(20, 8), 0x830 << 8);
	//0x50EC[12:8] delete

	//0x4000[13] = 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x4000, BIT(13), 0x0);
	//0x50E0[14] = 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x50E0, BIT(14), 0x0);
	//0x50A8[23:16] = 8'h3B
	regmap_update_bits(ss->regmap_phy[id], 0x50A8, GENMASK(23, 16), 0x3B << 16);
	//0x5098[20:16] = 5'h08
	regmap_update_bits(ss->regmap_phy[id], 0x5098, GENMASK(20, 16), 0x08 << 16);
	//0x5054[20:16] =5'h10
	regmap_update_bits(ss->regmap_phy[id], 0x5054, GENMASK(20, 16), 0x10 << 16);
	//0x30D4[21:16] = 6'h00
	regmap_update_bits(ss->regmap_phy[id], 0x30D4, GENMASK(21, 16), 0x0);
	//0x30D8[21:16] = 6'h00
	regmap_update_bits(ss->regmap_phy[id], 0x30D8, GENMASK(21, 16), 0x0);


	//0x3014[31:0] = 32'h60606060
	regmap_update_bits(ss->regmap_phy[id], 0x3014, GENMASK(31, 0), 0x60606060);
	//0x3018[31:0] =32'h30303030
	regmap_update_bits(ss->regmap_phy[id], 0x3018, GENMASK(31, 0), 0x30303030);
	//0x301C[31:0] =32'h60606060
	regmap_update_bits(ss->regmap_phy[id], 0x301C, GENMASK(31, 0), 0x60606060);
	//0x00EC[28:17] = 12'h492
	regmap_update_bits(ss->regmap_phy[id], 0x00EC, GENMASK(28, 17), 0x492 << 17);
	//0x2230[23:20] = 4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x2230, GENMASK(23, 20), 0x0);
	//0x223C[31:0] = 32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x223C, GENMASK(31, 0), 0x21000000);
	//0x2240[31:0] =32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x2240, GENMASK(31, 0), 0x21000000);
	// 0x2244[31:0] =32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x2244, GENMASK(31, 0), 0x21000000);
	//0x2248[31:0] = 32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x2248, GENMASK(31, 0), 0x21000000);
	//0x50F8[31:16] =16'h5555
	regmap_update_bits(ss->regmap_phy[id], 0x50F8, GENMASK(31, 16), 0x5555 << 16);
	//0x30F8[31:20] =12'h492
	regmap_update_bits(ss->regmap_phy[id], 0x30F8, GENMASK(31, 20), 0x492 << 20);
	//0x90D0[3:0] = 4'h3
	regmap_update_bits(ss->regmap_phy[id], 0x90D0, GENMASK(3, 0), 0x3);
	//0x0070[15] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(15), BIT(15));
	//0x0070[14] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(14), BIT(14));
	udelay(100);
	//0x0070[0] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(0), BIT(0));
	//0x0070[13:10] =4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(13, 10), 0x0);
	//0x0070[4] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	//0x0070[4] =1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(0), 0x0);
	udelay(100);
	//0x0070[17:16] =2'h2
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(17, 16), 0x2 << 16);
	//0x0070[4] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	//0x0070[4]= 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(100);
	//0x3080[5:4] = 2'h0
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x0);
	//0x00F4[5] =1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(5), 0x0);
	//0x3040[29:28] = 2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x3040, GENMASK(29, 28), 0x3 << 28);
}

static void mtk_usxgmii_setup_phya_an_10000(struct mtk_sgmii *ss, int id)
{
	unsigned int val;

	dev_err(g_eth->dev,"%s id: %d\n", __func__, id);

	if (id < 0 || !ss->regmap_phy[id])
		return ;

	//0x00f4[0]=1'h0
	regmap_update_bits(ss->regmap_phy[id], 0xf4, BIT(0), 0x0);
	//0x00f4[5]=1'h1
	regmap_update_bits(ss->regmap_phy[id], 0xf4, BIT(5), BIT(5));
	//0x0030[11:10]=2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x30, GENMASK(11, 10), 0x3 << 10);
	//0x00F8[8:0]= 9'h142
	regmap_update_bits(ss->regmap_phy[id], 0xf8, GENMASK(8, 0), 0x142);
	//0x00F8[25:16]= 10'h142
	regmap_update_bits(ss->regmap_phy[id], 0xf8, GENMASK(25, 16), 0x142 << 16);
	//0x5064[1:0] = 2'h1
	regmap_update_bits(ss->regmap_phy[id], 0x5064, GENMASK(1, 0), 0x1);
	//0x50B4[21:20] = 2'h1
	regmap_update_bits(ss->regmap_phy[id], 0x50b4, GENMASK(21, 20), 0x1 << 20);
	//0x0070[26:24] = 3'h2
	regmap_update_bits(ss->regmap_phy[id], 0x70, GENMASK(26, 24), 0x2 << 24);
	udelay(200);

	//0x3080[5:4] = 2'h2
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x2 << 4);

	//0x3028[17:0] =18'h08A01
	regmap_update_bits(ss->regmap_phy[id], 0x3028, GENMASK(17, 0), 0x8A01);
	//0x302C[17:0] =18'h0A884
	regmap_update_bits(ss->regmap_phy[id], 0x302C, GENMASK(17, 0), 0xA884);
	//0x3024[20:8] =13'h0830
	regmap_update_bits(ss->regmap_phy[id], 0x3024, GENMASK(20, 8), 0x830 << 8);
	//0x50EC[12:8] delete

	//0x3014[23:16] = 8'hFF
	regmap_update_bits(ss->regmap_phy[id], 0x3014, GENMASK(23, 16), 0xff << 16);
	//0x3018[23:16] = 8'h7F
	regmap_update_bits(ss->regmap_phy[id], 0x3018, GENMASK(23, 16), 0x7f << 16);
	//0x301C[23:16] = 8'hFF
	regmap_update_bits(ss->regmap_phy[id], 0x301C, GENMASK(23, 16), 0xff << 16);
	//0x00EC[28:17] = 12'h492
	regmap_update_bits(ss->regmap_phy[id], 0x00EC, GENMASK(28, 17), 0x492 << 17);
	//0x2230[23:20] = 4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x2230, GENMASK(23, 20), 0x0);

	//0x223C[31:0] = 32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x223C, GENMASK(31, 0), 0x21000000);
	//0x2240[31:0] =32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x2240, GENMASK(31, 0), 0x21000000);
	// 0x2244[31:0] =32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x2244, GENMASK(31, 0), 0x21000000);
	//0x2248[31:0] = 32'h21000000
	regmap_update_bits(ss->regmap_phy[id], 0x2248, GENMASK(31, 0), 0x21000000);
	//0x50F8[31:16] =16'hAA00
	regmap_update_bits(ss->regmap_phy[id], 0x50F8, GENMASK(31, 16), 0xAA00 << 16);
	//0x30F8[31:20] =12'h000
	regmap_update_bits(ss->regmap_phy[id], 0x30F8, GENMASK(31, 20), 0x0);
	regmap_update_bits(ss->regmap_phy[id], 0x3048, GENMASK(14, 8), 0x40 << 8);
	regmap_update_bits(ss->regmap_phy[id], 0x3048, GENMASK(22, 16), 0x70 << 16);
	regmap_update_bits(ss->regmap_phy[id], 0x3048, GENMASK(30, 24), 0x40 << 24);
	//don't set 0x3050/0x3054
	// regmap_update_bits(ss->regmap_phy[id], 0x3050, GENMASK(31, 26), 0x2A << 26);
	// regmap_update_bits(ss->regmap_phy[id], 0x3054, GENMASK(7, 0), 0xAA);
	regmap_update_bits(ss->regmap_phy[id], 0x306C, GENMASK(11, 8), 0xF << 8);
	regmap_update_bits(ss->regmap_phy[id], 0xA060, GENMASK(10, 8), 0x4 << 8);
	//0x90D0[3:0] = 4'h1
	regmap_update_bits(ss->regmap_phy[id], 0x90D0, GENMASK(3, 0), 0x1);
	//0x0070[15] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(15), BIT(15));
	//0x0070[14] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(14), BIT(14));
	udelay(150);
	//0x0070[0] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(0), BIT(0));
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(8), BIT(8));
	//0x0070[13:10] =4'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(13, 10), 0x0);
	//0x0070[4] =1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	udelay(1);
	//0x0070[4] =1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(10);
	//0x0070[17:16] =2'h2
	regmap_update_bits(ss->regmap_phy[id], 0x0070, GENMASK(17, 16), 0x2 << 16);
	//0x0070[4] = 1'h1
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), BIT(4));
	udelay(1);
	//0x0070[4]= 1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x0070, BIT(4), 0x0);
	udelay(100);
	//0x3080[5:4] = 2'h0
	regmap_update_bits(ss->regmap_phy[id], 0x3080, GENMASK(5, 4), 0x3 << 4);
	//0x00F4[5] =1'h0
	regmap_update_bits(ss->regmap_phy[id], 0x00F4, BIT(5), 0x0);
	//0x3040[29:28] = 2'h3
	regmap_update_bits(ss->regmap_phy[id], 0x3040, GENMASK(29, 28), 0x3 << 28);
}

int mtk_usxgmii_setup_mode_an(struct mtk_sgmii *ss, int id, int max_speed)
{
	unsigned int val = 0;

	dev_err(g_eth->dev,"%s id: %d \n", __func__, id);
	if (id < 0 || !ss->regmap[id] || !ss->regmap_uxphy[id]){
		dev_err(g_eth->dev,"%x id: %d regmap_uxphy fail\n",
			__func__, id);
		return -EINVAL;
	}

	if ((max_speed != SPEED_10000) && (max_speed != SPEED_5000)) {
		dev_err(g_eth->dev,"%s [%d]: serdes speed %d wrong\n",
			__func__, id, max_speed);
		return -EINVAL;
	}

	if (MTK_HAS_CAPS(g_eth->soc->caps, MTK_NETSYS2)) {
		regmap_read(ss->remap_pll, MT6990_XFI_PLL_OFFSET, &val);
		val |= MT6990_XFI_PLL_EN;
		regmap_write(ss->remap_pll, MT6990_XFI_PLL_OFFSET, val);
	}

	sgmii_runtime_control(id, false);
	sgmii_runtime_control(id, true);

	regmap_update_bits(ss->regmap_uxphy[id],
			MT6890_RG_PCS_AN_CTRL_0, BIT(0), 0x1);

	regmap_update_bits(ss->regmap_uxphy[id],
			0x810,GENMASK(30, 11), 0x1FF << 11);
	regmap_update_bits(ss->regmap_uxphy[id],
			0x818,GENMASK(9, 0), 0x7B);
	regmap_update_bits(ss->regmap_uxphy[id],
			0x818,GENMASK(19, 10), 0x7B << 10);
	regmap_update_bits(ss->regmap_uxphy[id],
			0x818,GENMASK(29, 20), 0x7B << 20);

	regmap_update_bits(ss->regmap_uxphy[id],
		MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(31), 0x0);
	regmap_update_bits(ss->regmap_uxphy[id],
		MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(29), 0x1);
	regmap_update_bits(ss->regmap_uxphy[id],
		MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(28), 0x0);
	regmap_update_bits(ss->regmap_uxphy[id],
		MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(10, 8), 0x0);

	if (max_speed == SPEED_10000) {
		regmap_update_bits(ss->regmap_uxphy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(6, 4), 0x0);
		regmap_update_bits(ss->regmap_uxphy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(2, 0), 0x0);
	} else 	if (max_speed == SPEED_5000) {
		regmap_update_bits(ss->regmap_uxphy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(6, 4), 0x1 << 4);
		regmap_update_bits(ss->regmap_uxphy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(2, 0), 0x1);

	} else {
		dev_err(g_eth->dev,"%x [%d]: serdes speed %d wrong\n",
			__func__, id, max_speed);
		return -EINVAL;
	}

	udelay(10);
	regmap_update_bits(ss->regmap_uxphy[id],
		MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(29), 0x0);

	if (max_speed == SPEED_10000) {
		mtk_usxgmii_setup_phya_an_10000(ss, id);
	} else {
		mtk_usxgmii_setup_phya_an_5000(ss, id);
	}
	return 0;
}


int mtk_usxgmii_setup_mode_force(struct mtk_sgmii *ss, int id, int max_speed)
{
	unsigned int val = 0;

	if (id < 0 || !ss->regmap[id] || !ss->regmap_uxphy[id]){
		dev_err(g_eth->dev,"%x id: %d regmap_uxphy fail\n",
			__func__, id);
		return -EINVAL;
	}

	dev_err(g_eth->dev,"%x [%d]: serdes speed %d\n",
		__func__, id, max_speed);

	if ((max_speed != SPEED_10000) && (max_speed != SPEED_5000)) {
		dev_err(g_eth->dev,"%x [%d]: serdes speed %d wrong\n",
			__func__, id, max_speed);
		return -EINVAL;
	}

	regmap_update_bits(ss->regmap_phy[id], MT6890_RG_PCS_AN_CTRL_0, BIT(0), BIT(0));
	regmap_read(ss->regmap_uxphy[id], MT6890_RG_PCS_AN_CTRL_0, &val);

	regmap_update_bits(ss->regmap_phy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(31), BIT(31));
	regmap_update_bits(ss->regmap_phy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(29), BIT(29));
	regmap_update_bits(ss->regmap_phy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(28), BIT(28));
	regmap_update_bits(ss->regmap_phy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(10, 8), 0x0);

	if (max_speed == SPEED_10000) {
		regmap_update_bits(ss->regmap_phy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(6, 4), 0x0 << 4 );
		regmap_update_bits(ss->regmap_phy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(2, 0), 0x0);
	} else if (max_speed == SPEED_5000) {
		regmap_update_bits(ss->regmap_phy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(6, 4), 0x1 << 4);
		regmap_update_bits(ss->regmap_phy[id],
			MT6890_RG_PCS_TOP_SPEED_CTRL_1, GENMASK(2, 0), 0x1);
	} else {
		dev_err(g_eth->dev,"%x [%d]: serdes speed %d wrong\n",
			__func__, id, max_speed);
		return -EINVAL;
	}

	regmap_read(ss->regmap_uxphy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, &val);
	dev_err(g_eth->dev,"%x [%d]MT6890_RG_PCS_TOP_SPEED_CTRL_1 %x\n",
		__func__, id, val);

	udelay(100);
	regmap_update_bits(ss->regmap_phy[id],
		MT6890_RG_PCS_TOP_SPEED_CTRL_1, BIT(29), 0x0);

	regmap_read(ss->regmap_uxphy[id], MT6890_RG_PCS_TOP_SPEED_CTRL_1, &val);
	dev_err(g_eth->dev,"%x [%d]MT6890_RG_PCS_TOP_SPEED_CTRL_1 %x\n",
		__func__, id, val);

	//debug info
	dump_usxgmii(ss, id);

	return 0;

}
