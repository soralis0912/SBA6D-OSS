#include "ql_phy_rw_yt8821.h"
#include "mtk_eth_soc.h"

u32 mtk_mdio_read_ext(struct mtk_eth *eth, int phy_addr, int phy_reg);
u32 mtk_mdio_write_ext(struct mtk_eth *eth, u32 phy_addr,
		    u32 phy_register, u32 write_data);

int ytphy_read_ext(struct mtk_eth *eth, int phy_addr, u32 regnum)
{
	int ret;

    ret = mtk_mdio_write_ext(eth, phy_addr, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		goto err_handle;

	ret = mtk_mdio_read_ext(eth, phy_addr, REG_DEBUG_DATA);
	if (ret < 0)
		goto err_handle;

err_handle:

	return ret;
}

int ytphy_write_ext(struct mtk_eth *eth, int phy_addr, u32 regnum, u16 val)
{
	int ret;

	ret = mtk_mdio_write_ext(eth, phy_addr, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		goto err_handle;

	ret = mtk_mdio_write_ext(eth, phy_addr, REG_DEBUG_DATA, val);
	if (ret < 0)
		goto err_handle;

err_handle:

	return ret;
}
