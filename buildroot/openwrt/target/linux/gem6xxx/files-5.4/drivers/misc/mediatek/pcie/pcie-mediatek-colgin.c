// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek PCIe host controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Jianjun Wang <jianjun.wang@mediatek.com>
 */

#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/msi.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_clk.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>

#include "../../../pci/pci.h"

/* PCIe per-port registers */
#define PCIE_BASE_CONF_REG		0x14
#define PCIE_SUPPORT_SPEED_MASK		GENMASK(15, 8)
#define PCIE_SUPPORT_SPEED_SHIFT	8
#define PCIE_SUPPORT_SPEED_2_5GT	BIT(8)
#define PCIE_SUPPORT_SPEED_5_0GT	BIT(9)
#define PCIE_SUPPORT_SPEED_8_0GT	BIT(10)
#define PCIE_SUPPORT_SPEED_16_0GT	BIT(11)

#define PCIE_SETTING_REG		0x80
#define PCIE_RC_MODE			BIT(0)
#define PCIE_GEN_SUPPORT_MASK		GENMASK(14, 12)
#define PCIE_GEN_SUPPORT_SHIFT		12
#define PCIE_GEN2_SUPPORT		BIT(12)
#define PCIE_GEN3_SUPPORT		BIT(13)
#define PCIE_GEN4_SUPPORT		BIT(14)

#define PCIE_GEN_SUPPORT(max_lspd) \
	GENMASK((max_lspd) - 2 + PCIE_GEN_SUPPORT_SHIFT, PCIE_GEN_SUPPORT_SHIFT)

#define PCIE_TARGET_SPEED_MASK          GENMASK(3, 0)

#define PCIE_VCORE_550_MILLIVOLT	0
#define PCIE_VCORE_600_MILLIVOLT	1

#define PCIE_PCI_IDS_1			0x9c
#define PCI_CLASS(class)		(class << 8)

#define PCIE_PEX_LINK			0xc8
#define ASPM_L1_TIMER_RECOUNT		BIT(21)

#define PCIE_CFGNUM_REG			0x140
#define PCIE_CFG_DEVFN(devfn)		((devfn) & GENMASK(7, 0))
#define PCIE_CFG_BUS(bus)		(((bus) << 8) & GENMASK(15, 8))
#define PCIE_CFG_BYTE_EN(bytes)		(((bytes) << 16) & GENMASK(19, 16))
#define PCIE_CFG_FORCE_BYTE_EN		BIT(20)
#define PCIE_CFG_OFFSET_ADDR		0x1000
#define PCIE_CFG_HEADER(devfn, bus) \
	(PCIE_CFG_DEVFN(devfn) | PCIE_CFG_BUS(bus))

#define PCIE_CFG_HEADER_FORCE_BE(devfn, bus, bytes) \
	(PCIE_CFG_HEADER(devfn, bus) | PCIE_CFG_BYTE_EN(bytes) \
	 | PCIE_CFG_FORCE_BYTE_EN)

#define PCIE_RST_CTRL_REG		0x148
#define PCIE_MAC_RSTB			BIT(0)
#define PCIE_PHY_RSTB			BIT(1)
#define PCIE_BRG_RSTB			BIT(2)
#define PCIE_PE_RSTB			BIT(3)

#define PCIE_MISC_STATUS_REG		0x14C
#define PCIE_LTR_MSG_RECEIVED		BIT(0)
#define PCIE_PCIE_MSG_RECEIVED		BIT(1)

#define PCIE_LTSSM_STATUS_REG		0x150
#define PCIE_LTSSM_STATE_MASK		GENMASK(28, 24)
#define PCIE_LTSSM_STATE(val)		((val & PCIE_LTSSM_STATE_MASK) >> 24)
#define PCIE_LTSSM_STATE_L0		0x10
#define PCIE_LTSSM_STATE_L1_IDLE	0x13
#define PCIE_LTSSM_STATE_L2_IDLE	0x14

#define PCIE_LINK_STATUS_REG		0x154
#define PCIE_PORT_LINKUP		BIT(8)

#define PCIE_MSI_SET_NUM		8
#define PCIE_MSI_IRQS_PER_SET		32
#define PCIE_MSI_IRQS_NUM \
	(PCIE_MSI_IRQS_PER_SET * (PCIE_MSI_SET_NUM))

#define PCIE_INT_ENABLE_REG		0x180
#define PCIE_MSI_ENABLE			GENMASK(PCIE_MSI_SET_NUM + 8 - 1, 8)
#define PCIE_INTX_SHIFT			24
#define PCIE_MSI_SHIFT			8
#define PCIE_INTX_MASK			GENMASK(27, 24)
#define PCIE_MSG_MASK			BIT(28)
#define PCIE_AER_MASK			BIT(29)
#define PCIE_PM_MASK			BIT(30)

#define PCIE_INT_STATUS_REG		0x184
#define PCIE_MSI_SET_ENABLE_REG		0x190
#define PCIE_MSI_SET_ENABLE		GENMASK(PCIE_MSI_SET_NUM - 1, 0)

#define PCIE_LOW_POWER_CTRL_REG		0x194
#define PCIE_DIS_LOWPWR_MASK		GENMASK(3, 0)
#define PCIE_DIS_L0S_MASK		BIT(0)
#define PCIE_DIS_L1_MASK		BIT(1)
#define PCIE_DIS_L11_MASK		BIT(2)
#define PCIE_DIS_L12_MASK		BIT(3)
#define PCIE_FORCE_DIS_LOWPWR		GENMASK(11, 8)
#define PCIE_FORCE_DIS_L0S		BIT(8)
#define PCIE_FORCE_DIS_L1		BIT(9)
#define PCIE_FORCE_DIS_L11		BIT(10)
#define PCIE_FORCE_DIS_L12		BIT(11)

#define PCIE_ICMD_PM_REG		0x198
#define PCIE_TURN_OFF_LINK		BIT(4)

#define PCIE_AXI_IF_CTRL		0x1a8
#define PCIE_AXI_TAG_EN			BIT(1)

#define PCIE_MSI_SET_BASE_REG		0xc00
#define PCIE_MSI_SET_OFFSET		0x10
#define PCIE_MSI_SET_STATUS_OFFSET	0x04
#define PCIE_MSI_SET_ENABLE_OFFSET	0x08

#define PCIE_MSI_SET_ADDR_HI_BASE	0xc80
#define PCIE_MSI_SET_ADDR_HI_OFFSET	0x04

#define PCIE_TRANS_TABLE_BASE_REG	0x800
#define PCIE_ATR_SRC_ADDR_MSB_OFFSET	0x4
#define PCIE_ATR_TRSL_ADDR_LSB_OFFSET	0x8
#define PCIE_ATR_TRSL_ADDR_MSB_OFFSET	0xc
#define PCIE_ATR_TRSL_PARAM_OFFSET	0x10
#define PCIE_ATR_TLB_SET_OFFSET		0x20

#define PCIE_MAX_TRANS_TABLES		8
#define ATR_SIZE(size)			(((size) << 1) & GENMASK(6, 1))
#define ATR_ID(id)			(id & GENMASK(3, 0))
#define ATR_PARAM(param)		(((param) << 16) & GENMASK(27, 16))

/* PCIe configuration registers */
#define PCIE_CONF_EXP_LNKCTL2_REG       0x10B0

#define CHIP_VER_E1	0x00
#define CHIP_VER_E2	0x01

struct tag_chipid {
	u32 size;
	u32 hw_code;
	u32 hw_subcode;
	u32 hw_ver;
	u32 sw_ver;
};

/**
 * struct mtk_msi_set - MSI information for each set
 * @base: IO mapped register base
 * @msg_addr: MSI message address
 * @saved_irq_state: IRQ enable state saved at suspend time
 */
struct mtk_msi_set {
	void __iomem *base;
	phys_addr_t msg_addr;
	u32 saved_irq_state;
};

/**
 * struct mtk_pcie_port - PCIe port information
 * @dev: PCIe device
 * @base: IO mapped register base
 * @reg_base: Physical register base
 * @mac_reset: mac reset control
 * @phy_reset: phy reset control
 * @phy: PHY controller block
 * @clks: PCIe clocks
 * @num_clks: PCIe clocks count for this port
 * @is_suspended: device suspend state
 * @irq: PCIe controller interrupt number
 * @intx_domain: legacy INTx IRQ domain
 * @msi_domain: MSI IRQ domain
 * @msi_top_domain: MSI IRQ top domain
 * @msi_info: MSI sets information
 * @lock: lock protecting IRQ bit map
 * @msi_irq_in_use: bit map for assigned MSI IRQ
 */
struct mtk_pcie_port {
	struct device *dev;
	void __iomem *base;
	phys_addr_t reg_base;
	struct reset_control *mac_reset;
	struct reset_control *phy_reset;
	struct phy *phy;
	struct clk **clks;
	int num_clks;
	int port_num;
	unsigned int busnr;
	int max_link_speed;
	enum pci_bus_speed link_speed;
	bool is_suspended;
	u32 sw_ver;

	int irq;
	u32 saved_irq_state;
	raw_spinlock_t irq_lock;
	struct irq_domain *intx_domain;
	struct irq_domain *msi_domain;
	struct irq_domain *msi_bottom_domain;
	struct mtk_msi_set msi_sets[PCIE_MSI_SET_NUM];
	struct mutex lock;
	DECLARE_BITMAP(msi_irq_in_use, PCIE_MSI_IRQS_NUM);
};

static int mtk_pcie_config_read(struct pci_bus *bus, unsigned int devfn,
				    int where, int size, u32 *val)
{
	struct mtk_pcie_port *port = bus->sysdata;
	int bytes;

	bytes = ((1 << size) - 1) << (where & 0x3);
	writel(PCIE_CFG_HEADER_FORCE_BE(devfn, bus->number, bytes),
	       port->base + PCIE_CFGNUM_REG);

	*val = readl(port->base + PCIE_CFG_OFFSET_ADDR + (where & ~0x3));

	if (size <= 2)
		*val = (*val >> (8 * (where & 0x3))) & ((1 << (size * 8)) - 1);

	return PCIBIOS_SUCCESSFUL;
}

static int mtk_pcie_config_write(struct pci_bus *bus, unsigned int devfn,
				     int where, int size, u32 val)
{
	struct mtk_pcie_port *port = bus->sysdata;
	int bytes;

	bytes = ((1 << size) - 1) << (where & 0x3);
	writel(PCIE_CFG_HEADER_FORCE_BE(devfn, bus->number, bytes),
	       port->base + PCIE_CFGNUM_REG);

	if (size <= 2)
		val = (val & ((1 << (size * 8)) - 1)) << ((where & 0x3) * 8);

	writel(val, port->base + PCIE_CFG_OFFSET_ADDR + (where & ~0x3));

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops mtk_pcie_ops = {
	.read  = mtk_pcie_config_read,
	.write = mtk_pcie_config_write,
};

static void mtk_pcie_set_trans_window(void __iomem *reg,
				      resource_size_t cpu_addr,
				      resource_size_t pci_addr, size_t size)
{
	writel(lower_32_bits(cpu_addr) | ATR_SIZE(fls(size) - 1) | 1, reg);
	writel(upper_32_bits(cpu_addr), reg + PCIE_ATR_SRC_ADDR_MSB_OFFSET);
	writel(lower_32_bits(pci_addr), reg + PCIE_ATR_TRSL_ADDR_LSB_OFFSET);
	writel(upper_32_bits(pci_addr), reg + PCIE_ATR_TRSL_ADDR_MSB_OFFSET);
	writel(ATR_ID(0) | ATR_PARAM(0), reg + PCIE_ATR_TRSL_PARAM_OFFSET);
}

static int mtk_pcie_set_trans_table(void __iomem *reg,
				    resource_size_t cpu_addr,
				    resource_size_t pci_addr, size_t size,
				    unsigned int num)
{
	void __iomem *table_base;

	if (num > PCIE_MAX_TRANS_TABLES)
		return -ENODEV;

	table_base = reg + num * PCIE_ATR_TLB_SET_OFFSET;
	mtk_pcie_set_trans_window(table_base, cpu_addr, pci_addr, size);

	return 0;
}

static void mtk_pcie_pre_init_mt6890(struct mtk_pcie_port *port)
{
	u32 val = 0;
	void __iomem *phy_mode;

	phy_mode = ioremap(0x10005000, 0x1000);

	/* Set phy mode to RC for port 0 */
	val = readl(phy_mode + 0x600);
	val |= 1 << 14;
	writel(val, phy_mode + 0x600);

	iounmap(phy_mode);

	/* enable low power */
	val = readl(port->base + PCIE_LOW_POWER_CTRL_REG);
	val |= PCIE_DIS_LOWPWR_MASK;
	val &= ~PCIE_FORCE_DIS_LOWPWR;
	writel(val, port->base + PCIE_LOW_POWER_CTRL_REG);
}

static unsigned long mtk_pcie_vcore_smc(unsigned long id,
			 unsigned long arg0, unsigned long arg1)
{
	struct arm_smccc_res res;

	arm_smccc_smc(id, arg0, arg1, 0, 0, 0, 0, 0, &res);
	return res.a0;
}

static unsigned long mtk_pcie_vcore_550_millivolt(int port_num)
{
	return mtk_pcie_vcore_smc(MTK_SIP_PCIE_CONTROL,
			port_num, PCIE_VCORE_550_MILLIVOLT);
}

static unsigned long mtk_pcie_vcore_600_millivolt(int port_num)
{
	return mtk_pcie_vcore_smc(MTK_SIP_PCIE_CONTROL,
			port_num, PCIE_VCORE_600_MILLIVOLT);
}

static int mtk_pcie_set_link_speed(struct mtk_pcie_port *port)
{
	u32 val;
	int err;

	if ((port->max_link_speed < 1) || (port->port_num < 0))
		return -EINVAL;

	val = readl(port->base + PCIE_BASE_CONF_REG);
	val = (val & PCIE_SUPPORT_SPEED_MASK) >> PCIE_SUPPORT_SPEED_SHIFT;
	if (val & (1 << (port->max_link_speed - 1))) {
		val = readl(port->base + PCIE_SETTING_REG);
		val &= ~PCIE_GEN_SUPPORT_MASK;

		if (port->max_link_speed > 1)
			val |= PCIE_GEN_SUPPORT(port->max_link_speed);

		writel(val, port->base + PCIE_SETTING_REG);

		/* Set target speed */
		val = readl(port->base + PCIE_CONF_EXP_LNKCTL2_REG);
		val &= ~PCIE_TARGET_SPEED_MASK;
		writel(val | port->max_link_speed,
			port->base + PCIE_CONF_EXP_LNKCTL2_REG);

		/* set vcore 550mV for GEN2, set vcore 600mV for above GEN3 */
		if (port->max_link_speed <= 2) {
			err = mtk_pcie_vcore_550_millivolt(port->port_num);
			if (err)
				dev_info(port->dev, "vcore adjust 550mV fail\n");
		} else {
			err = mtk_pcie_vcore_600_millivolt(port->port_num);
			if (err)
				dev_info(port->dev, "vcore adjust 600mV fail\n");
		}

		return 0;
	}

	return -EINVAL;
}

static int mtk_pcie_get_chipid(struct mtk_pcie_port *port)
{
	struct device_node *node;
	struct tag_chipid *chip_id;
	int len;

	node = of_find_node_by_path("/chosen");
	if (!node)
		node = of_find_node_by_path("/chosen@0");

	if (node) {
		chip_id = (struct tag_chipid *)of_get_property(node,
							       "atag,chipid",
							       &len);
		if (!chip_id) {
			pr_info("could not found atag,chipid in chosen\n");
			return -ENODEV;
		}
	} else {
		pr_info("chosen node not found in device tree\n");
		return -ENODEV;
	}

	port->sw_ver = chip_id->sw_ver;
	dev_info(port->dev, "current sw version: %s\n",
		 port->sw_ver == CHIP_VER_E1 ? "E1" : "E2");

	return 0;
}

static void mtk_pcie_enable_msi(struct mtk_pcie_port *port)
{
	int i;
	u32 val;

	for (i = 0; i < PCIE_MSI_SET_NUM; i++) {
		struct mtk_msi_set *msi_set = &port->msi_sets[i];

		msi_set->base = port->base + PCIE_MSI_SET_BASE_REG +
				i * PCIE_MSI_SET_OFFSET;
		msi_set->msg_addr = port->reg_base + PCIE_MSI_SET_BASE_REG +
				    i * PCIE_MSI_SET_OFFSET;

		/* Configure the MSI capture address */
		writel(lower_32_bits(msi_set->msg_addr), msi_set->base);
		writel(upper_32_bits(msi_set->msg_addr),
			       port->base + PCIE_MSI_SET_ADDR_HI_BASE +
			       i * PCIE_MSI_SET_ADDR_HI_OFFSET);
	}

	val = readl(port->base + PCIE_MSI_SET_ENABLE_REG);
	val |= PCIE_MSI_SET_ENABLE;
	writel(val, port->base + PCIE_MSI_SET_ENABLE_REG);

	val = readl(port->base + PCIE_INT_ENABLE_REG);
	val |= PCIE_MSI_ENABLE;
	writel(val, port->base + PCIE_INT_ENABLE_REG);
}

static int mtk_pcie_startup_port(struct mtk_pcie_port *port)
{
	struct resource_entry *entry;
	struct pci_host_bridge *host = pci_host_bridge_from_priv(port);
	u32 val;
	int err = 0;
	unsigned int table_index = 0;

	/* high speed ethernet hook point */

	/* set as RC mode */
	val = readl(port->base + PCIE_SETTING_REG);
	val |= PCIE_RC_MODE;
	writel(val, port->base + PCIE_SETTING_REG);

	/* set class code */
	val = readl(port->base + PCIE_PCI_IDS_1);
	val &= ~GENMASK(31, 8);
	val |= PCI_CLASS(PCI_CLASS_BRIDGE_PCI << 8);
	writel(val, port->base + PCIE_PCI_IDS_1);

	mtk_pcie_pre_init_mt6890(port);

	err = mtk_pcie_set_link_speed(port);
	if (err)
		dev_info(port->dev, "unsupported speed: GEN%d\n",
			 port->max_link_speed);

	/* Assert all reset signals */
	val = readl(port->base + PCIE_RST_CTRL_REG);
	val |= PCIE_MAC_RSTB | PCIE_PHY_RSTB | PCIE_BRG_RSTB | PCIE_PE_RSTB;
	writel(val, port->base + PCIE_RST_CTRL_REG);

	/* De-assert reset signals*/
	val &= ~(PCIE_MAC_RSTB | PCIE_PHY_RSTB | PCIE_BRG_RSTB);
	writel(val, port->base + PCIE_RST_CTRL_REG);

	/* Delay 100ms to wait the reference clocks become stable */
	usleep_range(100 * 1000, 120 * 1000);

	/* De-assert pe reset*/
	val &= ~PCIE_PE_RSTB;
	writel(val, port->base + PCIE_RST_CTRL_REG);

	/* Check if the link is up or not */
	err = readl_poll_timeout(port->base + PCIE_LINK_STATUS_REG, val,
				 !!(val & PCIE_PORT_LINKUP), 20,
				 50 * USEC_PER_MSEC);
	if (err) {
		val = readl(port->base + PCIE_LTSSM_STATUS_REG);
		dev_err(port->dev, "PCIe link down, ltssm reg val: %#x\n", val);
		return err;
	}

	mtk_pcie_enable_msi(port);

	/* Set PCIe translation windows */
	resource_list_for_each_entry(entry, &host->windows) {
		unsigned long type = resource_type(entry->res);
		struct resource *res = NULL;
		resource_size_t cpu_addr;
		resource_size_t pci_addr;

		if (!(type & (IORESOURCE_MEM | IORESOURCE_IO)))
			continue;

		res = entry->res;
		cpu_addr = res->start;
		pci_addr = res->start - entry->offset;
		mtk_pcie_set_trans_table(port->base + PCIE_TRANS_TABLE_BASE_REG,
					 cpu_addr, pci_addr, resource_size(res),
					 table_index);

		dev_dbg(port->dev, "Set %s trans window[%d]: cpu_addr = %#llx, pci_addr = %#llx, size = %#llx\n",
			(!!(type & IORESOURCE_MEM) ? "MEM" : "IO"), table_index,
			cpu_addr, pci_addr, resource_size(res));
		table_index++;
	}

	return err;
}

static int mtk_pcie_set_affinity(struct irq_data *data,
				 const struct cpumask *mask, bool force)
{
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(data);
	int ret;

	ret = irq_set_affinity_hint(port->irq, mask);
	if (ret)
		return ret;

	irq_data_update_effective_affinity(data, mask);

	return 0;
}

static void mtk_pcie_msi_irq_mask(struct irq_data *data)
{
	pci_msi_mask_irq(data);
	irq_chip_mask_parent(data);
}

static void mtk_pcie_msi_irq_unmask(struct irq_data *data)
{
	pci_msi_unmask_irq(data);
	irq_chip_unmask_parent(data);
}

static struct irq_chip mtk_msi_irq_chip = {
	.irq_ack = irq_chip_ack_parent,
	.irq_mask = mtk_pcie_msi_irq_mask,
	.irq_unmask = mtk_pcie_msi_irq_unmask,
	.name = "MSI",
};

static struct msi_domain_info mtk_msi_domain_info = {
	.flags	= (MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS |
		   MSI_FLAG_PCI_MSIX | MSI_FLAG_MULTI_PCI_MSI),
	.chip	= &mtk_msi_irq_chip,
};

static void mtk_compose_msi_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct mtk_msi_set *msi_set = irq_data_get_irq_chip_data(data);
	struct mtk_pcie_port *port = data->domain->host_data;
	unsigned long hwirq;

	hwirq =	data->hwirq % PCIE_MSI_IRQS_PER_SET;

	msg->address_hi = upper_32_bits(msi_set->msg_addr);
	msg->address_lo = lower_32_bits(msi_set->msg_addr);
	msg->data = hwirq;
	dev_dbg(port->dev, "msi#%#lx address_hi %#x address_lo %#x data %d\n",
		hwirq, msg->address_hi, msg->address_lo, msg->data);
}

static void mtk_msi_bottom_irq_ack(struct irq_data *data)
{
	struct mtk_msi_set *msi_set = irq_data_get_irq_chip_data(data);
	unsigned long hwirq;

	hwirq =	data->hwirq % PCIE_MSI_IRQS_PER_SET;

	writel(BIT(hwirq), msi_set->base + PCIE_MSI_SET_STATUS_OFFSET);
}

static void mtk_msi_bottom_irq_mask(struct irq_data *data)
{
	struct mtk_msi_set *msi_set = irq_data_get_irq_chip_data(data);
	struct mtk_pcie_port *port = data->domain->host_data;
	unsigned long hwirq, flags;
	u32 val;

	hwirq =	data->hwirq % PCIE_MSI_IRQS_PER_SET;

	raw_spin_lock_irqsave(&port->irq_lock, flags);
	val = readl(msi_set->base + PCIE_MSI_SET_ENABLE_OFFSET);
	val &= ~BIT(hwirq);
	writel(val, msi_set->base + PCIE_MSI_SET_ENABLE_OFFSET);
	raw_spin_unlock_irqrestore(&port->irq_lock, flags);
}

static void mtk_msi_bottom_irq_unmask(struct irq_data *data)
{
	struct mtk_msi_set *msi_set = irq_data_get_irq_chip_data(data);
	struct mtk_pcie_port *port = data->domain->host_data;
	unsigned long hwirq, flags;
	u32 val;

	hwirq =	data->hwirq % PCIE_MSI_IRQS_PER_SET;

	raw_spin_lock_irqsave(&port->irq_lock, flags);
	val = readl(msi_set->base + PCIE_MSI_SET_ENABLE_OFFSET);
	val |= BIT(hwirq);
	writel(val, msi_set->base + PCIE_MSI_SET_ENABLE_OFFSET);
	raw_spin_unlock_irqrestore(&port->irq_lock, flags);
}

static struct irq_chip mtk_msi_bottom_irq_chip = {
	.irq_ack		= mtk_msi_bottom_irq_ack,
	.irq_mask		= mtk_msi_bottom_irq_mask,
	.irq_unmask		= mtk_msi_bottom_irq_unmask,
	.irq_compose_msi_msg	= mtk_compose_msi_msg,
	.irq_set_affinity	= mtk_pcie_set_affinity,
	.name			= "MSI",
};

static int mtk_msi_bottom_domain_alloc(struct irq_domain *domain,
				       unsigned int virq, unsigned int nr_irqs,
				       void *arg)
{
	struct mtk_pcie_port *port = domain->host_data;
	struct mtk_msi_set *msi_set;
	int i, hwirq, set_idx;

	mutex_lock(&port->lock);

	hwirq = bitmap_find_free_region(port->msi_irq_in_use, PCIE_MSI_IRQS_NUM,
					order_base_2(nr_irqs));

	mutex_unlock(&port->lock);

	if (hwirq < 0)
		return -ENOSPC;

	set_idx = hwirq / PCIE_MSI_IRQS_PER_SET;
	msi_set = &port->msi_sets[set_idx];

	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_info(domain, virq + i, hwirq + i,
				    &mtk_msi_bottom_irq_chip, msi_set,
				    handle_edge_irq, NULL, NULL);

	return 0;
}

static void mtk_msi_bottom_domain_free(struct irq_domain *domain,
				       unsigned int virq, unsigned int nr_irqs)
{
	struct mtk_pcie_port *port = domain->host_data;
	struct irq_data *data = irq_domain_get_irq_data(domain, virq);

	mutex_lock(&port->lock);

	bitmap_release_region(port->msi_irq_in_use, data->hwirq,
			      order_base_2(nr_irqs));

	mutex_unlock(&port->lock);

	irq_domain_free_irqs_common(domain, virq, nr_irqs);
}

static const struct irq_domain_ops mtk_msi_bottom_domain_ops = {
	.alloc = mtk_msi_bottom_domain_alloc,
	.free = mtk_msi_bottom_domain_free,
};

static void mtk_intx_mask(struct irq_data *data)
{
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(data);
	u32 val;

	val = readl(port->base + PCIE_INT_ENABLE_REG);
	val &= ~(1 << (data->hwirq + PCIE_INTX_SHIFT));
	writel(val, port->base + PCIE_INT_ENABLE_REG);
}

static void mtk_intx_unmask(struct irq_data *data)
{
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(data);
	u32 val;

	val = readl(port->base + PCIE_INT_ENABLE_REG);
	val |= 1 << (data->hwirq + PCIE_INTX_SHIFT);
	writel(val, port->base + PCIE_INT_ENABLE_REG);
}

static void mtk_intx_eoi(struct irq_data *data)
{
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(data);
	unsigned long hwirq;

	/**
	 * As an emulated level irq, its interrupt status will be remained
	 * until receive the corresponding message of de-assert, hence that
	 * the status can only be cleared when the interrupt has been serviced.
	 */
	hwirq = data->hwirq + PCIE_INTX_SHIFT;
	writel(1 << hwirq, port->base + PCIE_INT_STATUS_REG);
}

static struct irq_chip mtk_intx_irq_chip = {
	.irq_mask		= mtk_intx_mask,
	.irq_unmask		= mtk_intx_unmask,
	.irq_eoi		= mtk_intx_eoi,
	.irq_set_affinity	= mtk_pcie_set_affinity,
	.name			= "PCIe",
};

static int mtk_pcie_intx_map(struct irq_domain *domain, unsigned int irq,
			     irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler_name(irq, &mtk_intx_irq_chip,
				      handle_fasteoi_irq, "INTx");
	irq_set_chip_data(irq, domain->host_data);

	return 0;
}

static const struct irq_domain_ops intx_domain_ops = {
	.map = mtk_pcie_intx_map,
};

static int mtk_pcie_init_irq_domains(struct mtk_pcie_port *port,
				     struct device_node *node)
{
	struct device *dev = port->dev;
	struct device_node *intc_node;
	struct fwnode_handle *fwnode = of_node_to_fwnode(node);
	int ret;

	raw_spin_lock_init(&port->irq_lock);

	/* Setup INTx */
	intc_node = of_get_child_by_name(node, "legacy-interrupt-controller");
	if (!intc_node) {
		dev_notice(dev, "Missing PCIe Intc node\n");
		return -ENODEV;
	}

	port->intx_domain = irq_domain_add_linear(intc_node, PCI_NUM_INTX,
						  &intx_domain_ops, port);
	if (!port->intx_domain) {
		dev_notice(dev, "failed to get INTx IRQ domain\n");
		return -ENODEV;
	}

	/* Setup MSI */
	mutex_init(&port->lock);

	port->msi_bottom_domain = irq_domain_add_linear(node, PCIE_MSI_IRQS_NUM,
				  &mtk_msi_bottom_domain_ops, port);
	if (!port->msi_bottom_domain) {
		dev_err(dev, "failed to create MSI bottom domain\n");
		ret = -ENODEV;
		goto err_msi_bottom_domain;
	}

	port->msi_domain = pci_msi_create_irq_domain(fwnode,
						     &mtk_msi_domain_info,
						     port->msi_bottom_domain);
	if (!port->msi_domain) {
		dev_err(dev, "failed to create MSI domain\n");
		ret = -ENODEV;
		goto err_msi_domain;
	}

	return 0;

err_msi_domain:
	irq_domain_remove(port->msi_bottom_domain);
err_msi_bottom_domain:
	irq_domain_remove(port->intx_domain);

	return ret;
}

static void mtk_pcie_irq_teardown(struct mtk_pcie_port *port)
{
	irq_set_chained_handler_and_data(port->irq, NULL, NULL);

	if (port->intx_domain)
		irq_domain_remove(port->intx_domain);

	if (port->msi_domain)
		irq_domain_remove(port->msi_domain);

	if (port->msi_bottom_domain)
		irq_domain_remove(port->msi_bottom_domain);

	irq_dispose_mapping(port->irq);
}

static void mtk_pcie_msi_handler(struct mtk_pcie_port *port, int set_idx)
{
	struct mtk_msi_set *msi_set = &port->msi_sets[set_idx];
	unsigned long msi_enable, msi_status;
	unsigned int virq;
	irq_hw_number_t bit, hwirq;

	msi_enable = readl(msi_set->base + PCIE_MSI_SET_ENABLE_OFFSET);

	do {
		msi_status = readl(msi_set->base +
					   PCIE_MSI_SET_STATUS_OFFSET);
		msi_status &= msi_enable;
		if (!msi_status)
			break;

		for_each_set_bit(bit, &msi_status, PCIE_MSI_IRQS_PER_SET) {
			hwirq = bit + set_idx * PCIE_MSI_IRQS_PER_SET;
			virq = irq_find_mapping(port->msi_bottom_domain, hwirq);
			generic_handle_irq(virq);
		}
	} while (true);
}

static void mtk_pcie_irq_handler(struct irq_desc *desc)
{
	struct mtk_pcie_port *port = irq_desc_get_handler_data(desc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	unsigned long status;
	unsigned int virq;
	irq_hw_number_t irq_bit = PCIE_INTX_SHIFT;

	chained_irq_enter(irqchip, desc);

	status = readl(port->base + PCIE_INT_STATUS_REG);
	for_each_set_bit_from(irq_bit, &status, PCI_NUM_INTX +
			      PCIE_INTX_SHIFT) {
		virq = irq_find_mapping(port->intx_domain,
					irq_bit - PCIE_INTX_SHIFT);
		generic_handle_irq(virq);
	}

	irq_bit = PCIE_MSI_SHIFT;
	for_each_set_bit_from(irq_bit, &status, PCIE_MSI_SET_NUM +
			      PCIE_MSI_SHIFT) {
		mtk_pcie_msi_handler(port, irq_bit - PCIE_MSI_SHIFT);

		writel(BIT(irq_bit), port->base + PCIE_INT_STATUS_REG);
	}

	chained_irq_exit(irqchip, desc);
}

static int mtk_pcie_setup_irq(struct mtk_pcie_port *port,
			      struct device_node *node)
{
	struct device *dev = port->dev;
	struct platform_device *pdev = to_platform_device(dev);
	int err;

	err = mtk_pcie_init_irq_domains(port, node);
	if (err) {
		dev_err(dev, "failed to init PCIe IRQ domain\n");
		return err;
	}

	port->irq = platform_get_irq(pdev, 0);
	if (port->irq < 0)
		return port->irq;

	irq_set_chained_handler_and_data(port->irq, mtk_pcie_irq_handler, port);

	return 0;
}

static int mtk_pcie_clk_init(struct mtk_pcie_port *port)
{
	struct device *dev = port->dev;
	struct device_node *np = dev->of_node;
	int i;

	port->num_clks = of_clk_get_parent_count(np);
	if (port->num_clks == 0) {
		dev_warn(dev, "pcie clock is not found\n");
		return 0;
	}

	port->clks = devm_kzalloc(dev, port->num_clks, GFP_KERNEL);
	if (!port->clks)
		return -ENOMEM;

	for (i = 0; i < port->num_clks; i++) {
		struct clk      *clk;
		int             ret;

		clk = of_clk_get(np, i);
		if (IS_ERR(clk)) {
			while (--i >= 0)
				clk_put(port->clks[i]);
			return PTR_ERR(clk);
		}

		ret = clk_prepare_enable(clk);
		if (ret < 0) {
			while (--i >= 0) {
				clk_disable_unprepare(port->clks[i]);
				clk_put(port->clks[i]);
			}
			clk_put(clk);

			return ret;
		}

		port->clks[i] = clk;
	}

	return 0;
}

static int mtk_pcie_disable_clk(struct mtk_pcie_port *port)
{
	int i;

	if (port->num_clks == 0)
		return 0;

	for (i = 0; i < port->num_clks; i++) {
		clk_disable_unprepare(port->clks[i]);
		clk_put(port->clks[i]);
	}
	port->num_clks = 0;

	return 0;
}

static int mtk_pcie_power_up(struct mtk_pcie_port *port)
{
	struct device *dev = port->dev;
	int err = 0;

	port->phy_reset = devm_reset_control_get_optional_exclusive(dev,
								    "phy-rst");
	if (IS_ERR(port->phy_reset))
		return PTR_ERR(port->phy_reset);

	/* phy power on and enable pipe clock */
	port->phy = devm_phy_optional_get(dev, "pcie-phy");
	if (IS_ERR(port->phy))
		return PTR_ERR(port->phy);

	reset_control_deassert(port->phy_reset);

	if (port->phy != NULL) {
		if (port->port_num >= 0)
			port->phy->id = port->port_num;

		err = phy_set_mode(port->phy, PHY_MODE_PCIE_RC);
		if (err) {
			dev_notice(dev, "failed to set phy mode\n");
			goto err_phy_mode;
		}

		err = phy_power_on(port->phy);
		if (err) {
			dev_notice(dev, "failed to power on pcie phy\n");
			goto err_phy_on;
		}

		err = phy_init(port->phy);
		if (err)
			dev_notice(dev, "failed to initialize phy impedance select, follow the default\n");
	}

	port->mac_reset = devm_reset_control_get_optional_exclusive(dev,
								    "mac-rst");
	if (IS_ERR(port->mac_reset)) {
		err = PTR_ERR(port->mac_reset);
		goto err_mac_rst;
	}

	reset_control_deassert(port->mac_reset);

	/* mac power on and enable transaction layer clocks */
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	err = mtk_pcie_clk_init(port);
	if (err) {
		dev_notice(dev, "clock init failed\n");
		goto err_clk_init;
	}

	return err;

err_clk_init:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	reset_control_assert(port->mac_reset);
err_mac_rst:
	phy_power_off(port->phy);
err_phy_mode:
err_phy_on:
	phy_exit(port->phy);
	reset_control_assert(port->phy_reset);

	return err;
}


static void mtk_pcie_mtcmos_fixup_mt6890(struct mtk_pcie_port *port, bool on)
{
	struct device *dev = port->dev;
	struct generic_pm_domain *pcie_pd;

	if (port->sw_ver != CHIP_VER_E1)
		return;

	dev_info(dev, "%s\n", __func__);

	if (dev->pm_domain) {
		// Configure the power domain as always on
		dev_info(dev, "%s\n", __func__);
		pcie_pd = pd_to_genpd(dev->pm_domain);
		if (on)
			pcie_pd->flags |= GENPD_FLAG_ALWAYS_ON;
		else
			pcie_pd->flags &= ~GENPD_FLAG_ALWAYS_ON;
	}
}

static void mtk_pcie_power_down(struct mtk_pcie_port *port)
{
	mtk_pcie_mtcmos_fixup_mt6890(port, false);
	phy_power_off(port->phy);
	phy_exit(port->phy);

	mtk_pcie_disable_clk(port);

	pm_runtime_put_sync(port->dev);
	pm_runtime_disable(port->dev);
}

static int mtk_pcie_setup(struct mtk_pcie_port *port)
{
	struct device *dev = port->dev;
	struct pci_host_bridge *host = pci_host_bridge_from_priv(port);
	struct platform_device *pdev = to_platform_device(dev);
	struct list_head *windows = &host->windows;
	struct resource *regs, *bus;
	int err;

	err = pci_parse_request_of_pci_ranges(dev, windows, &bus);
	if (err)
		return err;

	port->busnr = bus->start;

	regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pcie-mac");
	port->base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(port->base)) {
		dev_err(dev, "failed to map register base\n");
		return PTR_ERR(port->base);
	}

	port->reg_base = regs->start;

	port->max_link_speed = of_pci_get_max_link_speed(dev->of_node);
	if (port->max_link_speed > 0)
		dev_info(dev, "max speed to GEN%d\n", port->max_link_speed);

	port->port_num = of_get_pci_domain_nr(dev->of_node);
	if (port->port_num >= 0)
		dev_info(dev, "host bridge domain number %d\n", port->port_num);

	err = mtk_pcie_get_chipid(port);
	if (err) {
		dev_info(port->dev, "unknown chip version\n");
		port->sw_ver = CHIP_VER_E1;
	}

	mtk_pcie_mtcmos_fixup_mt6890(port, true);

	/* Don't touch the hardware registers before power up */
	err = mtk_pcie_power_up(port);
	if (err)
		return err;

	/* Try link up */
	err = mtk_pcie_startup_port(port);
	if (err) {
		dev_notice(dev, "PCIe link down\n");
		goto err_setup;
	}

	err = mtk_pcie_setup_irq(port, dev->of_node);
	if (err)
		goto err_setup;

	dev_info(dev, "PCIe link up success!\n");

	return 0;

err_setup:
	mtk_pcie_power_down(port);

	return err;
}

static int mtk_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_pcie_port *port;
	struct pci_host_bridge *host;
	int err;

	host = devm_pci_alloc_host_bridge(dev, sizeof(*port));
	if (!host)
		return -ENOMEM;

	port = pci_host_bridge_priv(host);

	port->dev = dev;
	platform_set_drvdata(pdev, port);

	err = mtk_pcie_setup(port);
	if (err)
		goto release_resource;

	host->busnr = port->busnr;
	host->dev.parent = port->dev;
	host->ops = &mtk_pcie_ops;
	host->map_irq = of_irq_parse_and_map_pci;
	host->swizzle_irq = pci_common_swizzle;
	host->sysdata = port;

	err = pci_host_probe(host);
	if (err)
		goto power_down;

	return 0;

power_down:
	mtk_pcie_power_down(port);
release_resource:
	pci_free_resource_list(&host->windows);

	return err;
}

static int mtk_pcie_remove(struct platform_device *pdev)
{
	struct mtk_pcie_port *port = platform_get_drvdata(pdev);
	struct pci_host_bridge *host = pci_host_bridge_from_priv(port);

	pci_lock_rescan_remove();
	pci_stop_root_bus(host->bus);
	pci_remove_root_bus(host->bus);
	pci_unlock_rescan_remove();

	mtk_pcie_irq_teardown(port);
	mtk_pcie_power_down(port);

	return 0;
}

static void mtk_pcie_suspend_noirq_fixup_mt6890(struct mtk_pcie_port *port)
{
	void __iomem *mtcmos;
	u32 val, offset;

	if (port->sw_ver != CHIP_VER_E1)
		return;

	dev_info(port->dev, "%s\n", __func__);

	if ((port->port_num < 0) || (port->port_num > 3)) {
		dev_notice(port->dev, "unknown port_num, workaround abort\n");
		return;
	}

	if (port->port_num < 3)
		offset = 4 * port->port_num;
	else
		offset = 4 * 2;

	mtcmos = ioremap(0x10006000, 0x1000);

	/* Reset MAC */
	val = readl(mtcmos + 0x330 + offset);
	val &= ~BIT(0);
	writel(val, mtcmos + 0x330 + offset);
	val = readl(mtcmos + 0x330 + offset);
	dev_info(port->dev, "MAC MTCMOS val = %#x\n", val);

	/* PHY power down */
	val = readl(mtcmos + 0x30c + offset);
	val |= BIT(1);
	writel(val, mtcmos + 0x30c + offset);
	val |= BIT(4);
	writel(val, mtcmos + 0x30c + offset);
	val &= ~BIT(0);
	writel(val, mtcmos + 0x30c + offset);
	val &= ~BIT(2);
	writel(val, mtcmos + 0x30c + offset);
	val &= ~BIT(3);
	writel(val, mtcmos + 0x30c + offset);

	val = readl(mtcmos + 0x30c + offset);
	dev_info(port->dev, "PHY MTCMOS val = %#x\n", val);

	iounmap(mtcmos);
}

static void mtk_pcie_resume_noirq_fixup_mt6890(struct mtk_pcie_port *port)
{
	void __iomem *mtcmos;
	u32 val, offset;

	if (port->sw_ver != CHIP_VER_E1)
		return;

	dev_info(port->dev, "%s\n", __func__);

	if ((port->port_num < 0) || (port->port_num > 3)) {
		dev_notice(port->dev, "unknown port_num, workaround abort\n");
		return;
	}

	if (port->port_num < 3)
		offset = 4 * port->port_num;
	else
		offset = 4 * 2;

	mtcmos = ioremap(0x10006000, 0x1000);

	/* PHY power up */
	val = readl(mtcmos + 0x30c + offset);
	val |= BIT(2);
	writel(val, mtcmos + 0x30c + offset);
	val |= BIT(3);
	writel(val, mtcmos + 0x30c + offset);
	val &= ~BIT(4);
	writel(val, mtcmos + 0x30c + offset);
	val &= ~BIT(1);
	writel(val, mtcmos + 0x30c + offset);
	val |= BIT(0);
	writel(val, mtcmos + 0x30c + offset);

	val = readl(mtcmos + 0x30c + offset);
	dev_info(port->dev, "PHY MTCMOS val = %#x\n", val);

	/* Release MAC */
	val = readl(mtcmos + 0x330 + offset);
	val |= BIT(0);
	writel(val, mtcmos + 0x330 + offset);
	dev_info(port->dev, "MAC MTCMOS val = %#x\n", val);

	iounmap(mtcmos);
}

static void __maybe_unused mtk_pcie_irq_save(struct mtk_pcie_port *port)
{
	int i;

	raw_spin_lock(&port->irq_lock);

	port->saved_irq_state = readl(port->base + PCIE_INT_ENABLE_REG);

	for (i = 0; i < PCIE_MSI_SET_NUM; i++) {
		struct mtk_msi_set *msi_set = &port->msi_sets[i];

		msi_set->saved_irq_state = readl(msi_set->base +
						 PCIE_MSI_SET_ENABLE_OFFSET);
	}

	raw_spin_unlock(&port->irq_lock);
}

static void __maybe_unused mtk_pcie_irq_restore(struct mtk_pcie_port *port)
{
	int i;

	raw_spin_lock(&port->irq_lock);

	writel(port->saved_irq_state, port->base + PCIE_INT_ENABLE_REG);

	for (i = 0; i < PCIE_MSI_SET_NUM; i++) {
		struct mtk_msi_set *msi_set = &port->msi_sets[i];

		writel(msi_set->saved_irq_state,
		       msi_set->base + PCIE_MSI_SET_ENABLE_OFFSET);
	}

	raw_spin_unlock(&port->irq_lock);
}

static int __maybe_unused mtk_pcie_turn_off_link(struct mtk_pcie_port *port)
{
	u32 val;

	val = readl(port->base + PCIE_ICMD_PM_REG);
	val |= PCIE_TURN_OFF_LINK;
	writel(val, port->base + PCIE_ICMD_PM_REG);

	/* Check the link is L2 */
	return readl_poll_timeout(port->base + PCIE_LTSSM_STATUS_REG, val,
				  (PCIE_LTSSM_STATE(val) ==
				   PCIE_LTSSM_STATE_L2_IDLE), 20,
				   50 * USEC_PER_MSEC);
}

static int __maybe_unused mtk_pcie_suspend_noirq(struct device *dev)
{
	struct mtk_pcie_port *port = dev_get_drvdata(dev);
	int i, err;

	if (port->is_suspended)
		return 0;

	mtk_pcie_irq_save(port);

	/* Trigger link to L2 state */
	err = mtk_pcie_turn_off_link(port);
	if (err) {
		dev_notice(port->dev, "can not enter L2 state\n");
		goto power_off;
	}

	/* Wait Harrier enter L2 state */
	usleep_range(10 * 1000, 20 * 1000);

	dev_info(port->dev, "enter L2 state success");

power_off:
	phy_power_off(port->phy);

	for (i = 0; i < port->num_clks; i++)
		clk_disable_unprepare(port->clks[i]);

	mtk_pcie_suspend_noirq_fixup_mt6890(port);

	port->is_suspended = true;

	return 0;
}

static int __maybe_unused mtk_pcie_resume_noirq(struct device *dev)
{
	struct mtk_pcie_port *port = dev_get_drvdata(dev);
	int i, err;

	if (!port->is_suspended)
		return 0;

	mtk_pcie_resume_noirq_fixup_mt6890(port);

	phy_power_on(port->phy);

	for (i = 0; i < port->num_clks; i++) {
		err = clk_prepare_enable(port->clks[i]);
		if (err < 0) {
			while (--i >= 0)
				clk_disable_unprepare(port->clks[i]);
			return err;
		}
	}

	err = mtk_pcie_startup_port(port);
	if (err) {
		dev_notice(port->dev, "resume failed\n");
		return err;
	}

	port->is_suspended = false;

	mtk_pcie_irq_restore(port);

	dev_info(port->dev, "resume done\n");

	return 0;
}

static const struct dev_pm_ops mtk_pcie_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(mtk_pcie_suspend_noirq,
				      mtk_pcie_resume_noirq)
};

static const struct of_device_id mtk_pcie_of_match[] = {
	{ .compatible = "mediatek,gen3-pcie" },
	{ .compatible = "mediatek,mt6880-pcie" },
	{ .compatible = "mediatek,mt6890-pcie" },
	{},
};

static struct platform_driver mtk_pcie_driver = {
	.probe = mtk_pcie_probe,
	.remove = mtk_pcie_remove,
	.driver = {
		.name = "mtk-pcie",
		.of_match_table = mtk_pcie_of_match,
		.pm = &mtk_pcie_pm_ops,
	},
};

module_platform_driver(mtk_pcie_driver);
MODULE_LICENSE("GPL v2");
