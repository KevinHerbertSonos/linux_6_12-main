/*
 *  drivers/irqchip/irq-dspg.c
 *
 *  Copyright (C) 2016 DSPG Technologies GmbH
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

#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/irqdomain.h>
#include <linux/syscore_ops.h>
#include <linux/irqchip.h>

#include <asm/exception.h>
#include <asm/mach/irq.h>
#include <asm/hardware/plicu.h>

static void __iomem *plicu_base;
static unsigned long irq_wake_mask[2];
static struct irq_domain *plicu_domain;

static void plicu_ack(struct irq_data *data)
{
	unsigned long reg_clr;
	unsigned int irq = data->hwirq;

	if (irq > 31) {
		irq -= 32;
		reg_clr = PLICU_CH_CLR_INT2;
	} else {
		reg_clr = PLICU_CH_CLR_INT1;
	}

	writel(1 << irq, plicu_base + reg_clr);
}

static void plicu_mask(struct irq_data *data)
{
	unsigned long reg_mask;
	unsigned int irq = data->hwirq;

	/* Beware the inverted semantic of 'masking' in the PLICU */
	if (irq > 31) {
		irq -= 32;
		reg_mask = PLICU_CLR_CH_MASK2;
	} else {
		reg_mask = PLICU_CLR_CH_MASK1;
	}


	writel(1 << irq, plicu_base + reg_mask);
}

static void plicu_unmask(struct irq_data *data)
{
	unsigned long reg_mask;
	unsigned int irq = data->hwirq;

	/* Beware the inverted semantic of 'masking' in the PLICU */
	if (irq > 31) {
		irq -= 32;
		reg_mask = PLICU_SET_CH_MASK2;
	} else {
		reg_mask = PLICU_SET_CH_MASK1;
	}

	writel(1 << irq, plicu_base + reg_mask);
}

static int plicu_set_type(struct irq_data *data, unsigned int flow_type)
{
	unsigned long idx, reg_edge, value;
	unsigned long flags;
	int ret = 0;

	idx = data->hwirq;
	if (idx > 31) {
		idx -= 32;
		reg_edge = PLICU_CH_EDGE_LEVEL2;
	} else {
		reg_edge = PLICU_CH_EDGE_LEVEL1;
	}

	local_irq_save(flags);

	value = readl(plicu_base + reg_edge);
	if ((flow_type & IRQ_TYPE_SENSE_MASK) == IRQ_TYPE_EDGE_RISING) {
		value |= 1 << idx;
		irq_set_handler_locked(data, handle_edge_irq);
	} else if ((flow_type & IRQ_TYPE_SENSE_MASK) == IRQ_TYPE_LEVEL_HIGH) {
		value &= ~(1 << idx);
		irq_set_handler_locked(data, handle_level_irq);
	} else
		ret = -EINVAL;
	writel(value, plicu_base + reg_edge);

	local_irq_restore(flags);

	return ret;
}

static int plicu_set_wake(struct irq_data *data, unsigned int enable)
{
	if (enable)
		set_bit(data->hwirq, irq_wake_mask);
	else
		clear_bit(data->hwirq, irq_wake_mask);

	return 0;
}

static struct irq_chip plicu_chip = {
	.name     = "dspg-plicu",
	.irq_ack      = plicu_ack,
	.irq_mask     = plicu_mask,
	.irq_unmask   = plicu_unmask,
	.irq_set_type = plicu_set_type,
	.irq_set_wake = plicu_set_wake,
};

#ifdef CONFIG_PM
static unsigned long irq_enable_save[2];

/*
 * Suspend the interrupt controller, leaving only wakeup capable interrupt
 * sources enabled.  Must be called with interrupts disabled.
 */
static int plicu_suspend(void)
{
	/* save mask of enabled interrupts */
	irq_enable_save[0] = readl(plicu_base + PLICU_CH_MASK1);
	irq_enable_save[1] = readl(plicu_base + PLICU_CH_MASK2);

	/* disable everything except wakeup capable sources */
	writel(~irq_wake_mask[0], plicu_base + PLICU_CLR_CH_MASK1);
	writel(~irq_wake_mask[1], plicu_base + PLICU_CLR_CH_MASK2);

	return 0;
}

static void plicu_resume(void)
{
	writel(irq_enable_save[0], plicu_base + PLICU_SET_CH_MASK1);
	writel(irq_enable_save[1], plicu_base + PLICU_SET_CH_MASK2);
}

struct syscore_ops plicu_syscore_ops = {
	.suspend	= plicu_suspend,
	.resume		= plicu_resume,
};

int plicu_pending(void)
{
	return readl(plicu_base + PLICU_CAUSE1) ||
	       readl(plicu_base + PLICU_CAUSE2);
}
EXPORT_SYMBOL(plicu_pending);

static int __init plicu_pm_init(void)
{
	if (plicu_base)
		register_syscore_ops(&plicu_syscore_ops);

	return 0;
}
late_initcall(plicu_pm_init);
#endif /* CONFIG_PM */

asmlinkage void __exception_irq_entry plicu_handle_irq(struct pt_regs *regs)
{
	u32 stat;

	stat = readl_relaxed(plicu_base + PLICU_CAUSE1);
	if (stat) {
		handle_domain_irq(plicu_domain, ffs(stat) - 1, regs);
		return;
	}

	stat = readl_relaxed(plicu_base + PLICU_CAUSE2);
	if (stat)
		handle_domain_irq(plicu_domain, 32 + ffs(stat) - 1, regs);
}

static int plicu_irq_map(struct irq_domain *dom, unsigned int irq,
			 irq_hw_number_t hw_irq)
{
	unsigned long reg_edge, value;
	unsigned long flags;

	/* use level irq by default */
	if (hw_irq > 31) {
		hw_irq -= 32;
		reg_edge = PLICU_CH_EDGE_LEVEL2;
	} else {
		reg_edge = PLICU_CH_EDGE_LEVEL1;
	}

	local_irq_save(flags);
	value = readl(plicu_base + reg_edge);
	value &= ~(1 << hw_irq);
	writel(value, plicu_base + reg_edge);
	local_irq_restore(flags);

	irq_set_chip_and_handler(irq, &plicu_chip, handle_level_irq);
	irq_clear_status_flags(irq, IRQ_NOREQUEST | IRQ_NOPROBE);
	irq_set_status_flags(irq, IRQ_LEVEL);

	return 0;
}

static struct irq_domain_ops plicu_irq_ops = {
	.map	= plicu_irq_map,
	.xlate	= irq_domain_xlate_onecell,
};

int __init plicu_init(struct device_node *node, struct device_node *parent)
{
	if (WARN(parent, "non-root PLICU not supported"))
		return -EINVAL;

	if (WARN(plicu_base, "only one PLICU supported"))
		return -EINVAL;

	plicu_base = of_iomap(node, 0);
	if (WARN_ON(!plicu_base))
		return -EIO;

	set_handle_irq(plicu_handle_irq);

	/* disable and clear all interrupt sources */
	writel(0xffffffff, plicu_base + PLICU_CLR_CH_MASK1);
	writel(0xffffffff, plicu_base + PLICU_CLR_CH_MASK2);
	writel(0xffffffff, plicu_base + PLICU_CH_CLR_INT1);
	writel(0xffffffff, plicu_base + PLICU_CH_CLR_INT2);

	plicu_domain = irq_domain_add_linear(node, 64, &plicu_irq_ops, NULL);
	if (WARN_ON(!plicu_domain))
		goto out_unmap;

	irq_set_default_host(plicu_domain);

	return 0;

 out_unmap:
	iounmap(plicu_base);
	plicu_base = 0;

	return -EIO;
}
IRQCHIP_DECLARE(dspg_plicu_int_controller, "dspg,plicu", plicu_init);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
