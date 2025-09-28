/*
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
 *
 * Common Clock Framework support for DSPG platforms
 */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "clk-common.h"
#include "clk-gate.h"

static DEFINE_SPINLOCK(_lock);

#define to_dspg_clk_gate(_hw) container_of(_hw, struct dspg_clk_gate, hw)

static int dspg_clk_generate_bitmask(struct device_node *node,
				     const char *propname, u32 *out)
{
	u32 mask = 0;
	u32 value;
	struct property *prop;
	const __be32 *p;

	if (!of_find_property(node, propname, NULL))
		return -EINVAL;

	of_property_for_each_u32(node, propname, prop, p, value)
		mask |= BIT(value);

	*out = mask;

	return 0;
}

struct dspg_clk_gate *dspg_create_clk_gate(struct device_node *node,
					   unsigned int reg_idx,
					   spinlock_t *lock)
{
	struct dspg_clk_gate *gate;
	void __iomem *reg;
	int ret;
	u8 flags = 0;
	u32 en_bits = 0;
	u32 dis_bits = 0;
	u32 stat_bits = 0;

	of_dspg_get_gate_flags(node, &flags);

	/* get gate enable bits from devicetree, zero by default */
	ret = dspg_clk_generate_bitmask(node, "en_bits", &en_bits);
	if (ret)
		return ERR_PTR(ret);

	/* get gate disable bits from devicetree, en_bits by default */
	ret = dspg_clk_generate_bitmask(node, "dis_bits", &dis_bits);
	if (ret)
		dis_bits = en_bits;

	/* get gate status bits from devicetree, en_bits by default */
	ret = dspg_clk_generate_bitmask(node, "stat_bits", &stat_bits);
	if (ret)
		stat_bits = en_bits;

	/* get io memory of divider gate register */
	reg = of_iomap(node, reg_idx);
	if (!reg)
		return ERR_PTR(-EINVAL);

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		return ERR_PTR(-ENOMEM);

	ret = of_property_read_u32(node, "phase_shift", &gate->phase_shift);
	if (ret)
		gate->phase_shift = -1;
	ret = of_property_read_u32(node, "phase_width", &gate->phase_width);
	if (ret)
		gate->phase_width = 0;

	gate->reg = reg;
	gate->en_bits = en_bits;
	gate->dis_bits = dis_bits;
	gate->stat_bits = stat_bits;
	gate->flags = flags;
	gate->lock = lock;

	return gate;
}

static int dspg_clk_gate_enable(struct clk_hw *hw)
{
	struct dspg_clk_gate *gate = to_dspg_clk_gate(hw);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(gate->lock, flags);

	reg = readl(gate->reg);
	reg |= gate->en_bits;
	writel(reg, gate->reg);

	spin_unlock_irqrestore(gate->lock, flags);

	return 0;
}

static void dspg_clk_gate_disable(struct clk_hw *hw)
{
	struct dspg_clk_gate *gate = to_dspg_clk_gate(hw);
	int set = gate->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(gate->lock, flags);

	reg = readl(gate->reg);

	if (set)
		reg |= gate->dis_bits;
	else
		reg &= ~gate->dis_bits;

	writel(reg, gate->reg);

	spin_unlock_irqrestore(gate->lock, flags);
}

static int dspg_clk_gate_is_enabled(struct clk_hw *hw)
{
	u32 reg;
	struct dspg_clk_gate *gate = to_dspg_clk_gate(hw);

	reg = readl(gate->reg);
	reg &= gate->stat_bits;

	return reg ? 1 : 0;
}

static int dspg_clk_gate_set_phase(struct clk_hw *hw, int degrees)
{
	u32 reg;
	u32 shift = degrees, resolution;
	struct dspg_clk_gate *gate = to_dspg_clk_gate(hw);
	unsigned long flags;

	if (gate->phase_shift < 0 || gate->phase_width <= 0)
		return -EINVAL;

	/* phase shifts by N/2 input clock cycles (input->divider->gate) */
	resolution = 2 * (clk_get_rate(clk_get_parent(clk_get_parent(hw->clk)))
		     / clk_get_rate(hw->clk));

	if (shift >= resolution)
		return -EINVAL;

	spin_lock_irqsave(gate->lock, flags);

	reg = readl(gate->reg);
	reg &= ~(0xFFFFFFFF >> (32 - gate->phase_width) << gate->phase_shift);
	reg |= shift << gate->phase_shift;
	writel(reg, gate->reg);

	spin_unlock_irqrestore(gate->lock, flags);

	return 0;
}

const struct clk_ops dspg_clk_gate_ops = {
	.enable = dspg_clk_gate_enable,
	.disable = dspg_clk_gate_disable,
	.is_enabled = dspg_clk_gate_is_enabled,
};

const struct clk_ops dspg_clk_gate_phased_ops = {
	.enable = dspg_clk_gate_enable,
	.disable = dspg_clk_gate_disable,
	.is_enabled = dspg_clk_gate_is_enabled,
	.set_phase = dspg_clk_gate_set_phase,
};

#ifdef CONFIG_OF
static const struct of_device_id dspg_gate_of_match[] = {
	{
		.compatible = "dspg,clk-gate-phased",
		.data = &dspg_clk_gate_phased_ops,
	},
	{ /* sentinel */ },
};

void __init of_dspg_clk_gate_setup(struct device_node *node)
{
	struct dspg_clk_gate *gate;
	struct clk *clk;
	struct clk_init_data init;
	const char *parent_name;
	const char *clk_name = node->name;
	unsigned long flags = 0;
	const struct of_device_id *match;

	of_property_read_string(node, "clock-output-names", &clk_name);

	parent_name = of_clk_get_parent_name(node, 0);
	if (!parent_name) {
		pr_err("no valid parent for gate %s\n", clk_name);
		return;
	}

	of_dspg_get_clk_flags(node, &flags);

	gate = dspg_create_clk_gate(node, 0, &_lock);
	if (IS_ERR_OR_NULL(gate)) {
		pr_err("no memory for gate %s\n", clk_name);
		return;
	}

	match = of_match_node(dspg_gate_of_match, node);
	if (match)
		init.ops = (const struct clk_ops *)match->data;
	else
		init.ops = &dspg_clk_gate_ops;

	init.name = clk_name;
	init.flags = flags;
	init.parent_names = &parent_name;
	init.num_parents = 1;

	gate->hw.init = &init;

	clk = clk_register(NULL, &gate->hw);
	if (IS_ERR(clk)) {
		pr_err("registration of gate %s failed with %ld\n",
		       clk_name, PTR_ERR(clk));
		kfree(gate);
	}
	of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(clk_gate, "dspg,clk-gate", of_dspg_clk_gate_setup);
#endif
