// SPDX-License-Identifier: GPL-2.0
/*
 *  Synaptics BERLIN CHIP ID support
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright (C) 2018 Synaptics Incorporated
 *  Copyright (c) 2014 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/version.h>

static const struct of_device_id berlin_chipid_of_match[] = {
	{ .compatible = "marvell,berlin-chipid", },
	{},
};
MODULE_DEVICE_TABLE(of, berlin_chipid_of_match);

static const char *berlin_id_to_family(u32 id)
{
	const char *soc_family;

	switch (id) {
	case 0x370:
		soc_family = "Synaptics AS370";
		break;
	case 0x390:
		soc_family = "Synaptics AS390";
		break;
	case 0x470:
		soc_family = "Synaptics MYNA2";
		break;
	case 0x640:
		soc_family = "Synaptics PLATYPUS";
		break;
	case 0x680:
		soc_family = "Synaptics DOLPHIN";
		break;
	case 0x2610:
		soc_family = "Synaptics SL261X";
		break;
	case 0x3005:
		soc_family = "Marvell Berlin BG2CD";
		break;
	case 0x3006:
		soc_family = "Marvell Berlin BG2CDP";
		break;
	case 0x3012:
		soc_family = "Marvell Berlin BG4DTV";
		break;
	case 0x3017:
		soc_family = "Marvell Berlin BG4CDP";
		break;
	case 0x3100:
		soc_family = "Marvell Berlin BG2";
		break;
	case 0x3108:
		soc_family = "Marvell Berlin BG2CT";
		break;
	case 0x3114:
		soc_family = "Marvell Berlin BG2Q";
		break;
	case 0x3215:
		soc_family = "Marvell Berlin BG2Q4K";
		break;
	case 0x3218:
		soc_family = "Marvell Berlin BG4CT";
		break;
	case 0x3288:
		soc_family = "Marvell Berlin BG5CT";
		break;
	case 0x4006:
		soc_family = "Marvell Berlin BG4CD";
		break;
	default:
		soc_family = "<unknown>";
		break;
	}
	return soc_family;
}

static void rev_fixup(u32 id, u32 *rev)
{
	if (id == 0x680) {
		if (*rev == 0xa0)
			*rev = 0;
		if (*rev == 0xb0)
			*rev = 0xa0;
		if (*rev == 0xb1)
			*rev = 0xa1;
	}
}

static int berlin_chipid_probe(struct platform_device *pdev)
{
	u32 val, rev;
	struct device_node *dt_root;
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	struct device_node *np = pdev->dev.of_node;
	int ret;
	void __iomem *id_base;

	soc_dev_attr = devm_kzalloc(&pdev->dev, sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENOMEM;

	id_base = of_iomap(np, 0);
	if (!id_base)
		return -ENOMEM;

	dt_root = of_find_node_by_path("/");
	ret = of_property_read_string(dt_root, "model", &soc_dev_attr->machine);
	of_node_put(dt_root);
	if (ret < 0)
		soc_dev_attr->machine = "<unknown>";

	val = readl_relaxed(id_base);
	val = (val >> 12) & 0xffff;
	soc_dev_attr->family = berlin_id_to_family(val);

	ret = of_property_read_u32(np, "chip-revision", &rev);
	if (ret)
		rev = readl_relaxed(id_base + 4);
	rev_fixup(val, &rev);
	soc_dev_attr->revision = devm_kasprintf(&pdev->dev, GFP_KERNEL, "%X", rev);

	iounmap(id_base);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev))
		return PTR_ERR(soc_dev);

	platform_set_drvdata(pdev, soc_dev);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0))
#define RET void
#define RETURN
#else
#define RET int
#define RETURN return 0
#endif

static RET berlin_chipid_remove(struct platform_device *pdev)
{
	struct soc_device *soc_dev = platform_get_drvdata(pdev);

	soc_device_unregister(soc_dev);

	RETURN;
}

static struct platform_driver berlin_chipid_driver = {
	.probe		= berlin_chipid_probe,
	.remove		= berlin_chipid_remove,
	.driver		= {
		.name	= "berlin-chipid",
		.of_match_table = berlin_chipid_of_match,
	},
};
module_platform_driver(berlin_chipid_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics berlin chipid driver");
