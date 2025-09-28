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

static DEFINE_SPINLOCK(_lock);

#ifdef CONFIG_OF
void __init of_dspg_clk_div_setup(struct device_node *node)
{
	int num_parents, ret;
	struct clk *clk;
	const char *clk_name = node->name;
	void __iomem *reg;
	const char *parent_name;
	u8 div_flags = 0, shift = 0, width;
	unsigned long flags = 0;
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
	shift = (u8)tmp;

	/* get divider width from devicetree */
	ret = of_property_read_u32(node, "div_width", &tmp);
	if (ret) {
		pr_err("invalid width property for divider %s\n", clk_name);
		return;
	}
	width = (u8)tmp;

	/* get io memory */
	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("error mapping io memory for divider %s\n", clk_name);
		return;
	}

	clk = clk_register_divider(NULL,
				   clk_name,
				   parent_name,
				   flags,
				   reg,
				   shift,
				   width,
				   div_flags,
				   &_lock);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	else
		pr_err("registration of divider %s failed with %ld\n",
		       clk_name, PTR_ERR(clk));
}
CLK_OF_DECLARE(clk_div, "dspg,clk-div", of_dspg_clk_div_setup);
#endif
