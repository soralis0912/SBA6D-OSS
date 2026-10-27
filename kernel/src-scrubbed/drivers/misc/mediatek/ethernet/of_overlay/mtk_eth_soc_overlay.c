#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/of_fdt.h>
#include "mtk_eth_soc.h"
#ifdef CONFIG_AIR_EN8801H_PHY
#include "ql_phy_rw_en8801.h"

int ql_airoha_pbus_write(struct mtk_eth *eth, int pbus_id,
			unsigned long pbus_address, unsigned long pbus_data);
unsigned long ql_airoha_pbus_read(struct mtk_eth *eth, int pbus_id,
			unsigned long pbus_address);
void en8801_rewrite_soft_phy_addr(struct mtk_eth *eth, unsigned int en8801s_mdio_phy_id);

#endif

#ifdef CONFIG_MOTORCOMM_PHY
#include "ql_phy_rw_yt8821.h"
#endif

#define NET_TAG "mtk_eth_soc_overlay"

#define QLOGD(f,...)        do{\
            printk("[%s]%s():%d " f " ",NET_TAG,__FUNCTION__,__LINE__,## __VA_ARGS__);}while(0);


static const struct of_device_id eth_overlay_id_table[] = {
	{ .compatible = "quectel,eth-phy-adapt",},
	{ },
};
MODULE_DEVICE_TABLE(of, eth_overlay_id_table);

u32 mtk_mdio_read_ext(struct mtk_eth *eth, int phy_addr, int phy_reg);
u32 mtk_mdio_write_ext(struct mtk_eth *eth, u32 phy_addr,
			   u32 phy_register, u32 write_data);
u32 _mtk_mdio_read_C45(struct mtk_eth *eth, u32 phy_addr, u32 phy_register, u32 devad);
u32 _mtk_mdio_write_C45(struct mtk_eth *eth, u32 phy_addr, u32 phy_register, u32 write_data, u32 devad);

void ql_enable_sgmii_pwr(int id, struct mac_phy_of_adapt_data *mac_data);

struct mac_phy_of_adapt *mac_phy_adapt;
EXPORT_SYMBOL_GPL(mac_phy_adapt);
struct mac_phy_of_overlay {
	const char *compatible;
	void *begin;
	void *end;
};

struct platform_device *g_pdev;

#define QL_OVERLAY_OF_DTB(mac_no, phy_type)					\
	extern char __dtb_ql_overlay_of_##mac_no##_##phy_type##_begin[];		\
	extern char __dtb_ql_overlay_of_##mac_no##_##phy_type##_end[]

#define QL_OVERLAY_OF_OVERLAY(mac_no, phy_type, phyname)					\
{								\
	.compatible = "phyname," #phyname,	\
	.begin = __dtb_ql_overlay_of_##mac_no##_##phy_type##_begin,	\
	.end = __dtb_ql_overlay_of_##mac_no##_##phy_type##_end,		\
}

/* -----------------------------------------------------------------------------
 * Mac Phy Dts Overlays
 */

QL_OVERLAY_OF_DTB(mac0, phy_2p5g);
QL_OVERLAY_OF_DTB(mac0, phy_10g);
QL_OVERLAY_OF_DTB(mac1, phy_2p5g);
QL_OVERLAY_OF_DTB(mac1, phy_10g);
QL_OVERLAY_OF_DTB(mac0, phy_en8801_1g);
QL_OVERLAY_OF_DTB(mac1, phy_en8801_1g);
QL_OVERLAY_OF_DTB(mac0, phy_en8811_2p5g);
QL_OVERLAY_OF_DTB(mac1, phy_en8811_2p5g);
QL_OVERLAY_OF_DTB(mac0, phy_yt8821_2p5g);
QL_OVERLAY_OF_DTB(mac1, phy_yt8821_2p5g);

static const struct mac_phy_of_overlay ql_overlay_of_overlays[] __initconst = {
	QL_OVERLAY_OF_OVERLAY(mac0, phy_2p5g, mac0_rtl8221),
	QL_OVERLAY_OF_OVERLAY(mac0, phy_10g, mac0_aqr113c),
	QL_OVERLAY_OF_OVERLAY(mac0, phy_10g, mac0_rtl8251),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_2p5g, mac1_mt7531),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_2p5g, mac1_rtl8221),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_10g, mac1_aqr113c),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_10g, mac1_rtl8251),
	QL_OVERLAY_OF_OVERLAY(mac0, phy_en8801_1g, mac0_en8801),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_en8801_1g, mac1_en8801),
	QL_OVERLAY_OF_OVERLAY(mac0, phy_en8811_2p5g, mac0_en8811),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_en8811_2p5g, mac1_en8811),
	QL_OVERLAY_OF_OVERLAY(mac0, phy_yt8821_2p5g, mac0_yt8821),
	QL_OVERLAY_OF_OVERLAY(mac1, phy_yt8821_2p5g, mac1_yt8821),

	{ /* Sentinel */ },
};


static int __init mac_phy_of_apply_overlay(const struct mac_phy_of_overlay *dtbs,
					   const char *phyname)
{
	const struct mac_phy_of_overlay *dtb = NULL;
	unsigned int i;
	int ovcs_id, ret = 0;

	for (i = 0; dtbs[i].compatible; ++i) {
		// QLOGD("%s %s\n", dtbs[i].compatible, phyname);
		if (strstr(dtbs[i].compatible, phyname)) {
			dtb = &dtbs[i];
			QLOGD("%s %s ", dtbs[i].compatible, phyname);
			QLOGD("Get and break\n");
			break;
		}
	}
	if (!dtb)
		return -ENODEV;

	//Frist update dtb overlay
	ovcs_id = 0;
	ret =  of_overlay_fdt_apply(dtb->begin, dtb->end - dtb->begin, &ovcs_id);
	QLOGD("Warning goto overlay %s dtb %s\n", dtbs[i].compatible, ret==0?"success":"failed");
	mdelay(1);

	return ret;
}

static int _mtk_mdio_and_pop_phy_id(struct mtk_eth *eth, int mac_id, long *phy_id)
{
	int addr = 0;
	u16 phy_cr1;
	unsigned long pbus_data;
	unsigned int retry;

	//1st close mdio broadcast respond
	QLOGD("close mdio broadcast respond \n");
	for(addr = 0; addr<=0x1f; addr++) {
		//Rtl8221 || Rtl8251
		phy_cr1 = _mtk_mdio_read_C45(eth, addr, 0xA430, MDIO_MMD_VEND2);
		ClrBit(phy_cr1, 13);
		_mtk_mdio_write_C45(eth, addr, 0xA430, phy_cr1, MDIO_MMD_VEND2);
		phy_cr1 = 0;
		//Aqr113c, default is disable
		// phy_cr1 = _mtk_mdio_read_C45(eth, addr, 0xC441, MDIO_MMD_VEND1);
		// ClrBit(phy_cr1, 14);
		// _mtk_mdio_write_C45(eth, addr, 0xC441, phy_cr1, MDIO_MMD_VEND1);

		//Yt8221
		phy_cr1 = ytphy_read_ext(eth, addr, 0xa005);
		phy_cr1 &= ~(BIT(5) | BIT(6));
		ytphy_write_ext(eth, addr, 0xa005, phy_cr1);
	}
	//c22
	for(addr = 0; addr<=0x1f; addr++) {
		*phy_id = 0x0;
		*phy_id = ((mtk_mdio_read_ext(eth, addr, MII_PHYSID2) & 0xffff) | ((mtk_mdio_read_ext(eth, addr, MII_PHYSID1) & 0xffff) << 16));
		if(*phy_id != 0x0) {
			//skip rtl8221 mdio broadcast address
			if(addr == 0x0 && (*phy_id == RTL8221_1_PHY_ID || *phy_id == RTL8221_2_PHY_ID || *phy_id == YT8821_PHY_ID)) {
				continue;
			}
			else {
				QLOGD("c22 addr:0x%x phy_id:0x%x\n", addr, *phy_id);
				break;
			}
		}
		mdelay(1);
	}
	//c45
	if(*phy_id == 0x0) {
		for(addr = 0; addr<=0x1f; addr++) {
			*phy_id = 0x0;
			*phy_id = ((_mtk_mdio_read_C45(eth, addr, MII_PHYSID2, 0x1e) & 0xffff) | ((_mtk_mdio_read_C45(eth, addr, MII_PHYSID1, 0x1e) & 0xffff) << 16));
			if(*phy_id != 0x0) {
				QLOGD("c45 addr:0x%x phy_id:0x%x\n", addr, *phy_id);
				break;
			}
		}
		mdelay(1);
	}

	/*
	Airoha en8801
	en8801s_mdio_phy_id:is new mdio addr.
	en8801s_pbus_phy_id:is en8801s_mdio_phy_id + 1
	*/
	if(*phy_id == EN8801S_PBUS_MDIO_ID) {
		/* EN8801 probe */
		for(addr = 0; addr <= 0x1f; addr ++)
		{
			for(retry = 0; retry < 2; retry ++)
			{
				pbus_data = ql_airoha_pbus_read(eth, EN8801S_PBUS_DEFAULT_ID, EN8801S_RG_ETHER_PHY_OUI); /* PHY OUI */
				if(EN8801S_PBUS_OUI == pbus_data)
				{
					*phy_id = EN8801S_PHY_ID;
					break;
				}
			}
			if(*phy_id == EN8801S_PHY_ID)
			{
				break;
			}
		}
		if(*phy_id == EN8801S_PHY_ID)
		{
			addr = 0;
		}
	}

	return addr;
}

u32 mac_phy_of_addr_overlay(char *name)
{
	u32 i, addr = 0;
	for(i = 0; i < MTK_MAX_DEVS; i++)
	{
		QLOGD("%s %s", mac_phy_adapt->mac_data[i].compatible, name);
		if (strstr(mac_phy_adapt->mac_data[i].compatible, name)) {
			addr = mac_phy_adapt->mac_data[i].sgmii_phy_addr;
			QLOGD("mac:%d find PHY address %d", i, addr);
			break;
		}
	}
	return addr;
}

static void rewrite_soft_phy_addr(struct mtk_eth *eth)
{
	unsigned long pbus_data;
	unsigned int en8801s_pbus_phy_id = 0, en8801s_mdio_phy_id = 0;
	int mac_id = 0;
	for(mac_id = 0; mac_id < MTK_MAX_DEVS; mac_id++)
	{

		if(eth->mac_data[mac_id].sgmii_phy_id == EN8801_PHY_ID)
		{
			if(mac_id == 0) {
				//1st power off other mac_id
				__ql_phy_power_on(&g_pdev->dev, mac_id==1? CONFIG_SGMII0_PHY_RESET_PIN:CONFIG_SGMII1_PHY_RESET_PIN, mac_id==1?0:1, 0);
				mdelay(100);
				//2nd power on suitable mac_id
				__ql_phy_power_on(&g_pdev->dev, mac_id==0? CONFIG_SGMII0_PHY_RESET_PIN:CONFIG_SGMII1_PHY_RESET_PIN, mac_id, 1);
				mdelay(300);
			}
			else if(mac_id == 1) {
				//2nd power on suitable mac_id
				__ql_phy_power_on(&g_pdev->dev, mac_id==0? CONFIG_SGMII0_PHY_RESET_PIN:CONFIG_SGMII1_PHY_RESET_PIN, mac_id, 1);
				mdelay(300);
			}

			///cross phy-type
			if(eth->mac_data[mac_id].sgmii_phy_id == EN8801_PHY_ID && \
				eth->mac_data[mac_id==0?1:0].sgmii_phy_id != EN8801_PHY_ID)
			{
				en8801s_mdio_phy_id = (eth->mac_data[mac_id==0?1:0].sgmii_phy_addr + 1);
				if(en8801s_mdio_phy_id >=31)
				{
					en8801s_mdio_phy_id = 1;
				}
			}///equal phy-type
			else if(eth->mac_data[mac_id].sgmii_phy_id == EN8801_PHY_ID && \
				eth->mac_data[mac_id==0?1:0].sgmii_phy_id == EN8801_PHY_ID)
			{
				en8801s_mdio_phy_id = mac_id*MTK_MAX_DEVS + 6;
			}
			QLOGD("set mac:%d en8801 new phy-addr:%d", mac_id, en8801s_mdio_phy_id);
			eth->mac_data[mac_id].sgmii_phy_addr = en8801s_mdio_phy_id;
			mac_phy_adapt->mac_data[mac_id].sgmii_phy_addr = eth->mac_data[mac_id].sgmii_phy_addr;
			// en8801_rewrite_soft_phy_addr(eth, en8801s_mdio_phy_id);
		}
	}
}

static int eth_overlay_probe(struct platform_device *pdev)
{
	int mac_id = 0;
    struct mtk_eth *eth;
	mac_phy_adapt = devm_kzalloc(&pdev->dev, sizeof(struct mac_phy_of_adapt), GFP_KERNEL);
    dev_err(&pdev->dev, "%s in\n", __func__);
    struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    eth = devm_kzalloc(&pdev->dev, sizeof(*eth), GFP_KERNEL);
    eth->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(eth->base)) {
        QLOGD("Error can't get reg!\n");
		return PTR_ERR(eth->base);
    }
	g_pdev = pdev;

	/*
	                      first pop
	  mac0<-->sgmii/usxgmii<-->phy link phy id(phy 0 id)
	  mac1<-->sgmii/usxgmii<-->phy link phy id(phy 1 id)
	*/
	long phy_id = 0x0;
	int addr = 0;
	ql_phy_power_on(&pdev->dev, CONFIG_SGMII0_PHY_POWER_PIN, 0, 1);

	for(mac_id = 0; mac_id < MTK_MAX_DEVS; mac_id++)
	{
		//1st power off other mac_id
		__ql_phy_power_on(&pdev->dev, mac_id==1? CONFIG_SGMII0_PHY_RESET_PIN:CONFIG_SGMII1_PHY_RESET_PIN, mac_id==1?0:1, 0);
		mdelay(100);
		//2nd power on suitable mac_id
		__ql_phy_power_on(&pdev->dev, mac_id==0? CONFIG_SGMII0_PHY_RESET_PIN:CONFIG_SGMII1_PHY_RESET_PIN, mac_id, 1);
		mdelay(300);

		QLOGD("goto scan mac:%d\n", mac_id);
		addr = _mtk_mdio_and_pop_phy_id(eth, mac_id, &phy_id);
		if(phy_id == AQR113C_1_PHY_ID || phy_id == AQR113C_2_PHY_ID)
		{
			QLOGD("phy is aqr113c mac_id:%d addr:%d\n", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_USXGMII_TYP;
			eth->mac_data[mac_id].sgmii_phy_addr = addr;
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}
		else if(phy_id == RTL8251_PHY_ID)
		{
			QLOGD("phy is rtl8251 mac_id:%d addr:%d\n", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_USXGMII_TYP;
			eth->mac_data[mac_id].sgmii_phy_addr = addr;
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}
		else if(phy_id == RTL8221_1_PHY_ID || phy_id == RTL8221_2_PHY_ID)
		{
			QLOGD("phy is rtl8221 mac_id:%d addr:%d\n", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_SGMII_TYP;
			eth->mac_data[mac_id].sgmii_phy_addr = addr;
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}
		else if(phy_id == MT7531_PHY_ID)
		{
			if(mac_id== 0)
			{
				QLOGD("Error mt7531 can't in sgmii0 & maybe have some unexpected problem!\n");
				WARN_ON(1);
			}
			QLOGD("phy is mt7531 mac_id:%d addr:%d\n", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_SGMII_TYP;
			eth->mac_data[mac_id].sgmii_phy_addr = addr;
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}
		else if(phy_id == EN8801_PHY_ID)
		{
			QLOGD("phy is %s mac_id:%d addr:%d\n", "EN8801", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_SGMII_TYP;
			// eth->mac_data[mac_id].sgmii_phy_addr = addr;	//will rewrite in rewrite_soft_phy_addr
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}
		else if(phy_id == EN8811_PHY_ID)
		{
			QLOGD("phy is %s mac_id:%d addr:%d\n", "EN8811", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_SGMII_TYP;
			eth->mac_data[mac_id].sgmii_phy_addr = addr;
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}
		else if(phy_id == YT8821_PHY_ID)
		{
			QLOGD("phy is %s mac_id:%d addr:%d\n", "YT8821", mac_id, addr);
			eth->mac_data[mac_id].sgmii_phy_id = phy_id;
			eth->mac_data[mac_id].sgmii_type = QL_SGMII_TYP;
			eth->mac_data[mac_id].sgmii_phy_addr = addr;
			ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
		}

		//3rd fixup dts by sgmii_type
		/* Parse and apply the overlay. This will resolve phandles. */
		if(eth->mac_data[mac_id].sgmii_type == QL_SGMII_TYP)
		{
			if(eth->mac_data[mac_id].sgmii_phy_id == RTL8221_1_PHY_ID || eth->mac_data[mac_id].sgmii_phy_id == RTL8221_2_PHY_ID)
			{
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_rtl8221":"mac1_rtl8221");
			}
			else if(eth->mac_data[mac_id].sgmii_phy_id == MT7531_PHY_ID)
			{
				//mt7531 only support in mac_id:1(sgmii1)
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_mt7531":"mac1_mt7531");
			}
			else if(eth->mac_data[mac_id].sgmii_phy_id == EN8801_PHY_ID)
			{
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_en8801":"mac1_en8801");
				//compatible should sync ql_overlay_of_mac0_phy*.dts mdio-bus node
				snprintf(eth->mac_data[mac_id].compatible, sizeof(eth->mac_data[mac_id].compatible), mac_id==0? "mac0_eth_1g" : "mac1_eth_1g");
			}
			else if(eth->mac_data[mac_id].sgmii_phy_id == EN8811_PHY_ID)
			{
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_en8811":"mac1_en8811");
				//compatible should sync ql_overlay_of_mac0_phy*.dts mdio-bus node
				snprintf(eth->mac_data[mac_id].compatible, sizeof(eth->mac_data[mac_id].compatible), mac_id==0? "mac0_eth_2p5g" : "mac1_eth_2p5g");
			}
			else if(eth->mac_data[mac_id].sgmii_phy_id == YT8821_PHY_ID)
			{
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_yt8821":"mac1_yt8821");
				//compatible should sync ql_overlay_of_mac0_phy*.dts mdio-bus node
				snprintf(eth->mac_data[mac_id].compatible, sizeof(eth->mac_data[mac_id].compatible), mac_id==0? "mac0_eth_2p5g" : "mac1_eth_2p5g");
			}
		}else if(eth->mac_data[mac_id].sgmii_type == QL_USXGMII_TYP)
		{
			if(eth->mac_data[mac_id].sgmii_phy_id == AQR113C_1_PHY_ID || eth->mac_data[mac_id].sgmii_phy_id == AQR113C_2_PHY_ID)
			{
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_aqr113c":"mac1_aqr113c");
				//compatible should sync ql_overlay_of_mac0_phy*.dts mdio-bus node
				snprintf(eth->mac_data[mac_id].compatible, sizeof(eth->mac_data[mac_id].compatible), mac_id==0? "mac0_eth_10g" : "mac1_eth_10g");
			}
			else if(eth->mac_data[mac_id].sgmii_phy_id == RTL8251_PHY_ID)
			{
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, mac_id==0?"mac0_rtl8251":"mac1_rtl8251");
				//compatible should sync ql_overlay_of_mac0_phy*.dts mdio-bus node
				snprintf(eth->mac_data[mac_id].compatible, sizeof(eth->mac_data[mac_id].compatible), mac_id==0? "mac0_eth_10g" : "mac1_eth_10g");
			}
		}
		else {
			QLOGD("Goto set default setting mac:%d\n", mac_id);
			eth->mac_data[mac_id].sgmii_type = QL_SGMII_TYP;
			eth->mac_data[mac_id].is_def_cfg = 1;
			if(mac_id == 0) {
				eth->mac_data[0].sgmii_phy_id = RTL8221_1_PHY_ID;
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, "mac0_rtl8221");
				ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
			}
			else {
				eth->mac_data[1].sgmii_phy_id = MT7531_PHY_ID;
				mac_phy_of_apply_overlay(ql_overlay_of_overlays, "mac1_mt7531");
				ql_enable_sgmii_pwr(mac_id, &eth->mac_data[mac_id]);
			}
		}

		//4th update mac_phy_adapt date
		mac_phy_adapt->base = eth->base;
		mac_phy_adapt->mac_data[mac_id].sgmii_type = eth->mac_data[mac_id].sgmii_type;
		mac_phy_adapt->mac_data[mac_id].is_def_cfg = eth->mac_data[mac_id].is_def_cfg;
		mac_phy_adapt->mac_data[mac_id].sgmii_phy_addr = eth->mac_data[mac_id].sgmii_phy_addr;
		mac_phy_adapt->mac_data[mac_id].sgmii_phy_id = eth->mac_data[mac_id].sgmii_phy_id;

		memcpy(mac_phy_adapt->mac_data[mac_id].compatible, eth->mac_data[mac_id].compatible, \
								sizeof(eth->mac_data[mac_id].compatible));
	}

	//5th rewrite software programm phy-addr
	if(mac_phy_adapt->mac_data[0].sgmii_phy_id == EN8801_PHY_ID || mac_phy_adapt->mac_data[1].sgmii_phy_id == EN8801_PHY_ID)
	{
		__ql_phy_power_on(&pdev->dev, CONFIG_SGMII0_PHY_RESET_PIN, 0, 0);
		__ql_phy_power_on(&pdev->dev, CONFIG_SGMII1_PHY_RESET_PIN, 1, 0);
		mdelay(100);
		rewrite_soft_phy_addr(eth);
	}

	//6th power on all mac_id
	__ql_phy_power_on(&pdev->dev, CONFIG_SGMII0_PHY_RESET_PIN, 0, 1);
	__ql_phy_power_on(&pdev->dev, CONFIG_SGMII1_PHY_RESET_PIN, 1, 1);
	mdelay(100);
	ql_phy_power_on_free(&pdev->dev, CONFIG_SGMII0_PHY_POWER_PIN, 0);
	ql_phy_power_on_free(&pdev->dev, CONFIG_SGMII1_PHY_POWER_PIN, 1);
	__ql_phy_power_on_free(&pdev->dev, CONFIG_SGMII0_PHY_RESET_PIN, 0);
	__ql_phy_power_on_free(&pdev->dev, CONFIG_SGMII1_PHY_RESET_PIN, 1);

	if(eth->mac_data[0].sgmii_phy_addr == eth->mac_data[1].sgmii_phy_addr)
	{
		QLOGD("Error with the same phy-addr & maybe have some unexpected problem!");
		// WARN_ON(1);
	}
	if(eth->mac_data[0].sgmii_phy_addr == 0 || eth->mac_data[1].sgmii_phy_addr == 0)
	{
		QLOGD("Error with the zero phy-addr & maybe have some unexpected problem!");
		// WARN_ON(1);
	}
    return 0;
}

static struct platform_driver mtk_eth_soc_overlay = {
	.probe		= eth_overlay_probe,
	.driver		= {
		.name	= "mtk_eth_soc_overlay",
		.owner	= THIS_MODULE,
		.of_match_table = eth_overlay_id_table,
	},
};

builtin_platform_driver(mtk_eth_soc_overlay);