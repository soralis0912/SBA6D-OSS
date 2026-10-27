#ifndef __QL_PHY_RW_EN8801S_H
#define __QL_PHY_RW_EN8801S_H

/* NAMING DECLARATIONS
 */
#define EN8801S_DRIVER_VERSION  "1.1.9_Generic"
#define EN8801S_PBUS_DEFAULT_ID 0x1e
#define EN8801S_RG_ETHER_PHY_OUI 0x19a4
#define EN8801S_RG_SMI_ADDR      0x19a8
#define EN8801S_RG_BUCK_CTL      0x1a20
#define EN8801S_RG_LTR_CTL       0x0cf8

#define EN8801S_PBUS_MDIO_ID 	0x1a750000
#define EN8801S_PBUS_OUI        0x17a5
#define EN8801S_PHY_ID1         0x03a2
#define EN8801S_PHY_ID2         0x9461
#define EN8801S_PHY_ID          \
			(unsigned long)((EN8801S_PHY_ID1 << 16) |\
							 EN8801S_PHY_ID2)
#define MAX_RETRY               5
#define MAX_OUI_CHECK           2
/* CL45 MDIO control */
#define MII_MMD_ACC_CTL_REG     0x0d
#define MII_MMD_ADDR_DATA_REG   0x0e
#define MMD_OP_MODE_DATA        BIT(14)

#define MAX_TRG_COUNTER         5

/* CL22 Reg Support Page Select */
#define RgAddr_Reg1Fh        0x1f
#define CL22_Page_Reg        0x0000
#define CL22_Page_ExtReg     0x0001
#define CL22_Page_MiscReg    0x0002
#define CL22_Page_LpiReg     0x0003
#define CL22_Page_tReg       0x02A3
#define CL22_Page_TrReg      0x52B5

/* CL45 Reg Support DEVID */
#define DEVID_03             0x03
#define DEVID_07             0x07
#define DEVID_1E             0x1E
#define DEVID_1F             0x1F

/* TokenRing Reg Access */
#define TrReg_PKT_XMT_STA    0x8000
#define TrReg_WR             0x8000
#define TrReg_RD             0xA000

#define RgAddr_LPI_1Ch       0x1c
#define RgAddr_AUXILIARY_1Dh 0x1d
#define RgAddr_PMA_00h       0x0f80
#define RgAddr_PMA_01h       0x0f82
#define RgAddr_PMA_17h       0x0fae
#define RgAddr_PMA_18h       0x0fb0
#define RgAddr_DSPF_03h      0x1686
#define RgAddr_DSPF_06h      0x168c
#define RgAddr_DSPF_08h      0x1690
#define RgAddr_DSPF_0Ch      0x1698
#define RgAddr_DSPF_0Dh      0x169a
#define RgAddr_DSPF_0Fh      0x169e
#define RgAddr_DSPF_10h      0x16a0
#define RgAddr_DSPF_11h      0x16a2
#define RgAddr_DSPF_13h      0x16a6
#define RgAddr_DSPF_14h      0x16a8
#define RgAddr_DSPF_1Bh      0x16b6
#define RgAddr_DSPF_1Ch      0x16b8
#define RgAddr_TR_26h        0x0ecc
#define RgAddr_R1000DEC_15h  0x03aa
#define RgAddr_R1000DEC_17h  0x03ae

/* Invalid data */
#define INVALID_DATA            0xffffffff

#define EN8801_MDIO_MAP_PBUS(mdio_addr, pbus_addr) \
do { \
	pbus_addr = mdio_addr + 1; \
}while(0);

#endif /* End of __QL_PHY_RW_EN8801S_H */