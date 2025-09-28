/*
 * Copyright (C) 2014 Marvell Technology Group Ltd.
 *
 * Marvell Berlin reset driver
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/types.h>

#define BERLIN_MAX_RESETS	32

#define to_berlin_reset_priv(p)		\
	container_of((p), struct berlin_reset_priv, rcdev)

struct berlin_reset_priv {
	spinlock_t			lock;
	void __iomem			*base;
	unsigned int			size;
	struct reset_controller_dev	rcdev;
};

static int berlin_reset_reset(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x7f);
	int sticky = (id & 0x80);

	if (sticky)
		return -EINVAL;

	writel(mask, priv->base + offset);

	/* let the reset be effective */
	udelay(10);

	return 0;
}

static int berlin_reset_assert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	u32 reg;
	unsigned long flags;
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x3f);
	int sticky = (id & 0x80);
	int inverted = (id & 0x40);

	if (!sticky)
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	reg = readl(priv->base + offset);
	if (inverted)
		reg |= mask;
	else
		reg &= ~mask;
	writel(reg, priv->base + offset);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int berlin_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	u32 reg;
	unsigned long flags;
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x3f);
	int sticky = (id & 0x80);
	int inverted = (id & 0x40);

	if (!sticky)
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	reg = readl(priv->base + offset);
	if (inverted)
		reg &= ~mask;
	else
		reg |= mask;
	writel(reg, priv->base + offset);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int berlin_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	u32 reg;
	unsigned long flags;
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x3f);
	int sticky = (id & 0x80);
	int inverted = (id & 0x40);

	if (!sticky)
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	reg = readl(priv->base + offset);
	spin_unlock_irqrestore(&priv->lock, flags);

	if (inverted)
		return !!(reg & mask);
	else
		return !(reg & mask);
}

static const struct reset_control_ops berlin_reset_ops = {
	.reset		= berlin_reset_reset,
	.assert		= berlin_reset_assert,
	.deassert	= berlin_reset_deassert,
	.status		= berlin_reset_status,
};

static int berlin_reset_xlate(struct reset_controller_dev *rcdev,
			      const struct of_phandle_args *reset_spec)
{
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	unsigned offset, bit, sticky, inverted;

	offset = reset_spec->args[0];
	bit = reset_spec->args[1];
	sticky = reset_spec->args[2];
	inverted = reset_spec->args[3];

	if (offset >= priv->size)
		return -EINVAL;

	if (bit >= BERLIN_MAX_RESETS)
		return -EINVAL;

	return (offset << 8) | (sticky << 7) | (inverted << 6) | bit;
}

static int berlin2_reset_probe(struct platform_device *pdev)
{
	struct berlin_reset_priv *priv;
	struct resource *res;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	devm_request_mem_region(&pdev->dev, res->start, resource_size(res), res->name);

	priv->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!priv->base)
		ret = -ENOMEM;

	priv->size = resource_size(res);

	spin_lock_init(&priv->lock);

	priv->rcdev.owner = THIS_MODULE;
	priv->rcdev.ops = &berlin_reset_ops;
	priv->rcdev.of_node = pdev->dev.of_node;
	priv->rcdev.of_reset_n_cells = 4;
	priv->rcdev.of_xlate = berlin_reset_xlate;

	return reset_controller_register(&priv->rcdev);
}

static const struct of_device_id berlin_reset_dt_match[] = {
	{ .compatible = "marvell,berlin2-chip-ctrl" },
	{ .compatible = "marvell,berlin2cd-chip-ctrl" },
	{ .compatible = "marvell,berlin2q-chip-ctrl" },
	{ .compatible = "marvell,berlin4ct-chip-ctrl" },
	{ .compatible = "marvell,berlin4cdp-chip-ctrl" },
	{ },
};
MODULE_DEVICE_TABLE(of, berlin_reset_dt_match);


static struct platform_driver berlin_reset_driver = {
	.probe	= berlin2_reset_probe,
	.driver	= {
		.name = "berlin2-reset",
		.of_match_table = berlin_reset_dt_match,
	},
};
module_platform_driver(berlin_reset_driver);

MODULE_AUTHOR("Antoine Tenart <antoine.tenart@free-electrons.com>");
MODULE_AUTHOR("Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>");
MODULE_DESCRIPTION("Synaptics Berlin reset controller");
MODULE_LICENSE("GPL");

