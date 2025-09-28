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

#include "clk-mux.h"
#include "clk-mux-dvf101.h"
#include "clk-common.h"

static DEFINE_SPINLOCK(_lock);

static const struct of_device_id dspg_mux_of_match[] = {
	{
		.compatible = "dspg,dvf-pll-mux",
	},
	/*
	 * muxer that checks the PLL_FSM_BYPASS bit to decide if it
	 * can be controlled or is just read-only
	 */
	{
		.compatible = "dspg,dvf101-pll-mux",
		.data = &dspg_dvf101_mux_data,
	},
	{},
};

static int dspg_retrieve_clk_mux(struct device_node *node,
				 unsigned int reg_idx, void __iomem **reg,
				 u32 **table, u32 *mask, u8 *shift, u8 *flags,
				 int *num_parents, const char ***parent_names)
{
	int i, ret;
	const char *clk_name = node->name;
	u32 tmp;

	of_dspg_get_mux_flags(node, flags);

	*num_parents = of_clk_get_parent_count(node);
	if (*num_parents <= 0) {
		pr_err("error getting nr of parents for mux %s\n", clk_name);
		return -EINVAL;
	}

	*parent_names = kcalloc(*num_parents, sizeof(**parent_names),
				GFP_KERNEL);
	if (!*parent_names) {
		pr_err("no memory left for mux %s\n", clk_name);
		return -ENOMEM;
	}

	for (i = 0; i < *num_parents; i++)
		(*parent_names)[i] = of_clk_get_parent_name(node, i);

	*table = NULL;
	if (of_find_property(node, "input-select", NULL)) {
		*table = kcalloc(*num_parents, sizeof(u32), GFP_KERNEL);
		if (!*table) {
			pr_err("no memory left for mux %s\n", clk_name);
			kfree(*parent_names);
			return -ENOMEM;
		}

		/* get table of index values (one for each selectable parent) */
		ret = of_property_read_u32_array(node,
						 "input-select",
						 *table,
						 *num_parents);
		if (ret) {
			pr_err("invalid input-select property for mux %s\n",
			       clk_name);
			kfree(*parent_names);
			kfree(*table);
			return -EINVAL;
		}
	}

	/* get shift from devicetree, zero by default */
	of_property_read_u32(node, "shift", &tmp);
	*shift = (u8)tmp;

	/* get mask from devicetree */
	ret = of_property_read_u32(node, "mask", mask);
	if (ret) {
		pr_err("invalid mask property for mux %s\n", clk_name);
		kfree(*parent_names);
		kfree(*table);
		return -EINVAL;
	}

	/* get io memory of divider gate register */
	*reg = of_iomap(node, reg_idx);
	if (!*reg)
		return -EINVAL;

	return 0;
}

struct clk_mux *dspg_create_clk_mux(struct device_node *node,
				    unsigned int reg_idx,
				    spinlock_t *lock)
{
	struct clk_mux *mux;
	void __iomem *reg;
	int num_parents, ret;
	const char **parent_names;
	u8 flags = 0;
	u8 shift = 0;
	u32 mask;
	u32 *table = NULL;

	ret = dspg_retrieve_clk_mux(node, reg_idx, &reg, &table, &mask, &shift,
				    &flags, &num_parents, &parent_names);
	if (ret < 0)
		return ERR_PTR(ret);

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return ERR_PTR(-ENOMEM);

	mux->reg = reg;
	mux->table = table;
	mux->mask = mask;
	mux->shift = shift;
	mux->flags = flags;
	mux->lock = lock;

	return mux;
}

#ifdef CONFIG_OF
void __init of_dspg_clk_mux_setup(struct device_node *node)
{
	const struct of_device_id *match;
	const struct dspg_mux_data *data = NULL;
	int num_parents, ret;
	struct clk *clk;
	const char *clk_name = node->name;
	const char **parent_names;
	void __iomem *reg;
	u8 shift = 0;
	u8 mux_flags = 0;
	u32 mask;
	unsigned long flags = 0;
	u32 *table = NULL;

	of_property_read_string(node, "clock-output-names", &clk_name);

	of_dspg_get_clk_flags(node, &flags);

	/* check for special muxer features */
	match = of_match_node(dspg_mux_of_match, node);
	if (match)
		data = (struct dspg_mux_data *)match->data;

	ret = dspg_retrieve_clk_mux(node, 0, &reg, &table, &mask, &shift,
				    &mux_flags, &num_parents, &parent_names);
	if (ret < 0)
		return;

	/* call dedicated init function for this muxer if any */
	if (data) {
		if (data->init)
			data->init(reg, &flags, &mux_flags);
	}

	clk = clk_register_mux_table(NULL,
				     clk_name,
				     parent_names,
				     num_parents,
				     flags, /* flags */
				     reg,
				     shift,
				     mask,
				     mux_flags, /* mux flags */
				     table,
				     &_lock);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	else {
		pr_err("registration of mux %s failed with %ld\n",
		       clk_name, PTR_ERR(clk));
		kfree(parent_names);
		kfree(table);
	}
}
CLK_OF_DECLARE(clk_mux, "dspg,clk-mux", of_dspg_clk_mux_setup);
#endif
