// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2024 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <linux/mfd/syscon.h>
#include <mach/hardware.h>
#include <dt-bindings/reset-controller/dvf99-resets.h>

struct dvf_reset_controller {
	spinlock_t lock;
	struct device *dev;
	struct reset_controller_dev rst;
	struct regmap *map;
};

#define SWCMWRST1 0x50

#define to_dvf_reset_controller(_rst) \
	container_of(_rst, struct dvf_reset_controller, rst)

static int dvf_set_reset(struct reset_controller_dev *rcdev, unsigned long id,
			 unsigned int val)
{
	struct dvf_reset_controller *rst = to_dvf_reset_controller(rcdev);

	if (id >= rcdev->nr_resets)
		return -EINVAL;

	dev_dbg(rst->dev,
		"writing 0x%x -> 0x%x / 0x%x\n",
		(unsigned int)(SWCMWRST1 + (id >> 5) * 4),
		(unsigned int)(1 << (id % 32)), (!!val) << (id % 32));

	return regmap_update_bits(rst->map, SWCMWRST1 + (id >> 5) * 4,
				  1 << (id % 32), (!!val) << (id % 32));
}

static int dvf_reset_assert(struct reset_controller_dev *rcdev,
			    unsigned long id)
{
	return dvf_set_reset(rcdev, id, 1);
}

static int dvf_reset_deassert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	return dvf_set_reset(rcdev, id, 0);
}

static int dvf_reset_status(struct reset_controller_dev *rcdev,
			    unsigned long id)
{
	struct dvf_reset_controller *rst = to_dvf_reset_controller(rcdev);
	unsigned int val;
	int ret;

	if (id >= rcdev->nr_resets)
		return -EINVAL;

	ret = regmap_read(rst->map, SWCMWRST1 + (id >> 5) * 4, &val);
	if (ret)
		return ret;

	dev_dbg(rst->dev,
		"reading 0x%x -> 0x%x / %d\n",
		(unsigned int)(SWCMWRST1 + (id >> 5) * 4), val,
		!!(val & (id % 32)));

	return !!(val & (id % 32));
}

static struct of_device_id dvf99_reset_match[] = {
	{
		.compatible = "dspg,dvf99-resets",
	},
	{ /* sentinel */ },
};

static struct reset_control_ops dvf_reset_ops = {
	.assert   = dvf_reset_assert,
	.deassert = dvf_reset_deassert,
	.status   = dvf_reset_status,
};

int dvf_reset_probe(struct platform_device *pdev)
{
	struct device *dev = pdev ? &pdev->dev : NULL;
	struct dvf_reset_controller *rc;
	int err;

	rc = devm_kzalloc(dev, sizeof(*rc), GFP_KERNEL);
	if (!rc)
		return -ENOMEM;

	spin_lock_init(&rc->lock);

	rc->rst.ops = &dvf_reset_ops,
	rc->rst.of_node = dev->of_node;
	rc->rst.nr_resets = 64;

	rc->map = syscon_regmap_lookup_by_phandle(dev->of_node, "dvf,cmu");
	if (IS_ERR(rc->map))
		return PTR_ERR(rc->map);

	err = reset_controller_register(&rc->rst);
	if (!err)
		dev_info(dev, "registered\n");

	return err;
}

static struct platform_driver dvf99_reset_driver = {
	.probe = dvf_reset_probe,
	.driver = {
		.name = "reset-dvf99",
		.of_match_table = dvf99_reset_match,
	},
};

static int __init dvf99_reset_init(void)
{
	return platform_driver_register(&dvf99_reset_driver);
}

arch_initcall(dvf99_reset_init);
