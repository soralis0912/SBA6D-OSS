// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Jianjun Wang <jianjun.wang@mediatek.com>
 */

#include <linux/bitfield.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#include "phy-mtk-io.h"

#define PEXTP_ANA_GLB_00_REG		0x9000
/* Internal Resistor Selection of TX Bias Current */
#define EFUSE_GLB_INTR_SEL		GENMASK(28, 24)

#define PEXTP_ANA_LN0_TRX_REG		0xa000

#define PEXTP_ANA_TX_REG		0x04
/* TX PMOS impedance selection */
#define EFUSE_LN_TX_PMOS_SEL		GENMASK(5, 2)
/* TX NMOS impedance selection */
#define EFUSE_LN_TX_NMOS_SEL		GENMASK(11, 8)

#define PEXTP_ANA_RX_REG		0x3c
/* RX impedance selection */
#define EFUSE_LN_RX_SEL			GENMASK(3, 0)

#define PEXTP_ANA_LANE_OFFSET		0x100

/* 6980 syscon regs */
#define PCIE_DUAL_2LX1_MAC_CON		0x8d8
#define PCIE_MODE_CON			BIT(7)
/* Gen3 PHY regs */
#define PEXTP_ANA_LN_TRX_30		0x30
#define RG_XTP_LN_RX_CDR_SEL		GENMASK(1, 0)

/* Gen4 SPHY regs*/
#define PEXTP_SIFSLV_DIG_LN_RX		0x5000
#define PEXTP_DIG_LN_RX_8C		0x8c
#define LEQ_GEN4_BWSEL			GENMASK(19, 15)

#define PEXTP_SIFSLV_DIG_LN_RX2	0x6000
#define PEXTP_DIG_LN_RX2_60		0x60
#define CDR_GEN4_FASTP_GAIN		GENMASK(15, 12)

#define PEXTP_DIG_LN_RX2_78		0x78
#define CDR_GEN3_IRR_GAIN		GENMASK(8, 6)
#define CDR_GEN3_IRR_CORNER		GENMASK(23, 21)

#define PEXTP_DIG_LN_RX2_D4		0xd4
#define RX_AEQ_GEN3_CYC_EN		BIT(18)
#define RX_AEQ_GEN4_CYC_EN		BIT(19)

#define PEXTP_SIFSLV_ANA_LN_TRX	0xa000
#define PEXTP_ANA_LN_TRX8		0x20
#define RG_XTP_LN_RX_EYES_OPTION2	GENMASK(29, 28)

#define PEXTP_ANA_LN_TRX9		0x24
#define RG_XTP_LN_RX_EYES_PAT		GENMASK(15, 0)


#define PCIE_PHY0_ID			0
#define PCIE_PHY1_ID			1
#define PCIE_PHY2_ID			2

/**
 * struct mtk_pcie_lane_efuse - eFuse data for each lane
 * @tx_pmos: TX PMOS impedance selection data
 * @tx_nmos: TX NMOS impedance selection data
 * @rx_data: RX impedance selection data
 * @lane_efuse_supported: software eFuse data is supported for this lane
 */
struct mtk_pcie_lane_efuse {
	u32 tx_pmos;
	u32 tx_nmos;
	u32 rx_data;
	bool lane_efuse_supported;
};

/**
 * struct mtk_pcie_phy_data - phy data for each SoC
 * @num_lanes: supported lane numbers
 * @sw_efuse_supported: support software to load eFuse data
 * @phy_init: special init function for each SoC
 */
struct mtk_pcie_phy_data {
	int num_lanes;
	bool sw_efuse_supported;
	int (*phy_init)(struct phy *phy);
};

/**
 * struct mtk_pcie_phy - PCIe phy driver main structure
 * @dev: pointer to device
 * @phy: pointer to generic phy
 * @sif_base: IO mapped register base address of system interface
 * @mode: indicates the mode of the PHY
 * @id: indicates PHY id
 * @data: pointer to SoC dependent data
 * @sw_efuse_en: software eFuse enable status
 * @efuse_glb_intr: internal resistor selection of TX bias current data
 * @efuse: pointer to eFuse data for each lane
 */
struct mtk_pcie_phy {
	struct device *dev;
	struct phy *phy;
	void __iomem *sif_base;
	enum phy_mode mode;
	int id;
	const struct mtk_pcie_phy_data *data;

	bool sw_efuse_en;
	u32 efuse_glb_intr;
	struct mtk_pcie_lane_efuse *efuse;
};

/**
 * struct chipid - SoC chip version information
 */
struct chipid {
	u32 size;
	u32 hw_code;
	u32 hw_subcode;
	u32 hw_ver;
	u32 sw_ver;
};

static int mtk_get_chipid(void)
{
	struct device_node *node;
	struct chipid *chip_id;
	int len;

	node = of_find_node_by_path("/chosen");
	if (!node)
		node = of_find_node_by_path("/chosen@0");

	if (!node) {
		pr_info("chosen node not found in device tree\n");
		return -ENODEV;
	}

	chip_id = (struct chipid *)of_get_property(node, "atag,chipid", &len);
	if (!chip_id) {
		pr_info("could not found atag,chipid in chosen\n");
		return -ENODEV;
	}

	pr_info("current sw version: %s\n", chip_id->sw_ver ? "E2" : "E1");

	return chip_id->sw_ver;
}

static void mtk_pcie_efuse_set_lane(struct mtk_pcie_phy *pcie_phy,
				    unsigned int lane)
{
	struct mtk_pcie_lane_efuse *data = &pcie_phy->efuse[lane];
	void __iomem *addr;

	if (!data->lane_efuse_supported)
		return;

	addr = pcie_phy->sif_base + PEXTP_ANA_LN0_TRX_REG +
	       lane * PEXTP_ANA_LANE_OFFSET;

	mtk_phy_update_bits(addr + PEXTP_ANA_TX_REG, EFUSE_LN_TX_PMOS_SEL,
			    FIELD_PREP(EFUSE_LN_TX_PMOS_SEL, data->tx_pmos));

	mtk_phy_update_bits(addr + PEXTP_ANA_TX_REG, EFUSE_LN_TX_NMOS_SEL,
			    FIELD_PREP(EFUSE_LN_TX_NMOS_SEL, data->tx_nmos));

	mtk_phy_update_bits(addr + PEXTP_ANA_RX_REG, EFUSE_LN_RX_SEL,
			    FIELD_PREP(EFUSE_LN_RX_SEL, data->rx_data));
}

/**
 * mtk_pcie_phy_init() - Initialize the phy
 * @phy: the phy to be initialized
 *
 * Initialize the phy by setting the efuse data.
 * The hardware settings will be reset during suspend, it should be
 * reinitialized when the consumer calls phy_init() again on resume.
 */
static int mtk_pcie_phy_init(struct phy *phy)
{
	struct mtk_pcie_phy *pcie_phy = phy_get_drvdata(phy);
	int i, ret;

	if (pcie_phy->data->phy_init) {
		ret = pcie_phy->data->phy_init(phy);
		if (ret)
			return ret;
	}

	if (!pcie_phy->sw_efuse_en)
		return 0;

	/* Set global data */
	mtk_phy_update_bits(pcie_phy->sif_base + PEXTP_ANA_GLB_00_REG,
			    EFUSE_GLB_INTR_SEL,
			    FIELD_PREP(EFUSE_GLB_INTR_SEL, pcie_phy->efuse_glb_intr));

	for (i = 0; i < pcie_phy->data->num_lanes; i++)
		mtk_pcie_efuse_set_lane(pcie_phy, i);

	return 0;
}

static int mtk_pcie_phy_exit(struct phy *phy)
{
	return 0;
}

static int mtk_pcie_phy_power_on(struct phy *phy)
{
	struct mtk_pcie_phy *pcie_phy = phy_get_drvdata(phy);

	pm_runtime_get_sync(pcie_phy->dev);

	return 0;
}

static int mtk_pcie_phy_power_off(struct phy *phy)
{
	struct mtk_pcie_phy *pcie_phy = phy_get_drvdata(phy);

	pm_runtime_put_sync(pcie_phy->dev);

	return 0;
}

static int mtk_pcie_phy_set_mode(struct phy *phy, enum phy_mode mode, int subm)
{
	struct mtk_pcie_phy *pcie_phy = phy_get_drvdata(phy);

	pcie_phy->mode = mode;

	return 0;
}

static const struct phy_ops mtk_pcie_phy_ops = {
	.init		= mtk_pcie_phy_init,
	.exit		= mtk_pcie_phy_exit,
	.power_on	= mtk_pcie_phy_power_on,
	.power_off	= mtk_pcie_phy_power_off,
	.set_mode	= mtk_pcie_phy_set_mode,
	.owner		= THIS_MODULE,
};

static void mtk_6980_e2_init(struct mtk_pcie_phy *pcie_phy)
{
	void __iomem *addr;
	int lane;

	for (lane = 0; lane < pcie_phy->data->num_lanes; lane++) {
		addr = pcie_phy->sif_base + PEXTP_SIFSLV_ANA_LN_TRX +
					lane * PEXTP_ANA_LANE_OFFSET;
		/* E2 lane margin setting*/
		if (pcie_phy->id == PCIE_PHY0_ID) {
			mtk_phy_update_bits(addr + PEXTP_ANA_LN_TRX8,
					RG_XTP_LN_RX_EYES_OPTION2, 0x3 << 28);
			mtk_phy_update_bits(addr + PEXTP_ANA_LN_TRX9,
					RG_XTP_LN_RX_EYES_PAT, 0xa800);
		}
		/* E2 fix FEDIG timing issue, need add SWWA setting */
		if (pcie_phy->id == PCIE_PHY1_ID || pcie_phy->id == PCIE_PHY2_ID)
			mtk_phy_update_bits(addr + PEXTP_ANA_LN_TRX_30,
					RG_XTP_LN_RX_CDR_SEL, 0);
	}
}

static int mtk_6980_phy_init(struct phy *phy)
{
	struct mtk_pcie_phy *pcie_phy = phy_get_drvdata(phy);
	struct device_node *node;
	struct regmap *cfg;
	void __iomem *addr;
	int sw_ver, lane;

	sw_ver = mtk_get_chipid();
	if (sw_ver == 1)
		mtk_6980_e2_init(pcie_phy);

	if (pcie_phy->mode == PHY_MODE_PCIE_EP)
		return 0;

	if (pcie_phy->id == PCIE_PHY0_ID) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mt6990-scpsys");
		if(!node)
			return 0;

		cfg = syscon_node_to_regmap(node);
		if (IS_ERR(cfg))
			return PTR_ERR(cfg);

		regmap_update_bits(cfg, PCIE_DUAL_2LX1_MAC_CON, PCIE_MODE_CON, PCIE_MODE_CON);

		/* update Gen3 PHYD interface and add Gen4 RX performance */
		for (lane = 0; lane < pcie_phy->data->num_lanes; lane++) {
			addr = pcie_phy->sif_base + PEXTP_SIFSLV_DIG_LN_RX +
						lane * PEXTP_ANA_LANE_OFFSET;
			mtk_phy_update_bits(addr + PEXTP_DIG_LN_RX_8C,
						LEQ_GEN4_BWSEL, 0x17 << 15);

			addr = pcie_phy->sif_base + PEXTP_SIFSLV_DIG_LN_RX2 +
					lane * PEXTP_ANA_LANE_OFFSET;
			mtk_phy_update_bits(addr + PEXTP_DIG_LN_RX2_78,
						CDR_GEN3_IRR_GAIN, 0x1 << 6);
			mtk_phy_update_bits(addr + PEXTP_DIG_LN_RX2_78,
						CDR_GEN3_IRR_CORNER, 0x4 << 21);
			mtk_phy_update_bits(addr + PEXTP_DIG_LN_RX2_60,
						CDR_GEN4_FASTP_GAIN, 0x7 << 12);
			mtk_phy_clear_bits(addr + PEXTP_DIG_LN_RX2_D4, RX_AEQ_GEN3_CYC_EN);
			mtk_phy_clear_bits(addr + PEXTP_DIG_LN_RX2_D4, RX_AEQ_GEN4_CYC_EN);
		}
	}

	return 0;
}

static int mtk_pcie_efuse_read_for_lane(struct mtk_pcie_phy *pcie_phy,
					unsigned int lane)
{
	struct mtk_pcie_lane_efuse *efuse = &pcie_phy->efuse[lane];
	struct device *dev = pcie_phy->dev;
	char efuse_id[16];
	int ret;

	snprintf(efuse_id, sizeof(efuse_id), "tx_ln%d_pmos", lane);
	ret = nvmem_cell_read_u32(dev, efuse_id, &efuse->tx_pmos);
	if (ret) {
		dev_err(dev, "Failed to read %s\n", efuse_id);
		return ret;
	}

	snprintf(efuse_id, sizeof(efuse_id), "tx_ln%d_nmos", lane);
	ret = nvmem_cell_read_u32(dev, efuse_id, &efuse->tx_nmos);
	if (ret) {
		dev_err(dev, "Failed to read %s\n", efuse_id);
		return ret;
	}

	snprintf(efuse_id, sizeof(efuse_id), "rx_ln%d", lane);
	ret = nvmem_cell_read_u32(dev, efuse_id, &efuse->rx_data);
	if (ret) {
		dev_err(dev, "Failed to read %s\n", efuse_id);
		return ret;
	}

	if (!(efuse->tx_pmos || efuse->tx_nmos || efuse->rx_data)) {
		dev_err(dev, "No eFuse data found for lane%d, but dts enable it\n", lane);
		return -EINVAL;
	}

	efuse->lane_efuse_supported = true;

	return 0;
}

static int mtk_pcie_read_efuse(struct mtk_pcie_phy *pcie_phy)
{
	struct device *dev = pcie_phy->dev;
	bool nvmem_enabled;
	int ret, i;

	/* nvmem data is optional */
	nvmem_enabled = device_property_present(dev, "nvmem-cells");
	if (!nvmem_enabled)
		return 0;

	ret = nvmem_cell_read_u32(dev, "glb_intr",
					      &pcie_phy->efuse_glb_intr);
	if (ret) {
		dev_err(dev, "Failed to read glb_intr\n");
		return ret;
	}

	pcie_phy->sw_efuse_en = true;

	pcie_phy->efuse = devm_kzalloc(dev, pcie_phy->data->num_lanes *
				       sizeof(*pcie_phy->efuse), GFP_KERNEL);
	if (!pcie_phy->efuse)
		return -ENOMEM;

	for (i = 0; i < pcie_phy->data->num_lanes; i++) {
		ret = mtk_pcie_efuse_read_for_lane(pcie_phy, i);
		if (ret)
			return ret;
	}

	return 0;
}

static int mtk_pcie_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *provider;
	struct resource *res;
	struct mtk_pcie_phy *pcie_phy;
	int ret;

	pcie_phy = devm_kzalloc(dev, sizeof(*pcie_phy), GFP_KERNEL);
	if (!pcie_phy)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sif");
	pcie_phy->sif_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pcie_phy->sif_base)) {
		dev_err(dev, "Failed to map phy-sif base\n");
		return PTR_ERR(pcie_phy->sif_base);
	}

	pcie_phy->phy = devm_phy_create(dev, dev->of_node, &mtk_pcie_phy_ops);
	if (IS_ERR(pcie_phy->phy)) {
		dev_err(dev, "Failed to create PCIe phy\n");
		return PTR_ERR(pcie_phy->phy);
	}

	pcie_phy->dev = dev;
	pcie_phy->data = of_device_get_match_data(dev);
	if (!pcie_phy->data) {
		dev_err(dev, "Failed to get phy data\n");
		return -EINVAL;
	}

	if(of_property_read_u32(dev->of_node, "phy-id", &pcie_phy->id))
		pcie_phy->id = -1;

	pm_runtime_enable(pcie_phy->dev);

	if (pcie_phy->data->sw_efuse_supported) {
		/*
		 * Failed to read the efuse data is not a fatal problem,
		 * ignore the failure and keep going.
		 */
		ret = mtk_pcie_read_efuse(pcie_phy);
		if (ret == -EPROBE_DEFER || ret == -ENOMEM)
			return ret;
	}

	phy_set_drvdata(pcie_phy->phy, pcie_phy);

	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(provider)) {
		dev_err(dev, "PCIe phy probe failed\n");
		return PTR_ERR(provider);
	}

	return 0;
}

static const struct mtk_pcie_phy_data mt8195_data = {
	.num_lanes = 2,
	.sw_efuse_supported = true,
};

static const struct mtk_pcie_phy_data mt6980_data = {
	.num_lanes = 2,
	.sw_efuse_supported = false,
	.phy_init = mtk_6980_phy_init,
};

static const struct of_device_id mtk_pcie_phy_of_match[] = {
	{ .compatible = "mediatek,mt8195-pcie-phy", .data = &mt8195_data },
	{ .compatible = "mediatek,mt6980-pcie-phy", .data = &mt6980_data },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_pcie_phy_of_match);

static struct platform_driver mtk_pcie_phy_driver = {
	.probe	= mtk_pcie_phy_probe,
	.driver	= {
		.name = "mtk-pcie-phy",
		.of_match_table = mtk_pcie_phy_of_match,
	},
};
module_platform_driver(mtk_pcie_phy_driver);

MODULE_DESCRIPTION("MediaTek PCIe PHY driver");
MODULE_AUTHOR("Jianjun Wang <jianjun.wang@mediatek.com>");
MODULE_LICENSE("GPL");
