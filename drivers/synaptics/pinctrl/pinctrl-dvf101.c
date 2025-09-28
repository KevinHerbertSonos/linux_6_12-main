/*
 * Core driver for the DVF101 pin controller
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
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/pinctrl-dvf.h>
#include "core.h"

#define DVF_GPIO_PINS_PER_BANK 32
#define DVF_EOT -1337

#define OF_GPIO_SET_PULL	(1 << 1) /* 0 to leave it unchanged */
#define OF_GPIO_PULL_CTRL	(1 << 2)
#define OF_GPIO_PULL_TYPE	(1 << 3)
#define OF_GPIO_SET_SLEW	(1 << 4) /* 0 to leave it unchanged */
#define OF_GPIO_SLEW_TYPE	(1 << 5)

struct dvf_pinconf {
	int		pin;
	int		reg;
	int		shift;
	int		altfunc;
	const char	*name;
	unsigned long	config;
};

struct dvf_pmx_func {
	const char	*name;
	const char	**groups;
	unsigned	ngroups;
};

struct dvf_pctl_group {
	const char		*name;
	unsigned int		*pins;
	unsigned		npins;
	struct dvf_pinconf	*pin_conf;
};

struct dvf_pinctrl {
	struct device			*dev;
	struct pinctrl_dev		*pctl;
	int				nbanks;
	struct dvf_pmx_func		*functions;
	int				nfunctions;
	struct dvf_pctl_group		*groups;
	int				ngroups;
	void __iomem			*irqmux_base;
	struct dspg_gpio		*gpio;
	struct clk			*clk;
	struct reset_control		*reset;
};

#define IOM1  0x200
#define IOM2  0x204
#define IOM3  0x208
#define IOM4  0x20C
#define IOM5  0x210
#define IOM6  0x214
#define IOM7  0x218
#define IOM8  0x21C
#define IOM9  0x220
#define IOM10 0x224
#define IOM11 0x228

#define IOREG1 0x0
#define IOREG2 0x4
#define IOREG3 0x8
#define IOREG4 0xC
#define IOREG5 0x10
#define IOREG6 0x14

struct pinctrl_config {
	unsigned int *reg_offs;
	struct dspg_io *dspg_ios;
};

enum {
	IODS_OFFSET = 0,
	IOSR_OFFSET,
	IOKE_OFFSET,
	IOPE_OFFSET,
	IOPV_OFFSET,
	IOIE_OFFSET,
};

static unsigned int *reg_offs;

static unsigned int reg_offs_97[] = {
	[IODS_OFFSET] = 0x21C,
	[IOSR_OFFSET] = 0x238,
	[IOKE_OFFSET] = 0x248,
	[IOPE_OFFSET] = 0x258,
	[IOPV_OFFSET] = 0x268,
	[IOIE_OFFSET] = 0x278,
};

static unsigned int reg_offs_101[] = {
	[IODS_OFFSET] = 0x22C,
	[IOSR_OFFSET] = 0x258,
	[IOKE_OFFSET] = 0x270,
	[IOPE_OFFSET] = 0x288,
	[IOPV_OFFSET] = 0x2A0,
	[IOIE_OFFSET] = 0x2B8,
};

struct dspg_io {
	int reg_mux;
	int shift;
	int value;
	int reg_io;
	int bit;
};

static struct dspg_io *dspg_ios;
static int dspg_ios_length;

static struct dspg_io dspg_ios_97[] = {
	/* GPIO (NX: 31) */
	{ IOM5, 20, 1, IOREG3, 26 },
	{ IOM5, 18, 0, IOREG3, 25 },
	{ IOM5, 16, 0, IOREG3, 24 },
	{ IOM4,  4, 2, IOREG2,  2 },
	{ IOM3,  0, 0, IOREG2, 16 },
	{ IOM6,  6, 2, IOREG3,  3 },
	{ IOM6,  8, 1, IOREG3,  4 },
	{ IOM5, 28, 1, IOREG3, 30 },
	{ IOM5, 22, 1, IOREG3, 27 },
	{ IOM4,  0, 1, IOREG2,  0 },
	{ IOM4,  2, 1, IOREG2,  1 },
	{ IOM4, 20, 3, IOREG2, 10 },
	{ IOM4, 18, 2, IOREG2,  9 },
	{ IOM4, 16, 2, IOREG2,  8 },
	{ IOM4, 14, 2, IOREG2,  7 },
	{ IOM4, 12, 2, IOREG2,  6 },
	{ IOM4, 10, 2, IOREG2,  5 },
	{ IOM4,  8, 2, IOREG2,  4 },
	{ IOM4,  6, 2, IOREG2,  3 },
	{ IOM5,  6, 1, IOREG3, 19 },
	{ IOM5,  4, 1, IOREG3, 18 },
	{ IOM5,  2, 1, IOREG3, 17 },
	{ IOM5,  0, 1, IOREG3, 16 },
	{ IOM4, 28, 0, IOREG2, 14 },
	{ IOM4, 26, 0, IOREG2, 13 },
	{ IOM4, 24, 0, IOREG2, 12 },
	{ IOM4, 22, 3, IOREG2, 11 },
	{ IOM5, 14, 2, IOREG3, 23 },
	{ IOM5, 12, 2, IOREG3, 22 },
	{ IOM5, 10, 1, IOREG3, 21 },
	{ IOM5,  8, 2, IOREG3, 20 },
	{   -1, -1, 0,     -1, -1 },
	{ DVF_EOT },
};

static struct dspg_io dspg_ios_101[] = {
	/* AGPIO (NX: 24-25) */
	{ IOM4,   0, 0, IOREG2,  0 },
	{ IOM5,  30, 0, IOREG3, 31 },
	{ IOM5,  28, 0, IOREG3, 30 },
	{ IOM5,  26, 0, IOREG3, 29 },
	{ IOM5,  24, 0, IOREG3, 28 },
	{ IOM5,  22, 0, IOREG3, 27 },
	{ IOM5,  20, 0, IOREG3, 26 },
	{ IOM5,  18, 0, IOREG3, 25 },
	{ IOM5,  16, 0, IOREG3, 24 },
	{ IOM5,  14, 0, IOREG3, 23 },
	{ IOM5,  12, 0, IOREG3, 22 },
	{ IOM5,  10, 0, IOREG3, 21 },
	{ IOM5,   8, 0, IOREG3, 20 },
	{ IOM5,   6, 0, IOREG3, 19 },
	{ IOM8,  30, 1, IOREG4, 15 },
	{ IOM8,  28, 1, IOREG4, 14 },
	{ IOM5,   4, 0, IOREG3, 18 },
	{ IOM5,   2, 0, IOREG3, 17 },
	{ IOM5,   0, 0, IOREG3, 16 },
	{ IOM6,  30, 0, IOREG3, 15 },
	{ IOM10,  0, 0, IOREG5,  0 },
	{ IOM11, 30, 0, IOREG6, 31 },
	{ IOM11, 28, 0, IOREG6, 30 },
	{ IOM11, 26, 0, IOREG6, 29 },
	{    -1, -1, 0,     -1, -1 },
	{    -1, -1, 0,     -1, -1 },
	{ IOM6,  28, 0, IOREG3, 14 },
	{ IOM6,  26, 0, IOREG3, 13 },
	{ IOM6,  24, 0, IOREG3, 12 },
	{ IOM6,  22, 0, IOREG3, 11 },
	{ IOM8,  26, 1, IOREG4, 13 },
	{ IOM8,  24, 1, IOREG4, 12 },

	/* BGPIO (NX: 15) */
	{ IOM10, 22, 0, IOREG5, 11 },
	{ IOM10, 20, 0, IOREG5, 10 },
	{ IOM10, 18, 0, IOREG5,  9 },
	{ IOM10, 16, 0, IOREG5,  8 },
	{ IOM8,  22, 1, IOREG4, 11 },
	{ IOM8,  18, 1, IOREG4,  9 },
	{ IOM8,  20, 1, IOREG4, 10 },
	{ IOM8,  16, 1, IOREG4,  8 },
	{ IOM10, 14, 0, IOREG5,  7 },
	{ IOM10, 12, 0, IOREG5,  6 },
	{ IOM10, 10, 0, IOREG5,  5 },
	{ IOM9,   4, 0, IOREG5, 18 },
	{ IOM10,  8, 0, IOREG5,  4 },
	{ IOM10,  6, 0, IOREG5,  3 },
	{ IOM10,  4, 0, IOREG5,  2 },
	{    -1, -1, 0,     -1, -1 },
	{ IOM10,  2, 0, IOREG5,  1 },
	{ IOM9,  26, 0, IOREG5, 29 },
	{ IOM9,  24, 0, IOREG5, 28 },
	{ IOM9,  22, 0, IOREG5, 27 },
	{ IOM9,  20, 0, IOREG5, 26 },
	{ IOM9,  18, 0, IOREG5, 25 },
	{ IOM9,  16, 0, IOREG5, 24 },
	{ IOM9,  14, 0, IOREG5, 23 },
	{ IOM9,  12, 0, IOREG5, 22 },
	{ IOM9,  10, 0, IOREG5, 21 },
	{ IOM1,  16, 0, IOREG1, 24 },
	{ IOM7,   0, 1, IOREG4, 16 },
	{ IOM8,  14, 1, IOREG4,  7 },
	{ IOM8,  10, 1, IOREG4,  5 },
	{ IOM8,  12, 1, IOREG4,  6 },
	{ IOM9,   2, 0, IOREG5, 17 },

	/* CGPIO */
	{ IOM2,  28, 0, IOREG1, 14 },
	{ IOM3,  22, 0, IOREG2, 27 },
	{ IOM2,  12, 0, IOREG1,  6 },
	{ IOM3,  12, 0, IOREG2, 22 },
	{ IOM2,   6, 0, IOREG1,  3 },
	{ IOM2,  22, 0, IOREG1, 11 },
	{ IOM3,  26, 0, IOREG2, 29 },
	{ IOM3,  30, 0, IOREG2, 31 },
	{ IOM1,  12, 0, IOREG1, 22 },
	{ IOM1,   4, 0, IOREG1, 18 },
	{ IOM1,   0, 0, IOREG1, 16 },
	{ IOM2,  20, 0, IOREG1, 10 },
	{ IOM3,  18, 0, IOREG2, 25 },
	{ IOM1,  22, 0, IOREG1, 27 },
	{ IOM1,  20, 0, IOREG1, 26 },
	{ IOM2,  16, 0, IOREG1,  8 },
	{ IOM1,  10, 0, IOREG1, 21 },
	{ IOM1,   8, 0, IOREG1, 20 },
	{ IOM1,  26, 0, IOREG1, 29 },
	{ IOM1,  18, 0, IOREG1, 25 },
	{ IOM3,  10, 0, IOREG2, 21 },
	{ IOM2,  18, 0, IOREG1,  9 },
	{ IOM2,   8, 0, IOREG1,  4 },
	{ IOM3,  16, 0, IOREG2, 24 },
	{ IOM1,  14, 0, IOREG1, 23 },
	{ IOM3,  14, 0, IOREG2, 23 },
	{ IOM2,  30, 0, IOREG1, 15 },
	{ IOM1,   2, 0, IOREG1, 17 },
	{ IOM2,   4, 0, IOREG1,  2 },
	{ IOM3,  24, 0, IOREG2, 28 },
	{ IOM2,  24, 0, IOREG1, 12 },
	{ IOM1,  24, 0, IOREG1, 28 },

	/* DGPIO (NX: 27-29) */
	{ IOM1,  30, 0, IOREG1, 31 },
	{ IOM3,  20, 0, IOREG2, 26 },
	{ IOM1,   6, 0, IOREG1, 19 },
	{ IOM2,  14, 0, IOREG1,  7 },
	{ IOM2,  10, 0, IOREG1,  5 },
	{ IOM2,  26, 0, IOREG1, 13 },
	{ IOM1,  28, 0, IOREG1, 30 },
	{ IOM2,   2, 0, IOREG1,  1 },
	{ IOM3,   8, 0, IOREG2, 20 },
	{ IOM3,   6, 0, IOREG2, 19 },
	{ IOM2,   0, 0, IOREG1,  0 },
	{ IOM3,  28, 0, IOREG2, 30 },
	{ IOM7,  18, 0, IOREG4, 25 },
	{ IOM7,  16, 0, IOREG4, 24 },
	{ IOM7,  24, 0, IOREG4, 28 },
	{ IOM7,  14, 0, IOREG4, 23 },
	{ IOM7,  12, 0, IOREG4, 22 },
	{ IOM7,  10, 0, IOREG4, 21 },
	{ IOM7,  22, 0, IOREG4, 27 },
	{ IOM7,  20, 0, IOREG4, 26 },
	{ IOM8,   4, 0, IOREG4,  2 },
	{ IOM8,   2, 0, IOREG4,  1 },
	{ IOM8,   0, 0, IOREG4,  0 },
	{ IOM9,  30, 0, IOREG5, 31 },
	{ IOM9,   8, 0, IOREG5, 20 },
	{ IOM9,   6, 0, IOREG5, 19 },
	{ IOM9,  28, 0, IOREG5, 30 },
	{   -1,  -1, 0,     -1, -1 },
	{   -1,  -1, 0,     -1, -1 },
	{   -1,  -1, 0,     -1, -1 },
	{ IOM7,   8, 0, IOREG4, 20 },
	{ IOM7,   6, 0, IOREG4, 19 },

	/* EGPIO */
	{ IOM3,   4, 0, IOREG2, 18 },
	{ IOM3,   2, 0, IOREG2, 17 },
	{ IOM3,   0, 0, IOREG2, 16 },
	{ IOM4,  30, 0, IOREG2, 15 },
	{ IOM4,  28, 0, IOREG2, 14 },
	{ IOM4,  26, 0, IOREG2, 13 },
	{ IOM4,  24, 0, IOREG2, 12 },
	{ IOM4,  22, 0, IOREG2, 11 },
	{ IOM4,  20, 0, IOREG2, 10 },
	{ IOM4,  18, 0, IOREG2,  9 },
	{ IOM4,  16, 0, IOREG2,  8 },
	{ IOM4,  14, 0, IOREG2,  7 },
	{ IOM4,  12, 0, IOREG2,  6 },
	{ IOM4,  10, 0, IOREG2,  5 },
	{ IOM4,   8, 0, IOREG2,  4 },
	{ IOM4,   6, 0, IOREG2,  3 },
	{ IOM4,   4, 0, IOREG2,  2 },
	{ IOM4,   2, 0, IOREG2,  1 },
	{ IOM6,  20, 0, IOREG3, 10 },
	{ IOM6,  18, 0, IOREG3,  9 },
	{ IOM6,  16, 0, IOREG3,  8 },
	{ IOM6,  14, 0, IOREG3,  7 },
	{ IOM6,  12, 0, IOREG3,  6 },
	{ IOM6,  10, 0, IOREG3,  5 },
	{ IOM6,   8, 0, IOREG3,  4 },
	{ IOM6,   6, 0, IOREG3,  3 },
	{ IOM6,   4, 0, IOREG3,  2 },
	{ IOM6,   2, 0, IOREG3,  1 },
	{ IOM6,   0, 0, IOREG3,  0 },
	{ IOM7,  30, 0, IOREG4, 31 },
	{ IOM7,  28, 0, IOREG4, 30 },
	{ IOM7,  26, 0, IOREG3, 29 },
	{ DVF_EOT },
};

static struct pinctrl_config dvf97_config = {
	&reg_offs_97[0], &dspg_ios_97[0]
};
static struct pinctrl_config dvf101_config = {
	&reg_offs_101[0], &dspg_ios_101[0]
};

#define GET_REG(priv, reg, gpio) ( \
	__raw_readl((priv)->regs + (__BANK(gpio) * 0x60) + (reg)) & \
		    (1 << __PIN(gpio)) \
	)

#define SET_REG(priv, reg, gpio) \
	__raw_writel(1 << __PIN(gpio), \
			(priv)->regs + (__BANK(gpio) * 0x60) + (reg))

static int dspg_get_pin(int reg_mux, int shift)
{
	int i;

	for (i = 0; i < dspg_ios_length; i++) {
		if ((((dspg_ios[i].reg_mux - IOM1) >> 2) == reg_mux) &&
		    (dspg_ios[i].shift == shift))
			return i;
	}

	return -1;
}

static int gpio_get_pull_enable(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	int ret, tmp;

	if (dspg_ios[gpio].reg_io == -1)
		return -ENODEV;
	ret = regmap_read(dspg_gpio->regmap,
			  dspg_ios[gpio].reg_io + reg_offs[IOPE_OFFSET], &tmp);
	if (ret < 0)
		return ret;
	if (tmp & (1 << dspg_ios[gpio].bit))
		return 1;

	return 0;
}

static int gpio_get_slew_type(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	int ret, tmp;

	if (dspg_ios[gpio].reg_io == -1)
		return -ENODEV;
	ret = regmap_read(dspg_gpio->regmap,
			  dspg_ios[gpio].reg_io + reg_offs[IOSR_OFFSET], &tmp);
	if (ret < 0)
		return ret;

	return !!(tmp & (1 << dspg_ios[gpio].bit));
}

static void gpio_set_pull(struct dspg_gpio *dspg_gpio, unsigned gpio,
			  int enable, int value)
{
	if (dspg_ios[gpio].reg_io == -1)
		return;
	regmap_update_bits(dspg_gpio->regmap,
			   dspg_ios[gpio].reg_io + reg_offs[IOPE_OFFSET],
			   1 << dspg_ios[gpio].bit,
			   !!enable << dspg_ios[gpio].bit);
	regmap_update_bits(dspg_gpio->regmap,
			   dspg_ios[gpio].reg_io + reg_offs[IOPV_OFFSET],
			   1 << dspg_ios[gpio].bit,
			   !!value << dspg_ios[gpio].bit);
}

static void gpio_set_slew(struct dspg_gpio *dspg_gpio, unsigned gpio,
			  int type)
{
	if (dspg_ios[gpio].reg_io == -1)
		return;
	regmap_update_bits(dspg_gpio->regmap,
			   dspg_ios[gpio].reg_io + reg_offs[IOSR_OFFSET],
			   1 << dspg_ios[gpio].bit,
			   !!type << dspg_ios[gpio].bit);
}

/*
 * Returns: 0: Output, 1: Input
 */
static int gpio_get_direction(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	return !!GET_REG(dspg_gpio, XGP_DIR, gpio);
}

static int dspg_gpio_get_value(struct dspg_gpio *dspg_gpio, unsigned int gpio)
{
	return !!GET_REG(dspg_gpio, XGP_IN, gpio);
}

static void dspg_gpio_set_value(struct dspg_gpio *dspg_gpio, unsigned gpio,
				int value)
{
	if (value)
		SET_REG(dspg_gpio, XGP_DATA_SET, gpio);
	else
		SET_REG(dspg_gpio, XGP_DATA_CLR, gpio);
}

static void gpio_enable(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	int ret;

	if (dspg_ios[gpio].reg_mux == -1)
		return;
	ret = regmap_update_bits(dspg_gpio->regmap,
				 dspg_ios[gpio].reg_mux,
				 3 << dspg_ios[gpio].shift,
				 dspg_ios[gpio].value << dspg_ios[gpio].shift);
	dev_dbg(dspg_gpio->chip.parent,
		"pinctrl enable reg: %8.8X, shift: 0x%8.8X, value: 0x%8.8X\n",
		dspg_ios[gpio].reg_mux, 3 << dspg_ios[gpio].shift,
		dspg_ios[gpio].value << dspg_ios[gpio].shift);
}

static int gpio_get_opendrain(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	return !!GET_REG(dspg_gpio, XGP_OD, gpio);
}

static int gpio_get_keeper(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	int ret, tmp;

	if (dspg_ios[gpio].reg_io == -1)
		return 0;
	ret = regmap_read(dspg_gpio->regmap,
			  dspg_ios[gpio].reg_io + reg_offs[IOKE_OFFSET], &tmp);
	if (ret < 0)
		return ret;
	if (tmp & (1 << dspg_ios[gpio].bit))
		return 1;

	return 0;
}

/*
 * Returns: 0: pull down, 1: pull up
 */
static int gpio_get_pull_selection(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	int ret, tmp;

	if (dspg_ios[gpio].reg_io == -1)
		return 0;
	ret = regmap_read(dspg_gpio->regmap,
			  dspg_ios[gpio].reg_io + reg_offs[IOPV_OFFSET], &tmp);
	if (ret < 0)
		return ret;
	if (tmp & (1 << dspg_ios[gpio].bit))
		return 1;

	return 0;
}

/*
 * Tests whether the Pull-Up/Dn internal resistor can be disconnected, in order
 * to save power.
 */
static void tst_pull_disconnect(struct dspg_gpio *dspg_gpio, unsigned gpio)
{
	if (!gpio_get_pull_enable(dspg_gpio, gpio))
		return;

	if (gpio_get_direction(dspg_gpio, gpio) == 0) {
		/* Output gpio */

		/* Note:
		 * "Wired-OR" is achived by enabling "Open-Drain" and
		 * No Pull-Down resistor, but the user may connect
		 * a Pull-Up resistor, if no such External resistor
		 * exists. So "open-drain" must be excluded !
		 */
		if (!gpio_get_opendrain(dspg_gpio, gpio))
			gpio_set_pull(dspg_gpio, gpio, 0, 0);
	} else {
		/* Input gpio */

		if (gpio_get_keeper(dspg_gpio, gpio))
			gpio_set_pull(dspg_gpio, gpio, 0, 0);
	}
}

static int
dspg_gpiolib_request(struct gpio_chip *chip, unsigned offset)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);

	if (dspg_gpio->pdata.nx_masks[__BANK(offset)] & (1 << __PIN(offset))) {
		dev_warn(chip->parent,
			 "request for non-existent %cGPIO%-2d denied!\n",
			 'A' + __BANK(offset), __PIN(offset));
		return -ENODEV;
	}

	return 0;
}

static int
dspg_gpiolib_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);

	SET_REG(dspg_gpio, XGP_DIR_IN, offset);
	tst_pull_disconnect(dspg_gpio, offset);

	return 0;
}

static int
dspg_gpiolib_direction_output(struct gpio_chip *chip, unsigned offset,
			      int value)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);

	dspg_gpio_set_value(dspg_gpio, offset, value);
	SET_REG(dspg_gpio, XGP_DIR_OUT, offset);
	tst_pull_disconnect(dspg_gpio, offset);

	gpio_enable(dspg_gpio, offset);

	return 0;
}

int
dspg_gpiolib_get(struct gpio_chip *chip, unsigned offset)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);

	return !!GET_REG(dspg_gpio, XGP_IN, offset);
}

static void
dspg_gpiolib_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);

	dspg_gpio_set_value(dspg_gpio, offset, value);
}

static void
dspg_gpiolib_show(struct seq_file *s, struct gpio_chip *chip)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);
	const char *label;
	unsigned i;

	for (i = 0; i < chip->ngpio; i++) {
		label = gpiochip_is_requested(chip, i);
		if (!label)
			continue;

		seq_printf(s, " %cGPIO%-2d (%-20.20s) %s %s%s",
			'A' + __BANK(i), __PIN(i), label,
			gpio_get_direction(dspg_gpio, i) ? "in " : "out",
			dspg_gpio_get_value(dspg_gpio, i) ? "hi" : "lo",
			gpio_get_pull_enable(dspg_gpio, i)
				? (gpio_get_pull_selection(dspg_gpio, i) ?
				" pu" : " pd") : ""
			);
		seq_puts(s, "\n");
	}
}

static int dspg_gpiolib_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct dspg_gpio *dspg_gpio =
				container_of(chip, struct dspg_gpio, chip);
	int i;

	/* search for matching GPIO and then map through existing IRQ domain */
	for (i = 0; i < dspg_gpio->nr_extints; i++) {
		if ((dspg_gpio->pdata.extint_gpios[i] == offset ||
		     dspg_gpio->pdata.extint_alt_gpios[i] == offset) &&
		    dspg_gpio->extints[i].irq_domain)
			return irq_create_mapping(
					dspg_gpio->extints[i].irq_domain, 0);
	}

	return -ENODEV;
}

#ifdef CONFIG_OF
static int dspg_gpiolib_xlate(struct gpio_chip *gc,
			      const struct of_phandle_args *gpiospec,
			      u32 *flags)
{
	struct dspg_gpio *dspg_gpio = container_of(gc, struct dspg_gpio, chip);
	unsigned pin;

	pin = gpiospec->args[0] * 32 + gpiospec->args[1];

	if (pin >= gc->ngpio || gpiospec->args[1] > 32)
		return -EINVAL;

	if (dspg_gpio->pdata.nx_masks[__BANK(pin)] & (1 << __PIN(pin)))
		return -ENODEV;

	if (flags)
		*flags = gpiospec->args[2];

	return pin;
}
#endif

/* Pinctrl Groups */
static int
dvf_pctl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->ngroups;
}

static const char *
dvf_pctl_get_group_name(struct pinctrl_dev *pctldev, unsigned selector)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->groups[selector].name;
}

static int
dvf_pctl_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector,
			const unsigned **pins, unsigned *npins)
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
			struct pinctrl_map **map, unsigned *num_maps)
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
		     unsigned num_maps)
{
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
dvf_pmx_get_fname(struct pinctrl_dev *pctldev, unsigned selector)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->functions[selector].name;
}

static int
dvf_pmx_get_groups(struct pinctrl_dev *pctldev, unsigned selector,
		   const char * const **grps, unsigned * const ngrps)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	*grps = info->functions[selector].groups;
	*ngrps = info->functions[selector].ngroups;

	return 0;
}

static int
dvf_pmx_set_mux(struct pinctrl_dev *pctldev, unsigned func_selector,
		unsigned group_selector)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	struct dvf_pinconf *conf = info->groups[group_selector].pin_conf;
	int i, ret;

	for (i = 0; i < info->groups[group_selector].npins; i++) {
		dev_dbg(pctldev->dev,
			"%d: pin %s: reg: 0x%8.8X, shift: 0x%8.8X, func: 0x%8.8X\n",
			i, conf[i].name, IOM1 + (conf[i].reg * 4),
			3 << conf[i].shift, conf[i].altfunc << conf[i].shift);
		ret = regmap_update_bits(info->gpio->regmap,
				 IOM1 + (conf[i].reg * 4), 3 << conf[i].shift,
				 conf[i].altfunc << conf[i].shift);

		if (conf[i].config & OF_GPIO_SET_PULL)
			gpio_set_pull(info->gpio, conf[i].pin,
				      conf[i].config & OF_GPIO_PULL_CTRL,
				      conf[i].config & OF_GPIO_PULL_TYPE);
		if (conf[i].config & OF_GPIO_SET_SLEW)
			gpio_set_slew(info->gpio, conf[i].pin,
				      conf[i].config & OF_GPIO_SLEW_TYPE);

		if (ret < 0)
			return ret;
	}

	return 0;
}

static struct pinmux_ops dvf_pmxops = {
	.get_functions_count	= dvf_pmx_get_funcs_count,
	.get_function_name	= dvf_pmx_get_fname,
	.get_function_groups	= dvf_pmx_get_groups,
	.set_mux		= dvf_pmx_set_mux,
};

static int
dvf_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin_id,
		unsigned long *configs, unsigned num_configs)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	int i;

	for (i = 0; i < num_configs; i++) {
		if (configs[i] & OF_GPIO_SET_PULL)
			gpio_set_pull(info->gpio, pin_id,
				      configs[i] & OF_GPIO_PULL_CTRL,
				      configs[i] & OF_GPIO_PULL_TYPE);
		if (configs[i] & OF_GPIO_SET_SLEW)
			gpio_set_slew(info->gpio, pin_id,
				      configs[i] & OF_GPIO_SLEW_TYPE);
	}

	return 0;
}

static int
dvf_pinconf_get(struct pinctrl_dev *pctldev, unsigned pin_id,
		unsigned long *config)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	*config = 0;

	if (gpio_get_pull_enable(info->gpio, pin_id))
		*config |= OF_GPIO_PULL_CTRL;
	if (gpio_get_pull_selection(info->gpio, pin_id))
		*config |= OF_GPIO_PULL_TYPE;
	if (gpio_get_slew_type(info->gpio, pin_id))
		*config |= OF_GPIO_SLEW_TYPE;

	return 0;
}

static void
dvf_pinconf_dbg_show(struct pinctrl_dev *pctldev,  struct seq_file *s,
		     unsigned pin_id)
{
	struct dvf_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	unsigned long config;

	dvf_pinconf_get(pctldev, pin_id, &config);

	seq_printf(s, " %s, value %d, pull ctrl %s, pull %s, %s slew\n",
		gpio_get_direction(info->gpio, pin_id) ?
		"input" : "output", dspg_gpio_get_value(info->gpio, pin_id),
		!!(config & OF_GPIO_PULL_CTRL) ? "enabled" : "disabled",
		!!(config & OF_GPIO_PULL_TYPE) ? "up" : "down",
		!!(config & OF_GPIO_SLEW_TYPE) ? "fast" : "slow");
}

static struct pinconf_ops dvf_confops = {
	.pin_config_get		= dvf_pinconf_get,
	.pin_config_set		= dvf_pinconf_set,
	.pin_config_dbg_show	= dvf_pinconf_dbg_show,
};

/*
 * Each pin is represented in of the below forms.
 * <gpio>
 */
static int
dvf_pinctrl_dt_parse_groups(struct device_node *np,
			    struct dvf_pctl_group *grp,
			    struct dvf_pinctrl *info, int idx)
{
	/* bank pad direction val altfunction */
	const __be32 *list;
	struct property *pp;
	struct dvf_pinconf *conf;
	struct device_node *pins;
	int i = 0, npins = 0;

	pins = of_get_child_by_name(np, "dvf,pins");
	if (!pins)
		return -ENODATA;

	for_each_property_of_node(pins, pp) {
		/* Skip those we do not want to process */
		if (!strcmp(pp->name, "name"))
			continue;

		if (pp && (pp->length/sizeof(__be32)) == 4) {
			npins++;
		} else {
			pr_warn("Invalid dvf,pins in %s node 0x%p %d\n",
				np->name, pp, pp->length/sizeof(__be32));
			return -EINVAL;
		}
	}

	grp->npins = npins;
	grp->name = np->name;
	grp->pins = devm_kzalloc(info->dev, npins * sizeof(u32), GFP_KERNEL);
	grp->pin_conf = devm_kzalloc(info->dev,
					npins * sizeof(*conf), GFP_KERNEL);

	if (!grp->pins || !grp->pin_conf)
		return -ENOMEM;

	/* <gpio> */
	for_each_property_of_node(pins, pp) {
		if (!strcmp(pp->name, "name"))
			continue;

		list = pp->value;
		/* there is no IOM0 */
		if (!list) {
			dev_warn(info->dev,
				 "there is no IOM0! skipping function %s.\n",
				 pp->name);
			continue;
		}
		conf = &grp->pin_conf[i];

		conf->reg = be32_to_cpup(list++) - 1;
		conf->shift = be32_to_cpup(list++);
		conf->altfunc = be32_to_cpup(list++);
		conf->config = be32_to_cpup(list++);
		conf->name = pp->name;
		grp->pins[i] = dspg_get_pin(conf->reg, conf->shift);
		conf->pin = grp->pins[i];

		i++;
	}
	of_node_put(pins);

	return 0;
}

static int
dvf_pctl_parse_functions(struct device_node *np, struct dvf_pinctrl *info,
			 u32 index, int *grp_index)
{
	struct device_node *child;
	struct dvf_pmx_func *func;
	struct dvf_pctl_group *grp;
	int ret, i = 0;

	func = &info->functions[index];
	func->name = np->name;
	func->ngroups = of_get_child_count(np);
	if (func->ngroups <= 0) {
		dev_dbg(info->dev, "No groups defined in node %s\n", np->name);
		return -EINVAL;
	}
	func->groups = devm_kzalloc(info->dev,
			func->ngroups * sizeof(char *), GFP_KERNEL);
	if (!func->groups)
		return -ENOMEM;

	for_each_child_of_node(np, child) {
		func->groups[i] = child->name;
		grp = &info->groups[*grp_index];
		*grp_index += 1;
		ret = dvf_pinctrl_dt_parse_groups(child, grp, info, i++);
		if (ret)
			return ret;
	}
	dev_dbg(info->dev, "Function[%d\t name:%s,\tgroups:%d]\n",
		index, func->name, func->ngroups);

	return 0;
}

static void
dvf_pctl_dt_child_count(struct dvf_pinctrl *info, struct device_node *np)
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
	{
		.compatible = "dspg,pinctrl-dvf101",
		.data = &dvf101_config,
	},
	{
		.compatible = "dspg,pinctrl-dvf97",
		.data = &dvf97_config,
	},
	{ /* sentinel */ }
};

static int
dvf_pctl_probe_dt(struct platform_device *pdev,
		  struct pinctrl_desc *pctl_desc, struct dvf_pinctrl *info)
{
	struct dspg_gpio_pdata *pdata = pdev->dev.platform_data;
	struct dspg_gpio *dspg_gpio;
	int len;
	int ret = 0;
	int i = 0, j = 0, k = 0;
	struct pinctrl_pin_desc *pdesc;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	int grp_index = 0;

	dspg_gpio = kzalloc(sizeof(*dspg_gpio), GFP_KERNEL);
	if (!dspg_gpio)
		return -ENOMEM;

	info->gpio = dspg_gpio;

	if (!pdata) {
		struct device_node *np = pdev->dev.of_node;
		struct device_node *child;
		int valid_ind = 0;

		if (!np) {
			dev_err(&pdev->dev, "missing platform data!\n");
			ret = -EINVAL;
			goto err_free;
		}

		ret = of_property_read_u32(np, "dspg,banks",
			&dspg_gpio->pdata.num_banks);
		if (ret) {
			dev_err(&pdev->dev, "missing 'dspg,banks' property!\n");
			goto err_free;
		}
		if (dspg_gpio->pdata.num_banks >
				ARRAY_SIZE(dspg_gpio->pdata.nx_masks)) {
			dev_err(&pdev->dev, "too many banks!\n");
			ret = -EINVAL;
			goto err_free;
		}

		ret = of_property_read_u32_array(np, "dspg,nx_masks",
			dspg_gpio->pdata.nx_masks, dspg_gpio->pdata.num_banks);
		if (ret && ret != -EINVAL) {
			dev_err(&pdev->dev, "invalid 'dspg,nx_masks' property!\n");
			goto err_free;
		}

		np = of_parse_phandle(np, "dvf,extints", 0);
		dspg_gpio->nr_extints = of_get_child_count(np);

		i = 0;
		for_each_child_of_node(np, child) {
			u32 pin[2];

			ret = of_property_read_u32_array(child, "dvf,pin",
							 pin, 2);
			if (ret == -EINVAL) {
				dev_dbg(&pdev->dev,
					"skipping empty extint #%d\n", i);
				goto extint_parse_continue;
			} else if (ret) {
				dev_err(&pdev->dev,
					"could not parse 'dvf,pin' #%d\n", i);
				goto err_free;
			}
			dspg_gpio->pdata.extint_gpios[valid_ind] =
							pin[0] * 32 + pin[1];
			dspg_gpio->extints[valid_ind].bit = i;
			dspg_gpio->extints[valid_ind].dspg_gpio = dspg_gpio;

			if (!of_property_read_u32_array(child, "dvf,alt",
							pin, 2)) {
				dspg_gpio->pdata.extint_alt_gpios[valid_ind] =
							pin[0] * 32 + pin[1];
			}

			/* valid extint found, increase index */
			valid_ind++;
extint_parse_continue:
			i++;
		}
	} else
		dspg_gpio->pdata = *pdata;

	dspg_gpio->mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!dspg_gpio->mem_res) {
		ret = -EINVAL;
		goto err_free;
	}


	len = resource_size(dspg_gpio->mem_res);
	if (!request_mem_region(dspg_gpio->mem_res->start, len, "gpio-dspg")) {
		ret = -EBUSY;
		goto err_free;
	}

	dspg_gpio->regs = devm_ioremap(&pdev->dev, dspg_gpio->mem_res->start,
				       len);
	if (!dspg_gpio->regs) {
		ret = -ENOMEM;
		goto err_release;
	}

	dspg_gpio->chip.parent = &pdev->dev;
	dspg_gpio->chip.label = "gpio-dspg";
	dspg_gpio->chip.owner = THIS_MODULE;
	dspg_gpio->chip.request = dspg_gpiolib_request;
	dspg_gpio->chip.direction_input = dspg_gpiolib_direction_input;
	dspg_gpio->chip.get = dspg_gpiolib_get;
	dspg_gpio->chip.direction_output = dspg_gpiolib_direction_output;
	dspg_gpio->chip.set = dspg_gpiolib_set;
	dspg_gpio->chip.dbg_show = dspg_gpiolib_show;
	dspg_gpio->chip.to_irq = dspg_gpiolib_to_irq;
	dspg_gpio->chip.ngpio =
			dspg_gpio->pdata.num_banks * DVF_GPIO_PINS_PER_BANK;
#ifdef CONFIG_OF
	dspg_gpio->chip.of_node = pdev->dev.of_node;
	dspg_gpio->chip.of_gpio_n_cells = 3;
	dspg_gpio->chip.of_xlate = dspg_gpiolib_xlate;
#endif

	ret = gpiochip_add(&dspg_gpio->chip);
	if (ret)
		goto err_release;

	platform_set_drvdata(pdev, dspg_gpio);

	dvf_pctl_dt_child_count(info, np);
	info->nbanks = dspg_gpio->pdata.num_banks;
	if (!info->nbanks) {
		dev_err(&pdev->dev, "you need atleast one gpio bank\n");
		return -EINVAL;
	}

	dev_dbg(&pdev->dev, "%d banks\n", info->nbanks);
	dev_dbg(&pdev->dev, "%d functions\n", info->nfunctions);
	dev_dbg(&pdev->dev, "%d groups\n", info->ngroups);

	info->functions = devm_kzalloc(&pdev->dev,
		info->nfunctions * sizeof(*info->functions), GFP_KERNEL);

	info->groups = devm_kzalloc(&pdev->dev,
			info->ngroups * sizeof(*info->groups), GFP_KERNEL);

	if (!info->functions || !info->groups)
		return -ENOMEM;

	dspg_gpio->regmap = syscon_regmap_lookup_by_phandle(np, "dvf,syscfg");
	if (IS_ERR(dspg_gpio->regmap)) {
		dev_err(info->dev, "No syscfg phandle specified\n");
		return PTR_ERR(dspg_gpio->regmap);
	}

	pctl_desc->npins = info->nbanks * DVF_GPIO_PINS_PER_BANK;
	pdesc = devm_kzalloc(&pdev->dev,
			sizeof(*pdesc) * pctl_desc->npins, GFP_KERNEL);
	if (!pdesc)
		return -ENOMEM;

	pctl_desc->pins = pdesc;

	i = 0;
	for_each_child_of_node(np, child) {
		dvf_pctl_parse_functions(child, info, i++, &grp_index);
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

err_release:
	release_mem_region(dspg_gpio->mem_res->start, len);
err_free:
	kfree(dspg_gpio);
	return ret;
}

static int
dvf_pinctrl_probe(struct platform_device *pdev)
{
	struct dvf_pinctrl *info;
	struct pinctrl_desc *pctl_desc;
	const struct of_device_id *match;
	struct pinctrl_config *conf;
	int ret;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "device node not found.\n");
		return -EINVAL;
	}

	match = of_match_node(dvf_pinctrl_of_match, pdev->dev.of_node);
	if (!match)
		return -ENODEV;
	conf = (struct pinctrl_config *)match->data;
	reg_offs = conf->reg_offs;
	dspg_ios = conf->dspg_ios;
	while (dspg_ios[dspg_ios_length].reg_mux != DVF_EOT)
		dspg_ios_length++;

	pctl_desc = devm_kzalloc(&pdev->dev, sizeof(*pctl_desc), GFP_KERNEL);
	if (!pctl_desc)
		return -ENOMEM;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->clk)) {
		dev_info(&pdev->dev, "could not get optional clock: %d\n",
			 (int)PTR_ERR(info->clk));
		info->clk = NULL;
	}

	info->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(info->reset)) {
		dev_info(&pdev->dev, "could not get optional reset: %d\n",
			 (int)PTR_ERR(info->reset));
		info->reset = NULL;
	}

	if (info->reset) {
		ret = reset_control_deassert(info->reset);
		if (ret)
			return ret;
	}

	if (info->clk) {
		ret = clk_prepare_enable(info->clk);
		if (ret) {
			dev_err(&pdev->dev, "failed to enable clock\n");
			return ret;
		}
	}

	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);
	ret = dvf_pctl_probe_dt(pdev, pctl_desc, info);
	if (ret)
		return ret;

	pctl_desc->owner	= THIS_MODULE;
	pctl_desc->pctlops	= &dvf_pctlops;
	pctl_desc->pmxops	= &dvf_pmxops;
	pctl_desc->confops	= &dvf_confops;
	pctl_desc->name		= dev_name(&pdev->dev);

	info->pctl = pinctrl_register(pctl_desc, &pdev->dev, info);
	if (!info->pctl) {
		dev_err(&pdev->dev, "pinctrl registration failed\n");
		return -EINVAL;
	}

	ret = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to populate subdevices\n");
		return -EINVAL;
	}

	dev_info(&pdev->dev, "successfully registered DVF pinctrl\n");

	return 0;
}

static struct platform_driver dvf_pinctrl_driver = {
	.driver = {
		.name = "dvf101-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = dvf_pinctrl_of_match,
	},
	.probe = dvf_pinctrl_probe,
};


static int __init dvf_pinctrl_init(void)
{
	return platform_driver_register(&dvf_pinctrl_driver);
}
arch_initcall_sync(dvf_pinctrl_init);
