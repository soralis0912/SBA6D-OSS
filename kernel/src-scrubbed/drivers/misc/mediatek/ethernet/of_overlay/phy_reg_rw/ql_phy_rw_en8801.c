#include "ql_phy_rw_en8801.h"
#include "mtk_eth_soc.h"

#define NET_TAG "ql_phy_rw_en8801"

u32 mtk_mdio_read_ext(struct mtk_eth *eth, int phy_addr, int phy_reg);
u32 mtk_mdio_write_ext(struct mtk_eth *eth, u32 phy_addr,
		    u32 phy_register, u32 write_data);

static int __airoha_pbus_write(struct mtk_eth *eth, int pbus_id,
			unsigned long pbus_address, unsigned long pbus_data)
{
	int ret = 0;

	ret = mtk_mdio_write_ext(eth, pbus_id, 0x1F,
				(unsigned int)(pbus_address >> 6));
	if (ret < 0)
		return ret;
	ret = mtk_mdio_write_ext(eth, pbus_id,
				(unsigned int)((pbus_address >> 2) & 0xf),
				(unsigned int)(pbus_data & 0xFFFF));
	if (ret < 0)
		return ret;
	ret = mtk_mdio_write_ext(eth, pbus_id, 0x10,
				(unsigned int)(pbus_data >> 16));
	if (ret < 0)
		return ret;
	return ret;
}

static unsigned long __airoha_pbus_read(struct mtk_eth *eth, int pbus_id,
			unsigned long pbus_address)
{
	unsigned long pbus_data;
	unsigned int pbus_data_low, pbus_data_high;
	int ret = 0;

	ret = mtk_mdio_write_ext(eth, pbus_id, 0x1F,
				(unsigned int)(pbus_address >> 6));
	if (ret < 0) {
		pr_err("%s fail. (ret=%d)\n", __func__, ret);
		return INVALID_DATA;
	}
	pbus_data_low = mtk_mdio_read_ext(eth, pbus_id,
				(unsigned int)((pbus_address >> 2) & 0xf));
	pbus_data_high = mtk_mdio_read_ext(eth, pbus_id, 0x10);
	pbus_data = (pbus_data_high << 16) + pbus_data_low;
	return pbus_data;
}

int ql_airoha_pbus_write(struct mtk_eth *eth, int pbus_id,
			unsigned long pbus_address, unsigned long pbus_data)
{
	int ret = 0;
	ret = __airoha_pbus_write(eth, pbus_id, pbus_address, pbus_data);
	return ret;
}

unsigned long ql_airoha_pbus_read(struct mtk_eth *eth, int pbus_id,
			unsigned long pbus_address)
{
	unsigned long pbus_data;
	pbus_data = __airoha_pbus_read(eth, pbus_id, pbus_address);
	return pbus_data;
}

void en8801_rewrite_soft_phy_addr(struct mtk_eth *eth, unsigned int en8801s_mdio_phy_id)
{
	unsigned long pbus_data, _pbus_data;
	unsigned int en8801s_pbus_phy_id = 0;

	pbus_data = ql_airoha_pbus_read(eth, EN8801S_PBUS_DEFAULT_ID, EN8801S_RG_SMI_ADDR); /* SMI ADDR */
	QLOGD("[Airoha] pbus_data:0x%lx\n", pbus_data);
	if(en8801s_mdio_phy_id >=31)
		en8801s_mdio_phy_id--;
	EN8801_MDIO_MAP_PBUS(en8801s_mdio_phy_id, en8801s_pbus_phy_id);

	pbus_data = (pbus_data & 0xffff0000) | (unsigned long)(en8801s_pbus_phy_id << 8) | (unsigned long)(en8801s_mdio_phy_id);
	ql_airoha_pbus_write(eth, EN8801S_PBUS_DEFAULT_ID, EN8801S_RG_SMI_ADDR, pbus_data);
	ql_airoha_pbus_write(eth, EN8801S_PBUS_DEFAULT_ID, EN8801S_RG_BUCK_CTL, 0x03);
	mdelay(50);

	_pbus_data = ql_airoha_pbus_read(eth, en8801s_pbus_phy_id, EN8801S_RG_SMI_ADDR); /* SMI ADDR */
	QLOGD("[Airoha] EN8801S SMI ADDR=0x%lx (renew) %s\n", _pbus_data==pbus_data? "success":"failed");
}
