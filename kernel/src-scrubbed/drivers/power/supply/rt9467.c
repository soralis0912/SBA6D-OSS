// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pm_runtime.h>
#include <linux/power_supply.h>

#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#ifdef CONFIG_REGMAP
#include <linux/regmap.h>
#endif

#include "rt9467.h"
#define I2C_ACCESS_MAX_RETRY	5
#define RT9467_DRV_VERSION	"1.0.19_MTK"

/* ======================= */
/* RT9467 Parameter        */
/* ======================= */


static const u32 rt9467_boost_oc_threshold[] = {
	500000, 700000, 1100000, 1300000, 1800000, 2100000, 2400000,
}; /* uA */

enum rt9467_irq_idx {
	RT9467_IRQIDX_CHG_STATC = 0,
	RT9467_IRQIDX_CHG_FAULT,
	RT9467_IRQIDX_TS_STATC,
	RT9467_IRQIDX_CHG_IRQ1,
	RT9467_IRQIDX_CHG_IRQ2,
	RT9467_IRQIDX_CHG_IRQ3,
	RT9467_IRQIDX_DPDM_IRQ,
	RT9467_IRQIDX_MAX,
};

enum rt9467_irq_stat {
	RT9467_IRQSTAT_CHG_STATC = 0,
	RT9467_IRQSTAT_CHG_FAULT,
	RT9467_IRQSTAT_TS_STATC,
	RT9467_IRQSTAT_MAX,
};

enum rt9467_chg_type {
	RT9467_CHG_TYPE_NOVBUS = 0,
	RT9467_CHG_TYPE_UNDER_GOING,
	RT9467_CHG_TYPE_SDP,
	RT9467_CHG_TYPE_SDPNSTD,
	RT9467_CHG_TYPE_DCP,
	RT9467_CHG_TYPE_CDP,
	RT9467_CHG_TYPE_MAX,
};

static const u8 rt9467_irq_maskall[RT9467_IRQIDX_MAX] = {
	0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

struct irq_mapping_tbl {
	const char *name;
	const int id;
};

#define RT9467_IRQ_MAPPING(_name, _id) {.name = #_name, .id = _id}
static const struct irq_mapping_tbl rt9467_irq_mapping_tbl[] = {
	RT9467_IRQ_MAPPING(chg_treg, 4),
	RT9467_IRQ_MAPPING(chg_aicr, 5),
	RT9467_IRQ_MAPPING(chg_mivr, 6),
	RT9467_IRQ_MAPPING(pwr_rdy, 7),
	RT9467_IRQ_MAPPING(chg_vsysuv, 12),
	RT9467_IRQ_MAPPING(chg_vsysov, 13),
	RT9467_IRQ_MAPPING(chg_vbatov, 14),
	RT9467_IRQ_MAPPING(chg_vbusov, 15),
	RT9467_IRQ_MAPPING(ts_batcold, 20),
	RT9467_IRQ_MAPPING(ts_batcool, 21),
	RT9467_IRQ_MAPPING(ts_batwarm, 22),
	RT9467_IRQ_MAPPING(ts_bathot, 23),
	RT9467_IRQ_MAPPING(ts_statci, 24),
	RT9467_IRQ_MAPPING(chg_faulti, 25),
	RT9467_IRQ_MAPPING(chg_statci, 26),
	RT9467_IRQ_MAPPING(chg_tmri, 27),
	RT9467_IRQ_MAPPING(chg_batabsi, 28),
	RT9467_IRQ_MAPPING(chg_adpbadi, 29),
	RT9467_IRQ_MAPPING(chg_rvpi, 30),
	RT9467_IRQ_MAPPING(otpi, 31),
	RT9467_IRQ_MAPPING(chg_aiclmeasi, 32),
	RT9467_IRQ_MAPPING(chg_ichgmeasi, 33),
	RT9467_IRQ_MAPPING(chgdet_donei, 34),
	RT9467_IRQ_MAPPING(wdtmri, 35),
	RT9467_IRQ_MAPPING(ssfinishi, 36),
	RT9467_IRQ_MAPPING(chg_rechgi, 37),
	RT9467_IRQ_MAPPING(chg_termi, 38),
	RT9467_IRQ_MAPPING(chg_ieoci, 39),
	RT9467_IRQ_MAPPING(adc_donei, 40),
	RT9467_IRQ_MAPPING(pumpx_donei, 41),
	RT9467_IRQ_MAPPING(bst_batuvi, 45),
	RT9467_IRQ_MAPPING(bst_midovi, 46),
	RT9467_IRQ_MAPPING(bst_olpi, 47),
	RT9467_IRQ_MAPPING(attachi, 48),
	RT9467_IRQ_MAPPING(detachi, 49),
	RT9467_IRQ_MAPPING(chgdeti, 54),
	RT9467_IRQ_MAPPING(dcdti, 55),
};

enum rt9467_charging_status {
	RT9467_CHG_STATUS_READY = 0,
	RT9467_CHG_STATUS_PROGRESS,
	RT9467_CHG_STATUS_DONE,
	RT9467_CHG_STATUS_FAULT,
	RT9467_CHG_STATUS_MAX,
};

static const u8 rt9467_val_en_hidden_mode[] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
};

enum rt9467_adc_sel {
	RT9467_ADC_VBUS_DIV5 = 1,
	RT9467_ADC_VBUS_DIV2,
	RT9467_ADC_VSYS,
	RT9467_ADC_VBAT,
	RT9467_ADC_TS_BAT = 6,
	RT9467_ADC_IBUS = 8,
	RT9467_ADC_IBAT,
	RT9467_ADC_REGN = 11,
	RT9467_ADC_TEMP_JC,
	RT9467_ADC_MAX,
};

/*
 * Unit for each ADC parameter
 * 0 stands for reserved
 * For TS_BAT, the real unit is 0.25.
 * Here we use 25, please remember to divide 100 while showing the value
 */
static const int rt9467_adc_unit[RT9467_ADC_MAX] = {
	0,
	RT9467_ADC_UNIT_VBUS_DIV5,
	RT9467_ADC_UNIT_VBUS_DIV2,
	RT9467_ADC_UNIT_VSYS,
	RT9467_ADC_UNIT_VBAT,
	0,
	RT9467_ADC_UNIT_TS_BAT,
	0,
	RT9467_ADC_UNIT_IBUS,
	RT9467_ADC_UNIT_IBAT,
	0,
	RT9467_ADC_UNIT_REGN,
	RT9467_ADC_UNIT_TEMP_JC,
};

static const int rt9467_adc_offset[RT9467_ADC_MAX] = {
	0,
	RT9467_ADC_OFFSET_VBUS_DIV5,
	RT9467_ADC_OFFSET_VBUS_DIV2,
	RT9467_ADC_OFFSET_VSYS,
	RT9467_ADC_OFFSET_VBAT,
	0,
	RT9467_ADC_OFFSET_TS_BAT,
	0,
	RT9467_ADC_OFFSET_IBUS,
	RT9467_ADC_OFFSET_IBAT,
	0,
	RT9467_ADC_OFFSET_REGN,
	RT9467_ADC_OFFSET_TEMP_JC,
};

struct rt9467_desc {
	bool en_wdt;
	bool en_irq_pulse;
	const char *chg_dev_name;
};

/* These default values will be applied if there's no property in dts */
static struct rt9467_desc rt9467_default_desc = {
	.en_wdt = true,
	.en_irq_pulse = false,
	.chg_dev_name = "primary_chg",
};

struct rt9467_info {
	struct i2c_client *client;
	struct mutex i2c_access_lock;
	struct mutex adc_access_lock;
	struct mutex irq_access_lock;
	struct mutex aicr_access_lock;
	struct mutex ichg_access_lock;
	struct mutex hidden_mode_lock;
	struct device *dev;
	struct rt9467_desc *desc;
	int irq;
	u32 intr_gpio;
	u8 chip_rev;
	u8 irq_flag[RT9467_IRQIDX_MAX];
	u8 irq_stat[RT9467_IRQSTAT_MAX];
	u8 irq_mask[RT9467_IRQIDX_MAX];
	u32 hidden_mode_cnt;
	struct regulator_dev *otg_rdev;
};

/* ======================= */
/* Register Address        */
/* ======================= */

static const unsigned char rt9467_reg_addr[] = {
	RT9467_REG_CORE_CTRL0,
	RT9467_REG_CHG_CTRL1,
	RT9467_REG_CHG_CTRL2,
	RT9467_REG_CHG_CTRL3,
	RT9467_REG_CHG_CTRL4,
	RT9467_REG_CHG_CTRL5,
	RT9467_REG_CHG_CTRL6,
	RT9467_REG_CHG_CTRL7,
	RT9467_REG_CHG_CTRL8,
	RT9467_REG_CHG_CTRL9,
	RT9467_REG_CHG_CTRL10,
	RT9467_REG_CHG_CTRL11,
	RT9467_REG_CHG_CTRL12,
	RT9467_REG_CHG_CTRL13,
	RT9467_REG_CHG_CTRL14,
	RT9467_REG_CHG_CTRL15,
	RT9467_REG_CHG_CTRL16,
	RT9467_REG_CHG_ADC,
	RT9467_REG_CHG_DPDM1,
	RT9467_REG_CHG_DPDM2,
	RT9467_REG_CHG_DPDM3,
	RT9467_REG_CHG_CTRL19,
	RT9467_REG_CHG_CTRL17,
	RT9467_REG_CHG_CTRL18,
	RT9467_REG_DEVICE_ID,
	RT9467_REG_CHG_STAT,
	RT9467_REG_CHG_NTC,
	RT9467_REG_ADC_DATA_H,
	RT9467_REG_ADC_DATA_L,
	RT9467_REG_ADC_DATA_TUNE_H,
	RT9467_REG_ADC_DATA_TUNE_L,
	RT9467_REG_ADC_DATA_ORG_H,
	RT9467_REG_ADC_DATA_ORG_L,
	RT9467_REG_CHG_STATC,
	RT9467_REG_CHG_FAULT,
	RT9467_REG_TS_STATC,
	/* Skip IRQ evt to prevent reading clear while dumping registers */
	RT9467_REG_CHG_STATC_CTRL,
	RT9467_REG_CHG_FAULT_CTRL,
	RT9467_REG_TS_STATC_CTRL,
	RT9467_REG_CHG_IRQ1_CTRL,
	RT9467_REG_CHG_IRQ2_CTRL,
	RT9467_REG_CHG_IRQ3_CTRL,
	RT9467_REG_DPDM_IRQ_CTRL,
};


/* ========================= */
/* I2C operations            */
/* ========================= */

#ifdef CONFIG_REGMAP
static const struct regmap_config rt9467_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = RT9467_REG_MAX,
	.cache_type = REGCACHE_NONE,
};
#endif /* CONFIG_REGMAP */

static int rt9467_device_read(void *client, u32 addr, int leng, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);
}

static int rt9467_device_write(void *client, u32 addr, int leng,
	const void *src)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);
}

static inline int __rt9467_i2c_write_byte(struct rt9467_info *info, u8 cmd,
	u8 data)
{
	int ret = 0, retry = 0;

	do {
		ret = rt9467_device_write(info->client, cmd, 1, &data);
		retry++;
		if (ret < 0)
			udelay(10);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0)
		dev_notice(info->dev, "%s: I2CW[0x%02X] = 0x%02X fail\n",
			__func__, cmd, data);
	else
		dev_dbg(info->dev, "%s: I2CW[0x%02X] = 0x%02X\n", __func__,
			cmd, data);

	return ret;
}

static int rt9467_i2c_write_byte(struct rt9467_info *info, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = __rt9467_i2c_write_byte(info, cmd, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int __rt9467_i2c_read_byte(struct rt9467_info *info, u8 cmd)
{
	int ret = 0, ret_val = 0, retry = 0;

	do {
		ret = rt9467_device_read(info->client, cmd, 1, &ret_val);
		retry++;
		if (ret < 0)
			udelay(10);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0) {
		dev_notice(info->dev, "%s: I2CR[0x%02X] fail\n", __func__, cmd);
		return ret;
	}

	ret_val = ret_val & 0xFF;

	dev_dbg(info->dev, "%s: I2CR[0x%02X] = 0x%02X\n", __func__, cmd,
		ret_val);

	return ret_val;
}

static int rt9467_i2c_read_byte(struct rt9467_info *info, u8 cmd)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = __rt9467_i2c_read_byte(info, cmd);
	mutex_unlock(&info->i2c_access_lock);

	if (ret < 0)
		return ret;

	return (ret & 0xFF);
}

static inline int __rt9467_i2c_block_write(struct rt9467_info *info, u8 cmd,
	u32 leng, const u8 *data)
{
	int ret = 0;

	ret = rt9467_device_write(info->client, cmd, leng, data);

	return ret;
}


static int rt9467_i2c_block_write(struct rt9467_info *info, u8 cmd, u32 leng,
	const u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = __rt9467_i2c_block_write(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int __rt9467_i2c_block_read(struct rt9467_info *info, u8 cmd,
	u32 leng, u8 *data)
{
	int ret = 0;

	ret = rt9467_device_read(info->client, cmd, leng, data);

	return ret;
}


static int rt9467_i2c_block_read(struct rt9467_info *info, u8 cmd, u32 leng,
	u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = __rt9467_i2c_block_read(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}


static int rt9467_i2c_test_bit(struct rt9467_info *info, u8 cmd, u8 shift,
	bool *is_one)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9467_i2c_read_byte(info, cmd);
	if (ret < 0) {
		*is_one = false;
		return ret;
	}

	data = ret & (1 << shift);
	*is_one = (data == 0 ? false : true);

	return ret;
}

static int rt9467_i2c_update_bits(struct rt9467_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = __rt9467_i2c_read_byte(info, cmd);
	if (ret < 0) {
		mutex_unlock(&info->i2c_access_lock);
		return ret;
	}

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	ret = __rt9467_i2c_write_byte(info, cmd, reg_data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int rt9467_set_bit(struct rt9467_info *info, u8 reg, u8 mask)
{
	return rt9467_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9467_clr_bit(struct rt9467_info *info, u8 reg, u8 mask)
{
	return rt9467_i2c_update_bits(info, reg, 0x00, mask);
}

/* ================== */
/* Internal Functions */
/* ================== */
static int rt9467_kick_wdt(struct rt9467_info *info);
static int rt9467_enable_hidden_mode(struct rt9467_info *info, bool en);

static inline void rt9467_irq_set_flag(struct rt9467_info *info, u8 *irq,
	u8 mask)
{
	mutex_lock(&info->irq_access_lock);
	*irq |= mask;
	mutex_unlock(&info->irq_access_lock);
}

static inline void rt9467_irq_clr_flag(struct rt9467_info *info, u8 *irq,
	u8 mask)
{
	mutex_lock(&info->irq_access_lock);
	*irq &= ~mask;
	mutex_unlock(&info->irq_access_lock);
}

static inline const char *rt9467_get_irq_name(struct rt9467_info *info,
	int irqnum)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(rt9467_irq_mapping_tbl); i++) {
		if (rt9467_irq_mapping_tbl[i].id == irqnum)
			return rt9467_irq_mapping_tbl[i].name;
	}

	return "not found";
}

static inline void rt9467_irq_mask(struct rt9467_info *info, int irqnum)
{
	if (irqnum < 0)
		return;
	dev_dbg(info->dev, "%s: irq = %d, %s\n", __func__, irqnum,
		rt9467_get_irq_name(info, irqnum));
	info->irq_mask[irqnum / 8] |= (1 << (irqnum % 8));
}

static inline void rt9467_irq_unmask(struct rt9467_info *info, int irqnum)
{
	if (irqnum < 0)
		return;
	dev_dbg(info->dev, "%s: irq = %d, %s\n", __func__, irqnum,
		rt9467_get_irq_name(info, irqnum));
	info->irq_mask[irqnum / 8] &= ~(1 << (irqnum % 8));
}

static inline u8 rt9467_closest_reg(u32 min, u32 max, u32 step, u32 target)
{
	/* Smaller than minimum supported value, use minimum one */
	if (target < min)
		return 0;

	/* Greater than maximum supported value, use maximum one */
	if (target >= max)
		return (max - min) / step;

	return (target - min) / step;
}

static inline u8 rt9467_closest_reg_via_tbl(const u32 *tbl, u32 tbl_size,
	u32 target)
{
	u32 i = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target < tbl[0])
		return 0;

	for (i = 0; i < tbl_size - 1; i++) {
		if (target >= tbl[i] && target < tbl[i + 1])
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return tbl_size - 1;
}

static inline u32 rt9467_closest_value(u32 min, u32 max, u32 step, u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static int rt9467_get_aicr(struct rt9467_info *info, u32 *aicr);
static int rt9467_get_ichg(struct rt9467_info *info, u32 *ichg);
static int rt9467_get_adc(struct rt9467_info *info,
	enum rt9467_adc_sel adc_sel, int *adc_val)
{
	int ret = 0, i = 0;
	const int max_wait_times = 6;
	u8 adc_data[6] = {0};
	u32 aicr = 0, ichg = 0;
	bool adc_start = false;

	mutex_lock(&info->adc_access_lock);

	rt9467_enable_hidden_mode(info, true);

	/* Select ADC to desired channel */
	ret = rt9467_i2c_update_bits(info, RT9467_REG_CHG_ADC,
		adc_sel << RT9467_SHIFT_ADC_IN_SEL, RT9467_MASK_ADC_IN_SEL);
	if (ret < 0) {
		dev_notice(info->dev, "%s: select ch to %d fail(%d)\n",
			__func__, adc_sel, ret);
		goto out;
	}

	/* Workaround for IBUS & IBAT */
	if (adc_sel == RT9467_ADC_IBUS) {
		mutex_lock(&info->aicr_access_lock);
		ret = rt9467_get_aicr(info, &aicr);
		if (ret < 0) {
			dev_notice(info->dev, "%s: get aicr fail\n", __func__);
			goto out_unlock_all;
		}
	} else if (adc_sel == RT9467_ADC_IBAT) {
		mutex_lock(&info->ichg_access_lock);
		ret = rt9467_get_ichg(info, &ichg);
		if (ret < 0) {
			dev_notice(info->dev, "%s: get ichg fail\n", __func__);
			goto out_unlock_all;
		}
	}

	/* Start ADC conversation */
	ret = rt9467_set_bit(info, RT9467_REG_CHG_ADC, RT9467_MASK_ADC_START);
	if (ret < 0) {
		dev_notice(info->dev, "%s: start con fail(%d), sel = %d\n",
			__func__, ret, adc_sel);
		goto out_unlock_all;
	}

	for (i = 0; i < max_wait_times; i++) {
		msleep(35);
		ret = rt9467_i2c_test_bit(info, RT9467_REG_CHG_ADC,
			RT9467_SHIFT_ADC_START, &adc_start);
		if (ret >= 0 && !adc_start)
			break;
	}
	if (i == max_wait_times) {
		dev_notice(info->dev, "%s: wait con fail(%d), sel = %d\n",
			__func__, ret, adc_sel);
		ret = -EINVAL;
		goto out_unlock_all;
	}

	mdelay(1);

	/* Read ADC data high/low byte */
	ret = rt9467_i2c_block_read(info, RT9467_REG_ADC_DATA_H, 6, adc_data);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read ADC data fail\n", __func__);
		goto out_unlock_all;
	}
	dev_dbg(info->dev,
		"%s: adc_tune = (0x%02X, 0x%02X), adc_org = (0x%02X, 0x%02X)\n",
		__func__, adc_data[2], adc_data[3], adc_data[4], adc_data[5]);

	/* Calculate ADC value */
	*adc_val = ((adc_data[0] << 8) + adc_data[1]) * rt9467_adc_unit[adc_sel]
		+ rt9467_adc_offset[adc_sel];

	dev_dbg(info->dev,
		"%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X, val = %d\n",
		__func__, adc_sel, adc_data[0], adc_data[1], *adc_val);

	ret = 0;

out_unlock_all:
	/* Coefficient of IBUS & IBAT */
	if (adc_sel == RT9467_ADC_IBUS) {
		if (aicr < 400000) /* 400mA */
			*adc_val = *adc_val * 67 / 100;
		mutex_unlock(&info->aicr_access_lock);
	} else if (adc_sel == RT9467_ADC_IBAT) {
		if (ichg >= 100000 && ichg <= 450000) /* 100~450mA */
			*adc_val = *adc_val * 57 / 100;
		else if (ichg >= 500000 && ichg <= 850000) /* 500~850mA */
			*adc_val = *adc_val * 63 / 100;
		mutex_unlock(&info->ichg_access_lock);
	}

out:
	rt9467_enable_hidden_mode(info, false);
	mutex_unlock(&info->adc_access_lock);
	return ret;
}

static inline int __rt9467_enable_chgdet_flow(struct rt9467_info *info, bool en)
{
	int ret = 0;

	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	ret = (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_DPDM1, RT9467_MASK_USBCHGEN);
	return ret;
}

/* Prevent back boost */
static int rt9467_toggle_cfo(struct rt9467_info *info)
{
	int ret = 0;
	u8 data = 0;

	dev_info(info->dev, "%s\n", __func__);
	mutex_lock(&info->i2c_access_lock);
	ret = rt9467_device_read(info->client, RT9467_REG_CHG_CTRL2, 1, &data);
	if (ret < 0) {
		dev_notice(info->dev, "%s read cfo fail(%d)\n", __func__, ret);
		goto out;
	}

	/* CFO off */
	data &= ~RT9467_MASK_CFO_EN;
	ret = rt9467_device_write(info->client, RT9467_REG_CHG_CTRL2, 1, &data);
	if (ret < 0) {
		dev_notice(info->dev, "%s cfo off fail(%d)\n", __func__, ret);
		goto out;
	}

	/* CFO on */
	data |= RT9467_MASK_CFO_EN;
	ret = rt9467_device_write(info->client, RT9467_REG_CHG_CTRL2, 1, &data);
	if (ret < 0)
		dev_notice(info->dev, "%s cfo on fail(%d)\n", __func__, ret);

out:
	mutex_unlock(&info->i2c_access_lock);
	return ret;
}

/* IRQ handlers */
static int rt9467_pwr_rdy_irq_handler(struct rt9467_info *info)
{
#ifndef CONFIG_TCPC_CLASS
	int ret = 0;
	bool pwr_rdy = false;
#endif /* CONFIG_TCPC_CLASS */

	dev_notice(info->dev, "%s\n", __func__);

#ifndef CONFIG_TCPC_CLASS
	ret = rt9467_i2c_test_bit(info, RT9467_REG_CHG_STATC,
		RT9467_SHIFT_PWR_RDY, &pwr_rdy);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read pwr rdy fail\n", __func__);
		goto out;
	}

	if (!pwr_rdy) {
		dev_info(info->dev, "%s: pwr rdy = 0\n", __func__);
		goto out;
	}

out:
#endif /* CONFIG_TCPC_CLASS */

	return 0;
}

static int rt9467_chg_mivr_irq_handler(struct rt9467_info *info)
{
	int ret = 0;
	bool mivr_act = false;
	int adc_ibus = 0;

	dev_notice(info->dev, "%s\n", __func__);

	/* Check whether MIVR loop is active */
	ret = rt9467_i2c_test_bit(info, RT9467_REG_CHG_STATC,
		RT9467_SHIFT_CHG_MIVR, &mivr_act);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read mivr stat fail\n", __func__);
		goto out;
	}

	if (!mivr_act) {
		dev_info(info->dev, "%s: mivr loop is not active\n", __func__);
		goto out;
	}

	if (strcmp(info->desc->chg_dev_name, "primary_chg") == 0) {
		/* Check IBUS ADC */
		ret = rt9467_get_adc(info, RT9467_ADC_IBUS, &adc_ibus);
		if (ret < 0) {
			dev_notice(info->dev, "%s: get ibus fail\n", __func__);
			return ret;
		}
		if (adc_ibus < 100000) { /* 100mA */
			ret = rt9467_toggle_cfo(info);
			return ret;
		}
	}
out:
	return 0;
}

static int rt9467_chg_aicr_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_treg_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_vsysuv_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_vsysov_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_vbatov_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_vbusov_irq_handler(struct rt9467_info *info)
{
	int ret = 0;
	bool vbusov = false;

	dev_notice(info->dev, "%s\n", __func__);
	ret = rt9467_i2c_test_bit(info, RT9467_REG_CHG_FAULT,
		RT9467_SHIFT_VBUSOV, &vbusov);
	if (ret < 0)
		return ret;

	dev_info(info->dev, "%s: vbusov = %d\n", __func__, vbusov);

	return 0;
}

static int rt9467_ts_bat_cold_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_ts_bat_cool_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_ts_bat_warm_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_ts_bat_hot_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_ts_statci_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_faulti_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_statci_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_tmri_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_batabsi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_adpbadi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_rvpi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_otpi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_aiclmeasi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_ichgmeasi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chgdet_donei_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_wdtmri_irq_handler(struct rt9467_info *info)
{
	int ret = 0;

	dev_notice(info->dev, "%s\n", __func__);
	ret = rt9467_kick_wdt(info);
	if (ret < 0)
		dev_notice(info->dev, "%s: kick wdt fail\n", __func__);

	return ret;
}

static int rt9467_ssfinishi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_rechgi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_termi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_chg_ieoci_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_adc_donei_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_pumpx_donei_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_bst_batuvi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_bst_midovi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_bst_olpi_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_attachi_irq_handler(struct rt9467_info *info)
{
	int ret = 0;

	dev_notice(info->dev, "%s\n", __func__);
	return ret;
}

static int rt9467_detachi_irq_handler(struct rt9467_info *info)
{
	int ret = 0;

	dev_notice(info->dev, "%s\n", __func__);
	return ret;
}

static int rt9467_chgdeti_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9467_dcdti_irq_handler(struct rt9467_info *info)
{
	dev_notice(info->dev, "%s\n", __func__);
	return 0;
}

typedef int (*rt9467_irq_fptr)(struct rt9467_info *);
static rt9467_irq_fptr rt9467_irq_handler_tbl[56] = {
	NULL,
	NULL,
	NULL,
	NULL,
	rt9467_chg_treg_irq_handler,
	rt9467_chg_aicr_irq_handler,
	rt9467_chg_mivr_irq_handler,
	rt9467_pwr_rdy_irq_handler,
	NULL,
	NULL,
	NULL,
	NULL,
	rt9467_chg_vsysuv_irq_handler,
	rt9467_chg_vsysov_irq_handler,
	rt9467_chg_vbatov_irq_handler,
	rt9467_chg_vbusov_irq_handler,
	NULL,
	NULL,
	NULL,
	NULL,
	rt9467_ts_bat_cold_irq_handler,
	rt9467_ts_bat_cool_irq_handler,
	rt9467_ts_bat_warm_irq_handler,
	rt9467_ts_bat_hot_irq_handler,
	rt9467_ts_statci_irq_handler,
	rt9467_chg_faulti_irq_handler,
	rt9467_chg_statci_irq_handler,
	rt9467_chg_tmri_irq_handler,
	rt9467_chg_batabsi_irq_handler,
	rt9467_chg_adpbadi_irq_handler,
	rt9467_chg_rvpi_irq_handler,
	rt9467_chg_otpi_irq_handler,
	rt9467_chg_aiclmeasi_irq_handler,
	rt9467_chg_ichgmeasi_irq_handler,
	rt9467_chgdet_donei_irq_handler,
	rt9467_wdtmri_irq_handler,
	rt9467_ssfinishi_irq_handler,
	rt9467_chg_rechgi_irq_handler,
	rt9467_chg_termi_irq_handler,
	rt9467_chg_ieoci_irq_handler,
	rt9467_adc_donei_irq_handler,
	rt9467_pumpx_donei_irq_handler,
	NULL,
	NULL,
	NULL,
	rt9467_bst_batuvi_irq_handler,
	rt9467_bst_midovi_irq_handler,
	rt9467_bst_olpi_irq_handler,
	rt9467_attachi_irq_handler,
	rt9467_detachi_irq_handler,
	NULL,
	NULL,
	NULL,
	rt9467_chgdeti_irq_handler,
	rt9467_dcdti_irq_handler,
};

static inline int rt9467_enable_irqrez(struct rt9467_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL13, RT9467_MASK_IRQ_REZ);
}

static int __rt9467_irq_handler(struct rt9467_info *info)
{
	int ret = 0, i = 0, j = 0,
	    evt_num = (int)RT9467_IRQIDX_MAX,
	    state_num = (int)RT9467_IRQSTAT_MAX;
	u8 evt[RT9467_IRQIDX_MAX] = {0};
	u8 mask[RT9467_IRQIDX_MAX] = {0};
	u8 stat[RT9467_IRQSTAT_MAX] = {0};
	u8 usb_status_old = 0, usb_status_new = 0;

	dev_info(info->dev, "%s\n", __func__);

	/* Read DPDM status before reading evts */
	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_DPDM2);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read type fail\n", __func__);
		goto err_read_irq;
	}
	usb_status_old = (ret & RT9467_MASK_USB_STATUS) >>
		RT9467_SHIFT_USB_STATUS;

	/* Read event and skip CHG_IRQ3 */
	ret = rt9467_i2c_block_read(info, RT9467_REG_CHG_IRQ1, 2, &evt[3]);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read evt1 fail(%d)\n", __func__,
			ret);
		goto err_read_irq;
	}

	ret = rt9467_i2c_block_read(info, RT9467_REG_DPDM_IRQ, 1, &evt[6]);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read evt2 fail(%d)\n", __func__,
			ret);
		goto err_read_irq;
	}

	ret = rt9467_i2c_block_read(info, RT9467_REG_CHG_STATC, 3, evt);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read stat fail(%d)\n", __func__,
			ret);
		goto err_read_irq;
	}

	/* Read DPDM status after reading evts */
	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_DPDM2);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read type fail\n", __func__);
		goto err_read_irq;
	}
	usb_status_new = (ret & RT9467_MASK_USB_STATUS) >>
		RT9467_SHIFT_USB_STATUS;

	/* Read mask */
	ret = rt9467_i2c_block_read(info, RT9467_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(mask), mask);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read mask fail(%d)\n", __func__,
			ret);
		goto err_read_irq;
	}

	/* Detach */
	if (usb_status_old != RT9467_CHG_TYPE_NOVBUS &&
		usb_status_new == RT9467_CHG_TYPE_NOVBUS)
		evt[RT9467_IRQIDX_DPDM_IRQ] |= 0x02;

	/* Attach */
	if (usb_status_new >= RT9467_CHG_TYPE_SDP &&
		usb_status_new <= RT9467_CHG_TYPE_CDP &&
		usb_status_old != usb_status_new)
		evt[RT9467_IRQIDX_DPDM_IRQ] |= 0x01;

	/* Store/Update stat */
	memcpy(stat, info->irq_stat, RT9467_IRQSTAT_MAX);

	for (i = 0; i < evt_num; i++) {
		evt[i] &= ~mask[i];
		if (i < state_num) {
			info->irq_stat[i] = evt[i];
			evt[i] ^= stat[i];
		}
		for (j = 0; j < 8; j++) {
			if (!(evt[i] & (1 << j)))
				continue;
			if (rt9467_irq_handler_tbl[i * 8 + j])
				rt9467_irq_handler_tbl[i * 8 + j](info);
		}
	}

err_read_irq:
	return ret;
}

static irqreturn_t rt9467_irq_handler(int irq, void *data)
{
	int ret = 0;
	struct rt9467_info *info = (struct rt9467_info *)data;

	dev_info(info->dev, "%s\n", __func__);

	ret = __rt9467_irq_handler(info);
	ret = rt9467_enable_irqrez(info, true);
	if (ret < 0)
		dev_notice(info->dev, "%s: en irqrez fail\n", __func__);

	return IRQ_HANDLED;
}

static int rt9467_irq_register(struct rt9467_info *info)
{
	int ret = 0, len = 0;
	char *name = NULL;
	int n;

	if (strcmp(info->desc->chg_dev_name, "secondary_chg") == 0)
		return 0;

	dev_info(info->dev, "%s\n", __func__);

	/* request gpio */
	len = strlen(info->desc->chg_dev_name);
	name = devm_kzalloc(info->dev, len + 10, GFP_KERNEL);
	n = snprintf(name,  len + 10, "%s_irq_gpio", info->desc->chg_dev_name);
	if (n < 0 || n >= len + 10)
		pr_err("%s:%s:%d - snprintf error(%d)!\n", __FILE__, __func__, __LINE__, n);

	ret = devm_gpio_request_one(info->dev, info->intr_gpio, GPIOF_IN, name);
	if (ret < 0) {
		dev_notice(info->dev, "%s: gpio request fail\n", __func__);
		return ret;
	}

	ret = gpio_to_irq(info->intr_gpio);
	if (ret < 0) {
		dev_notice(info->dev, "%s: irq mapping fail\n", __func__);
		return ret;
	}
	info->irq = ret;
	dev_info(info->dev, "%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	name = devm_kzalloc(info->dev, len + 5, GFP_KERNEL);
	n = snprintf(name, len + 5, "%s_irq", info->desc->chg_dev_name);
	if (n < 0 || n >= len + 5)
		pr_err("%s:%s:%d - snprintf error(%d)!\n", __FILE__, __func__, __LINE__, n);
	ret = devm_request_threaded_irq(info->dev, info->irq, NULL,
		rt9467_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, name,
		info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: request thread irq fail\n",
			__func__);
		return ret;
	}
	device_init_wakeup(info->dev, true);

	return 0;
}

static inline int rt9467_maskall_irq(struct rt9467_info *info)
{
	dev_info(info->dev, "%s\n", __func__);
	return rt9467_i2c_block_write(info, RT9467_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(rt9467_irq_maskall), rt9467_irq_maskall);
}

static inline int rt9467_irq_init(struct rt9467_info *info)
{
	dev_info(info->dev, "%s\n", __func__);
	return rt9467_i2c_block_write(info, RT9467_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(info->irq_mask), info->irq_mask);
}

static bool rt9467_is_hw_exist(struct rt9467_info *info)
{
	int ret = 0;
	u8 vendor_id = 0, chip_rev = 0;

	ret = i2c_smbus_read_byte_data(info->client, RT9467_REG_DEVICE_ID);
	if (ret < 0)
		return false;

	vendor_id = ret & 0xF0;
	chip_rev = ret & 0x0F;
	if (vendor_id != RT9467_VENDOR_ID) {
		dev_notice(info->dev, "%s: vendor id is incorrect (0x%02X)\n",
			__func__, vendor_id);
		return false;
	}

	dev_info(info->dev, "%s: 0x%02X\n", __func__, chip_rev);
	info->chip_rev = chip_rev;

	return true;
}

static inline int rt9467_enable_wdt(struct rt9467_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL13, RT9467_MASK_WDT_EN);
}

static int rt9467_enable_hidden_mode(struct rt9467_info *info, bool en)
{
	int ret = 0;

	mutex_lock(&info->hidden_mode_lock);

	if (en) {
		if (info->hidden_mode_cnt == 0) {
			ret = rt9467_i2c_block_write(info, 0x70,
				ARRAY_SIZE(rt9467_val_en_hidden_mode),
				rt9467_val_en_hidden_mode);
			if (ret < 0)
				goto err;
		}
		info->hidden_mode_cnt++;
	} else {
		if (info->hidden_mode_cnt == 1) /* last one */
			ret = rt9467_i2c_write_byte(info, 0x70, 0x00);
		info->hidden_mode_cnt--;
		if (ret < 0)
			goto err;
	}
	dev_dbg(info->dev, "%s: en = %d\n", __func__, en);
	goto out;

err:
	dev_notice(info->dev, "%s: en = %d fail(%d)\n", __func__, en, ret);
out:
	mutex_unlock(&info->hidden_mode_lock);
	return ret;
}

static int rt9467_sw_workaround(struct rt9467_info *info)
{
	int ret = 0;

	dev_info(info->dev, "%s\n", __func__);

	rt9467_enable_hidden_mode(info, true);

	/* Modify UG driver */
	ret = rt9467_i2c_update_bits(info, RT9467_REG_CHG_HIDDEN_CTRL4, 0xC0,
		0xF0);
	if (ret < 0)
		dev_notice(info->dev, "%s: set UG driver fail\n", __func__);

	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_HIDDEN_CTRL4);
	dev_info(info->dev, "%s: reg0x23 = 0x%02X\n", __func__, ret);

	/* Disable TS auto sensing */
	ret = rt9467_clr_bit(info, RT9467_REG_CHG_HIDDEN_CTRL15, 0x01);
	if (ret < 0)
		goto out;

	/* Only revision <= E1 needs the following workaround */
	if (info->chip_rev > RT9467_CHIP_REV_E1)
		goto out;

	/* ICC: modify sensing node, make it more accurate */
	ret = rt9467_i2c_write_byte(info, RT9467_REG_CHG_HIDDEN_CTRL8, 0x00);
	if (ret < 0)
		goto out;

	/* DIMIN level */
	ret = rt9467_i2c_write_byte(info, RT9467_REG_CHG_HIDDEN_CTRL9, 0x86);

out:
	rt9467_enable_hidden_mode(info, false);
	return ret;
}

static inline int rt9467_enable_hz(struct rt9467_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL1, RT9467_MASK_HZ_EN);
}

/* Reset all registers' value to default */
static int rt9467_reset_chip(struct rt9467_info *info)
{
	int ret = 0;

	dev_info(info->dev, "%s\n", __func__);

	/* disable hz before reset chip */
	ret = rt9467_enable_hz(info, false);
	if (ret < 0) {
		dev_notice(info->dev, "%s: disable hz fail\n", __func__);
		return ret;
	}

	return rt9467_set_bit(info, RT9467_REG_CORE_CTRL0, RT9467_MASK_RST);
}

static inline int rt9467_enable_jeita(struct rt9467_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL16, RT9467_MASK_JEITA_EN);
}


static int rt9467_get_charging_status(struct rt9467_info *info,
	enum rt9467_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_STAT);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT9467_MASK_CHG_STAT) >> RT9467_SHIFT_CHG_STAT;

	return ret;
}

static inline int __rt9467_is_charging_enable(struct rt9467_info *info,
	bool *en)
{
	return rt9467_i2c_test_bit(info, RT9467_REG_CHG_CTRL2,
		RT9467_SHIFT_CHG_EN, en);
}

static int __rt9467_get_ichg(struct rt9467_info *info, u32 *ichg)
{
	int ret = 0;
	u8 reg_ichg = 0;

	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_CTRL7);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT9467_MASK_ICHG) >> RT9467_SHIFT_ICHG;
	*ichg = rt9467_closest_value(RT9467_ICHG_MIN, RT9467_ICHG_MAX,
		RT9467_ICHG_STEP, reg_ichg);

	return ret;
}

static inline int rt9467_enable_irq_pulse(struct rt9467_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL1, RT9467_MASK_IRQ_PULSE);
}

static inline int rt9467_get_irq_number(struct rt9467_info *info,
	const char *name)
{
	int i = 0;

	if (!name) {
		dev_notice(info->dev, "%s: null name\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(rt9467_irq_mapping_tbl); i++) {
		if (!strcmp(name, rt9467_irq_mapping_tbl[i].name))
			return rt9467_irq_mapping_tbl[i].id;
	}

	return -EINVAL;
}

static int rt9467_parse_dt(struct rt9467_info *info, struct device *dev)
{
	int ret = 0, irq_cnt = 0;
	struct rt9467_desc *desc = NULL;
	struct device_node *np = dev->of_node;
	const char *name = NULL;
	int irqnum = 0;

	dev_info(info->dev, "%s\n", __func__);

	if (!np) {
		dev_notice(info->dev, "%s: no device node\n", __func__);
		return -EINVAL;
	}

	info->desc = &rt9467_default_desc;

	desc = devm_kzalloc(dev, sizeof(struct rt9467_desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	memcpy(desc, &rt9467_default_desc, sizeof(struct rt9467_desc));

	if (of_property_read_string(np, "charger_name",
		&desc->chg_dev_name) < 0)
		dev_notice(info->dev, "%s: no charger name\n", __func__);

#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	ret = of_get_named_gpio(np, "rt,intr_gpio", 0);
	if (ret < 0)
		return ret;
	info->intr_gpio = ret;
#else
	ret = of_property_read_u32(np, "rt,intr_gpio_num", &info->intr_gpio);
	if (ret < 0)
		return ret;
#endif

	dev_info(info->dev, "%s: intr gpio = %d\n", __func__,
		info->intr_gpio);

	desc->en_wdt = of_property_read_bool(np, "en_wdt");
	desc->en_irq_pulse = of_property_read_bool(np, "en_irq_pulse");

	while (true) {
		ret = of_property_read_string_index(np, "interrupt-names",
			irq_cnt, &name);
		if (ret < 0)
			break;
		irq_cnt++;
		irqnum = rt9467_get_irq_number(info, name);
		if (irqnum >= 0)
			rt9467_irq_unmask(info, irqnum);
	}

	info->desc = desc;

	return 0;
}


/* =========================================================== */
/* Released interfaces                                         */
/* =========================================================== */

static int rt9467_set_boost_current_limit(struct rt9467_info *info,
	u32 current_limit)
{
	u8 reg_ilimit = 0;
	int ret = 0;

	reg_ilimit = rt9467_closest_reg_via_tbl(rt9467_boost_oc_threshold,
		ARRAY_SIZE(rt9467_boost_oc_threshold), current_limit);

	dev_info(info->dev, "%s: boost ilimit = %d(0x%02X)\n", __func__,
		current_limit, reg_ilimit);

	/*adjust inducer ocp level with different current limit*/
	ret = ((current_limit >= 2100000) ? rt9467_set_bit :rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL13, RT9467_MASK_OCP);
	if (ret < 0) {
		dev_notice(info->dev, "%s: set ocp fail\n", __func__);
		return ret;
	}

	return rt9467_i2c_update_bits(info, RT9467_REG_CHG_CTRL10,
		reg_ilimit << RT9467_SHIFT_BOOST_OC, RT9467_MASK_BOOST_OC);
}

static int rt9467_enable_otg(struct rt9467_info *info, bool en)
{
	int ret = 0;
	bool en_otg = false;
	u8 hidden_val = en ? 0x00 : 0x0F;
	u8 lg_slew_rate = en ? 0xCC : 0xC3;

	dev_info(info->dev, "%s: en = %d\n", __func__, en);

	rt9467_enable_hidden_mode(info, true);

	/* Set OTG_OC to 1300mA */
	ret = rt9467_set_boost_current_limit(info, 1300000);
	if (ret < 0) {
		dev_notice(info->dev, "%s: set current limit fail\n", __func__);
		return ret;
	}

	/*
	 * Woraround : slow Low side mos Gate driver slew rate
	 * for decline VBUS noise
	 * reg[0x23] = 0xCC after entering OTG mode
	 * reg[0x23] = 0xC3 after leaving OTG mode
	 */
	ret = rt9467_i2c_write_byte(info, RT9467_REG_CHG_HIDDEN_CTRL4,
		lg_slew_rate);
	if (ret < 0) {
		dev_notice(info->dev,
			"%s: set Low side mos Gate drive speed fail(%d)\n",
			__func__, ret);
		goto out;
	}

	/* Enable WDT */
	if (en && info->desc->en_wdt) {
		ret = rt9467_enable_wdt(info, true);
		if (ret < 0) {
			dev_notice(info->dev, "%s: en wdt fail\n", __func__);
			goto err_en_otg;
		}
	}

	/* Switch OPA mode */
	ret = (en ? rt9467_set_bit : rt9467_clr_bit)
		(info, RT9467_REG_CHG_CTRL1, RT9467_MASK_OPA_MODE);

	msleep(20);

	if (en) {
		ret = rt9467_i2c_test_bit(info, RT9467_REG_CHG_CTRL1,
			RT9467_SHIFT_OPA_MODE, &en_otg);
		if (ret < 0 || !en_otg) {
			dev_notice(info->dev, "%s: otg fail(%d)\n", __func__,
				ret);
			goto err_en_otg;
		}
	}

	/*
	 * Woraround reg[0x25] = 0x00 after entering OTG mode
	 * reg[0x25] = 0x0F after leaving OTG mode
	 */
	ret = rt9467_i2c_write_byte(info, RT9467_REG_CHG_HIDDEN_CTRL6,
		hidden_val);
	if (ret < 0)
		dev_notice(info->dev, "%s: workaroud fail(%d)\n", __func__,
			ret);

	/* Disable WDT */
	if (!en) {
		ret = rt9467_enable_wdt(info, false);
		if (ret < 0)
			dev_notice(info->dev, "%s: disable wdt fail\n",
				__func__);
	}
	goto out;

err_en_otg:
	/* Disable WDT */
	ret = rt9467_enable_wdt(info, false);
	if (ret < 0)
		dev_notice(info->dev, "%s: disable wdt fail\n", __func__);

	/* Recover Low side mos Gate slew rate */
	ret = rt9467_i2c_write_byte(info, RT9467_REG_CHG_HIDDEN_CTRL4, 0x73);
	if (ret < 0)
		dev_notice(info->dev,
			"%s: recover Low side mos Gate drive speed fail(%d)\n",
			__func__, ret);
	ret = -EIO;
out:
	rt9467_enable_hidden_mode(info, false);
	return ret;
}

static int rt9467_enable_discharge(struct rt9467_info *info, bool en)
{
	int ret = 0, i = 0;
	const int check_dischg_max = 3;
	bool is_dischg = true;

	dev_info(info->dev, "%s: en = %d\n", __func__, en);

	ret = rt9467_enable_hidden_mode(info, true);
	if (ret < 0)
		return ret;

	/* Set bit2 of reg[0x21] to 1 to enable discharging */
	ret = (en ? rt9467_set_bit : rt9467_clr_bit)(info,
		RT9467_REG_CHG_HIDDEN_CTRL2, 0x04);
	if (ret < 0) {
		dev_notice(info->dev, "%s: en = %d, fail\n", __func__, en);
		return ret;
	}

	if (!en) {
		for (i = 0; i < check_dischg_max; i++) {
			ret = rt9467_i2c_test_bit(info,
				RT9467_REG_CHG_HIDDEN_CTRL2, 2, &is_dischg);
			if (ret >= 0 && !is_dischg)
				break;
			/* Disable discharging */
			ret = rt9467_clr_bit(info, RT9467_REG_CHG_HIDDEN_CTRL2,
				0x04);
		}
		if (i == check_dischg_max)
			dev_notice(info->dev, "%s: disable dischg fail(%d)\n",
				__func__, ret);
	}

	rt9467_enable_hidden_mode(info, false);
	return ret;
}

static int rt9467_get_ichg(struct rt9467_info *info, u32 *ichg)
{
	return __rt9467_get_ichg(info, ichg);
}

static int rt9467_get_aicr(struct rt9467_info *info, u32 *aicr)
{
	int ret = 0;
	u8 reg_aicr = 0;

	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_CTRL3);
	if (ret < 0)
		return ret;

	reg_aicr = (ret & RT9467_MASK_AICR) >> RT9467_SHIFT_AICR;
	*aicr = rt9467_closest_value(RT9467_AICR_MIN, RT9467_AICR_MAX,
		RT9467_AICR_STEP, reg_aicr);

	return ret;
}


#if 0
static int rt9467_get_ibat(struct charger_device *chg_dev, u32 *ibat)
{
	int ret = 0, adc_ibat = 0;
	struct rt9467_info *info = dev_get_drvdata(&chg_dev->dev);

	/* Get value from ADC */
	ret = rt9467_get_adc(info, RT9467_ADC_IBAT, &adc_ibat);
	if (ret < 0)
		return ret;

	*ibat = adc_ibat;

	dev_info(info->dev, "%s: ibat = %dmA\n", __func__, adc_ibat);
	return ret;
}
#endif

#if 0 /* Uncomment if you need this API */
static int rt9467_get_vbus(struct charger_device *chg_dev, u32 *vbus)
{
	int ret = 0, adc_vbus = 0;
	struct rt9467_info *info = dev_get_drvdata(&chg_dev->dev);

	/* Get value from ADC */
	ret = rt9467_get_adc(info, RT9467_ADC_VBUS_DIV2, &adc_vbus);
	if (ret < 0)
		return ret;

	*vbus = adc_vbus;

	dev_info(info->dev, "%s: vbus = %dmA\n", __func__, adc_vbus);
	return ret;
}
#endif

static int rt9467_kick_wdt(struct rt9467_info *info)
{
	enum rt9467_charging_status chg_status = RT9467_CHG_STATUS_READY;

	/* Any I2C communication can reset watchdog timer */
	return rt9467_get_charging_status(info, &chg_status);
}

static int __rt9467_enable_auto_sensing(struct rt9467_info *info, bool en)
{
	int ret = 0;
	u8 auto_sense = 0;
	u8 *data = 0x00;

	/* enter hidden mode */
	ret = rt9467_device_write(info->client, 0x70,
		ARRAY_SIZE(rt9467_val_en_hidden_mode),
		rt9467_val_en_hidden_mode);
	if (ret < 0)
		return ret;

	ret = rt9467_device_read(info->client, RT9467_REG_CHG_HIDDEN_CTRL15, 1,
		&auto_sense);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read auto sense fail\n", __func__);
		goto out;
	}

	if (en)
		auto_sense &= 0xFE; /* clear bit0 */
	else
		auto_sense |= 0x01; /* set bit0 */
	ret = rt9467_device_write(info->client, RT9467_REG_CHG_HIDDEN_CTRL15, 1,
		&auto_sense);
	if (ret < 0)
		dev_notice(info->dev, "%s: en = %d fail\n", __func__, en);

out:
	return rt9467_device_write(info->client, 0x70, 1, &data);
}

/*
 * This function is used in shutdown function
 * Use i2c smbus directly
 */
static int rt9467_sw_reset(struct rt9467_info *info)
{
	int ret = 0;
	u8 evt[RT9467_IRQIDX_MAX] = {0};

	/* Register 0x01 ~ 0x10 */
	u8 reg_data[] = {
		0x10, 0x03, 0x23, 0x3C, 0x67, 0x0B, 0x4C, 0xA1,
		0x3C, 0x58, 0x2C, 0x02, 0x52, 0x05, 0x00, 0x10
	};

	dev_info(info->dev, "%s\n", __func__);

	/* Disable auto sensing/Enable HZ,ship mode of secondary charger */
	if (strcmp(info->desc->chg_dev_name, "secondary_chg") == 0) {
		mutex_lock(&info->hidden_mode_lock);
		mutex_lock(&info->i2c_access_lock);
		__rt9467_enable_auto_sensing(info, false);
		mutex_unlock(&info->i2c_access_lock);
		mutex_unlock(&info->hidden_mode_lock);

		reg_data[0] = 0x14; /* HZ */
		reg_data[1] = 0x83; /* Shipping mode */
	}

	/* Mask all irq */
	mutex_lock(&info->i2c_access_lock);
	ret = rt9467_device_write(info->client, RT9467_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(rt9467_irq_maskall), rt9467_irq_maskall);
	if (ret < 0)
		dev_notice(info->dev, "%s: mask all irq fail\n", __func__);

	/* Read all irq */
	ret = rt9467_device_read(info->client, RT9467_REG_CHG_STATC, 5, evt);
	if (ret < 0)
		dev_notice(info->dev, "%s: read evt1 fail(%d)\n", __func__,
			ret);

	ret = rt9467_device_read(info->client, RT9467_REG_DPDM_IRQ, 1, &evt[6]);
	if (ret < 0)
		dev_notice(info->dev, "%s: read evt2 fail(%d)\n", __func__,
			ret);

	/* Reset necessary registers */
	ret = rt9467_device_write(info->client, RT9467_REG_CHG_CTRL1,
		ARRAY_SIZE(reg_data), reg_data);
	if (ret < 0)
		dev_notice(info->dev, "%s: reset registers fail\n", __func__);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static int rt9467_init_setting(struct rt9467_info *info)
{
	int ret = 0;
	struct rt9467_desc *desc = info->desc;
	u8 evt[RT9467_IRQIDX_MAX] = {0};

	dev_info(info->dev, "%s\n", __func__);

	ret = rt9467_clr_bit(info, RT9467_REG_CHG_CTRL2, RT9467_MASK_CHG_EN);
	if (ret < 0) {	
		dev_notice(info->dev, "%s: disable chg_en fail\n",
			__func__);
		goto err;
	}

	/* disable USB charger type detection before reset IRQ */
	ret = __rt9467_enable_chgdet_flow(info, false);
	if (ret < 0) {
		dev_notice(info->dev, "%s: disable usb chrdet fail\n",
			__func__);
		goto err;
	}

	ret = rt9467_clr_bit(info, RT9467_REG_CHG_DPDM1, 0x40);
	if (ret < 0) {
		dev_notice(info->dev, "%s: disable attach delay fail\n",
			__func__);
		goto err;
	}

	/* mask all irq */
	ret = rt9467_maskall_irq(info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: mask all irq fail\n", __func__);
		goto err;
	}

	/* clear event */
	ret = rt9467_i2c_block_read(info, RT9467_REG_CHG_STATC, ARRAY_SIZE(evt),
		evt);
	if (ret < 0) {
		dev_notice(info->dev, "%s: clr evt fail(%d)\n", __func__, ret);
		goto err;
	}

	ret = rt9467_enable_irq_pulse(info, desc->en_irq_pulse);
	if (ret < 0)
		dev_notice(info->dev, "%s: set irq pulse fail\n", __func__);

	ret = rt9467_sw_workaround(info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: workaround fail\n", __func__);
		return ret;
	}

err:
	return ret;
}

int rt9467_enable_regulator_otg(struct regulator_dev *rdev)
{
	int ret = 0;
	struct rt9467_info *info = rdev_get_drvdata(rdev);

	dev_info(info->dev, "%s\n", __func__);

	ret = rt9467_enable_discharge(info, false);
	if (ret < 0) {
		dev_notice(info->dev, "%s: disable discharge fail\n", __func__);
		return ret;
	}

	return rt9467_enable_otg(info, true);
}

int rt9467_disable_regulator_otg(struct regulator_dev *rdev)
{
	int ret = 0;
	struct rt9467_info *info = rdev_get_drvdata(rdev);

	dev_info(info->dev, "%s\n", __func__);

	ret = rt9467_enable_otg(info, false);
	if (ret < 0) {
		dev_notice(info->dev, "%s: disable otg fail\n", __func__);
		return ret;
	}

	return rt9467_enable_discharge(info, true);
}

static int rt9467_set_current_limit(struct regulator_dev *rdev,
						int min_uA, int max_uA)
{
	struct rt9467_info *info = rdev_get_drvdata(rdev);
	int num = ARRAY_SIZE(rt9467_boost_oc_threshold);

	if (min_uA < rt9467_boost_oc_threshold[0])
		min_uA = rt9467_boost_oc_threshold[0];
	else if (min_uA > rt9467_boost_oc_threshold[num - 1])
		min_uA = rt9467_boost_oc_threshold[num - 1];

	return rt9467_set_boost_current_limit(info, min_uA);
}

static int rt9467_get_current_limit(struct regulator_dev *rdev)
{
	int ret = 0, val = 0;
	struct rt9467_info *info = rdev_get_drvdata(rdev);

	ret = rt9467_i2c_read_byte(info, RT9467_REG_CHG_CTRL10);
	if (ret < 0) {
		dev_notice(info->dev, "%s: read otg_cc fail\n", __func__);
		return ret;
	}
	val = (ret & RT9467_MASK_BOOST_OC) >> RT9467_SHIFT_BOOST_OC;
	return rt9467_boost_oc_threshold[val];
}

static const struct regulator_ops rt9467_chg_otg_ops = {
	.enable = rt9467_enable_regulator_otg,
	.disable = rt9467_disable_regulator_otg,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear, 
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_current_limit = rt9467_set_current_limit,
	.get_current_limit = rt9467_get_current_limit,
};

static const struct regulator_desc rt9467_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &rt9467_chg_otg_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
	.min_uV = 4425000,
	.uV_step = 25000, /* step  25mV */
	.n_voltages = 57, /* 4425mV to 5825mV */
	.vsel_reg = RT9467_REG_CHG_CTRL5,
	.vsel_mask = RT9467_MASK_BOOST_VOREG,
	.enable_reg = RT9467_REG_CHG_CTRL1,
	.enable_mask = RT9467_MASK_OPA_MODE,
};
/* ========================= */
/* I2C driver function       */
/* ========================= */

static int rt9467_probe(struct i2c_client *client,
	const struct i2c_device_id *dev_id)
{
	int ret = 0;
	struct rt9467_info *info = NULL;
	struct regulator_config config = { };

	pr_info("%s(%s)\n", __func__, RT9467_DRV_VERSION);

	info = devm_kzalloc(&client->dev, sizeof(struct rt9467_info),
		GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	mutex_init(&info->i2c_access_lock);
	mutex_init(&info->adc_access_lock);
	mutex_init(&info->irq_access_lock);
	mutex_init(&info->aicr_access_lock);
	mutex_init(&info->ichg_access_lock);
	mutex_init(&info->hidden_mode_lock);

	info->client = client;
	info->dev = &client->dev;
	info->hidden_mode_cnt = 0;
	memcpy(info->irq_mask, rt9467_irq_maskall, RT9467_IRQIDX_MAX);

	/* Is HW exist */
	if (!rt9467_is_hw_exist(info)) {
		dev_notice(info->dev, "%s: no rt9467 exists\n", __func__);
		ret = -ENODEV;
		goto err_no_dev;
	}
	i2c_set_clientdata(client, info);

	ret = rt9467_parse_dt(info, &client->dev);
	if (ret < 0) {
		dev_notice(info->dev, "%s: parse dt fail\n", __func__);
		goto err_parse_dt;
	}

	ret = rt9467_reset_chip(info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: reset chip fail\n", __func__);
		goto err_reset_chip;
	}

	ret = rt9467_init_setting(info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: init setting fail\n", __func__);
		goto err_init_setting;
	}

	ret = rt9467_irq_register(info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: irq register fail\n", __func__);
		goto err_irq_register;
	}

	ret = rt9467_irq_init(info);
	if (ret < 0) {
		dev_notice(info->dev, "%s: irq init fail\n", __func__);
		goto err_irq_init;
	}

	/* otg regulator */
	config.dev = info->dev;
	config.driver_data = info;
	config.regmap = devm_regmap_init_i2c(info->client,
					    &rt9467_regmap_config);

	info->otg_rdev = devm_regulator_register(info->dev, &rt9467_otg_rdesc,
						&config);
	if (IS_ERR(info->otg_rdev)) {
		dev_notice(info->dev, "%s : regulator register fail\n", __func__);
		goto err_regulator_dev;
	}

	dev_info(info->dev, "%s: successfully\n", __func__);
	return ret;

err_regulator_dev:
err_irq_init:
err_irq_register:
err_init_setting:
err_reset_chip:
err_parse_dt:
err_no_dev:
	mutex_destroy(&info->i2c_access_lock);
	mutex_destroy(&info->adc_access_lock);
	mutex_destroy(&info->irq_access_lock);
	mutex_destroy(&info->aicr_access_lock);
	mutex_destroy(&info->ichg_access_lock);
	mutex_destroy(&info->hidden_mode_lock);
	return ret;
}


static int rt9467_remove(struct i2c_client *client)
{
	int ret = 0;
	struct rt9467_info *info = i2c_get_clientdata(client);

	pr_info("%s\n", __func__);

	if (info) {
		mutex_destroy(&info->i2c_access_lock);
		mutex_destroy(&info->adc_access_lock);
		mutex_destroy(&info->irq_access_lock);
		mutex_destroy(&info->aicr_access_lock);
		mutex_destroy(&info->ichg_access_lock);
		mutex_destroy(&info->hidden_mode_lock);
	}

	return ret;
}

static void rt9467_shutdown(struct i2c_client *client)
{
	int ret = 0;
	struct rt9467_info *info = i2c_get_clientdata(client);

	pr_info("%s\n", __func__);
	if (info) {
		ret = rt9467_sw_reset(info);
		if (ret < 0)
			pr_notice("%s: sw reset fail\n", __func__);
	}
}

static int rt9467_suspend(struct device *dev)
{
	struct rt9467_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);
	if (device_may_wakeup(dev))
		enable_irq_wake(info->irq);

	return 0;
}

static int rt9467_resume(struct device *dev)
{
	struct rt9467_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);
	if (device_may_wakeup(dev))
		disable_irq_wake(info->irq);

	return 0;
}

static SIMPLE_DEV_PM_OPS(rt9467_pm_ops, rt9467_suspend, rt9467_resume);

static const struct i2c_device_id rt9467_i2c_id[] = {
	{"rt9467", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rt9467_i2c_id);

static const struct of_device_id rt9467_of_match[] = {
	{ .compatible = "richtek,rt9467", },
	{},
};
MODULE_DEVICE_TABLE(of, rt9467_of_match);

#ifndef CONFIG_OF
#define RT9467_BUSNUM 1

static struct i2c_board_info rt9467_i2c_board_info __initdata = {
	I2C_BOARD_INFO("rt9467", RT9467_SALVE_ADDR)
};
#endif /* CONFIG_OF */


static struct i2c_driver rt9467_i2c_driver = {
	.driver = {
		.name = "rt9467",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt9467_of_match),
		.pm = &rt9467_pm_ops,
	},
	.probe = rt9467_probe,
	.remove = rt9467_remove,
	.shutdown = rt9467_shutdown,
	.id_table = rt9467_i2c_id,
};

static int __init rt9467_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	pr_info("%s: with dts\n", __func__);
#else
	pr_info("%s: without dts\n", __func__);
	i2c_register_board_info(RT9467_BUSNUM, &rt9467_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9467_i2c_driver);
	if (ret < 0)
		pr_notice("%s: register i2c driver fail\n", __func__);

	return ret;
}
module_init(rt9467_init);


static void __exit rt9467_exit(void)
{
	i2c_del_driver(&rt9467_i2c_driver);
}
module_exit(rt9467_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_DESCRIPTION("RT9467 Charger Driver");
MODULE_VERSION(RT9467_DRV_VERSION);

/*
 * Release Note
 * 1.0.19
 * (1) Fast toggle chgdet flow
 * (2) When chg_type is DCP, enable chgdet for D+ 0.6V
 * (3) mutex_unlock() once in rt9467_attachi_irq_handler()
 * (4) Lower UG driver slew rate
 * (5) Show VBUS, CV in rt9467_dump_register()
 *
 * 1.0.18
 * (1) Check tchg 3 times if it >= 120 degree
 *
 * 1.0.17
 * (1) Add ichg workaround
 *
 * 1.0.16
 * (1) Fix type error of enable_auto_sensing in sw_reset
 * (2) Move irq_mask to info structure
 * (3) Remove config of Charger_Detect_Init/Release
 *
 * 1.0.15
 * (1) Do ilim select in WQ and register charger class in probe
 *
 * 1.0.14
 * (1) Disable attach delay
 * (2) Enable IRQ_RZE at the end of irq handler
 * (3) Remove IRQ related registers from reg_addr
 * (4) Recheck status in irq handler
 * (5) Use bc12_access_lock instead of chgdet_lock
 *
 * 1.0.13
 * (1) Add do event interface for polling mode
 * (2) Check INT pin after reading evt
 *
 * 1.0.12
 * (1) Add MTK_SSUSB config for Charger_Detect_Init/Release
 *
 * 1.0.11
 * (1) Disable psk mode in pep20_reest
 * (2) For secondary chg, enter shipping mode before shdn
 * (3) Add BC12 sdp workaround
 * (4) Remove enabling/disabling ILIM in chgdet_handler
 *
 * 1.0.10
 * (1) Add IEOC workaround
 * (2) Release set_ieoc/enable_te interface
 * (3) Fix type errors
 *
 * 1.0.9
 * (1) Add USB workaround for CDP port
 * (2) Plug in -> usbsw to charger, after chgdet usbsw to AP
 *     Plug out -> usbsw to AP
 * (3) Filter out not changed irq state
 * (4) Not to use CHG_IRQ3
 *
 * 1.0.8
 * (1) Set irq to wake up system
 * (2) Refine I2C driver related table
 *
 * 1.0.7
 * (1) Enable/Disable ILIM in chgdet_handler
 *
 * 1.0.6
 * (1) Prevent backboot
 * (2) Add CEB pin control for secondary charger
 * (3) After PE pattern -> Enable skip mode
 *     Disable skip mode -> Start PE pattern
 * (4) Disable BC12 detection before reset IRQ in init_setting
 *
 * 1.0.5
 * (1) Remove wait CDP flow
 * (2) Add rt9467_chgdet_handler for attachi/detachi
 * (3) Set secondary chg to HZ if it is not in charging mode
 * (4) Add is_charging_enabled, get_min_ichg OPS
 *
 * 1.0.4
 * (1) Set ichg&aicr, enable chg before sending PE+ series pattern
 * (2) Add enable_cable_drop_com OPS
 *
 * 1.0.3
 * (1) IRQs are default unmasked before E4, need to mask them manually
 *
 * 1.0.2
 * (1) Fix AICL workqueue lock issue
 *
 * 1.0.1
 * (1) Fix IRQ init sequence
 *
 * 1.0.0
 * (1) Initial released
 */
