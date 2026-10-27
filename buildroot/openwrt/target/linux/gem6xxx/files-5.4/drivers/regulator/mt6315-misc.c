// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/pmif.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/mt6315-misc.h>
#include <linux/regulator/mt6315-regulator.h>

#define LP_INIT_SETTING_VERIFIED 1

#define MT6315_DECL_CHIP(_mid, _saddr)\
{				\
	.master_idx = _mid,	\
	.slave_addr = _saddr,	\
	.regmap = NULL,		\
}

static struct mt6315_misc mt6315_misc[] = {
	MT6315_DECL_CHIP(SPMI_MASTER_1, MT6315_SLAVE_ID_5),
};

static struct mt6315_misc *mt6315_find_chip_sid(u32 sid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt6315_misc); i++) {
		if (mt6315_misc[i].slave_addr == sid)
			return &mt6315_misc[i];
	}
	return NULL;
}


#if LP_INIT_SETTING_VERIFIED
static void mt6315_vbuck1_lp_setting(struct regmap *regmap,
		enum MT6315_BUCK_EN_USER user, unsigned char mode,
		unsigned char en, unsigned char cfg)
{
	if (user == MT6315_SRCLKEN0) {
		regmap_update_bits(regmap,
			MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_MODE_ADDR,
			0x1 << MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_MODE_SHIFT,
			mode << MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_MODE_SHIFT);
		regmap_update_bits(regmap,
			MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_EN_ADDR,
			0x1 << MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_EN_SHIFT,
			en << MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_EN_SHIFT);
		regmap_update_bits(regmap,
			MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_CFG_ADDR,
			0x1 << MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_CFG_SHIFT,
			cfg << MT6315_PMIC_RG_BUCK_VBUCK1_HW0_OP_CFG_SHIFT);
	} else {
		pr_info("%s non support user control(%d).\n", __func__, user);
	}
}

static void mt6315_vbuck3_lp_setting(struct regmap *regmap,
		enum MT6315_BUCK_EN_USER user, unsigned char mode,
		unsigned char en, unsigned char cfg)
{
	if (user == MT6315_SRCLKEN0) {
		regmap_update_bits(regmap,
			MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_MODE_ADDR,
			0x1 << MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_MODE_SHIFT,
			mode << MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_MODE_SHIFT);
		regmap_update_bits(regmap,
			MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_EN_ADDR,
			0x1 << MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_EN_SHIFT,
			en << MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_EN_SHIFT);
		regmap_update_bits(regmap,
			MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_CFG_ADDR,
			0x1 << MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_CFG_SHIFT,
			cfg << MT6315_PMIC_RG_BUCK_VBUCK3_HW0_OP_CFG_SHIFT);
	} else {
		pr_info("%s non support user control(%d).\n", __func__, user);
	}
}


static void mt6315_lp_set(unsigned char slave_id, unsigned char buck_id,
		   enum MT6315_BUCK_EN_USER user, unsigned char op_mode,
		   unsigned char op_en, unsigned char op_cfg)
{
	struct mt6315_misc *mt6315;
	struct regmap *regmap;

	mt6315 = mt6315_find_chip_sid(slave_id);
	if (!mt6315) {
		pr_info("%s MT6315S%d not ready\n", __func__, slave_id);
		return;
	}

	regmap = mt6315->regmap;
	if (!regmap) {
		pr_info("%s null regmap.\n", __func__);
		return;
	}

	if (buck_id == 1)
		mt6315_vbuck1_lp_setting(regmap, user, op_mode, op_en, op_cfg);
	else if (buck_id == 3)
		mt6315_vbuck3_lp_setting(regmap, user, op_mode, op_en, op_cfg);
	else
		pr_info("%s invalid buck_id=%d.\n", __func__, buck_id);
}

/* enable VDIG18 SRCLKEN low power mode */
static void mt6315_vdig18_hw_op_set(unsigned char slave_id, unsigned char en)
{
	struct mt6315_misc *mt6315;
	struct regmap *regmap;

	mt6315 = mt6315_find_chip_sid(slave_id);
	if (!mt6315) {
		pr_info("%s MT6315S%d not ready\n", __func__, slave_id);
		return;
	}

	regmap = mt6315->regmap;
	if (!regmap) {
		pr_info("%s null regmap.\n", __func__);
		return;
	}

	regmap_write(regmap, MT6315_PMIC_DIG_WPK_KEY_H_ADDR, 0x63);
	regmap_write(regmap, MT6315_PMIC_DIG_WPK_KEY_ADDR, 0x15);
	regmap_update_bits(regmap,
		   MT6315_PMIC_RG_LDO_VDIG18_HW_OP_EN_ADDR,
		   0x1 << MT6315_PMIC_RG_LDO_VDIG18_HW_OP_EN_SHIFT,
		   en << MT6315_PMIC_RG_LDO_VDIG18_HW_OP_EN_SHIFT);
	regmap_write(regmap, MT6315_PMIC_DIG_WPK_KEY_ADDR, 0x0);
	regmap_write(regmap, MT6315_PMIC_DIG_WPK_KEY_H_ADDR, 0x0);
}
#endif /* End of LP_INIT_SETTING_VERIFIED */

static void mt6315_S5_lp_initial_setting(void)
{
#if LP_INIT_SETTING_VERIFIED
	mt6315_vdig18_hw_op_set(MT6315_SLAVE_ID_5, 1);
	/* vproc/vmdd2 */
	mt6315_lp_set(MT6315_SLAVE_ID_5, 1, MT6315_SRCLKEN0, 1, 1, HW_LP);
	mt6315_lp_set(MT6315_SLAVE_ID_5, 3, MT6315_SRCLKEN0, 1, 1, HW_LP);
#endif
}

static void mt6315_misc_initial_setting(u32 sid)
{
	switch (sid) {
	case MT6315_SLAVE_ID_5:
		mt6315_S5_lp_initial_setting();
		break;

	default:
		pr_info("unsupported chip sid: %d\n", sid);
		return;
	}
}

void mt6315_misc_init(u32 sid, struct regmap *regmap)
{
	struct mt6315_misc *mt6315;

	mt6315 = mt6315_find_chip_sid(sid);
	if (mt6315)
		mt6315->regmap = regmap;

	mt6315_misc_initial_setting(sid);

	pr_info("%s sid=%d done\n", __func__, sid);
}
