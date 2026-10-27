/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2019 MediaTek Inc.
 *  Author: FangRu Wang <fangju.wang@mediatek.com>
 */

#ifndef MTK_COMMON_H
#define MTK_COMMON_H

enum mtk_interface_state {
	MTK_IF_UNKNOWN,
	MTK_IF_UP,
	MTK_IF_DOWN,
};

int pinctrl_init(struct mtk_eth *eth);

u32 mtk_mdio_read_ext(struct mtk_eth *eth, int phy_addr, int phy_reg);
u32 mtk_mdio_write_ext(struct mtk_eth *eth, u32 phy_addr,
		    u32 phy_register, u32 write_data);

u32 mtk_cl45_ind_read(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 *data);
u32 mtk_cl45_ind_write(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 data);

int mtk_mdio_busy_wait_ext(struct mtk_eth *eth);


u32 _mtk_mdio_write_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data, u32 devad);

int netsys_pm_resume(struct device *device);
int netsys_pm_suspend(struct device *device);
void mtk_rx_irq_disable_ext(struct mtk_eth *eth, u32 mask);

int netsys_pm_resume_poweroff(struct device *device);
int netsys_pm_suspend_poweroff(struct device *device);


void mtk_tx_irq_disable_ext(struct mtk_eth *eth, u32 mask);
void mtk_clk_disable_ext(struct mtk_eth *eth);
int mtk_clk_enable_ext(struct mtk_eth *eth);

void mtk_tx_irq_enable_ext(struct mtk_eth *eth, u32 mask);

void mtk_rx_irq_enable_ext(struct mtk_eth *eth, u32 mask);
u32 _mtk_mdio_read_C45(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 devad);


void mtk_eth_auto_qos_init(struct mtk_eth *eth);

int mtk_eth_sgmii_runtime_config(struct mtk_eth *eth, int id, int en);
int mtk_eth_sgmii_runtime_set(struct mtk_eth *eth, u32 id, int en, int if_status);
int mtk_eth_sgmii_runtime_status_action(struct mtk_eth *eth, u32 id, int if_status);
int mtk_eth_sgmii_set_force_mode(struct mtk_eth *eth, u32 id, int en);
irqreturn_t mtk_handle_irq_ext(int irq, void *_eth);
void mtk_mac_status_polling_aqr(struct mtk_eth *eth, int id, int type);


int mtk_phy_event_init(struct mtk_eth *eth, int type);
irqreturn_t mtk_handle_irq_ext1(int irq, void *_eth);
void mtk_mt7531_polling(struct mtk_eth *eth, int id, int if_status);
void mtk_flow_ctrl_aqr(struct mtk_eth *eth, int type);


#endif /* MTK_COMMON_H */
