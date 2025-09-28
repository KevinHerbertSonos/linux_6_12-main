/*
 * DSPG DVFA10 DSI-LDO pmu driver
 *
 * Copyright (c) 2017, DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dvf9a.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>

#define DVFA10_LDO_DSI "dvfa10-ldo-dsi"

struct dvfa10_ldo_dsi {
	struct dvf9a *dvfa10;
	struct platform_device *pdev;

	char name[14];
	struct regulator_dev *regulator;
};


static int
dvfa10_ldo_dsi_enable(struct regulator_dev *reg)
{
	struct dvfa10_ldo_dsi *dvfa10_ldo_dsi = rdev_get_drvdata(reg);

	daif_write(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI,
		   daif_read(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI) | 0x1);
	usleep_range(2000, 2100);
	daif_write(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI,
		   daif_read(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI) | 0x3);

	return 0;
}

static int
dvfa10_ldo_dsi_disable(struct regulator_dev *reg)
{
	struct dvfa10_ldo_dsi *dvfa10_ldo_dsi = rdev_get_drvdata(reg);

	daif_write(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI,
		   daif_read(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI) & ~0x3);

	return 0;
}

static int
dvfa10_ldo_dsi_is_enabled(struct regulator_dev *reg)
{
	struct dvfa10_ldo_dsi *dvfa10_ldo_dsi = rdev_get_drvdata(reg);

	return daif_read(dvfa10_ldo_dsi->dvfa10, DAIF_LDODSI) & 0x1;
}

static int
dvfa10_ldo_dsi_get_voltage(struct regulator_dev *reg)
{
	return reg->constraints->max_uV;
}

static struct regulator_ops dvfa10_ldo_dsi_ops = {
	.enable = dvfa10_ldo_dsi_enable,
	.disable = dvfa10_ldo_dsi_disable,
	.is_enabled = dvfa10_ldo_dsi_is_enabled,
	.get_voltage = dvfa10_ldo_dsi_get_voltage,
};

static const struct regulator_desc dvfa10_ldo_dsi_reg = {
	.name = "dvfa10-ldo-dsi",
	.id = 0,
	.ops = &dvfa10_ldo_dsi_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
};

static int
dvfa10_ldo_dsi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct dvf9a *dvfa10 = dev_get_drvdata(pdev->dev.parent);
	struct dvfa10_ldo_dsi *dvfa10_ldo_dsi;
	struct regulator_init_data *initdata;
	struct regulator_config config = { };

	if (!daif_has_dvfa10(dvfa10))
		return -ENODEV;

	dvfa10_ldo_dsi = devm_kzalloc(&pdev->dev, sizeof(*dvfa10_ldo_dsi),
				      GFP_KERNEL);
	if (!dvfa10_ldo_dsi)
		return -ENOMEM;

	dvfa10_ldo_dsi->pdev = pdev;
	dvfa10_ldo_dsi->dvfa10 = dvfa10;

	initdata = of_get_regulator_init_data(&pdev->dev, np,
					      &dvfa10_ldo_dsi_reg);

	config.dev = &pdev->dev;
	config.init_data = initdata;
	config.driver_data = dvfa10_ldo_dsi;
	config.of_node = np;

	dvfa10_ldo_dsi->regulator = devm_regulator_register(&pdev->dev,
							    &dvfa10_ldo_dsi_reg,
							    &config);
	if (IS_ERR(dvfa10_ldo_dsi->regulator)) {
		dev_err(&pdev->dev,
			"failed to register LDO-DSI regulator\n");
		return PTR_ERR(dvfa10_ldo_dsi->regulator);
	}

	platform_set_drvdata(pdev, dvfa10_ldo_dsi);

	dev_info(&pdev->dev, "successfully registered\n");

	return 0;
}

static int
dvfa10_ldo_dsi_remove(struct platform_device *pdev)
{
	struct dvfa10_ldo_dsi *dvfa10_ldo_dsi = platform_get_drvdata(pdev);

	kfree(dvfa10_ldo_dsi);
	dvfa10_ldo_dsi = NULL;

	return 0;
}

static const struct of_device_id dvfa10_ldo_dsi_of_ids[] = {
	{ .compatible = "dspg,dvfa10-ldo-dsi" },
	{ },
};

static struct platform_driver dvfa10_ldo_dsi_platform_driver = {
	.driver = {
		.name	= DVFA10_LDO_DSI,
		.owner	= THIS_MODULE,
		.of_match_table = dvfa10_ldo_dsi_of_ids,
	},
	.probe		= dvfa10_ldo_dsi_probe,
	.remove		= dvfa10_ldo_dsi_remove,
};
module_platform_driver(dvfa10_ldo_dsi_platform_driver);

MODULE_DESCRIPTION("DSPG DVFA10-LDO-DSI driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:dvfa10-ldo-dsi");
