#ifndef __QL_PHY_RW_YT8821_H
#define __QL_PHY_RW_YT8821_H
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

/* for wol feature */
#include <linux/netdevice.h>

#include "mtk_eth_soc.h"

#define YT_LINUX_MAJOR         	2
#define YT_LINUX_MINOR  		2
#define YT_LINUX_SUBVERSION 	21925
#define YT_LINUX_VERSIONID  	"2.2.21925"

/********************************************
 **** configuration section begin ***********/

/* if system depends on ethernet packet to restore from sleep,
 * please define this macro to 1 otherwise, define it to 0.
 */
#define SYS_WAKEUP_BASED_ON_ETH_PKT     1

/* to enable system WOL feature of phy, please define this macro to 1
 * otherwise, define it to 0.
 */
#define YTPHY_WOL_FEATURE_ENABLE        0

/* some GMAC need clock input from PHY, for eg., 125M,
 * please enable this macro
 * by degault, it is set to 0
 * NOTE: this macro will need macro SYS_WAKEUP_BASED_ON_ETH_PKT to set to 1
 */
#define GMAC_CLOCK_INPUT_NEEDED         0

/* the max number of yt8521 chip on pcb board
 * the most case is only 1 chip per board, but
 * by default, we support up to 8.
 */
#define YTPHY_BOARD_MAX_NUM_OF_CHIP_8521    8
#define YTPHY_BOARD_MAX_NUM_OF_CHIP_8614    4

/* for YT8531 package A xtal init config */
#define YTPHY8531A_XTAL_INIT                (0)

/**** configuration section end ***********
 ******************************************/

/* no need to change below */
#define MOTORCOMM_PHY_ID_MASK           0xffffffff

#define PHY_ID_YT8010                   0x00000309
#define PHY_ID_YT8010AS                 0x4f51eb19
#define PHY_ID_YT8011                   0x4f51eb01
#define PHY_ID_YT8510                   0x00000109
#define PHY_ID_YT8511                   0x0000010a
#define PHY_ID_YT8512                   0x00000128
#define PHY_ID_YT8522                   0x4f51e928
#define PHY_ID_YT8521                   0x0000011a
#define PHY_ID_YT8531S                  0x4f51e91a
#define PHY_ID_YT8531                   0x4f51e91b
#define PHY_ID_YT8614                   0x4f51e899
#define PHY_ID_YT8614Q                  0x4f51e8a9
#define PHY_ID_YT8618                   0x0000e889
#define PHY_ID_YT8821                   0x4f51ea19

#define REG_PHY_SPEC_STATUS             0x11
#define REG_DEBUG_ADDR_OFFSET           0x1e
#define REG_DEBUG_DATA                  0x1f
#define REG_MII_MMD_CTRL      			0x0D    /* MMD access control register */
#define REG_MII_MMD_DATA      			0x0E    /* MMD access data register    */

#define YT8512_EXTREG_LED0              0x40c0
#define YT8512_EXTREG_LED1              0x40c3

#define YT8512_EXTREG_SLEEP_CONTROL1    0x2027

#define YT_SOFTWARE_RESET               0x8000

#define YT8512_LED0_ACT_BLK_IND         0x1000
#define YT8512_LED0_DIS_LED_AN_TRY      0x0001
#define YT8512_LED0_BT_BLK_EN           0x0002
#define YT8512_LED0_HT_BLK_EN           0x0004
#define YT8512_LED0_COL_BLK_EN          0x0008
#define YT8512_LED0_BT_ON_EN            0x0010
#define YT8512_LED1_BT_ON_EN            0x0010
#define YT8512_LED1_TXACT_BLK_EN        0x0100
#define YT8512_LED1_RXACT_BLK_EN        0x0200
#define YT8512_SPEED_MODE               0xc000
#define YT8512_DUPLEX                   0x2000

#define YT8512_SPEED_MODE_BIT           14
#define YT8512_DUPLEX_BIT               13
#define YT8512_EN_SLEEP_SW_BIT          15

#define YT8522_TX_CLK_DELAY          	0x4210
#define YT8522_ANAGLOG_IF_CTRL       	0x4008
#define YT8522_DAC_CTRL              	0x2057
#define YT8522_INTERPOLATOR_FILTER_1 	0x14
#define YT8522_INTERPOLATOR_FILTER_2 	0x15
#define YT8522_EXTENDED_COMBO_CTRL_1 	0x4000

#define YT8521_EXTREG_SLEEP_CONTROL1    0x27
#define YT8521_EN_SLEEP_SW_BIT          15

#define YTXXXX_SPEED_MODE               0xc000
#define YTXXXX_DUPLEX                   0x2000
#define YTXXXX_SPEED_MODE_BIT           14
#define YTXXXX_DUPLEX_BIT               13
#define YTXXXX_LINK_STATUS_BIT          10

/* based on yt8521 wol feature config register */
#define YTPHY_UTP_INTR_REG              0x12
/* WOL Feature Event Interrupt Enable */
#define YTPHY_WOL_FEATURE_INTR          BIT(6)

/* Magic Packet MAC address registers */
#define YTPHY_WOL_FEATURE_MACADDR2_4_MAGIC_PACKET    0xa007
#define YTPHY_WOL_FEATURE_MACADDR1_4_MAGIC_PACKET    0xa008
#define YTPHY_WOL_FEATURE_MACADDR0_4_MAGIC_PACKET    0xa009

#define YTPHY_WOL_FEATURE_REG_CFG               0xa00a
#define YTPHY_WOL_FEATURE_TYPE_CFG              BIT(0)    /* WOL TYPE Config */
#define YTPHY_WOL_FEATURE_ENABLE_CFG            BIT(3)    /* WOL Enable Config */
#define YTPHY_WOL_FEATURE_INTR_SEL_CFG          BIT(6)    /* WOL Event Interrupt Enable Config */
#define YTPHY_WOL_FEATURE_WIDTH1_CFG            BIT(1)    /* WOL Pulse Width Config */
#define YTPHY_WOL_FEATURE_WIDTH2_CFG            BIT(2)    /* WOL Pulse Width Config */

#define YTPHY_REG_SPACE_UTP             0
#define YTPHY_REG_SPACE_FIBER           2

int ytphy_read_ext(struct mtk_eth *eth, int phy_addr, u32 regnum);
int ytphy_write_ext(struct mtk_eth *eth, int phy_addr, u32 regnum, u16 val);

#endif