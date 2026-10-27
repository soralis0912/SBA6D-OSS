#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>

#define PMIC_TOP_INFO_RSV_ADDR \
MT6361_TOP_INFO2
#define PMIC_TOP_INFO_RSV_MASK 0x3
#define PMIC_TOP_INFO_RSV_SHIFT 4

#define MT6361_TOP_INFO2 (0x0001)

static int cpu_buck_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regulator *regulator;
	struct regmap *regmap;
	unsigned int value = 0;
	int ret = 0;

	regulator = devm_regulator_get_optional(dev, "vsram");
	if (IS_ERR(regulator)) {
		if (PTR_ERR(regulator) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
		}
		dev_err(dev, "No regulator found\n");
		regulator = NULL;
		return ret;
	}

	regmap = regulator_get_regmap(regulator);
	regmap_read(regmap, PMIC_TOP_INFO_RSV_ADDR, &value);
	if ((value >> PMIC_TOP_INFO_RSV_SHIFT) && PMIC_TOP_INFO_RSV_MASK != 0) {
		ret = regulator_enable(regulator);
		dev_err(dev, "np Sample!\n");
		if (ret)
			return ret;
	}

	return 0;
}

static int cpu_buck_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regulator *regulator;
	struct regmap *regmap;
	unsigned int value = 0;
	int ret = 0;

	regulator = devm_regulator_get_optional(dev, "vsram");
	if (IS_ERR(regulator)) {
		if (PTR_ERR(regulator) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
		}
		dev_err(dev, "No regulator found\n");
		regulator = NULL;
		return ret;
	}

	regmap = regulator_get_regmap(regulator);
	regmap_read(regmap, PMIC_TOP_INFO_RSV_ADDR, &value);
	if ((value >> PMIC_TOP_INFO_RSV_SHIFT) && PMIC_TOP_INFO_RSV_MASK != 0) {
		ret = regulator_disable(regulator);
		dev_err(dev, "cpu sram_proc disable!\n");
		if (ret)
			return ret;
	}

	return 0;
}

static const struct of_device_id cpu_buck_match[] = {
	{
		.compatible = "mediatek-cpu-sram-proc",
	},
	{
	},
};
MODULE_DEVICE_TABLE(of, lvts_of_match);

static struct platform_driver cpu_buck = {
	.probe = cpu_buck_probe,
	.remove = cpu_buck_remove,
	.driver = {
		.name = "mtk-cpu-buck",
		.of_match_table = cpu_buck_match,
	},
};

module_platform_driver(cpu_buck);
MODULE_AUTHOR("Meng-Hsun Chou <Meng-hsun.chou@mediatek.com>");
MODULE_DESCRIPTION("Mediatek soc temperature driver");
MODULE_LICENSE("GPL v2");