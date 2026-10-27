// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_PCIE_H__
#define __MTK_PCIE_H__

/* PCIe Register Map */
#define PCIE_DEVICE_ID		0x98
#define PCIE_CLASS_CODE		0x9c
#define PCIE_LTSSM_STATUS	0x150
#define PCIE_LTSSM_MASK		GENMASK(28, 24)
#define PCIE_LTSSM_L0		0x10
#define PCIE_LTSSM_L0S		0x11
#define PCIE_LTSSM_L1_IDLE	0x13
#define PCIE_LTSSM_L2_IDLE	0x14
#define PCIE_LTSSM_HOTRESET	0x1a

#define PCIE_LINK_STATUS	0x154
#define PCIE_DATA_LINK_UP	BIT(8)

#define PCIE_DIPC_HOST_FASTBOOT 0x3

#define PCIE_DIPC_STAGE_LB	0
#define PCIE_DIPC_STAGE_HB	2
#define PCIE_DIPC_BROM_LB	4
#define PCIE_DIPC_BROM_HB	6
#define PCIE_DIPC_LK_LB		8
#define PCIE_DIPC_LK_HB		10
#define PCIE_DIPC_LINUX_LB	12
#define PCIE_DIPC_LINUX_HB	15
#define PCIE_DIPC_PORT_LB	16
#define PCIE_DIPC_PORT_HB	27
#define PCIE_DIPC_HOST_LB	28
#define PCIE_DIPC_HOST_HB	31

struct mtk_pcie_ep;

struct pcie_dipc_ops {
	int (*set_data)(struct mtk_pcie_ep *info, u32 data);
	int (*set_port_data)(struct mtk_pcie_ep *info, u32 data);
	int (*set_event_data)(struct mtk_pcie_ep *info, u32 data);
	int (*set_stage_data)(struct mtk_pcie_ep *info, u32 data);
	int (*irq_to_host)(struct mtk_pcie_ep *info);
	int (*irq_clear)(struct mtk_pcie_ep *info);
	int (*get_ltssm_status)(struct mtk_pcie_ep *info);
	int (*get_link_status)(struct mtk_pcie_ep *info);
};

struct mtk_pcie_ep {
	void __iomem *pcie_reg;
	struct device *dev;
	struct device *class_dev;
	struct class *ep_class;
	struct phy *phy;
	struct cdev cdev;	/* char device for dipc */
	struct cdev ep_cdev;	/* char device for pcie ep */
	struct pcie_dipc_ops *ops;
	bool destroyed;		/*char devices is destroyed */
	int irq_channel;	/* mhccif irq channel */
	int irq;
	int count;
};

struct pcie_ccci_msg {
	char cmd;
	char res0;
	char res1;
	char res2;
};

/* LK information */
#define LK_NORMAL	0x00
#define LK_POST_DUMP	0x01
#define LK_REBOOT	0x07
#define LK_EVENT_MASK	0x07

/* Linux information */
#define LINUX_NORMAL	0x00
#define LINUX_PCIE_MODE	0x01	/* PCIe only mode or PCIe Advance mode */
#define LINUX_DIPC_MODE	0x02
#define LINUX_REBOOT	0x0f

/* BROM information */
#define BROM_NORMAL	0x00
#define BROM_JUMP_BL	0x01
#define BROM_TIMEOUT	0x02
#define BROM_JUMP_DA	0x03
#define BROM_DL_PORT	0x04	/* Create DL port */
#define BROM_REBOOT	0x07

/* Host information */
#define HOST_NULL		0x00	/* Not defined */
#define HOST_BROM_REBOOT	0x01	/* Ready for BROM reboot */
#define HOST_LK_REBOOT		0x02	/* Ready for LK reboot */

/* Device stage information */
#define DEV_INITIALIZE		0x00
#define DEV_BROM_DL_MODE	0x01
#define DEV_BROM_DL_ACT		0x02
#define DEV_LK_BRANCH		0x03
#define DEV_LINUX_BRANCH	0x04
#define DEV_PL_DA		0x05	/* Reserved */

#define PCIE_DIPC_REG		0xd1c

#define DECLARE_CHECK_LINK_STATE(state) \
static inline bool PCIE_LINK_IS_##state(struct mtk_pcie_ep *info) \
{ \
	return info->ops->get_ltssm_status(info) == PCIE_LTSSM_##state ? 1 : 0; \
}

DECLARE_CHECK_LINK_STATE(L0);
DECLARE_CHECK_LINK_STATE(L0S);
DECLARE_CHECK_LINK_STATE(L1_IDLE);
DECLARE_CHECK_LINK_STATE(L2_IDLE);

#define pcie_link_is_l0(info)	PCIE_LINK_IS_L0(info)
#define pcie_link_is_l0s(info)	PCIE_LINK_IS_L0S(info)
#define pcie_link_is_l1(info)	PCIE_LINK_IS_L1_IDLE(info)
#define pcie_link_is_l2(info)	PCIE_LINK_IS_L2_IDLE(info)

static inline void pcie_dipc_write_port(struct mtk_pcie_ep *info, u32 port)
{
	info->ops->set_port_data(info, port);
	info->ops->set_stage_data(info, DEV_LINUX_BRANCH);
}

static inline void pcie_dipc_write_mode(struct mtk_pcie_ep *info, u32 mode)
{
	info->ops->set_event_data(info, mode);
	info->ops->set_stage_data(info, DEV_LINUX_BRANCH);
}

static inline void pcie_dipc_trigger_irq(struct mtk_pcie_ep *info)
{
	info->ops->irq_to_host(info);
}

static inline bool pcie_is_linkup(struct mtk_pcie_ep *info)
{

	return (pcie_link_is_l0(info) || pcie_link_is_l0s(info)
		|| pcie_link_is_l1(info));
}

void mtk_pcie_destroy_ep_cdev(struct mtk_pcie_ep *info);

#ifdef CONFIG_MTK_PCIE_DIPC
int mtk_pcie_create_cdev(struct mtk_pcie_ep *info);
void mtk_pcie_destroy_cdev(struct mtk_pcie_ep *info);
#else
static inline int mtk_pcie_create_cdev(struct mtk_pcie_ep *info)
{ return 0; }
static inline void mtk_pcie_destroy_cdev(struct mtk_pcie_ep *info) { }
#endif /* MTK_PCIE_DIPC */

int mtk_pcie_is_brom_download_mode(struct mtk_pcie_ep *info);

#endif
