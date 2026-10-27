// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define QL_PCIE_SMT_CUSTOMIZED 1

#define IOCTL_DEV_IOCTLID	'P'
#define PCIE_SMT_TEST_SLOT	_IOW(IOCTL_DEV_IOCTLID, 0, int)

#define PCIE_SMT_DEV_NAME	"pcie_smt"
#define PCIE_SYSFS_NAME		"pcie_test"
#define PCIE_PORT_MAX		4

struct cmd_tbl {
	char *cmd_name;
	int (*cb_func)(int argc, char **argv);
	char *help;
};

struct mtk_pcie_info {
	char *name;
	void __iomem **regs;
	struct phy *phys[PCIE_PORT_MAX];
	struct clk_bulk_data *clks[PCIE_PORT_MAX];
	int num_clks[PCIE_PORT_MAX];
	unsigned int *test_lane;
	unsigned int *max_lane;
	unsigned int *max_speed;
	unsigned int max_port;

	/* char device */
	struct cdev smt_cdev;
	struct class *f_class;
	struct kobject *pcie_test_kobj;
};

/**
 * struct match_table - differentiate between IC
 * @compatible: compatible info of phy in dts
 * @phy_base: physical address of each port
 * @test_lane: number of test lane per port
 * @max_lane: number of max lane per port
 * @max_speed: max speed of each port
 * @max_port: max number of ports
 */
struct match_table {
	char compatible[128];
	unsigned int phy_base[PCIE_PORT_MAX];
	unsigned int test_lane[PCIE_PORT_MAX];
	unsigned int max_lane[PCIE_PORT_MAX];
	unsigned int max_speed[PCIE_PORT_MAX];
	unsigned int max_port;
};

static struct match_table test_table[] =
{
	/* Colgin PCIe info */
	{
	.compatible = "mediatek,mt6880-pcie-phy",
	.phy_base = {0x11e40000, 0x11e60000, 0x11e80000, 0x11ea0000},
	.test_lane = {1, 1, 1, 1},
	.max_lane = {2, 2, 1, 1},
	.max_speed = {3, 3, 3, 3},
	.max_port = 4,
	},
	/* Bolligner PCIe info */
	{
	.compatible = "mediatek,mt6980-pcie-phy",
	.phy_base = {0x11310000, 0x11e00000, 0x11e20000},
	.test_lane = {2, 2, 2},
	.max_lane = {2, 2, 2},
	.max_speed = {4, 3, 3},
	.max_port = 3,
	},
};

static struct mtk_pcie_info *pcie_smt;

#if QL_PCIE_SMT_CUSTOMIZED
#include <stdbool.h>
#define QL_PCIE_SMT_POWER_CTL 1
#if QL_PCIE_SMT_POWER_CTL
static bool ql_pcie_isPoweron=false;
#endif
#endif

static void mtk_pcie_cpe_gen1(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen1 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x30000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern  for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(8,40,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x08 << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x28 << 16);
	writel(val, phy_base + 0x3048);

	pr_info("Gen1 compliance setting completed!\n");
}

static void mtk_pcie_cpe_gen2_35db(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen2 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 16);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(8,40,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x08 << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x28 << 16);
	writel(val, phy_base + 0x3048);

	pr_info("Gen2 3.5db compliance setting completed!\n");
}

static void mtk_pcie_cpe_gen2_6db(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen2 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 16);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* TX -6dB: (Cp1,C0,Cm1)=(12,36,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x0c << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x24 << 16);
	writel(val, phy_base + 0x3048);

	pr_info("Gen2 6db compliance setting completed!\n");
}

static void mtk_pcie_cpe_gen3(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen3 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x02 << 16);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x28);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x28);

	/* P7 Settings */
	/* Set TX output Compliance Pattern */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* TX P7: (Cp1,C0,Cm1)=(10,34,4)*/
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffffc0ff) | (0x04 << 8);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x0a << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x22 << 16);
	writel(val, phy_base + 0x3048);

	pr_info("Gen3 compliance setting completed!\n");
}
#if QL_PCIE_SMT_CUSTOMIZED

static void ql_pcie_cpe_gen1_sphy2(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen1 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x30000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern  for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern  for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(8,40,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x08 << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x28 << 16);
	writel(val, phy_base + 0x3048);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(8,40,0) for lane1 */
	val = readl(phy_base + 0xa108);
	val &= (~0x200);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xc0ffffff) | (0x08 << 24);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xffc0ffff) | (0x28 << 16);
	writel(val, phy_base + 0x3148);

	pr_info("Gen1 compliance setting completed!\n");
}

static void ql_pcie_cpe_gen1_sphy4(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen1 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= ~GENMASK(15, 12);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(3, 0);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(7, 4);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(12,36,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x2064);
	val |= BIT(14);
	writel(val, phy_base + 0x2064);
	val |= BIT(15);
	writel(val, phy_base + 0x2064);
	val |= BIT(16);
	writel(val, phy_base + 0x2064);

	val = readl(phy_base + 0x2000);
	val &= ~GENMASK(29, 24);
	writel(val, phy_base + 0x2000);

	val = readl(phy_base + 0x2004);
	val &= ~GENMASK(5, 0);
	val |= 0x28;
	writel(val, phy_base + 0x2004);

	val &= ~GENMASK(13, 8);
	val |= BIT(11);
	writel(val, phy_base + 0x2004);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(12,36,0) for lane1 */
	val = readl(phy_base + 0xa108);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x2164);
	val |= BIT(14);
	writel(val, phy_base + 0x2164);
	val |= BIT(15);
	writel(val, phy_base + 0x2164);
	val |= BIT(16);
	writel(val, phy_base + 0x2164);

	val = readl(phy_base + 0x2100);
	val &= ~GENMASK(29, 24);
	writel(val, phy_base + 0x2100);

	val = readl(phy_base + 0x2104);
	val &= ~GENMASK(5, 0);
	val |= 0x24;
	writel(val, phy_base + 0x2104);

	val &= ~GENMASK(13, 8);
	val |= BIT(11);
	writel(val, phy_base + 0x2104);

	pr_info("Gen1 compliance setting completed!\n");
}

static void ql_pcie_cpe_gen2_35db_sphy2(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen2 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 16);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(8,40,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x08 << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x28 << 16);
	writel(val, phy_base + 0x3048);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(8,40,0) for lane1 */
	val = readl(phy_base + 0xa108);
	val &= (~0x200);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xc0ffffff) | (0x08 << 24);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xffc0ffff) | (0x28 << 16);
	writel(val, phy_base + 0x3148);

	pr_info("Gen2 3.5db compliance setting completed!\n");
}

static void ql_pcie_cpe_gen2_35db_sphy4(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen2 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= ~GENMASK(15, 12);
	val |= BIT(12);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(3, 0);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(7, 4);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(12,36,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x2064);
	val |= BIT(14);
	writel(val, phy_base + 0x2064);
	val |= BIT(15);
	writel(val, phy_base + 0x2064);
	val |= BIT(16);
	writel(val, phy_base + 0x2064);

	val = readl(phy_base + 0x2000);
	val &= ~GENMASK(29, 24);
	writel(val, phy_base + 0x2000);

	val = readl(phy_base + 0x2004);
	val &= ~GENMASK(5, 0);
	val |= 0x28;
	writel(val, phy_base + 0x2004);

	val &= ~GENMASK(13, 8);
	val |= BIT(11);
	writel(val, phy_base + 0x2004);

	/* TX -3.5dB: (Cp1,C0,Cm1)=(12,36,0) for lane1 */
	val = readl(phy_base + 0xa108);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x2164);
	val |= BIT(14);
	writel(val, phy_base + 0x2164);
	val |= BIT(15);
	writel(val, phy_base + 0x2164);
	val |= BIT(16);
	writel(val, phy_base + 0x2164);

	val = readl(phy_base + 0x2100);
	val &= ~GENMASK(29, 24);
	writel(val, phy_base + 0x2100);

	val = readl(phy_base + 0x2104);
	val &= ~GENMASK(5, 0);
	val |= 0x28;
	writel(val, phy_base + 0x2104);

	val &= ~GENMASK(13, 8);
	val |= BIT(11);
	writel(val, phy_base + 0x2104);

	pr_info("Gen2 3.5db compliance setting completed!\n");
}

static void ql_pcie_cpe_gen2_6db_sphy2(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen2 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= ~GENMASK(17, 16);
	val |= (0x01 << 16);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	/* TX -6dB: (Cp1,C0,Cm1)=(12,36,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x0c << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x24 << 16);
	writel(val, phy_base + 0x3048);

	/* TX -6dB: (Cp1,C0,Cm1)=(12,36,0) for lane1 */
	val = readl(phy_base + 0xa108);
	val &= (~0x200);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val &= (~0x3f00);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xc0ffffff) | (0x0c << 24);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xffc0ffff) | (0x24 << 16);
	writel(val, phy_base + 0x3148);

	pr_info("Gen2 6db compliance setting completed!\n");
}

static void ql_pcie_cpe_gen2_6db_sphy4(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen2 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= ~GENMASK(15, 12);
	val |= BIT(12);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(3, 0);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(7, 4);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	/* TX -6dB: (Cp1,C0,Cm1)=(12,36,0) for lane0 */
	val = readl(phy_base + 0xa008);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x2064);
	val |= BIT(14);
	writel(val, phy_base + 0x2064);
	val |= BIT(15);
	writel(val, phy_base + 0x2064);
	val |= BIT(16);
	writel(val, phy_base + 0x2064);

	val = readl(phy_base + 0x2000);
	val &= ~GENMASK(29, 24);
	writel(val, phy_base + 0x2000);

	val = readl(phy_base + 0x2004);
	val &= ~GENMASK(5, 0);
	val |= 0x24;
	writel(val, phy_base + 0x2004);

	val &= ~GENMASK(13, 8);
	val |= GENMASK(11, 10);
	writel(val, phy_base + 0x2004);

	/* TX -6dB: (Cp1,C0,Cm1)=(12,36,0) for lane1 */
	val = readl(phy_base + 0xa108);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x2164);
	val |= BIT(14);
	writel(val, phy_base + 0x2164);
	val |= BIT(15);
	writel(val, phy_base + 0x2164);
	val |= BIT(16);
	writel(val, phy_base + 0x2164);

	val = readl(phy_base + 0x2100);
	val &= ~GENMASK(29, 24);
	writel(val, phy_base + 0x2100);

	val = readl(phy_base + 0x2104);
	val &= ~GENMASK(5, 0);
	val |= 0x24;
	writel(val, phy_base + 0x2104);

	val &= ~GENMASK(13, 8);
	val |= GENMASK(11, 10);
	writel(val, phy_base + 0x2104);

	pr_info("Gen2 6db compliance setting completed!\n");
}

static void ql_pcie_cpe_gen3_sphy2(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen3 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= ~GENMASK(17, 16);
	val |= (0x02 << 16);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* P7 Settings */
	/* Set TX output Compliance Pattern */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= (~0x40);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for Lane1*/
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 7);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= (0x01 << 6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= (~0x40);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x28);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x28);

	/* TX P7: (Cp1,C0,Cm1)=(10,34,4)*/
	val = readl(phy_base + 0xa008);
	val &= (~0x200);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffffc0ff) | (0x04 << 8);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xc0ffffff) | (0x0a << 24);
	writel(val, phy_base + 0x3048);

	val = readl(phy_base + 0x3048);
	val = (val & 0xffc0ffff) | (0x22 << 16);
	writel(val, phy_base + 0x3048);

	/* TX P7: (Cp1,C0,Cm1)=(10,34,4) for Lane1*/
	val = readl(phy_base + 0xa108);
	val &= (~0x200);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 14);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 22);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xffffc0ff) | (0x04 << 8);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xc0ffffff) | (0x0a << 24);
	writel(val, phy_base + 0x3148);

	val = readl(phy_base + 0x3148);
	val = (val & 0xffc0ffff) | (0x22 << 16);
	writel(val, phy_base + 0x3148);

	pr_info("Gen3 compliance setting completed!\n");
}

static void ql_pcie_cpe_gen3_sphy4(unsigned int port)
{
	void __iomem *phy_base = pcie_smt->regs[port];
	int val;

	/* PCIe rate = Gen3 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= ~GENMASK(15, 12);
	val |= BIT(13);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force Pipe P0 */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(3, 0);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x74);
	val &= ~GENMASK(7, 4);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);

	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Compliance Pattern for lane0 */
	val = readl(phy_base + 0x4010);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4010);

	val = readl(phy_base + 0x4010);
	val |= BIT(6);
	writel(val, phy_base + 0x4010);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4010);
	val &= ~BIT(6);
	writel(val, phy_base + 0x4010);

	/* Set TX output Compliance Pattern for lane1 */
	val = readl(phy_base + 0x4110);
	val = (val & (~0xf)) | (0x06);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x4110);
	val |= BIT(6);
	writel(val, phy_base + 0x4110);

	usleep_range(100, 200);

	val = readl(phy_base + 0x4110);
	val &= ~BIT(6);
	writel(val, phy_base + 0x4110);

	val = readl(phy_base + 0x28);
	val |= BIT(30);
	writel(val, phy_base + 0x28);

	/* Lane 0 Tx P7 Preset */
	val = readl(phy_base + 0xa008);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa008);

	val = readl(phy_base + 0x2064);
	val |= BIT(14);
	writel(val, phy_base + 0x2064);

	val = readl(phy_base + 0x2064);
	val |= BIT(15);
	writel(val, phy_base + 0x2064);

	val = readl(phy_base + 0x2064);
	val |= BIT(16);
	writel(val, phy_base + 0x2064);

	val = readl(phy_base + 0x2000);
	val &= ~GENMASK(29, 24);
	val |= (0x4 << 24);
	writel(val, phy_base + 0x2000);

	val = readl(phy_base + 0x2004);
	val &= ~GENMASK(5, 0);
	val |= 0x22;
	writel(val, phy_base + 0x2004);

	val = readl(phy_base + 0x2004);
	val &= ~GENMASK(13, 8);
	val |= (0xa << 8);
	writel(val, phy_base + 0x2004);

	/* Lane 1 Tx P7 Preset */
	val = readl(phy_base + 0xa108);
	val &= ~BIT(5);
	writel(val, phy_base + 0xa108);

	val = readl(phy_base + 0x2164);
	val |= BIT(14);
	writel(val, phy_base + 0x2164);

	val = readl(phy_base + 0x2164);
	val |= BIT(15);
	writel(val, phy_base + 0x2164);

	val = readl(phy_base + 0x2164);
	val |= BIT(16);
	writel(val, phy_base + 0x2164);

	val = readl(phy_base + 0x2100);
	val &= ~GENMASK(29, 24);
	val |= (0x4 << 24);
	writel(val, phy_base + 0x2100);

	val = readl(phy_base + 0x2104);
	val &= ~GENMASK(5, 0);
	val |= 0x22;
	writel(val, phy_base + 0x2104);

	val = readl(phy_base + 0x2104);
	val &= ~GENMASK(13, 8);
	val |= (0xa << 8);
	writel(val, phy_base + 0x2104);

	pr_info("Gen3 compliance setting completed!\n");
}

#endif
/* for gen3 phy */
static int mtk_pcie_loopback_test_gen3(struct mtk_pcie_info *pcie_smt, unsigned int port)
{
	int val = 0, ret = 0, i = 0, err_count = 0;
	void __iomem *phy_base = pcie_smt->regs[port];

	pr_info("pcie loopback test start\n");

	/* L1ss = enable */
	val = readl(phy_base + 0x28);
	val |= (0x01 << 5);
	writel(val, phy_base + 0x28);

	val = readl(phy_base + 0x28);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x28);

	val = readl(phy_base + 0x28);
	val &= (~0x200);
	writel(val, phy_base + 0x28);

	val = readl(phy_base + 0x28);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x28);

	val = readl(phy_base + 0x28);
	val &= (~0x800);
	writel(val, phy_base + 0x28);

	val = readl(phy_base + 0x28);
	val |= (0x01 << 10);
	writel(val, phy_base + 0x28);

	/* Set Rate=Gen1 */
	usleep_range(100, 200);
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x30000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);
	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force PIPE (P0) */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xc00);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0x3000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);
	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set TX output Pattern for each lane */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		usleep_range(100, 200);
		val = readl(phy_base + 0x4010 + (0x100 * i));
		val = ((val & ~0xf) | 0x0d);
		writel(val, phy_base + 0x4010 + (0x100 * i));
	}

	/* Set TX PTG Enable */
	val = readl(phy_base + 0x28);
	val |= (0x01 << 30);
	writel(val, phy_base + 0x28);

	/* Set RX Pattern Checker (Type & Enable)  for each lane */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		val = readl(phy_base + 0x501c + (0x100 * i));
		val |= (0x01 << 1);
		writel(val, phy_base + 0x501c + (0x100 * i));

		val = readl(phy_base + 0x501c + (0x100 * i));
		val = ((val & ~0xf0) | 0xd0);
		writel(val, phy_base + 0x501c + (0x100 * i));
	}
	/* toggle ptc_en for status counter clear for each lane */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		val = readl(phy_base + 0x501c + (0x100 * i));
		val &= (~0x2);
		writel(val, phy_base + 0x501c + (0x100 * i));
		usleep_range(100, 200);
		val = readl(phy_base + 0x501c + (0x100 * i));
		val |= (0x01 << 1);
		writel(val, phy_base + 0x501c + (0x100 * i));
	}

	msleep(50);
	/* Check status */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		val = readl(phy_base + 0x50c8 + (0x100 * i));
		if ((val & 0x3) != 0x3) {
			err_count = val >> 12;
			pr_info("PCIe lane%i test failed: %#x!\n", i, val);
			pr_info("lane%i error count: %d\n", i, err_count);
			ret = -1;
		} else {
			pr_info("lane%i loopback test success! \n", i);
		}
	}

	return ret;
}

/* for GEN4 phy*/
static int mtk_pcie_loopback_test_gen4(struct mtk_pcie_info *pcie_smt, unsigned int port)
{
	int val = 0, ret = 0, i = 0, err_count = 0;
	void __iomem *phy_base = pcie_smt->regs[port];

	pr_info("pcie loopback test start\n");

	/* Set Rate=Gen1 */
	usleep_range(100, 200);
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val &= (~0xf000);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);
	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Force PIPE (P0) */
	val = readl(phy_base + 0x70);
	val |= (0x01);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x74);
	val &= (~0xf);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x74);
	val &= (~0xf0);
	writel(val, phy_base + 0x74);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 8);
	writel(val, phy_base + 0x70);

	val = readl(phy_base + 0x70);
	val |= (0x01 << 4);
	writel(val, phy_base + 0x70);

	usleep_range(100, 200);
	val = readl(phy_base + 0x70);
	val &= (~0x10);
	writel(val, phy_base + 0x70);

	/* Set T2R loopback */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		usleep_range(100, 200);
		val = readl(phy_base + 0x3000 + (0x100 * i));
		val |= 0x1;
		writel(val, phy_base + 0x3000 + (0x100 * i));
	}

	/* Set TX output Pattern for each lane */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		usleep_range(100, 200);
		val = readl(phy_base + 0x4010 + (0x100 * i));
		val = ((val & ~0xf) | 0x0d);
		writel(val, phy_base + 0x4010 + (0x100 * i));
	}

	/* Set TX PTG Enable */
	val = readl(phy_base + 0x28);
	val |= 0x40000000;
	writel(val, phy_base + 0x28);

	/* toggle ptc_en for status counter clear for each lane */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		val = readl(phy_base + 0x501c + (0x100 * i));
		val &= (~0x2);
		writel(val, phy_base + 0x501c + (0x100 * i));
		usleep_range(100, 200);
	}

	/* Set RX Pattern Checker (Type & Enable)  for each lane */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		val = readl(phy_base + 0x501c + (0x100 * i));
		val = ((val & ~0xf0) | 0xd0);
		writel(val, phy_base + 0x501c + (0x100 * i));

		val = readl(phy_base + 0x501c + (0x100 * i));
		val |= (0x01 << 1);
		writel(val, phy_base + 0x501c + (0x100 * i));
	}

	msleep(50);
	/* Check status */
	for (i = 0; i < pcie_smt->test_lane[port]; i++) {
		val = readl(phy_base + 0x50c8 + (0x100 * i));
		if ((val & 0x3) != 0x3) {
			err_count = val >> 16;
			pr_info("PCIe lane%i test failed: %#x!\n", i, val);
			pr_info("lane%i error count: %d\n", i, err_count);
			ret = -1;
		} else {
			pr_info("lane%i loopback test success! \n", i);
		}
	}

	return ret;
}

static int mtk_pcie_loopback_test(struct mtk_pcie_info *pcie_smt, unsigned int port)
{
	if (pcie_smt->max_speed[port] == 3)
		return mtk_pcie_loopback_test_gen3(pcie_smt, port);
	else if (pcie_smt->max_speed[port] == 4)
		return mtk_pcie_loopback_test_gen4(pcie_smt, port);

	pr_info("unsupport max speed!\n");

	return -1;
}

static int mtk_pcie_ioctl_loopback(struct mtk_pcie_info *pcie_smt, int port)
{
	if (!pcie_smt) {
		pr_info("pcie_smt not found\n");
		return -ENODEV;
	}

	if (!pcie_smt->regs[port]) {
		pr_info("phy_base is not initialed!\n");
		return -EINVAL;
	}

	if (port >= pcie_smt->max_port) {
		pr_info("Unsupported slot number: [%d]\n", port);
		return -EINVAL;
	}

	return mtk_pcie_loopback_test(pcie_smt, port);
}

static int mtk_pcie_test_open(struct inode *inode, struct file *file)
{
	struct mtk_pcie_info *pcie_smt;

	pcie_smt = container_of(inode->i_cdev, struct mtk_pcie_info, smt_cdev);
	file->private_data = pcie_smt;
	pr_info("mtk_pcie_test_open: successful\n");

	return 0;
}

static int mtk_pcie_test_release(struct inode *inode, struct file *file)
{

	pr_info("mtk_pcie_test_release: successful\n");
	return 0;
}

static ssize_t mtk_pcie_test_read(struct file *file, char *buf, size_t count,
		loff_t *ptr)
{

	pr_info("mtk_pcie_test_read: returning zero bytes\n");
	return 0;
}

static ssize_t mtk_pcie_test_write(struct file *file, const char *buf,
		size_t count, loff_t *ppos)
{

	pr_info("mtk_pcie_test_write: accepting zero bytes\n");
	return 0;
}

static long mtk_pcie_test_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct mtk_pcie_info *pcie_smt = file->private_data;

	switch (cmd) {
		case PCIE_SMT_TEST_SLOT:
			pr_info("pcie_smt port: %d\r\n", (unsigned int)arg);

			return mtk_pcie_ioctl_loopback(pcie_smt, (unsigned int)arg);
		default:
			return -ENOTTY;
	}

	return 0;
}

static const struct file_operations pcie_smt_fops = {
	.owner = THIS_MODULE,
	.read = mtk_pcie_test_read,
	.write = mtk_pcie_test_write,
	.unlocked_ioctl = mtk_pcie_test_ioctl,
	.compat_ioctl = mtk_pcie_test_ioctl,
	.open = mtk_pcie_test_open,
	.release = mtk_pcie_test_release,
};

static unsigned int mtk_get_value(char *str)
{
	unsigned int index, value = 0;
	char *format;

	if (str[0] == '0' && str[1] == 'x') {
		index = 2;
		format = "%x";
	} else {
		index = 0;
		format = "%d";
	}

	sscanf(&str[index], format, &value);
	return value;
}

static int mtk_pcie_sysfs_loopback(int argc, char **argv)
{
	unsigned int port = 0, lane = 1;
#if QL_PCIE_SMT_POWER_CTL
	if(!ql_pcie_isPoweron){
		pr_err("%s() power not ready\n", __func__);
		return -EINVAL;
	}
#endif
	if (argc > 1) {
		port = mtk_get_value(argv[1]);
		if (port >= pcie_smt->max_port) {
			pr_info("Unsupported port number: %d\n", port);
			return -EINVAL;
		} else {
			pr_info("Test port number is %d.\n", port);
		}
	}

	if (argc > 2) {
		lane = mtk_get_value(argv[2]);
		if (lane > pcie_smt->max_lane[port] || lane <= 0) {
			pr_info("Unsupported number of lane: %d*lane\n", lane);
			return -EINVAL;
		} else {
			pr_info("Test number of lane: %d*lane\n", lane);
			pcie_smt->test_lane[port] = lane;
		}
	}

	return mtk_pcie_loopback_test(pcie_smt, port);
}

static int mtk_pcie_sysfs_complaince(int argc, char **argv)
{
	unsigned int port = 0;
#if QL_PCIE_SMT_POWER_CTL
	if (!ql_pcie_isPoweron) {
		pr_err("%s() power not ready\n", __func__);
		return -EINVAL;
	}
#endif
	if (argc > 1) {
		port = mtk_get_value(argv[1]);
		if (port >= pcie_smt->max_port) {
			pr_err("Unsupported port number: %d\n", port);
			return -EINVAL;
		} else {
			pr_info("Test port number is %d.\n", port);
		}
	}

	if (argc > 2) {
#if QL_PCIE_SMT_CUSTOMIZED
		unsigned int ql_api = 0;
		if (!strncmp(argv[2], "ql_", 3)) {
			ql_api   = 3;
			argv[2] = argv[2] + ql_api;
		}
		pr_info("argv: %d, %s\n", ql_api, argv[2]);
#endif
		if (!strcmp(argv[2], "gen1")) {
			pr_info("Start Gen1 compliance test\n");
#if QL_PCIE_SMT_CUSTOMIZED
			if (ql_api > 0) {
			if (port)
					ql_pcie_cpe_gen1_sphy2(port);
			else
					ql_pcie_cpe_gen1_sphy4(port);	
			} else
#endif
			mtk_pcie_cpe_gen1(port);
		} else if (!strcmp(argv[2], "gen2_35db")) {
			pr_info("Start Gen2 3.5DB compliance test\n");
#if QL_PCIE_SMT_CUSTOMIZED
			if (ql_api > 0) {
			if (port)
					ql_pcie_cpe_gen2_35db_sphy2(port);
			else
					ql_pcie_cpe_gen2_35db_sphy4(port);	
			} else
#endif
			mtk_pcie_cpe_gen2_35db(port);
		} else if (!strcmp(argv[2], "gen2_6db")) {
			pr_info("Start Gen2 6DB compliance test\n");
#if QL_PCIE_SMT_CUSTOMIZED
			if (ql_api > 0) {
			if (port)
					ql_pcie_cpe_gen2_6db_sphy2(port);
			else
					ql_pcie_cpe_gen2_6db_sphy4(port);
			} else
#endif
			mtk_pcie_cpe_gen2_6db(port);
		} else if (!strcmp(argv[2], "gen3")) {
			if (pcie_smt->max_speed[port] >= 3) {
				pr_info("Start Gen3 compliance test\n");
#if QL_PCIE_SMT_CUSTOMIZED
				if (ql_api > 0) {
				if (port)
						ql_pcie_cpe_gen3_sphy2(port);
				else
						ql_pcie_cpe_gen3_sphy4(port);
				} else
#endif
				mtk_pcie_cpe_gen3(port);
			} else {
				pr_err("Unsupported speed: %s\n", argv[2]);
				return -EINVAL;
			}
		} else {
			pr_err("Unknow command: %s\n", argv[2]);
			return -EINVAL;
		}
	}

	return 0;
}
#if QL_PCIE_SMT_CUSTOMIZED
static int mtk_pcie_phy_power_on(struct mtk_pcie_info *smt_info);
static int mtk_pcie_ioctl_loopback(struct mtk_pcie_info *pcie_smt, int port);
static int ql_pcie_sysfs_complaince(int argc, char** argv) {
	int ret = 0;
	if (!pcie_smt) {
		pr_err("pcie_smt not ready");
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "poweron")) {
#if QL_PCIE_SMT_POWER_CTL
			if (!ql_pcie_isPoweron) {
				if (mtk_pcie_phy_power_on(pcie_smt)) {
				pr_err("mtk_pcie_phy_power_on failed");
				return -EINVAL;
				}
				ql_pcie_isPoweron = true;
			}
			pr_info("mtk_pcie_phy_power_on finished");
#endif
		} else if (!strcmp(argv[1], "loopback") && argc > 2) {
			int port = argv[2][0] - '0';
			if (port > pcie_smt->max_port) {
				pr_err("port oversize\n");
				return -EINVAL;
			}
			return mtk_pcie_ioctl_loopback(pcie_smt, port);
		} else {
			pr_err("unknow cmd\n");
			return -EINVAL;
		}
	}
	return ret;
}
#endif
static struct cmd_tbl cmd_table[] = {
	{	"loopback",
		&mtk_pcie_sysfs_loopback,
		"PCIe Loopback Test [port num] [num of lane]"
	},
	{	"cpe",
		&mtk_pcie_sysfs_complaince,
		"PCIe Compliance Test [port num] [cpe type]"
	},
#if QL_PCIE_SMT_CUSTOMIZED
	{	"ql",
		&ql_pcie_sysfs_complaince,
		"Quectel config [cmd] [argv]\n"
	},
#endif
	{}
};

static int mtk_pcie_test_ctrl(char *buf)
{
	struct cmd_tbl *cmd_list = cmd_table;
	char *argv[10];
	int argc = 0;

	do {
		argv[argc] = strsep(&buf, " ");
		pr_info("[%d] %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);

	while (cmd_list->cmd_name != NULL) {
		if (!strcmp(cmd_list->cmd_name, argv[0])) {
			if (cmd_list->cb_func != NULL) {
				return cmd_list->cb_func(argc, argv);
			}
			pr_err("Call back function not found.\n");
			break;
		}
		cmd_list++;
	}

	return -EINVAL;
}

static ssize_t cli_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	struct cmd_tbl *cmd = cmd_table;

	pr_info("Support commands:\n");

	while (cmd->cmd_name != NULL || cmd->help != NULL) {
		pr_info("%-20s %s\n", cmd->cmd_name, cmd->help);
		cmd++;
	}

	pr_info("***************Test cmd user guide****************\n");
	pr_info("[port num]       0, 1, 2 .... \n");
	pr_info("[num of lane]    1, 2, 4.\n");
	pr_info("[cpe type]       gen1, gen2_35db, gen2_6db, gen3.\n");
	pr_info("e.g: echo loopback 0 1 > cli \n");
	pr_info("e.g: echo cpe 0 gen2_6db > cli \n");
	pr_info("NOTE: Switch use loopback and cpe cmd need reboot.\n");
#if QL_PCIE_SMT_CUSTOMIZED
#if QL_PCIE_SMT_POWER_CTL
	pr_info("Quectel cmd: poweron\n");
	pr_info("e.g: echo ql poweron> cli \n");
	pr_info("e.g: echo cpe 0 ql_gen2_6db > cli \n");
#endif
	pr_info("Quectel lookback\n");
	pr_info("e.g: echo ql lookback 0 > cli \n");
#endif
	return 1;
}

static ssize_t cli_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t n)
{
	int ret = 0;
	char *ch;

	ch = kmalloc(n, GFP_KERNEL);
	memcpy(ch, buf, n);
	if (ch[n-1] == '\n')
		ch[n-1] = '\0';

	ret = mtk_pcie_test_ctrl(ch);
	if (ret < 0) {
		pr_err("pcie test cli fail\n");
		kfree(ch);
		return -EINVAL;
	}

	kfree(ch);
	pr_info("%s", buf);

	return n;
}

#define pcie_test_attr(_name) \
	static struct kobj_attribute _name##_attr = {   \
		.attr	= {.name = __stringify(_name),  \
			   .mode = 0644,                \
		},                                      \
		.show	= _name##_show,                 \
		.store	= _name##_store,                \
	}
pcie_test_attr(cli);

static struct attribute *pcie_test_func[] = {
	&cli_attr.attr,
	NULL,
};

static struct attribute_group pcie_test_kobj_group = {
	.attrs = pcie_test_func,
};

static int mtk_pcie_gpio_init(struct device *dev)
{
	struct pinctrl *pcie_gpio;
	struct pinctrl_state *state;

	pcie_gpio = devm_pinctrl_get(dev);
	if (IS_ERR(pcie_gpio)) {
		dev_info(dev, "Can't find PCIe gpio\n");
		return PTR_ERR(pcie_gpio);
	}

	state = pinctrl_lookup_state(pcie_gpio, "default");
	if (IS_ERR(state)) {
		dev_info(dev, "Can't find PCIe gpio default state\n");
		return PTR_ERR(state);
	}

	pinctrl_select_state(pcie_gpio, state);

	return 0;
}

static int mtk_pcie_phy_power_on(struct mtk_pcie_info *smt_info)
{
	struct platform_device *pdev;
	struct device_node *pcie_node = NULL;
	struct phy *pcie_phy;
	struct clk_bulk_data *clks;
	int port, err, num_clks;

	for (port = 0; port < smt_info->max_port; port++) {
		pcie_node = of_find_node_by_name(pcie_node, "pcie");
		if (!pcie_node)
			return -ENODEV;

		pdev = of_find_device_by_node(pcie_node);
		if (!pdev)
			return -ENODEV;

		num_clks = devm_clk_bulk_get_all(&pdev->dev, &clks);
		if (num_clks < 0) {
			pr_err("failed to get clocks\n");
			return num_clks;
		}

		err = mtk_pcie_gpio_init(&pdev->dev);
		if (err)
			return err;

		pcie_phy = devm_phy_optional_get(&pdev->dev, "pcie-phy");
		if (IS_ERR(pcie_phy))
			return PTR_ERR(pcie_phy);

		err = phy_init(pcie_phy);
		if (err)
			goto err_init;

		err = phy_power_on(pcie_phy);
		if (err)
			goto err_power_on;

		err = clk_bulk_prepare_enable(num_clks, clks);
		if (err) {
			pr_err("failed to enable clocks\n");
			goto err_clk_init;
		}

		smt_info->num_clks[port] = num_clks;
		smt_info->clks[port] = clks;
		smt_info->phys[port] = pcie_phy;
	}

	return 0;

err_clk_init:
	phy_power_off(pcie_phy);
err_power_on:
	phy_exit(pcie_phy);
err_init:
	pm_runtime_disable(&pcie_phy->dev);

	return err;
}

static void mtk_pcie_phy_power_off(struct mtk_pcie_info *smt_info)
{
	int port;

	for (port = 0; port < smt_info->max_port; port++) {
		struct phy *pcie_phy = smt_info->phys[port];

		clk_bulk_disable_unprepare(smt_info->num_clks[port],
					   smt_info->clks[port]);
		phy_power_off(pcie_phy);
		phy_exit(pcie_phy);
		pm_runtime_disable(&pcie_phy->dev);
	}
}

static int __init mtk_pcie_test_init(void)
{
	struct cdev *dev_ctx;
	struct device_node *np;
	dev_t dev;
	int ret, size, i, j;

	pcie_smt = kzalloc(sizeof(*pcie_smt), GFP_KERNEL);
	if (!pcie_smt) {
		pr_info("pcie_smt alloc failed\n");
		return -ENOMEM;
	}

	/* match test table*/
	size = sizeof(test_table)/sizeof(struct match_table);
	for (i = 0; i < size; i++) {
		np = of_find_compatible_node(NULL, NULL, test_table[i].compatible);
		if (np != NULL) {
			pr_info("Test table match success!\n");
			break;
		} else if (i == size -1) {
			pr_err("Test table match failed!\n");
			kfree(pcie_smt);
			return -1;
		}
	}

	/* Init struct pcie_smt */
	pcie_smt->name = PCIE_SMT_DEV_NAME;
	pcie_smt->max_port = test_table[i].max_port;
	pcie_smt->test_lane = &test_table[i].test_lane[0];
	pcie_smt->max_lane = &test_table[i].max_lane[0];
	pcie_smt->max_speed = &test_table[i].max_speed[0];

	pcie_smt->regs = kzalloc(
			(pcie_smt->max_port) * (sizeof(void __iomem)), GFP_KERNEL);
	if (!pcie_smt->regs) {
		pr_info("pcie_smt->regs alloc failed\n");
		kfree(pcie_smt);
		return -ENOMEM;
	}

	for (j = 0; j < pcie_smt->max_port; j++)
		pcie_smt->regs[j] = ioremap(test_table[i].phy_base[j], 0x10000);

#if !QL_PCIE_SMT_POWER_CTL
	ret = mtk_pcie_phy_power_on(pcie_smt);
	if (ret)
		goto release;
#endif

	/* dev node support */
	dev_ctx = &pcie_smt->smt_cdev;

	ret = alloc_chrdev_region(&dev, 0, 1, pcie_smt->name);
	if (ret) {
		pr_info("mtk_pcie_test_init alloc_chrdev_region failed -\n");
		goto err_alloc_cdev_region;
	}

	cdev_init(dev_ctx, &pcie_smt_fops);
	dev_ctx->owner = THIS_MODULE;

	ret = cdev_add(dev_ctx, dev, 1);
	if (ret) {
		pr_info("failed to add cdev\n");
		goto err_alloc_cdev;
	}

	pcie_smt->f_class = class_create(THIS_MODULE, pcie_smt->name);
	device_create(pcie_smt->f_class, NULL, dev_ctx->dev, NULL, pcie_smt->name);

	/* sysfs support */
	pcie_smt->pcie_test_kobj = kobject_create_and_add(PCIE_SYSFS_NAME, NULL);
	if (IS_ERR(pcie_smt->pcie_test_kobj)) {
		pr_info("PCIe test kobject creat fail!\n");
		ret = PTR_ERR(pcie_smt->pcie_test_kobj);
		goto err_kobj_create;
	}

	ret = sysfs_create_group(pcie_smt->pcie_test_kobj, &pcie_test_kobj_group);
	if (ret) {
		pr_info("PCIe test sysfs creat fail\n");
		goto err_sysfs_create;
	}

	return 0;

err_sysfs_create:
	kobject_del(pcie_smt->pcie_test_kobj);
err_kobj_create:
	device_destroy(pcie_smt->f_class, dev_ctx->dev);
	class_destroy(pcie_smt->f_class);
err_alloc_cdev:
	unregister_chrdev_region(dev, MINORMASK);
err_alloc_cdev_region:
#if !QL_PCIE_SMT_POWER_CTL
	mtk_pcie_phy_power_off(pcie_smt);
release:
#endif
	for (j = 0; j < pcie_smt->max_port; j++)
		iounmap(pcie_smt->regs[j]);

	kfree(pcie_smt->regs);
	kfree(pcie_smt);

	return ret;
}

static void __exit mtk_pcie_test_exit(void)
{
	struct cdev *dev_ctx = &pcie_smt->smt_cdev;
	int port;

	/* remove char device */
	device_destroy(pcie_smt->f_class, dev_ctx->dev);
	class_destroy(pcie_smt->f_class);
	unregister_chrdev_region(dev_ctx->dev, MINORMASK);

	/* remove sysfs */
	sysfs_remove_group(pcie_smt->pcie_test_kobj, &pcie_test_kobj_group);
	kobject_del(pcie_smt->pcie_test_kobj);

	for (port = 0; port < pcie_smt->max_port; port++)
		iounmap(pcie_smt->regs[port]);

	mtk_pcie_phy_power_off(pcie_smt);

	kfree(pcie_smt->regs);
	kfree(pcie_smt);
}

module_init(mtk_pcie_test_init);
module_exit(mtk_pcie_test_exit);
