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
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "clk-common.h"
#include "clk-gate.h"

static DEFINE_SPINLOCK(_lock);

static struct clk_divider *dspg_create_clk_div(void __iomem *reg,
					       const struct clk_div_table *tbl,
					       u8 flags,
					       u8 shift,
					       u8 width)
{
	struct clk_divider *div;

	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		return NULL;

	div->reg = reg;
	div->shift = shift;
	div->width = width;
	div->flags = flags;
	div->table = tbl;
	div->lock = &_lock;

	return div;
}

static unsigned long clk_divider_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	return clk_divider_ops.recalc_rate(hw, parent_rate);
}

/*
 * try to round to the requested rate but do not get below it
 */
static long clk_divider_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	unsigned long rrate;
	unsigned long tmp_rate = rate;

	do {
		rrate = clk_divider_ops.round_rate(hw, tmp_rate, prate);
		tmp_rate++;

	} while (rrate < rate && tmp_rate <= *prate);

	return rrate;
}

static int clk_divider_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	return clk_divider_ops.set_rate(hw, rate, parent_rate);
}

static const struct clk_ops dspg_clk_divider_ops = {
	.recalc_rate = clk_divider_recalc_rate,
	.round_rate = clk_divider_round_rate,
	.set_rate = clk_divider_set_rate,
};

#ifdef CONFIG_OF
void __init of_dspg_clk_gated_div_setup(struct device_node *node)
{
	int num_parents, ret;
	struct clk *clk;
	struct clk_divider *clk_div;
	struct clk_div_table *div_table = NULL;
	struct dspg_clk_gate *clk_gate;
	const char *clk_name = node->name;
	void __iomem *reg_div;
	const char *parent_name;
	u8 div_flags = 0, div_shift = 0, div_width;
	unsigned long flags = 0;
	const struct clk_ops *ops;
	u32 tmp;

	of_property_read_string(node, "clock-output-names", &clk_name);

	of_dspg_get_clk_flags(node, &flags);
	of_dspg_get_div_flags(node, &div_flags);

	num_parents = of_clk_get_parent_count(node);
	if (num_parents != 1) {
		pr_err("invalid nr of parents for divider %s\n", clk_name);
		return;
	}
	parent_name = of_clk_get_parent_name(node, 0);

	/* get divider shift from devicetree, zero by default */
	of_property_read_u32(node, "div_shift", &tmp);
	div_shift = (u8)tmp;

	/* get divider width from devicetree */
	ret = of_property_read_u32(node, "div_width", &tmp);
	if (ret) {
		pr_err("invalid width property for divider %s\n", clk_name);
		return;
	}
	div_width = (u8)tmp;

	/* read/generate divisior table */
	ret = of_dspg_get_div_table(node, div_width, div_flags, &div_table);
	if (ret)
		return;

	if (of_find_property(node, "div-round-min", NULL))
		ops = &dspg_clk_divider_ops;
	else
		ops = &clk_divider_ops;

	/* get io memory of divider register */
	reg_div = of_iomap(node, 0);
	if (!reg_div) {
		pr_err("error mapping io memory for divider %s\n", clk_name);
		kfree(div_table);
		return;
	}

	clk_div = dspg_create_clk_div(reg_div,
				      div_table,
				      div_flags,
				      div_shift,
				      div_width);
	if (!clk_div) {
		kfree(div_table);
		return;
	}

	clk_gate = dspg_create_clk_gate(node, 1, &_lock);
	if (IS_ERR_OR_NULL(clk_gate)) {
		kfree(div_table);
		kfree(clk_div);
		return;
	}

	clk = clk_register_composite(NULL,
				     clk_name,
				     &parent_name,
				     1,
				     NULL, NULL,
				     &clk_div->hw, ops,
				     &clk_gate->hw, &dspg_clk_gate_ops,
				     flags);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	else {
		pr_err("registration of divider %s failed with %ld\n",
		       clk_name, PTR_ERR(clk));
		kfree(div_table);
		kfree(clk_div);
		kfree(clk_gate);
	}
}
CLK_OF_DECLARE(clk_div, "dspg,clk-gated-div", of_dspg_clk_gated_div_setup);
#endif
