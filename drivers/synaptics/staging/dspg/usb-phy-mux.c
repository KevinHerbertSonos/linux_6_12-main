/*
 * linux/drivers/staging/dspg/usb-phy-mux.c
 *
 *  Copyright (C) 2016 DSP Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/sysfs.h>

#define USB_CSR                 0x58
#define USB_PHY1_CTRL1          0x80
#define USB_PHY2_CTRL1          0x88

struct usb_phy_mux {
	struct device *dev;
	struct regmap_field *f;
	struct regmap_field *phy1_ctrl;
	struct regmap_field *phy2_ctrl;
	unsigned int route_to_host;
};

static const struct reg_field usb_phy_mux_regfield =
	REG_FIELD(USB_CSR, 0, 0);
static const struct reg_field usb_phy1_ctrl_regfield =
	REG_FIELD(USB_PHY1_CTRL1, 8, 9);
static const struct reg_field usb_phy2_ctrl_regfield =
	REG_FIELD(USB_PHY2_CTRL1, 8, 9);

static int set_route(struct usb_phy_mux *pdata)
{
	int ret;

	ret = regmap_field_write(pdata->f, pdata->route_to_host);
	if (ret) {
		dev_err(pdata->dev, "failed to set USB PHY muxer");
		return ret;
	}

	/* Enable D-/D+ pull-down resistors in host mode.
	 * Always for PHY1, depending on "route_to_host" for PHY2.
	 */
	ret = regmap_field_write(pdata->phy1_ctrl, 3);
	if (ret)
		dev_err(pdata->dev,
			"failed to set USB control register");

	if (pdata->route_to_host)
		ret = regmap_field_write(pdata->phy2_ctrl, 3);
	else
		ret = regmap_field_write(pdata->phy2_ctrl, 0);
	if (ret)
		dev_err(pdata->dev,
			"failed to set USB control register");

	return ret;
}

static const char *xlate_mux_setting[] = {
	"device", "host",
};

static ssize_t mux_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct usb_phy_mux *pdata = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", xlate_mux_setting[pdata->route_to_host]);
}

static ssize_t mux_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t size)
{
	struct usb_phy_mux *pdata = dev_get_drvdata(dev);
	int old = pdata->route_to_host;
	int ret = 0;

	if (size > 6 && !strncmp(buf, "device", 6))
		pdata->route_to_host = 0;
	else if (size > 4 && !strncmp(buf, "host", 4))
		pdata->route_to_host = 1;
	else
		return -EFAULT;

	if (old != pdata->route_to_host)
		ret = set_route(pdata);

	if (!ret)
		ret = size;

	return ret;
}

static DEVICE_ATTR_RW(mux);

static const struct attribute *dvf101_usb_phy_mux_sysfs[] = {
	&dev_attr_mux.attr,
	NULL,
};

static int usb_phy_mux_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct regmap *map;
	struct usb_phy_mux *pdata;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	pdata->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, pdata);

	map = syscon_regmap_lookup_by_phandle(np, "dvf,syscfg");
	if (IS_ERR(map))
		return PTR_ERR(map);

	pdata->f = devm_regmap_field_alloc(&pdev->dev, map,
					   usb_phy_mux_regfield);
	if (IS_ERR(pdata->f))
		return PTR_ERR(pdata->f);

	pdata->phy1_ctrl = devm_regmap_field_alloc(&pdev->dev, map,
						   usb_phy1_ctrl_regfield);
	if (IS_ERR(pdata->phy1_ctrl))
		return PTR_ERR(pdata->phy1_ctrl);

	pdata->phy2_ctrl = devm_regmap_field_alloc(&pdev->dev, map,
						   usb_phy2_ctrl_regfield);
	if (IS_ERR(pdata->phy2_ctrl))
		return PTR_ERR(pdata->phy2_ctrl);

	pdata->route_to_host = !!of_property_read_bool(np, "port2-to-host");

	ret = set_route(pdata);
	if (ret)
		return ret;

	ret = sysfs_create_files(&pdev->dev.kobj, dvf101_usb_phy_mux_sysfs);
	if (ret)
		return ret;

	dev_info(&pdev->dev, "PHY2 routed to %s",
		 xlate_mux_setting[pdata->route_to_host]);
	dev_info(&pdev->dev, "successfully registered USB PHY muxer");

	return 0;
}

static struct of_device_id usb_phy_mux_of_device_ids[] = {
	{ .compatible = "dspg,dvf101-usb-phy-mux" },
	{ },
};

static struct platform_driver usb_phy_mux_driver = {
	.probe = usb_phy_mux_probe,
	.driver = {
		.name = "usb-phy-mux",
		.owner = THIS_MODULE,
		.of_match_table = usb_phy_mux_of_device_ids,
	},
};

static int __init usb_phy_mux_init(void)
{
	return platform_driver_register(&usb_phy_mux_driver);
}
arch_initcall(usb_phy_mux_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("USB PHY muxer");
MODULE_AUTHOR("DSP Group, Inc.");
