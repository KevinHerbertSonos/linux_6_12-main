/*
 * drivers/clocksource/clksrc-dvf-rtc.c
 *
 * Copyright (C) 2016 DSP Group
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
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/reset.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/sched_clock.h>
#include <linux/delay.h>
#include <asm/mach/time.h>

#define DMW_RTC_CFG		0x00
#define DMW_RTC_CTL		0x04
#define DMW_RTC_ALARMVAL	0x08
#define DMW_RTC_STATUS		0x0C
#define DMW_RTC_INTCLR		0x10
#define DMW_RTC_TIME		0x14

struct dspg_rtc {
	void *base;
	struct clk *clk;
	struct notifier_block clk_change_nb;

	struct clocksource clocksource;

	struct timespec64 persistent_ts;
	u64 last_cycles;
};

#define nb_to_dspg_rtc(x)	container_of(x, struct dspg_rtc, \
					     clk_change_nb);

static struct dspg_rtc *rtc_cs;

static unsigned long dspg_rtc_read(struct dspg_rtc *rtc)
{
	unsigned long val1, val2;
	const void *reg = rtc->base + DMW_RTC_TIME;

	do {
		val1 = __raw_readl(reg);
		val2 = __raw_readl(reg);
	} while (val1 != val2);

	return val2;
}

static u64 rtc_get_cycles(struct clocksource *cs)
{
	struct dspg_rtc *rtc = container_of(cs, struct dspg_rtc, clocksource);

	return dspg_rtc_read(rtc);
}

static unsigned long dspg_rtc_rate(struct dspg_rtc *rtc)
{
	return clk_get_rate(rtc->clk);
}

static u64 notrace rtc_sched_read_count(void)
{
	return dspg_rtc_read(rtc_cs);
}

static unsigned long rtc_delay_timer_read(void)
{
	return dspg_rtc_read(rtc_cs);
}

static struct delay_timer rtc_delay_timer = {
	.read_current_timer = rtc_delay_timer_read,
};

/**
 * dspg_rtc_persistent_clock -  Return time for read_persistent_clock().
 *
 * Reads the time from the RTC which isn't disabled during PM.  Convert the
 * cycles elapsed since last read into nsecs and adds to a monotonically
 * increasing timespec.
 */
void dspg_rtc_persistent_clock(struct timespec64 *ts)
{
	static u64 cycles;
	unsigned long long nsecs;
	u64 delta;
	struct timespec64 *tsp = &rtc_cs->persistent_ts;

	cycles = dspg_rtc_read(rtc_cs);
	delta = cycles - rtc_cs->last_cycles;
	rtc_cs->last_cycles = cycles;

	nsecs = clocksource_cyc2ns(delta, rtc_cs->clocksource.mult,
				   rtc_cs->clocksource.shift);
	timespec64_add_ns(tsp, nsecs);
	*ts = *tsp;
}

static int dspg_rtc_clk_change_cb(struct notifier_block *nb,
				  unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct dspg_rtc *rtc = nb_to_dspg_rtc(nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		clocksource_unregister(&rtc->clocksource);
		break;
	case POST_RATE_CHANGE:
		clocksource_register_hz(&rtc->clocksource, ndata->new_rate);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static void __init dspg_rtc_register_clocksource(struct dspg_rtc *rtc)
{
	int ret;

	rtc->clocksource.name	= "dspg-rtc";
	rtc->clocksource.rating	= 100;
	rtc->clocksource.read	= rtc_get_cycles;
	rtc->clocksource.mask	= CLOCKSOURCE_MASK(32);
	rtc->clocksource.flags	= CLOCK_SOURCE_IS_CONTINUOUS |
				  CLOCK_SOURCE_VALID_FOR_HRES;

	ret = clocksource_register_hz(&rtc->clocksource, dspg_rtc_rate(rtc));
	if (ret)
		panic("could not initialize rtc clocksource: %d\n", ret);

	sched_clock_register(rtc_sched_read_count, 32, dspg_rtc_rate(rtc));

	rtc_delay_timer.freq = dspg_rtc_rate(rtc);
	register_current_timer_delay(&rtc_delay_timer);
}

static void __init dspg_rtc_reset(struct device_node *node)
{
	int ret;
	struct reset_control *rc;

	rc = of_reset_control_get(node, NULL);
	if (IS_ERR(rc)) {
		pr_warn("unable to get reset control for rtc: %ld\n",
			PTR_ERR(rc));
		return;
	}

	ret = reset_control_deassert(rc);
	if (ret)
		panic("unable to release rtc reset: %d\n", ret);
}

static int __init dspg_rtc_clocksource_init(struct device_node *node)
{
	struct dspg_rtc *rtc;

	dspg_rtc_reset(node);

	rtc = kzalloc(sizeof(*rtc), GFP_KERNEL);
	if (!rtc)
		panic("no memory for clocksource rtc\n");
	rtc_cs = rtc;

	rtc->base = of_iomap(node, 0);
	if (!rtc->base)
		panic("unable to map io memory for rtc clocksource\n");

	rtc->clk = of_clk_get(node, 0);
	if (IS_ERR(rtc->clk))
		panic("unabled to get clock for timer clocksource: %ld",
		      PTR_ERR(rtc->clk));

	rtc->clk_change_nb.notifier_call = dspg_rtc_clk_change_cb;
	if (clk_notifier_register(rtc->clk, &rtc->clk_change_nb))
		panic("unable to register clock notifier\n");

	/* Set up RTC */
	clk_prepare_enable(rtc->clk);
	writel(0, rtc->base + DMW_RTC_CFG);

	dspg_rtc_register_clocksource(rtc);
	rtc_cs = rtc;
	register_persistent_clock(dspg_rtc_persistent_clock);

	return 0;
}

TIMER_OF_DECLARE(dspg_rtc_clksrc, "dspg,rtc-clocksource",
		 dspg_rtc_clocksource_init);
