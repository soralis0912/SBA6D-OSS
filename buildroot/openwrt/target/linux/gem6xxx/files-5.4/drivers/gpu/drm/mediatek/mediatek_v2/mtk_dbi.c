// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_panel.h>
#include <drm/drm_of.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/gpio.h>
#include <linux/iopoll.h>
#include <linux/irq.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <video/mipi_display.h>

#include "mtk_drm_ddp.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_crtc.h"
#include "mtk_dbi.h"

#include <linux/timer.h>
#include <linux/jiffies.h>

//#define ENABLE_UT   //Hardcode testing without LCM driver
//#define ENABLE_DBI_PATTERN  //DBI Self pattern

// Use to enable debug log, default should be turned off
//#define ENABLE_DEBUG_LOG

// Use to enable release data pin when transmission timeout,
// please adjust LOCKED_TIMEOUT according to your LCM drvier.
#define AUTO_RELEASE_DATA_PIN
#define LOCKED_TIMEOUT 50    /* milliseconds */

#ifdef ENABLE_DEBUG_LOG
#define dbi_dev_info(dev, fmt, ...)         \
    _dev_info(dev, dev_fmt(fmt), ##__VA_ARGS__)
#else
#define dbi_dev_info(dev, fmt, ...)         \
({                                          \
    if(0)                                   \
        _dev_info(dev, dev_fmt(fmt), ##__VA_ARGS__);    \
})
#endif

// NLI ARB address mapping
#define MT6890_NLI_ARB_CON_BASE 0x11f30000
#define MT6990_NLI_ARB_CON_BASE 0x11f50000

#define NLI_ARB_CON 0x014  //NLI_ARB_CON
// nf_bypass_mode: NLI_ARB_CON[28], MT6990 added
// favor_nli: NLI_ARB_CON[25]
// favor_dbi: NLI_ARB_CON[24]
#define NLI_ARB_CON_NFI_BYPASS_MASK BIT(28)
#define NLI_ARB_CON_NFI_BYPASS_SHIFT 28
#define NLI_ARB_CON_FAVOR_NLI_DBI_MASK BIT(24) | BIT(25) | BIT(28)
#define NLI_ARB_CON_FAVOR_DBI_SHIFT 24

#define NLI_ARB_SHAREPIN_CON 0x020

#define TOPCKGEN_BASE 0x10000000
// TOPCKGEN separate data pin
// 0: NFI_CLK
// 1: DBI_CLK
#define TOPCKGEN_DBI_CLK_SRC 0x0150
#define TOPCKGEN_DBI_CLK_SRC_MASK BIT(20)
#define TOPCKGEN_DBI_CLK_SRC_SHIFT 20

#define MMSYS_BASE 0x14000000
// MMSYS separate data pin
// 0: share nand Pin
// 1: separate DBI PIN
#define MMSYS_DBI_OUTPUT_DATA 0x0240
#define MMSYS_DBI_OUTPUT_DATA_MASK BIT(0)
#define MMSYS_DBI_OUTPUT_DATA_SHIFT 0

// GPIO address mapping
#define MT6890_GPIO_BASE 0x10005000
#define MT6990_GPIO_BASE 0x10004000

#define GPIO_MODE24 0x480
// 30:28	GPIO207	SRW	Aux mode of PAD_MSDC0_DAT3
//		000:B:GPIO207;001:B1:MSDC0_DAT3;010:B0:LPD3;
// 14:12	GPIO203	SRW	Aux mode of PAD_MSDC0_DAT4
//		000:B:GPIO203;001:B1:MSDC0_DAT4;010:B0:LPD4;
// 10:8	GPIO202	SRW	Aux mode of PAD_MSDC0_DAT5
//		000:B:GPIO202;001:B1:MSDC0_DAT5;010:B0:LPD5;
// 6:4	GPIO201	SRW	Aux mode of PAD_MSDC0_DAT6
//		000:B:GPIO201;001:B1:MSDC0_DAT6;010:B0:LPD6;
// 2:0	GPIO200	SRW	Aux mode of PAD_MSDC0_DAT7
//		000:B:GPIO200;001:B1:MSDC0_DAT7;010:B0:LPD7;
#define GPIO_MODE25 0x490
// 10:8	GPIO210	SRW	Aux mode of PAD_MSDC0_DAT0
//		000:B:GPIO210;001:B1:MSDC0_DAT0;010:B0:LPD0;
// 6:4	GPIO209	SRW	Aux mode of PAD_MSDC0_DAT1
//		000:B:GPIO209;001:B1:MSDC0_DAT1;010:B0:LPD1;
// 2:0	GPIO208	SRW	Aux mode of PAD_MSDC0_DAT2
//		000:B:GPIO208;001:B1:MSDC0_DAT2;010:B0:LPD2;
#define GPIO_MODE26 0x4A0
#define GPIO_MODE27 0x4B0

// Driving current address mapping
#define MT6890_IOCFG_TL_BASE 0x11f00000
#define MT6890_IOCFG_TL_DRV_CFG1_MASK 0x3FFC0

#define MT6990_IOCFG_RT8_BASE 0x11c00000
#define MT6990_IOCFG_RT8_DRV_CFG0_MASK 0x3FFFFFC0
#define MT6990_IOCFG_RT9_BASE 0x11c10000
#define MT6990_IOCFG_RT9_DRV_CFG0_MASK 0x3FFF8007
#define MT6990_IOCFG_RT9_DRV_CFG1_CTL_MASK 0xFC0000
#define MT6990_IOCFG_RT9_DRV_CFG1_MASK 0xFC01FF

#define IOCFG_TL_DRV_CFG0 0x00
#define IOCFG_TL_DRV_CFG1 0x10

// DBI registers
#define DBI_BASE        0x1400f000
#define DBI_START       0x0C
#define DBI_START_FLAG  BIT(15)
#define DBI_SRST_FLAG   BIT(0)      //Software reset

#define DBI_STA       0x00
#define DBI_STA_BUSY        BIT(4)

#define DBI_INTEN       0x04
#define DBI_INTSTA      0x08
#define DBI_INT_FLAG_CPL  BIT(0) //Enable frame complete interrupt
#define DBI_INT_FLAG_HTT  BIT(2) //Enable HTT calculation interrupt
#define DBI_INT_FLAG_SYNC BIT(3) //Enable TE sync interrupt
#define DBI_INT_FLAG_TE   BIT(4) //Enable TE detection interrupt
#define DBI_INT_FLAG_APB_TIMEOUT   BIT(5) //Enable LCD timeout interrupt

#define DBI_SIF_PIX_CON  0x18
#define SIF0_CS_STAY_LOW BIT(7)
#define SIF0_SINGLE_A0   BIT(6)
#define SIF0_PIX_2PIN    BIT(4)
#define SIF0_2PIN_SIZE   (BIT(0) | BIT(1) | BIT(2))

#define DBI_SIF_TIMING0 0x1c
#define CSS             0x00f00000
#define CSH             0x000f0000
#define RD1ST           0x0000f000
#define RD2ND           0x00000f00
#define WR1ST           0x000000f0
#define WR2ND           0x0000000f

#define DBI_SCNF        0x28
#define SIF_HW_CS       BIT(24)
#define SIF0_DIV2       BIT(7)
#define SIF0_SCK_DEF    BIT(6)
#define SIF0_1ST_POL    BIT(5)
#define SIF0_SDI        BIT(4)
#define SIF0_3WIRE      BIT(3)
#define SIF0_SIZE       BIT(0) | BIT(1) | BIT(2)

#define DBI_PCNF0       0x30
#define C2RH_SHIFT      28
#define C2RH            0xf0000000
#define C2RS_SHIFT      24
#define C2RS            0x0f000000
#define RLT_SHIFT       16
#define RLT             0x003f0000
#define C2WH_SHIFT      12
#define C2WH            0x0000f000
#define C2WS_SHIFT      8
#define C2WS            0x00000f00
#define WST             0x3f

#define DBI_PDW         0x3c
#define P0DW            0x7
#define P1DW            0x7 << 4
#define P2DW            0x7 << 8
#define P0CHW           0xf << 16
#define P0CHW_SHIFT     16

#define DBI_TECON       0x40
#define TE_ENABLE       BIT(0)
#define TE_EDGE         BIT(1)
#define TE_SYNC         BIT(2)
#define TE_REPEART      BIT(3)
#define SW_TE           BIT(15)

#define DBI_ROICON      0x60
#define COLOR_SEQ       BIT(0)
#define COLOR_FORMAT    (BIT(3) | BIT(4) | BIT(5)) //Bit[3:5] color format
#define ITF_SIZE        (BIT(6) | BIT(7))

#define DBI_ROI_CADD    0x64
#define CADD_ADDR       0xf0

#define DBI_ROI_DADD    0x68
#define DADD_ADDR       0xf0

#define DBI_ROI_SIZE    0x6c
#define ROI_SIZE_ROW    0x7ff << 16
#define ROI_SIZE_COL    0x7ff

#define DBI_PCMD0       0xf00
#define DBI_PDAT0       0xf10

#define DBI_SCMD0       0xf80
#define DBI_SDAT0       0xf90

#define DBI_ULTRA_CON   0x90
#define DBI_PATTERN     0xe8
#define DBI_RSTB        0x10

// ENUM
typedef enum {
	RETURN_OK = 0,
	RETURN_ERROR = -1,
} DBI_RETURN_VALUE;

typedef enum {
	LCM_DBI_DATA_INTERFACE_8BITS = 0,
	LCM_DBI_DATA_INTERFACE_9BITS = 1,
} LCM_DBI_DATA_INTERFACE;

struct mtk_dbi_data {
	enum mtk_mmsys_id mmsys_id;
};

struct mtk_dbi {
	const struct mtk_dbi_data *data;
	struct mtk_ddp_comp ddp_comp;
	struct device *dev;
	struct drm_encoder encoder;
	struct drm_connector conn;
	struct drm_panel *panel;
	struct drm_bridge *bridge;
	struct regmap *mmsys_sw_rst_b;
	struct pinctrl *gpio_pinctrl;
	u32 sw_rst_b;

	void __iomem *regs;
	void __iomem *gpio_base;
	void __iomem *iocfg_rt8_base;
	void __iomem *iocfg_rt9_base;
	void __iomem *nli_arb_base;
	void __iomem *ovl_base;
	void __iomem *mmsys_base;
	void __iomem *topckgen_base;

	struct clk *mm_clk;
	struct clk *mm_dbi_clk;
	struct clk *mm_dbi_sel;
	struct clk *infra_nfi_clk;
	struct clk *engine_clk;
	struct clk *parent_clk_26;
	struct clk *parent_clk_78;
	struct clk *parent_clk_91;
	struct clk *parent_clk_104;
	struct clk *parent_clk_125;
	struct clk *parent_clk_137;
	struct clk *parent_clk_156;
	struct clk *parent_clk_182;
	struct clk *mm_smi_clk;

	int refcount;
	bool enabled;
	int poweron;
	bool panel_enabled;
	u32 irq_data;
	wait_queue_head_t irq_wait_queue;

#ifdef AUTO_RELEASE_DATA_PIN
	struct timer_list locked_timer;
	bool locked;
#endif

	struct drm_display_mode mode;
	struct mipi_dbi_device host_info;
};

static struct drm_panel *_panel = NULL;
static bool _panel_attached = false;
static struct mipi_dbi_device _host_info;

static inline struct mtk_dbi *encoder_to_dbi(struct drm_encoder *e)
{
	return container_of(e, struct mtk_dbi, encoder);
}

static inline struct mtk_dbi *connector_to_dbi(struct drm_connector *c)
{
	return container_of(c, struct mtk_dbi, conn);
}

static void mtk_dbi_mask(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->regs + offset);

	writel((temp & ~mask) | (data & mask), dbi->regs + offset);
}

static void mtk_dbi_mask_arb(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->nli_arb_base + offset);

	writel((temp & ~mask) | (data & mask), dbi->nli_arb_base + offset);
}

static void mtk_dbi_mask_driving(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->iocfg_rt9_base + offset);

	writel((temp & ~mask) | (data & mask), dbi->iocfg_rt9_base + offset);
}

#ifdef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
static void mtk_dbi_mask_driving_msdc(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->iocfg_rt8_base + offset);

	writel((temp & ~mask) | (data & mask), dbi->iocfg_rt8_base + offset);
}

static void mtk_dbi_mask_gpio(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->gpio_base + offset);

	writel((temp & ~mask) | (data & mask), dbi->gpio_base + offset);
}

static void mtk_dbi_mask_topckgen(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->topckgen_base + offset);

	writel((temp & ~mask) | (data & mask), dbi->topckgen_base + offset);
}

static void mtk_dbi_mask_mmsys(struct mtk_dbi *dbi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dbi->mmsys_base + offset);

	writel((temp & ~mask) | (data & mask), dbi->mmsys_base + offset);
}
#endif

static void mtk_dbi_dump_regs(struct mtk_dbi *dbi)
{
	switch (dbi->data->mmsys_id) {
	case MMSYS_MT6890:
		dev_info(dbi->dev, "[%s] enter ctrl-type = %d", __func__, dbi->host_info.ctrl_type);
		// Dump arbiter
		dev_info(dbi->dev, "[arbiter]Address: 0x%08X, Value: 0x%08X",
				MT6890_NLI_ARB_CON_BASE + NLI_ARB_CON, readl(dbi->nli_arb_base + NLI_ARB_CON));
		dev_info(dbi->dev, "[arbiter]Address: 0x%08X, Value: 0x%08X",
				MT6890_NLI_ARB_CON_BASE + NLI_ARB_SHAREPIN_CON, readl(dbi->nli_arb_base + NLI_ARB_SHAREPIN_CON));

		// Dump GPIO
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6890_GPIO_BASE + GPIO_MODE25, readl(dbi->gpio_base + GPIO_MODE25));
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6890_GPIO_BASE + GPIO_MODE26, readl(dbi->gpio_base + GPIO_MODE26));
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6890_GPIO_BASE + GPIO_MODE27, readl(dbi->gpio_base + GPIO_MODE27));

		// Dump driving current
		//dev_info(dbi->dev, "[Driving]Address: 0x%08X, Value: 0x%08X",
		//		MT6890_IOCFG_TL_BASE + IOCFG_TL_DRV_CFG1,
		//		readl(dbi->iocfg_tl_base + IOCFG_TL_DRV_CFG1));
		break;
	case MMSYS_MT6990:
		dev_info(dbi->dev, "[%s] enter ctrl-type = %d", __func__, dbi->host_info.ctrl_type);
		// Dump arbiter
		dev_info(dbi->dev, "[arbiter]Address: 0x%08X, Value: 0x%08X",
				MT6990_NLI_ARB_CON_BASE + NLI_ARB_CON, readl(dbi->nli_arb_base + NLI_ARB_CON));
		dev_info(dbi->dev, "[arbiter]Address: 0x%08X, Value: 0x%08X",
				MT6990_NLI_ARB_CON_BASE + NLI_ARB_SHAREPIN_CON, readl(dbi->nli_arb_base + NLI_ARB_SHAREPIN_CON));

		// Dump GPIO
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6990_GPIO_BASE + GPIO_MODE24, readl(dbi->gpio_base + GPIO_MODE24));
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6890_GPIO_BASE + GPIO_MODE25, readl(dbi->gpio_base + GPIO_MODE25));
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6990_GPIO_BASE + GPIO_MODE26, readl(dbi->gpio_base + GPIO_MODE26));
		dev_info(dbi->dev, "[GPIO]Address: 0x%08X, Value: 0x%08X",
				MT6990_GPIO_BASE + GPIO_MODE27, readl(dbi->gpio_base + GPIO_MODE27));

		// Dump driving current
		dev_info(dbi->dev, "[Driving]Address: 0x%08X, Value: 0x%08X",
				MT6990_IOCFG_RT9_BASE + IOCFG_TL_DRV_CFG0,
				readl(dbi->iocfg_rt9_base + IOCFG_TL_DRV_CFG0));
		dev_info(dbi->dev, "[Driving]Address: 0x%08X, Value: 0x%08X",
				MT6990_IOCFG_RT9_BASE + IOCFG_TL_DRV_CFG1,
				readl(dbi->iocfg_rt9_base + IOCFG_TL_DRV_CFG1));
		dev_info(dbi->dev, "[Driving]Address: 0x%08X, Value: 0x%08X",
				MT6990_IOCFG_RT8_BASE + IOCFG_TL_DRV_CFG0,
				readl(dbi->iocfg_rt8_base + IOCFG_TL_DRV_CFG0));

	#ifdef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
		// Dump separate data registers
		dev_info(dbi->dev, "[TOPCKGEN_DBI_CLK_SRC]Address: 0x%08X, Value: 0x%08X",
				TOPCKGEN_BASE + TOPCKGEN_DBI_CLK_SRC,
				readl(dbi->topckgen_base + TOPCKGEN_DBI_CLK_SRC));
		dev_info(dbi->dev, "[MMSYS_DBI_OUTPUT_DATA]Address: 0x%08X, Value: 0x%08X",
				MMSYS_BASE + MMSYS_DBI_OUTPUT_DATA,
				readl(dbi->mmsys_base+ MMSYS_DBI_OUTPUT_DATA));
	#endif
		break;

	default:
		dev_info(dbi->dev, "[%s]unknown dbi->data->mmsys_id = %d", __func__, dbi->data->mmsys_id);
	}

	// Dump DBI registers
	dev_info(dbi->dev, "[DBI_START]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_START, readl(dbi->regs + DBI_START));
	dev_info(dbi->dev, "[DBI_INTEN]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_INTEN, readl(dbi->regs + DBI_INTEN));
	dev_info(dbi->dev, "[DBI_INTSTA]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_INTSTA, readl(dbi->regs + DBI_INTSTA));
	dev_info(dbi->dev, "[DBI_ROI_CADD]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_ROI_CADD, readl(dbi->regs + DBI_ROI_CADD));
	dev_info(dbi->dev, "[DBI_ROI_DADD]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_ROI_DADD, readl(dbi->regs + DBI_ROI_DADD));
	dev_info(dbi->dev, "[DBI_ROICON]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_ROICON, readl(dbi->regs + DBI_ROICON));
	dev_info(dbi->dev, "[DBI_ROI_SIZE]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_ROI_SIZE, readl(dbi->regs + DBI_ROI_SIZE));
	dev_info(dbi->dev, "[DBI_TECON]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_TECON, readl(dbi->regs + DBI_TECON));
	dev_info(dbi->dev, "[DBI_PATTERN]Address: 0x%08X, Value: 0x%08X",
			DBI_BASE + DBI_PATTERN, readl(dbi->regs + DBI_PATTERN));

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI) {
		dev_info(dbi->dev, "[DBI_PCNF0]Address: 0x%08X, Value: 0x%08X",
				DBI_BASE + DBI_PCNF0, readl(dbi->regs + DBI_PCNF0));
		dev_info(dbi->dev, "[DBI_PDW]Address: 0x%08X, Value: 0x%08X",
				DBI_BASE + DBI_PDW, readl(dbi->regs + DBI_PDW));
	} else {
		dev_info(dbi->dev, "[DBI_SCNF]Address: 0x%08X, Value: 0x%08X",
				DBI_BASE + DBI_SCNF, readl(dbi->regs + DBI_SCNF));
		dev_info(dbi->dev, "[DBI_SIF_TIMING0]Address: 0x%08X, Value: 0x%08X",
				DBI_BASE + DBI_SIF_TIMING0, readl(dbi->regs + DBI_SIF_TIMING0));
		dev_info(dbi->dev, "[DBI_SIF_PIX_CON]Address: 0x%08X, Value: 0x%08X",
				DBI_BASE + DBI_SIF_PIX_CON, readl(dbi->regs + DBI_SIF_PIX_CON));
	}

	dev_info(dbi->dev, "[%s] exit", __func__);
}

static const struct mtk_dbi_data mt6890_dbi_driver_data = {
	.mmsys_id = MMSYS_MT6890,
};

static const struct mtk_dbi_data mt6990_dbi_driver_data = {
	.mmsys_id = MMSYS_MT6990,
};

static const struct of_device_id mtk_dbi_of_match[] = {
	{ .compatible = "mediatek,mt6890-dbi0",
	  .data = &mt6890_dbi_driver_data},
	{ .compatible = "mediatek,mt6990-dbi0",
	  .data = &mt6990_dbi_driver_data},
	{ },
};

static void mtk_dbi_nli_arb_init(struct mtk_dbi *dbi)
{
	struct device_node *node;

	/* get NLI arbiter base address */
	node = of_find_compatible_node(NULL, NULL, "mediatek,nli_arb");
	if (!node) {
		pr_err("error: can't find nli_arb node\n");
		WARN_ON(1);
		return;
	}

	dbi->nli_arb_base = of_iomap(node, 0);
	if (!dbi->nli_arb_base) {
		pr_err("error: iomap fail for nli_arb\n");
		WARN_ON(1);
		return;
	}

	dbi_dev_info(dbi->dev, "[%s] ctrl_type = %d, mmsys_id = %d",
			__func__, dbi->host_info.ctrl_type, dbi->data->mmsys_id);

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI) {
		// Set NLI_ARB_CON to allow DBI-B interface,
		// need to clear favor_nfi and favor_dbi

		switch (dbi->data->mmsys_id) {
		case MMSYS_MT6890:
		case MMSYS_MT6990:
		#ifndef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
			mtk_dbi_mask_arb(dbi, NLI_ARB_CON, NLI_ARB_CON_NFI_BYPASS_MASK,
					0 << NLI_ARB_CON_NFI_BYPASS_SHIFT);

		#endif
			break;
		default:
			dbi_dev_info(dbi->dev, "[%s]unknown dbi->data->mmsys_id = %d", __func__, dbi->data->mmsys_id);
		}
	}
}

static void mtk_dbi_set_gpio(struct mtk_dbi *dbi)
{
	int ret = RETURN_OK;
	int i;
	struct device_node *node;

	int type = DBI_GPIO_DISABLE;
	mtk_dbi_lcm_interface_size itf_size = dbi->host_info.dbi_b_params.itf_size;

	dbi_dev_info(dbi->dev, "[%s] start ctrl_type = %d\n", __func__, dbi->host_info.ctrl_type);

	dbi->gpio_pinctrl = devm_pinctrl_get(dbi->dev);
	if (IS_ERR(dbi->gpio_pinctrl)) {
		ret = PTR_ERR(dbi->gpio_pinctrl);
		dev_err(dbi->dev, "%s(), ret %d, cannot get dbi->gpio_pinctrl!\n",
			__func__, ret);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(dbi_gpios); i++) {
		dbi_gpios[i].gpioctrl = pinctrl_lookup_state(dbi->gpio_pinctrl,
							     dbi_gpios[i].name);
		if (IS_ERR(dbi_gpios[i].gpioctrl)) {
			ret = PTR_ERR(dbi_gpios[i].gpioctrl);
			dev_err(dbi->dev, "%s(), pinctrl_lookup_state %s fail, ret %d\n",
				__func__, dbi_gpios[i].name, ret);
		} else {
			dbi_gpios[i].gpio_prepare = true;
			dev_err(dbi->dev, "%s(), pinctrl_lookup_state %s successful.\n",
				__func__, dbi_gpios[i].name);
		}
	}

#ifdef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
	/* get topckgen base address */
	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (!node) {
		pr_err("error: can't find topckgen node\n");
		WARN_ON(1);
		return;
	}

	dbi->topckgen_base = of_iomap(node, 0);
	if (!dbi->nli_arb_base) {
		pr_err("error: iomap fail for topckgen\n");
		WARN_ON(1);
		return;
	}

	/* get mmsys base address */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6990-mmsys");
	if (!node) {
		pr_err("error: can't find mmsys node\n");
		WARN_ON(1);
		return;
	}

	dbi->mmsys_base = of_iomap(node, 0);
	if (!dbi->mmsys_base) {
		pr_err("error: iomap fail for mmsys\n");
		WARN_ON(1);
		return;
	}
#endif

	/* get GPIO base address */
	node = of_find_compatible_node(NULL, NULL, "mediatek,gpio");
	if (!node) {
		pr_err("error: can't find GPIO node\n");
		WARN_ON(1);
		return;
	}

	dbi->gpio_base = of_iomap(node, 0);
	if (!dbi->gpio_base) {
		pr_err("error: iomap fail for GPIO\n");
		WARN_ON(1);
		return;
	}

	switch (dbi->host_info.ctrl_type) {
	// Set DBI-B GPIO
	case MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI:
		if (itf_size == MTK_DBI_LCM_INTERFACE_SIZE_8BITS) {
			type = DBI_GPIO_TYPE_B_8BITS;
		} else {
			type = DBI_GPIO_TYPE_B_9BITS;
		}

		// Set DBI-B GPIO
		ret = pinctrl_select_state(dbi->gpio_pinctrl, dbi_gpios[type].gpioctrl);
		if (ret)
			dev_err(dbi->dev, "%s(), error, can not set gpio type %d\n",
				__func__, type);

#ifdef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
		mtk_dbi_mask_topckgen(dbi, TOPCKGEN_DBI_CLK_SRC, TOPCKGEN_DBI_CLK_SRC_MASK,
				1 << TOPCKGEN_DBI_CLK_SRC_SHIFT);
		mtk_dbi_mask_mmsys(dbi, MMSYS_DBI_OUTPUT_DATA, MMSYS_DBI_OUTPUT_DATA_MASK,
				1);

		ret = pinctrl_select_state(dbi->gpio_pinctrl, dbi_gpios[DBI_GPIO_SEPARATE_DATA_PINS].gpioctrl);
		if (ret) {
			dev_err(dbi->dev, "%s(), error, can not set gpio seperate data pin with ret = %d\n",
				__func__, ret);
			mtk_dbi_mask_gpio(dbi, GPIO_MODE25, 0x70007777, 0x20002222);
			mtk_dbi_mask_gpio(dbi, GPIO_MODE26, 0x00000777, 0x00000222);
		}

		dbi_dev_info(dbi->dev, "[%s] enable separate data pin\n", __func__);
#else
		ret = pinctrl_select_state(dbi->gpio_pinctrl, dbi_gpios[DBI_GPIO_CO_NFI_DATA_PINS].gpioctrl);
		if (ret)
			dev_err(dbi->dev, "%s(), error, can not set gpio co nfi data pin with ret = %d\n",
				__func__, ret);
#endif

		break;
	case MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI:
		// Set DBI-C GPIO
		type = DBI_GPIO_TYPE_C;
		ret = pinctrl_select_state(dbi->gpio_pinctrl, dbi_gpios[type].gpioctrl);
		if (ret)
			dev_err(dbi->dev, "%s(), error, can not set gpio type %d\n",
				__func__, type);

		break;
	default: // Non-DBI, set DBI PIN to GPIO mode
		// Keep LPRSTB in GPIO mode to avoid unexpected behavio
		type = DBI_GPIO_DISABLE;
		ret = pinctrl_select_state(dbi->gpio_pinctrl, dbi_gpios[type].gpioctrl);
		if (ret)
			dev_err(dbi->dev, "%s(), error, can not set gpio type %d\n",
				__func__, type);

	}

	dbi_dev_info(dbi->dev, "%s(), set gpio type %d, ret = %d\n", __func__, type, ret);

}

static int mtk_dbi_set_driving_current(struct mtk_dbi *dbi)
{
	int ret = RETURN_OK;
	int driving_current;
	struct device_node *node;

	dbi_dev_info(dbi->dev, "[%s] mmsys_id = %d", __func__, dbi->data->mmsys_id);

	/* get iocfg_tl base address */
	node = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_rt8");
	if (!node) {
		pr_err("error: can't find iocfg_rt8 node\n");
		WARN_ON(1);
		return 0;
	}

	dbi->iocfg_rt8_base = of_iomap(node, 0);
	if (!dbi->iocfg_rt8_base) {
		pr_err("error: iomap fail for iocfg_rt8\n");
		WARN_ON(1);
		return 0;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_rt9");
	if (!node) {
		pr_err("error: can't find iocfg_rt9 node\n");
		WARN_ON(1);
		return 0;
	}

	dbi->iocfg_rt9_base = of_iomap(node, 0);
	if (!dbi->iocfg_rt9_base) {
		pr_err("error: iomap fail for iocfg_rt9\n");
		WARN_ON(1);
		return 0;
	}

	switch (dbi->data->mmsys_id) {
	case MMSYS_MT6890:
		// IOCFG_TL_BASE+IOCFG_TL_DRV_CFG1[17:15] for PAD_LPA0,PAD_LSDI
		// IOCFG_TL_BASE+IOCFG_TL_DRV_CFG1[14:12] for PAD_LPCE0B,PAD_LPRDB,PAD_LPRSTB,PAD_LPWRB
		// IOCFG_TL_BASE+IOCFG_TL_DRV_CFG1[11:9]  for PAD_NLD2,PAD_NLD4,PAD_NLD6,PAD_NLD7
		// IOCFG_TL_BASE+IOCFG_TL_DRV_CFG1[8:6]   for PAD_NLD1,PAD_NLD3,PAD_NLD5,PAD_NLD0
		driving_current = ((dbi->host_info.driving_current.group3 << 6)
				+ (dbi->host_info.driving_current.group3 << 9)
				+ (dbi->host_info.driving_current.group2 << 12)
				+ (dbi->host_info.driving_current.group1 << 15));

		mtk_dbi_mask_driving(dbi, IOCFG_TL_DRV_CFG1, MT6890_IOCFG_TL_DRV_CFG1_MASK, driving_current);
		break;
	case MMSYS_MT6990:
#ifdef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
		driving_current = ((dbi->host_info.driving_current.group3 << 6)
				+ (dbi->host_info.driving_current.group3 << 9)
				+ (dbi->host_info.driving_current.group3 << 12)
				+ (dbi->host_info.driving_current.group3 << 15)
				+ (dbi->host_info.driving_current.group3 << 18)
				+ (dbi->host_info.driving_current.group3 << 21)
				+ (dbi->host_info.driving_current.group3 << 24)
				+ (dbi->host_info.driving_current.group3 << 27));
		mtk_dbi_mask_driving_msdc(dbi, IOCFG_TL_DRV_CFG0, MT6990_IOCFG_RT8_DRV_CFG0_MASK, driving_current);

		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[23:21] for PAD_LSDI; PAD_LPRDB; PAD_DSI_TE; PAD_LPWRB
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[20:18] for PAD_LPA0; PAD_DISP_PWM; PAD_LPCE0B; PAD_LPRSTB
		driving_current = ((dbi->host_info.driving_current.group2 << 18)
				+ (dbi->host_info.driving_current.group1 << 21));
		mtk_dbi_mask_driving(dbi, IOCFG_TL_DRV_CFG1, MT6990_IOCFG_RT9_DRV_CFG1_CTL_MASK, driving_current);

#else
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG0[29:27] for PAD_NLD4
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG0[26:24] for PAD_NLD3
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG0[23:21] for PAD_NLD2
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG0[20:18] for PAD_NLD1
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG0[17:15] for PAD_NLD0
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG0[2:0]	for LCM_RST
		driving_current = (0	//((dbi->host_info.driving_current.group3)
				+ (dbi->host_info.driving_current.group3 << 15)
				+ (dbi->host_info.driving_current.group3 << 18)
				+ (dbi->host_info.driving_current.group3 << 21)
				+ (dbi->host_info.driving_current.group3 << 24)
				+ (dbi->host_info.driving_current.group3 << 27));
		mtk_dbi_mask_driving(dbi, IOCFG_TL_DRV_CFG0, MT6990_IOCFG_RT9_DRV_CFG0_MASK, driving_current);

		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[23:21] for PAD_LSDI; PAD_LPRDB; PAD_DSI_TE; PAD_LPWRB
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[20:18] for PAD_LPA0; PAD_DISP_PWM; PAD_LPCE0B; PAD_LPRSTB
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[8:6]   for PAD_NLD7
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[5:3]   for PAD_NLD6
		// IOCFG_RT9_BASE+IOCFG_TL_DRV_CFG1[2:0]	for PAD_NLD5
		driving_current = ((dbi->host_info.driving_current.group3)
				+ (dbi->host_info.driving_current.group3 << 3)
				+ (dbi->host_info.driving_current.group3 << 6)
				+ (dbi->host_info.driving_current.group2 << 18)
				+ (dbi->host_info.driving_current.group1 << 21));
		mtk_dbi_mask_driving(dbi, IOCFG_TL_DRV_CFG1, MT6990_IOCFG_RT9_DRV_CFG1_MASK, driving_current);
#endif
		break;
	default:
		dbi_dev_info(dbi->dev, "[%s]unknown dbi->data->mmsys_id = %d", __func__, dbi->data->mmsys_id);
	}

	return ret;
}

static void mtk_dbi_unlock_data_pin(struct mtk_dbi *dbi)
{
	dbi_dev_info(dbi->dev, "[%s] ctrl_type = %d, locked = %d", __func__, dbi->host_info.ctrl_type, dbi->locked);

	// Workaround for ARB
	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI) {
		mtk_dbi_mask_arb(dbi, NLI_ARB_CON, NLI_ARB_CON_FAVOR_NLI_DBI_MASK,
				0 << NLI_ARB_CON_FAVOR_DBI_SHIFT);
		writel(0x0, dbi->nli_arb_base + NLI_ARB_SHAREPIN_CON);

#ifdef AUTO_RELEASE_DATA_PIN
		if (dbi->locked) {
			del_timer(&dbi->locked_timer);
			dbi->locked = false;
		}
#endif

		dev_info(dbi->dev, "[%s] write arb to avoid blocking nfi", __func__);
	}
}

#ifdef AUTO_RELEASE_DATA_PIN
void mtk_dbi_locked_timer_callback(struct timer_list *timer)
{
	struct mtk_dbi *dbi = from_timer(dbi, timer, locked_timer);

	dbi_dev_info(dbi->dev, "[%s] locked = %d", __func__, dbi->locked);

	if (dbi->locked) {
		mtk_dbi_unlock_data_pin(dbi);
	}
}
#endif

static void mtk_dbi_lock_data_pin(struct mtk_dbi *dbi)
{
	dbi_dev_info(dbi->dev, "[%s] ctrl_type = %d", __func__, dbi->host_info.ctrl_type);

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI) {
		// Workaround for ARB
		mtk_dbi_mask_arb(dbi, NLI_ARB_CON, NLI_ARB_CON_FAVOR_NLI_DBI_MASK,
				1 << NLI_ARB_CON_FAVOR_DBI_SHIFT);
		writel(0x01, dbi->nli_arb_base + NLI_ARB_SHAREPIN_CON);

#ifdef AUTO_RELEASE_DATA_PIN
		dbi_dev_info(dbi->dev, "[%s] enable timer with %d ms",
			__func__, LOCKED_TIMEOUT);

		if (timer_pending(&dbi->locked_timer)) {
			dev_info(dbi->dev, "[%s] timer is currently pending", __func__);
		} else {
			// Set up timer to auto release data pin to avoid blocking
			timer_setup(&dbi->locked_timer, &mtk_dbi_locked_timer_callback, 0);
			dbi->locked_timer.expires = jiffies + msecs_to_jiffies(LOCKED_TIMEOUT);
			add_timer(&dbi->locked_timer);
			dbi->locked = true;
		}
#endif
		dev_info(dbi->dev, "[%s] write arb to block nfi", __func__);
	}
}

static void mtk_dbi_start(struct mtk_dbi *dbi)
{
	dbi_dev_info(dbi->dev, "[%s] enter", __func__);

#ifndef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
	mtk_dbi_lock_data_pin(dbi);
#endif

	//Reset
	mtk_dbi_mask(dbi, DBI_START, DBI_SRST_FLAG, 1);
	mtk_dbi_mask(dbi, DBI_START, DBI_SRST_FLAG, 0);

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI)
		writel(0x2C, dbi->regs + DBI_SCMD0);
	else
		writel(0x2C, dbi->regs + DBI_PCMD0);

	mtk_dbi_mask(dbi, DBI_START, DBI_START_FLAG, 0 << 15);
	mtk_dbi_mask(dbi, DBI_START, DBI_START_FLAG, 1 << 15);
}

static void mtk_dbi_stop(struct mtk_dbi *dbi)
{
	mtk_dbi_mask(dbi, DBI_START, DBI_START_FLAG,  0 << 15);
}

static void mtk_dbi_set_roi_cmd_data(struct mtk_dbi *dbi)
{
	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI) {
		/* Command and data are sent to DBI-C LCM CS0 */
		mtk_dbi_mask(dbi, DBI_ROI_CADD, CADD_ADDR, 8 << 4);
		mtk_dbi_mask(dbi, DBI_ROI_DADD, DADD_ADDR, 9 << 4);
	} else {
		/* Command and data are sent to DBI-B LCM CS0 */
		mtk_dbi_mask(dbi, DBI_ROI_CADD, CADD_ADDR, 0 << 4);
		mtk_dbi_mask(dbi, DBI_ROI_DADD, DADD_ADDR, 1 << 4);
	}
}

static void mtk_dbi_set_roi_con(struct mtk_dbi *dbi)
{
	if (dbi->host_info.color_order == MTK_DBI_LCM_COLOR_ORDER_BGR)
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_SEQ, 0);	//0: BGR
	else
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_SEQ, 1);	//1: RGB

	//[Color format] 000: RGB332, 001: RGB444, 010: RGB565, 011: RGB666, 100: RGB888
	switch (dbi->host_info.color_format) {
	case MTK_DBI_LCM_COLOR_FORMAT_RGB332:
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_FORMAT, 0);
		break;
	case MTK_DBI_LCM_COLOR_FORMAT_RGB444:
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_FORMAT, 1 << 3);
		break;
	case MTK_DBI_LCM_COLOR_FORMAT_RGB565:
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_FORMAT, 2 << 3);
		break;
	case MTK_DBI_LCM_COLOR_FORMAT_RGB666:
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_FORMAT, 3 << 3);
		break;
	case MTK_DBI_LCM_COLOR_FORMAT_RGB888:
		mtk_dbi_mask(dbi, DBI_ROICON, COLOR_FORMAT, 4 << 3);
		break;

	default:
		dbi_dev_info(dbi->dev, "[%s], unknown color format:%d\n",
				__func__, dbi->host_info.color_format);
	}

	//if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI) {
	//	DRM_WARN("mtk_dbi_set_roi_con, ignore data_width setting for DBI-C\n");
	//} else {
		// [Interface size] 00: 8bit, 01: 16bit, 10: 9bit, 11:18bit
		switch (dbi->host_info.dbi_b_params.itf_size) {
		case MTK_DBI_LCM_INTERFACE_SIZE_8BITS:
			mtk_dbi_mask(dbi, DBI_ROICON, ITF_SIZE, 0);
			break;
		case MTK_DBI_LCM_INTERFACE_SIZE_9BITS:
			mtk_dbi_mask(dbi, DBI_ROICON, ITF_SIZE, 2 << 6);
			break;
		case MTK_DBI_LCM_INTERFACE_SIZE_16BITS:
			mtk_dbi_mask(dbi, DBI_ROICON, ITF_SIZE, 1 << 6);
			break;
		case MTK_DBI_LCM_INTERFACE_SIZE_18BITS:
			mtk_dbi_mask(dbi, DBI_ROICON, ITF_SIZE, 3 << 6);
			break;
		default:
			dbi_dev_info(dbi->dev, "[%s], unknown data_width:%d\n",
					__func__, dbi->host_info.dbi_b_params.itf_size);
		}
	//}
}

static void mtk_dbi_set_roi_size(struct mtk_dbi *dbi)
{
	mtk_dbi_mask(dbi, DBI_ROI_SIZE, ROI_SIZE_ROW, dbi->mode.vdisplay << 16);
	mtk_dbi_mask(dbi, DBI_ROI_SIZE, ROI_SIZE_COL, dbi->mode.hdisplay);
}

static void mtk_dbi_set_timing(struct mtk_dbi *dbi)
{
	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI) {
		// Set DBI serial interface configuration
		// 0: Chip selection of serial interface is controlled by software
		//    by manipulating register DBI_SCNF_CS.
		// 1: Chip selection of serial interface is controlled by hardware.
		mtk_dbi_mask(dbi, DBI_SCNF, SIF_HW_CS, 1 << 24);

		// Slow down the serial interface timing
		// 0: Disable
		// 1: Enable
		mtk_dbi_mask(dbi, DBI_SCNF, SIF0_DIV2,
			dbi->host_info.dbi_c_params.rw_timing.slow_down << 7);

		// The default value of LSCK for serial interface when not transfer data
		// 0: Default of LSCK is low.
		// 1: Default of LSCK is high.
		mtk_dbi_mask(dbi, DBI_SCNF, SIF0_SCK_DEF,
			dbi->host_info.dbi_c_params.rw_timing.lsck_default << 6);

		// The first phase polarity of LSCK for serial interface 0
		// 0: The first phase of LSCK is low
		// 1: The first phase of LSCK is high
		mtk_dbi_mask(dbi, DBI_SCNF, SIF0_1ST_POL,
			dbi->host_info.dbi_c_params.rw_timing.lsck_1st_pol << 5);

		// Set to 1 to read data from LSDI pin;
		// Otherwise LCD will use the bi-directional LSDA pin.
		mtk_dbi_mask(dbi, DBI_SCNF, SIF0_SDI,
			dbi->host_info.dbi_c_params.rw_timing.read_data_from_sdi << 4);

		if (dbi->host_info.dbi_c_params.wire_num == MTK_DBI_LCM_INTERFACE_3WIRE)
			mtk_dbi_mask(dbi, DBI_SCNF, SIF0_3WIRE, 1 << 3);

		mtk_dbi_mask(dbi, DBI_SCNF, SIF0_SIZE, dbi->host_info.data_width);

		// Set DBI serial interface read/write timing
		mtk_dbi_mask(dbi, DBI_SIF_TIMING0, CSS,
			dbi->host_info.dbi_c_params.rw_timing.cs_setup_time << 20);
		mtk_dbi_mask(dbi, DBI_SIF_TIMING0, CSH,
			dbi->host_info.dbi_c_params.rw_timing.cs_hold_time << 16);
		mtk_dbi_mask(dbi, DBI_SIF_TIMING0, RD1ST,
			dbi->host_info.dbi_c_params.rw_timing.read_1st << 12);
		mtk_dbi_mask(dbi, DBI_SIF_TIMING0, RD2ND,
			dbi->host_info.dbi_c_params.rw_timing.read_2nd << 8);
		mtk_dbi_mask(dbi, DBI_SIF_TIMING0, WR1ST,
			dbi->host_info.dbi_c_params.rw_timing.write_1st << 4);
		mtk_dbi_mask(dbi, DBI_SIF_TIMING0, WR2ND,
			dbi->host_info.dbi_c_params.rw_timing.write_2nd);

	} else {
		mtk_dbi_mask(dbi, DBI_PCNF0, C2RH,
			dbi->host_info.dbi_b_params.rw_timing.cs_to_read_hold_time << C2RH_SHIFT);
		mtk_dbi_mask(dbi, DBI_PCNF0, C2RS,
			dbi->host_info.dbi_b_params.rw_timing.cs_to_read_setup_time << C2RS_SHIFT);
		mtk_dbi_mask(dbi, DBI_PCNF0, RLT,
			dbi->host_info.dbi_b_params.rw_timing.read_latency_time << RLT_SHIFT);
		mtk_dbi_mask(dbi, DBI_PCNF0, C2WH,
			dbi->host_info.dbi_b_params.rw_timing.cs_to_write_hold_time << C2WH_SHIFT);
		mtk_dbi_mask(dbi, DBI_PCNF0, C2WS,
			dbi->host_info.dbi_b_params.rw_timing.cs_to_write_setup_time << C2WS_SHIFT);
		mtk_dbi_mask(dbi, DBI_PCNF0, WST,
			dbi->host_info.dbi_b_params.rw_timing.write_wait_state_time);
		mtk_dbi_mask(dbi, DBI_PDW, P0CHW,
			dbi->host_info.dbi_b_params.rw_timing.cs_remain_high_time << P0CHW_SHIFT);
	}
}

static void mtk_dbi_set_data_width(struct mtk_dbi *dbi)
{
	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI)
		mtk_dbi_mask(dbi, DBI_PDW, P0DW, dbi->host_info.data_width); // DBI PDW
	else
		mtk_dbi_mask(dbi, DBI_SCNF, SIF0_SIZE, dbi->host_info.data_width);
}

static void mtk_dbi_set_te_control(struct mtk_dbi *dbi)
{
	dbi_dev_info(dbi->dev, "[%s], TE_ENABLE=%d", __func__,
			dbi->host_info.te_control.te_enable);

	mtk_dbi_mask(dbi, DBI_TECON, TE_ENABLE,
			dbi->host_info.te_control.te_enable);
	mtk_dbi_mask(dbi, DBI_TECON, TE_EDGE,
			dbi->host_info.te_control.te_edge << 1);
	mtk_dbi_mask(dbi, DBI_TECON, TE_SYNC,
			dbi->host_info.te_control.sync_mode << 2);
	mtk_dbi_mask(dbi, DBI_TECON, TE_REPEART,
			dbi->host_info.te_control.te_repeat << 3);
	mtk_dbi_mask(dbi, DBI_TECON, SW_TE,
			dbi->host_info.te_control.software_te << 15);
}

static void mtk_dbi_set_pixel_data_config(struct mtk_dbi *dbi)
{
	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI) {
		mtk_dbi_mask(dbi, DBI_SIF_PIX_CON, SIF0_CS_STAY_LOW,
			dbi->host_info.dbi_c_params.cs_stay_low << 7);
		mtk_dbi_mask(dbi, DBI_SIF_PIX_CON, SIF0_SINGLE_A0,
			dbi->host_info.dbi_c_params.single_a0_mode << 6);
		mtk_dbi_mask(dbi, DBI_SIF_PIX_CON, SIF0_PIX_2PIN,
			dbi->host_info.dbi_c_params.two_data_pin_enable << 4);
		mtk_dbi_mask(dbi, DBI_SIF_PIX_CON, SIF0_2PIN_SIZE,
			dbi->host_info.dbi_c_params.two_data_pin_itf_size);
	}
}

static void mtk_dbi_reset_unused_regs(struct mtk_dbi *dbi)
{
	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI) {
		// Clear DBI-C register
		writel(0, dbi->regs + DBI_SCNF);
		writel(0, dbi->regs + DBI_SIF_TIMING0);
	} else {
		// Clear DBI-B register
		writel(0, dbi->regs + DBI_PDW);
		writel(0, dbi->regs + DBI_PCNF0);
	}
}

#ifdef ENABLE_DBI_PATTERN
static void mtk_dbi_set_pattern(struct mtk_dbi *dbi)
{
	dbi_dev_info(dbi->dev, "[%s], value=0x00ff0041", __func__);

	writel(0x00ff0041, dbi->regs + DBI_PATTERN);
}
#endif

static void mtk_dbi_send_cmd(
		struct drm_panel *panel, unsigned int cmd)
{
	struct mtk_dbi *dbi = connector_to_dbi(panel->connector);

	dbi_dev_info(dbi->dev, "[%s]ctrl_type = %d, cmd = %02X", __func__,
			dbi->host_info.ctrl_type, cmd);

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI)
		writel(cmd, dbi->regs + DBI_SCMD0);
	else // DBI-B
		writel(cmd, dbi->regs + DBI_PCMD0);
}

static void mtk_dbi_send_data(
		struct drm_panel *panel, unsigned int data)
{
	struct mtk_dbi *dbi = connector_to_dbi(panel->connector);

	dbi_dev_info(dbi->dev, "[%s]ctrl_type = %d, data = %02X", __func__,
			dbi->host_info.ctrl_type, data);

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI)
		writel(data, dbi->regs + DBI_SDAT0);
	else // DBI-B
		writel(data, dbi->regs + DBI_PDAT0);
}

static unsigned int mtk_dbi_read_data(struct drm_panel *panel)
{
	struct mtk_dbi *dbi = connector_to_dbi(panel->connector);
	unsigned int data = 0;

	if (dbi->host_info.ctrl_type == MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI)
		data = readl(dbi->regs + DBI_SDAT0);
	else // DBI-B
		data = readl(dbi->regs + DBI_PDAT0);

	dbi_dev_info(dbi->dev, "[%s]ctrl_type = %d, data = %02X", __func__,
			dbi->host_info.ctrl_type, data);

	return data;
}

static void mtk_dbi_occupy_data_pin(struct drm_panel *panel)
{
	struct mtk_dbi *dbi = connector_to_dbi(panel->connector);

	dbi_dev_info(dbi->dev, "[%s] ctrl_type = %d", __func__, dbi->host_info.ctrl_type);

#ifndef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
	mtk_dbi_lock_data_pin(dbi);
#endif
}

static void mtk_dbi_release_data_pin(struct drm_panel *panel)
{
	struct mtk_dbi *dbi = connector_to_dbi(panel->connector);

	dbi_dev_info(dbi->dev, "[%s]ctrl_type = %d", __func__, dbi->host_info.ctrl_type);

#ifndef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
	mtk_dbi_unlock_data_pin(dbi);
#endif
}

static void mtk_dbi_set_interrupt_enable(struct mtk_dbi *dbi)
{
	u32 inten = DBI_INT_FLAG_CPL;

	if (dbi->host_info.te_control.te_enable == 1)
		inten = inten | DBI_INT_FLAG_SYNC | DBI_INT_FLAG_TE;

	dbi_dev_info(dbi->dev, "[%s] inten = %d", __func__, inten);

	writel(inten, dbi->regs + DBI_INTEN);
}

#if 0
static void mtk_dbi_irq_data_set(struct mtk_dbi *dbi, u32 irq_bit)
{
	dbi->irq_data |= irq_bit;
}
#endif

static irqreturn_t mtk_dbi_irq(int irq, void *dev_id)
{
	struct mtk_dbi *dbi = dev_id;
	u32 status;
	u32 flag = DBI_INT_FLAG_CPL | DBI_INT_FLAG_SYNC | DBI_INT_FLAG_TE;

	status = readl(dbi->regs + DBI_INTSTA) & flag;

	// dbi_dev_info(dbi->dev, "[%s] status=%d", __func__, status);

	if (status) {
		mtk_dbi_mask(dbi, DBI_INTSTA, status, 0);

#ifndef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
		if (status & DBI_INT_FLAG_CPL)
			mtk_dbi_unlock_data_pin(dbi);
#endif
	}

	return IRQ_HANDLED;
}

static void mtk_dbi_copy_host_info_data(
		struct mipi_dbi_device *target, struct mipi_dbi_device *src)
{
	target->ctrl_type = src->ctrl_type;
	target->clk_freq = src->clk_freq;

	// Drviing Current
	target->driving_current.group1 = src->driving_current.group1;
	target->driving_current.group2 = src->driving_current.group2;
	target->driving_current.group3 = src->driving_current.group3;

	// Color control
	target->data_width = src->data_width;
	target->color_order = src->color_order;
	target->color_format = src->color_format;

	// Tearing control
	target->te_control.software_te
			= src->te_control.software_te;
	target->te_control.te_repeat
			= src->te_control.te_repeat;
	target->te_control.sync_mode
			= src->te_control.sync_mode;
	target->te_control.te_edge
			= src->te_control.te_edge;
	target->te_control.te_enable
			= src->te_control.te_enable;

	if (src->ctrl_type == MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI) {
		// Interface size
		target->dbi_b_params.itf_size = src->dbi_b_params.itf_size;

		// Timing
		target->dbi_b_params.rw_timing.cs_to_read_hold_time
				= src->dbi_b_params.rw_timing.cs_to_read_hold_time;
		target->dbi_b_params.rw_timing.cs_to_read_setup_time
				= src->dbi_b_params.rw_timing.cs_to_read_setup_time;
		target->dbi_b_params.rw_timing.read_latency_time
				= src->dbi_b_params.rw_timing.read_latency_time;
		target->dbi_b_params.rw_timing.cs_to_write_hold_time
				= src->dbi_b_params.rw_timing.cs_to_write_hold_time;
		target->dbi_b_params.rw_timing.cs_to_write_setup_time
				= src->dbi_b_params.rw_timing.cs_to_write_setup_time;
		target->dbi_b_params.rw_timing.write_wait_state_time
				= src->dbi_b_params.rw_timing.write_wait_state_time;
		target->dbi_b_params.rw_timing.cs_remain_high_time
				= src->dbi_b_params.rw_timing.cs_remain_high_time;
	} else if (MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI) {
		// Wire number
		target->dbi_c_params.wire_num = src->dbi_c_params.wire_num;

		// Timing
		target->dbi_c_params.rw_timing.cs_setup_time
				= src->dbi_c_params.rw_timing.cs_setup_time;
		target->dbi_c_params.rw_timing.cs_hold_time
				= src->dbi_c_params.rw_timing.cs_hold_time;
		target->dbi_c_params.rw_timing.read_1st
				= src->dbi_c_params.rw_timing.read_1st;
		target->dbi_c_params.rw_timing.read_2nd
				= src->dbi_c_params.rw_timing.read_2nd;
		target->dbi_c_params.rw_timing.write_1st
				= src->dbi_c_params.rw_timing.write_1st;
		target->dbi_c_params.rw_timing.write_2nd
				= src->dbi_c_params.rw_timing.write_2nd;
		target->dbi_c_params.rw_timing.slow_down
				= src->dbi_c_params.rw_timing.slow_down;
		target->dbi_c_params.rw_timing.lsck_default
				= src->dbi_c_params.rw_timing.lsck_default;
		target->dbi_c_params.rw_timing.lsck_1st_pol
				= src->dbi_c_params.rw_timing.lsck_1st_pol;
		target->dbi_c_params.rw_timing.read_data_from_sdi
				= src->dbi_c_params.rw_timing.read_data_from_sdi;

		// Stay low
		target->dbi_c_params.cs_stay_low = src->dbi_c_params.cs_stay_low;

		// Single A0
		target->dbi_c_params.single_a0_mode = src->dbi_c_params.single_a0_mode;

		// two data pin
		target->dbi_c_params.two_data_pin_enable
				= src->dbi_c_params.two_data_pin_enable;
		target->dbi_c_params.two_data_pin_itf_size
				= src->dbi_c_params.two_data_pin_itf_size;

	}

}

static unsigned int mtk_dbi_poweron(struct mtk_dbi *dbi)
{
	int ret = 0;
	struct clk *parent_clk;
	struct clk *engine_clk;

	dbi_dev_info(dbi->dev, "[%s] Enter, power on = %d\n", __func__, dbi->poweron);

	// Enable clk
	ret = clk_prepare_enable(dbi->mm_clk);
	if (ret < 0) {
		dev_err(dbi->dev, "Failed to enable mm clock: %d\n", ret);
		goto err_clk;
	}

	ret = clk_prepare_enable(dbi->mm_dbi_clk);
	if (ret < 0) {
		dev_err(dbi->dev, "Failed to enable mm mm_dbi_clk: %d\n", ret);
		goto err_disable_mm_clk;
	}

	ret = clk_prepare_enable(dbi->mm_smi_clk);
	if (ret < 0) {
		dev_err(dbi->dev, "Failed to enable mm_smi_clk clock: %d\n", ret);
		goto err_disable_mm_dbi_clk;
	}

	ret = clk_prepare_enable(dbi->infra_nfi_clk);
	if (ret < 0) {
		dev_err(dbi->dev, "Failed to enable infra_nfi_clk clock: %d\n", ret);
		goto err_disable_mm_smi_clk;
	}


#ifndef CONFIG_MTK_DBI_SEPARATE_DATA_PIN
	engine_clk = dbi->engine_clk;
	dbi_dev_info(dbi->dev, "[%s] choose nfi_sel clk\n", __func__);
#else
	engine_clk = dbi->mm_dbi_sel;
	dbi_dev_info(dbi->dev, "[%s] choose dbi_sel clk\n", __func__);
#endif

	// Set DBI clock MUX and parent clk
	ret = clk_prepare_enable(engine_clk);
	if (ret < 0) {
		dev_err(dbi->dev, "Failed to engine_clk clock: %d\n", ret);
		goto err_disable_infra_nfi_clk;
	}

	switch (dbi->host_info.clk_freq) {
	case MTK_DBI_LCM_CLOCK_FREQ_26M:
		parent_clk = dbi->parent_clk_26;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_78M:
		parent_clk = dbi->parent_clk_78;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_91M:
		parent_clk = dbi->parent_clk_91;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_104M:
		parent_clk = dbi->parent_clk_104;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_125M:
		parent_clk = dbi->parent_clk_125;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_137M:
		parent_clk = dbi->parent_clk_137;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_156M:
		parent_clk = dbi->parent_clk_156;
		break;
	case MTK_DBI_LCM_CLOCK_FREQ_182M:
		parent_clk = dbi->parent_clk_182;
		break;
	default:
		dev_err(dbi->dev, "Unknown clock freq: %d\n", dbi->host_info.clk_freq);
		goto err_disable_engine_clk;
	}

	ret = clk_set_parent(engine_clk, parent_clk);
	if (ret < 0) {
		dev_err(dbi->dev, "Failed to parent clock: %d\n", ret);
		goto err_disable_engine_clk;
	}

	dbi_dev_info(dbi->dev, "[%s] set clock done.\n", __func__);

	mtk_dbi_reset_unused_regs(dbi);

	// Set GPIO again to avoid GPIO reset issue
	mtk_dbi_set_gpio(dbi);

	mtk_dbi_set_roi_cmd_data(dbi);
	mtk_dbi_set_timing(dbi);
	mtk_dbi_set_roi_con(dbi);
	mtk_dbi_set_data_width(dbi);
	mtk_dbi_set_roi_size(dbi);
	mtk_dbi_set_te_control(dbi);
	mtk_dbi_set_interrupt_enable(dbi);
	mtk_dbi_set_pixel_data_config(dbi);

	// Add for debug
	mtk_dbi_dump_regs(dbi);
	if (dbi->panel) {
		if (drm_panel_prepare(dbi->panel) < 0) {
			DRM_ERROR("failed to prepare the panel\n");
			ret = -1;
			goto err_disable_engine_clk;
		}
	}

	dbi->poweron++;

	dbi_dev_info(dbi->dev, "[%s] Exit (ret = %d, poweron count = %d)\n", __func__, ret, dbi->poweron);

	return 0;

err_disable_engine_clk:
	clk_disable_unprepare(engine_clk);
err_disable_infra_nfi_clk:
	clk_disable_unprepare(dbi->infra_nfi_clk);
err_disable_mm_smi_clk:
	clk_disable_unprepare(dbi->mm_smi_clk);
err_disable_mm_dbi_clk:
	clk_disable_unprepare(dbi->mm_dbi_clk);
err_disable_mm_clk:
	clk_disable_unprepare(dbi->mm_clk);

err_clk:
	dbi_dev_info(dbi->dev, "[%s] Exit (ret = %d)\n", __func__, ret);
	return ret;
}

static int mtk_dbi_poweroff(struct mtk_dbi *dbi)
{
	int ret = 0;

	dbi_dev_info(dbi->dev, "[%s] Enter, power on = %d\n", __func__, dbi->poweron);

	if (dbi->poweron == 0)
		return 0;

	mtk_dbi_stop(dbi);

	if (dbi->panel) {
		if (drm_panel_unprepare(dbi->panel)) {
			DRM_ERROR("failed to unprepare the panel\n");
			return RETURN_ERROR;
		}
	}

	// Disable clk
	clk_disable_unprepare(dbi->engine_clk);
	clk_disable_unprepare(dbi->infra_nfi_clk);
	clk_disable_unprepare(dbi->mm_smi_clk);
	clk_disable_unprepare(dbi->mm_dbi_clk);
	clk_disable_unprepare(dbi->mm_clk);

	dbi->poweron--;

	dbi_dev_info(dbi->dev, "[%s] Exit(poweron count = %d)\n", __func__, dbi->poweron);

	return ret;
}

static void mtk_dbi_ddp_prepare(struct mtk_ddp_comp *comp)
{
	struct mtk_dbi *dbi = container_of(comp, struct mtk_dbi, ddp_comp);

	mtk_dbi_poweron(dbi);
}

static void mtk_dbi_ddp_unprepare(struct mtk_ddp_comp *comp)
{
	struct mtk_dbi *dbi = container_of(comp, struct mtk_dbi, ddp_comp);

	mtk_dbi_poweroff(dbi);
}

static int mtk_dbi_connector_get_modes(struct drm_connector *connector)
{
	struct mtk_dbi *dbi = connector_to_dbi(connector);

	return drm_panel_get_modes(dbi->panel);
}

static const struct drm_connector_funcs mtk_dbi_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
	mtk_dbi_connector_helper_funcs = {
	.get_modes = mtk_dbi_connector_get_modes,
};


static int mtk_dbi_create_connector(struct drm_device *drm, struct mtk_dbi *dbi)
{
	int ret;

	ret = drm_connector_init(drm, &dbi->conn, &mtk_dbi_connector_funcs,
				 DRM_MODE_CONNECTOR_DBI);
	if (ret) {
		DRM_ERROR("Failed to connector init to drm\n");
		return ret;
	}

	drm_connector_helper_add(&dbi->conn, &mtk_dbi_connector_helper_funcs);

	dbi->conn.dpms = DRM_MODE_DPMS_OFF;
	drm_connector_attach_encoder(&dbi->conn, &dbi->encoder);

	if (dbi->panel) {
		ret = drm_panel_attach(dbi->panel, &dbi->conn);
		if (ret) {
			DRM_ERROR("Failed to attach panel to drm\n");
			goto err_connector_cleanup;
		}
	}

	return 0;

err_connector_cleanup:
	drm_connector_cleanup(&dbi->conn);
	dev_err(dbi->dev, "[%s] ret = %d", __func__, ret);
	return ret;
}

static bool mtk_dbi_encoder_mode_fixup(struct drm_encoder *encoder,
					   const struct drm_display_mode *mode,
					   struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void mtk_dbi_encoder_mode_set(struct drm_encoder *encoder,
					 struct drm_display_mode *mode,
					 struct drm_display_mode *adjusted)
{
	struct mtk_dbi *dbi = encoder_to_dbi(encoder);

	dbi->mode.clock = mode->clock;
	dbi->mode.hdisplay = mode->hdisplay;
	dbi->mode.vdisplay = mode->vdisplay;
	dbi->mode.height_mm = mode->height_mm;
	dbi->mode.width_mm = mode->width_mm;
}

static void mtk_output_dbi_enable(struct mtk_dbi *dbi)
{
	int ret;

	dbi_dev_info(dbi->dev, "[%s] Enter, enabled = %d\n", __func__, dbi->enabled);

	if (dbi->enabled)
		return;

	ret = mtk_dbi_poweron(dbi);
	if (ret < 0) {
		DRM_ERROR("failed to power on dbi\n");
		return;
	}

#ifdef ENABLE_DBI_PATTERN
	mtk_dbi_set_pattern(dbi);
#endif

	dbi->enabled = true;

	// Add for debug
	mtk_dbi_dump_regs(dbi);

	dbi_dev_info(dbi->dev, "[%s] Exit\n", __func__);

	return;
//err_dbi_power_off:
//	mtk_dbi_stop(dbi);
//	mtk_dbi_poweroff(dbi);
}

static void mtk_dbi_encoder_enable(struct drm_encoder *encoder)
{
	struct mtk_dbi *dbi = encoder_to_dbi(encoder);

	mtk_output_dbi_enable(dbi);
}

static void mtk_output_dbi_disable(struct mtk_dbi *dbi)
{
	dbi_dev_info(dbi->dev, "[%s] Enter, enabled = %d\n", __func__, dbi->enabled);

	if (!dbi->enabled)
		return;

	if (dbi->panel) {
		if (drm_panel_disable(dbi->panel)) {
			DRM_ERROR("failed to disable the panel\n");
			return;
		}

		dbi->panel_enabled = false;
	}

	mtk_dbi_poweroff(dbi);

	dbi->enabled = false;

	dbi_dev_info(dbi->dev, "[%s] Exit\n", __func__);
}

static void mtk_dbi_encoder_disable(struct drm_encoder *encoder)
{
	struct mtk_dbi *dbi = encoder_to_dbi(encoder);

	mtk_output_dbi_disable(dbi);
}

static const struct drm_encoder_funcs mtk_dbi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_encoder_helper_funcs mtk_dbi_encoder_helper_funcs = {
	.mode_fixup = mtk_dbi_encoder_mode_fixup,
	.mode_set = mtk_dbi_encoder_mode_set,
	.disable = mtk_dbi_encoder_disable,
	.enable = mtk_dbi_encoder_enable,
};

static int mtk_dbi_create_conn_enc(struct drm_device *drm, struct mtk_dbi *dbi)
{
	int ret;

	ret = drm_encoder_init(drm, &dbi->encoder, &mtk_dbi_encoder_funcs,
				   DRM_MODE_ENCODER_DBI, NULL);
	if (ret) {
		DRM_ERROR("Failed to encoder init to drm\n");
		return ret;
	}
	drm_encoder_helper_add(&dbi->encoder, &mtk_dbi_encoder_helper_funcs);

	/*
	 * Currently display data paths are statically assigned to a crtc each.
	 * crtc 0 is OVL0 -> RDMA0 -> PQ modules -> DBI0 -> PWM
	 */
	dbi->encoder.possible_crtcs =
		mtk_drm_find_possible_crtc_by_comp(drm, dbi->ddp_comp);
	dbi_dev_info(dbi->dev, "[%s] dbi->encoder.possible_crtcs:%d\n", __func__,
		dbi->encoder.possible_crtcs);

	/* If there's a bridge, attach to it and let it create the connector */
	if (dbi->bridge) {
		ret = drm_bridge_attach(&dbi->encoder, dbi->bridge, NULL);
		if (ret) {
			DRM_ERROR("Failed to attach bridge to drm\n");
			goto err_encoder_cleanup;
		}
	} else {
		/* Otherwise create our own connector and attach to a panel */
		ret = mtk_dbi_create_connector(drm, dbi);
		if (ret)
			goto err_encoder_cleanup;
	}

	return 0;

err_encoder_cleanup:
	drm_encoder_cleanup(&dbi->encoder);
	dev_err(dbi->dev, "[%s] ret=%d\n", __func__, ret);
	return ret;
}

#ifdef ENABLE_UT // Add for testing
static void mtk_dbi_enable_for_testing(struct mtk_dbi *dbi)
{
	dbi->host_info.ctrl_type = MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI;
	dbi->host_info.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_125M;
	dbi->host_info.driving_current.grpup1 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	dbi->host_info.driving_current.grpup2 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	dbi->host_info.driving_current.grpup3 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	dbi->host_info.data_width = MTK_DBI_LCM_DATA_WIDTH_18BITS;
	dbi->host_info.color_order = MTK_DBI_LCM_COLOR_ORDER_RGB;
	dbi->host_info.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB666;

	dbi->host_info.te_control.software_te = 0;
	dbi->host_info.te_control.te_repeat = 0;
	dbi->host_info.te_control.sync_mode = 1;
	dbi->host_info.te_control.te_edge = 0;
	dbi->host_info.te_control.te_enable = 1;

	dbi->host_info.dbi_b_params.itf_size = MTK_DBI_LCM_INTERFACE_SIZE_9BITS;
	dbi->host_info.dbi_b_params.rw_timing.cs_to_read_hold_time = 1;
	dbi->host_info.dbi_b_params.rw_timing.cs_to_read_setup_time = 1;
	dbi->host_info.dbi_b_params.rw_timing.read_latency_time = 4;
	dbi->host_info.dbi_b_params.rw_timing.cs_to_write_hold_time = 1;
	dbi->host_info.dbi_b_params.rw_timing.cs_to_write_setup_time = 0;
	dbi->host_info.dbi_b_params.rw_timing.write_wait_state_time = 5;

	dbi->mode.hdisplay = 320;
	dbi->mode.vdisplay = 480;

	mtk_dbi_dump_regs(dbi);

	mtk_dbi_set_gpio(dbi);
	mtk_dbi_set_driving_current(dbi);
	mtk_output_dbi_enable(dbi);

	mtk_dbi_dump_regs(dbi);
}
#endif

static int mtk_dbi_bind(
		struct device *dev, struct device *master, void *data)
{
	int ret;
	struct drm_device *drm = data;
	struct mtk_dbi *dbi = dev_get_drvdata(dev);

	dbi_dev_info(dbi->dev, "[%s] enter\n", __func__);

	ret = mtk_ddp_comp_register(drm, &dbi->ddp_comp);
	if (ret < 0) {
		dev_err(dev, "Failed to register component %pOF: %d\n",
			dev->of_node, ret);
		return ret;
	}

	ret = mtk_dbi_create_conn_enc(drm, dbi);
	if (ret) {
		DRM_ERROR("Encoder create failed with %d\n", ret);
		goto err_unregister;
	}

	dbi_dev_info(dbi->dev, "[%s] exit\n", __func__);

	mtk_dbi_dump_regs(dbi);

	return 0;

err_unregister:
	dev_err(dbi->dev, "[%s] ret = %d\n", __func__, ret);
	mtk_ddp_comp_unregister(drm, &dbi->ddp_comp);
	return ret;
}

static void mtk_dbi_destroy_conn_enc(struct mtk_dbi *dbi)
{
	drm_encoder_cleanup(&dbi->encoder);
	/* Skip connector cleanup if creation was delegated to the bridge */
	if (dbi->conn.dev)
		drm_connector_cleanup(&dbi->conn);
	if (dbi->panel)
		drm_panel_detach(dbi->panel);
}

static void mtk_dbi_unbind(
		struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct mtk_dbi *dbi = dev_get_drvdata(dev);

	mtk_dbi_destroy_conn_enc(dbi);
	mtk_ddp_comp_unregister(drm, &dbi->ddp_comp);
}

static void mtk_dbi_ddp_atomic_flush(struct mtk_ddp_comp *comp, bool drm_release)
{
	struct mtk_dbi *dbi = container_of(comp, struct mtk_dbi, ddp_comp);

	dbi_dev_info(dbi->dev, "[%s] enter, enabled=%d, drm_release=%d",
			__func__, dbi->enabled, drm_release);

	if (dbi->enabled && !drm_release) {
		mtk_dbi_start(dbi);

		if (!dbi->panel_enabled) {
			if (dbi->panel) {
				if (drm_panel_enable(dbi->panel))
					DRM_ERROR("failed to enable the panel\n");
			}

			dbi->panel_enabled = true;
		}
	}
}

static const struct mtk_ddp_comp_funcs mtk_dbi_funcs = {
	.prepare = mtk_dbi_ddp_prepare,
	.unprepare = mtk_dbi_ddp_unprepare,
	.atomic_flush = mtk_dbi_ddp_atomic_flush,
};

static const struct component_ops mtk_dbi_component_ops = {
	.bind = mtk_dbi_bind,
	.unbind = mtk_dbi_unbind,
};

static const struct mipi_dbi_host_ops mtk_dbi_ops = {
	.send_cmd = mtk_dbi_send_cmd,
	.send_data = mtk_dbi_send_data,
	.read_data = mtk_dbi_read_data,
	.occupy_data_pin = mtk_dbi_occupy_data_pin,
	.release_data_pin = mtk_dbi_release_data_pin,
};

void mtk_dbi_host_attach(struct drm_panel *panel,
		struct mipi_dbi_device *host,
		const struct mipi_dbi_host_ops **ops)
{
	*ops = &mtk_dbi_ops;

	_panel = panel;
	_panel_attached = true;
	mtk_dbi_copy_host_info_data(&_host_info, host);
}

void mtk_dbi_host_detach(struct mipi_dbi_device *host)
{
	//host.ops = &mtk_dbi_ops;
}

static int mtk_dbi_probe(struct platform_device *pdev)
{
	struct mtk_dbi *dbi;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;

	struct resource *regs;
	int irq_num;
	int comp_id;
	int ret;
	//struct regmap *regmap;

	dbi = devm_kzalloc(dev, sizeof(*dbi), GFP_KERNEL);
	if (!dbi)
		return -ENOMEM;

	dbi->dev = dev;
	dbi->data = of_device_get_match_data(dev);
	dbi->poweron = 0;

#ifdef AUTO_RELEASE_DATA_PIN
	dbi->locked = false;
#endif

#ifndef CONFIG_DRM_DSI_SUPPORT
	dev_info(dbi->dev, "[%s] setting panel, panel_attached = %d\n", __func__, _panel_attached);

	if (!_panel_attached) {
		ret = -EPROBE_DEFER;
		dev_err(dbi->dev, "[%s] panel not attached, retry again test\n", __func__);
		goto error;
	}
	dbi->panel = _panel;
#endif
	mtk_dbi_copy_host_info_data(&dbi->host_info, &_host_info);

	mtk_dbi_nli_arb_init(dbi);
	mtk_dbi_set_gpio(dbi);
	mtk_dbi_set_driving_current(dbi);

	of_id = of_match_device(mtk_dbi_of_match, &pdev->dev);

	dbi->mm_clk = devm_clk_get(dev, "clk_mm_dbpi");
	if (IS_ERR(dbi->mm_clk)) {
		ret = PTR_ERR(dbi->mm_clk);
		dev_err(dev, "Failed to get mm clock: %d\n", ret);
		goto error;
	}

	dbi->mm_dbi_clk = devm_clk_get(dev, "clk_mm_dbi");
	if (IS_ERR(dbi->mm_dbi_clk)) {
		ret = PTR_ERR(dbi->mm_dbi_clk);
		dev_err(dev, "Failed to get clk_mm_dbi clock: %d\n", ret);
		goto error;
	}

	dbi->mm_dbi_sel = devm_clk_get(dev, "clk_dbi_sel");
	if (IS_ERR(dbi->mm_dbi_sel)) {
		ret = PTR_ERR(dbi->mm_dbi_sel);
		dev_err(dev, "Failed to get mm_dbi_sel clock: %d\n", ret);
		goto error;
	}

	dbi->mm_smi_clk = devm_clk_get(dev, "clk_smi_common");
	if (IS_ERR(dbi->mm_smi_clk)) {
		ret = PTR_ERR(dbi->mm_smi_clk);
		dev_err(dev, "Failed to get mm_smi_clk clock: %d\n", ret);
		goto error;
	}


	dbi->infra_nfi_clk = devm_clk_get(dev, "clk_infra_nfi");
	if (IS_ERR(dbi->infra_nfi_clk)) {
		ret = PTR_ERR(dbi->infra_nfi_clk);
		dev_err(dev, "Failed to get clk_infra_nfi: %d\n", ret);
		goto error;
	}

	// Get clock and top clock
	dbi->engine_clk = devm_clk_get(dev, "clk_engine");
	if (IS_ERR(dbi->engine_clk)) {
		ret = PTR_ERR(dbi->engine_clk);
		dev_err(dev, "Failed to get engine_clk: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_26 = devm_clk_get(dev, "clk_top_26");
	if (IS_ERR(dbi->parent_clk_26)) {
		ret = PTR_ERR(dbi->parent_clk_26);
		dev_err(dev, "Failed to get clk_top_26: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_78 = devm_clk_get(dev, "clk_top_78");
	if (IS_ERR(dbi->parent_clk_78)) {
		ret = PTR_ERR(dbi->parent_clk_78);
		dev_err(dev, "Failed to get clk_top_78: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_91 = devm_clk_get(dev, "clk_top_91");
	if (IS_ERR(dbi->parent_clk_91)) {
		ret = PTR_ERR(dbi->parent_clk_91);
		dev_err(dev, "Failed to get clk_top_91: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_104 = devm_clk_get(dev, "clk_top_104");
	if (IS_ERR(dbi->parent_clk_104)) {
		ret = PTR_ERR(dbi->parent_clk_104);
		dev_err(dev, "Failed to get clk_top_104: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_125 = devm_clk_get(dev, "clk_top_125");
	if (IS_ERR(dbi->parent_clk_125)) {
		ret = PTR_ERR(dbi->parent_clk_125);
		dev_err(dev, "Failed to get clk_top_125: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_137 = devm_clk_get(dev, "clk_top_137");
	if (IS_ERR(dbi->parent_clk_137)) {
		ret = PTR_ERR(dbi->parent_clk_137);
		dev_err(dev, "Failed to get clk_top_137: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_156 = devm_clk_get(dev, "clk_top_156");
	if (IS_ERR(dbi->parent_clk_156)) {
		ret = PTR_ERR(dbi->parent_clk_156);
		dev_err(dev, "Failed to get clk_top_156: %d\n", ret);
		goto error;
	}

	dbi->parent_clk_182 = devm_clk_get(dev, "clk_top_182");
	if (IS_ERR(dbi->parent_clk_182)) {
		ret = PTR_ERR(dbi->parent_clk_182);
		dev_err(dev, "Failed to get clk_top_182: %d\n", ret);
		goto error;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dbi->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(dbi->regs)) {
		ret = PTR_ERR(dbi->regs);
		dev_err(dev, "Failed to ioremap memory: %d\n", ret);
		goto error;
	}

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DBI);
	if (comp_id < 0) {
		dev_err(dev, "Failed to identify by alias: %d\n", comp_id);
		ret = comp_id;
		goto error;
	}

	dbi_dev_info(dbi->dev, "[%s] comp_id: %d\n", __func__, comp_id);

	ret = mtk_ddp_comp_init(dev, dev->of_node, &dbi->ddp_comp, comp_id,
				&mtk_dbi_funcs);
	if (ret) {
		dev_err(dev, "Failed to initialize component: %d\n", ret);
		goto error;
	}

	irq_num = platform_get_irq(pdev, 0);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "failed to get dbi irq_num: %d\n", irq_num);
		ret = irq_num;
		goto error;
	}


	irq_set_status_flags(irq_num, IRQ_TYPE_LEVEL_HIGH);

	ret = devm_request_irq(&pdev->dev, irq_num, mtk_dbi_irq,
				   IRQF_TRIGGER_NONE | IRQF_SHARED, dev_name(&pdev->dev), dbi);

	if (ret) {
		dev_err(&pdev->dev, "failed to request mediatek dbi irq\n");
		goto error;
	}

	// init_waitqueue_head(&dbi->irq_wait_queue);

	platform_set_drvdata(pdev, dbi);

	ret = component_add(&pdev->dev, &mtk_dbi_component_ops);
	if (ret) {
		dev_err(&pdev->dev, "failed to add component: %d\n", ret);
		goto error;
	}

        timer_setup(&dbi->locked_timer, mtk_dbi_locked_timer_callback, 0);

	return 0;

error:
	dev_err(dbi->dev, "[%s] exit, ret = %d\n", __func__, ret);
	return ret;
}

static int mtk_dbi_remove(struct platform_device *pdev)
{
	struct mtk_dbi *dbi = platform_get_drvdata(pdev);

	mtk_output_dbi_disable(dbi);
	mtk_dbi_stop(dbi);
	mtk_dbi_poweroff(dbi);
	component_del(&pdev->dev, &mtk_dbi_component_ops);

	return 0;
}

struct platform_driver mtk_dbi_driver = {
	.probe = mtk_dbi_probe,
	.remove = mtk_dbi_remove,
	.driver = {
		.name = "mtk-dbi",
		.of_match_table = mtk_dbi_of_match,
	},
};
