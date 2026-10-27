// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Felix Fietkau <nbd@nbd.name> */

#include <linux/etherdevice.h>
#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include "mtk_ppe.h"
#include "mtk_ppe_regs.h"

u32 debug_level;

struct mtk_ppe_addr_info
{
	u32 *src, *dest;
	u16 *src_port, *dest_port;
	bool ipv6;
};


int mtk_ppe_dbg_str_to_ip(char *str) {
        char *p_delimiter = ".";
	u32 i = 0, ip = 0, c[4];
	char *token;

        do {
		token = strsep(&str, p_delimiter);
                if (!token)
                        return 0; /* parse error */

		kstrtou32(token, 10, &c[i]);

        } while (++i < 4);


        ip = (c[0]<<24) + (c[1]<<16) + (c[2]<<8) + c[3];

	/* pr_notice("%s, %d.%d.%d.%d\n", __func__, c[0], c[1], c[2], c[3]); */
        return ip;
}

int mtk_ppe_dbg_str_to_mac(char * str, u32 *mac_hi, u16 *mac_lo) {

	char *p_delimiter = ":";
	char *token;
        int i = 0;
	u32 hi[4], lo[2];

	do {
		token = strsep(&str, p_delimiter);
                if (!token)
                        return -1; /* parse error */

		if (i < 4)
			kstrtou32(token, 16, &hi[i]);
		else
			kstrtou32(token, 16, &lo[i - 4]);
        } while (++i < 6);

	*mac_hi = (hi[0]<<24) + (hi[1]<<16) + (hi[2]<<8) + hi[3];
	*mac_lo = (lo[0]<<8) + lo[1];

	/* pr_notice("%s, %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, hi[0], hi[1], hi[2], hi[3], lo[0], lo[1]); */
        return 0;
}


void mtk_ppe_dbg_l2_info(struct mtk_foe_entry *entry, struct mtk_static_bind_info *sb_info) {
	if (sb_info->pkt_type == 0) {
		entry->ipv4.l2.dest_mac_hi = sb_info->dst_mac_hi;
		entry->ipv4.l2.dest_mac_lo = sb_info->dst_mac_lo;
		entry->ipv4.l2.src_mac_hi = sb_info->src_mac_hi;
		entry->ipv4.l2.src_mac_lo = sb_info->src_mac_lo;

		entry->ipv4.l2.vlan1 = sb_info->vlan1;
		entry->ipv4.l2.vlan2 = sb_info->vlan2;
	}
}

void mtk_ppe_dbg_l3_info(struct mtk_foe_entry *entry, struct mtk_static_bind_info *sb_info) {
	if (sb_info->pkt_type ==0) {
		entry->ipv4.orig.src_ip = sb_info->ing_sip;
		entry->ipv4.orig.dest_ip = sb_info->ing_dip;
		entry->ipv4.new.src_ip = sb_info->eg_sip;
		entry->ipv4.new.dest_ip = sb_info->eg_dip;
	}
}

void mtk_ppe_dbg_l4_info(struct mtk_foe_entry *entry, struct mtk_static_bind_info *sb_info) {
	if (sb_info->pkt_type == 0) {
		entry->ipv4.orig.dest_port = sb_info->ing_dp;
		entry->ipv4.orig.src_port = sb_info->ing_sp;
		entry->ipv4.new.dest_port = sb_info->eg_dp;
		entry->ipv4.new.src_port = sb_info->eg_sp;
	}
}

void mtk_ppe_dbg_ib1_info(struct mtk_foe_entry *entry,
	struct mtk_static_bind_info *sb_info, struct mtk_ppe *ppe) {

	if (sb_info->pkt_type == 0) {

		entry->ib1 = FIELD_PREP(MTK_FOE_IB1_STATE, MTK_FOE_STATE_BIND) |
	        	     FIELD_PREP(MTK_FOE_IB1_PACKET_TYPE, sb_info->pkt_type) |
	     		     FIELD_PREP(MTK_FOE_IB1_UDP, sb_info->is_udp) |
	     		     FIELD_PREP(MTK_FOE_IB1_BIND_VLAN_LAYER, sb_info->vlan_layer) |
	      		     MTK_FOE_IB1_BIND_TTL |
	      		     MTK_FOE_IB1_BIND_CACHE |
	      		     MTK_FOE_IB1_BIND_KEEPALIVE |
	      		     MTK_FOE_IB1_STATIC |
	      		     FIELD_PREP(MTK_FOE_IB1_BIND_TIMESTAMP, mtk_eth_timestamp(ppe->eth));
	}
}

void mtk_ppe_dbg_ib2_info(struct mtk_foe_entry *entry, struct mtk_static_bind_info *sb_info) {


	if (sb_info->pkt_type == 0) {
		entry->ipv4.ib2 =
		#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			FIELD_PREP(MTK_FOE_IB2_DEST_PORT, sb_info->pse_port & 0xf) |
		#else /* CONFIG_MEDIATEK_NETSYS_V2 */
			FIELD_PREP(MTK_FOE_IB2_DEST_PORT, sb_info->pse_port & 0x7) |
	       		FIELD_PREP(MTK_FOE_IB2_DEST_PORT2, (sb_info->pse_port & 0x8) >> 3)|
	       	#endif /* CONFIG_MEDIATEK_NETSYS_V2 */
	       		FIELD_PREP(MTK_FOE_IB2_PSE_QOS, sb_info->fqos) |
	       	#if defined(CONFIG_MEDIATEK_NETSYS_V2)
			FIELD_PREP(MTK_FOE_IB2_QID, sb_info->qid & 0x7f) |
		#else /* CONFIG_MEDIATEK_NETSYS_V2 */
	       		FIELD_PREP(MTK_FOE_IB2_QID, sb_info->qid & 0xf) |
			FIELD_PREP(MTK_FOE_IB2_QID2, ( sb_info->qid & 0x30) >> 4) |
		#endif /* CONFIG_MEDIATEK_NETSYS_V2 */
			FIELD_PREP(MTK_FOE_IB2_MULTICAST, 0) |
			FIELD_PREP(MTK_FOE_IB2_MIB_FLAG, 1) |
			FIELD_PREP(MTK_FOE_IB2_PORT_AG, sb_info->pse_port);
	}
}


static void
mtk_ppe_dbg_print_addr_info(struct mtk_ppe_addr_info *ai, bool egress)
{
	u32 src_addr[4], dest_addr[4], i;
	u16 src_port = (ai->src_port) ? (*ai->src_port) : 0;
	u16 dest_port = (ai->dest_port) ? (*ai->dest_port) : 0;

	if (!ai->ipv6) {
		pr_notice(" %s: %pI4h:%d->%pI4h:%d", egress ? "Egress": "Ingress",
			ai->src, src_port, ai->dest, dest_port);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(src_addr); i++)
		src_addr[i] = htonl(*(ai->src + i));

	for (i = 0; i < ARRAY_SIZE(dest_addr); i++)
		dest_addr[i] = htonl(*(ai->dest + i));

	pr_notice("%s: %pI6 %d->%pI6 %d", egress ? "Egress": "Ingress",
		src_addr, src_port, dest_addr, dest_port);
}


void mtk_ppe_dbg_dump_entry(uint32_t index, uint32_t ppe_index)
{
	u32 i = 0;
	u32 print_cnt;
	struct mtk_foe_entry *entry = &(nf_ppe.ppe[ppe_index]->foe_table[index]);
	u32 *p = (uint32_t *)entry;
	int type, state, sta, udp;
	struct mtk_foe_mac_info *l2;
	struct mtk_ppe_addr_info ai = {};
	unsigned char h_source[ETH_ALEN];
	unsigned char h_dest[ETH_ALEN];
	u32 ib2;

	type = FIELD_GET(MTK_FOE_IB1_PACKET_TYPE, entry->ib1);

	if (type >= MTK_PPE_PKT_TYPE_IPV4_DSLITE) {
		l2 = &entry->ipv6.l2;
		ib2 = entry->ipv6.ib2;
	} else {
		l2 = &entry->ipv4.l2;
		ib2 = entry->ipv4.ib2;
	}

	pr_notice("==========<Flow Table(%d) Entry=%d (%p)>===============\n", ppe_index, index, entry);

	print_cnt = 24;
	for (i = 0; i < print_cnt; i++)
		pr_notice("%02d: %08X\n", i, *(p + i));

	pr_notice("-----------------<Flow Info>------------------\n");
	pr_notice("Information Block 1: %08X\n", entry->ib1);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
	pr_notice("Information Block 2=%x (FP=%d FQOS=%d QID=%d)",
		ib2,
		FIELD_GET(MTK_FOE_IB2_DEST_PORT, ib2),
		FIELD_GET(MTK_FOE_IB2_PSE_QOS, ib2),
		FIELD_GET(MTK_FOE_IB2_QID, ib2));
#else /* CONFIG_MEDIATEK_NETSYS_V2 */
	pr_notice("Information Block 2=%x (FP=%d FQOS=%d QID=%d)",
		ib2,
		(FIELD_GET(MTK_FOE_IB2_DEST_PORT, ib2) & 0x7) | (FIELD_GET(MTK_FOE_IB2_DEST_PORT2, ib2) << 3),
		FIELD_GET(MTK_FOE_IB2_PSE_QOS, ib2),
		(FIELD_GET(MTK_FOE_IB2_QID, ib2) | ((FIELD_GET(MTK_FOE_IB2_QID2, ib2) & 0x3) << 4)));
#endif /* CONFIG_MEDIATEK_NETSYS_V2 */

	switch (type) {
	case MTK_PPE_PKT_TYPE_IPV4_HNAPT:
	case MTK_PPE_PKT_TYPE_IPV4_DSLITE:
		ai.src_port = &entry->ipv4.orig.src_port;
		ai.dest_port = &entry->ipv4.orig.dest_port;
		fallthrough;
	case MTK_PPE_PKT_TYPE_IPV4_ROUTE:
		ai.src = &entry->ipv4.orig.src_ip;
		ai.dest = &entry->ipv4.orig.dest_ip;
		break;
	case MTK_PPE_PKT_TYPE_IPV6_ROUTE_5T:
		ai.src_port = &entry->ipv6.src_port;
		ai.dest_port = &entry->ipv6.dest_port;
		fallthrough;
	case MTK_PPE_PKT_TYPE_IPV6_ROUTE_3T:
	case MTK_PPE_PKT_TYPE_IPV6_6RD:
		ai.src = entry->ipv6.src_ip;
		ai.dest = entry->ipv6.dest_ip;
		ai.ipv6 = true;
		break;
	}

	pr_notice("Create %s entry\n", mtk_foe_pkt_type_str(type));

	mtk_ppe_dbg_print_addr_info(&ai, false);

	switch (type) {
	case MTK_PPE_PKT_TYPE_IPV4_HNAPT:
	case MTK_PPE_PKT_TYPE_IPV4_DSLITE:
		ai.src_port = &entry->ipv4.new.src_port;
		ai.dest_port = &entry->ipv4.new.dest_port;
		fallthrough;
	case MTK_PPE_PKT_TYPE_IPV4_ROUTE:
		ai.src = &entry->ipv4.new.src_ip;
		ai.dest = &entry->ipv4.new.dest_ip;

		mtk_ppe_dbg_print_addr_info(&ai, true);
		break;
	}

	*((__be32 *)h_source) = htonl(l2->src_mac_hi);
	*((__be16 *)&h_source[4]) = htons(l2->src_mac_lo);
	*((__be32 *)h_dest) = htonl(l2->dest_mac_hi);
	*((__be16 *)&h_dest[4]) = htons(l2->dest_mac_lo);

	pr_notice("DMAC=%pM SMAC=%pM\n", h_dest, h_source);

	state = FIELD_GET(MTK_FOE_IB1_STATE, entry->ib1);
	sta = FIELD_GET(MTK_FOE_IB1_STATIC, entry->ib1);
	pr_notice("State = %s, STC: %s", state == 0 ? "Invalid" :
				state == 1 ? "Unbind" :
				state == 2 ? "BIND" :
				state == 3 ? "FIN" : "Unknown",
				sta == 0 ? "Dynamic" :
				sta == 1 ? "static" : "Unknown");

	pr_notice("Vlan_Layer = %u, Eth_type = 0x%x, vlan1=0x%x, vlan2=0x%x\n",
		FIELD_GET(MTK_FOE_IB1_BIND_VLAN_LAYER, entry->ib1),
		ntohs(l2->etype), l2->vlan1, l2->vlan2);

	udp = FIELD_GET(MTK_FOE_IB1_UDP, entry->ib1);
	pr_notice("mib = %d, multicast = %d, pppoe = %d, proto = %s\n",
		FIELD_GET(MTK_FOE_IB2_MIB_FLAG, ib2),
		FIELD_GET(MTK_FOE_IB2_MULTICAST, ib2),
		FIELD_GET(MTK_FOE_IB1_BIND_PPPOE, entry->ib1),
		udp == 0 ? "TCP" : udp == 1 ? "UDP" : "Unknown");

	pr_notice("=========================================\n\n");
}

static int
mtk_ppe_dbg_foe_show(struct seq_file *m, struct mtk_ppe *ppe, bool bind)
{
	int i, count = 0;

	for (i = 0; i < MTK_PPE_ENTRIES; i++) {
		struct mtk_foe_entry *entry = &ppe->foe_table[i];
		struct mtk_flow_addr_info ai = {};
		int type, state;
		u32 ib2;


		state = FIELD_GET(MTK_FOE_IB1_STATE, entry->ib1);
		if (!state)
			continue;

		if ((bind && state != MTK_FOE_STATE_BIND) ||
		    (!bind && state != MTK_FOE_STATE_UNBIND))
			continue;

		mtk_foe_entry_get_mib(ppe, i, NULL);

		type = FIELD_GET(MTK_FOE_IB1_PACKET_TYPE, entry->ib1);

		seq_printf(m, "%s=%d(%d):",
		 	mtk_foe_pkt_type_str(type), i, ppe->id);

		switch (type) {
		case MTK_PPE_PKT_TYPE_IPV4_HNAPT:
		case MTK_PPE_PKT_TYPE_IPV4_DSLITE:
			ai.src_port = &entry->ipv4.orig.src_port;
			ai.dest_port = &entry->ipv4.orig.dest_port;
			fallthrough;
		case MTK_PPE_PKT_TYPE_IPV4_ROUTE:
			ai.src = &entry->ipv4.orig.src_ip;
			ai.dest = &entry->ipv4.orig.dest_ip;
			break;
		case MTK_PPE_PKT_TYPE_IPV6_ROUTE_5T:
			ai.src_port = &entry->ipv6.src_port;
			ai.dest_port = &entry->ipv6.dest_port;
			fallthrough;
		case MTK_PPE_PKT_TYPE_IPV6_ROUTE_3T:
		case MTK_PPE_PKT_TYPE_IPV6_6RD:
			ai.src = &entry->ipv6.src_ip;
			ai.dest = &entry->ipv6.dest_ip;
			ai.ipv6 = true;
			break;
		}

		mtk_print_addr_info(m, &ai);

		switch (type) {
		case MTK_PPE_PKT_TYPE_IPV4_HNAPT:
		case MTK_PPE_PKT_TYPE_IPV4_DSLITE:
			ai.src_port = &entry->ipv4.new.src_port;
			ai.dest_port = &entry->ipv4.new.dest_port;
			fallthrough;
		case MTK_PPE_PKT_TYPE_IPV4_ROUTE:
			ai.src = &entry->ipv4.new.src_ip;
			ai.dest = &entry->ipv4.new.dest_ip;
			seq_printf(m, " => ");
			mtk_print_addr_info(m, &ai);
			break;
		}

		if (type >= MTK_PPE_PKT_TYPE_IPV4_DSLITE)
			ib2 = entry->ipv6.ib2;
		else
			ib2 = entry->ipv4.ib2;
	#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		seq_printf(m, "(FP=%ld FQOS=%ld QID=%ld)\n",
			FIELD_GET(MTK_FOE_IB2_DEST_PORT, ib2),
			FIELD_GET(MTK_FOE_IB2_PSE_QOS, ib2),
			FIELD_GET(MTK_FOE_IB2_QID, ib2));
	#else /* CONFIG_MEDIATEK_NETSYS_V2 */
		seq_printf(m, "(FP=%ld FQOS=%ld QID=%ld)\n",
			(FIELD_GET(MTK_FOE_IB2_DEST_PORT, ib2) & 0x7) | (FIELD_GET(MTK_FOE_IB2_DEST_PORT2, ib2) << 3),
			FIELD_GET(MTK_FOE_IB2_PSE_QOS, ib2),
			(FIELD_GET(MTK_FOE_IB2_QID, ib2) | ((FIELD_GET(MTK_FOE_IB2_QID2, ib2) & 0x3) << 4)));
	#endif /* CONFIG_MEDIATEK_NETSYS_V2 */

		count ++;

	}

	return count;
}


int mtk_ppe_dbg_get_unbind_read(struct seq_file *seq, void *v)
{
	int i, count = 0;

	for (i = 0; i < nf_ppe.ppe_num; i++)
		count += mtk_ppe_dbg_foe_show(seq, nf_ppe.ppe[i], false);

	seq_printf(seq, "Total Entry Count = %d\n", count);

	return 0;

}

int mtk_ppe_dbg_get_bind_read(struct seq_file *seq, void *v)
{
	int i, count = 0;

	for (i = 0; i < nf_ppe.ppe_num; i++)
		count += mtk_ppe_dbg_foe_show(seq, nf_ppe.ppe[i], true);

	seq_printf(seq, "Total Entry Count = %d\n", count);
	return 0;
}



void mtk_ppe_dbg_mib_dump(u32 entry_num)
{
	int i;
	struct mtk_foe_accounting diff;
	struct mtk_foe_accounting *acct;

	for (i = 0; i < 2; i++) {

		acct = mtk_foe_entry_get_mib(nf_ppe.ppe[i], entry_num, &diff);

		pr_notice("************PPE%d Entry = %d ************\n", i, entry_num);
		pr_notice("Packet Cnt = %llu (+%llu)\n", acct->packets, diff.packets);

		pr_notice("Byte Cnt = %llu (+%llu)\n", acct->bytes, diff.bytes);
	}

	pr_notice("**************************************************************\n");

}


void mtk_ppe_dbg_set_debug_level(uint32_t level)
{
	/* Set debug level */

	pr_notice("****************************\n");
	pr_notice("%s, debug_level:%d\n", __func__, level);

	debug_level = level;
}

void mtk_ppe_dbg_enable_hw_qos(uint32_t enable)
{
	/* Enable HW QoS */

	pr_notice("****************************\n");
	pr_notice("%s, enable:%d\n", __func__, enable);

	fqos = enable;
}


void mtk_ppe_dbg_enable_eth_auto_qos(uint32_t enable)
{
	/* Enable ETH auto QoS */

	pr_notice("****************************\n");
	pr_notice("%s, enable:%d\n", __func__, enable);

	eth_qos_enable = enable;
}



void mtk_ppe_dbg_set_bind_threshold(uint32_t threshold)
{
	/* Set reach bind rate for unbind state */
	u32 val;

	pr_notice("****************************\n");
	pr_notice("%s, threshold:%d\n", __func__, threshold);

	val = FIELD_PREP(MTK_PPE_BIND_RATE_BIND, threshold);
	ppe_w32(nf_ppe.ppe[0], MTK_PPE_BIND_RATE, val);
	ppe_w32(nf_ppe.ppe[1], MTK_PPE_BIND_RATE, val);

}


void mtk_ppe_dbg_set_bind_lifetime(u16 tcp_life, uint16_t udp_life, uint16_t fin_life)
{
	u32 val;

	pr_notice("****************************\n");
	pr_notice("%s, tcp:%d, udp:%d, fin:%d\n", __func__, tcp_life, udp_life, fin_life);

	/* set Delta time for aging out an bind UDP FOE entry */
	val = FIELD_PREP(MTK_PPE_BIND_AGE0_DELTA_UDP, udp_life);

	ppe_w32(nf_ppe.ppe[0], MTK_PPE_BIND_AGE0, val);
	ppe_w32(nf_ppe.ppe[1], MTK_PPE_BIND_AGE0, val);

	/* set Delta time for aging out an bind TCP_FIN and TCP FOE entry */
	val = FIELD_PREP(MTK_PPE_BIND_AGE1_DELTA_TCP_FIN, fin_life) |
	      FIELD_PREP(MTK_PPE_BIND_AGE1_DELTA_TCP, tcp_life);

	ppe_w32(nf_ppe.ppe[0], MTK_PPE_BIND_AGE1, val);
	ppe_w32(nf_ppe.ppe[1], MTK_PPE_BIND_AGE1, val);

}

u32 mtk_ppe_dbg_get_hash_ipv4(u32 saddr, u32 daddr, u32 sport, u32 dport) {

	u32 t_hvt_31;
	u32 t_hvt_63;
	u32 t_hvt_95;
	u32 t_hvt_sd;

	u32 t_hvt_sd_23;
	u32 t_hvt_sd_31_24;
	u32 t_hash_32;
	u32 t_hashs_16;
	u32 t_ha16k;

	u32 hash_index;

	t_hvt_31 = sport << 16 | dport;
	t_hvt_63 = daddr;
	t_hvt_95 = saddr;

	if (DFL_FOE_HASH_MODE == 1)/* hash mode 1 */
		t_hvt_sd = (t_hvt_31 & t_hvt_63) | ((~t_hvt_31) & t_hvt_95);
	else                            /* hash mode 2 */
		t_hvt_sd = t_hvt_63 ^ (t_hvt_31 & (~t_hvt_95));

	t_hvt_sd_23 = t_hvt_sd & 0xffffff;
	t_hvt_sd_31_24 = t_hvt_sd & 0xff000000;
	t_hash_32 = t_hvt_31 ^ t_hvt_63 ^ t_hvt_95 ^ ((t_hvt_sd_23 << 8) | (t_hvt_sd_31_24 >> 24));
	t_hashs_16 = ((t_hash_32 & 0xffff0000) >> 16) ^ (t_hash_32 & 0xfffff);

	if (FOE_4TB_SIZ == 32768 || FOE_4TB_SIZ == 16384)
		t_ha16k = t_hashs_16 & 0x1fff;  /* FOE_16k */
	else if (FOE_4TB_SIZ == 8192)
		t_ha16k = t_hashs_16 & 0xfff;  /* FOE_8k */
	else if (FOE_4TB_SIZ == 4096)
		t_ha16k = t_hashs_16 & 0x7ff;  /* FOE_4k */
	else if (FOE_4TB_SIZ == 2048)
		t_ha16k = t_hashs_16 & 0x3ff;  /* FOE_2k */
	else
		t_ha16k = t_hashs_16 & 0x1ff;  /* FOE_1k */

	hash_index = (u32)t_ha16k * 4;

	pr_notice("%s, saddr = %x, daddr=%x, sport=%d, dport=%d, hash_index=%d\n",
		__func__, saddr, daddr, sport, dport, hash_index);

	return hash_index;
}


void mtk_ppe_dbg_add_static_bind(struct mtk_static_bind_info *sb_info) {

	struct mtk_ppe *ppe;
	struct mtk_foe_entry *entry;

	u32 hash_index = mtk_ppe_dbg_get_hash_ipv4(
		sb_info->ing_sip, sb_info->ing_dip, sb_info->ing_sp, sb_info->ing_dp);

	if (hash_index <= 0 || hash_index > FOE_4TB_SIZ) {
		pr_notice("%s, invalid hash index\n");
		return;
	}

	if (sb_info->ppe_idx != 0 && sb_info->ppe_idx != 1) {
		pr_notice("%s, wrong ppe_idx: %d\n", sb_info->ppe_idx);
		return;
	}

	ppe = nf_ppe.ppe[sb_info->ppe_idx];
	entry = &ppe->foe_table[hash_index];

	memset(entry, 0, sizeof(*entry));


	mtk_ppe_dbg_l2_info(entry, sb_info);
	mtk_ppe_dbg_l3_info(entry, sb_info);
	if (sb_info->pkt_type == 0)
		mtk_ppe_dbg_l4_info(entry, sb_info);

	mtk_ppe_dbg_ib1_info(entry, sb_info, ppe);
	mtk_ppe_dbg_ib2_info(entry, sb_info);

	mtk_ppe_cache_enable(ppe, true);

	mtk_ppe_dbg_dump_entry(hash_index, sb_info->ppe_idx);
}


void mtk_ppe_dbg_tbl_clear(void) {

	/* ppe0 */
	ppe_m32(nf_ppe.ppe[0], MTK_PPE_TB_CFG, MTK_PPE_TB_CFG_SEARCH_MISS,
		MTK_PPE_SEARCH_MISS_ACTION_FORWARD);
	mtk_ppe_init_foe_table(nf_ppe.ppe[0]);
	mtk_ppe_cache_enable(nf_ppe.ppe[0], true);
	ppe_m32(nf_ppe.ppe[0], MTK_PPE_TB_CFG, MTK_PPE_TB_CFG_SEARCH_MISS,
		MTK_PPE_SEARCH_MISS_ACTION_FORWARD_BUILD);

	/* ppe1 */
	ppe_m32(nf_ppe.ppe[1], MTK_PPE_TB_CFG, MTK_PPE_TB_CFG_SEARCH_MISS,
		MTK_PPE_SEARCH_MISS_ACTION_FORWARD);
	mtk_ppe_init_foe_table(nf_ppe.ppe[1]);
	mtk_ppe_cache_enable(nf_ppe.ppe[1], true);
	ppe_m32(nf_ppe.ppe[1], MTK_PPE_TB_CFG, MTK_PPE_TB_CFG_SEARCH_MISS,
		MTK_PPE_SEARCH_MISS_ACTION_FORWARD_BUILD);

}


int mtk_ppe_dbg_set_help(struct seq_file *seq, void *v)
{
	char setfs[] = "/sys/kernel/debug/mtk_ppe/set";

	seq_printf(seq, "1.[show help]\n");
	seq_printf(seq, "cat %s\n\n", setfs);

	seq_printf(seq, "2.[set age-out threshold]\n");
	seq_printf(seq, "echo U [TCP_age] [UDP_age] [FIN_age] > %s\n\n", setfs);

	seq_printf(seq, "3.[set bind threshold]\n");
	seq_printf(seq, "echo N [rate] > %s\n\n", setfs);

	seq_printf(seq, "4.[set debug level]\n");
	seq_printf(seq, "echo d [debug_level] > %s\n\n", setfs);

	seq_printf(seq, "5.[enable HW QoS]\n");
	seq_printf(seq, "echo Q [1:enable, 0:disable] > %s\n\n", setfs);

	seq_printf(seq, "6.[enable ETH auto QoS]\n");
	seq_printf(seq, "echo q [1:enable, 0:disable] > %s\n\n", setfs);

	seq_printf(seq, "7.[add static bind]\n");
	seq_printf(seq, "echo a [PPE0|PPE1] [ipv4|ipv6] [src mac] [dst mac]\n");
	seq_printf(seq, "     [src ip] [dst ip] [src port] [dst port]\n");
	seq_printf(seq, "     [new src ip] [new dst ip] [new src port] [new dst port]\n");
	seq_printf(seq, "     [Tcp/Udp] [CPU|GE1|GE2|Px] [ring_id] [abb(a:fqos,b:qid]\n");
	seq_printf(seq, "     [aaabbbccc(a:vlan_layer,b:vlan1,c:vlan2]\n");
	seq_printf(seq, " > %s\n\n", setfs);

	seq_printf(seq, "7. Example:\n");
	seq_printf(seq, "echo a PPE0 ipv4 00:00:00:00:00:01 00:00:00:00:00:02\n");
	seq_printf(seq, "     192.168.1.117 192.168.1.217 53041 5201\n");
	seq_printf(seq, "     192.168.1.117 192.168.1.217 53041 5201\n");
	seq_printf(seq, "     Tcp GE1 0 0 0 > %s\n\n", setfs);

	seq_printf(seq, "8.[dump ppe entry's mib count]\n");
	seq_printf(seq, "echo m [entry_idx] > %s\n\n", setfs);

	seq_printf(seq, "9.[dump ppe entry]\n");
	seq_printf(seq, "echo c [entry_idx] P [ppe_idx] > %s\n\n", setfs);

	seq_printf(seq, "10.[remove all ppe entries]\n");
	seq_printf(seq, "echo y > %s\n\n", setfs);

	return 0;
}

ssize_t mtk_ppe_dbg_set_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	char buf[1024], *token, *p = buf;
	int len = count;
	char *p_delimiter = " \t";
	u32 val1 = 0, val2 = 0, val3 = 0;
	int ret = 0;
	struct mtk_static_bind_info sb_info = {0};

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, buffer, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, p_delimiter);
	if (!token)
		return -EINVAL;

	switch (*token) {

	case 'U':
		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val2);
		if (!token || ret < 0)
			return -EINVAL;

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val3);
		if (!token || ret < 0)
			return -EINVAL;

		mtk_ppe_dbg_set_bind_lifetime(val1, val2, val3);
		break;

	case 'N':

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		mtk_ppe_dbg_set_bind_threshold(val1);
		break;

	case 'd':

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		mtk_ppe_dbg_set_debug_level(val1);
		break;

	case 'Q':

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		mtk_ppe_dbg_enable_hw_qos(val1);
		break;

	case 'q':

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 10, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		mtk_ppe_dbg_enable_eth_auto_qos(val1);
		break;

	/* static bind */
	case 'a':

		token = strsep(&p, p_delimiter);
		if (token) {
			if (strcmp(token, "PPE0") == 0)
				sb_info.ppe_idx = 0;
			else if (strcmp(token, "PPE1") == 0)
				sb_info.ppe_idx = 1;
			else
				ret = -1;
		}
		if (!token || ret < 0) {
			pr_notice("%s, can't parse [PPE0|PPE1]\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			if (strcmp(token, "ipv4") == 0)
				sb_info.pkt_type = 0;
			else if (strcmp(token, "ipv6") == 0)
				sb_info.pkt_type = 5;
			else
				ret = -1;
		}
		if (!token || ret < 0) {
			pr_notice("%s, can't parse [ipv4|ipv6]\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			if (is_valid_ether_addr(token))
				ret = mtk_ppe_dbg_str_to_mac(token, &sb_info.src_mac_hi, &sb_info.src_mac_lo);
			else {
				pr_notice("%s, invalid_ether_addr (src_mac):%s\n", __func__, token);
				ret = -1;
			}
		}
		if (!token || ret < 0) {
			pr_notice("%s, can't parse src_mac\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			if (is_valid_ether_addr(token))
				ret = mtk_ppe_dbg_str_to_mac(token, &sb_info.dst_mac_hi, &sb_info.dst_mac_lo);
			else {
				pr_notice("%s, invalid_ether_addr (dst_mac):%s\n", __func__, token);
				ret = -1;
			}
		}
		if (!token || ret < 0) {
			pr_notice("%s, can't parse dst_mac\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			sb_info.ing_sip = mtk_ppe_dbg_str_to_ip(token);
			if (sb_info.ing_sip == 0)
				ret = -1;
		}

		if (!token || ret < 0) {
			pr_notice("%s, can't parse ing_sip\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			sb_info.ing_dip = mtk_ppe_dbg_str_to_ip(token);
			if (sb_info.ing_dip == 0)
				ret = -1;
		}

		if (!token || ret < 0) {
			pr_notice("%s, can't parse ing_dip\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou16(token, 10, &sb_info.ing_sp);

		if (!token || ret < 0) {
			pr_notice("%s, can't parse ing_sp\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou16(token, 10, &sb_info.ing_dp);

		if (!token || ret < 0) {
			pr_notice("%s, can't parse ing_dp\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			sb_info.eg_sip = mtk_ppe_dbg_str_to_ip(token);
			if (sb_info.eg_sip == 0)
				ret = -1;
		}

		if (!token || ret < 0) {
			pr_notice("%s, can't parse eg_sip\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			sb_info.eg_dip = mtk_ppe_dbg_str_to_ip(token);
			if (sb_info.eg_dip == 0)
				ret = -1;
		}

		if (!token || ret < 0) {
			pr_notice("%s, can't parse eg_dip\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou16(token, 10, &sb_info.eg_sp);

		if (!token || ret < 0) {
			pr_notice("%s, can't parse eg_sp\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou16(token, 10, &sb_info.eg_dp);

		if (!token || ret < 0) {
			pr_notice("%s, can't parse eg_dp\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			if (strcmp(token, "Tcp") == 0)
				sb_info.is_udp = 0;
			else if (strcmp(token, "Udp") == 0)
				sb_info.is_udp = 1;
			else
				ret = -1;
		}
		if (!token || ret < 0) {
			pr_notice("%s, can't parse [Tcp|Udp]\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			if (strcmp(token, "CPU") == 0)
				sb_info.pse_port = 0;
			else if (strncmp(token, "GE", 2) == 0)
				ret = kstrtou8(token + 2, 10, &sb_info.pse_port);
			else if (strncmp(token, "P", 1) == 0)
				ret = kstrtou8(token + 1, 10, &sb_info.pse_port);
			else
				ret = -1;
		}
		if (!token || ret < 0) {
			pr_notice("%s, can't parse [CPU|GE1|GE2|Px]\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou8(token, 10, &sb_info.ring_id);

		if (!token || ret < 0) {
			pr_notice("%s, can't parse ring_id\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			ret = kstrtou32(token, 10, &val1);

			if (ret == 0) {
				sb_info.fqos = val1 / 100;
				sb_info.qid = (val1 % 100);
			}
		}

		if (!token || ret < 0) {
			pr_notice("%s, can't parse [abb(a:fqos,b:qid]\n", __func__);
			return -EINVAL;
		}

		token = strsep(&p, p_delimiter);
		if (token) {
			ret = kstrtou32(token, 10, &val1);

			if (ret == 0) {
				sb_info.vlan_layer = val1 / 1000000;
		 		sb_info.vlan1 = (val1 % 1000000) / 1000;
		 		sb_info.vlan2 = (val1 % 1000);
			}
		}

		if (!token || ret < 0) {
			pr_notice("%s, can't parse [aaabbbccc(a:vlan_layer,b:vlan1,c:vlan2]\n", __func__);
			return -EINVAL;
		}

		mtk_ppe_dbg_add_static_bind(&sb_info);
		break;

	case 'm':
		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 0, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		if (val1 >= MTK_PPE_ENTRIES)
			return -EINVAL;

		mtk_ppe_dbg_mib_dump(val1);
		break;

	case 'c':
		/* entry index */
		token = strsep(&p, p_delimiter);
		if (token)
			ret = kstrtou32(token, 0, &val1);
		if (!token || ret < 0)
			return -EINVAL;

		if (val1 >= MTK_PPE_ENTRIES)
			return -EINVAL;

		/* ppe index */
		token = strsep(&p, p_delimiter);
		if (token && *token == 'P') {
			token = strsep(&p, p_delimiter);
			if (token)
				ret = kstrtou32(token, 0, &val2);
			if (!token || ret < 0)
				return -EINVAL;
		}

		if (val2 >= 2)
			return -EINVAL;

		mtk_ppe_dbg_dump_entry(val1, val2);
		break;

	case 'y':
		/* clear ppe table */
		mtk_ppe_dbg_tbl_clear();

	default:
		pr_notice("%s, undefined command: %c\n\r", __func__, *token);

	}

	return len;

}


static int
mtk_ppe_dbg_set_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_ppe_dbg_set_help,
			   inode->i_private);
}

static int
mtk_ppe_dbg_get_unbind_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_ppe_dbg_get_unbind_read,
			   inode->i_private);
}

static int
mtk_ppe_dbg_get_bind_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_ppe_dbg_get_bind_read,
			   inode->i_private);
}



int mtk_ppe_dbg_init(struct dentry *root)
{
	static const struct file_operations fops_set = {
		.open = mtk_ppe_dbg_set_open,
		.read = seq_read,
		.write = mtk_ppe_dbg_set_write,
		.llseek = seq_lseek,
		.release = single_release,
	};

	static const struct file_operations fops_get_unbind = {
		.open = mtk_ppe_dbg_get_unbind_open,
		.read = seq_read,
		.llseek = seq_lseek,
		.release = single_release,
	};

	static const struct file_operations fops_get_bind = {
		.open = mtk_ppe_dbg_get_bind_open,
		.read = seq_read,
		.llseek = seq_lseek,
		.release = single_release,
	};

	debugfs_create_file("set", S_IFREG | S_IWUSR, root, 0, &fops_set);
	debugfs_create_file("get_unbind", S_IFREG | S_IWUSR, root, 0, &fops_get_unbind);
	debugfs_create_file("get_bind", S_IFREG | S_IWUSR, root, 0, &fops_get_bind);
	return 0;
}
