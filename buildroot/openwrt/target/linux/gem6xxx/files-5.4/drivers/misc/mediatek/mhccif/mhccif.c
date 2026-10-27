/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/bitops.h>

#include "mtk_mhccif.h"

struct mtk_mhccif_chip_data {
	void __iomem *rc_base; /* rc side register */
	void __iomem *ep_base; /* ep side register */
	int irq;
	struct irq_domain *domain;
};

struct mtk_mhccif_alloc_info {
	int id;
};

static struct irq_domain *mtk_mhccif_irq_domain = NULL;

static void mtk_mhccif_ack_irq(struct irq_data *data)
{
	struct mtk_mhccif_chip_data *chip_data;

	chip_data = irq_data_get_irq_chip_data(data);
	writel(BIT(data->hwirq), chip_data->ep_base + MHCCIF_IRQ_ACK);
}

static void mtk_mhccif_mask_irq(struct irq_data *data)
{
	struct mtk_mhccif_chip_data *chip_data;

	chip_data = irq_data_get_irq_chip_data(data);
	writel(BIT(data->hwirq), chip_data->ep_base + MHCCIF_MASK_SET);
}

static void mtk_mhccif_unmask_irq(struct irq_data *data)
{
	struct mtk_mhccif_chip_data *chip_data;

	chip_data = irq_data_get_irq_chip_data(data);
	writel(BIT(data->hwirq), chip_data->ep_base + MHCCIF_MASK_CLR);
}

static struct irq_chip mtk_mhccif_irq_chip = {
	.name = "MTK_MHCCIF",
	.irq_ack = mtk_mhccif_ack_irq,
	.irq_mask = mtk_mhccif_mask_irq,
	.irq_unmask = mtk_mhccif_unmask_irq,
};

static int mtk_mhccif_domain_alloc(struct irq_domain *domain, unsigned int virq,
				   unsigned int nr_irqs, void *arg)
{
	struct mtk_mhccif_alloc_info *info = arg;
	int hwirq = info->id;

	if (hwirq <= 0)
		return -ENXIO;

	WARN_ON(nr_irqs != 1);

	irq_domain_set_info(domain, virq, hwirq, &mtk_mhccif_irq_chip,
			    domain->host_data, handle_level_irq, NULL, NULL);

	return 0;
}

static void mtk_mhccif_domain_free(struct irq_domain *domain, unsigned int virq,
				   unsigned int nr_irqs)
{
	irq_domain_free_irqs(virq, nr_irqs);
}

static const struct irq_domain_ops mhccif_domain_ops = {
	.alloc = mtk_mhccif_domain_alloc,
	.free = mtk_mhccif_domain_free,
};

static void mtk_mhccif_intr_handler(struct irq_desc *desc)
{
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	struct mtk_mhccif_chip_data *chip_data = irq_desc_get_handler_data(desc);
	u32 mask, bit;
	unsigned long status;
	int virq;

	chained_irq_enter(irqchip, desc);

	status = readl(chip_data->ep_base + MHCCIF_INT_STS);
	mask = readl(chip_data->ep_base + MHCCIF_MASK_STS);
	status &= ~mask;

	for_each_set_bit(bit, &status, MHCCIF_IRQS_NUM) {
		virq = irq_find_mapping(chip_data->domain, bit);
		generic_handle_irq(virq);
	}

	chained_irq_exit(irqchip, desc);
}

static struct irq_domain *mtk_mhccif_get_irq_domain(void)
{
	return mtk_mhccif_irq_domain;
}

/**
 * mtk_mhccif_alloc_irq - Allocate interrupts from mhccif domain
 * @id: user id
 * Returns error code or allocated IRQ number
 */
int mtk_mhccif_alloc_irq(int id)
{
	int virq;
	struct mtk_mhccif_alloc_info arg;
	struct irq_domain *domain;
	struct irq_data *irq_data;

	if (id > MHCCIF_IRQS_NUM)
		return -ENOSPC;

	domain = mtk_mhccif_get_irq_domain();
	if (!domain)
		return -EINVAL;

	memset(&arg, 0, sizeof(arg));
	arg.id = id;

	virq = irq_domain_alloc_irqs(domain, 1, -1, &arg);
	if (virq < 0)
		return -ENOSPC;

	irq_data = irq_domain_get_irq_data(domain, virq);
	irq_domain_activate_irq(irq_data, 0);

	return virq;
}

/**
 * mtk_mhccif_irq_to_host - Trigger interrupt to host side
 * @channel: mhccif channel
 * Returns zero on success or a negative error code.
 */
int mtk_mhccif_irq_to_host(int channel)
{
	struct irq_domain *domain;
	struct mtk_mhccif_chip_data *chip_data;

	domain = mtk_mhccif_get_irq_domain();
	if (!domain)
		return -EINVAL;

	chip_data = domain->host_data;
	writel(channel, chip_data->ep_base + MHCCIF_SW_TCHNUM);

	return 0;
}

static int __init mtk_mhccif_of_init(struct device_node *node,
				     struct device_node *parent)
{
	struct mtk_mhccif_chip_data *chip_data;
	int ret;

	chip_data = kzalloc(sizeof(*chip_data), GFP_KERNEL);
	if (!chip_data)
		return -ENOMEM;

	/* RC side register, only for debug purpose, do not touch casually */
	chip_data->rc_base = of_iomap(node, 0);
	if (!chip_data->rc_base) {
		pr_notice("couldn't map rc register region\n");
		ret = -ENODEV;
		goto rc_remap_err;
	}

	chip_data->ep_base = of_iomap(node, 1);
	if (!chip_data->ep_base) {
		pr_notice("couldn't map ep register region\n");
		ret = -ENODEV;
		goto ep_remap_err;
	}

	chip_data->irq = irq_of_parse_and_map(node, 0);
	if (chip_data->irq <= 0) {
		pr_notice("mhccif: unable to parse irq number\n");
		ret = -EINVAL;
		goto irq_parse_err;
	}

	chip_data->domain = irq_domain_add_hierarchy(NULL, 0, 0, node,
					  &mhccif_domain_ops, chip_data);
	if (!chip_data->domain) {
		ret = -ENOMEM;
		goto irq_domain_err;
	}
	mtk_mhccif_irq_domain = chip_data->domain;

	irq_set_chained_handler_and_data(chip_data->irq,
					 mtk_mhccif_intr_handler, chip_data);

	pr_notice("mtk mhccif init done.\n");

	return 0;

irq_domain_err:
	irq_dispose_mapping(chip_data->irq);
irq_parse_err:
	iounmap(chip_data->ep_base);
ep_remap_err:
	iounmap(chip_data->rc_base);
rc_remap_err:
	kfree(chip_data);

	return ret;
}

IRQCHIP_DECLARE(mtk_mhccif, "mediatek,mt6880-mhccif", mtk_mhccif_of_init);
