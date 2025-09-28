/*
 * drivers/clocksource/clksrc-dvf-timer.c
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
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/sched_clock.h>
#include <linux/delay.h>

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
	struct notifier_block clk_change_nb;

	struct irqaction irq;
	struct clock_event_device clockevent;

	struct clocksource clocksource;
	struct timespec64 persistent_ts;
	u64 last_cycles;
};

#define nb_to_dspg_tmr(x)	container_of(x, struct dspg_tmr, \
					     clk_change_nb);

static int
set_state_periodic(struct clock_event_device *clk)
{
	struct dspg_tmr *tmr = container_of(clk, struct dspg_tmr, clockevent);
	unsigned long period;
	int ret;

	ret = clk_enable(tmr->clk);
	if (ret)
		return ret;

	period = DIV_ROUND_UP(clk_get_rate(tmr->clk)/DVF_TMR_DIFF, HZ);
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
	writel(DVF_TIM_CR_DEFAULT | DVF_TIM_CR_ONESHOT,
	       tmr->base + DVF_TMR_CONTROL);
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

static void __init dspg_tmr_register_clockevent(struct dspg_tmr *tmr)
{
	tmr->clockevent.name = "dspg-tmr";
	tmr->clockevent.features = CLOCK_EVT_FEAT_PERIODIC |
				   CLOCK_EVT_FEAT_ONESHOT;
	tmr->clockevent.set_state_periodic	= set_state_periodic;
	tmr->clockevent.set_state_oneshot	= set_state_oneshot;
	tmr->clockevent.set_state_shutdown	= set_state_shutdown;
	tmr->clockevent.tick_resume		= tick_resume;
	tmr->clockevent.set_next_event = set_next_event;

	clockevents_config_and_register(&tmr->clockevent,
					clk_get_rate(tmr->clk)/DVF_TMR_DIFF,
					3, 0xfffffff0ul);
}

static void __init dspg_tmr_reset(struct device_node *node)
{
	int ret;
	struct reset_control *rc;

	rc = of_reset_control_get(node, NULL);
	if (IS_ERR(rc)) {
		pr_warn("unable to get reset control for timer: %ld\n",
			PTR_ERR(rc));
		return;
	}

	ret = reset_control_deassert(rc);
	if (ret)
		panic("unable to release timer reset: %d\n", ret);
}

static int __init dspg_tmr_clockevent_init(struct device_node *node)
{
	struct dspg_tmr *tmr;
	int irq, ret = 0;

	dspg_tmr_reset(node);

	tmr = kzalloc(sizeof(*tmr), GFP_KERNEL);
	if (!tmr)
		panic("no memory for clockevent timer\n");

	tmr->base = of_iomap(node, 0);
	if (!tmr->base)
		panic("unable to map io memory for timer clockevent\n");

	tmr->clk = of_clk_get(node, 0);
	if (IS_ERR(tmr->clk))
		panic("unable to get clock for timer clockevent: %ld",
		      PTR_ERR(tmr->clk));

	irq = of_irq_get(node, 0);
	if (irq < 0)
		panic("unable to get irq for timer clockevent: %d", irq);

	/* Set up timer */
	clk_prepare_enable(tmr->clk);
	writel(0, tmr->base + DVF_TMR_CONTROL);

	ret = request_irq(irq, tmr_interrupt, IRQF_TIMER, "tmr_clockevent", tmr);
	if (ret)
		panic("unable to setup irq for timer clockevent: %d\n", ret);

	dspg_tmr_register_clockevent(tmr);

	return 0;
}
TIMER_OF_DECLARE(dspg_tmr_clkevt, "dspg,tmr-clockevent",
		 dspg_tmr_clockevent_init);

static struct dspg_tmr *tmr_cs;

static u32 notrace dspg_tmr_read(struct dspg_tmr *tmr)
{
	return 0xffffffff - readl(tmr->base + DVF_TMR_VALUE);
}

static u64 tmr_get_cycles(struct clocksource *cs)
{
	struct dspg_tmr *tmr = container_of(cs, struct dspg_tmr, clocksource);

	return dspg_tmr_read(tmr);
}

static unsigned long dspg_tmr_rate(struct dspg_tmr *tmr)
{
	return clk_get_rate(tmr->clk) / 16;
}

static u64 notrace tmr_sched_read_count(void)
{
	return dspg_tmr_read(tmr_cs);
}

static unsigned long dvf_delay_timer_read(void)
{
	return dspg_tmr_read(tmr_cs);
}

static struct delay_timer dvf_delay_timer = {
	.read_current_timer = dvf_delay_timer_read,
};

static void __init dspg_tmr_register_clocksource(struct dspg_tmr *tmr)
{
	int ret;

	tmr->clocksource.name	= "dspg-tmr";
	tmr->clocksource.rating	= 100;
	tmr->clocksource.read	= tmr_get_cycles;
	tmr->clocksource.mask	= CLOCKSOURCE_MASK(32);
	tmr->clocksource.flags	= CLOCK_SOURCE_IS_CONTINUOUS |
				  CLOCK_SOURCE_VALID_FOR_HRES;

	ret = clocksource_register_hz(&tmr->clocksource, dspg_tmr_rate(tmr));
	if (ret)
		panic("could not initialize timer clocksource: %d\n", ret);

	sched_clock_register(tmr_sched_read_count, 32, dspg_tmr_rate(tmr));

	dvf_delay_timer.freq = dspg_tmr_rate(tmr);
	register_current_timer_delay(&dvf_delay_timer);
}

static int dspg_tmr_clk_change_cb(struct notifier_block *nb,
				  unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct dspg_tmr *tmr = nb_to_dspg_tmr(nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		clocksource_unregister(&tmr->clocksource);
		break;
	case POST_RATE_CHANGE:
		clocksource_register_hz(&tmr->clocksource, ndata->new_rate);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static int __init dspg_tmr_clocksource_init(struct device_node *node)
{
	dspg_tmr_reset(node);

	tmr_cs = kzalloc(sizeof(*tmr_cs), GFP_KERNEL);
	if (!tmr_cs)
		panic("no memory for clocksource timer\n");

	tmr_cs->base = of_iomap(node, 0);
	if (!tmr_cs->base)
		panic("unable to map io memory for timer clocksource\n");

	tmr_cs->clk = of_clk_get(node, 0);
	if (IS_ERR(tmr_cs->clk))
		panic("unabled to get clock for timer clocksource: %ld",
		      PTR_ERR(tmr_cs->clk));

	tmr_cs->clk_change_nb.notifier_call = dspg_tmr_clk_change_cb;
	if (clk_notifier_register(tmr_cs->clk, &tmr_cs->clk_change_nb))
		panic("unable to register clock notifier\n");

	/* Set up timer */
	clk_prepare_enable(tmr_cs->clk);
	writel(0, tmr_cs->base + DVF_TMR_CONTROL);
	writel(~0, tmr_cs->base + DVF_TMR_LOAD);
	writel(DVF_TIM_CR_SIZE32 | DVF_TIM_CR_DIV16 | DVF_TIM_CR_RUN |
	       DVF_TIM_CR_PERIOD, tmr_cs->base + DVF_TMR_CONTROL);

	dspg_tmr_register_clocksource(tmr_cs);

	return 0;
}

TIMER_OF_DECLARE(dspg_tmr_clksrc, "dspg,tmr-clocksource",
		 dspg_tmr_clocksource_init);
