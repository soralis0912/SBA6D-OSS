/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018 MediaTek Inc.
*/
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/of_net.h>
#include <linux/u64_stats_sync.h>
#include <linux/refcount.h>

#define MTK_QDMA_PAGE_SIZE	2048
#define	MTK_MAX_RX_LENGTH	1536
#define MTK_TX_DMA_BUF_LEN	0xffff
#define MTK_DMA_SIZE		2048
#define MTK_NAPI_WEIGHT		256
#define MTK_MAC_COUNT		2
#define MTK_RX_ETH_HLEN		(VLAN_ETH_HLEN + VLAN_HLEN + ETH_FCS_LEN)
#define MTK_RX_HLEN		(NET_SKB_PAD + MTK_RX_ETH_HLEN + NET_IP_ALIGN)
#define MTK_DMA_DUMMY_DESC	0xffffffff
#define MTK_DEFAULT_MSG_ENABLE	(NETIF_MSG_DRV | \
				 NETIF_MSG_PROBE | \
				 NETIF_MSG_LINK | \
				 NETIF_MSG_TIMER | \
				 NETIF_MSG_IFDOWN | \
				 NETIF_MSG_IFUP | \
				 NETIF_MSG_RX_ERR | \
				 NETIF_MSG_TX_ERR)
#define MTK_HW_FEATURES		(NETIF_F_IP_CSUM | \
				 NETIF_F_RXCSUM | \
				 NETIF_F_SG | NETIF_F_TSO | \
				 NETIF_F_TSO6 | \
				 NETIF_F_IPV6_CSUM)
#define NEXT_RX_DESP_IDX(X, Y)	(((X) + 1) & ((Y) - 1))

#define MTK_MAX_RX_RING_NUM	4
#define MTK_HW_LRO_DMA_SIZE	8

#define	MTK_MAX_LRO_RX_LENGTH		(4096 * 3)
#define	MTK_MAX_LRO_IP_CNT		2
#define	MTK_HW_LRO_TIMER_UNIT		1	/* 20 us */
#define	MTK_HW_LRO_REFRESH_TIME		50000	/* 1 sec. */
#define	MTK_HW_LRO_AGG_TIME		10	/* 200us */
#define	MTK_HW_LRO_AGE_TIME		50	/* 1ms */
#define	MTK_HW_LRO_MAX_AGG_CNT		64
#define	MTK_HW_LRO_BW_THRE		3000
#define	MTK_HW_LRO_REPLACE_DELTA	1000
#define	MTK_HW_LRO_SDL_REMAIN_ROOM	1522

/* Frame Engine Global Reset Register */
#define MTK_RST_GL		0x04
#define RST_GL_PSE		BIT(0)

/* Frame Engine Interrupt Status Register */
#define MTK_INT_STATUS2		0x08
#define MTK_GDM1_AF		BIT(28)
#define MTK_GDM2_AF		BIT(29)

/* PDMA HW LRO Alter Flow Timer Register */
#define MTK_PDMA_LRO_ALT_REFRESH_TIMER	0x1c

/* Frame Engine Interrupt Grouping Register */
#define MTK_FE_INT_GRP		0x20

/* CDMP Ingress Control Register */
#define MTK_CDMQ_IG_CTRL	0x1400
#define MTK_CDMQ_STAG_EN	BIT(0)

/* CDMP Ingress Control Register */
#define MTK_CDMP_IG_CTRL       0x400

/* CDMP Exgress Control Register */
#define MTK_CDMP_EG_CTRL	0x404

/* GDM Exgress Control Register */
#define MTK_GDMA_FWD_CFG(x)	(0x500 + (x * 0x1000))
#define MTK_GDMA_ICS_EN		BIT(22)
#define MTK_GDMA_TCS_EN		BIT(21)
#define MTK_GDMA_UCS_EN		BIT(20)
#define MTK_GDMA_DROP_ALL       0x7777

/* Unicast Filter MAC Address Register - Low */
#define MTK_GDMA_MAC_ADRL(x)	(0x508 + (x * 0x1000))

/* Unicast Filter MAC Address Register - High */
#define MTK_GDMA_MAC_ADRH(x)	(0x50C + (x * 0x1000))

/* PDMA RX Base Pointer Register */
#define MTK_PRX_BASE_PTR0	0x900
#define MTK_PRX_BASE_PTR_CFG(x)	(MTK_PRX_BASE_PTR0 + (x * 0x10))

/* PDMA RX Maximum Count Register */
#define MTK_PRX_MAX_CNT0	0x904
#define MTK_PRX_MAX_CNT_CFG(x)	(MTK_PRX_MAX_CNT0 + (x * 0x10))

/* PDMA RX CPU Pointer Register */
#define MTK_PRX_CRX_IDX0	0x908
#define MTK_PRX_CRX_IDX_CFG(x)	(MTK_PRX_CRX_IDX0 + (x * 0x10))

/* PDMA HW LRO Control Registers */
#define MTK_PDMA_LRO_CTRL_DW0	0x980
#define MTK_LRO_EN			BIT(0)
#define MTK_L3_CKS_UPD_EN		BIT(7)
#define MTK_LRO_ALT_PKT_CNT_MODE	BIT(21)
#define MTK_LRO_RING_RELINQUISH_REQ	(0x7 << 26)
#define MTK_LRO_RING_RELINQUISH_DONE	(0x7 << 29)

#define MTK_PDMA_LRO_CTRL_DW1	0x984
#define MTK_PDMA_LRO_CTRL_DW2	0x988
#define MTK_PDMA_LRO_CTRL_DW3	0x98c
#define MTK_ADMA_MODE		BIT(15)
#define MTK_LRO_MIN_RXD_SDL	(MTK_HW_LRO_SDL_REMAIN_ROOM << 16)

/* PDMA Global Configuration Register */
#define MTK_PDMA_GLO_CFG	0xa04
#define MTK_MULTI_EN		BIT(10)

/* PDMA Reset Index Register */
#define MTK_PDMA_RST_IDX	0xa08
#define MTK_PST_DRX_IDX0	BIT(16)
#define MTK_PST_DRX_IDX_CFG(x)	(MTK_PST_DRX_IDX0 << (x))

/* PDMA Delay Interrupt Register */
#define MTK_PDMA_DELAY_INT		0xa0c
#define MTK_PDMA_DELAY_RX_EN		BIT(15)
#define MTK_PDMA_DELAY_RX_PINT		4
#define MTK_PDMA_DELAY_RX_PINT_SHIFT	8
#define MTK_PDMA_DELAY_RX_PTIME		4
#define MTK_PDMA_DELAY_RX_DELAY		\
	(MTK_PDMA_DELAY_RX_EN | MTK_PDMA_DELAY_RX_PTIME | \
	(MTK_PDMA_DELAY_RX_PINT << MTK_PDMA_DELAY_RX_PINT_SHIFT))

/* PDMA Interrupt Status Register */
#define MTK_PDMA_INT_STATUS	0xa20

/* PDMA Interrupt Mask Register */
#define MTK_PDMA_INT_MASK	0xa28

/* PDMA HW LRO Alter Flow Delta Register */
#define MTK_PDMA_LRO_ALT_SCORE_DELTA	0xa4c

/* PDMA Interrupt grouping registers */
#define MTK_PDMA_INT_GRP1	0xa50
#define MTK_PDMA_INT_GRP2	0xa54

/* PDMA HW LRO IP Setting Registers */
#define MTK_LRO_RX_RING0_DIP_DW0	0xb04
#define MTK_LRO_DIP_DW0_CFG(x)		(MTK_LRO_RX_RING0_DIP_DW0 + (x * 0x40))
#define MTK_RING_MYIP_VLD		BIT(9)

/* PDMA HW LRO Ring Control Registers */
#define MTK_LRO_RX_RING0_CTRL_DW1	0xb28
#define MTK_LRO_RX_RING0_CTRL_DW2	0xb2c
#define MTK_LRO_RX_RING0_CTRL_DW3	0xb30
#define MTK_LRO_CTRL_DW1_CFG(x)		(MTK_LRO_RX_RING0_CTRL_DW1 + (x * 0x40))
#define MTK_LRO_CTRL_DW2_CFG(x)		(MTK_LRO_RX_RING0_CTRL_DW2 + (x * 0x40))
#define MTK_LRO_CTRL_DW3_CFG(x)		(MTK_LRO_RX_RING0_CTRL_DW3 + (x * 0x40))
#define MTK_RING_AGE_TIME_L		((MTK_HW_LRO_AGE_TIME & 0x3ff) << 22)
#define MTK_RING_AGE_TIME_H		((MTK_HW_LRO_AGE_TIME >> 10) & 0x3f)
#define MTK_RING_AUTO_LERAN_MODE	(3 << 6)
#define MTK_RING_VLD			BIT(8)
#define MTK_RING_MAX_AGG_TIME		((MTK_HW_LRO_AGG_TIME & 0xffff) << 10)
#define MTK_RING_MAX_AGG_CNT_L		((MTK_HW_LRO_MAX_AGG_CNT & 0x3f) << 26)
#define MTK_RING_MAX_AGG_CNT_H		((MTK_HW_LRO_MAX_AGG_CNT >> 6) & 0x3)

/* QDMA TX Queue Configuration Registers */
#define MTK_QTX_CFG(x)		(0x1800 + (x * 0x10))
#define QDMA_RES_THRES		4

/* QDMA TX Queue Scheduler Registers */
#define MTK_QTX_SCH(x)		(0x1804 + (x * 0x10))

/* QDMA RX Base Pointer Register */
#define MTK_QRX_BASE_PTR0	0x1900

/* QDMA RX Maximum Count Register */
#define MTK_QRX_MAX_CNT0	0x1904

/* QDMA RX CPU Pointer Register */
#define MTK_QRX_CRX_IDX0	0x1908

/* QDMA RX DMA Pointer Register */
#define MTK_QRX_DRX_IDX0	0x190C

/* QDMA Global Configuration Register */
#define MTK_QDMA_GLO_CFG	0x1A04
#define MTK_RX_2B_OFFSET	BIT(31)
#define MTK_RX_BT_32DWORDS	(3 << 11)
#define MTK_NDP_CO_PRO		BIT(10)
#define MTK_TX_WB_DDONE		BIT(6)
#define MTK_DMA_SIZE_16DWORDS	(2 << 4)
#define MTK_CHK_DDONE_EN	BIT(28)
#define MTK_DMAD_WR_WDONE	BIT(26)
#define MTK_WCOMP_EN		BIT(24)
#define MTK_MULTI_DMA		BIT(8)
#define MTK_RX_DMA_BUSY		BIT(3)
#define MTK_TX_DMA_BUSY		BIT(1)
#define MTK_RX_DMA_EN		BIT(2)
#define MTK_TX_DMA_EN		BIT(0)
#define MTK_DMA_BUSY_TIMEOUT	HZ
#define MTK_DESC_32B_E             (1 << 8)
/* QDMA Reset Index Register */
#define MTK_QDMA_RST_IDX	0x1A08

/* QDMA Delay Interrupt Register */
#define MTK_QDMA_DELAY_INT	0x1A0C

/* QDMA Flow Control Register */
#define MTK_QDMA_FC_THRES	0x1A10
#define FC_THRES_DROP_MODE	BIT(20)
#define FC_THRES_DROP_EN	(7 << 16)
#define FC_THRES_MIN		0x4444

/* QDMA Interrupt Status Register */
#define MTK_QMTK_INT_STATUS	0x1A18
#define MTK_RX_DONE_DLY		BIT(30)
#define MTK_RX_DONE_INT3	BIT(19)
#define MTK_RX_DONE_INT2	BIT(18)
#define MTK_RX_DONE_INT1	BIT(17)
#define MTK_RX_DONE_INT0	BIT(16)
#define MTK_TX_DONE_INT3	BIT(3)
#define MTK_TX_DONE_INT2	BIT(2)
#define MTK_TX_DONE_INT1	BIT(1)
#define MTK_TX_DONE_INT0	BIT(0)
#define MTK_RX_DONE_INT		MTK_RX_DONE_DLY
#define MTK_TX_DONE_DLY         BIT(28)
#define MTK_TX_DONE_INT         MTK_TX_DONE_DLY

/* QDMA Interrupt grouping registers */
#define MTK_QDMA_INT_GRP1	0x1a20
#define MTK_QDMA_INT_GRP2	0x1a24
#define MTK_RLS_DONE_INT	BIT(0)

/* QDMA Interrupt Status Register */
#define MTK_QDMA_INT_MASK	0x1A1C

/* QDMA Interrupt Mask Register */
#define MTK_QDMA_HRED2		0x1A44

/* QDMA TX Forward CPU Pointer Register */
#define MTK_QTX_CTX_PTR		0x1B00

/* QDMA TX Forward DMA Pointer Register */
#define MTK_QTX_DTX_PTR		0x1B04

/* QDMA TX Release CPU Pointer Register */
#define MTK_QTX_CRX_PTR		0x1B10

/* QDMA TX Release DMA Pointer Register */
#define MTK_QTX_DRX_PTR		0x1B14

/* QDMA FQ Head Pointer Register */
#define MTK_QDMA_FQ_HEAD	0x1B20

/* QDMA FQ Head Pointer Register */
#define MTK_QDMA_FQ_TAIL	0x1B24

/* QDMA FQ Free Page Counter Register */
#define MTK_QDMA_FQ_CNT		0x1B28

/* QDMA FQ Free Page Buffer Length Register */
#define MTK_QDMA_FQ_BLEN	0x1B2C

/* GMA1 Received Good Byte Count Register */
#define MTK_GDM1_TX_GBCNT	0x2400
#define MTK_STAT_OFFSET		0x40

/* QDMA TX NUM */
#define MTK_QDMA_TX_NUM		16
#define MTK_QDMA_TX_MASK	((MTK_QDMA_TX_NUM) - 1)
#define QID_LOW_BITS(x)		((x) & 0xf)
#define QID_HIGH_BITS(x)	((((x) >> 4) & 0x3) << 20)

/* QDMA descriptor txd3 */
#define TX_DMA_OWNER_CPU	BIT(31)
#define TX_DMA_LS0		BIT(30)


/* QDMA DMAD descriptor rxd2 */
#define RX_DMA_DONE		BIT(31)
#define RX_DMA_PLEN0(_x)	(((_x) & 0x3fff) << 16)
#define RX_DMA_GET_PLEN0(_x)	(((_x) >> 16) & 0x3fff)

/* QDMA DMAD descriptor rxd3 */
#define RX_DMA_VID(_x)		((_x) & 0xfff)

/* QDMA DMAD descriptor rxd4 */
#define RX_DMA_L4_VALID		BIT(24)
#define RX_DMA_FPORT_SHIFT	19
#define RX_DMA_FPORT_MASK	0x7


/* QDMA DMAD descriptor txd5 */
#define TX_DMA_CHKSUM		(0x7 << 28)
#define TX_DMA_TSO		BIT(31)
/* QDMA DMAD descriptor txd4 */
#define TX_DMA_BURST		BIT(31)
#define TX_DMA_SWC		BIT(30)
#define TX_DMA_FPORT_SHIFT	8
#define TX_DMA_FPORT_MASK	0xf
#define TX_DMA_PLEN0(_x)	(((_x) & MTK_TX_DMA_BUF_LEN) << 8)
#define TX_DMA_SDL(_x)		(((_x) & 0xffff) << 8)

/* PHY Indirect Access Control registers */
#define MTK_PHY_IAC		0x10004
#define PHY_IAC_ACCESS		BIT(31)
#define PHY_IAC_READ		BIT(19)
#define PHY_IAC_WRITE		BIT(18)
#define PHY_IAC_START		BIT(16)
#define PHY_IAC_ADDR_SHIFT	20
#define PHY_IAC_REG_SHIFT	25
#define PHY_IAC_TIMEOUT		HZ

#define MTK_MAC_MISC		0x1000c
#define MTK_MUX_TO_ESW		BIT(0)

/* Mac control registers */
#define MTK_MAC_MCR(x)		(0x10100 + (x * 0x100))
#define MAC_MCR_MAX_RX_1536	BIT(24)
#define MAC_MCR_IPG_CFG		(BIT(18) | BIT(16))
#define MAC_MCR_FORCE_MODE	BIT(15)
#define MAC_MCR_TX_EN		BIT(14)
#define MAC_MCR_RX_EN		BIT(13)
#define MAC_MCR_BACKOFF_EN	BIT(9)
#define MAC_MCR_BACKPR_EN	BIT(8)
#define MAC_MCR_MDIO_EEE_1000T  BIT(7)
#define MAC_MCR_MDIO_EEE_100TX  BIT(6)
#define MAC_MCR_FORCE_RX_FC	BIT(5)
#define MAC_MCR_FORCE_TX_FC	BIT(4)
#define MAC_MCR_SPEED_1000	BIT(3)
#define MAC_MCR_SPEED_100	BIT(2)
#define MAC_MCR_FORCE_DPX	BIT(1)
#define MAC_MCR_FORCE_LINK	BIT(0)
#define MAC_MCR_FIXED_LINK	(MAC_MCR_MAX_RX_1536 | MAC_MCR_IPG_CFG | \
				 MAC_MCR_FORCE_MODE | MAC_MCR_TX_EN | \
				 MAC_MCR_RX_EN | MAC_MCR_BACKOFF_EN | \
				 MAC_MCR_BACKPR_EN | MAC_MCR_FORCE_RX_FC | \
				 MAC_MCR_FORCE_TX_FC | MAC_MCR_SPEED_1000 | \
				 MAC_MCR_FORCE_DPX | MAC_MCR_FORCE_LINK)

/* TRGMII RXC control register */
#define TRGMII_RCK_CTRL		0x10300
#define DQSI0(x)		((x << 0) & GENMASK(6, 0))
#define DQSI1(x)		((x << 8) & GENMASK(14, 8))
#define RXCTL_DMWTLAT(x)	((x << 16) & GENMASK(18, 16))
#define RXC_DQSISEL		BIT(30)
#define RCK_CTRL_RGMII_1000	(RXC_DQSISEL | RXCTL_DMWTLAT(2) | DQSI1(16))
#define RCK_CTRL_RGMII_10_100	RXCTL_DMWTLAT(2)

/* TRGMII RXC control register */
#define TRGMII_TCK_CTRL		0x10340
#define TXCTL_DMWTLAT(x)	((x << 16) & GENMASK(18, 16))
#define TXC_INV			BIT(30)
#define TCK_CTRL_RGMII_1000	TXCTL_DMWTLAT(2)
#define TCK_CTRL_RGMII_10_100	(TXC_INV | TXCTL_DMWTLAT(2))

/* TRGMII Interface mode register */
#define INTF_MODE		0x10390
#define TRGMII_INTF_DIS		BIT(0)
#define TRGMII_MODE		BIT(1)
#define TRGMII_CENTRAL_ALIGNED	BIT(2)
#define INTF_MODE_RGMII_1000    (TRGMII_MODE | TRGMII_CENTRAL_ALIGNED)
#define INTF_MODE_RGMII_10_100  0

/* GPIO port control registers for GMAC 2*/
#define GPIO_OD33_CTRL8		0x4c0
#define GPIO_BIAS_CTRL		0xed0
#define GPIO_DRV_SEL10		0xf00

/* ethernet subsystem chip id register */
#define ETHSYS_CHIPID0_3	0x0
#define ETHSYS_CHIPID4_7	0x4
#define MT7623_ETH		7623
#define MT7622_ETH		7622

/* ethernet system control register */
#define ETHSYS_SYSCFG		0x10
#define SYSCFG_DRAM_TYPE_DDR2	BIT(4)

/* ethernet subsystem config register */
#define ETHSYS_SYSCFG0		0x14
#define SYSCFG0_GE_MASK		0x3
#define SYSCFG0_GE_MODE(x, y)	(x << (12 + (y * 2)))
#define SYSCFG0_SGMII_MASK     GENMASK(9, 8)
#define SYSCFG0_SGMII_GMAC1    ((2 << 8) & SYSCFG0_SGMII_MASK)
#define SYSCFG0_SGMII_GMAC2    ((3 << 8) & SYSCFG0_SGMII_MASK)
#define SYSCFG0_SGMII_GMAC1_V2 BIT(9)
#define SYSCFG0_SGMII_GMAC2_V2 BIT(8)


/* ethernet subsystem clock register */
#define ETHSYS_CLKCFG0		0x2c
#define ETHSYS_TRGMII_CLK_SEL362_5	BIT(11)

/* ethernet reset control register */
#define ETHSYS_RSTCTRL		0x34
#define RSTCTRL_FE		BIT(6)
#define RSTCTRL_PPE		BIT(31)

/* SGMII subsystem config registers */
/* Register to auto-negotiation restart */
#define SGMSYS_PCS_CONTROL_1	0x0
#define SGMII_AN_RESTART	BIT(9)

/* Register to programmable link timer, the unit in 2 * 8ns */
#define SGMSYS_PCS_LINK_TIMER	0x18
#define SGMII_LINK_TIMER_DEFAULT	(0x186a0 & GENMASK(19, 0))

/* Register to control remote fault */
#define SGMSYS_SGMII_MODE	0x20
#define SGMII_RESERVED 		0x24
#define SGMII_REMOTE_FAULT_DIS	BIT(8)

/* Register to power up QPHY */
#define SGMSYS_QPHY_PWR_STATE_CTRL 0xe8
#define	SGMII_PHYA_PWD		BIT(4)

/* Register to select SGMII-0 for SNPS MAC */
#define SGMSYS_UTIF_CTRL	0xf4
#define SGMII_MAC_SEL	BIT(24)

#define syspll_prediv 			0x720
#define RG_sgmii_pcs_sel		BIT(15)
#define DA_SSUSB_CDR_REFCK_SEL		0xc00
#define RG_SSUSB_XTAL_EXT_EN_PE1H	GENMASK(13, 12)
#define RG_SSUSB_XTAL_EXT_EN_PE2H	GENMASK(17, 16)
#define B2_PHYD_TOP1			0xa00
#define RG_ssusb_force_clktx_en		BIT(10)
#define RG_ssusb_force_tx_eidle_lp_en	BIT(16)
#define SSUSB_PLL_FBKDIV_PE1H		0xc28
#define RG_SSUSB_PLL_FBKDIV_PE1H	GENMASK(31, 0)
#define SSUSB_PLL_FBKDIV_PE1D		0xc2c
#define RG_SSUSB_PLL_FBKDIV_PE1D	GENMASK(31, 0)
#define SSUSB_PLL_FBKDIV_PE2H		0xc30
#define RG_SSUSB_PLL_FBKDIV_PE2H	GENMASK(31, 0)
#define SSUSB_PLL_FBKDIV_PE2D		0xc34
#define RG_SSUSB_PLL_FBKDIV_PE2D	GENMASK(31, 0)
#define DA_SSUSB_PLL_SSC_DELTA1		0xc38
#define RG_SSUSB_PLL_SSC_DELTA1_PE1H	GENMASK(31, 16)
#define	DA_SSUSB_PLL_SSC_DELTA1_3C	0xc3c
#define RG_SSUSB_PLL_SSC_DELTA1_PE1D	GENMASK(15, 0)
#define RG_SSUSB_PLL_SSC_DELTA1_PE2H	GENMASK(31, 16)
#define DA_SSUSB_PLL_SSC_DELTA1_40	0xc40
#define RG_SSUSB_PLL_SSC_DELTA1_PE2D	GENMASK(15, 0)
#define DA_SSUSB_PLL_SSC_DELTA1_44	0xc44
#define RG_SSUSB_PLL_SSC_DELTA_PE1H	GENMASK(15, 0)
#define RG_SSUSB_PLL_SSC_DELTA_PE1D	GENMASK(31, 16)
#define DA_SSUSB_PLL_SSC_DELTA1_48	0xc48
#define RG_SSUSB_PLL_SSC_DELTA_PE2H	GENMASK(15, 0)
#define RG_SSUSB_PLL_SSC_DELTA_PE2D	GENMASK(31, 16)
#define SSUSB_PHYA_EQ3			0x97c
#define RG_SSUSB_EQ_DLEQ_LFI_GEN1	GENMASK(27, 24)
#define RG_SSUSB_EQ_DLEQ_LFI_GEN2	GENMASK(31, 28)
#define	DA_PCIE_MODE			0xc04
#define RG_PCIE_MODE_U3			BIT(0)
#define RG_PCIE_MODE_PE1H		BIT(1)
#define RG_PCIE_MODE_PE1D		BIT(2)
#define RG_PCIE_SPEED_U3		BIT(20)
#define RG_PCIE_SPEED_PE1H		BIT(21)
#define RG_PCIE_SPEED_PE1D		BIT(22)
#define DA_SSUSB_EQ_RSTEP2		0xc64
#define RG_SSUSB_TX_DEMG1_EN_PE1H	BIT(28)
#define RG_SSUSB_TX_DEMG1_EN_PE1D	BIT(29)
#define RG_SSUSB_TX_DEMG1_EN_PE2H	BIT(30)
#define RG_SSUSB_TX_DEMG1_EN_PE2D	BIT(31)
#define DA_SSUSB_DIGCLK_DEMUX		0xc80
#define RG_SSUSB_RX_DEMUX_REP_PE1H	BIT(0)
#define RG_SSUSB_RX_DEMUX_REP_PE1D	BIT(2)
#define RG_SSUSB_RX_DEMUX_REP_PE2H	BIT(4)
#define RG_SSUSB_RX_DEMUX_REP_PE2D	BIT(6)
#define RG_SSUSB_CDR_PICLK_DIV_PE1H	GENMASK(12, 11)
#define RG_SSUSB_CDR_PICLK_DIV_PE1D	GENMASK(15, 14)
#define RG_SSUSB_CDR_PICLK_DIV_PE2H	GENMASK(18, 17)
#define RG_SSUSB_CDR_PICLK_DIV_PE2D	GENMASK(21, 20)
#define RG_SSUSB_CDR_DIGCLK_DIV_PE1H	GENMASK(27, 26)
#define RG_SSUSB_CDR_DIGCLK_DIV_PE1D	GENMASK(30, 29)
#define DA_SSUSB_DIGCLK_IPATH		0xc84
#define RG_SSUSB_CDR_DIGCLK_DIV_PE2H	GENMASK(1, 0)
#define RG_SSUSB_CDR_DIGCLK_DIV_PE2D	GENMASK(4, 3)
#define DA_SSUSB_PHYD_MIX2		0x928
#define RG_ssusb_g1_cdr_ppath_dvn_ltr	GENMASK(4, 0)
#define RG_ssusb_g1_cdr_ppath_dvn_ltd0	GENMASK(10, 6)
#define B2_PHYD_TOP8 			0xa78
#define RG_ssusb_g1_cdr_ppath_dvn_ltd1	GENMASK(16, 12)
#define RG_ssusb_g2_cdr_ppath_dvn_ltr	GENMASK(22, 18)
#define RG_ssusb_g2_cdr_ppath_dvn_ltd0	GENMASK(28, 24)
#define RG_ssusb_g2_cdr_ppath_dvn_ltd1	GENMASK(4, 0)
#define RG_SSUSB_CDR_IIR_GAIN_PE1H	GENMASK(7, 4)
#define B2_PHYD_TOP9 			0xa7c
#define RG_SSUSB_CDR_IIR_GAIN_PE1D	GENMASK(11, 8)
#define DA_SSUSB_CDR_IIR_GAIN		0xc88
#define RG_SSUSB_CDR_IIR_GAIN_PE2H      GENMASK(15, 12)
#define RG_SSUSB_CDR_IIR_GAIN_PE2D      GENMASK(19, 16)
#define RG_SSUSB_CDR_IIR_CORNER_PE1H    GENMASK(25, 23)
#define RG_SSUSB_CDR_IIR_CORNER_PE1D    GENMASK(28, 26)
#define RG_SSUSB_CDR_IIR_CORNER_PE2H    GENMASK(31, 29)
#define RG_SSUSB_CDR_IIR_CORNER_PE2D    GENMASK(2, 0)
#define RG_SSUSB_TX_EIDLE_CM            GENMASK(31, 28)
#define DA_SSUSB_DESSC_IIR		0xc8c
#define RG_ssusb_idrvsel                GENMASK(19, 15)
#define DA_SSUB_B20			0xb20
#define RG_ssusb_force_idrvsel          BIT(14)
#define DA_SSUSB_PHYD_MIX6		0x9e8
#define RG_ssusb_idemsel                GENMASK(25, 21)
#define RG_ssusb_force_idemsel          BIT(20]
#define RG_ssusb_cdr_chg_lo_dly         GENMASK(23, 20)
#define RG_ssusb_cdr_chg_hi_dly         GENMASK(30, 26)
#define DA_SSUSB_PHYD_MIX4		0x948
#define RG_ssusb_cdros_cnt              GENMASK(29, 24)
#define EQ_DFE2				0x994
#define RG_SSUSB_EQ_LEQOSC_DLYCNT       GENMASK(18, 16)
#define DA_SSUSB_PHYD_MIX3		0x938
#define RG_ssusb_force_pll_sscen        BIT(15)
#define DA_SSUSB_PHYD_EBUF_CTL		0x940
#define RG_ssusb_rxtermination          BIT(30)
#define DA_SSUSB PHYD_PLL_CTL		0x960
#define RG_ssusb_force_pcie_sigdet_out  BIT(13)
#define RG_c60802_gpio_ctle             BIT(24)
#define A60802_CHIP_RSV			0x810
#define RG_ssusb_frc_pipe_powerdown	BIT(11)
#define DA_SSUSB_PHYD_PIPE_CTL0		0x940
#define RG_ssusb_rate                   BIT(6)
#define RG_ssusb_force_rate        	BIT(5)
#define RG_ssusb_force_pcie_sigdet_out  BIT(13)
#define DA_SSUSB_PHYD_CP_PATTERN	0x934
#define RG_ssusb_cppat_prbs_en          BIT(20)
/* Infrasys subsystem config registers*/
#define INFRA_MISC2            0x70c
#define CO_QPHY_SEL            BIT(0)
#define GEPHY_MAC_SEL          BIT(1)

/*MDIO control*/
#define MII_MMD_ACC_CTL_REG             0x0d
#define MII_MMD_ADDR_DATA_REG           0x0e
#define MMD_OP_MODE_DATA BIT(14)


/* struct mtk_hw_stats - the structure that holds the traffic statistics.
 * @stats_lock:		make sure that stats operations are atomic
 * @reg_offset:		the status register offset of the SoC
 * @syncp:		the refcount
 *
 * All of the supported SoCs have hardware counters for traffic statistics.
 * Whenever the status IRQ triggers we can read the latest stats from these
 * counters and store them in this struct.
 */
struct mtk_hw_stats {
	u64 tx_bytes;
	u64 tx_packets;
	u64 tx_skip;
	u64 tx_collisions;
	u64 rx_bytes;
	u64 rx_packets;
	u64 rx_overflow;
	u64 rx_fcs_errors;
	u64 rx_short_errors;
	u64 rx_long_errors;
	u64 rx_checksum_errors;
	u64 rx_flow_control_packets;

	spinlock_t		stats_lock;
	u32			reg_offset;
	struct u64_stats_sync	syncp;
};

enum mtk_tx_flags {
	/* PDMA descriptor can point at 1-2 segments. This enum allows us to
	 * track how memory was allocated so that it can be freed properly.
	 */
	MTK_TX_FLAGS_SINGLE0	= 0x01,
	MTK_TX_FLAGS_PAGE0	= 0x02,

	/* MTK_TX_FLAGS_FPORTx allows tracking which port the transmitted
	 * SKB out instead of looking up through hardware TX descriptor.
	 */
	MTK_TX_FLAGS_FPORT0	= 0x04,
	MTK_TX_FLAGS_FPORT1	= 0x08,
};

/* This enum allows us to identify how the clock is defined on the array of the
 * clock in the order
 */
enum mtk_clks_map {
	MTK_CLK_ETHIF,
	MTK_CLK_SGMIITOP,
	MTK_CLK_ESW,
	MTK_CLK_GP0,
	MTK_CLK_GP1,
	MTK_CLK_GP2,
	MTK_CLK_FE,
	MTK_CLK_TRGPLL,
	MTK_CLK_SGMII_TX_250M,
	MTK_CLK_SGMII_RX_250M,
	MTK_CLK_SGMII_CDR_REF,
	MTK_CLK_SGMII_CDR_FB,
	MTK_CLK_SGMII2_TX_250M,
	MTK_CLK_SGMII2_RX_250M,
	MTK_CLK_SGMII2_CDR_REF,
	MTK_CLK_SGMII2_CDR_FB,
	MTK_CLK_SGMII_CK,
	MTK_CLK_ETH2PLL,
	MTK_CLK_MAX
};

#define MT7623_CLKS_BITMAP	(BIT(MTK_CLK_ETHIF) | BIT(MTK_CLK_ESW) |  \
				 BIT(MTK_CLK_GP1) | BIT(MTK_CLK_GP2) | \
				 BIT(MTK_CLK_TRGPLL))
#define MT7622_CLKS_BITMAP	(BIT(MTK_CLK_ETHIF) | BIT(MTK_CLK_ESW) |  \
				 BIT(MTK_CLK_GP0) | BIT(MTK_CLK_GP1) | \
				 BIT(MTK_CLK_GP2) | \
				 BIT(MTK_CLK_SGMII_TX_250M) | \
				 BIT(MTK_CLK_SGMII_RX_250M) | \
				 BIT(MTK_CLK_SGMII_CDR_REF) | \
				 BIT(MTK_CLK_SGMII_CDR_FB) | \
				 BIT(MTK_CLK_SGMII_CK) | \
				 BIT(MTK_CLK_ETH2PLL))
#define LEOPARD_CLKS_BITMAP	(BIT(MTK_CLK_ETHIF) | BIT(MTK_CLK_ESW) |  \
				BIT(MTK_CLK_GP0) | BIT(MTK_CLK_GP1) | \
				BIT(MTK_CLK_GP2) | BIT(MTK_CLK_FE) | \
				BIT(MTK_CLK_SGMII_TX_250M) | \
				BIT(MTK_CLK_SGMII_RX_250M) | \
				BIT(MTK_CLK_SGMII_CDR_REF) | \
				BIT(MTK_CLK_SGMII_CDR_FB) | \
				BIT(MTK_CLK_SGMII2_TX_250M) | \
				BIT(MTK_CLK_SGMII2_RX_250M) | \
				BIT(MTK_CLK_SGMII2_CDR_REF) | \
				BIT(MTK_CLK_SGMII2_CDR_FB) | \
				BIT(MTK_CLK_SGMII_CK) | \
				BIT(MTK_CLK_ETH2PLL) | BIT(MTK_CLK_SGMIITOP))

enum mtk_eth_mux {
	MTK_ETH_MUX_GDM1_TO_GMAC1_ESW,
	MTK_ETH_MUX_GMAC2_GMAC0_TO_GEPHY,
	MTK_ETH_MUX_U3_GMAC2_TO_QPHY,
	MTK_ETH_MUX_GMAC1_GMAC2_TO_SGMII_RGMII,
	MTK_ETH_MUX_GMAC12_TO_GEPHY_SGMII,
	MTK_ETH_MUX_MAX,
};

enum mtk_eth_path {
	MTK_ETH_PATH_GMAC1_RGMII,
	MTK_ETH_PATH_GMAC1_TRGMII,
	MTK_ETH_PATH_GMAC1_SGMII,
	MTK_ETH_PATH_GMAC2_RGMII,
	MTK_ETH_PATH_GMAC2_SGMII,
	MTK_ETH_PATH_GMAC2_GEPHY,
	MTK_ETH_PATH_GDM1_ESW,
	MTK_ETH_PATH_MAX,
};

/* Capability for function group */
#define MTK_RGMII			BIT(0)
#define MTK_TRGMII			BIT(1)
#define MTK_SGMII			BIT(2)
#define MTK_SGMII_PHY			BIT(3)
#define MTK_ESW				BIT(4)
#define MTK_GEPHY			BIT(5)
#define MTK_MUX				BIT(6)
#define MTK_INFRA			BIT(7)
#define MTK_SHARED_SGMII		BIT(8)
#define MTK_HWLRO			BIT(9)
#define MTK_SHARED_INT			BIT(10)
#define MTK_UNIFY_DMAD			BIT(11)
#define MTK_FPGA_CLK			BIT(12)

/* Capability for features on SoCs */
#define MTK_PATH_BIT(x)         BIT((x) + 20)

#define MTK_GMAC1_RGMII \
	(MTK_PATH_BIT(MTK_ETH_PATH_GMAC1_RGMII) | MTK_RGMII)

#define MTK_GMAC1_TRGMII \
	(MTK_PATH_BIT(MTK_ETH_PATH_GMAC1_TRGMII) | MTK_TRGMII)

#define MTK_GMAC1_SGMII \
	(MTK_PATH_BIT(MTK_ETH_PATH_GMAC1_SGMII) | MTK_SGMII)

#define MTK_GMAC2_RGMII \
	(MTK_PATH_BIT(MTK_ETH_PATH_GMAC2_RGMII) | MTK_RGMII)

#define MTK_GMAC2_SGMII \
	(MTK_PATH_BIT(MTK_ETH_PATH_GMAC2_SGMII) | MTK_SGMII)

#define MTK_GMAC2_GEPHY \
	(MTK_PATH_BIT(MTK_ETH_PATH_GMAC2_GEPHY) | MTK_GEPHY)

#define MTK_GDM1_ESW \
	(MTK_PATH_BIT(MTK_ETH_PATH_GDM1_ESW) | MTK_ESW)

#define MTK_MUX_BIT(x)          BIT((x) + 20)

/* Capability for MUXes present on SoCs */
/* 0: GDM1 -> GMAC1, 1: GDM1 -> ESW */
#define MTK_MUX_GDM1_TO_GMAC1_ESW       \
	(MTK_MUX_BIT(MTK_ETH_MUX_GDM1_TO_GMAC1_ESW) | MTK_MUX)

/* 0: GMAC2 -> GEPHY, 1: GMAC0 -> GePHY */
#define MTK_MUX_GMAC2_GMAC0_TO_GEPHY    \
	(MTK_MUX_BIT(MTK_ETH_MUX_GMAC2_GMAC0_TO_GEPHY) | MTK_MUX | MTK_INFRA)

/* 0: U3 -> QPHY, 1: GMAC2 -> QPHY */
#define MTK_MUX_U3_GMAC2_TO_QPHY        \
	(MTK_MUX_BIT(MTK_ETH_MUX_U3_GMAC2_TO_QPHY) | MTK_MUX | MTK_INFRA)

/* 2: GMAC1 -> SGMII, 3: GMAC2 -> SGMII */
#define MTK_MUX_GMAC1_GMAC2_TO_SGMII_RGMII	\
	(MTK_MUX_BIT(MTK_ETH_MUX_GMAC1_GMAC2_TO_SGMII_RGMII) | MTK_MUX | \
	 MTK_SHARED_SGMII)

/* 0: GMACx -> GEPHY, 1: GMACx -> SGMII where x is 1 or 2 */
#define MTK_MUX_GMAC12_TO_GEPHY_SGMII	\
	(MTK_MUX_BIT(MTK_ETH_MUX_GMAC12_TO_GEPHY_SGMII) | MTK_MUX)

#define MTK_HAS_CAPS(caps, _x)		(((caps) & (_x)) == (_x))

#define MT7622_CAPS  (MTK_GMAC1_RGMII | MTK_GMAC1_SGMII | MTK_GMAC2_RGMII | \
		      MTK_GMAC2_SGMII | MTK_GDM1_ESW | \
		      MTK_MUX_GDM1_TO_GMAC1_ESW | \
		      MTK_MUX_GMAC1_GMAC2_TO_SGMII_RGMII)

#define MT7623_CAPS  (MTK_GMAC1_RGMII | MTK_GMAC1_TRGMII | MTK_GMAC2_RGMII)

#define LEOPARD_CAPS  (MTK_GMAC1_SGMII | MTK_GMAC2_SGMII | MTK_GMAC2_GEPHY | \
		      MTK_GDM1_ESW | MTK_MUX_GDM1_TO_GMAC1_ESW | \
		      MTK_MUX_GMAC2_GMAC0_TO_GEPHY | \
		      MTK_MUX_U3_GMAC2_TO_QPHY | \
		      MTK_MUX_GMAC12_TO_GEPHY_SGMII)

#define COLGIN_FPGA_CAPS  (MTK_GMAC1_SGMII | MTK_GMAC2_SGMII | MTK_MUX_GMAC12_TO_GEPHY_SGMII | MTK_FPGA_CLK)

#define MT6890_CAPS  (MTK_GMAC1_SGMII | MTK_GMAC2_SGMII | \
		      MTK_MUX_GDM1_TO_GMAC1_ESW | \
		      MTK_MUX_GMAC2_GMAC0_TO_GEPHY | \
		      MTK_MUX_U3_GMAC2_TO_QPHY | \
		      MTK_SGMII_PHY)

/* currently no SoC has more than 2 macs */
#define MTK_MAX_DEVS			2

#define MTK_SGMII_PHYSPEED_AN          BIT(31)
#define MTK_SGMII_PHYSPEED_MASK        GENMASK(2, 0)
#define MTK_SGMII_PHYSPEED_1000        BIT(0)
#define MTK_SGMII_PHYSPEED_2500        BIT(1)
#define MTK_HAS_FLAGS(flags, _x)       (((flags) & (_x)) == (_x))

/* SGMII PLL register */
#define MT6990_XFI_PLL_OFFSET 0x008
#define MT6990_XFI_PLL_EN BIT(31)

/* struct mtk_sgmii -  This is the structure holding sgmii regmap and its
 *                     characteristics
 * @regmap:            The register map pointing at the range used to setup
 *                     SGMII modes
 * @flags:             The enum refers to which mode the sgmii wants to run on
 * @ana_rgc3:          The offset refers to register ANA_RGC3 related to regmap
 */

struct mtk_sgmii {
	struct regmap   *regmap[MTK_MAX_DEVS];
	struct regmap   *regmap_phy[MTK_MAX_DEVS];
	struct regmap	*remap_pll;
	u32             flags[MTK_MAX_DEVS];
	u32             ana_rgc3;
};

void mediatek_sgmii_phy_gen1(struct mtk_sgmii *ss, int id);
void mediatek_sgmii_phy_gen2(struct mtk_sgmii *ss, int id);
int mediatek_sgmii_setup_mode_an(struct mtk_sgmii *ss, int id);
int mediatek_sgmii_setup_mode_force(struct mtk_sgmii *ss, int id);
int mediatek_sgmii_path_setup(struct mtk_sgmii *ss, int mac_id);
int mediatek_sgmii_init(struct mtk_sgmii *ss, struct device_node *r, struct device *dev, u32 ana_rgc3);
