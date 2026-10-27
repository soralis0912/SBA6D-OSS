/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_MHCCIF_H__
#define __MTK_MHCCIF_H__

/* Register map */
#define MHCCIF_SW_TCHNUM	0x0c
#define MHCCIF_INT_STS		0x10
#define MHCCIF_IRQ_ACK		0x14
#define MHCCIF_MASK_STS		0x20
#define MHCCIF_MASK_SET		0x30
#define MHCCIF_MASK_CLR		0x40
#define MHCCIF_IRQS_NUM		32

/* Host to Device interrupt */
#define MHCCIF_H2D_INT_PCIE_PM_SUSPEND_REQ_AP	11
#define MHCCIF_H2D_INT_PCIE_PM_RESUME_REQ_AP	12
#define MHCCIF_H2D_INT_DEVICE_RESET		13

/* Device to Host interrupt */
#define MHCCIF_D2H_INT_DIPC	5
#define MHCCIF_D2H_INT_PCIE_PM_SUSPEND_ACK_AP	13
#define MHCCIF_D2H_INT_PCIE_PM_RESUME_ACK_AP	14
#define MHCCIF_D2H_INT_ASYNC	15

#if defined(CONFIG_MTK_MHCCIF)
int mtk_mhccif_alloc_irq(int id);
int mtk_mhccif_irq_to_host(int channel);
#else
static inline int mtk_mhccif_alloc_irq(int id) { return -ENODEV; }
static inline int mtk_mhccif_irq_to_host(int channel) { return -ENODEV; }
#endif

#endif
