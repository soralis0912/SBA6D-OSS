// SPDX-License-Identifier: GPL-2.0-only

#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/firmware.h>
#include <linux/of_address.h>
#include <linux/soc/mediatek/mtk_wed.h>
#include "mtk_wed_regs.h"
#include "mtk_wed_mcu.h"
#include "mtk_wed_wo.h"
#include "mtk_wed.h"

struct sk_buff *
mtk_wed_mcu_msg_alloc(struct mtk_wed_wo *wo,
		      const void *data, int data_len)
{
	const struct wed_wo_mcu_ops *ops = wo->mcu_ops;
	int length = ops->headroom + data_len;
	struct sk_buff *skb;

	skb = alloc_skb(length, GFP_KERNEL);
	if (!skb)
		return NULL;

	memset(skb->head, 0, length);
	skb_reserve(skb, ops->headroom);

	if (data && data_len)
		skb_put_data(skb, data, data_len);

	return skb;
}

struct sk_buff *
mtk_wed_mcu_get_response(struct mtk_wed_wo *wo, unsigned long expires)
{
	unsigned long timeout;

	if (!time_is_after_jiffies(expires))
		return NULL;

	timeout = expires - jiffies;
	wait_event_timeout(wo->mcu.wait,
			   (!skb_queue_empty(&wo->mcu.res_q)),
			   timeout);

	return skb_dequeue(&wo->mcu.res_q);
}

int
mtk_wed_mcu_skb_send_and_get_msg(struct mtk_wed_wo *wo,
				 int to_id, int cmd, struct sk_buff *skb,
				 bool wait_resp, struct sk_buff **ret_skb)
{
	unsigned long expires;
	int ret, seq;

	if (ret_skb)
		*ret_skb = NULL;

	mutex_lock(&wo->mcu.mutex);

	ret = wo->mcu_ops->mcu_skb_send_msg(wo, to_id, cmd, skb, &seq, wait_resp);
	if (ret < 0)
		goto out;

	if (!wait_resp) {
		ret = 0;
		goto out;
	}

	expires = jiffies + wo->mcu.timeout;

	do {
		skb = mtk_wed_mcu_get_response(wo, expires);
		ret = wo->mcu_ops->mcu_parse_response(wo, cmd, skb, seq);

		if (!ret && ret_skb)
			*ret_skb = skb;
		else
			dev_kfree_skb(skb);
	} while (ret == -EAGAIN);

out:
	mutex_unlock(&wo->mcu.mutex);

	return ret;
}

void mtk_wed_mcu_rx_event(struct mtk_wed_wo *wo,
			struct sk_buff *skb)
{
	skb_queue_tail(&wo->mcu.res_q, skb);
	wake_up(&wo->mcu.wait);
}

static int mtk_wed_mcu_send_and_get_msg(struct mtk_wed_wo *wo,
			int to_id, int cmd, const void *data, int len,
			bool wait_resp, struct sk_buff **ret_skb)
{
	struct sk_buff *skb;

	skb = mtk_wed_mcu_msg_alloc(wo, data, len);
	if (!skb)
		return -ENOMEM;

	return mtk_wed_mcu_skb_send_and_get_msg(wo, to_id, cmd, skb, wait_resp, ret_skb);
}

int
mtk_wed_mcu_send_msg(struct mtk_wed_wo *wo,
			int to_id, int cmd,
			const void *data, int len, bool wait_resp)
{
	struct sk_buff *skb = NULL;
	int ret = 0;

	ret = mtk_wed_mcu_send_and_get_msg(wo, to_id, cmd, data,
					   len, wait_resp, &skb);
	if (skb)
		dev_kfree_skb(skb);

	return ret;
}

int mtk_wed_exception_init(struct mtk_wed_wo *wo)
{
	struct wed_wo_exception *exp = &wo->exp;
	struct {
		u32 arg0;
		u32 arg1;
	}req;

	exp->log_size = EXCEPTION_LOG_SIZE;
	exp->log = kmalloc(exp->log_size, GFP_ATOMIC);
	if (!exp->log)
		return -ENOMEM;

	memset(exp->log, 0, exp->log_size);
	exp->phys = dma_map_single(wo->hw->dev, exp->log, exp->log_size,
				   DMA_FROM_DEVICE);

	if (unlikely(dma_mapping_error(wo->hw->dev, exp->phys))) {
		dev_info(wo->hw->dev, "dma map error\n");
		goto free;
	}

	req.arg0 = (u32)exp->phys;
	req.arg1 = (u32)exp->log_size;

	return mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_EXCEPTION_INIT,
				    &req, sizeof(req), false);

free:
	kfree(exp->log);
	return -ENOMEM;
}

int
mtk_wed_mcu_cmd_sanity_check(struct mtk_wed_wo *wo, struct sk_buff *skb)
{
	struct wed_cmd_hdr *hdr = (struct wed_cmd_hdr *)skb->data;

	if (hdr->ver != 0)
		return WARP_INVALID_PARA_STATUS;

	if (skb->len < sizeof(struct wed_cmd_hdr))
		return WARP_INVALID_PARA_STATUS;

	if (skb->len != hdr->length)
		return WARP_INVALID_PARA_STATUS;

	return WARP_OK_STATUS;
}

void
mtk_wed_mcu_rx_unsolicited_event(struct mtk_wed_wo *wo, struct sk_buff *skb)
{
	struct mtk_wed_device *wed = wo->hw->wed_dev;
	struct wed_cmd_hdr *hdr = (struct wed_cmd_hdr *)skb->data;
	struct wed_wo_log *record;
	struct mtk_wed_wo_rx_stats *rxcnt;
	char *msg = (char *)(skb->data + sizeof(struct wed_cmd_hdr));
	u16 msg_len = skb->len - sizeof(struct wed_cmd_hdr);
	u32 i, cnt = 0;

	switch (hdr->cmd_id) {
	case WO_EVT_LOG_DUMP:
		pr_info("[WO LOG]: %s\n", msg);
		break;
	case WO_EVT_PROFILING:
		cnt = msg_len / (sizeof(struct wed_wo_log));
		record = (struct wed_wo_log *) msg;
		dev_info(wo->hw->dev, "[WO Profiling]: %d report arrived!\n", cnt);

		for (i = 0 ; i < cnt ; i++) {
			//PROFILE_STAT(wo->total, record[i].total);
			//PROFILE_STAT(wo->mod, record[i].mod);
			//PROFILE_STAT(wo->rro, record[i].rro);

			dev_info(wo->hw->dev, "[WO Profiling]:  SN:%u with latency: total=%u, rro:%u, mod:%u\n",
				 record[i].sn,
				 record[i].total,
				 record[i].rro,
				 record[i].mod);
		}
		break;
	case WO_EVT_RXCNT_INFO:
		rxcnt = (struct mtk_wed_wo_rx_stats *)((u32 *)msg);

		if (wed->wlan.update_wo_rx_stats)
			wed->wlan.update_wo_rx_stats(wed, rxcnt);
		break;
	default:
		break;
	}

	dev_kfree_skb(skb);

}

void get_wo_heartbeat(struct mtk_wed_wo *wo) {
	u32 addr = 0, hb_val = 0;

	switch (wo->hw->index) {

	case WARP_WED1:
		addr = WOX_MCU_CFG_LS_WOX_COM_REG0_ADDR;
		break;

	case WARP_WED2:
		addr = WOX_MCU_CFG_LS_WOX_COM_REG1_ADDR;
		break;

	case WARP_WED3:
		addr = WOX_MCU_CFG_LS_WOX_COM_REG2_ADDR;
		break;

	default:
		pr_err("%s: wrong addr, hw->index is %d!\n", __func__, wo->hw->index);
		break;

	}

        hb_val = wo_r32(wo, addr);
        pr_err("%s: wo heartbeat: 0x%x!\n", __func__, hb_val);


}


static int
mtk_wed_load_firmware(struct mtk_wed_wo *wo)
{
	struct fw_info {
		__le32 decomp_crc;
		__le32 decomp_len;
		__le32 decomp_blk_sz;
		u8 reserved[4];
		__le32 addr;
		__le32 len;
		u8 feature_set;
		u8 reserved1[15];
	} __packed *region;

	char *mcu;
	const struct mtk_wed_fw_trailer *hdr;
	static u8 shared[MAX_REGION_SIZE] = {0};
	const struct firmware *fw;
	int ret, i;
	u32 ofs = 0;
	u32 boot_cr, val;

	switch (wo->hw->index) {
		case WARP_WED1:
			mcu = MT6990_FIRMWARE_WO_1;
			break;

		case WARP_WED2:
			mcu = MT6990_FIRMWARE_WO_2;
			break;

		case WARP_WED3:
			mcu = MT6990_FIRMWARE_WO_3;
			break;

		default:
			mcu = "";
			pr_err("%s: wrong wo firmware path, hw->index is %d!\n", __func__, wo->hw->index);
			break;
	}


	ret = request_firmware(&fw, mcu, wo->hw->dev);
	if (ret)
		return ret;

	hdr = (const struct mtk_wed_fw_trailer *)(fw->data + fw->size -
						  sizeof(*hdr));

	dev_info(wo->hw->dev, "WO Firmware Version: %.10s, Build Time: %.15s\n",
		 hdr->fw_ver, hdr->build_date);

	for (i = 0; i < hdr->n_region; i++) {
		int j = 0;
		region = (struct fw_info *)(fw->data + fw->size -
					    sizeof(*hdr) -
					    sizeof(*region) *
					    (hdr->n_region - i));

		while (j < MAX_REGION_SIZE) {
			struct mtk_wed_fw_region *wo_region;

			wo_region = &wo->region[j];
			if (!wo_region->addr)
				break;

			if (wo_region->addr_pa == region->addr) {
				if (!wo_region->shared) {
					memcpy(wo_region->addr,
					       fw->data + ofs, region->len);
				} else if (!shared[j]) {
					memcpy(wo_region->addr,
					       fw->data + ofs, region->len);
					shared[j] = true;
				}
			}
			j++;
		}

		if (j == __WO_REGION_MAX) {
			ret = -ENOENT;
			goto done;
		}
		ofs += region->len;
	}

	/* write the start address for mt6990 wo*/
	boot_cr = WOX_MCU_CFG_LS_WM_BOOT_ADDR_ADDR;
	wo_w32(wo, boot_cr, (wo->region[WO_REGION_EMI].addr_pa >> 16));

	/* wo firmware reset */
	wo_w32(wo, WOX_MCU_CFG_LS_WF_MCCR_CLR_ADDR, 0xc00);

	val = wo_r32(wo, WOX_MCU_CFG_LS_WF_MCU_CFG_WM_WA_ADDR);

	val |= WOX_MCU_CFG_LS_WOX_MCU_CFG_WO0_WO1_WOX_CPU_RSTB_MASK;

	wo_w32(wo, WOX_MCU_CFG_LS_WF_MCU_CFG_WM_WA_ADDR, val);

done:
	release_firmware(fw);

	return ret;
}

static int
mtk_wed_get_firmware_region(struct mtk_wed_wo *wo)
{
	struct device_node *node, *np = wo->hw->node;
	struct mtk_wed_fw_region *region;
	struct resource res;
	const char *compat;
	int i, ret;

	static const char *const wo_region_compat[__WO_REGION_MAX] = {
		[WO_REGION_EMI] = WOCPU_EMI_DEV_NODE,
		[WO_REGION_ILM] = WOCPU_ILM_DEV_NODE,
		[WO_REGION_DATA] = WOCPU_DATA_DEV_NODE,
		[WO_REGION_BOOT] = WOCPU_BOOT_DEV_NODE,
	};

	for (i = 0; i < __WO_REGION_MAX; i++) {
		region = &wo->region[i];
		compat = wo_region_compat[i];

		node = of_parse_phandle(np, compat, 0);
		if (!node)
			return -ENODEV;

		ret = of_address_to_resource(node, 0, &res);
		if (ret)
			return ret;

		region->addr_pa = res.start;
		region->size = resource_size(&res);
		region->addr = ioremap(region->addr_pa, region->size);

		of_property_read_u32_index(node, "shared", 0, &region->shared);
	}

	return 0;
}

static int
wo_mcu_send_message(struct mtk_wed_wo *wo,
			int to_id, int cmd, struct sk_buff *skb,
			int *wait_seq, bool wait_resp)
{
	struct wed_cmd_hdr  *hdr;
	u8 seq = 0;

	/* TDO: make dynamic based on msg type */
	wo->mcu.timeout = 20 * HZ;

	if (wait_resp && wait_seq) {
		seq = wo->mcu.msg_seq++ ;
		*wait_seq = seq;
	}

	hdr = (struct wed_cmd_hdr *)skb_push(skb, sizeof(*hdr));

	hdr->cmd_id = cmd;
	hdr->length = cpu_to_le16(skb->len);
	hdr->uni_id = seq;

	if (to_id == MODULE_ID_WO)
		hdr->flag |= WARP_CMD_FLAG_FROM_TO_WO;

	if (wait_resp && wait_seq)
		hdr->flag |= WARP_CMD_FLAG_NEED_RSP;

	return mtk_wed_wo_q_tx_skb(wo, &wo->q_tx, skb);
}

static int
wo_mcu_parse_response(struct mtk_wed_wo *wo, int cmd,
			  struct sk_buff *skb, int seq)
{
	struct mtk_wed_device *wed = wo->hw->wed_dev;
	struct wed_cmd_hdr  *hdr;
	struct mtk_wed_wo_rx_stats *rxcnt = NULL;
	u32 i, cnt = 0;

	if (!skb) {
		dev_err(wo->hw->dev, "Message %08x (seq %d) timeout\n",
			cmd, seq);
		return -ETIMEDOUT;
	}

	hdr = (struct wed_cmd_hdr *)skb->data;
	if (seq != hdr->uni_id) {
		dev_err(wo->hw->dev, "Message %08x (seq %d) with not match uid(%d)\n",
			cmd, seq, hdr->uni_id);
		return -EAGAIN;
	}

	skb_pull(skb, sizeof(struct wed_cmd_hdr));

	switch (cmd) {
	case MTK_WED_WO_CMD_RXCNT_INFO:
		cnt = *(u32 *)skb->data;
		rxcnt = (struct mtk_wed_wo_rx_stats *)((u32 *)skb->data+1);

		for (i = 0; i < cnt; i++)
			if (wed->wlan.update_wo_rx_stats)
				wed->wlan.update_wo_rx_stats(wed, &rxcnt[i]);
		break;
	default:
		break;
	}

	return 0;
}

int wed_wo_mcu_init(struct mtk_wed_wo *wo)
{
	static const struct wed_wo_mcu_ops wo_mcu_ops = {
		.headroom = sizeof(struct wed_cmd_hdr),
		.mcu_skb_send_msg = wo_mcu_send_message,
		.mcu_parse_response = wo_mcu_parse_response,
		/*TDO .mcu_restart = wo_mcu_restart,*/
	};
	unsigned long timeout = jiffies + FW_DL_TIMEOUT;
	int ret;
	u32 val;

	wo->mcu_ops = &wo_mcu_ops;

	//no fwdl check done here

	ret = mtk_wed_get_firmware_region(wo);
	if (ret)
		return ret;

	/* set dummy cr */
	wed_w32(wo->hw->wed_dev, MTK_WED_SCR0 + 4 * WED_DUMMY_CR_FWDL,
		wo->hw->index + 1);

	ret = mtk_wed_load_firmware(wo);
	if (ret)
		return ret;

	do {
		/* get dummy cr */
		val = wed_r32(wo->hw->wed_dev, MTK_WED_SCR0 + 4 * WED_DUMMY_CR_FWDL);
	} while (val != 0 && !time_after(jiffies, timeout));

	if (val)
		return -EBUSY;

	//no unmap here

	return 0;
}

static ssize_t
mtk_wed_wo_ctrl(struct file *file,
			 const char __user *user_buf,
			 size_t count,
			 loff_t *ppos)
{
	struct mtk_wed_hw *hw = file->private_data;
	struct mtk_wed_wo *wo = hw->wed_wo;
	char buf[100], *cmd = NULL, *input[11] = {0};
	char msgbuf[128] = {0};
	struct wo_cmd_query *query = (struct wo_cmd_query *)msgbuf;
	u32 cmd_id;
	bool wait = false;
	char *sub_str = NULL;
	int  input_idx = 0, input_total = 0, scan_num = 0;
	char *p;

	if (count > sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (count && buf[count - 1] == '\n')
		buf[count - 1] = '\0';
	else
		buf[count] = '\0';

	p = buf;

	while ((sub_str = strsep(&p, " ")) != NULL) {
		input[input_idx] = sub_str;
		input_idx++;
		input_total++;
	}
	cmd = input[0];
	if (input_total == 1 && cmd) {
		if (strncmp(cmd, "bainfo", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_BA_INFO_DUMP;
		} else if (strncmp(cmd, "bactrl", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_BA_CTRL_DUMP;
		} else if (strncmp(cmd, "fbcmdq", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_FBCMD_Q_DUMP;
		} else if (strncmp(cmd, "logflush", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_LOG_FLUSH;
		} else if (strncmp(cmd, "cpustat.dump", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_CPU_STATS_DUMP;
		} else if (strncmp(cmd, "state", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_WED_RX_STAT;
		} else if (strncmp(cmd, "prof_hit_dump", strlen(cmd)) == 0) {
			//wo_profiling_report();
			return count;
		} else if (strncmp(cmd, "rxcnt_info", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_RXCNT_INFO;
			wait = true;
		} else {
			pr_info("(%s) unknown comand string(%s)!\n", __func__, cmd);
			 return count;
		}
	}  else if (input_total > 1) {
		for (input_idx = 1 ; input_idx < input_total ; input_idx++) {
			scan_num = sscanf(input[input_idx], "%u", &query->query0+(input_idx - 1));

			if (scan_num < 1) {
				pr_info("(%s) require more input!\n", __func__);
				return count;
			}
		}
		if(strncmp(cmd, "devinfo", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_DEV_INFO_DUMP;
		} else if (strncmp(cmd, "bssinfo", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_BSS_INFO_DUMP;
		} else if (strncmp(cmd, "starec", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_STA_REC_DUMP;
		} else if (strncmp(cmd, "starec_ba", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_STA_BA_DUMP;
		} else if (strncmp(cmd, "logctrl", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_FW_LOG_CTRL;
		} else if (strncmp(cmd, "cpustat.en", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_CPU_STATS_ENABLE;
		} else if (strncmp(cmd, "prof_conf", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_PROF_CTRL;
		} else if (strncmp(cmd, "rxcnt_ctrl", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_RXCNT_CTRL;
		} else if (strncmp(cmd, "dbg_set", strlen(cmd)) == 0) {
			cmd_id = MTK_WED_WO_CMD_DBG_INFO;
		}
	} else {
		dev_info(hw->dev, "usage: echo cmd='cmd_str' > wo_write\n");
		dev_info(hw->dev, "cmd_str value range:\n");
		dev_info(hw->dev, "\tbainfo:\n");
		dev_info(hw->dev, "\tbactrl:\n");
		dev_info(hw->dev, "\tfbcmdq:\n");
		dev_info(hw->dev, "\tlogflush:\n");
		dev_info(hw->dev, "\tcpustat.dump:\n");
		dev_info(hw->dev, "\tprof_hit_dump:\n");
		dev_info(hw->dev, "\trxcnt_info:\n");
		dev_info(hw->dev, "\tdevinfo:\n");
		dev_info(hw->dev, "\tbssinfo:\n");
		dev_info(hw->dev, "\tstarec:\n");
		dev_info(hw->dev, "\tstarec_ba:\n");
		dev_info(hw->dev, "\tlogctrl:\n");
		dev_info(hw->dev, "\tcpustat.en:\n");
		dev_info(hw->dev, "\tprof_conf:\n");
		dev_info(hw->dev, "\trxcnt_ctrl:\n");
		dev_info(hw->dev, "\tdbg_set [level] [category]:\n");
		return count;
	}

	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, cmd_id, (void *)msgbuf, sizeof(struct wo_cmd_query), wait);

	return count;

}

static const struct file_operations fops_wo_ctrl = {
	.write = mtk_wed_wo_ctrl,
	.open = simple_open,
	.llseek = default_llseek,
};

void wed_wo_mcu_debugfs(struct mtk_wed_hw *hw, struct dentry *dir)
{
	if (!dir)
		return;

	debugfs_create_file("wo_write", 0600, dir, hw, &fops_wo_ctrl);
}

