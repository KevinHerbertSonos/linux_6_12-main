/*
 * drivers/clocksource/clksrc-dspg-rtc.c
 *
 * Copyright (C) 2012 DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/clk.h>
#include <linux/time64.h>
#include <linux/clksrc-dspg-rtc.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>

#define DMW_RTC_CFG		0x00
#define DMW_RTC_CTL		0x04
#define DMW_RTC_ALARMVAL	0x08
#define DMW_RTC_STATUS		0x0C
#define DMW_RTC_INTCLR		0x10
#define DMW_RTC_TIME		0x14

struct dspg_rtc {
	void *base;
	struct clk *clk;

	struct clocksource clocksource;

	struct timespec64 persistent_ts;
	u64 last_cycles;
};

static u64 rtc_get_cycles(struct clocksource *cs)
{
	struct dspg_rtc *rtc = container_of(cs, struct dspg_rtc, clocksource);

	return dspg_rtc_read(rtc);
}

unsigned long dspg_rtc_read(struct dspg_rtc *rtc)
{
	unsigned long val1, val2;
	const void *reg = rtc->base + DMW_RTC_TIME;

	do {
		val1 = __raw_readl(reg);
		val2 = __raw_readl(reg);
	} while (val1 != val2);

	return val2;
}

unsigned long dspg_rtc_rate(struct dspg_rtc *rtc)
{
	return clk_get_rate(rtc->clk);
}

/**
 * dspg_rtc_persistent_clock -  Return time for read_persistent_clock().
 *
 * Reads the time from the RTC which isn't disabled during PM.  Convert the
 * cycles elapsed since last read into nsecs and adds to a monotonically
 * increasing timespec.
 */
void dspg_rtc_persistent_clock(struct dspg_rtc *rtc, struct timespec64 *ts)
{
	static u64 cycles;
	u64 nsecs, delta;
	struct timespec64 *tsp = &rtc->persistent_ts;

	cycles = dspg_rtc_read(rtc);
	delta = cycles - rtc->last_cycles;
	rtc->last_cycles = cycles;

	nsecs = clocksource_cyc2ns(delta, rtc->clocksource.mult,
				   rtc->clocksource.shift);
	timespec64_add_ns(tsp, nsecs);
	*ts = *tsp;
}

struct dspg_rtc __init *dspg_rtc_init(struct device_node *node)
{
	struct dspg_rtc *rtc;
	int ret = 0;

	rtc = kzalloc(sizeof(*rtc), GFP_KERNEL);
	if (!rtc)
		return ERR_PTR(-ENOMEM);

	rtc->base = of_iomap(node, 0);
	if (!rtc->base) {
		ret = -ENOMEM;
		goto err_free;
	}

	rtc->clk = clk_get_sys(node->name, NULL);
	if (IS_ERR(rtc->clk)) {
		ret = PTR_ERR(rtc->clk);
		goto err_unmap;
	}

	/* Set up RTC */
	clk_enable(rtc->clk);
	writel(0, rtc->base + DMW_RTC_CFG);

	return rtc;

err_unmap:
	iounmap(rtc->base);
err_free:
	kfree(rtc);
	return ERR_PTR(ret);
}

int __init dspg_rtc_register_clocksource(struct dspg_rtc *rtc)
{
	rtc->clocksource.name	= "dspg-rtc";
	rtc->clocksource.rating	= 100;
	rtc->clocksource.read	= rtc_get_cycles;
	rtc->clocksource.mask	= CLOCKSOURCE_MASK(32);
	rtc->clocksource.flags	= CLOCK_SOURCE_IS_CONTINUOUS |
				  CLOCK_SOURCE_VALID_FOR_HRES;

	return clocksource_register_hz(&rtc->clocksource, clk_get_rate(rtc->clk));
}
