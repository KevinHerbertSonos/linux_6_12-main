// SPDX-License-Identifier: GPL-2.0-only
/*
 * Synaptics GPIO mux driver
 *
 * Copyright (C) 2025 Synaptics Incorporated
 *
 * Author: Andreas Weissel <Andreas.Weissel@synaptics.com>
 */

#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#define MAX_GPIO_CHIPS 2

struct syna_gpio_mux {
	struct gpio_chip *parent;
	struct gpio_chip gpio[MAX_GPIO_CHIPS];
	void __iomem *base_addr;
	spinlock_t lock;
};

static void syna_gpio_mux_port_ctrl(struct syna_gpio_mux *mux,
				    struct gpio_chip *gc,
				    unsigned int offset)
{
	uint32_t val;
	unsigned long flags;

	spin_lock_irqsave(&mux->lock, flags);
	val = readl_relaxed(mux->base_addr);
	if (gc == &mux->gpio[0])
		val &= ~(1 << offset);
	else
		val |= 1 << offset;
	writel_relaxed(val, mux->base_addr);
	spin_unlock_irqrestore(&mux->lock, flags);
}

static int syna_gpio_mux_dir_in(struct gpio_chip *gc, unsigned int offset)
{
	struct syna_gpio_mux *mux = gpiochip_get_data(gc);

	syna_gpio_mux_port_ctrl(mux, gc, offset);
	mux->parent->direction_input(mux->parent, offset);

	return 0;
}

static int syna_gpio_mux_dir_out(struct gpio_chip *gc, unsigned int offset,
				 int value)
{
	struct syna_gpio_mux *mux = gpiochip_get_data(gc);

	syna_gpio_mux_port_ctrl(mux, gc, offset);
	mux->parent->direction_output(mux->parent, offset, value);

	return 0;
}

static int syna_gpio_mux_get(struct gpio_chip *gc, unsigned int offset)
{
	struct syna_gpio_mux *mux = gpiochip_get_data(gc);

	syna_gpio_mux_port_ctrl(mux, gc, offset);
	mux->parent->direction_input(mux->parent, offset);

	return !!mux->parent->get(mux->parent, offset);
}

static void syna_gpio_mux_set(struct gpio_chip *gc, unsigned int offset,
			      int value)
{
	struct syna_gpio_mux *mux = gpiochip_get_data(gc);

	syna_gpio_mux_port_ctrl(mux, gc, offset);
	mux->parent->direction_output(mux->parent, offset, value);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0))
static int of_gpiochip_match_node(struct gpio_chip *chip, void *data)
{
	return chip->of_node == data;
}
#endif

static int syna_gpio_mux_probe(struct platform_device *pdev)
{
	struct syna_gpio_mux *mux;
	struct gpio_chip *chip;
	struct device_node *controller;
	struct fwnode_handle *fwnode;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0))
	struct gpio_device *gpio_dev;
#endif
	unsigned int index;
	const __be32 *prop;
	int ret = 0;

	mux = devm_kzalloc(&pdev->dev, sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return -ENOMEM;

	prop = of_get_property(pdev->dev.of_node, "gpio-controller", NULL);
	if (!prop) {
		dev_err(&pdev->dev,
			"required 'gpio-controller' property missing\n");
		return -EINVAL;
	}

	controller = of_find_node_by_phandle(be32_to_cpup(prop));
	if (!controller) {
		dev_err(&pdev->dev,
			"failed to find the parent gpio controller node\n");
		return -ENODEV;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0))
	gpio_dev = gpio_device_find_by_fwnode(of_fwnode_handle(controller));
	if (gpio_dev) {
		mux->parent = gpio_device_get_chip(gpio_dev);
		gpio_device_put(gpio_dev);
	}
#else
	mux->parent = gpiochip_find(controller, of_gpiochip_match_node);
#endif
	if (!mux->parent) {
		of_node_put(controller);
		return -EPROBE_DEFER;
	}

	of_node_put(controller);

	mux->base_addr = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mux->base_addr))
		return PTR_ERR(mux->base_addr);

	spin_lock_init(&mux->lock);
	platform_set_drvdata(pdev, mux);

	device_for_each_child_node(&pdev->dev, fwnode) {
		ret = fwnode_property_read_u32(fwnode, "reg", &index);
		if (ret)
			break;
		if (index >= MAX_GPIO_CHIPS) {
			ret = -ENODEV;
			break;
		}

		chip = &mux->gpio[index];

		ret = fwnode_property_read_string(fwnode, "label",
						  &chip->label);
		if (ret)
			break;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
		chip->fwnode = fwnode;
#else
		chip->of_node = to_of_node(fwnode);
#endif
		chip->base = -1;
		chip->ngpio = mux->parent->ngpio;
		chip->direction_input = syna_gpio_mux_dir_in;
		chip->direction_output = syna_gpio_mux_dir_out;
		chip->get = syna_gpio_mux_get;
		chip->set = syna_gpio_mux_set;

		ret = devm_gpiochip_add_data(&pdev->dev, chip, mux);
		if (ret)
			break;
	}

	return ret;
}

static const struct of_device_id syna_gpio_mux_match[] = {
	{ .compatible = "syna,gpio-mux" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, syna_gpio_mux_match);

static struct platform_driver syna_gpio_mux_driver = {
	.probe	= syna_gpio_mux_probe,
	.driver	= {
		.name = "syna_gpio_mux",
		.of_match_table = syna_gpio_mux_match,
	},
};

module_platform_driver(syna_gpio_mux_driver);
MODULE_AUTHOR("Andreas Weissel <Andreas.Weissel@synaptics.com>");
MODULE_DESCRIPTION("Synaptics GPIO muxing driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:syna_gpio_mux");
