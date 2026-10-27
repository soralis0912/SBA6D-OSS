/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * OF helpers for the MDIO (Ethernet PHY) API
 *
 * Copyright (c) 2009 Secret Lab Technologies, Ltd.
 */

#ifndef __LINUX_OF_MDIO_H
#define __LINUX_OF_MDIO_H

#include <linux/phy.h>
#include <linux/of.h>

#if IS_ENABLED(CONFIG_OF_MDIO)
extern int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np);
extern struct phy_device *of_phy_find_device(struct device_node *phy_np);
extern struct phy_device *of_phy_connect(struct net_device *dev,
					 struct device_node *phy_np,
					 void (*hndlr)(struct net_device *),
					 u32 flags, phy_interface_t iface);
extern struct phy_device *
of_phy_get_and_connect(struct net_device *dev, struct device_node *np,
		       void (*hndlr)(struct net_device *));
struct phy_device *of_phy_attach(struct net_device *dev,
				 struct device_node *phy_np, u32 flags,
				 phy_interface_t iface);

extern struct mii_bus *of_mdio_find_bus(struct device_node *mdio_np);
extern int of_phy_register_fixed_link(struct device_node *np);
extern void of_phy_deregister_fixed_link(struct device_node *np);
extern bool of_phy_is_fixed_link(struct device_node *np);

#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
u32 mac_phy_of_addr_overlay(char *name);
#endif

static inline int of_mdio_parse_addr(struct device *dev,
				     const struct device_node *np)
{
	u32 addr;
	int ret;

	ret = of_property_read_u32(np, "reg", &addr);
	if (ret < 0) {
		dev_err(dev, "%s %s has invalid PHY address\n", __func__, np->full_name);
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
		ret = 0;
		addr = mac_phy_of_addr_overlay(np->full_name);
		goto PHY_ADAPT;
#endif
		return ret;
	}
#if defined(CONFIG_QUECTEL_ETHERNET_PHY_ADAPT_SUPPORT)
PHY_ADAPT:
#endif
	/* A PHY must have a reg property in the range [0-31] */
	if (addr >= PHY_MAX_ADDR) {
		dev_err(dev, "%s PHY address %i is too large\n",
			np->full_name, addr);
		return -EINVAL;
	}

	return addr;
}

#else /* CONFIG_OF_MDIO */
static inline int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np)
{
	/*
	 * Fall back to the non-DT function to register a bus.
	 * This way, we don't have to keep compat bits around in drivers.
	 */

	return mdiobus_register(mdio);
}

static inline struct phy_device *of_phy_find_device(struct device_node *phy_np)
{
	return NULL;
}

static inline struct phy_device *of_phy_connect(struct net_device *dev,
						struct device_node *phy_np,
						void (*hndlr)(struct net_device *),
						u32 flags, phy_interface_t iface)
{
	return NULL;
}

static inline struct phy_device *
of_phy_get_and_connect(struct net_device *dev, struct device_node *np,
		       void (*hndlr)(struct net_device *))
{
	return NULL;
}

static inline struct phy_device *of_phy_attach(struct net_device *dev,
					       struct device_node *phy_np,
					       u32 flags, phy_interface_t iface)
{
	return NULL;
}

static inline struct mii_bus *of_mdio_find_bus(struct device_node *mdio_np)
{
	return NULL;
}

static inline int of_mdio_parse_addr(struct device *dev,
				     const struct device_node *np)
{
	return -ENOSYS;
}
static inline int of_phy_register_fixed_link(struct device_node *np)
{
	return -ENOSYS;
}
static inline void of_phy_deregister_fixed_link(struct device_node *np)
{
}
static inline bool of_phy_is_fixed_link(struct device_node *np)
{
	return false;
}
#endif


#endif /* __LINUX_OF_MDIO_H */
