#ifndef QL_ETH_PHY_H
#define QL_ETH_PHY_H

#define MDIO_BCAST_ADDR 0x0
#define MTK_GPIO_OFFSET 276

#define TRUE 1
#define FALSE 0

#define QLOGD(f,...)        do{\
            printk("[%s]%s():%d " f " ",NET_TAG,__FUNCTION__,__LINE__,## __VA_ARGS__);}while(0);

#define _TRACE_(id) QLOGD("mac:%d\n", id)

#define GetBit(x,y) ((x) >> (y)&1)
#define SetBit(x,y) x|=(1<<y)
#define ClrBit(x,y) x&=~(1<<y)

enum ql_speed_bits_type {
    speed_down_bits = -1,
    speed_10M_bits = 0,
    speed_100M_bits = 1,
    speed_1G_bits = 2,
    speed_10G_bits = 3,
    speed_2p5G_bits = 4,
    speed_5G_bits = 5,
};

struct mtk_mac;
struct mtk_eth;

typedef int (*func_phy_init)(struct mtk_eth *eth, int mac_id);
typedef int (*func_phy_start)(struct mtk_eth *eth, int mac_id);
typedef int (*func_phy_stop)(struct mtk_eth *eth, int mac_id);
typedef int (*func_phy_ack_interrupt)(struct mtk_eth *eth, int mac_id);
typedef int (*func_phy_pop_lp_pause)(struct mtk_eth *eth, int mac_id, u32 *lp_pause);
typedef int (*func_phy_pop_adv_cap)(struct mtk_eth *eth, int mac_id, u32 *adv_cap);
typedef int (*func_phy_pop_phy_link)(struct mtk_eth *eth, int mac_id, u32 *link);
typedef int (*func_phy_pop_phy_speed)(struct mtk_eth *eth, int mac_id, u32 *speed_bits, u32 *duplex_bit);
typedef int (*func_phy_pop_link_change)(struct mtk_eth *eth, int mac_id);
typedef int (*func_phy_check_phy_state)(struct mtk_eth *eth, int mac_id, int type);
typedef int (*func_phy_load_phy_fw)(struct mtk_eth *eth, int mac_id, int type);

struct phy_cb_func {
    func_phy_init               init_cb;
    func_phy_start              start_cb;
    func_phy_stop               stop_cb;
    func_phy_init               post_init_cb;
    func_phy_ack_interrupt      phy_ack_interrupt_cb;
    func_phy_pop_lp_pause       pop_lp_pause_cb;
    func_phy_pop_adv_cap        pop_adv_cap_cb;
    func_phy_pop_phy_link       pop_phy_link_cb;
    func_phy_pop_phy_speed      pop_phy_speed_cb;
    func_phy_pop_link_change    pop_link_change_cb;
    func_phy_check_phy_state    check_phy_state_cb;
    func_phy_load_phy_fw        load_phy_fw_cb;
};

struct ql_sgmii_phy {
    int phy_addr;
    int link;
    int sgmii_phy_type;
    int reset_pin;
    int interrupt_pin;
    int phy_power_pin;
    void *phy_private_data; //use by phy
    struct phy_cb_func cb_func;
    struct device class_dev;	/* sysfs device */
};

struct _mtk_eth {
    struct mtk_eth *eth;
    int phy_addr;
};

void init_callback_func(struct mtk_mac *mac, int type);
int __ql_phy_init(struct mtk_eth *eth, int mac_id, int type);
int __ql_phy_start(struct mtk_eth *eth, int mac_id, int type);
int __ql_phy_stop(struct mtk_eth *eth, int mac_id, int type);
int __ql_phy_post_init(struct mtk_eth *eth, int mac_id, int type);
int ql_phy_ack_interrupt(struct mtk_eth *eth, int mac_id, int type);
int ql_phy_pop_lp_pause(struct mtk_eth *eth, int mac_id, int type, u32 *lp_pause);
int ql_phy_pop_adv_cap(struct mtk_eth *eth, int mac_id, int type, u32 *adv_cap);
int ql_phy_pop_phy_link(struct mtk_eth *eth, int mac_id, int type, u32 *link);
int ql_phy_pop_link_change(struct mtk_eth *eth, int mac_id);
int ql_phy_pop_phy_speed(struct mtk_eth *eth, int mac_id, int type, u32 *speed_bits, u32 *duplex_bit);
int ql_check_phy_state(struct mtk_eth *eth, int mac_id, int type);
int ql_phy_load_firmware(struct mtk_eth *eth, int mac_id, int type);

void ql_phy_power_on(struct device *dev, int power_gpio, int mac_id, int is_on);
void ql_phy_power_on_free(struct device *dev, int power_gpio, int mac_id);
void __ql_phy_power_on(struct device *dev, int reset_gpio, int mac_id, int is_on);
void __ql_phy_power_on_free(struct device *dev, int reset_gpio, int mac_id);
void ql_phy_reset(struct device *dev, int power_gpio, int mac_id);
void ql_phy_reset_free(struct device *dev, int reset_gpio, int mac_id);

#endif /* QL_ETH_PHY_H */