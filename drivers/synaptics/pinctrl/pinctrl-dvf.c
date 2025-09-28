/*
 * Core driver for the DVF99 pin controller
 *
 * Copyright (C) 2016 DSPG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/gpio-dspg.h>
#include <linux/seq_file.h>
#include "core.h"

#define DVF_GPIO_PINS_PER_BANK	32

#define OF_GPIO_SET_PULL	(1 << 1)
#define OF_GPIO_PULL_CTRL	(1 << 2)
#define OF_GPIO_PULL_TYPE	(1 << 3)

#define GCR2			0x08
#define SPCR1			0x54

struct dvf_pinconf {
	int			pin;
	const char		*name;
	unsigned long		config;
	int			altfunc;
};

struct dvf_padconf {
	int			pad;
	int			value;
};

struct dvf_pmx_func {
	const char		*name;
	const char		**groups;
	unsigned int		ngroups;
};

struct dvf_pctl_group {
	const char		*name;
	unsigned int		*pins;
	unsigned int		npins;
	unsigned int		npads;
	struct dvf_pinconf	*pin_conf;
	struct dvf_padconf	*pad_conf;
};

struct dvf_gpio_bank {
	u32 offset;
	struct gpio_chip	gpio_chip;
	struct pinctrl_gpio_range range;
	void __iomem		*base;
	struct irq_domain	*domain;
	unsigned long		irq_edge_conf;
	spinlock_t		lock;
};

struct dvf_pinctrl {
	struct device		*dev;
	struct pinctrl_dev	*pctl;
	struct dvf_gpio_bank	*banks;
	int			nbanks;
	struct dvf_pmx_func	*functions;
	int			nfunctions;
	struct dvf_pctl_group	*groups;
	int			ngroups;
	struct regmap		*regmap;
	void __iomem		*irqmux_base;
	struct dvf_gpio		*gpio;
};

#define EXTINT_CFG		0x2A0
#define EXTINT_EDGE_CLEAR	0x2AC

#define EXTINT_FLAG_MASKED	(1 << 0)
#define EXTINT_FLAG_BOTH_EDGES	(1 << 1)

struct dvf_gpio;

struct dvf_extint {
	unsigned int		bit;
	u16			hwirq;
	u16			flags;
	struct dvf_gpio		*dvf_gpio;
	struct irq_domain	*irq_domain;
	int			upstream_irq;
};

struct dvf_gpio {
	struct gpio_chip	chip;
	void			*regs;
	struct resource		*mem_res;
	struct dspg_gpio_pdata	pdata;
	struct irq_domain	*extint_domain;
	int			nr_extints;
	struct dvf_extint	extints[16];
};

static DEFINE_SPINLOCK(dvf_extint_lock);

#define GET_REG(priv, reg, gpio) ( \
	__raw_readl((priv)->regs + (__BANK(gpio) * 0x60) + (reg)) & \
		    (1 << __PIN(gpio)) \
	)

#define SET_REG(priv, reg, gpio) \
	__raw_writel(1 << __PIN(gpio), \
		     (priv)->regs + (__BANK(gpio) * 0x60) + (reg))

static int
dvf_gpiolib_get_pull_enable(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	return !!GET_REG(dvf_gpio, XGP_PULL_CTRL, gpio);
}

/*
 * Returns: 0: Output, 1: Input
 */
static int
dvf_gpiolib_get_direction(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	return !!GET_REG(dvf_gpio, XGP_DIR, gpio);
}

static int
dvf_gpiolib_get_value(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	return !!GET_REG(dvf_gpio, XGP_DATA, gpio);
}

static void
dvf_gpiolib_set_value(struct dvf_gpio *dvf_gpio, unsigned int gpio, int value)
{
	if (value)
		SET_REG(dvf_gpio, XGP_DATA_SET, gpio);
	else
		SET_REG(dvf_gpio, XGP_DATA_CLR, gpio);
}

static void
dvf_gpiolib_set_enable(struct dvf_gpio *dvf_gpio, unsigned int gpio, int value)
{
	if (value)
		SET_REG(dvf_gpio, XGP_EN_SET, gpio);
	else
		SET_REG(dvf_gpio, XGP_EN_CLR, gpio);
}

static int
dvf_gpiolib_get_opendrain(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	return !!GET_REG(dvf_gpio, XGP_OD, gpio);
}

static int
dvf_gpiolib_get_keeper(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	return !!GET_REG(dvf_gpio, XGP_KEEP, gpio);
}

static int
dvf_gpiolib_get_pull_selection(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	return !!GET_REG(dvf_gpio, XGP_PULL_TYPE, gpio);
}

static void
dvf_gpiolib_set_pull(struct dvf_gpio *dvf_gpio, unsigned int gpio, int ctrl,
		     int value)
{
	if (value)
		SET_REG(dvf_gpio, XGP_PULL_UP, gpio);
	else
		SET_REG(dvf_gpio, XGP_PULL_DOWN, gpio);
	if (ctrl)
		SET_REG(dvf_gpio, XGP_PULL_EN, gpio);
	else
		SET_REG(dvf_gpio, XGP_PULL_DIS, gpio);
}

/*
 * Tests whether the Pull-Up/Dn internal resistor can be disconnected, in order
 * to save power.
 */
static void
dvf_gpiolib_tst_pull_disconnect(struct dvf_gpio *dvf_gpio, unsigned int gpio)
{
	if (!dvf_gpiolib_get_pull_enable(dvf_gpio, gpio))
		return;

	if (dvf_gpiolib_get_direction(dvf_gpio, gpio) == 0) {
		/* Output gpio */

		/* Note:
		 * "Wired-OR" is achived by enabling "Open-Drain" and
		 * No Pull-Down resistor, but the user may connect
		 * a Pull-Up resistor, if no such External resistor
		 * exists. So "open-drain" must be excluded !
		 */
		if (!dvf_gpiolib_get_opendrain(dvf_gpio, gpio))
			SET_REG(dvf_gpio, XGP_PULL_DIS, gpio);
	} else {
		/* Input gpio */

		if (dvf_gpiolib_get_keeper(dvf_gpio, gpio))
			SET_REG(dvf_gpio, XGP_PULL_DIS, gpio);
	}
}

static int
dvf_gpiolib_request(struct gpio_chip *chip, unsigned int offset)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);

	if (dvf_gpio->pdata.nx_masks[__BANK(offset)] & (1 << __PIN(offset))) {
		dev_warn(chip->parent,
			 "request for non-existent %cGPIO%-2d denied!\n",
			 'A' + __BANK(offset), __PIN(offset));
		return -ENODEV;
	}

	return 0;
}

static int
dvf_gpiolib_direction_input(struct gpio_chip *chip, unsigned int offset)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);

	SET_REG(dvf_gpio, XGP_DIR_IN, offset);
	dvf_gpiolib_tst_pull_disconnect(dvf_gpio, offset);
	dvf_gpiolib_set_enable(dvf_gpio, offset, 1);

	return 0;
}

static int
dvf_gpiolib_direction_output(struct gpio_chip *chip, unsigned int offset,
			     int value)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);

	dvf_gpiolib_set_value(dvf_gpio, offset, value);
	SET_REG(dvf_gpio, XGP_DIR_OUT, offset);
	dvf_gpiolib_tst_pull_disconnect(dvf_gpio, offset);
	dvf_gpiolib_set_enable(dvf_gpio, offset, 1);

	return 0;
}

static int
dvf_gpiolib_get(struct gpio_chip *chip, unsigned int offset)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);

	return !!GET_REG(dvf_gpio, XGP_DATA, offset);
}

static void
dvf_gpiolib_set(struct gpio_chip *chip, unsigned int offset, int value)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);

	dvf_gpiolib_set_value(dvf_gpio, offset, value);
}

static void
dvf_gpiolib_show(struct seq_file *s, struct gpio_chip *chip)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);
	const char *label;
	unsigned int i;

	for (i = 0; i < chip->ngpio; i++) {
		label = gpiochip_is_requested(chip, i);
		if (!label)
			continue;

		seq_printf(s, " %cGPIO%-2d (%-20.20s) %s %s%s",
			   'A' + __BANK(i), __PIN(i), label,
			   dvf_gpiolib_get_direction(dvf_gpio, i) ?
				"in " : "out",
			   dvf_gpiolib_get(chip, i) ? "hi" : "lo",
			   dvf_gpiolib_get_pull_enable(dvf_gpio, i)
				? (dvf_gpiolib_get_pull_selection(dvf_gpio, i)
				? " pu" : " pd") : "");
		seq_puts(s, "\n");
	}
}

static int
dvf_gpiolib_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	struct dvf_gpio *dvf_gpio = container_of(chip, struct dvf_gpio, chip);
	int i;

	/* search for matching GPIO and then map through exting IRQ domain */
	for (i = 0; i < dvf_gpio->nr_extints; i++) {
		if (dvf_gpio->pdata.extint_gpios[i] == offset)
			return irq_create_mapping(dvf_gpio->extint_domain, i);
	}

	return -ENODEV;
}

#ifdef CONFIG_OF
static int
dvf_gpiolib_xlate(struct gpio_chip *gc,
		  const struct of_phandle_args *gpiospec, u32 *flags)
{
	struct dvf_gpio *dvf_gpio = container_of(gc, struct dvf_gpio, chip);
	unsigned int pin;

	pin = gpiospec->args[0] * 32 + gpiospec->args[1];

	if (pin >= gc->ngpio || gpiospec->args[1] > 32)
		return -EINVAL;

	if (dvf_gpio->pdata.nx_masks[__BANK(pin)] & (1 << __PIN(pin)))
		return -ENODEV;

	if (flags)
		*flags = gpiospec->args[2];

	return pin;
}
#endif

static void
dvf_extint_adjust_pol(struct dvf_extint *extint)
{
	unsigned long value;
	int pol, shift = extint->bit * 2;

	pol = !dvf_gpiolib_get(&extint->dvf_gpio->chip,
		extint->dvf_gpio->pdata.extint_gpios[extint->hwirq]);

	value = readl(extint->dvf_gpio->regs + EXTINT_CFG);
	value &= ~(0x1UL << shift);
	value |= pol << shift;
	writel(value, extint->dvf_gpio->regs + EXTINT_CFG);
}

static void
dvf_extint_irq_ack(struct irq_data *data)
{
	struct dvf_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;

	writel(1 << extint->bit, extint->dvf_gpio->regs + EXTINT_EDGE_CLEAR);

	if (extint->flags & EXTINT_FLAG_BOTH_EDGES) {
		spin_lock_irqsave(&dvf_extint_lock, flags);
		dvf_extint_adjust_pol(extint);
		spin_unlock_irqrestore(&dvf_extint_lock, flags);
	}
}

static void
dvf_extint_irq_mask(struct irq_data *data)
{
	struct dvf_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;

	spin_lock_irqsave(&dvf_extint_lock, flags);
	if (!(extint->flags & EXTINT_FLAG_MASKED)) {
		disable_irq_nosync(extint->upstream_irq);
		extint->flags |= EXTINT_FLAG_MASKED;
	}
	spin_unlock_irqrestore(&dvf_extint_lock, flags);
}

static void
dvf_extint_irq_unmask(struct irq_data *data)
{
	struct dvf_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags;

	spin_lock_irqsave(&dvf_extint_lock, flags);
	if (extint->flags & EXTINT_FLAG_MASKED) {
		if (extint->flags & EXTINT_FLAG_BOTH_EDGES)
			dvf_extint_adjust_pol(extint);

		enable_irq(extint->upstream_irq);
		extint->flags &= ~EXTINT_FLAG_MASKED;
	}
	spin_unlock_irqrestore(&dvf_extint_lock, flags);
}

static int
dvf_extint_irq_set_type(struct irq_data *data, unsigned int type)
{
	struct dvf_extint *extint = irq_data_get_irq_chip_data(data);
	unsigned long flags, value;
	int pol = 0; /* 0 = active low; 1 = active high */
	int mode = 0;     /* 0 = level; 1 = edge */
	int shift;

	type &= IRQ_TYPE_SENSE_MASK;

	spin_lock_irqsave(&dvf_extint_lock, flags);

	if ((type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		/*
		 * emulate EDGE_BOTH by setting LEVEL_HIGH or LEVEL_LOW
		 * depending on current gpio value
		 */
		extint->flags |= EXTINT_FLAG_BOTH_EDGES;
		pol = !dvf_gpiolib_get(&extint->dvf_gpio->chip,
			extint->dvf_gpio->pdata.extint_gpios[extint->hwirq]);
	} else {
		extint->flags &= ~EXTINT_FLAG_BOTH_EDGES;

		if (type & IRQ_TYPE_EDGE_BOTH) {
			/* specific edge type irq */
			mode = 1;

			if (type & IRQ_TYPE_EDGE_RISING)
				pol = 1;
		} else if (type & IRQ_TYPE_LEVEL_HIGH)
			pol = 1;
	}

	shift = extint->bit << 1; /* extint * 2 */

	value = readl(extint->dvf_gpio->regs + EXTINT_CFG);
	value &= ~(0x3UL << shift);
	value |= ((mode << 1) | pol) << shift;
	writel(value, extint->dvf_gpio->regs + EXTINT_CFG);

	spin_unlock_irqrestore(&dvf_extint_lock, flags);

	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH))
		irq_set_handler_locked(data, handle_level_irq);
	else if (type & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		irq_set_handler_locked(data, handle_edge_irq);

	return 0;
}

static int
dvf_extint_irq_set_wake(struct irq_data *data, unsigned int enable)
{
	struct dvf_extint *extint = irq_data_get_irq_chip_data(data);

	return irq_set_irq_wake(extint->upstream_irq, enable);
}

static struct irq_chip dvf_extint_irq_chip = {
	.name		= "gpio",
	.irq_ack	= dvf_extint_irq_ack,
	.irq_mask	= dvf_extint_irq_mask,
	.irq_unmask	= dvf_extint_irq_unmask,
	.irq_set_type	= dvf_extint_irq_set_type,
	.irq_set_wake	= dvf_extint_irq_set_wake,
};

static irqreturn_t
dvf_extint_irq_handler(int irq, void *dev_id)
{
	unsigned long flags;
	struct dvf_extint *extint = dev_id;

	local_irq_save(flags);
	generic_handle_irq(irq_linear_revmap(extint->irq_domain,
			   extint->hwirq));
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
	struct dvf_gpio *dvf_gpio = dom->host_data;

	irq_set_lockdep_class(irq, &gpio_lock_class, &gpio_request_class);
	irq_set_chip_data(irq, &(dvf_gpio->extints[hw_irq]));
	irq_set_chip_and_handler(irq, &dvf_extint_irq_chip, handle_level_irq);

	return 0;
}

static const struct irq_domain_ops extint_irq_ops = {
	.map	= dvf_extint_irq_map,
	.xlate	= irq_domain_xlate_twocell,
};

/* Pinctrl Groups */
static int
dvf_pctl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->ngroups;
}

static const char *
dvf_pctl_get_group_name(struct pinctrl_dev *pctldev, unsigned int selector)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->groups[selector].name;
}

static int
dvf_pctl_get_group_pins(struct pinctrl_dev *pctldev, unsigned int selector,
			const unsigned int **pins, unsigned int *npins)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= info->ngroups)
		return -EINVAL;

	*pins = info->groups[selector].pins;
	*npins = info->groups[selector].npins;

	return 0;
}

static const inline struct dvf_pctl_group *
dvf_pctl_find_group_by_name(const struct dvf_pinctrl *info, const char *name)
{
	int i;

	for (i = 0; i < info->ngroups; i++) {
		if (!strcmp(info->groups[i].name, name))
			return &info->groups[i];
	}

	return NULL;
}

static int
dvf_pctl_dt_node_to_map(struct pinctrl_dev *pctldev, struct device_node *np,
			struct pinctrl_map **map, unsigned int *num_maps)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	const struct dvf_pctl_group *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	int map_num, i;

	grp = dvf_pctl_find_group_by_name(info, np->name);
	if (!grp) {
		dev_err(info->dev, "unable to find group for node %s\n",
			np->name);
		return -EINVAL;
	}

	map_num = grp->npins + 1;
	new_map = devm_kzalloc(pctldev->dev,
			       sizeof(*new_map) * map_num, GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	parent = of_get_parent(np);
	if (!parent) {
		dev_err(info->dev, "unable to find parent for node %s\n",
			np->name);
		devm_kfree(pctldev->dev, new_map);
		return -EINVAL;
	}

	*map = new_map;
	*num_maps = map_num;
	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = parent->name;
	new_map[0].data.mux.group = np->name;
	of_node_put(parent);

	/* create config map per pin */
	new_map++;
	for (i = 0; i < grp->npins; i++) {
		new_map[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
		new_map[i].data.configs.group_or_pin =
				pin_get_name(pctldev, grp->pins[i]);
		new_map[i].data.configs.configs = &grp->pin_conf[i].config;
		new_map[i].data.configs.num_configs = 1;
	}
	dev_dbg(pctldev->dev, "maps: function %s group %s num %d\n",
		(*map)->data.mux.function, grp->name, map_num);

	return 0;
}

static void
dvf_pctl_dt_free_map(struct pinctrl_dev *pctldev, struct pinctrl_map *map,
		     unsigned int num_maps)
{
	devm_kfree(pctldev->dev, map);
}

static struct pinctrl_ops dvf_pctlops = {
	.get_groups_count	= dvf_pctl_get_groups_count,
	.get_group_pins		= dvf_pctl_get_group_pins,
	.get_group_name		= dvf_pctl_get_group_name,
	.dt_node_to_map		= dvf_pctl_dt_node_to_map,
	.dt_free_map		= dvf_pctl_dt_free_map,
};

/* Pinmux */
static int
dvf_pmx_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->nfunctions;
}

static const char *
dvf_pmx_get_fname(struct pinctrl_dev *pctldev, unsigned int selector)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->functions[selector].name;
}

static int
dvf_pmx_get_groups(struct pinctrl_dev *pctldev, unsigned int selector,
		   const char * const **grps, unsigned * const ngrps)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	*grps = info->functions[selector].groups;
	*ngrps = info->functions[selector].ngroups;

	return 0;
}

static void
pad_configure(struct dvf_pinctrl *info, int pad, int value)
{
	if (pad < 64)
		regmap_update_bits(info->regmap, SPCR1 + 4*(pad/32),
				   1 << (pad % 32), (!!value) << (pad % 32));
	else if (pad == 64)
		regmap_update_bits(info->regmap, GCR2, 1, !!value);
}

static int
dvf_pmx_set_mux(struct pinctrl_dev *pctldev, unsigned int fselector,
		unsigned int group_selector)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	struct dvf_pinconf *conf = info->groups[group_selector].pin_conf;
	struct dvf_padconf *pad = info->groups[group_selector].pad_conf;
	int i;

	for (i = 0; i < info->groups[group_selector].npins; i++) {
		dev_dbg(pctldev->dev,
			"GPIO disable %d: pin %d (%cGPIO%d), func %d\n", i,
			conf[i].pin, (conf[i].pin/32) + 'A', conf[i].pin % 32,
			conf[i].altfunc);
		if (conf[i].config & OF_GPIO_SET_PULL)
			dvf_gpiolib_set_pull(info->gpio, conf[i].pin,
					!!(conf[i].config & OF_GPIO_PULL_CTRL),
					!!(conf[i].config & OF_GPIO_PULL_TYPE));
		dvf_gpiolib_set_enable(info->gpio, conf[i].pin, 0);
	}
	for (i = 0; i < info->groups[group_selector].npads; i++) {
		dev_dbg(pctldev->dev, "PAD configure %d: pad %d, value %d\n",
			i, pad[i].pad, pad[i].value);
		pad_configure(info, pad[i].pad, pad[i].value);
	}

	return 0;
}

static struct pinmux_ops dvf_pmxops = {
	.get_functions_count	= dvf_pmx_get_funcs_count,
	.get_function_name	= dvf_pmx_get_fname,
	.get_function_groups	= dvf_pmx_get_groups,
	.set_mux		= dvf_pmx_set_mux,
};

/* Pinconf */
static int
dvf_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin_id,
		unsigned long *configs, unsigned int num_configs)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	int i;

	for (i = 0; i < num_configs; i++) {
		if (configs[i] & OF_GPIO_SET_PULL)
			dvf_gpiolib_set_pull(info->gpio, pin_id,
					!!(configs[i] & OF_GPIO_PULL_CTRL),
					!!(configs[i] & OF_GPIO_PULL_TYPE));
	}

	return 0;
}

static int
dvf_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin_id,
		unsigned long *config)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	*config = 0;

	if (dvf_gpiolib_get_pull_enable(info->gpio, pin_id))
		*config |= OF_GPIO_PULL_CTRL;
	if (dvf_gpiolib_get_pull_selection(info->gpio, pin_id))
		*config |= OF_GPIO_PULL_TYPE;

	return 0;
}

static void
dvf_pinconf_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
		     unsigned int pin_id)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	unsigned long config;

	dvf_pinconf_get(pctldev, pin_id, &config);

	seq_printf(s, " %s, value %d, pull ctrl %s, pull %s\n",
		dvf_gpiolib_get_direction(info->gpio, pin_id) ?
		"input" : "output", dvf_gpiolib_get_value(info->gpio, pin_id),
		!!(config & OF_GPIO_PULL_CTRL) ? "enabled" : "disabled",
		!!(config & OF_GPIO_PULL_TYPE) ? "up" : "down");
}

static struct pinconf_ops dvf_confops = {
	.pin_config_get		= dvf_pinconf_get,
	.pin_config_set		= dvf_pinconf_set,
	.pin_config_dbg_show	= dvf_pinconf_dbg_show,
};

static int
dvf_pinctrl_dt_parse_groups(struct device_node *np,
			    struct dvf_pctl_group *grp,
			    struct dvf_pinctrl *info, int idx)
{
	const __be32 *list;
	struct property *pp;
	struct dvf_pinconf *conf;
	struct dvf_padconf *pad;
	struct device_node *pins;
	int i = 0, j = 0, k, npins = 0, npads = 0, nr_props;

	pins = of_get_child_by_name(np, "dvf,pins");
	if (!pins)
		return -ENODATA;

	for_each_property_of_node(pins, pp) {
		/* Skip those we do not want to proceed */
		if (!strcmp(pp->name, "name"))
			continue;
		if (!strcmp(pp->name, "pad")) {
			npads += (pp->length/sizeof(__be32)) / 2;
			continue;
		}

		if (pp && (pp->length/sizeof(__be32)) == 3) {
			npins++;
		} else {
			pr_warn("Invalid dvf,pins in %s node 0x%p %d\n",
				np->name, pp, pp->length/sizeof(__be32));
			return -EINVAL;
		}
	}

	grp->npins = npins;
	grp->npads = npads;
	grp->name = np->name;
	grp->pins = devm_kzalloc(info->dev, npins * sizeof(u32), GFP_KERNEL);
	grp->pin_conf = devm_kzalloc(info->dev,
					npins * sizeof(*conf), GFP_KERNEL);
	grp->pad_conf = devm_kzalloc(info->dev,
					npads * sizeof(*pad), GFP_KERNEL);

	if (!grp->pins || !grp->pin_conf || !grp->pad_conf)
		return -ENOMEM;

	for_each_property_of_node(pins, pp) {
		if (!strcmp(pp->name, "name"))
			continue;
		if (!strcmp(pp->name, "pad")) {
			nr_props = pp->length/sizeof(u32);
			list = pp->value;
			for (k = 0; k < nr_props/2; k++) {
				pad = &grp->pad_conf[j];
				pad->pad = be32_to_cpup(list++);
				pad->value = be32_to_cpup(list++);
				j++;
			}
			continue;
		}
		nr_props = pp->length/sizeof(u32);
		list = pp->value;
		conf = &grp->pin_conf[i];

		conf->pin = be32_to_cpup(list++) * 32;
		conf->pin += be32_to_cpup(list++);
		conf->config = be32_to_cpup(list++);
		conf->name = pp->name;
		grp->pins[i] = conf->pin;
		i++;
	}
	of_node_put(pins);

	return 0;
}

static int
dvf_pinctrl_parse_functions(struct device_node *np, struct dvf_pinctrl *info,
			    u32 index, int *grp_index)
{
	struct device_node *child;
	struct dvf_pmx_func *func;
	struct dvf_pctl_group *grp;
	int ret, i;

	func = &info->functions[index];
	func->name = np->name;
	func->ngroups = of_get_child_count(np);
	if (func->ngroups <= 0) {
		dev_err(info->dev, "No groups defined\n");
		return -EINVAL;
	}
	func->groups = devm_kzalloc(info->dev,
			func->ngroups * sizeof(char *), GFP_KERNEL);
	if (!func->groups)
		return -ENOMEM;

	i = 0;
	for_each_child_of_node(np, child) {
		func->groups[i] = child->name;
		grp = &info->groups[*grp_index];
		*grp_index += 1;
		ret = dvf_pinctrl_dt_parse_groups(child, grp, info, i++);
		if (ret)
			return ret;
	}
	dev_dbg(info->dev, "Function[%d\t name:%s,\tgroups:%d]\n", index,
		func->name, func->ngroups);

	return 0;
}

static void
dvf_pinctrl_dt_child_count(struct dvf_pinctrl *info, struct device_node *np)
{
	struct device_node *child;

	for_each_child_of_node(np, child) {
		if (of_property_read_bool(child, "gpio-controller")) {
			info->nbanks++;
		} else {
			info->nfunctions++;
			info->ngroups += of_get_child_count(child);
		}
	}
}

static const struct of_device_id dvf_pinctrl_of_match[] = {
	{ .compatible = "dspg,pinctrl-dvf99" },
	{ /* sentinel */ }
};

static int
dvf_pinctrl_probe_dt(struct platform_device *pdev,
		     struct pinctrl_desc *pctl_desc, struct dvf_pinctrl *info)
{
	struct dspg_gpio_pdata *pdata = pdev->dev.platform_data;
	struct dvf_gpio *dvf_gpio;
	struct resource *res;
	int len, extsize = 0;
	int ret = 0;
	int i = 0, j = 0, k = 0;
	struct pinctrl_pin_desc *pdesc;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	int grp_index = 0;

	dvf_gpio = kzalloc(sizeof(*dvf_gpio), GFP_KERNEL);
	if (!dvf_gpio)
		return -ENOMEM;

	info->gpio = dvf_gpio;

	if (!pdata) {
		struct device_node *np = pdev->dev.of_node;

		if (!np) {
			dev_err(&pdev->dev, "missing platform data!\n");
			ret = -EINVAL;
			goto err_free;
		}

		ret = of_property_read_u32(np, "dspg,banks",
			&dvf_gpio->pdata.num_banks);
		if (ret) {
			dev_err(&pdev->dev,
				"missing 'dspg,banks' property!\n");
			goto err_free;
		}
		if (dvf_gpio->pdata.num_banks >
				ARRAY_SIZE(dvf_gpio->pdata.nx_masks)) {
			dev_err(&pdev->dev, "too many banks!\n");
			ret = -EINVAL;
			goto err_free;
		}

		ret = of_property_read_u32_array(np, "dspg,nx_masks",
						 dvf_gpio->pdata.nx_masks,
						 dvf_gpio->pdata.num_banks);
		if (ret && ret != -EINVAL) {
			dev_err(&pdev->dev,
				"invalid 'dspg,nx_masks' property!\n");
			goto err_free;
		}

		dvf_gpio->nr_extints = 16;
		ret = (int)of_find_property(np, "dspg,extints", &extsize);
		if (ret)
			dvf_gpio->nr_extints = extsize / sizeof(u32) / 3;

		for (i = 0; i < dvf_gpio->nr_extints; i++) {
			u32 value1, value2;

			of_property_read_u32_index(np, "dspg,extints", i * 3,
						   &value1);
			of_property_read_u32_index(np, "dspg,extints",
						   i * 3 + 1, &value2);
			dvf_gpio->pdata.extint_gpios[i] = value1 * 32 +
							  value2;

			of_property_read_u32_index(np, "dspg,extints",
						   i * 3 + 2, &value1);
			dvf_gpio->extints[i].bit = value1;
		}
	} else
		dvf_gpio->pdata = *pdata;

	dvf_gpio->mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!dvf_gpio->mem_res) {
		ret = -EINVAL;
		goto err_free;
	}

	for (i = 0; i < dvf_gpio->nr_extints; i++) {
		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!res) {
			dev_err(&pdev->dev, "Missing IRQ resource\n");
			ret = -EINVAL;
			goto err_free;
		}
		dvf_gpio->extints[i].upstream_irq = res->start;
	}

	len = resource_size(dvf_gpio->mem_res);
	if (!request_mem_region(dvf_gpio->mem_res->start, len, "gpio-dspg")) {
		ret = -EBUSY;
		goto err_free;
	}

	dvf_gpio->regs = ioremap(dvf_gpio->mem_res->start, len);
	if (!dvf_gpio->regs) {
		ret = -ENOMEM;
		goto err_release;
	}

	dvf_gpio->chip.parent		= &pdev->dev;
	dvf_gpio->chip.label		= "gpio-dspg";
	dvf_gpio->chip.owner		= THIS_MODULE;
	dvf_gpio->chip.request		= dvf_gpiolib_request;
	dvf_gpio->chip.direction_input	= dvf_gpiolib_direction_input;
	dvf_gpio->chip.get		= dvf_gpiolib_get;
	dvf_gpio->chip.direction_output	= dvf_gpiolib_direction_output;
	dvf_gpio->chip.set		= dvf_gpiolib_set;
	dvf_gpio->chip.dbg_show		= dvf_gpiolib_show;
	dvf_gpio->chip.to_irq		= dvf_gpiolib_to_irq;
	dvf_gpio->chip.ngpio		= dvf_gpio->pdata.num_banks * 32;
#ifdef CONFIG_OF
	dvf_gpio->chip.of_node		= pdev->dev.of_node;
	dvf_gpio->chip.of_gpio_n_cells	= 3;
	dvf_gpio->chip.of_xlate	= dvf_gpiolib_xlate;
#endif

	dvf_gpio->extint_domain = irq_domain_add_linear(pdev->dev.of_node,
		dvf_gpio->nr_extints, &extint_irq_ops, dvf_gpio);
	if (!dvf_gpio->extint_domain) {
		ret = -ENOMEM;
		goto err_unmap;
	}

	for (i = 0; i < dvf_gpio->nr_extints; i++) {
		int irq = dvf_gpio->extints[i].upstream_irq;

		dvf_gpio->extints[i].dvf_gpio = dvf_gpio;
		dvf_gpio->extints[i].irq_domain = dvf_gpio->extint_domain;
		dvf_gpio->extints[i].hwirq = i;
		dvf_gpio->extints[i].flags = EXTINT_FLAG_MASKED;

		irq_set_status_flags(irq, IRQ_NOAUTOEN);
		ret = request_irq(irq, dvf_extint_irq_handler, 0, "gpio-dspg",
			&(dvf_gpio->extints[i]));
		if (ret) {
			dev_err(&pdev->dev,
				"cannot request upstream irq %d!\n", irq);
			goto err_free_irq;
		}
	}

	ret = gpiochip_add(&dvf_gpio->chip);
	if (ret)
		goto err_free_irq;

	platform_set_drvdata(pdev, dvf_gpio);

	dvf_pinctrl_dt_child_count(info, np);
	info->nbanks = dvf_gpio->pdata.num_banks;
	if (!info->nbanks) {
		dev_err(&pdev->dev, "you need atleast one gpio bank\n");
		ret = -EINVAL;
		goto err_free_irq;
	}

	info->functions = devm_kzalloc(&pdev->dev,
		info->nfunctions * sizeof(*info->functions), GFP_KERNEL);

	info->groups = devm_kzalloc(&pdev->dev,
			info->ngroups * sizeof(*info->groups), GFP_KERNEL);

	info->banks = devm_kzalloc(&pdev->dev,
			info->nbanks * sizeof(*info->banks), GFP_KERNEL);

	if (!info->functions || !info->groups || !info->banks) {
		ret = -ENOMEM;
		goto err_free_irq;
	}

	info->regmap = syscon_regmap_lookup_by_phandle(np, "dvf,syscfg");
	if (IS_ERR(info->regmap)) {
		dev_err(info->dev, "No syscfg phandle specified\n");
		ret = PTR_ERR(info->regmap);
		goto err_free_irq;
	}

	pctl_desc->npins = info->nbanks * DVF_GPIO_PINS_PER_BANK;
	pdesc = devm_kzalloc(&pdev->dev,
			sizeof(*pdesc) * pctl_desc->npins, GFP_KERNEL);
	if (!pdesc) {
		ret = -ENOMEM;
		goto err_free_irq;
	}

	pctl_desc->pins = pdesc;

	i = 0;
	for_each_child_of_node(np, child) {
		ret = dvf_pinctrl_parse_functions(child, info, i++,
						  &grp_index);
		if (ret) {
			dev_err(&pdev->dev, "No functions found.\n");
			goto err_free_irq;
		}
	}
	for (i = 0; i < info->nbanks; i++) {
		for (j = 0; j < DVF_GPIO_PINS_PER_BANK; j++, k++) {
			pdesc->number = (i * DVF_GPIO_PINS_PER_BANK) + j;
			pdesc->name = kasprintf(GFP_KERNEL, "%c[%d]",
						'A' + i, j);
			pdesc++;
		}
	}

	return 0;

err_free_irq:
	for (i--; i >= 0; i--)
		free_irq(dvf_gpio->extints[i].upstream_irq,
			 &(dvf_gpio->extints[i]));
err_unmap:
	iounmap(dvf_gpio->regs);
err_release:
	release_mem_region(dvf_gpio->mem_res->start, len);
err_free:
	kfree(dvf_gpio);
	return ret;
}

static int
dvf_pinctrl_probe(struct platform_device *pdev)
{
	struct dvf_pinctrl *info;
	struct pinctrl_desc *pctl_desc;
	int ret = 0;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "device node not found.\n");
		return -EINVAL;
	}

	pctl_desc = devm_kzalloc(&pdev->dev, sizeof(*pctl_desc), GFP_KERNEL);
	if (!pctl_desc)
		return -ENOMEM;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		ret = -ENOMEM;
		goto err_free;
	}

	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);
	ret = dvf_pinctrl_probe_dt(pdev, pctl_desc, info);
	if (ret) {
		dev_err(&pdev->dev, "devicetree parsing failed\n");
		goto err_free;
	}

	pctl_desc->owner	= THIS_MODULE;
	pctl_desc->pctlops	= &dvf_pctlops;
	pctl_desc->pmxops	= &dvf_pmxops;
	pctl_desc->confops	= &dvf_confops;
	pctl_desc->name		= dev_name(&pdev->dev);

	info->pctl = pinctrl_register(pctl_desc, &pdev->dev, info);
	if (!info->pctl) {
		dev_err(&pdev->dev, "Failed pinctrl registration\n");
		ret = -EINVAL;
		goto err_free;
	}

	dev_info(&pdev->dev, "successfully registered\n");

err_free:
	return ret;
}

static struct platform_driver dvf_pinctrl_driver = {
	.driver = {
		.name = "dvf99-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = dvf_pinctrl_of_match,
	},
	.probe = dvf_pinctrl_probe,
};

static int __init dvf_pinctrl_init(void)
{
	return platform_driver_register(&dvf_pinctrl_driver);
}
arch_initcall(dvf_pinctrl_init);
