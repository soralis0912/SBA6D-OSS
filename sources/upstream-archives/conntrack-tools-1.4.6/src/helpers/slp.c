/*
 * This helper creates and expectation to allow unicast replies to multicast
 * requests (RFC2608 section 6.1). While the destination address of the
 * outcoming request is known, the reply can come from any unicast address so
 * that we need to allow replies from any source address. Default expectation]
 * timeout is set one second longer than default CONFIG_MC_MAX from RFC2608
 * section 13.
 *
 * Example usage:
 *
 *     nfct add helper slp inet udp
 *     iptables -t raw -A OUTPUT -m addrtype --dst-type MULTICAST \
 *         -p udp --dport 427 -j CT --helper slp
 *     iptables -t raw -A OUTPUT -m addrtype --dst-type BROADCAST \
 *         -p udp --dport 427 -j CT --helper slp
 *     iptables -t filter -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED \
 *         -j ACCEPT
 *
 * Requires Linux 3.12 or higher. NAT is unsupported.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "conntrackd.h"
#include "helper.h"
#include "myct.h"
#include "log.h"

#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <linux/netfilter.h>

static int slp_helper_cb(struct pkt_buff *pkt, uint32_t protoff,
			 struct myct *myct, uint32_t ctinfo)
{
	struct nf_expect *exp;
	int dir = CTINFO2DIR(ctinfo);
	union nfct_attr_grp_addr saddr;
	uint16_t sport, dport;

	exp = nfexp_new();
	if (!exp) {
		pr_debug("conntrack_slp: failed to allocate expectation\n");
		return NF_ACCEPT;
	}

	cthelper_get_addr_src(myct->ct, dir, &saddr);
	cthelper_get_port_src(myct->ct, dir, &sport);
	cthelper_get_port_src(myct->ct, !dir, &dport);

	if (cthelper_expect_init(exp,
				 myct->ct,
				 0 /* class */,
				 NULL /* saddr */,
				 &saddr /* daddr */,
				 IPPROTO_UDP,
				 &dport /* sport */,
				 &sport /* dport */,
				 NF_CT_EXPECT_PERMANENT)) {
		pr_debug("conntrack_slp: failed to init expectation\n");
		nfexp_destroy(exp);
		return NF_ACCEPT;
	}

	myct->exp = exp;
	return NF_ACCEPT;
}

static struct ctd_helper slp_helper = {
	.name		= "slp",
	.l4proto	= IPPROTO_UDP,
	.priv_data_len	= 0,
	.cb		= slp_helper_cb,
	.policy		= {
		[0] = {
			.name		= "slp",
			.expect_max	= 8,
			.expect_timeout	= 16, /* default CONFIG_MC_MAX + 1 */
		},
	},
};

static void __attribute__ ((constructor)) slp_init(void)
{
	helper_register(&slp_helper);
}
