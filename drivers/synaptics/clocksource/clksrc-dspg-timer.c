/*
 * drivers/clocksource/clksrc-dspg-timer.c
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
#include <linux/clksrc-dspg-timer.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>

#define DVF_TMR_LOAD		0x00
#define DVF_TMR_VALUE		0x04
#define DVF_TMR_CONTROL		0x08
#define DVF_TMR_INTCLR		0x0C

#define DVF_TIM_CR_ONESHOT	1
#define DVF_TIM_CR_SIZE32	(1 << 1)
#define DVF_TIM_CR_DIV16	(1 << 2)
#define DVF_TIM_CR_DIV256	(1 << 3)
#define DVF_TIM_CR_IRQ		(1 << 5)
#define DVF_TIM_CR_PERIOD	(1 << 6)
#define DVF_TIM_CR_RUN		(1 << 7)

#define DVF_TIM_CR_DEFAULT (DVF_TIM_CR_SIZE32 | DVF_TIM_CR_DIV16 | \
                            DVF_TIM_CR_IRQ    | DVF_TIM_CR_RUN)
#define DVF_TMR_DIFF		16

struct dspg_tmr {
	void *base;
	struct clk *clk;

	struct irqaction irq;
	struct clock_event_device clockevent;

	struct clocksource clocksource;
	struct timespec64 persistent_ts;
	u64 last_cycles;
};

static int
set_state_periodic(struct clock_event_device *clk)
{
	struct dspg_tmr *tmr = container_of(clk, struct dspg_tmr, clockevent);
	unsigned long period;
	int ret;

	ret = clk_enable(tmr->clk);
	if (ret)
		return ret;

	period = DIV_ROUND_UP(clk_get_rate(tmr->clk) / DVF_TMR_DIFF, HZ);
	writel(0, tmr->base + DVF_TMR_CONTROL);
	writel(period, tmr->base + DVF_TMR_LOAD);
	writel(DVF_TIM_CR_DEFAULT | DVF_TIM_CR_PERIOD,
	       tmr->base + DVF_TMR_CONTROL);

	return 0;
}

static int
set_state_oneshot(struct clock_event_device *clk)
{
	struct dspg_tmr *tmr = container_of(clk, struct dspg_tmr, clockevent);

	writel(0, tmr->base + DVF_TMR_CONTROL);
	writel(~0, tmr->base + DVF_TMR_LOAD);
	writel(DVF_TIM_CR_DEFAULT | DVF_TIM_CR_ONESHOT,
	       tmr->base + DVF_TMR_CONTROL);

	return 0;
}

static int
set_state_shutdown(struct clock_event_device *clk)
{
	struct dspg_tmr *tmr = container_of(clk, struct dspg_tmr, clockevent);

	clk_disable(tmr->clk);

	return 0;
}

static int
tick_resume(struct clock_event_device *clk)
{
	struct dspg_tmr *tmr = container_of(clk, struct dspg_tmr, clockevent);

	return clk_enable(tmr->clk);
}

static int set_next_event(unsigned long delta, struct clock_event_device *clk)
{
	struct dspg_tmr *tmr = container_of(clk, struct dspg_tmr, clockevent);
	unsigned long flags;

	if (delta < 1)
		delta = 2;

	raw_local_irq_save(flags);
	writel(delta, tmr->base + DVF_TMR_LOAD);
	writel(DVF_TIM_CR_DEFAULT | DVF_TIM_CR_ONESHOT, tmr->base + DVF_TMR_CONTROL);
	raw_local_irq_restore(flags);

	return 0;
}

static irqreturn_t tmr_interrupt(int irq, void *dev_id)
{
	struct dspg_tmr *tmr = dev_id;

	writel(0, tmr->base + DVF_TMR_INTCLR);

	tmr->clockevent.event_handler(&tmr->clockevent);

	return IRQ_HANDLED;
}

struct dspg_tmr __init *dspg_tmr_clockevent_init(struct device_node *node)
{
	struct dspg_tmr *tmr;
	int irq, ret = 0;

	tmr = kzalloc(sizeof(*tmr), GFP_KERNEL);
	if (!tmr)
		return ERR_PTR(-ENOMEM);

	tmr->base = of_iomap(node, 0);
	if (!tmr->base) {
		ret = -ENOMEM;
		goto err_free;
	}

	tmr->clk = clk_get_sys(node->name, NULL);
	if (IS_ERR(tmr->clk)) {
		ret = PTR_ERR(tmr->clk);
		goto err_unmap;
	}

	irq = irq_of_parse_and_map(node, 0);
	if (!irq) {
		ret = -ENODEV;
		goto err_put_clk;
	}

	/* Set up timer */
	clk_enable(tmr->clk);
	writel(0, tmr->base + DVF_TMR_CONTROL);

	tmr->irq.name    = "tmr_clockevent";
	tmr->irq.flags   = IRQF_TIMER;
	tmr->irq.handler = tmr_interrupt;
	tmr->irq.dev_id  = tmr;
	ret = setup_irq(irq, &tmr->irq);
	if (ret) {
		clk_disable(tmr->clk);
		goto err_put_clk;
	}

	return tmr;

err_put_clk:
	clk_put(tmr->clk);
err_unmap:
	iounmap(tmr->base);
err_free:
	kfree(tmr);
	return ERR_PTR(ret);
}

static u64 tmr_get_cycles(struct clocksource *cs)
{
	struct dspg_tmr *tmr = container_of(cs, struct dspg_tmr, clocksource);

	return dspg_tmr_read(tmr);
}

unsigned long dspg_tmr_read(struct dspg_tmr *tmr)
{
	return 0xffffffff - readl(tmr->base + DVF_TMR_VALUE);
}

unsigned long dspg_tmr_rate(struct dspg_tmr *tmr)
{
	return clk_get_rate(tmr->clk)/16;
}

void dspg_tmr_persistent_clock(struct dspg_tmr *tmr, struct timespec64 *ts)
{
	u64 cycles, nsecs, delta;
	struct timespec64 *tsp = &tmr->persistent_ts;

	cycles = dspg_tmr_read(tmr);
	delta = cycles - tmr->last_cycles;
	tmr->last_cycles = cycles;

	nsecs = clocksource_cyc2ns(delta, tmr->clocksource.mult,
				   tmr->clocksource.shift);
	timespec64_add_ns(tsp, nsecs);
	*ts = *tsp;
}

struct dspg_tmr __init *dspg_tmr_clocksource_init(struct device_node *node)
{
	struct dspg_tmr *tmr;
	int ret = 0;

	tmr = kzalloc(sizeof(*tmr), GFP_KERNEL);
	if (!tmr)
		return ERR_PTR(-ENOMEM);

	tmr->base = of_iomap(node, 0);
	if (!tmr->base) {
		ret = -ENOMEM;
		goto err_free;
	}

	tmr->clk = clk_get_sys(node->name, NULL);
	if (IS_ERR(tmr->clk)) {
		ret = PTR_ERR(tmr->clk);
		goto err_unmap;
	}

	/* Set up timer */
	clk_enable(tmr->clk);
	writel(0, tmr->base + DVF_TMR_CONTROL);
	writel(~0, tmr->base + DVF_TMR_LOAD);
	writel(DVF_TIM_CR_SIZE32 | DVF_TIM_CR_DIV16 | DVF_TIM_CR_RUN |
	       DVF_TIM_CR_PERIOD, tmr->base + DVF_TMR_CONTROL);

	return tmr;

err_unmap:
	iounmap(tmr->base);
err_free:
	kfree(tmr);
	return ERR_PTR(ret);
}

void __init dspg_tmr_register_clockevent(struct dspg_tmr *tmr)
{
	tmr->clockevent.name           = "dspg-tmr";
	tmr->clockevent.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	tmr->clockevent.set_state_periodic	= set_state_periodic;
	tmr->clockevent.set_state_oneshot	= set_state_oneshot;
	tmr->clockevent.set_state_shutdown	= set_state_shutdown;
	tmr->clockevent.tick_resume		= tick_resume;

	tmr->clockevent.set_next_event = set_next_event;

	clockevents_config_and_register(&tmr->clockevent,
					clk_get_rate(tmr->clk)/DVF_TMR_DIFF,
					3, 0xfffffff0ul);
}

int __init dspg_tmr_register_clocksource(struct dspg_tmr *tmr)
{
	tmr->clocksource.name	= "dspg-tmr";
	tmr->clocksource.rating	= 100;
	tmr->clocksource.read	= tmr_get_cycles;
	tmr->clocksource.mask	= CLOCKSOURCE_MASK(32);
	tmr->clocksource.flags	= CLOCK_SOURCE_IS_CONTINUOUS |
				  CLOCK_SOURCE_VALID_FOR_HRES;

	return clocksource_register_hz(&tmr->clocksource, clk_get_rate(tmr->clk)/16);
}
