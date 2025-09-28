// SPDX-License-Identifier: GPL-2.0+

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl-dvf.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

/* regmap offset is 0x050002a0 */
#define EXTINT_CFG		0x0
#define EXTINT_EDGE_CLEAR	0xc

#define EXTINT_FLAG_MASKED	(1 << 0)
#define EXTINT_FLAG_BOTH_EDGES	(1 << 1)

struct dspg_extint_ctrl {
	struct regmap		*regmap;
	struct dspg_gpio	*dspg_gpio;
	spinlock_t		extint_lock;
	int			index;
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*states[2];
	bool			alt_mux;
};

static void
dvf_extint_adjust_pol(struct dspg_extint *extint)
{
	unsigned int value;
	int pol, shift = extint->bit * 2;

	pol = !dspg_gpiolib_get(&extint->dspg_gpio->chip,
		extint->dspg_gpio->pdata.extint_gpios[extint->ctrl->index]);

	regmap_read(extint->ctrl->regmap, EXTINT_CFG, &value);
	value &= ~(0x1 << shift);
	value |= pol << shift;
	regmap_update_bits(extint->ctrl->regmap, EXTINT_CFG, 0x1 << shift,
			   value);
}

static void
dvf_extint_irq_ack(struct irq_data *data)
{
	struct dspg_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;

	regmap_update_bits(extint->ctrl->regmap, EXTINT_EDGE_CLEAR,
			   1 << extint->bit, 1 << extint->bit);

	if (extint->flags & EXTINT_FLAG_BOTH_EDGES) {
		spin_lock_irqsave(&extint->ctrl->extint_lock, flags);
		dvf_extint_adjust_pol(extint);
		spin_unlock_irqrestore(&extint->ctrl->extint_lock, flags);
	}
}

static void
dvf_extint_irq_mask(struct irq_data *data)
{
	struct dspg_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;

	spin_lock_irqsave(&extint->ctrl->extint_lock, flags);
	if (!(extint->flags & EXTINT_FLAG_MASKED)) {
		disable_irq_nosync(extint->upstream_irq);
		extint->flags |= EXTINT_FLAG_MASKED;
	}
	spin_unlock_irqrestore(&extint->ctrl->extint_lock, flags);
}

static void
dvf_extint_irq_unmask(struct irq_data *data)
{
	struct dspg_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;

	spin_lock_irqsave(&extint->ctrl->extint_lock, flags);
	if (extint->flags & EXTINT_FLAG_MASKED) {
		if (extint->flags & EXTINT_FLAG_BOTH_EDGES)
			dvf_extint_adjust_pol(extint);

		enable_irq(extint->upstream_irq);
		extint->flags &= ~EXTINT_FLAG_MASKED;
	}
	spin_unlock_irqrestore(&extint->ctrl->extint_lock, flags);
}

static int
dvf_extint_irq_set_type(struct irq_data *data, unsigned int type)
{
	struct dspg_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;
	unsigned int value;
	int pol = 0;  /* 0 = active low; 1 = active high */
	int mode = 0; /* 0 = level; 1 = edge */
	int shift = extint->bit * 2;

	type &= IRQ_TYPE_SENSE_MASK;

	spin_lock_irqsave(&extint->ctrl->extint_lock, flags);

	if ((type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		/*
		 * emulate EDGE_BOTH by setting LEVEL_HIGH or LEVEL_LOW
		 * depending on current gpio value
		 */
		extint->flags |= EXTINT_FLAG_BOTH_EDGES;
		pol = !dspg_gpiolib_get(&extint->dspg_gpio->chip,
			extint->dspg_gpio->
				pdata.extint_gpios[extint->ctrl->index]);
	} else {
		extint->flags &= ~EXTINT_FLAG_BOTH_EDGES;

		if (type & IRQ_TYPE_EDGE_BOTH) {
			/* specific edge type irq */
			mode = 1;

			if (type & IRQ_TYPE_EDGE_RISING)
				pol = 1;
		} else if (type & IRQ_TYPE_LEVEL_HIGH) {
			pol = 1;
		}
	}

	regmap_read(extint->ctrl->regmap, EXTINT_CFG, &value);
	value &= ~(0x3 << shift);
	value |= ((mode << 1) | pol) << shift;
	regmap_update_bits(extint->ctrl->regmap, EXTINT_CFG, 0x3 << shift,
			   value);

	spin_unlock_irqrestore(&extint->ctrl->extint_lock, flags);

	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH))
		irq_set_handler_locked(data, handle_level_irq);
	else if (type & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		irq_set_handler_locked(data, handle_edge_irq);

	return 0;
}

static int
dvf_extint_irq_set_wake(struct irq_data *data, unsigned int enable)
{
	struct dspg_extint *extint = irq_data_get_irq_chip_data(data);

	return irq_set_irq_wake(extint->upstream_irq, enable);
}

static struct irq_chip extint_irq_chip = {
	.name		= "dvf-extint",
	.irq_ack	= dvf_extint_irq_ack,
	.irq_mask	= dvf_extint_irq_mask,
	.irq_unmask	= dvf_extint_irq_unmask,
	.irq_set_type	= dvf_extint_irq_set_type,
	.irq_set_wake	= dvf_extint_irq_set_wake,
};

static irqreturn_t
dspg_extint_irq_handler(int irq, void *dev_id)
{
	unsigned long flags;
	struct dspg_extint *extint = dev_id;

	local_irq_save(flags);
	generic_handle_irq(irq_linear_revmap(extint->irq_domain, 0));
	local_irq_restore(flags);

	return IRQ_HANDLED;
}

/*
 * This lock class tells lockdep that EXTINTs are in a different category than
 * their parents, so it won't report false recursion.
 */
static struct lock_class_key gpio_lock_class;
static struct lock_class_key gpio_request_class;

static int
dvf_extint_irq_map(struct irq_domain *dom, unsigned int irq,
		   irq_hw_number_t hw_irq)
{
	struct dspg_extint *extint = dom->host_data;
	int ret;

	ret = pinctrl_select_state(extint->ctrl->pinctrl,
				extint->ctrl->states[extint->ctrl->alt_mux]);
	if (ret)
		return ret;

	irq_set_lockdep_class(irq, &gpio_lock_class, &gpio_request_class);
	irq_set_chip_data(irq, extint);
	irq_set_chip_and_handler(irq, &extint_irq_chip, handle_edge_irq);
	irq_set_status_flags(irq, IRQ_TYPE_EDGE_BOTH);

	irq_clear_status_flags(irq, IRQ_NOREQUEST);

	return 0;
}

static int
dvf_extint_irq_domain_xlate(struct irq_domain *d, struct device_node *ctrlr,
			    const u32 *intspec, unsigned int intsize,
			    unsigned long *out_hwirq, unsigned int *out_type)
{
	*out_hwirq = 0;
	if (intsize > 0)
		*out_type = intspec[0] & IRQ_TYPE_SENSE_MASK;
	else
		*out_type = IRQ_TYPE_NONE;

	return 0;
}

static const struct irq_domain_ops dspg_extint_irq_ops = {
	.map	= dvf_extint_irq_map,
	.xlate	= dvf_extint_irq_domain_xlate,
};

static int
dvf_extint_probe(struct platform_device *pdev)
{
	struct dspg_extint_ctrl *dspg_extint;
	struct dspg_gpio *dspg_gpio = dev_get_drvdata(pdev->dev.parent);
	struct dspg_extint *extint;
	int irq, ret = 0;
	struct device_node *np = pdev->dev.of_node;
	const char *name;
	u32 index;

	dspg_extint = devm_kzalloc(&pdev->dev, sizeof(*dspg_extint),
				   GFP_KERNEL);
	if (!dspg_extint)
		return -ENOMEM;
	dspg_extint->dspg_gpio = dspg_gpio;
	spin_lock_init(&dspg_extint->extint_lock);

	ret = of_property_read_u32(np, "reg", &index);
	if (ret) {
		dev_err(&pdev->dev, "could not parse 'index' property\n");
		return -EINVAL;
	}
	dspg_extint->index = index;
	extint = &dspg_gpio->extints[index];

	dspg_extint->alt_mux = of_property_read_bool(np, "alt-mux");

	dspg_extint->regmap = syscon_regmap_lookup_by_phandle(np,
							      "extint-regmap");
	if (IS_ERR(dspg_extint->regmap)) {
		dev_err(&pdev->dev, "error looking up regmap phandle\n");
		return PTR_ERR(dspg_extint->regmap);
	}
	if (dspg_gpio->extints[index].irq_domain) {
		dev_err(&pdev->dev,
			"EXTINT %d domain already exists\n", index);
		return -EEXIST;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "invalid IRQ resource\n");
		return -EINVAL;
	}

	extint->upstream_irq = irq;
	extint->ctrl = dspg_extint;
	extint->dspg_gpio = dspg_gpio;
	extint->irq_domain = irq_domain_add_linear(np, 1, &dspg_extint_irq_ops,
						   extint);
	if (!extint->irq_domain) {
		dev_err(&pdev->dev, "cannot register IRQ domain\n");
		return -ENOMEM;
	}
	extint->flags = EXTINT_FLAG_MASKED;

	{
		u32 value;
		int pol, shift = extint->bit * 2;

		pol = !dspg_gpiolib_get(&extint->dspg_gpio->chip,
			extint->dspg_gpio->
				pdata.extint_gpios[extint->ctrl->index]);

		regmap_read(extint->ctrl->regmap, EXTINT_CFG, &value);
		value &= ~(0x3 << shift);
		value |= pol << shift;
		regmap_update_bits(extint->ctrl->regmap, EXTINT_CFG,
				   0x3 << shift, value);
		extint->flags |= EXTINT_FLAG_BOTH_EDGES;
	}

	irq_clear_status_flags(irq, IRQ_NOREQUEST);
	irq_set_status_flags(irq, IRQ_NOAUTOEN);
	ret = devm_request_irq(&pdev->dev, irq, dspg_extint_irq_handler, 0,
			       "gpio-dspg", extint);
	if (ret) {
		dev_err(&pdev->dev, "cannot request upstream irq %d\n",
			irq);
		ret = -EINVAL;
		goto out_irq_domain;
	}

	dspg_extint->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(dspg_extint->pinctrl)) {
		dev_err(&pdev->dev, "could not get pinctrl\n");
		ret = -ENODEV;
		goto out_irq_domain;
	}

	of_property_read_string_index(np, "pinctrl-names", 0, &name);
	dspg_extint->states[0] = pinctrl_lookup_state(
						dspg_extint->pinctrl, name);
	if (of_property_count_strings(np, "pinctrl-names") > 1) {
		of_property_read_string_index(np, "pinctrl-names", 1, &name);
		dspg_extint->states[1] = pinctrl_lookup_state(
						dspg_extint->pinctrl, name);
	}

	if (IS_ERR(dspg_extint->states[0]) ||
	    (dspg_extint->alt_mux && IS_ERR(dspg_extint->states[1]))) {
		dev_err(&pdev->dev, "parsing pinctrl states failed\n");
		ret = -EINVAL;
		goto out_irq_domain;
	}

	return 0;
out_irq_domain:
	irq_domain_remove(extint->irq_domain);
	extint->irq_domain = NULL;

	return ret;
}

static const struct of_device_id dvf_extint_of_match[] = {
	{ .compatible = "dspg,extint-dvf" },
	{ /* sentinel */ }
};

static struct platform_driver dvf_extint_driver = {
	.driver = {
		.name		= "dvf-extint",
		.owner		= THIS_MODULE,
		.of_match_table	= dvf_extint_of_match,
	},
	.probe = dvf_extint_probe,
};
module_platform_driver(dvf_extint_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
