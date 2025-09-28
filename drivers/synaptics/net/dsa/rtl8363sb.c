// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014, 2019 DSP Group, 2022 Synaptics
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <net/dsa.h>

#define MAX_NR_PORTS 4

struct rtl8363sb_priv {
	struct mii_bus *bus;
	struct dsa_switch *ds;
	struct device *dev;
};

static int
rtl8363sb_dsa_setup(struct dsa_switch *ds)
{
	return 0;
}

static int
rtl8363sb_smi_read(struct mii_bus *bus, int phy_id, int regnum)
{
	int err = 0;

	err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 31, 0x000e);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 23, regnum);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 21, 0x0001);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_read_nested(bus, 0, 25);

	return err;
}

static int
rtl8363sb_smi_write(struct mii_bus *bus, int phy_id, int regnum, u16 val)
{
	int err = 0;

	err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 31, 0x000e);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 23, regnum);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 24, val);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 29, 0xffff);
	if (!err)
		err = mdiobus_write_nested(bus, 0, 21, 0x0003);

	return err;
}

static enum dsa_tag_protocol
rtl8363sb_get_tag_protocol(struct dsa_switch *ds, int port,
			   enum dsa_tag_protocol m)
{
	return DSA_TAG_PROTO_EDSA;
}

static int
rtl8363sb_dsa_read(struct dsa_switch *ds, int port, int regnum)
{
	struct rtl8363sb_priv *priv = ds->priv;
	int err = 0;

	if (port > MAX_NR_PORTS)
		return 0xffff;

	err = rtl8363sb_smi_write(priv->bus, 0, 0x2000 + (port << 5) + 31, 0);
	if (!err)
		err = rtl8363sb_smi_read(priv->bus, 0,
					 0x2000 + (port << 5) + regnum);

	return err;
}

static int
rtl8363sb_dsa_write(struct dsa_switch *ds, int port, int regnum, u16 val)
{
	struct rtl8363sb_priv *priv = ds->priv;
	int err = 0;

	if (port > MAX_NR_PORTS)
		return 0xffff;

	err = rtl8363sb_smi_write(priv->bus, 0, 0x2000 + (port << 5) + 31, 0);
	if (!err)
		err = rtl8363sb_smi_write(priv->bus, 0,
					  0x2000 + (port << 5) + regnum, val);

	return err;
}

static struct dsa_switch_ops rtl8363sb_switch_ops = {
	.get_tag_protocol = rtl8363sb_get_tag_protocol,
	.setup		= rtl8363sb_dsa_setup,
	.phy_read	= rtl8363sb_dsa_read,
	.phy_write	= rtl8363sb_dsa_write,
};

static int
rtl8363sb_probe(struct mdio_device *mdiodev)
{
	struct rtl8363sb_priv *priv;

	priv = devm_kzalloc(&mdiodev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->bus = mdiodev->bus;
	priv->dev = &mdiodev->dev;

	priv->ds = devm_kzalloc(&mdiodev->dev, sizeof(*priv->ds), GFP_KERNEL);
	if (!priv->ds)
		return -ENOMEM;

	priv->ds->dev = &mdiodev->dev;
	priv->ds->num_ports = MAX_NR_PORTS;
	priv->ds->priv = priv;
	priv->ds->ops = &rtl8363sb_switch_ops;

	dev_set_drvdata(&mdiodev->dev, priv);

	return dsa_register_switch(priv->ds);
}

static void
rtl8363sb_remove(struct mdio_device *mdiodev)
{
	struct rtl8363sb_priv *priv = dev_get_drvdata(&mdiodev->dev);

	dsa_unregister_switch(priv->ds);
}

static const struct of_device_id rtl8363sb_dt_ids[] = {
	{ .compatible = "rtl8363sb,dsa", },
	{ .compatible = "rtl8363nb,dsa", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rtl8363sb_dt_ids);

static struct mdio_driver rtl8363sb_driver = {
	.probe = rtl8363sb_probe,
	.remove = rtl8363sb_remove,
	.mdiodrv.driver = {
		.name = "RTL8363SB",
		.of_match_table = of_match_ptr(rtl8363sb_dt_ids),
	},
};

mdio_module_driver(rtl8363sb_driver);

MODULE_DESCRIPTION("Realtek 8363SB DSA driver");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
