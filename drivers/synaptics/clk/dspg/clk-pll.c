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

#include "clk-pll.h"
#include "clk-pll-dvf.h"
#include "clk-pll-dvf97.h"
#include "clk-pll-dvf101.h"

unsigned long dspg_pll_read_reg(struct dspg_pll *pll, unsigned long reg)
{
	return readl(pll->regs + reg);
}

void dspg_pll_write_reg(struct dspg_pll *pll, unsigned long reg,
			unsigned long value)
{
	writel(value, pll->regs + reg);
}

unsigned long dspg_pll_read_reg_field(struct dspg_pll *pll, unsigned long reg,
				      unsigned long shift, unsigned long mask)
{
	return (dspg_pll_read_reg(pll, reg) >> shift) & mask;
}

void dspg_pll_write_reg_field(struct dspg_pll *pll, unsigned long reg,
			      unsigned long shift, unsigned long mask,
			      unsigned long value)
{
	unsigned long regv;

	value &= mask;
	regv = dspg_pll_read_reg(pll, reg);
	regv &= ~(mask << shift);
	regv |= (value << shift);
	dspg_pll_write_reg(pll, reg, regv);
}

static const struct of_device_id dspg_pll_of_match[] = {
	{
		.compatible = "dspg,dvf-pll",
		.data = &dspg_dvf_pll_data,
	},
	{
		.compatible = "dspg,dvf-pll-always-on",
		.data = &dspg_dvf_pll_always_on_data,
	},
	{
		.compatible = "dspg,dvf97-pll",
		.data = &dspg_dvf97_pll_data,
	},
	{
		.compatible = "dspg,dvf97-pll-always-on",
		.data = &dspg_dvf97_pll_always_on_data,
	},
	{
		.compatible = "dspg,dvf101-pll",
		.data = &dspg_dvf101_pll_data,
	},
	{
		.compatible = "dspg,dvf101-pll-always-on",
		.data = &dspg_dvf101_pll_always_on_data,
	},
	{},
};

static struct clk * __init dspg_pll_register(const char *parent_name,
					     struct device_node *node,
					     struct dspg_pll_data *pll_data,
					     void __iomem *reg,
					     const char *clk_name)
{
	struct dspg_pll *pll;
	struct clk *clk;
	struct clk_init_data init;
	struct device_node *np;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	np = of_find_node_by_type(NULL, "cpu");
	if (np && of_parse_phandle(np, "clocks", 0) == node) {
		np = of_parse_phandle(np, "operating-points-v2", 0);
		if (np) {
			struct device_node *opp;
			struct dspg_pll_precomp *precomp;
			u64 tmp;

			pll->precomp_count = of_get_child_count(np);
			pll->precomp = kzalloc(sizeof(*pll->precomp) *
					       pll->precomp_count,
					       GFP_KERNEL);
			if (!pll->precomp)
				pll->precomp_count = 0;

			precomp = pll->precomp;
			for_each_child_of_node(np, opp) {
				of_property_read_u64(opp, "opp-hz", &tmp);
				/* FIXME: get actual input rate */
				precomp->in = 25000000;
				precomp->out_desired = (unsigned long)tmp;
				precomp++;
			}
		}
	}

	init.name = clk_name;
	init.ops = pll_data->ops;

	init.flags = 0;
	init.parent_names = &parent_name;
	init.num_parents  = 1;

	pll->data = pll_data;
	pll->regs = reg;
	pll->hw.init = &init;


	if (pll_data->init)
		pll_data->init(pll, node, &init);

	clk = clk_register(NULL, &pll->hw);
	if (IS_ERR(clk)) {
		kfree(pll);
		return clk;
	}
	return clk;
}

#ifdef CONFIG_OF
void __init of_dspg_clk_pll_setup(struct device_node *node)
{
	const struct of_device_id *match;
	struct dspg_pll_data *data;
	const char *parent_name;
	struct clk *clk;
	const char *clk_name = node->name;
	void __iomem *reg;

	of_property_read_string(node, "clock-output-names", &clk_name);

	match = of_match_node(dspg_pll_of_match, node);
	if (!match) {
		pr_err("no compatible pll driver for %s\n", clk_name);
		return;
	}
	data = (struct dspg_pll_data *)match->data;

	parent_name = of_clk_get_parent_name(node, 0);
	if (!parent_name) {
		pr_err("no valid parent for pll %s\n", clk_name);
		return;
	}

	/* get io memory */
	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("error mapping io memory for pll %s\n", clk_name);
		return;
	}

	clk = dspg_pll_register(parent_name, node, data, reg, clk_name);
	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	else
		pr_err("registration of pll %s failed with %ld\n",
		       clk_name, PTR_ERR(clk));
}
CLK_OF_DECLARE(clk_pll, "dspg,clk-pll", of_dspg_clk_pll_setup);
#endif
