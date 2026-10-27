// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/interrupt.h>
#include <linux/mfd/mt6330/core.h>
#include <linux/mfd/mt6330/registers.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#undef MT6330_SPMIMST_RCSCLR
#define MT6330_SPMIMST_STARTADDR  (0x10029000)
#define MT6330_SPMIMST_ENDADDR    (0x100290FF)
#define MT6330_REG_SPMIMST_RCSCLR (0x24)
#define MT6330_MSK_SPMIMST_RCSCLR (0xFF)

struct irq_top_t {
	int hwirq_base;
	unsigned int num_int_regs;
	unsigned int en_reg;
	unsigned int en_reg_shift;
	unsigned int sta_reg;
	unsigned int sta_reg_shift;
	unsigned int top_offset;
	unsigned int top_mask_offset;
};

struct pmic_irq_data {
	unsigned int num_top;
	unsigned int num_pmic_irqs;
	unsigned int reg_width;
	unsigned short top_int_status_reg_l;
	unsigned short top_int_status_reg_h;
	unsigned short top_int_mask_set_reg;
	unsigned short top_int_mask_clr_reg;
	bool *enable_hwirq;
	bool *cache_hwirq;
	struct irq_top_t *pmic_ints;
};

static struct irq_top_t mt6330_ints[] = {
	MT6330_TOP_GEN(MISC),
	MT6330_TOP_GEN(BUCK),
	MT6330_TOP_GEN(LDO),
	MT6330_TOP_GEN(PSC),
	MT6330_TOP_GEN(SCK),
};

#ifdef MT6330_SPMIMST_RCSCLR
static inline void mt6330_clear_spmimst_rcs(struct mt6330_chip *chip)
{
	writel(MT6330_MSK_SPMIMST_RCSCLR,
	       (chip->spmimst_base + MT6330_REG_SPMIMST_RCSCLR));
}

static struct resource spmimst_resource = {
	.start = MT6330_SPMIMST_STARTADDR,
	.end   = MT6330_SPMIMST_ENDADDR,
	.flags = IORESOURCE_MEM,
	.name  = "spmimst",
};
#endif /* MT6362_SPMIMST_RCSCLR */

static void pmic_irq_enable(struct irq_data *data)
{
	unsigned int hwirq = irqd_to_hwirq(data);
	struct mt6330_chip *chip = irq_data_get_irq_chip_data(data);
	struct pmic_irq_data *irqd = chip->irq_data;

	irqd->enable_hwirq[hwirq] = true;
}

static void pmic_irq_disable(struct irq_data *data)
{
	unsigned int hwirq = irqd_to_hwirq(data);
	struct mt6330_chip *chip = irq_data_get_irq_chip_data(data);
	struct pmic_irq_data *irqd = chip->irq_data;

	irqd->enable_hwirq[hwirq] = false;
}

static void pmic_irq_lock(struct irq_data *data)
{
	struct mt6330_chip *chip = irq_data_get_irq_chip_data(data);

	mutex_lock(&chip->irqlock);
}

static void pmic_irq_sync_unlock(struct irq_data *data)
{
	unsigned int i, top_gp, en_reg, int_regs, shift;
	struct mt6330_chip *chip = irq_data_get_irq_chip_data(data);
	struct pmic_irq_data *irqd = chip->irq_data;

	for (i = 0; i < irqd->num_pmic_irqs; i++) {
		if (irqd->enable_hwirq[i] == irqd->cache_hwirq[i])
			continue;

		top_gp = 0;
		while ((top_gp + 1) < irqd->num_top &&
			i >= irqd->pmic_ints[top_gp + 1].hwirq_base)
			top_gp++;

		if (top_gp >= irqd->num_top) {
			mutex_unlock(&chip->irqlock);
			dev_err(chip->dev,
				"Failed to get top_group: %d\n", top_gp);
			return;
		}

		int_regs = (i - irqd->pmic_ints[top_gp].hwirq_base) /
			    irqd->reg_width;
		en_reg = irqd->pmic_ints[top_gp].en_reg +
			irqd->pmic_ints[top_gp].en_reg_shift * int_regs;
		shift = (i - irqd->pmic_ints[top_gp].hwirq_base) %
			irqd->reg_width;
		regmap_update_bits(chip->regmap, en_reg, BIT(shift),
				   irqd->enable_hwirq[i] << shift);
		irqd->cache_hwirq[i] = irqd->enable_hwirq[i];
	}
	mutex_unlock(&chip->irqlock);
}

static struct irq_chip mt6330_irq_chip = {
	.name = "mt6330-irq",
	.flags = IRQCHIP_SKIP_SET_WAKE,
	.irq_enable = pmic_irq_enable,
	.irq_disable = pmic_irq_disable,
	.irq_bus_lock = pmic_irq_lock,
	.irq_bus_sync_unlock = pmic_irq_sync_unlock,
};

static void mt6330_irq_sp_handler(struct mt6330_chip *chip,
				  unsigned int top_gp)
{
	unsigned int sta_reg, irq_status = 0;
	unsigned int hwirq, virq;
	int ret, i, j;
	struct pmic_irq_data *irqd = chip->irq_data;

	for (i = 0; i < irqd->pmic_ints[top_gp].num_int_regs; i++) {
		sta_reg = irqd->pmic_ints[top_gp].sta_reg +
			irqd->pmic_ints[top_gp].sta_reg_shift * i;
		ret = regmap_read(chip->regmap, sta_reg, &irq_status);
		if (ret) {
			dev_err(chip->dev,
				"Failed to read irq status: %d\n", ret);
			return;
		}

		if (!irq_status)
			continue;

		for (j = 0; j < irqd->reg_width ; j++) {
			if ((irq_status & BIT(j)) == 0)
				continue;
			hwirq = irqd->pmic_ints[top_gp].hwirq_base +
				irqd->reg_width * i + j;
			virq = irq_find_mapping(chip->irq_domain, hwirq);
			dev_info(chip->dev,
				"Reg[0x%x]=0x%x,hwirq=%d,type=%d\n",
				sta_reg, irq_status, hwirq,
				irq_get_trigger_type(virq));
			if (virq)
				handle_nested_irq(virq);
		}

#ifdef MT6330_SPMIMST_RCSCLR
		mt6330_clear_spmimst_rcs(chip);
#endif
		regmap_write(chip->regmap, sta_reg, irq_status);
	}
}
#if IS_ENABLED(CONFIG_SPMI_MTK_PMIF)
extern void spmi_dump_spmimst_all_reg(void);
#endif
static irqreturn_t mt6330_irq_handler(int irq, void *data)
{
	struct mt6330_chip *chip = data;
	struct pmic_irq_data *irqd = chip->irq_data;
	unsigned int top_irq_status_h = 0, top_irq_status = 0;
	unsigned int i;
	int ret;

	ret = regmap_read(chip->regmap,
			  irqd->top_int_status_reg_h,
			  &top_irq_status_h);
	if (ret) {
		dev_err(chip->dev, "Can't read TOP_INT_STATUS0 ret=%d\n", ret);
		return IRQ_NONE;
	}

	ret = regmap_read(chip->regmap,
			  irqd->top_int_status_reg_l,
			  &top_irq_status);
	if (ret) {
		dev_err(chip->dev, "Can't read TOP_INT_STATUS1 ret=%d\n", ret);
		return IRQ_NONE;
	}
	top_irq_status |= (top_irq_status_h << 8);

	dev_info(chip->dev, "%s: top_irq_sts:0x%x\n", __func__, top_irq_status);
	if (!top_irq_status) {
#if IS_ENABLED(CONFIG_SPMI_MTK_PMIF)
		spmi_dump_spmimst_all_reg();
#endif
	}
	for (i = 0; i < irqd->num_top; i++) {
		if (top_irq_status & BIT(irqd->pmic_ints[i].top_offset)) {
			/* Mask top INT */
			regmap_write(chip->regmap, irqd->top_int_mask_set_reg,
						BIT(irqd->pmic_ints[i].top_mask_offset));

			mt6330_irq_sp_handler(chip, i);

			/* Umask top INT */
			regmap_write(chip->regmap, irqd->top_int_mask_clr_reg,
						BIT(irqd->pmic_ints[i].top_mask_offset));
		}
	}

	return IRQ_HANDLED;
}

static int pmic_irq_domain_map(struct irq_domain *d, unsigned int irq,
			       irq_hw_number_t hw)
{
	struct mt6330_chip *mt6330 = d->host_data;

	irq_set_chip_data(irq, mt6330);
	irq_set_chip_and_handler(irq, &mt6330_irq_chip, handle_level_irq);
	irq_set_nested_thread(irq, 1);
	irq_set_noprobe(irq);

	return 0;
}

static const struct irq_domain_ops mt6330_irq_domain_ops = {
	.map = pmic_irq_domain_map,
	.xlate = irq_domain_xlate_twocell,
};

int mt6330_irq_init(struct mt6330_chip *chip)
{
	int i, j, ret;
	struct pmic_irq_data *irqd;

	irqd = devm_kzalloc(chip->dev, sizeof(*irqd), GFP_KERNEL);
	if (!irqd)
		return -ENOMEM;

	chip->irq_data = irqd;

	mutex_init(&chip->irqlock);
	irqd->num_top = ARRAY_SIZE(mt6330_ints);
	irqd->num_pmic_irqs = MT6330_IRQ_NR;
	irqd->reg_width = MT6330_REG_WIDTH;
	irqd->top_int_status_reg_l = MT6330_TOP_INT_STATUS0;
	irqd->top_int_status_reg_h = MT6330_TOP_INT_STATUS1;
	irqd->top_int_mask_set_reg = MT6330_TOP_INT_MASK_CON0_SET;
	irqd->top_int_mask_clr_reg = MT6330_TOP_INT_MASK_CON0_CLR;
	irqd->pmic_ints = mt6330_ints;

	dev_info(chip->dev, "mt6330_irq_init +++\n");
	irqd->enable_hwirq = devm_kcalloc(chip->dev,
					  irqd->num_pmic_irqs,
					  sizeof(bool),
					  GFP_KERNEL);
	if (!irqd->enable_hwirq)
		return -ENOMEM;

	irqd->cache_hwirq = devm_kcalloc(chip->dev,
					 irqd->num_pmic_irqs,
					 sizeof(bool),
					 GFP_KERNEL);
	if (!irqd->cache_hwirq)
		return -ENOMEM;

#ifdef MT6330_SPMIMST_RCSCLR
	chip->spmimst_base = devm_ioremap(chip->dev, spmimst_resource.start,
					  resource_size(&spmimst_resource));
	if (!chip->spmimst_base) {
		dev_notice(chip->dev,
			   "Failed to ioremap spmi master address\n");
		return -EINVAL;
	}
	mt6330_clear_spmimst_rcs(chip);
#endif /* MT6330_SPMIMST_RCSCLR */

	/* Disable all interrupt for initializing */
	for (i = 0; i < irqd->num_top; i++) {
		for (j = 0; j < irqd->pmic_ints[i].num_int_regs; j++)
			regmap_write(chip->regmap,
				     irqd->pmic_ints[i].en_reg +
				     irqd->pmic_ints[i].en_reg_shift * j, 0);
	}

	chip->irq_domain = irq_domain_add_linear(chip->dev->of_node,
						 irqd->num_pmic_irqs,
						 &mt6330_irq_domain_ops, chip);
	if (!chip->irq_domain) {
		dev_err(chip->dev, "could not create irq domain\n");
		return -ENODEV;
	}

	ret = devm_request_threaded_irq(chip->dev, chip->irq, NULL,
					mt6330_irq_handler, IRQF_ONESHOT,
					mt6330_irq_chip.name, chip);
	if (ret) {
		dev_err(chip->dev, "failed to register irq=%d; err: %d\n",
			chip->irq, ret);
		return ret;
	}

	enable_irq_wake(chip->irq);
	dev_info(chip->dev, "mt6330_irq_init ---\n");
	return ret;
}
