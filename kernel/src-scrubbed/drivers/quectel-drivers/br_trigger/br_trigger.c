
#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define _TAG "BR MAC"
#define QLOGD(f,...)        do{\
            printk("[%s]%s():%d " f " ",_TAG,__FUNCTION__,__LINE__,## __VA_ARGS__);}while(0);

#define BR_NETLINK_MSG    25
#define BR_NETLINK_PORT   52

#define MTK_WIFI_CLIENT_ONLINE 0x3000
#define MTK_WIFI_CLIENT_OFFLINE 0x3001

struct sock *br_nl_fd = NULL;

#define NETDEV_STR_LEN   (32)
struct _br_netdev_port_data {
	char port_name[NETDEV_STR_LEN];
	char mac_addr[ETH_ALEN];
	int link_state;
};

static struct _br_netdev_port_data g_br_state;

struct br_netdev_data {

	struct delayed_work work;
	struct notifier_block notifier;

	int interval;
	struct _br_netdev_port_data port_data[1];

};
struct br_netdev_data g_trigger_data;

static char rai_ap_mac[ETH_ALEN];
static char rax_ap_mac[ETH_ALEN];
static char ra_ap_mac[ETH_ALEN];
void inject_rai_ap_mac(const u8 *mac_addr)
{
	memcpy(rai_ap_mac, mac_addr, ETH_ALEN);
}
EXPORT_SYMBOL(inject_rai_ap_mac);
void inject_rax_ap_mac(const u8 *mac_addr)
{
	memcpy(rax_ap_mac, mac_addr, ETH_ALEN);
}
EXPORT_SYMBOL(inject_rax_ap_mac);
void inject_ra_ap_mac(const u8 *mac_addr)
{
	memcpy(ra_ap_mac, mac_addr, ETH_ALEN);
}
EXPORT_SYMBOL(inject_ra_ap_mac);

void br_update_link(char *port_name, int status, u8 *mac_addr)
{
	memcpy(g_br_state.mac_addr, mac_addr, ETH_ALEN);
	memcpy(g_br_state.port_name, port_name, NETDEV_STR_LEN);
	g_br_state.link_state = status;
}

int br_send_msg(struct _br_netdev_port_data *br_state)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;

    int ret;
    nl_skb = nlmsg_new(sizeof(struct _br_netdev_port_data), GFP_ATOMIC);
    if(!nl_skb)
    {
        pr_err("netlink_alloc_skb error\n");
        return -1;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, BR_NETLINK_MSG, sizeof(struct _br_netdev_port_data), 0);

    if(nlh == NULL)
    {
        pr_err("nlmsg_put() error\n");
        nlmsg_free(nl_skb);
        return -1;
    }
    memcpy(nlmsg_data(nlh), br_state, sizeof(struct _br_netdev_port_data));

    ret = netlink_unicast(br_nl_fd, nl_skb, BR_NETLINK_PORT, MSG_DONTWAIT);

    return ret;
}
static void recv_cb(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    void *data = "NULL";

    QLOGD("skb->len:%u\n", skb->len);
    if(skb->len >= nlmsg_total_size(0))
    {
        nlh = nlmsg_hdr(skb);
        data = NLMSG_DATA(nlh);
        if(data)
        {
            QLOGD("kernel receive data: %s\n", (char *)data);
            br_send_msg(&g_br_state);
        }
    }
}

struct netlink_kernel_cfg br_trig_cfg =
{
    .input = recv_cb,
};

enum BR_DEV_NO {
	SWITCH_P0,
	SWITCH_P1,
	SWITCH_P2,
	SWITCH_P3,
	SWITCH_P4,
};
unsigned long __pack_devno_evt(enum BR_DEV_NO dev_no, unsigned long evt)
{
	return (unsigned long)(dev_no<<8 | evt&0xff)&0xffff;
}
EXPORT_SYMBOL(__pack_devno_evt);
void __unpack_devno_evt(unsigned long event, enum BR_DEV_NO *dev_no, unsigned long *evt)
{
	*dev_no = (event>>8)&0xff;
	*evt = event&0xff;
}
EXPORT_SYMBOL(__unpack_devno_evt);

int net_sw_trig_notify(enum BR_DEV_NO dev_n, unsigned long evt)
{
	bool up;

	switch (evt) {
	case NETDEV_CHANGENAME:
	case NETDEV_REGISTER:
		break;
	case NETDEV_UNREGISTER:
		break;
	case NETDEV_UP:
	case NETDEV_DOWN:
		if(evt == NETDEV_UP)
			up = 1;
		if(evt == NETDEV_DOWN)
			up = 0;
		cancel_delayed_work_sync(&g_trigger_data.work);
		snprintf(g_trigger_data.port_data[0].port_name, \
				sizeof(g_trigger_data.port_data[0].port_name), "switch_port:%d", dev_n);
		g_trigger_data.port_data[0].link_state = up;
		schedule_delayed_work(&g_trigger_data.work, g_trigger_data.interval);
		break;
	}
	return 0;
}

BLOCKING_NOTIFIER_HEAD(br_trig_chain_head);
EXPORT_SYMBOL_GPL(br_trig_chain_head);
static int br_trig_chain_notify(struct notifier_block *nb, unsigned long event, void *_unused)
{
	enum BR_DEV_NO dev_n = 0;
	unsigned long evt = 0;
	//|<--BR_DEV_NO-->|<--evt-->|
	// QLOGD("evt %x\n", event);
	__unpack_devno_evt(event, &dev_n, &evt);
	net_sw_trig_notify(dev_n, evt);
	return 0;
}
static struct notifier_block br_trig_chain_nb = {
	.notifier_call = br_trig_chain_notify,
};

static int netdev_trig_notify(struct notifier_block *nb,
			      unsigned long evt, void *dv)
{
	struct net_device *dev =
		netdev_notifier_info_to_dev((struct netdev_notifier_info *)dv);
	struct br_netdev_data *trigger_data =
		container_of(nb, struct br_netdev_data, notifier);
	bool up;

	if (evt != NETDEV_UP && evt != NETDEV_DOWN && evt != NETDEV_CHANGE
	    && evt != NETDEV_REGISTER && evt != NETDEV_UNREGISTER
	    && evt != NETDEV_CHANGENAME && evt != MTK_WIFI_CLIENT_ONLINE && evt != MTK_WIFI_CLIENT_OFFLINE)
		return NOTIFY_DONE;

	switch (evt) {
	case NETDEV_CHANGENAME:
	case NETDEV_REGISTER:
		break;
	case NETDEV_UNREGISTER:
		break;
	case NETDEV_DOWN:
	case NETDEV_UP:
	case NETDEV_CHANGE:
		if(!(strstr(dev->name, "br-lan") || strstr(dev->name, "lan")))
		{
			cancel_delayed_work_sync(&trigger_data->work);
			up = netif_carrier_ok(dev);
			snprintf(trigger_data->port_data[0].port_name, \
					sizeof(trigger_data->port_data[0].port_name), "%s", dev->name);
			trigger_data->port_data[0].link_state = up;
			schedule_delayed_work(&trigger_data->work, trigger_data->interval);
			QLOGD("%s %s\n", dev->name, up?"up":"down");
		}
		break;
	case MTK_WIFI_CLIENT_ONLINE:
	case MTK_WIFI_CLIENT_OFFLINE:	//in logan/wifi_driver/mt_wifi/os/linux/cfg80211/cfg80211.c
		{
			cancel_delayed_work_sync(&trigger_data->work);
			if(evt == MTK_WIFI_CLIENT_ONLINE){
				up = 1;
			}
			if(evt == MTK_WIFI_CLIENT_OFFLINE) {
				up = 0;
			}

			if(strstr(dev->name, "rai")) {
				memcpy(trigger_data->port_data[0].mac_addr, rai_ap_mac, ETH_ALEN);
			}
			else if(strstr(dev->name, "rax")) {
				memcpy(trigger_data->port_data[0].mac_addr, rax_ap_mac, ETH_ALEN);
			}
			else if(strstr(dev->name, "ra")) {
				memcpy(trigger_data->port_data[0].mac_addr, ra_ap_mac, ETH_ALEN);
			}
			QLOGD("%s %s mac:%02x:%02x:%02x:%02x:%02x:%02x\n", \
				dev->name, up?"up":"down", \
				trigger_data->port_data[0].mac_addr[0], trigger_data->port_data[0].mac_addr[1], trigger_data->port_data[0].mac_addr[2], \
				trigger_data->port_data[0].mac_addr[3], trigger_data->port_data[0].mac_addr[4], trigger_data->port_data[0].mac_addr[5]);

			snprintf(trigger_data->port_data[0].port_name, \
					sizeof(trigger_data->port_data[0].port_name), "%s", dev->name);
			trigger_data->port_data[0].link_state = up;

			schedule_delayed_work(&trigger_data->work, trigger_data->interval);
			break;
		}
	}

	return NOTIFY_DONE;
}

/* here's the real work! */
static void netdev_trig_work(struct work_struct *work)
{
	struct br_netdev_data *trigger_data =
		container_of(work, struct br_netdev_data, work.work);
	QLOGD("%s %s send to app\n", \
			trigger_data->port_data[0].port_name, trigger_data->port_data[0].link_state?"up":"down");
	br_update_link(trigger_data->port_data[0].port_name, trigger_data->port_data[0].link_state, trigger_data->port_data[0].mac_addr);
	br_send_msg(&g_br_state);
}

static int netdev_trig_activate(struct br_netdev_data *trigger_data)
{
	int rc;

	trigger_data->notifier.notifier_call = netdev_trig_notify;
	trigger_data->notifier.priority = 10;
	INIT_DELAYED_WORK(&trigger_data->work, netdev_trig_work);

	trigger_data->interval = msecs_to_jiffies(1500);
	trigger_data->port_data[0].link_state = 0;

	rc = register_netdevice_notifier(&trigger_data->notifier);
	if (rc){
		kfree(trigger_data);
	}else {
		br_nl_fd = netlink_kernel_create(&init_net, BR_NETLINK_MSG, &br_trig_cfg);
		if (br_nl_fd == NULL)
		{
			QLOGD("get a NULL netlink fd!!!");
			goto OUT;
		}
		QLOGD("Success..\n");
		blocking_notifier_chain_register(&br_trig_chain_head, &br_trig_chain_nb);
	}
OUT:
	return rc;
}

static int __init netdev_trig_init(void)
{
	QLOGD("\n");
	return netdev_trig_activate(&g_trigger_data);
}

static void __exit netdev_trig_exit(void)
{
	// unregister_netdevice_notifier(&g_trigger_data.notifier);

	// cancel_delayed_work_sync(&g_trigger_data.work);

}

late_initcall(netdev_trig_init);
module_exit(netdev_trig_exit);

MODULE_AUTHOR("Joe Huang <joe.huang@quectel.com>");
MODULE_DESCRIPTION("Netdev br-lan trigger & push link status to app");
MODULE_LICENSE("GPL v2");
