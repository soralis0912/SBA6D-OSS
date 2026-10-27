/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Chao Hao <chao.hao@mediatek.com>
 */

#ifndef _DTS_IOMMU_PORT_MT6880_H_
#define _DTS_IOMMU_PORT_MT6880_H_

#define MTK_M4U_ID(larb, port)		 (((larb) << 5) | (port))

/* Local arbiter ID */
#define MTK_M4U_TO_LARB(id)		 (((id) >> 5) & 0xf)
/* PortID within the local arbiter */
#define MTK_M4U_TO_PORT(id)		 ((id) & 0x1f)

#define MTK_IOMMU_LARB_NR		 (1)

/* larb0 */
#define M4U_LARB0_ID			 (0)
#define M4U_PORT_OVL_RDMA0		MTK_M4U_ID(M4U_LARB0_ID, 0)
#define M4U_PORT_DISP_RDMA0		MTK_M4U_ID(M4U_LARB0_ID, 1)
#define M4U_PORT_MDP_RDMA0		MTK_M4U_ID(M4U_LARB0_ID, 2)
#define M4U_PORT_DISP_WDMA0		MTK_M4U_ID(M4U_LARB0_ID, 3)
#define M4U_PORT_MDP_WROT0		MTK_M4U_ID(M4U_LARB0_ID, 4)
#define M4U_PORT_DISP_FAKE0		MTK_M4U_ID(M4U_LARB0_ID, 5)

#if 0
/* larb1 not used */
#define M4U_LARB1_ID			 (1)
#define M4U_PORT_OVL_2L_RDMA0_HDR	MTK_M4U_ID(M4U_LARB1_ID, 0)
#define M4U_PORT_OVL_2L_RDMA0		MTK_M4U_ID(M4U_LARB1_ID, 1)
#define M4U_PORT_DISP_RDMA0			MTK_M4U_ID(M4U_LARB1_ID, 2)
#define M4U_PORT_DISP_WDMA0			MTK_M4U_ID(M4U_LARB1_ID, 3)
#define M4U_PORT_DISP_FAKE1			MTK_M4U_ID(M4U_LARB1_ID, 4)
#endif

#define M4U_PORT_UNKNOWN			(M4U_PORT_DISP_FAKE0 + 1)

#endif
