/*
 * dspg_wdt.c - DSPG watchdog userspace interface
 *
 * Copyright (c) 2016 DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/mfd/syscon.h>
#include <linux/io.h>

#include <dt-bindings/watchdog/dvf-watchdog.h>

#define DSPG_WDT "dspg_wdt"

#define GCU_RST_CTRL          0x0000
#define CMU_RSTSR             0x0048

#define WD_CTRL               0x00
#define WD_ENABLE             0x04
#define WD_REFRESH            0x08
#define WD_SMST               0x0c
#define WD_CVR1               0x10
#define WD_CVR2               0x14
#define WD_CVR3               0x18
#define WD_INTCLR             0x1c
#define WD_INTSTAT            0x20

#define WD_CTRL_PERIOD_1s     (0 << 0)
#define WD_CTRL_PERIOD_10s    (1 << 0)
#define WD_CTRL_PERIOD_100s   (2 << 0)
#define WD_CTRL_PERIOD_1000s  (3 << 0)
#define WD_CTRL_INTEN         (1 << 2)

#define WD_ENABLE_RETRY       10

#define WD_HW_DISABLED        0x00
#define WD_SW_DISABLED        0x01
#define WD_ENABLED            0x02

static unsigned int bootstatus[IND_MAX];
static void __iomem *dspg_wdt_base;
static DEFINE_SPINLOCK(dspg_wdt_lock);
static unsigned long wdt_clk_rate;
static int wdt_clk_active;
static struct clk *wdt_clk;
static int wdt_state;
static int wdt_interrupt;
static int wdt_ignore_interrupt;

static unsigned long
dspg_wdt_read_counter(void)
{
	unsigned long counter = 0;

	counter  = (readw(dspg_wdt_base + WD_CVR3) & 0x1FF) << 16;
	counter |= (readw(dspg_wdt_base + WD_CVR2) & 0xFF) << 8;
	counter |=  readw(dspg_wdt_base + WD_CVR1) & 0xFF;

	return counter;
}

static void
dspg_wdt_refresh(void)
{
	unsigned long flags;
	unsigned long counter;
	unsigned long rcounter;

	raw_spin_lock_irqsave(&dspg_wdt_lock, flags);
	counter = dspg_wdt_read_counter();
	if (counter == 0) {
		/* in this case we loop forever below */
		pr_crit("Watchdog refreshed too fast\n");
		raw_spin_unlock_irqrestore(&dspg_wdt_lock, flags);
		return;
	}
	/* if this loop fails the watchdog will anyway reset the chip */
	do {
		writew(0xa5a5, dspg_wdt_base + WD_REFRESH);
		writew(0x5a5a, dspg_wdt_base + WD_REFRESH);
		rcounter = dspg_wdt_read_counter();
	} while (rcounter >= counter);

	wdt_interrupt = wdt_ignore_interrupt;

	raw_spin_unlock_irqrestore(&dspg_wdt_lock, flags);
}

static int
dspg_wdt_enable(struct watchdog_device *wdt_dev)
{
	unsigned long flags;
	unsigned int val, retry = WD_ENABLE_RETRY;

	if (!wdt_clk_active)
		wdt_clk_active = 1;

	raw_spin_lock_irqsave(&dspg_wdt_lock, flags);
	do {
		writew(0xabcd, dspg_wdt_base + WD_ENABLE);
		writew(0xabcd, dspg_wdt_base + WD_ENABLE);
		retry--;
	} while (((readl(dspg_wdt_base + WD_SMST) & 0x7) != 3) && retry);

	wdt_state = WD_ENABLED;
	val = readw(dspg_wdt_base + WD_CTRL);
	val |= WD_CTRL_INTEN;
	writew(val, dspg_wdt_base + WD_CTRL);

	raw_spin_unlock_irqrestore(&dspg_wdt_lock, flags);

	return (retry == 0 ? -EIO : 0);
}

static int
dspg_wdt_disable(struct watchdog_device *wdt_dev)
{
	wdt_state = WD_SW_DISABLED;

	return 0;
}

static unsigned int
dspg_wdt_status(struct watchdog_device *wdt_dev)
{
	return 0;
}

static int
dspg_wdt_ping(struct watchdog_device *wdt_dev)
{
	dspg_wdt_refresh();

	return 0;
}

static int
dspg_wdt_timeout_in_seconds(unsigned int timeout)
{
	unsigned long seconds;

	seconds = 32768000 / wdt_clk_rate;

	switch (timeout) {
	case WD_CTRL_PERIOD_1s:
		seconds /= 1000;
		break;

	case WD_CTRL_PERIOD_10s:
		seconds /= 100;
		break;

	case WD_CTRL_PERIOD_100s:
		seconds /= 10;
		break;

	case WD_CTRL_PERIOD_1000s:
		break;

	default:
		return -EINVAL;
	}

	if (seconds < 1)
		seconds = 1;

	return seconds;
}

static int
dspg_wdt_seconds_to_timeout(unsigned int seconds)
{
	unsigned long timeout;
	int period = WD_CTRL_PERIOD_1000s;

	timeout = 32768000 / wdt_clk_rate;

	while ((seconds <= timeout/10) && (period > WD_CTRL_PERIOD_1s)) {
		period--;
		timeout /= 10;
	}

	return period;
}

static int
dspg_wdt_get_timeout(void)
{
	int timeout = readw(dspg_wdt_base + WD_CTRL) & 0x3;

	return dspg_wdt_timeout_in_seconds(timeout);
}

static irqreturn_t
dspg_wdt_isr(int irq, void *unused)
{
	struct device *dev = unused;

	if (wdt_state != WD_ENABLED) {
		dspg_wdt_refresh();
	} else {
		if (!wdt_interrupt)
			dspg_wdt_refresh();
		wdt_interrupt = 1;
		dev_crit(dev, "Watchdog interrupt generated (task %s)\n",
			 current->comm);
	}

	writew(0x1, dspg_wdt_base + WD_INTCLR);

	return IRQ_HANDLED;
}

static struct watchdog_info dspg_wdt_info = {
	.options = WDIOF_CARDRESET | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = DSPG_WDT,
};

static struct watchdog_ops dspg_wdt_ops = {
	.owner = THIS_MODULE,
	.start = dspg_wdt_enable,
	.stop = dspg_wdt_disable,
	.ping = dspg_wdt_ping,
	.status = dspg_wdt_status,
};

static struct watchdog_device dspg_wdt_dev = {
	.info = &dspg_wdt_info,
	.ops = &dspg_wdt_ops,
};

static ssize_t state_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%u\n", wdt_state);
}
static DEVICE_ATTR(state, 0444, state_show, NULL);

static int __init
dspg_wdt_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct reset_control *reset;
	int i, irq, ret, period = 1; /* default period is at least 1s */
	unsigned int gcu_rst_ctrl;
	struct resource *res;
	struct regmap *map;

	wdt_interrupt = wdt_ignore_interrupt;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource specified\n");
		return -EINVAL;
	}

	dspg_wdt_base = devm_ioremap_resource(&pdev->dev, res);
	if (!dspg_wdt_base) {
		dev_err(&pdev->dev, "failed to map registers\n");
		return -ENOMEM;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(&pdev->dev, irq, dspg_wdt_isr, 0, DSPG_WDT,
			       &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "error requesting irq\n");
		return ret;
	}

	wdt_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(wdt_clk)) {
		dev_err(&pdev->dev, "failed to get slow clock\n");
		return PTR_ERR(wdt_clk);
	}

	ret = of_property_read_u32(np, "period", &period);
	if ((ret && ret != -EINVAL) || period < 1) {
		dev_err(&pdev->dev, "invalid 'period'\n");
		return -EINVAL;
	}

	wdt_clk_rate = 32768;
	if (period != 1 && period != 10 && period != 100 && period < 1000) {
		unsigned long mult = 1;

		if (period > 100)
			mult = 100;
		else if (period > 10)
			mult = 10;
		wdt_clk_rate = (32768 * 1000 * mult) / period / 100;
	}

	wdt_clk_rate = clk_round_rate(wdt_clk, wdt_clk_rate);
	clk_set_rate(wdt_clk, wdt_clk_rate);
	clk_prepare_enable(wdt_clk);

	reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(reset)) {
		dev_dbg(&pdev->dev, "cannot get reset control\n");
	} else {
		ret = reset_control_deassert(reset);
		if (ret)
			return ret;
	}

	wdt_clk_active = 0;
	wdt_state = WD_HW_DISABLED;
	if ((readw(dspg_wdt_base + WD_SMST) & 0x7) == 0x3) {
		unsigned int val;

		wdt_state = WD_ENABLED;
		val = readw(dspg_wdt_base + WD_CTRL);
		val |= WD_CTRL_INTEN;
		writew(val, dspg_wdt_base + WD_CTRL);
	}

	writew(dspg_wdt_seconds_to_timeout(period), dspg_wdt_base + WD_CTRL);

	wdt_ignore_interrupt = of_property_read_bool(np, "ignore-interrupt");

	dspg_wdt_dev.min_timeout =
		dspg_wdt_timeout_in_seconds(WD_CTRL_PERIOD_1s);
	dspg_wdt_dev.max_timeout =
		dspg_wdt_timeout_in_seconds(WD_CTRL_PERIOD_1000s);
	dspg_wdt_dev.timeout = dspg_wdt_get_timeout();

	for (i = 0; i < IND_MAX * 2; i += 2) {
		u32 index;

		ret = of_property_read_u32_index(np, "bootstatus", i, &index);
		if (ret == -EINVAL || ret == -EOVERFLOW)
			break;
		else if (ret || index >= IND_MAX)
			dev_err(&pdev->dev, "invalid bootstatus @%d\n", i);

		ret = of_property_read_u32_index(np, "bootstatus", i + 1,
						 &bootstatus[index]);
		if (ret == -EINVAL || ret == -EOVERFLOW)
			break;
		else if (ret || index >= IND_MAX)
			dev_err(&pdev->dev, "invalid bootstatus @%d\n", i + 1);
	}

	map = syscon_regmap_lookup_by_phandle(np, "dvf,syscfg");
	if (IS_ERR(map))
		return PTR_ERR(map);

	if (of_machine_is_compatible("dspg,dvf101")) {
		ret = regmap_read(map, GCU_RST_CTRL, &gcu_rst_ctrl);
		if (ret)
			return ret;
		ret = regmap_write(map, GCU_RST_CTRL,
				   1 << 19 | 1 << 17 | 1 << 16);
		if (ret)
			return ret;

		if (gcu_rst_ctrl & BIT(19))
			dspg_wdt_dev.bootstatus = bootstatus[IND_JTAG];
		else if (gcu_rst_ctrl & BIT(17))
			dspg_wdt_dev.bootstatus = bootstatus[IND_SOFTWARE];
		else if (gcu_rst_ctrl & BIT(16))
			dspg_wdt_dev.bootstatus = bootstatus[IND_WATCHDOG];
		else
			dspg_wdt_dev.bootstatus = bootstatus[IND_UNKNOWN];
	} else {
		ret = regmap_read(map, CMU_RSTSR, &gcu_rst_ctrl);
		if (ret)
			return ret;
		ret = regmap_write(map, CMU_RSTSR, 1 << 5 | 1 << 4 | 1 << 3);
		if (ret)
			return ret;

		if (gcu_rst_ctrl & BIT(3))
			dspg_wdt_dev.bootstatus = bootstatus[IND_WATCHDOG];
		else if (gcu_rst_ctrl & (BIT(4) | BIT(5)))
			dspg_wdt_dev.bootstatus = bootstatus[IND_SOFTWARE];
		else
			dspg_wdt_dev.bootstatus = bootstatus[IND_UNKNOWN];
	}

	/* If watchdog is already running, refresh it once. */
	if ((readw(dspg_wdt_base + WD_SMST) & 0x7) == 0x3)
		dspg_wdt_refresh();

	ret = watchdog_register_device(&dspg_wdt_dev);
	if (ret) {
		dev_err(&pdev->dev, "error registering watchdog device: %d\n",
			ret);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_state);
	if (ret < 0)
		dev_warn(&pdev->dev, "failed to add sysfs\n");

	dev_info(&pdev->dev, "initialized, not started, period %d seconds\n",
		 dspg_wdt_dev.timeout);

	return 0;
}

static const struct of_device_id dspg_wdt_of_ids[] = {
	{ .compatible = "dspg,dspg_wdt" },
	{ },
};

static struct platform_driver watchdog_driver = {
	.driver = {
		.name = DSPG_WDT,
		.owner = THIS_MODULE,
		.of_match_table = dspg_wdt_of_ids,
	},
};

static int __init
dspg_wdt_init(void)
{
	return platform_driver_probe(&watchdog_driver, dspg_wdt_probe);
}
module_init(dspg_wdt_init);

MODULE_DESCRIPTION("DSPG watchdog driver");
MODULE_LICENSE("GPL");
