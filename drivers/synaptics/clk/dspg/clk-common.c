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
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/clk-provider.h>

int of_dspg_get_mux_flags(struct device_node *node, u8 *flags)
{
	if (of_find_property(node, "mux-index-one", NULL))
		*flags |= CLK_MUX_INDEX_ONE;
	if (of_find_property(node, "mux-index-bit", NULL))
		*flags |= CLK_MUX_INDEX_BIT;
	if (of_find_property(node, "mux-hiword-mask", NULL))
		*flags |= CLK_MUX_HIWORD_MASK;
	if (of_find_property(node, "mux-read-only", NULL))
		*flags |= CLK_MUX_READ_ONLY;
	if (of_find_property(node, "mux-round-closest", NULL))
		*flags |= CLK_MUX_ROUND_CLOSEST;
	return 0;
}

int of_dspg_get_div_flags(struct device_node *node, u8 *flags)
{
	if (of_find_property(node, "div-one-based", NULL))
		*flags |= CLK_DIVIDER_ONE_BASED;
	if (of_find_property(node, "div-power-of-two", NULL))
		*flags |= CLK_DIVIDER_POWER_OF_TWO;
	if (of_find_property(node, "div-allow-zero", NULL))
		*flags |= CLK_DIVIDER_ALLOW_ZERO;
	if (of_find_property(node, "div-hiword-mask", NULL))
		*flags |= CLK_DIVIDER_HIWORD_MASK;
	if (of_find_property(node, "div-round-closest", NULL))
		*flags |= CLK_DIVIDER_ROUND_CLOSEST;
	if (of_find_property(node, "div-read-only", NULL))
		*flags |= CLK_DIVIDER_READ_ONLY;
	return 0;
}

int of_dspg_get_gate_flags(struct device_node *node, u8 *flags)
{
	if (of_find_property(node, "gate-set-to-disable", NULL))
		*flags |= CLK_GATE_SET_TO_DISABLE;
	if (of_find_property(node, "gate-hiword-mask", NULL))
		*flags |= CLK_GATE_HIWORD_MASK;
	return 0;
}

int of_dspg_get_clk_flags(struct device_node *node, unsigned long *flags)
{
	if (of_find_property(node, "clk-set-rate-gate", NULL))
		*flags |= CLK_SET_RATE_GATE;
	if (of_find_property(node, "clk-set-parent-gate", NULL))
		*flags |= CLK_SET_PARENT_GATE;
	if (of_find_property(node, "clk-set-rate-parent", NULL))
		*flags |= CLK_SET_RATE_PARENT;
	if (of_find_property(node, "clk-ignore-unused", NULL))
		*flags |= CLK_IGNORE_UNUSED;
	if (of_find_property(node, "clk-get-rate-nocache", NULL))
		*flags |= CLK_GET_RATE_NOCACHE;
	if (of_find_property(node, "clk-set-rate-no-reparent", NULL))
		*flags |= CLK_SET_RATE_NO_REPARENT;
	if (of_find_property(node, "clk-get-accuracy-nocache", NULL))
		*flags |= CLK_GET_ACCURACY_NOCACHE;
	if (of_find_property(node, "clk-is-critical", NULL))
		*flags |= CLK_IS_CRITICAL;
	return 0;
}

int of_dspg_get_div_table(struct device_node *node, unsigned int div_width,
			  unsigned int div_flags,
			  struct clk_div_table **rdiv_table)
{
	unsigned int min_div;
	const char *clk_name = node->name;
	int i, num_dividers;
	struct clk_div_table *div_table = NULL, *dte;


	/* get divider table if available */
	num_dividers = of_property_count_u32_elems(node, "div-table");

	if ((num_dividers <= 0 || num_dividers % 2) &&
	     num_dividers != -EINVAL) {
		pr_err("invalid width divider table for %s\n", clk_name);
		return -EINVAL;
	}

	if (num_dividers != -EINVAL) {
		num_dividers /= 2;
		div_table = kcalloc(num_dividers, sizeof(*div_table),
				    GFP_KERNEL);
		if (!div_table)
			return -ENOMEM;

		for (i = 0, dte = &div_table[0];
		     i < num_dividers;
		     i++, dte++) {
			of_property_read_u32_index(node,
						   "div-table",
						   i * 2,
						   &dte->div);
			of_property_read_u32_index(node,
						   "div-table",
						   i * 2 + 1,
						   &dte->val);
		}
	}

	/*
	 * generate an even divider table if required, if there is a div-table
	 * and a div-table-even property then the div-table has precedence
	 */
	if (of_find_property(node, "div-table-even", NULL) && !div_table) {
		min_div = div_flags & CLK_DIVIDER_ALLOW_ZERO ? 0 : 2;
		num_dividers = (1 << div_width >> 1) + (min_div ? 0 : 1);

		div_table = kcalloc(num_dividers, sizeof(*div_table),
				    GFP_KERNEL);
		if (!div_table)
			return -ENOMEM;

		dte = &div_table[0];
		for (i = 0; i < num_dividers; i++, dte++) {
			dte->div = min_div + i * 2;
			dte->val = dte->div - (dte->div ? 1 : 0);
		}
	}

	*rdiv_table = div_table;

	return 0;
}
