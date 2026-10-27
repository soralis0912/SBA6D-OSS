/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2019 MediaTek Inc.
 * Copyright (C) 2009-2016 John Crispin <blogic@openwrt.org>
 * Copyright (C) 2009-2016 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2013-2016 Michael Lee <igvtee@gmail.com>
 */

#include <linux/trace_seq.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/u64_stats_sync.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/of_mdio.h>
#include <linux/regmap.h>

#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"
#include "mtk_common.h"
#include "mtk_hwlro_dbg.h"
#include "mtk_sfq.h"
#include "mtk_sgmii_pwr.h"

static struct mtk_eth *g_eth;
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
extern struct mtk_gmii *mtk_gmii_glb[2][3];
#endif

unsigned int M2Q_table[64] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
	21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,57,58,59,60,61,62,63};
EXPORT_SYMBOL(M2Q_table);


void mt7530_mdio_w32(struct mtk_eth *eth, u32 reg, u32 val)
{
	mutex_lock(&eth->mii_bus->mdio_lock);

	mtk_mdio_write_ext(eth, 0x1f, 0x1f, (reg >> 6) & 0x3ff);
	mtk_mdio_write_ext(eth, 0x1f, (reg >> 2) & 0xf,  val & 0xffff);
	mtk_mdio_write_ext(eth, 0x1f, 0x10, val >> 16);

	mutex_unlock(&eth->mii_bus->mdio_lock);
}

u32 mt7530_mdio_r32(struct mtk_eth *eth, u32 reg)
{
	u16 high, low;

	mutex_lock(&eth->mii_bus->mdio_lock);

	mtk_mdio_write_ext(eth, 0x1f, 0x1f, (reg >> 6) & 0x3ff);
	low = mtk_mdio_read_ext(eth, 0x1f, (reg >> 2) & 0xf);
	high = mtk_mdio_read_ext(eth, 0x1f, 0x10);

	mutex_unlock(&eth->mii_bus->mdio_lock);

	return (high << 16) | (low & 0xffff);
}

void mtk_switch_w32(struct mtk_eth *eth, u32 val, unsigned reg)
{
	mtk_w32(eth, val, reg + 0x10000);
}
EXPORT_SYMBOL(mtk_switch_w32);

u32 mtk_switch_r32(struct mtk_eth *eth, unsigned reg)
{
	return mtk_r32(eth, reg + 0x10000);
}
EXPORT_SYMBOL(mtk_switch_r32);


int mtk_eth_search_port_by_mac_table(const u8 *ptr_mac, u32 vid, int *port){
	u32 i, value;
	u32 hw_mac = 0;
	u8 port_bmp = 0;

	if (ptr_mac == NULL)
		return -1;

	pr_info("%s = %x %x %x %x %x %x\n", __func__, ptr_mac[0],
		ptr_mac[1], ptr_mac[2], ptr_mac[3], ptr_mac[4], ptr_mac[5]);
	hw_mac = ptr_mac[0] << 24 | ptr_mac[1] << 16 |
		ptr_mac[2] << 8 | ptr_mac[3];
	mii_mgr_write_combine(g_eth, 0x1f, REG_ATA1_ADDR, hw_mac);
	hw_mac = ptr_mac[4] << 8 | ptr_mac[5];
	value = hw_mac << 16;
	value |= 1 << 15;	//IVL=0
	value |= vid;
	mii_mgr_write_combine(g_eth, 0x1f, REG_ATA2_ADDR, value);
	value = 0x8000; //w_mac_cmd
	mii_mgr_write_combine(g_eth, 0x1f, REG_ATC_ADDR, value);
	//usleep_range(500, 1000);
	for (i = 0; i < 20; i++) {
		mii_mgr_read_combine(g_eth, 0x1f, REG_ATC_ADDR, &value);
		if ((value & 0x8000) == 0) { //mac address busy
			break;
		}
		//usleep_range(500, 1000);
	}
	if (i == 20) {
		pr_info(" %x : search timeout.\n", __func__);
		return -1;
	}
	if (value & 0x1000) {
		pr_info(" %x : search no entry.\n", __func__);
		return -1;
	}
	mii_mgr_read_combine(g_eth, 0x1f, REG_ATRD_ADDR, &value);
	port_bmp = (value >> 4) & 0xff; //r_port_map
	//pr_info(" %x : value %x port_bmp %x\n", __func__, value, port_bmp);
	for (i = 0; i < 8; i++) {
		if ((1 << i) & port_bmp)
			break;
	}
	pr_info(" %x : *port = %d\n", __func__, i);
	*port = i;
	return 0;
}
EXPORT_SYMBOL(mtk_eth_search_port_by_mac_table);





static int mtketh_debug_show(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;
	struct mtk_mac *mac = 0;
	u32 d;
	int  i, j = 0;

	for (i = 0 ; i < MTK_MAX_DEVS ; i++) {
		if (!eth->mac[i] ||
		    of_phy_is_fixed_link(eth->mac[i]->of_node))
			continue;
		mac = eth->mac[i];

		while (j < 30) {
			d =  mtk_mdio_read_ext(eth, mac->id, j);

			seq_printf(m, "phy=%d, reg=0x%08x, data=0x%08x\n",
				   mac->id, j, d);
			j++;
		}
	}
	return 0;
}

static int mtketh_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtketh_debug_show, inode->i_private);
}

static const struct file_operations mtketh_debug_fops = {
	.open = mtketh_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mtketh_mt7530sw_debug_show(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;
	u32  offset, data;
	int i;
	struct mt7530_ranges {
		u32 start;
		u32 end;
	} ranges[] = {
		{0x0, 0xac},
		{0x1000, 0x10e0},
		{0x1100, 0x1140},
		{0x1200, 0x1240},
		{0x1300, 0x1340},
		{0x1400, 0x1440},
		{0x1500, 0x1540},
		{0x1600, 0x1640},
		{0x1800, 0x1848},
		{0x1900, 0x1948},
		{0x1a00, 0x1a48},
		{0x1b00, 0x1b48},
		{0x1c00, 0x1c48},
		{0x1d00, 0x1d48},
		{0x1e00, 0x1e48},
		{0x1f60, 0x1ffc},
		{0x2000, 0x212c},
		{0x2200, 0x222c},
		{0x2300, 0x232c},
		{0x2400, 0x242c},
		{0x2500, 0x252c},
		{0x2600, 0x262c},
		{0x3000, 0x3014},
		{0x30c0, 0x30f8},
		{0x3100, 0x3114},
		{0x3200, 0x3214},
		{0x3300, 0x3314},
		{0x3400, 0x3414},
		{0x3500, 0x3514},
		{0x3600, 0x3614},
		{0x4000, 0x40d4},
		{0x4100, 0x41d4},
		{0x4200, 0x42d4},
		{0x4300, 0x43d4},
		{0x4400, 0x44d4},
		{0x4500, 0x45d4},
		{0x4600, 0x46d4},
		{0x4f00, 0x461c},
		{0x7000, 0x7038},
		{0x7120, 0x7124},
		{0x7800, 0x7804},
		{0x7810, 0x7810},
		{0x7830, 0x7830},
		{0x7a00, 0x7a7c},
		{0x7b00, 0x7b04},
		{0x7e00, 0x7e04},
		{0x7ffc, 0x7ffc},
	};

	if (!mt7530_exist(eth))
		return -EOPNOTSUPP;

	if ((!eth->mac[0] || !of_phy_is_fixed_link(eth->mac[0]->of_node)) &&
	    (!eth->mac[1] || !of_phy_is_fixed_link(eth->mac[1]->of_node))) {
		seq_puts(m, "no switch found\n");
		return 0;
	}

	for (i = 0 ; i < ARRAY_SIZE(ranges) ; i++) {
		for (offset = ranges[i].start;
		     offset <= ranges[i].end; offset += 4) {
			data =  mt7530_mdio_r32(eth, offset);
			seq_printf(m, "mt7530 switch reg=0x%08x, data=0x%08x\n",
				   offset, data);
		}
	}

	return 0;
}

static int mtketh_debug_mt7530sw_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtketh_mt7530sw_debug_show, inode->i_private);
}

static const struct file_operations mtketh_debug_mt7530sw_fops = {
	.open = mtketh_debug_mt7530sw_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mtketh_mt7530sw_debugfs_write(struct file *file,
					     const char __user *ptr,
					     size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	char buf[32], *token, *p = buf;
	unsigned long reg, value, phy;
	int ret;

	if (!mt7530_exist(eth))
		return -EOPNOTSUPP;

	if (*off != 0)
		return 0;

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, ptr, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&phy))
		return -EINVAL;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&reg))
		return -EINVAL;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&value))
		return -EINVAL;

	pr_notice("%s:phy=%d, reg=0x%lx, val=0x%lx\n", __func__,
		0x1f, reg, value);
	mt7530_mdio_w32(eth, reg, value);
	pr_notice("%s:phy=%d, reg=0x%lx, val=0x%x confirm..\n", __func__,
		0x1f, reg, mt7530_mdio_r32(eth, reg));

	return len;
}

static ssize_t mtketh_debugfs_write(struct file *file, const char __user *ptr,
				    size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	char buf[32], *token, *p = buf;
	unsigned long reg, value, phy;
	int ret;

	if (*off != 0)
		return 0;

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, ptr, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&phy))
		return -EINVAL;

	token = strsep(&p, " ");

	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&reg))
		return -EINVAL;

	token = strsep(&p, " ");

	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&value))
		return -EINVAL;

	pr_notice("%s:phy=%ld, reg=0x%lx, val=0x%lx\n", __func__,
		phy, reg, value);

	mtk_mdio_write_ext(eth, phy,  reg, value);

	pr_notice("%s:phy=%ld, reg=0x%lx, val=0x%x confirm..\n", __func__,
		phy, reg, mtk_mdio_read_ext(eth, phy, reg));

	return len;
}

static ssize_t mtketh_debugfs_reset(struct file *file, const char __user *ptr,
				    size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;

	schedule_work(&eth->pending_work);
	return len;
}

static const struct file_operations fops_reg_w = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_debugfs_write,
	.llseek = noop_llseek,
};

static const struct file_operations fops_eth_reset = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_debugfs_reset,
	.llseek = noop_llseek,
};

static const struct file_operations fops_mt7530sw_reg_w = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_mt7530sw_debugfs_write,
	.llseek = noop_llseek,
};

void mtketh_debugfs_exit(struct mtk_eth *eth)
{
	debugfs_remove_recursive(eth->debug.root);
}

int mtketh_debugfs_init(struct mtk_eth *eth)
{
	int ret = 0;

	eth->debug.root = debugfs_create_dir("mtketh", NULL);
	if (!eth->debug.root) {
		dev_notice(eth->dev, "%s:err at %d\n", __func__, __LINE__);
		ret = -ENOMEM;
	}

	debugfs_create_file("phy_regs", S_IRUGO,
			    eth->debug.root, eth, &mtketh_debug_fops);
	debugfs_create_file("phy_reg_w", S_IFREG | S_IWUSR,
			    eth->debug.root, eth,  &fops_reg_w);
	debugfs_create_file("reset", S_IFREG | S_IWUSR,
			    eth->debug.root, eth,  &fops_eth_reset);
	if (mt7530_exist(eth)) {
		debugfs_create_file("mt7530sw_regs", S_IRUGO,
				    eth->debug.root, eth,
				    &mtketh_debug_mt7530sw_fops);
		debugfs_create_file("mt7530sw_reg_w", S_IFREG | S_IWUSR,
				    eth->debug.root, eth,
				    &fops_mt7530sw_reg_w);
	}
	return ret;
}

void mii_mgr_read_combine(struct mtk_eth *eth, u32 phy_addr, u32 phy_register,
			  u32 *read_data)
{
	if (mt7530_exist(eth) && phy_addr == 31) {
		*read_data = mt7530_mdio_r32(eth, phy_register);

	} else {
		mutex_lock(&eth->mii_bus->mdio_lock);
		*read_data = mtk_mdio_read_ext(eth, phy_addr, phy_register);
		mutex_unlock(&eth->mii_bus->mdio_lock);
	}
}

void mii_mgr_write_combine(struct mtk_eth *eth, u32 phy_addr, u32 phy_register,
			   u32 write_data)
{
	if (mt7530_exist(eth) && phy_addr == 31) {
		mt7530_mdio_w32(eth, phy_register, write_data);

	} else {
		mutex_lock(&eth->mii_bus->mdio_lock);
		mtk_mdio_write_ext(eth, phy_addr, phy_register, write_data);
		mutex_unlock(&eth->mii_bus->mdio_lock);
	}
}

static void mii_mgr_read_cl45(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 *data)
{
	mtk_cl45_ind_read(eth, port, devad, reg, data);
}

static void mii_mgr_write_cl45(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 data)
{
	mtk_cl45_ind_write(eth, port, devad, reg, data);
}


int mtk_qdma_ioctl(struct mtk_eth *eth, struct ifreq *ifr,  struct mtk_qdma_ioctl_data *data)
{
	int ret = 0;
	unsigned int page = 0;
	unsigned int tmp = 0, orig = 0;
	int offset =0;
	unsigned int bit_shift;


	pr_info("%s cmd:%x id: %x val: %x mask: %x\n",
		__func__, data->cmd, data->queue_id, data->val, data->mask);

	switch (data->cmd) {
	case MTKETH_QDMA_SCH_READ:
		if (data->queue_id <= 15) {
			page = 0;
			offset =0;
		} else if (data->queue_id > 15 && data->queue_id <= 31) {
			page = 1;
			offset = -16;
		} else if (data->queue_id > 31 && data->queue_id <= 47) {
			page = 2;
			offset = -32;
		} else if (data->queue_id > 47 && data->queue_id <= 63) {
			page = 3;
			offset = -48;
		} else {
			pr_notice("%s invalid value %x\n",  __func__, data->queue_id);
			ret = -EINVAL;
			break;
		}
		mtk_w32(eth, page, MTK_QDMA_SEL);
		pr_info("shift = %x\n", MTK_QTX_SCH(data->queue_id + offset));
		data->val = mtk_r32(eth, MTK_QTX_SCH(data->queue_id + offset));
		mtk_w32(eth, 0, MTK_QDMA_SEL);
		pr_info("%s read reg off:%x val:%x\n", __func__, data->queue_id, data->val);
		ret = copy_to_user(ifr->ifr_data, data, sizeof(*data));
		if (ret) {
			pr_info("ret=%d\n", ret);
			ret = -EFAULT;
		}
		break;
	case MTKETH_QDMA_SCH_WRITE:
		if (data->queue_id <= 15) {
			page = 0;
			offset = 0;
		} else if (data->queue_id > 15 && data->queue_id <= 31) {
			page = 1;
			offset = -16;
		} else if (data->queue_id > 31 && data->queue_id <= 47) {
			page = 2;
			offset = -32;
		} else if (data->queue_id > 47 && data->queue_id <= 63) {
			page = 3;
			offset = -48;
		} else {
			pr_notice("%s invalid value %x\n",  __func__, data->queue_id);
			ret = -EINVAL;
			break;
		}
		mtk_w32(eth, page, MTK_QDMA_SEL);
		pr_info("shift = %x\n", MTK_QTX_SCH((data->queue_id + offset)));
		orig = mtk_r32(eth, MTK_QTX_SCH((data->queue_id + offset)));
		tmp = orig & ~data->mask;
		tmp |= data->val & data->mask;
		mtk_w32(eth, tmp , MTK_QTX_SCH((data->queue_id + offset)));
		mtk_w32(eth, 0, MTK_QDMA_SEL);
		break;
	case MTKETH_QDMA_SCH_RATE_WRITE:
		pr_info("%s  SCH %d rate = %d\n", __func__, data->queue_id, data->val);
		switch (data->queue_id) {
			case 0:
				offset = MTK_QDMA_TX_SCH1_SCH2;
				bit_shift = 0;
				break;
			case 1:
				offset = MTK_QDMA_TX_SCH1_SCH2;
				bit_shift = 16;

				break;
			case 2:
				offset = MTK_QDMA_TX_SCH3_SCH4;
				bit_shift = 0;

				break;
			case 3:
				offset = MTK_QDMA_TX_SCH3_SCH4;
				bit_shift = 16;
				break;
			default:
				pr_notice("%s invalid value %x\n",  __func__, data->cmd);
				return ret;
		}
		orig = mtk_r32(eth, offset);
		tmp = orig & ~(data->mask << bit_shift);
		tmp |= (data->val & data->mask) << bit_shift;
		mtk_w32(eth, tmp , offset);
		break;
	case MTKETH_QDMA_QUEUE_MAPPING:
		pr_info("%s  QUEUE MAP %d = %x\n", __func__, data->queue_id, data->val);
		if (data->queue_id < 64)
			M2Q_table[data->queue_id] = data->val;
		break;
	case MTKETH_QDMA_SCH_POLICY_WRITE:
		pr_info("%s  SCH %d policy = %d\n", __func__, data->queue_id, data->val);
		switch (data->queue_id) {
			case 0:
				offset = MTK_QDMA_TX_SCH1_SCH2;
				bit_shift = 15;
				break;
			case 1:
				offset = MTK_QDMA_TX_SCH1_SCH2;
				bit_shift = 31;

				break;
			case 2:
				offset = MTK_QDMA_TX_SCH3_SCH4;
				bit_shift = 15;

				break;
			case 3:
				offset = MTK_QDMA_TX_SCH3_SCH4;
				bit_shift = 31;
				break;
			default:
				pr_notice("%s invalid value %x\n",  __func__, data->cmd);
				return ret;
		}
		orig = mtk_r32(eth, offset);
		tmp = orig & ~(1 << bit_shift);
		tmp |= (data->val & 0x1) << bit_shift;
		mtk_w32(eth, tmp , offset);
		break;
	default:
		pr_notice("%s invalid command %x\n",  __func__, data->cmd);
		ret = -EINVAL;
		break;
	}
	return ret;
}

int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	struct mtk_mii_ioctl_data mii;
	struct mtk_esw_reg reg;
	struct mtk_qdma_ioctl_data qdma_data;
	int ret = 0;

	switch (cmd) {
	case MTKETH_MII_READ:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_read_combine(eth, mii.phy_id, mii.reg_num,
				     &mii.val_out);
		if (copy_to_user(ifr->ifr_data, &mii, sizeof(mii)))
			goto err_copy;

		return 0;
	case MTKETH_MII_WRITE:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_write_combine(eth, mii.phy_id, mii.reg_num,
				      mii.val_in);

		return 0;
	case MTKETH_MII_READ_CL45:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_read_cl45(eth, mii.port_num, mii.dev_addr, mii.reg_addr,
				  &mii.val_out);
		if (copy_to_user(ifr->ifr_data, &mii, sizeof(mii)))
			goto err_copy;

		return 0;
	case MTKETH_MII_WRITE_CL45:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_write_cl45(eth, mii.port_num, mii.dev_addr, mii.reg_addr,
				   mii.val_in);
		return 0;
	case MTKETH_ESW_REG_READ:
		if (!mt7530_exist(eth))
			return -EOPNOTSUPP;
		if (copy_from_user(&reg, ifr->ifr_data, sizeof(reg)))
			goto err_copy;
		if (reg.off > REG_ESW_MAX)
			return -EINVAL;
		reg.val = mtk_switch_r32(eth, reg.off);

		if (copy_to_user(ifr->ifr_data, &reg, sizeof(reg)))
			goto err_copy;

		return 0;
	case MTKETH_ESW_REG_WRITE:
		if (!mt7530_exist(eth))
			return -EOPNOTSUPP;
		if (copy_from_user(&reg, ifr->ifr_data, sizeof(reg)))
			goto err_copy;
		if (reg.off > REG_ESW_MAX)
			return -EINVAL;
		mtk_switch_w32(eth, reg.val, reg.off);
		return 0;
	case MTKETH_QDMA_IOCTL:
		if (copy_from_user(&qdma_data, ifr->ifr_data, sizeof(qdma_data)))
			goto err_copy;
		ret = mtk_qdma_ioctl(eth, ifr, &qdma_data);
		return ret;
	default:
		pr_notice("%s ioctl %d is not supported", __func__, cmd);
		break;
	}

	return -EOPNOTSUPP;
err_copy:
	return -EFAULT;
}




int qos_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	u32 i,j;

	seq_puts(seq, " 	<<SCH CONFIG>>\n");
	seq_printf(seq, "SCH1_SCH2:\t0x%08x\n", mtk_r32(eth, MTK_QDMA_TX_SCH1_SCH2));
	seq_printf(seq, "SCH3_SCH4:\t0x%08x\n", mtk_r32(eth, MTK_QDMA_TX_SCH3_SCH4));

	mtk_w32(eth, 0, MTK_QDMA_SEL);
	seq_puts(seq, " 	<<TXQ SCH>>\n");
	for (j =0; j < 4 ; j++) {
		mtk_w32(eth, j, MTK_QDMA_SEL);
		for (i = 0; i < 16; i+=4) {
			seq_printf(seq, "%d:\t0x%08x 0x%08x 0x%08x 0x%08x\n", (i+ (j*0x10)),
				mtk_r32(eth, MTK_QTX_SCH(i)), mtk_r32(eth, MTK_QTX_SCH((i+1))),
				mtk_r32(eth, MTK_QTX_SCH((i+2))), mtk_r32(eth, MTK_QTX_SCH((i+3))));
		}
	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "\n");

	mtk_w32(eth, 0, MTK_QDMA_SEL);

	seq_puts(seq, " 	<<M2Q Table>>\n");
	for (i = 0; i < 64; i+= 4) {
		seq_printf(seq, "%d:\t0x%08x 0x%08x 0x%08x 0x%08x\n", i,
			M2Q_table[i], M2Q_table[i+1], M2Q_table[i+2], M2Q_table[i+3]);
	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "\n");

	return 0;
}

int sfq_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	sfq_print_status(eth);

	return 0;
}

int sgmii_dump_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	unsigned int cr_base;
	int cr_max;
	u32 i;
	unsigned int val0 = 0, val1 = 0, val2 = 0, val3 = 0;
	struct mtk_sgmii *ss = eth->sgmii;

	cr_base = 0x11304000;
	cr_max = 0xF0;

	if (ss->regmap[0]) {
		seq_puts(seq, "       <<SGMII CR DUMP>>\n");
		for (i = 0x0; i <= cr_max; i = i + 0x10) {
			regmap_read(ss->regmap[0], i, &val0);
			regmap_read(ss->regmap[0], (i + 4), &val1);
			regmap_read(ss->regmap[0], (i + 8), &val2);
			regmap_read(ss->regmap[0], (i + 0xc), &val3);
			seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
				val0, val1, val2, val3);
		}
		regmap_read(ss->regmap[0], (0x128), &val3);
		seq_puts(seq, "\n");
		seq_printf(seq, "0x%08x : 0x%08x\n", cr_base + 0x128, val3);
		seq_puts(seq, "+-----------------------------------------------+\n");
	}

	cr_base = 0x11305000;
	cr_max = 0xF0;
	if (ss->regmap[1]) {
		seq_puts(seq, "       <<SGMII CR DUMP>>\n");
		for (i = 0x0; i <= cr_max; i = i + 0x10) {
			regmap_read(ss->regmap[1], i, &val0);
			regmap_read(ss->regmap[1], (i + 4), &val1);
			regmap_read(ss->regmap[1], (i + 8), &val2);
			regmap_read(ss->regmap[1], (i + 0xc), &val3);
			seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
				val0, val1, val2, val3);
		}
		seq_puts(seq, "\n");
		regmap_read(ss->regmap[1], (0x128), &val3);
		seq_printf(seq, "0x%08x : 0x%08x\n", cr_base + 0x128, val3);

		seq_puts(seq, "+-----------------------------------------------+\n");
	}

	if (ss->regmap_uxphy[0]) {

		cr_base = 0x11306000;
		cr_max = 0x1000;

		seq_puts(seq, "       <<USXGMII 0 CR DUMP>>\n");
		for (i = 0x800; i <= cr_max; i = i + 0x10) {
			regmap_read(ss->regmap_uxphy[0], i, &val0);
			regmap_read(ss->regmap_uxphy[0], (i + 4), &val1);
			regmap_read(ss->regmap_uxphy[0], (i + 8), &val2);
			regmap_read(ss->regmap_uxphy[0], (i + 0xc), &val3);
			seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
				val0, val1, val2, val3);
		}
		seq_puts(seq, "\n");
		seq_puts(seq, "+-----------------------------------------------+\n");
	}


	if (ss->regmap_uxphy[1]) {

		cr_base = 0x11307000;
		cr_max = 0x1000;

		seq_puts(seq, " 	  <<USXGMII 1 CR DUMP>>\n");
		for (i = 0x800; i <= cr_max; i = i + 0x10) {
			regmap_read(ss->regmap_uxphy[1], i, &val0);
			regmap_read(ss->regmap_uxphy[1], (i + 4), &val1);
			regmap_read(ss->regmap_uxphy[1], (i + 8), &val2);
			regmap_read(ss->regmap_uxphy[1], (i + 0xc), &val3);
			seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
				val0, val1, val2, val3);
		}
		seq_puts(seq, "\n");
		seq_puts(seq, "+-----------------------------------------------+\n");
	}



	if (ss->regmap_phy[0]) {

		cr_base = 0x11E50000;
		cr_max = 0x10000;

		seq_puts(seq, "       <<UX PHY 0 CR DUMP>>\n");
		for (i = 0x0; i <= cr_max; i = i + 0x10) {
			regmap_read(ss->regmap_phy[0], i, &val0);
			regmap_read(ss->regmap_phy[0], (i + 4), &val1);
			regmap_read(ss->regmap_phy[0], (i + 8), &val2);
			regmap_read(ss->regmap_phy[0], (i + 0xc), &val3);
			seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
				val0, val1, val2, val3);
		}
		seq_puts(seq, "\n");
		seq_puts(seq, "+-----------------------------------------------+\n");
	}

	if (ss->regmap_phy[1]) {

		cr_base = 0x11E60000;
		cr_max = 0x10000;

		seq_puts(seq, "       <<UX PHY 1 CR DUMP>>\n");
		for (i = 0x0; i <= cr_max; i = i + 0x10) {
			regmap_read(ss->regmap_phy[1], i, &val0);
			regmap_read(ss->regmap_phy[1], (i + 4), &val1);
			regmap_read(ss->regmap_phy[1], (i + 8), &val2);
			regmap_read(ss->regmap_phy[1], (i + 0xc), &val3);
			seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
				val0, val1, val2, val3);
		}
		seq_puts(seq, "\n");
		seq_puts(seq, "+-----------------------------------------------+\n");
	}

	return 0;
}


int cr_dump_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	unsigned int cr_base;
	int cr_max;
	u32 i;

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	cr_base = 0x15100000;
	cr_max = 0x2900;
	seq_puts(seq, "       <<FE CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, i), mtk_r32(eth, i + 4),
			mtk_r32(eth, i + 8), mtk_r32(eth, i + 0xc));
	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	cr_base = 0x15100000 + QDMA_BASE;
	cr_max = 0x400;
	seq_puts(seq, "       <<QDMA CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, QDMA_BASE + i), mtk_r32(eth, (QDMA_BASE + i + 4)),
			mtk_r32(eth, (QDMA_BASE + i + 8)), mtk_r32(eth, (QDMA_BASE + i + 0xc)));
	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	cr_base = 0x15100000 + PDMA_BASE;
	cr_max = 0x300;
	seq_puts(seq, "       <<ADMA CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, PDMA_BASE + i), mtk_r32(eth, (PDMA_BASE + i + 4)),
			mtk_r32(eth, (PDMA_BASE + i + 8)), mtk_r32(eth, (PDMA_BASE + i + 0xc)));

	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	cr_base = 0x15100000 + LRO_BASE;
	cr_max = 0x200;
	seq_puts(seq, "       <<LRO CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, LRO_BASE + i), mtk_r32(eth, (LRO_BASE + i + 4)),
			mtk_r32(eth, (LRO_BASE + i + 8)), mtk_r32(eth, (LRO_BASE + i + 0xc)));

	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	cr_base = 0x15100000 + RSS_BASE;
	cr_max = 0x100;
	seq_puts(seq, "       <<RSS CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, RSS_BASE + i), mtk_r32(eth, (RSS_BASE + i + 4)),
			mtk_r32(eth, (RSS_BASE + i + 8)), mtk_r32(eth, (RSS_BASE + i + 0xc)));

	}
	seq_puts(seq, "+-----------------------------------------------+\n");


	seq_puts(seq, "+-----------------------------------------------+\n");
	cr_base = 0x15100000 + 0x11000;
	cr_max = 0x550;
	seq_puts(seq, "       <<XFI MAC 0 CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, 0x11000 + i), mtk_r32(eth, (0x11000 + i + 4)),
			mtk_r32(eth, (0x11000 + i + 8)), mtk_r32(eth, (0x11000 + i + 0xc)));

	}
	seq_puts(seq, "+-----------------------------------------------+\n");

	seq_puts(seq, "+-----------------------------------------------+\n");
	cr_base = 0x15100000 + 0x12000;
	cr_max = 0x550;
	seq_puts(seq, " 	  <<XFI MAC 1 CR DUMP>>\n");
	for (i = 0x0; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, 0x12000 + i), mtk_r32(eth, (0x12000 + i + 4)),
			mtk_r32(eth, (0x12000 + i + 8)), mtk_r32(eth, (0x12000 + i + 0xc)));

	}
	seq_puts(seq, "+-----------------------------------------------+\n");

#else
	cr_base = 0x15100000;
	cr_max = 5000 * 4;
	seq_puts(seq, "       <<FE CR DUMP>>\n");
	for (i = 0x0; i < 0x230; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, i), mtk_r32(eth, i + 4),
			mtk_r32(eth, i + 8), mtk_r32(eth, i + 0xc));
	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	for (i = 0x400; i < cr_max; i = i + 0x10) {
		seq_printf(seq, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			mtk_r32(eth, i), mtk_r32(eth, i + 4),
			mtk_r32(eth, i + 8), mtk_r32(eth, i + 0xc));
	}
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "\n");

#endif
	return 0;
}

int esw_cnt_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	u32 tmp, i;
	unsigned int cr_base;
	u32 tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0;

	cr_base = 0x10000;

	tmp1 = mtk_r32(eth, 0x8);
	mtk_w32(eth, tmp1, 0x8);
	tmp2 = mtk_r32(eth, 0x28);
	mtk_w32(eth, tmp2, 0x28);

	seq_puts(seq, "       <<PSE DROP CNT>>\n");
	seq_printf(seq, "| FE_INT_STATUS        : %x |\n", tmp1);
	seq_printf(seq, "| FE_INT_STATUS2        : %x |\n", tmp2);

	seq_printf(seq, "| FQ_PCNT_MIN : %010u |\n",
		(mtk_r32(eth, 0x240) & 0x1ff0000) >> 16);
	seq_printf(seq, "| FQ_PCNT     : %010u |\n",
		mtk_r32(eth, 0x240) & 0x01ff);
	seq_printf(seq, "| FE_DROP_FQ            : %010u |\n",
		mtk_r32(eth, 0x244));
	seq_printf(seq, "| FE_DROP_FC            : %010u |\n",
		mtk_r32(eth, 0x248));
	seq_printf(seq, "| FE_DROP_PPE1          : %010u |\n",
		mtk_r32(eth, 0x24c));
	seq_printf(seq, "| FE_DROP_PPE0          : %010u |\n",
		mtk_r32(eth, 0x2a0));
	seq_printf(seq, "| FE_DROP_PD_6          : %010u |\n",
		mtk_r32(eth, 0x250));
	seq_printf(seq, "| FE_DROP_PD_5          : %010u |\n",
		mtk_r32(eth, 0x254));
	seq_printf(seq, "| FE_DROP_PD_4          : %010u |\n",
		mtk_r32(eth, 0x258));
	seq_printf(seq, "| FE_DROP_PD_3          : %010u |\n",
		mtk_r32(eth, 0x25c));
	seq_printf(seq, "| FE_DROP_PD_2          : %010u |\n",
		mtk_r32(eth, 0x260));
	seq_printf(seq, "| FE_DROP_PD_1          : %010u |\n",
		mtk_r32(eth, 0x264));
	seq_printf(seq, "| FE_DROP_PD_0          : %010u |\n",
		mtk_r32(eth, 0x268));
	seq_printf(seq, "| FE_DROP_PD_14         : %010u |\n",
		mtk_r32(eth, 0x270));
	seq_printf(seq, "| FE_DROP_PD_13         : %010u |\n",
		mtk_r32(eth, 0x274));
	seq_printf(seq, "| FE_DROP_PD_12         : %010u |\n",
		mtk_r32(eth, 0x278));
	seq_printf(seq, "| FE_DROP_PD_11         : %010u |\n",
		mtk_r32(eth, 0x27c));
	seq_printf(seq, "| FE_DROP_PD_10         : %010u |\n",
		mtk_r32(eth, 0x280));
	seq_printf(seq, "| FE_DROP_PD_9          : %010u |\n",
		mtk_r32(eth, 0x284));
	seq_printf(seq, "| FE_DROP_PD_8          : %010u |\n",
		mtk_r32(eth, 0x288));
	seq_printf(seq, "| FE_DROP_PD_7          : %010u |\n",
		mtk_r32(eth, 0x28c));
	seq_printf(seq, "| FE_DROP_PPE0_P0_R01   : %010u |\n",
		mtk_r32(eth, 0x2a4));
	seq_printf(seq, "| FE_DROP_PPE0_P0_R23   : %010u |\n",
		mtk_r32(eth, 0x2a8));
	seq_printf(seq, "| FE_DROP_PPE0_P5       : %010u |\n",
		mtk_r32(eth, 0x2ac));
	seq_printf(seq, "| FE_DROP_PPE0_P8       : %010u |\n",
		mtk_r32(eth, 0x2b0));
	seq_printf(seq, "| FE_DROP_PPE0_P9       : %010u |\n",
		mtk_r32(eth, 0x2b4));
	seq_printf(seq, "| FE_DROP_PPE0_P10      : %010u |\n",
		mtk_r32(eth, 0x2b8));
	seq_printf(seq, "| FE_DROP_PPE0_P11      : %010u |\n",
		mtk_r32(eth, 0x2bc));
	seq_printf(seq, "| FE_DROP_PPE0_P12      : %010u |\n",
		mtk_r32(eth, 0x2c0));
	seq_printf(seq, "| FE_DROP_PPE1_P0_R01   : %010u |\n",
		mtk_r32(eth, 0x2c4));
	seq_printf(seq, "| FE_DROP_PPE1_P0_R23   : %010u |\n",
		mtk_r32(eth, 0x2c8));
	seq_printf(seq, "| FE_DROP_PPE1_P5       : %010u |\n",
		mtk_r32(eth, 0x2cc));
	seq_printf(seq, "| FE_DROP_PPE1_P8       : %010u |\n",
		mtk_r32(eth, 0x2d0));
	seq_printf(seq, "| FE_DROP_PPE1_P9       : %010u |\n",
		mtk_r32(eth, 0x2d4));
	seq_printf(seq, "| FE_DROP_PPE1_P10      : %010u |\n",
		mtk_r32(eth, 0x2d8));
	seq_printf(seq, "| FE_DROP_PPE1_P11      : %010u |\n",
		mtk_r32(eth, 0x2dc));
	seq_printf(seq, "| FE_DROP_PPE1_P12      : %010u |\n",
		mtk_r32(eth, 0x2e0));
	seq_printf(seq, "| PSE_IQ_STA1       : %x |\n",
		mtk_r32(eth, 0x180));
	seq_printf(seq, "| PSE_IQ_STA2       : %x |\n",
		mtk_r32(eth, 0x184));
	seq_printf(seq, "| PSE_IQ_STA3       : %x |\n",
		mtk_r32(eth, 0x188));
	seq_printf(seq, "| PSE_IQ_STA4       : %x |\n",
		mtk_r32(eth, 0x18c));
	seq_printf(seq, "| PSE_IQ_STA5       : %x |\n",
		mtk_r32(eth, 0x190));
	seq_printf(seq, "| PSE_IQ_STA6       : %x |\n",
		mtk_r32(eth, 0x194));
	seq_printf(seq, "| PSE_IQ_STA7       : %x |\n",
		mtk_r32(eth, 0x198));
	seq_printf(seq, "| PSE_IQ_STA8       : %x |\n",
		mtk_r32(eth, 0x19c));
	seq_printf(seq, "| PSE_OQ_STA1       : %x |\n",
		mtk_r32(eth, 0x1a0));
	seq_printf(seq, "| PSE_OQ_STA2       : %x |\n",
		mtk_r32(eth, 0x1a4));
	seq_printf(seq, "| PSE_OQ_STA3       : %x |\n",
		mtk_r32(eth, 0x1a8));
	seq_printf(seq, "| PSE_OQ_STA4       : %x |\n",
		mtk_r32(eth, 0x1ac));
	seq_printf(seq, "| PSE_OQ_STA5       : %x |\n",
		mtk_r32(eth, 0x1b0));
	seq_printf(seq, "| PSE_OQ_STA6       : %x |\n",
		mtk_r32(eth, 0x1b4));
	seq_printf(seq, "| PSE_OQ_STA7       : %x |\n",
		mtk_r32(eth, 0x1b8));
	seq_printf(seq, "| PSE_OQ_STA8       : %x |\n",
		mtk_r32(eth, 0x1bc));
	mtk_w32(eth, 0x90000000, (QDMA_BASE + 0x2bc));
	for (i = 0; i < 64; i++) {
		if (i <= 15) {
			mtk_w32(eth, 0, (QDMA_BASE + 0x1f0));
			pr_notice("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				//sys_reg_read(QTX_CFG_0 + i * 16),
				mtk_r32(eth, QDMA_BASE + i * 16),
				mtk_r32(eth, (QDMA_BASE + 0x4) + i * 16));
		} else if (i > 15 && i <= 31) {
			mtk_w32(eth, 1, (QDMA_BASE + 0x1f0));
			pr_notice("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				mtk_r32(eth, QDMA_BASE + (i - 16) * 16),
				mtk_r32(eth, (QDMA_BASE + 0x4) + (i - 16) * 16));
		} else if (i > 31 && i <= 47) {
			mtk_w32(eth, 2, (QDMA_BASE + 0x1f0));
			pr_notice("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				mtk_r32(eth, QDMA_BASE + (i - 32) * 16),
				mtk_r32(eth, (QDMA_BASE + 0x4) + (i - 32) * 16));
		} else if (i > 47 && i <= 63) {
			mtk_w32(eth, 3, (QDMA_BASE + 0x1f0));
			pr_notice("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				mtk_r32(eth, QDMA_BASE + (i - 48) * 16),
				mtk_r32(eth, (QDMA_BASE + 0x4) + (i - 48) * 16));
		}
	}
	mtk_w32(eth, 0, (QDMA_BASE + 0x1f0));
	mtk_w32(eth, 0, (QDMA_BASE + 0x2bc));
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "\n");

#if defined(CONFIG_MEDIATEK_NETSYS_V2)

	pr_notice("==============GMAC1 counter=================\n");

	tmp1 = mtk_r32(eth, 0x10400);
	tmp2 = mtk_r32(eth, 0x10404);
	tmp3 = mtk_r32(eth, 0x10408);
	tmp4 = mtk_r32(eth, 0x1040c);

	pr_notice(
		"GMAC1 RX UCST_PKTCNT H/L:\t%010u %010u\n\
GMAC1 RX UCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x410);
	tmp2 = mtk_r32(eth, cr_base + 0x414);
	tmp3 = mtk_r32(eth, cr_base + 0x418);
	tmp4 = mtk_r32(eth, cr_base + 0x41c);

	pr_notice(
		"GMAC1 RX MCST_PKTCNT H/L:\t%010u %010u\n\
GMAC1 RX MCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x420);
	tmp2 = mtk_r32(eth, cr_base + 0x424);
	tmp3 = mtk_r32(eth, cr_base + 0x428);
	tmp4 = mtk_r32(eth, cr_base + 0x42c);

	pr_notice(
		"GMAC1 RX BCST_PKTCNT H/L:\t%010u %010u\n\
GMAC1 RX BCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x430);
	tmp2 = mtk_r32(eth, cr_base + 0x434);
	tmp3 = mtk_r32(eth, cr_base + 0x438);
	tmp4 = mtk_r32(eth, cr_base + 0x43c);

	pr_notice(
		"GMAC1 TX UCST_PKTCNT H/L:\t%010u %010u\n\
GMAC1 TX UCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x440);
	tmp2 = mtk_r32(eth, cr_base + 0x444);
	tmp3 = mtk_r32(eth, cr_base + 0x448);
	tmp4 = mtk_r32(eth, cr_base + 0x44c);

	pr_notice(
		"GMAC1 TX MCST_PKTCNT H/L:\t%010u %010u\n\
GMAC1 TX MCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x450);
	tmp2 = mtk_r32(eth, cr_base + 0x454);
	tmp3 = mtk_r32(eth, cr_base + 0x458);
	tmp4 = mtk_r32(eth, cr_base + 0x45c);

	pr_notice(
		"GMAC1 TX BCST_PKTCNT H/L:\t%010u %010u\n\
GMAC1 TX BCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	pr_notice("==============GMAC2 counter=================\n");

	tmp1 = mtk_r32(eth, cr_base + 0x460);
	tmp2 = mtk_r32(eth, cr_base + 0x464);
	tmp3 = mtk_r32(eth, cr_base + 0x468);
	tmp4 = mtk_r32(eth, cr_base + 0x46c);

	pr_notice(
		"GMAC2 RX UCST_PKTCNT H/L:\t%010u %010u\n\
GMAC2 RX UCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x470);
	tmp2 = mtk_r32(eth, cr_base + 0x474);
	tmp3 = mtk_r32(eth, cr_base + 0x478);
	tmp4 = mtk_r32(eth, cr_base + 0x47c);

	pr_notice(
		"GMAC2 RX MCST_PKTCNT H/L:\t%010u %010u\n\
GMAC2 RX MCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x480);
	tmp2 = mtk_r32(eth, cr_base + 0x484);
	tmp3 = mtk_r32(eth, cr_base + 0x488);
	tmp4 = mtk_r32(eth, cr_base + 0x48c);

	pr_notice(
		"GMAC2 RX BCST_PKTCNT H/L:\t%010u %010u\n\
GMAC2 RX BCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);


	tmp1 = mtk_r32(eth, cr_base + 0x490);
	tmp2 = mtk_r32(eth, cr_base + 0x494);
	tmp3 = mtk_r32(eth, cr_base + 0x498);
	tmp4 = mtk_r32(eth, cr_base + 0x49c);

	pr_notice(
		"GMAC2 TX UCST_PKTCNT H/L:\t%010u %010u\n\
GMAC2 TX UCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x4a0);
	tmp2 = mtk_r32(eth, cr_base + 0x4a4);
	tmp3 = mtk_r32(eth, cr_base + 0x4a8);
	tmp4 = mtk_r32(eth, cr_base + 0x4ac);

	pr_notice(
		"GMAC2 TX MCST_PKTCNT H/L:\t%010u %010u\n\
GMAC2 TX MCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	tmp1 = mtk_r32(eth, cr_base + 0x4b0);
	tmp2 = mtk_r32(eth, cr_base + 0x4b4);
	tmp3 = mtk_r32(eth, cr_base + 0x4b8);
	tmp4 = mtk_r32(eth, cr_base + 0x4bc);

	pr_notice(
		"GMAC2 TX BCST_PKTCNT H/L:\t%010u %010u\n\
GMAC2 TX BCST_BYTECNT H/L:\t%010u %010u\n", tmp2, tmp1, tmp4, tmp3);

	//XFI MAC
	for (i = 0; i < 2; i ++ ) {
		seq_printf(seq,	"==============XFI MAC %d counter=================\n", i);

		tmp1 = mtk_r32(eth, cr_base + 0x1150 + (i * 0x1000));
		tmp2 = mtk_r32(eth, cr_base + 0x1154 + (i * 0x1000));
		tmp3 = mtk_r32(eth, cr_base + 0x1158 + (i * 0x1000));
		tmp4 = mtk_r32(eth, cr_base + 0x115C + (i * 0x1000));

		seq_printf(seq, "| XFIMAC%d TX_UCST PKTCNT H/L:\t%010u %010u\n\
| XFIMAC%d TX_UCST_BYTECNT H/L:\t%010u %010u\n",
			i, tmp2, tmp1, i, tmp4, tmp3);

		tmp1 = mtk_r32(eth, cr_base + 0x1160 + (i * 0x1000));
		tmp2 = mtk_r32(eth, cr_base + 0x1164 + (i * 0x1000));
		tmp3 = mtk_r32(eth, cr_base + 0x1168 + (i * 0x1000));
		tmp4 = mtk_r32(eth, cr_base + 0x116C + (i * 0x1000));
		seq_printf(seq, "| XFIMAC%d TX_MCST PKTCNT H/L:\t%010u %010u\n\
| XFIMAC%d TX_MCST_BYTECNT H/L:\t%010u %010u\n",
			i, tmp2, tmp1, i, tmp4, tmp3);

		tmp1 = mtk_r32(eth, cr_base + 0x1170 + (i * 0x1000));
		tmp2 = mtk_r32(eth, cr_base + 0x1174 + (i * 0x1000));
		tmp3 = mtk_r32(eth, cr_base + 0x1178 + (i * 0x1000));
		tmp4 = mtk_r32(eth, cr_base + 0x117C + (i * 0x1000));
		seq_printf(seq, "| XFIMAC%d TX_BCST PKTCNT H/L:\t%010u %010u\n\
| XFIMAC%d TX_BCST_BYTECNT H/L:\t%010u %010u\n",
			i, tmp2, tmp1, i, tmp4, tmp3);

		tmp1 = mtk_r32(eth, cr_base + 0x11C0 + (i * 0x1000));
		tmp2 = mtk_r32(eth, cr_base + 0x11C4 + (i * 0x1000));
		tmp3 = mtk_r32(eth, cr_base + 0x11C8 + (i * 0x1000));
		tmp4 = mtk_r32(eth, cr_base + 0x11CC + (i * 0x1000));
		seq_printf(seq, "| XFIMAC%d RX_UCST PKTCNT H/L:\t%010u %010u\n\
| XFIMAC%d RX_UCST_BYTECNT H/L:\t%010u %010u\n",
			i, tmp2, tmp1, i, tmp4, tmp3);

		tmp1 = mtk_r32(eth, cr_base + 0x11D0 + (i * 0x1000));
		tmp2 = mtk_r32(eth, cr_base + 0x11D4 + (i * 0x1000));
		tmp3 = mtk_r32(eth, cr_base + 0x11D8 + (i * 0x1000));
		tmp4 = mtk_r32(eth, cr_base + 0x11DC + (i * 0x1000));
		seq_printf(seq, "| XFIMAC%d RX_MCST PKTCNT H/L:\t%010u %010u\n\
| XFIMAC%d RX_MCST_BYTECNT H/L:\t%010u %010u\n",
			i, tmp2, tmp1, i, tmp4, tmp3);

		tmp1 = mtk_r32(eth, cr_base + 0x11E0 + (i * 0x1000));
		tmp2 = mtk_r32(eth, cr_base + 0x11E4 + (i * 0x1000));
		tmp3 = mtk_r32(eth, cr_base + 0x11E8 + (i * 0x1000));
		tmp4 = mtk_r32(eth, cr_base + 0x11EC + (i * 0x1000));
		seq_printf(seq, "| XFIMAC%d RX_BCST PKTCNT H/L:\t%010u %010u\n\
| XFIMAC%d RX_BCST_BYTECNT H/L:\t%010u %010u\n",
			i, tmp2, tmp1, i, tmp4, tmp3);

		tmp = mtk_r32(eth, cr_base + 0x1188 + (i * 0x1000));
		seq_printf(seq, "| XFI %d Rx Pkt Cnt : %010u|\n", i, tmp);
		tmp = mtk_r32(eth, cr_base + 0x118C + (i * 0x1000));
		seq_printf(seq, "| XFI %d Rx Eth Pkt Cnt : %010u|\n", i, tmp);
		tmp = mtk_r32(eth, cr_base + 0x1190 + (i * 0x1000));
		seq_printf(seq, "| XFI %d Rx Pause Cnt : %010u|\n", i, tmp);
		tmp = mtk_r32(eth, cr_base + 0x1194 + (i * 0x1000));
		seq_printf(seq, "| XFI %d Rx Len Error Pkt Cnt : %010u|\n", i, tmp);
		tmp = mtk_r32(eth, cr_base + 0x1198 + (i * 0x1000));
		seq_printf(seq, "| XFI %d Rx CRC Error Pkt Cnt : %010u|\n", i, tmp);
		tmp = mtk_r32(eth, cr_base + 0x119C + (i * 0x1000));
		seq_printf(seq, "| XFI %d RXMBI Interface Rx Pkt Cnt : %010u|\n", i, tmp);
		tmp = mtk_r32(eth, cr_base + 0x11A0 + (i * 0x1000));
		seq_printf(seq, "| XFI %d RXMBI Interface Rx Err Drop Pkt Cnt : %010u|\n", i, tmp);
	}
	seq_printf(seq, "+-----------------------------------------------+\n");

#endif
	return 0;
}

int sgmii_pwr_read(struct seq_file *seq, void *v){

	struct mtk_eth *eth = g_eth;
	u32 gmac_sts = mtk_r32(eth, MTK_XGMAC_STS);

	dev_err(eth->dev, "%s:  sts= %x \n",
		__func__, gmac_sts);

	return 0;
}

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static ssize_t usxgmii_pwr_read(struct file *filp, char __user *usr_buf, size_t count, loff_t *offset)
{
	static char buf[256];
	int ret;

	if(*offset > 0){
		return 0;
	}
	ret = sprintf(buf, "%d %d\n", mtk_gmii_glb[0][2]->enable, mtk_gmii_glb[1][2]->enable);
	ret = copy_to_user(usr_buf, buf, strlen(buf));
	if(ret) {
		pr_err("[%s] copy to user failed.\n", __func__);
		return -EFAULT;
	}
	*offset += strlen(buf);
	return strlen(buf);
}
#endif

static int switch_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, esw_cnt_read, 0);
}

static int cr_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, cr_dump_read, 0);
}

static int sgmii_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, sgmii_dump_read, 0);
}

static int qos_open(struct inode *inode, struct file *file)
{
	return single_open(file, qos_read, 0);
}

static int sfq_open(struct inode *inode, struct file *file)
{
	return single_open(file, sfq_read, 0);
}


static int sgmii_pwr_open(struct inode *inode, struct file *file)
{
	return single_open(file, sgmii_pwr_read, 0);
}

static ssize_t	qos_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data) {

	struct mtk_eth *eth = g_eth;
	char buf[128];
	char *p_buf;
	u32 len = count;
	int i;
	long val[7] = {0};
	char *p_token = NULL;
	char *p_delimiter = " \t\n";
	int ret;
	struct mtk_qdma_ioctl_data qdma_request = {0};
	u8 mac_addr[6];
	int port = -1;


	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}
	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);

	if (p_token != NULL && strncmp(p_token, "sch_policy", strlen(p_token)) == 0) {
		//echo sch_policy [sch] [wrr|sp] > proc/mtketh/qos
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token) {
			val[0] = 0;
		} else {
			ret = kstrtol(p_token, 10, &val[0]);
			if ( (ret != 0) | (val[0] >= 4) | (val[0] < 0)) {
				pr_info("command wrong = %s\n", buf);
				return len;
			}
			pr_info("queue number = %ld\n", val[0]);
		}

		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token) {
			val[1] = 0;
		} else {
			pr_info("p_token = %s\n", p_token);
			if(strncmp(p_token, "wrr", strlen(p_token)) == 0){
				val[1] = 1;
				pr_info("queue number = %ld use wrr\n", val[0]);
			} else {
				val[1] = 0;
				pr_info("queue number = %ld use sp\n", val[0]);
			}
		}
		qdma_request.cmd = MTKETH_QDMA_SCH_POLICY_WRITE;
		qdma_request.queue_id = (unsigned int) val[0];
		qdma_request.val = (unsigned int) val[1];
		mtk_qdma_ioctl(eth, NULL, &qdma_request);
	} else if (p_token != NULL && strncmp(p_token, "sch_rate", strlen(p_token)) == 0) {
		//echo sch_rate [sch] [max_en] [max_rate] [max_rate_exp] > proc/mtketh/qos
		for (i = 0; i < 4; i++){
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token)
				val[i] = 0;
			else
				ret = kstrtol(p_token, 10, &val[i]);
		}
		//sanity check
		if ( ((val[0] >= 4) || (val[0] < 0)) ||
		      !(val[1] == 1 || val[1] == 0) ||
		      (val[2] < 0 || val[2] > 127)) {
			pr_info("qos_write command wrong = %s\n", buf);
			return len;
		}
		pr_info("val = %lx %lx %lx %lx\n",
			val[0], val[1], val[2], val[3]);

		qdma_request.cmd = MTKETH_QDMA_SCH_RATE_WRITE;
		qdma_request.queue_id = (unsigned int) val[0];
		qdma_request.val = (unsigned int) (((val[1] & 0x1) << 11) | ((val[2] & 0x7f) << 4) |
			((val[3] & 0xf)) );
		pr_info("val = %x\n", qdma_request.val);
		qdma_request.mask = 0x00000fff;
		mtk_qdma_ioctl(eth, NULL, &qdma_request);
	} else if (p_token != NULL && strncmp(p_token, "weight", strlen(p_token)) == 0) {
		//echo weight [queue] [weighting] > proc/mtketh/qos
		for (i = 0; i < 2; i++){
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token)
				val[i] = 0;
			else
				ret = kstrtol(p_token, 10, &val[i]);
		}
		qdma_request.cmd = MTKETH_QDMA_SCH_WRITE;
		qdma_request.queue_id = (unsigned int) (val[0] & 0x3f);
		qdma_request.val = (unsigned int) ((val[1] & 0xf) << 12);
		qdma_request.mask = (0xf << 12);
		mtk_qdma_ioctl(eth, NULL, &qdma_request);
	} else if (p_token != NULL && strncmp(p_token, "rate", strlen(p_token)) == 0) {
		//echo rate [queue] [min_en] [min_rate][min_rate_exp] [max_en] [max_rate] [max_rate_exp] > proc/mtketh/qos
		for (i = 0; i < 7; i++){
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token)
				val[i] = 0;
			else
				ret = kstrtol(p_token, 10, &val[i]);
		}
		//sanity check
		if ( ((val[0] >= 64) || (val[0] < 0)) ||
			!(val[1] == 1 || val[1] == 0) ||
			!(val[4] == 1 || val[4] == 0) ||
			(val[2] < 0 || val[2] > 127) || (val[5] < 0 || val[5] > 127)) {
			pr_info("qos_write command wrong = %s\n", buf);
			return len;
		}
		pr_info("val = %lx %lx %lx %lx %lx %lx %lx\n",
			val[0], val[1], val[2], val[3], val[4], val[5],val[6]);

		qdma_request.cmd = MTKETH_QDMA_SCH_WRITE;
		qdma_request.queue_id = (unsigned int) (val[0] & 0x3f);
		qdma_request.val = (unsigned int) (((val[1] & 0x1) << 27) | ((val[2] & 0x7f) << 20) |
			((val[3] & 0xf) << 16) | ((val[4] & 0x1) << 11) | ((val[5] & 0x7f) << 4) |
			((val[6] & 0xf)) );
		pr_info("val = %x\n", qdma_request.val);
		qdma_request.mask = 0x0fff0fff;
		mtk_qdma_ioctl(eth, NULL, &qdma_request);
	} else if (p_token != NULL && strncmp(p_token, "m2q", strlen(p_token)) == 0) {
		//echo m2q [queue] [val] > proc/mtketh/qos
		for (i = 0; i < 2; i++){
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token)
				val[i] = 0;
			else
				ret = kstrtol(p_token, 10, &val[i]);
		}
		pr_info("val = %lx %lx\n", val[0], val[1]);
		qdma_request.cmd = MTKETH_QDMA_QUEUE_MAPPING;
		qdma_request.queue_id = (unsigned int) (val[0] & 0x3f);
		qdma_request.val = val[1];
		pr_info("queue_id = %x, val = %x\n", qdma_request.queue_id, qdma_request.val);
		mtk_qdma_ioctl(eth, NULL, &qdma_request);

	} else if (p_token != NULL && strncmp(p_token, "debug", strlen(p_token)) == 0) {
		//echo debug 1 1 > proc/mtketh/qos
		for (i = 0; i < 2; i++){
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token)
				val[i] = 0;
			else
				ret = kstrtol(p_token, 10, &val[i]);
		}
		pr_info("val = %lx %lx\n", val[0], val[1]);
		//debug usage
		eth->dbg_flag =val[0];
		eth->dbg_flag2 =val[1];
		pr_info("g_eth->dbg_flag = %d dbg_flag2=%d", val[0], val[1]);

	} else if (p_token != NULL && strncmp(p_token, "macToPort", strlen(p_token)) == 0) {
		//echo macToPort  > proc/mtketh/qos
		for (i = 0; i < 6; i++) {
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token) {
				val[i] = 0;
			} else{
				ret = kstrtol(p_token, 16, &val[i]);
				pr_info("macToPort %s val %x\n", p_token, val[i]);
				mac_addr[i] = (u8) val[i];
			}
		}
		mtk_eth_search_port_by_mac_table(mac_addr, 0, &port);
		pr_info("mtk_eth_search_port_by_mac_table = %d\n", port);

	} else {
		pr_info("qos_write command wrong = %s\n", buf);
	}
	return len;
}

ssize_t sfq_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *data)
{
	struct mtk_eth *eth = g_eth;
	char buf[32];
	char *p_buf;
	u32 len = count;
	long arg0 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t\n";
	int ret;

	if (len >= sizeof(buf)) {
		pr_notice("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_notice("write parameter data = %s\n\r", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	/* echo [enable] > proc/mtketh/sfq */
	switch (arg0) {
		case 0:
			sfq_disable(eth);
			break;
		case 1:
			sfq_enable(eth);
			break;
	}

	return len;
}

static ssize_t	sgmii_pwr_write(struct file *file,
	const char __user *buffer,	size_t count, loff_t *data){

	struct mtk_eth *eth = g_eth;
	char buf[128];
	char *p_buf;
	u32 len = count;
	int i;
	long val[7] = {0};
	char *p_token = NULL;
	char *p_delimiter = " \t\n";
	int ret;
	unsigned int id = 0;
	struct mtk_qdma_ioctl_data qdma_request = {0};

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}
	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);

	if (eth->soc->aqr113c_phy0_support ||
		eth->soc->aqr113c_phy1_support) {
		pr_info("10G PHY does not support force usxgmii\n");
		return len;
	}

	//echo dynamic [id] [en] > proc/mtketh/sgmii_pwr
	if (p_token != NULL && strncmp(p_token, "dynamic", strlen(p_token)) == 0) {
		for (i = 0; i < 2; i++){
			p_token = strsep(&p_buf, p_delimiter);
			if (!p_token) {
				val[i] = -2;
			} else {
				ret = kstrtol(p_token, 10, &val[i]);
			}
		}
		//sanity check
		if ((val[0] != 0) && (val[0] != 1)) {
			pr_info("input parameter wrong: format: dynamic [id] [en] "
				"do nothing and return!!!\n");
			return len;
		}
		if ((val[1] != -1) && (val[1] != 1)) {
			pr_info("input parameter wrong: format: dynamic [id] [en] "
				"do nothing and return!!!\n");
			return len;
		}

		pr_info("%s val = %lx %lx\n", __func__, val[0], val[1]);
		ret = mtk_eth_sgmii_set_force_mode(eth, (int) val[0], (int) val[1]);
		if (ret < 0) {
			pr_info("no change\n");
			return len;
		}

		id = val[0];
		mtk_eth_sgmii_runtime_status_action(eth, (int) val[0],
			((netif_running(eth->netdev[id]))? MTK_IF_UP : MTK_IF_DOWN));
	}
	return len;
}




static const struct file_operations switch_count_fops = {
	.owner = THIS_MODULE,
	.open = switch_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations cr_dump_fops = {
	.owner = THIS_MODULE,
	.open = cr_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations  sgmii_dump_fops = {
	.owner = THIS_MODULE,
	.open = sgmii_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations  qos_fops = {
	.owner = THIS_MODULE,
	.open = qos_open,
	.write  = qos_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations  sfq_fops = {
	.owner = THIS_MODULE,
	.open = sfq_open,
	.write  = sfq_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations  sgmii_pwr_fops = {
	.owner = THIS_MODULE,
	.open = sgmii_pwr_open,
	.write  = sgmii_pwr_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static const struct file_operations  usxgmii_pwr_fops = {
	.owner = THIS_MODULE,
	.read = usxgmii_pwr_read,
};
#endif
static struct proc_dir_entry *proc_tx_ring, *proc_rx_ring;

int tx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_tx_ring *ring = &g_eth->tx_ring;
	struct mtk_tx_dma *tx_ring;
	int i = 0;

	tx_ring =
	    kmalloc(sizeof(struct mtk_tx_dma) * MTK_DMA_SIZE, GFP_KERNEL);
	if (!tx_ring) {
		seq_puts(seq, " allocate temp tx_ring fail.\n");
		return 0;
	}

	for (i = 0; i < MTK_DMA_SIZE; i++)
		tx_ring[i] = ring->dma[i];

	seq_printf(seq, "free count = %d\n", (int)atomic_read(&ring->free_count));
	seq_printf(seq, "cpu next free: %d\n", (int)(ring->next_free - ring->dma));
	seq_printf(seq, "cpu last free: %d\n", (int)(ring->last_free - ring->dma));
	for (i = 0; i < MTK_DMA_SIZE; i++) {
		dma_addr_t tmp = ring->phys + i * sizeof(*tx_ring);

		seq_printf(seq, "%d (%pad): %08x %08x %08x %08x %08x %08x %08x %08x\n", i, &tmp,
			   *(int *)&tx_ring[i].txd1, *(int *)&tx_ring[i].txd2,
			   *(int *)&tx_ring[i].txd3, *(int *)&tx_ring[i].txd4 ,
			   *(int *)&tx_ring[i].txd5, *(int *)&tx_ring[i].txd6,
			   *(int *)&tx_ring[i].txd7, *(int *)&tx_ring[i].txd8);
	}

	kfree(tx_ring);
	return 0;
}

static int tx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, tx_ring_read, NULL);
}

static const struct file_operations tx_ring_fops = {
	.owner = THIS_MODULE,
	.open = tx_ring_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int rx_ring_readv2(struct seq_file *seq, void *v)
{
#if defined(CONFIG_MTK_ADMAV2)
	struct mtk_rx_ring *ring[MTK_MAX_RX_RING_NUM];
	struct mtk_rx_dma *rx_ring[MTK_MAX_RX_RING_NUM];

	int i = 0, j = 0;

	for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
		ring[i] = &g_eth->rx_ring[i];
		rx_ring[i] =
			kmalloc(sizeof(struct mtk_rx_dma) * MTK_DMA_SIZE, GFP_KERNEL);
	  	if (!rx_ring[i]) {
			seq_printf(seq, " allocate temp rx_ring  %d fail.\n", i);
			goto err_ring;
		}
	}
	for (j = 0; j < MTK_DMA_SIZE; j++) {
		//normal ring 0
		rx_ring[0][j] = ring[0]->dma[j];
		if (g_eth->hwrss) {
			//rss ring 1~3
			rx_ring[1][j] = ring[1]->dma[j];
			rx_ring[2][j] = ring[2]->dma[j];
			rx_ring[3][j] = ring[3]->dma[j];
		}
	}
	if (g_eth->hwlro) {
		for (j = 0; j < MTK_HW_LRO_DMA_SIZE; j++) {
			//lro ring 4~7
			rx_ring[4][j] = ring[4]->dma[j];
			rx_ring[5][j] = ring[5]->dma[j];
			rx_ring[6][j] = ring[6]->dma[j];
			rx_ring[7][j] = ring[7]->dma[j];
		}
	}

	for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
		if (ring[i] != NULL)
			seq_printf(seq, "Ring %d next to read: %d\n", i,
			   NEXT_DESP_IDX(ring[i]->calc_idx, MTK_DMA_SIZE));
	}
	i = 0;
	seq_printf(seq, "!!!!!!!!!!! RING %d !!!!!!!!!!!\n", i);
	for (j = 0; j < MTK_DMA_SIZE; j++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x %08x %08x\n", j,
			   *(int *)&rx_ring[i][j].rxd1, *(int *)&rx_ring[i][j].rxd2,
			   *(int *)&rx_ring[i][j].rxd3, *(int *)&rx_ring[i][j].rxd4,
		   	   *(int *)&rx_ring[i][j].rxd5, *(int *)&rx_ring[i][j].rxd6);
	}
	if (g_eth->hwrss) {
		for (i = 1; i <= 3; i++) {
				seq_printf(seq, "!!!!!!!!!!! RING %d !!!!!!!!!!!\n", i);
			for (j = 0; j < MTK_DMA_SIZE; j++) {
				seq_printf(seq, "%d: %08x %08x %08x %08x %08x %08x\n", j,
				   *(int *)&rx_ring[i][j].rxd1, *(int *)&rx_ring[i][j].rxd2,
				   *(int *)&rx_ring[i][j].rxd3, *(int *)&rx_ring[i][j].rxd4,
				   *(int *)&rx_ring[i][j].rxd5, *(int *)&rx_ring[i][j].rxd6);
			}
		}
	}
	if (g_eth->hwlro) {
		for (i = 4; i <= 7; i++) {
			seq_printf(seq, "!!!!!!!!!!! RING %d !!!!!!!!!!!\n", i);
			for (j = 0; j < MTK_HW_LRO_DMA_SIZE; j++) {
				seq_printf(seq, "%d: %08x %08x %08x %08x %08x %08x\n", j,
				   *(int *)&rx_ring[i][j].rxd1, *(int *)&rx_ring[i][j].rxd2,
				   *(int *)&rx_ring[i][j].rxd3, *(int *)&rx_ring[i][j].rxd4,
				   *(int *)&rx_ring[i][j].rxd5, *(int *)&rx_ring[i][j].rxd6);
			}
		}
	}
err_ring:

	for (i = 0; i < MTK_MAX_RX_RING_NUM; i++) {
		if (ring[i] != NULL)
			kfree(rx_ring[i]);
	}
#endif

	return 0;
}


int rx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_rx_ring *ring = &g_eth->rx_ring[0];
	struct mtk_rx_ring *ring1 = &g_eth->rx_ring[1];
	struct mtk_rx_ring *ring2 = &g_eth->rx_ring[2];
	struct mtk_rx_ring *ring3 = &g_eth->rx_ring[3];
	struct mtk_rx_dma *rx_ring, *rx_ring1, *rx_ring2, *rx_ring3;

	int i = 0;

#if defined(CONFIG_MTK_ADMAV2)
	rx_ring_readv2(seq, v);
	return 0;
#else


	rx_ring =
	    kmalloc(sizeof(struct mtk_rx_dma) * MTK_DMA_SIZE, GFP_KERNEL);
  	if (!rx_ring) {
		seq_puts(seq, " allocate temp rx_ring fail.\n");
		goto err_ring;
	}

	rx_ring1 =
	    kmalloc(sizeof(struct mtk_rx_dma) * MTK_HW_LRO_DMA_SIZE, GFP_KERNEL);
	if (!rx_ring1) {
		seq_puts(seq, " allocate temp rx_ring1 fail.\n");
		goto err_ring1;
	}
	rx_ring2 =
	    kmalloc(sizeof(struct mtk_rx_dma) * MTK_HW_LRO_DMA_SIZE, GFP_KERNEL);
	if (!rx_ring2) {
		seq_puts(seq, " allocate temp rx_ring2 fail.\n");
		goto err_ring2;
	}

	rx_ring3 =
	    kmalloc(sizeof(struct mtk_rx_dma) * MTK_HW_LRO_DMA_SIZE, GFP_KERNEL);
	if (!rx_ring3) {
		seq_puts(seq, " allocate temp rx_ring3 fail.\n");
		goto err_ring3;
	}

	for (i = 0; i < MTK_DMA_SIZE; i++)
		rx_ring[i] = ring->dma[i];

	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++)
		rx_ring1[i] = ring1->dma[i];

	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++)
		rx_ring2[i] = ring2->dma[i];

	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++)
		rx_ring3[i] = ring3->dma[i];

	seq_printf(seq, "next to read: %d\n",
		   NEXT_DESP_IDX(ring->calc_idx, MTK_DMA_SIZE));
	for (i = 0; i < MTK_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring[i].rxd1, *(int *)&rx_ring[i].rxd2,
			   *(int *)&rx_ring[i].rxd3, *(int *)&rx_ring[i].rxd4);
	}

	seq_printf(seq, "!!!!!!!!!!! HW LRO RING1 !!!!!!!!!!!\n");
	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring1[i].rxd1, *(int *)&rx_ring1[i].rxd2,
			   *(int *)&rx_ring1[i].rxd3, *(int *)&rx_ring1[i].rxd4);
	}

	seq_printf(seq, "!!!!!!!!!!! HW LRO RING2 !!!!!!!!!!!\n");
	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring2[i].rxd1, *(int *)&rx_ring2[i].rxd2,
			   *(int *)&rx_ring2[i].rxd3, *(int *)&rx_ring2[i].rxd4);
	}

	seq_printf(seq, "!!!!!!!!!!! HW LRO RING3 !!!!!!!!!!!\n");
	for (i = 0; i < MTK_HW_LRO_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring3[i].rxd1, *(int *)&rx_ring3[i].rxd2,
			   *(int *)&rx_ring3[i].rxd3, *(int *)&rx_ring3[i].rxd4);
	}
	kfree(rx_ring);
	kfree(rx_ring1);
	kfree(rx_ring2);
	kfree(rx_ring3);
	return 0;
err_ring3:
	kfree(rx_ring3);
err_ring2:
	kfree(rx_ring2);
err_ring1:
	kfree(rx_ring1);
err_ring:
	kfree(rx_ring);
	return 0;

#endif
}

static int rx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring_read, NULL);
}

static const struct file_operations rx_ring_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
extern struct mac_phy_of_adapt *mac_phy_adapt;
static struct proc_dir_entry *proc_sgmii_phy_id;
#define PROCREG_SGMII_PHY_ID	"sgmii_phy_id"

int sgmii_phy_id_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "mac0:0x%x\nmac1:0x%x", \
			mac_phy_adapt->mac_data[0].sgmii_phy_id, mac_phy_adapt->mac_data[1].sgmii_phy_id);

	return 0;
}

static int sgmii_phy_id_open(struct inode *inode, struct file *file)
{
	return single_open(file, sgmii_phy_id_read, NULL);
}
static const struct file_operations sgmii_phy_id_fops = {
	.owner = THIS_MODULE,
	.open = sgmii_phy_id_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif

#define PROCREG_CR_DUMP         "cr_dump"
#define PROCREG_ESW_CNT         "esw_cnt"
#define PROCREG_TXRING          "tx_ring"
#define PROCREG_RXRING          "rx_ring"
#define PROCREG_DIR             "mtketh"
#define PROCREG_SGMII           "sgmii"
#define PROCREG_QOS           "qos"
#define PROCREG_SFQ           "sfq"
#define PROCREG_EXT_INT           "ext_int"
#define PROCREG_SGMII_PWR	"sgmii_pwr"
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
#define PROCREG_USXGMII_PWR	"usxgmii_pwr"
#endif

struct proc_dir_entry *proc_reg_dir;
static struct proc_dir_entry *proc_esw_cnt;
static struct proc_dir_entry *proc_cr_dump;
static struct proc_dir_entry *proc_sgmii;
static struct proc_dir_entry *proc_qos;
static struct proc_dir_entry *proc_sfq;

static struct proc_dir_entry *proc_sgmii_pwr;
#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
static struct proc_dir_entry *proc_usxgmii_pwr;
#endif



int debug_proc_init(struct mtk_eth *eth)
{
	g_eth = eth;

	if (!proc_reg_dir)
		proc_reg_dir = proc_mkdir(PROCREG_DIR, NULL);

	proc_tx_ring =
	    proc_create(PROCREG_TXRING, 0400, proc_reg_dir, &tx_ring_fops);
	if (!proc_tx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_TXRING);

	proc_rx_ring =
	    proc_create(PROCREG_RXRING, 0400, proc_reg_dir, &rx_ring_fops);
	if (!proc_rx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_RXRING);

	proc_esw_cnt =
	    proc_create(PROCREG_ESW_CNT,0400, proc_reg_dir, &switch_count_fops);
	if (!proc_esw_cnt)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_ESW_CNT);

	proc_cr_dump =
	    proc_create(PROCREG_CR_DUMP, 0400, proc_reg_dir, &cr_dump_fops);
	if (!proc_cr_dump)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_CR_DUMP);

	proc_sgmii =
	    proc_create(PROCREG_SGMII, 0400, proc_reg_dir, &sgmii_dump_fops);
	if (!proc_sgmii)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_SGMII);

	proc_qos =
		proc_create(PROCREG_QOS, 0400, proc_reg_dir, &qos_fops);
	if (!proc_qos)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_QOS);

	proc_sfq =
		proc_create(PROCREG_SFQ, 0400, proc_reg_dir, &sfq_fops);
	if (!proc_sfq)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_SFQ);

	proc_sgmii_pwr =
		proc_create(PROCREG_SGMII_PWR, 0400, proc_reg_dir, &sgmii_pwr_fops);
	if (!proc_sgmii_pwr)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_SGMII_PWR);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	proc_usxgmii_pwr =
		proc_create(PROCREG_USXGMII_PWR, 0444, proc_reg_dir, &usxgmii_pwr_fops);
	if (!proc_usxgmii_pwr)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_USXGMII_PWR);
#endif

#if defined(CONFIG_MTK_ADMAV2)
	if (MTK_HAS_CAPS(g_eth->hwlro, MTK_HWLRO))
		hwlrov2_debug_proc_init(proc_reg_dir, g_eth);
#else
	if (MTK_HAS_CAPS(g_eth->hwlro, MTK_HWLRO))
		hwlro_debug_proc_init(proc_reg_dir, g_eth);
#endif
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
	proc_sgmii_phy_id =
	     proc_create(PROCREG_SGMII_PHY_ID, 0400, proc_reg_dir,
			 &sgmii_phy_id_fops);
	if (!proc_sgmii_phy_id)
		pr_info("!! FAIL to create %s PROC !!\n",
			PROCREG_SGMII_PHY_ID);
#endif

	return 0;
}

void debug_proc_exit(void)
{
	if (proc_tx_ring)
		remove_proc_entry(PROCREG_TXRING, proc_reg_dir);
	if (proc_rx_ring)
		remove_proc_entry(PROCREG_RXRING, proc_reg_dir);

	if (proc_esw_cnt)
		remove_proc_entry(PROCREG_ESW_CNT, proc_reg_dir);

	if (proc_cr_dump)
		remove_proc_entry(PROCREG_CR_DUMP, proc_reg_dir);

	if (proc_sgmii)
		remove_proc_entry(PROCREG_SGMII, proc_reg_dir);

	if (proc_qos)
		remove_proc_entry(PROCREG_QOS, proc_reg_dir);

	if (proc_sfq)
		remove_proc_entry(PROCREG_SFQ, proc_reg_dir);

	hwlro_debug_proc_exit(proc_reg_dir);

	if (proc_sgmii_pwr)
		remove_proc_entry(PROCREG_SGMII_PWR, proc_sgmii_pwr);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY)
	if (proc_usxgmii_pwr)
		remove_proc_entry(PROCREG_USXGMII_PWR, proc_usxgmii_pwr);
#endif
	if (proc_reg_dir)
		remove_proc_entry(PROCREG_DIR, 0);
}

