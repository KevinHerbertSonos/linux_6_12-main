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
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>
#include <linux/reset.h>
#include <linux/mfd/syscon.h>

#include "clk-common.h"

#define to_clk_mux_regmap(_hw) container_of(_hw, struct clk_mux_regmap, hw)

struct clk_mux_regmap {
	struct clk_hw hw;
	struct clk_init_data clk_init_data;
	u8 flags;
	u8 shift;
	u32 mask;
	u32 *table;
	struct clk *clk;
	spinlock_t lock;
	struct regmap_field *f;
};

static u8 clk_mux_get_parent(struct clk_hw *hw)
{
	struct clk_mux_regmap *mux = to_clk_mux_regmap(hw);
	int num_parents = clk_hw_get_num_parents(hw);
	u32 val;
	int ret;

	ret = regmap_field_read(mux->f, &val);
	if (ret)
		return (u8)-EIO;

	/* copy from drivers/clk/clk-mux.c */
	if (mux->table) {
		int i;

		for (i = 0; i < num_parents; i++)
			if (mux->table[i] == val)
				return i;
		return (u8)-EINVAL;
	}

	if (val && (mux->flags & CLK_MUX_INDEX_BIT))
		val = ffs(val) - 1;

	if (val && (mux->flags & CLK_MUX_INDEX_ONE))
		val--;

	if (val >= num_parents)
		return (u8)-EINVAL;

	return val;
}

static int clk_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_mux_regmap *mux = to_clk_mux_regmap(hw);
	int ret;
	unsigned long flags = 0;

	/* copy from drivers/clk/clk-mux.c */
	if (mux->table)
		index = mux->table[index];
	else {
		if (mux->flags & CLK_MUX_INDEX_BIT)
			index = 1 << index;

		if (mux->flags & CLK_MUX_INDEX_ONE)
			index++;
	}

	spin_lock_irqsave(&mux->lock, flags);
	ret = regmap_field_write(mux->f, index);
	spin_unlock_irqrestore(&mux->lock, flags);

	return ret;
}

static const struct clk_ops clk_mux_regmap_ops = {
	.get_parent = clk_mux_get_parent,
	.set_parent = clk_mux_set_parent,
	.determine_rate = __clk_mux_determine_rate_closest,
};

static int clk_mux_regmap_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct clk_init_data *init_data;
	int num_parents, i, ret;
	const char *clk_name;
	const char **parent_names;
	struct clk_mux_regmap *cm_regmap;
	struct regmap *map;
	struct regmap_field *f;
	struct resource *res;
	u32 tmp;

	if (!np)
		return -EINVAL;

	clk_name = np->name;

	cm_regmap = devm_kzalloc(&pdev->dev, sizeof(*cm_regmap), GFP_KERNEL);
	if (!cm_regmap)
		return -ENOMEM;

	init_data = &cm_regmap->clk_init_data;

	spin_lock_init(&cm_regmap->lock);

	of_property_read_string(np, "clock-output-names", &init_data->name);

	of_dspg_get_clk_flags(np, &init_data->flags);
	of_dspg_get_mux_flags(np, &cm_regmap->flags);

	num_parents = of_clk_get_parent_count(np);
	if (num_parents <= 0) {
		dev_err(&pdev->dev, "error getting nr of parents for mux %s\n",
			clk_name);
		return -EINVAL;
	}

	parent_names = devm_kzalloc(&pdev->dev, sizeof(*parent_names),
				    GFP_KERNEL);
	if (!parent_names)
		return -EINVAL;

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(np, i);

	if (of_find_property(np, "input-select", NULL)) {
		cm_regmap->table = devm_kzalloc(&pdev->dev,
						sizeof(u32) * num_parents,
						GFP_KERNEL);
		if (!cm_regmap->table) {
			dev_err(&pdev->dev, "no memory left for mux %s\n",
				clk_name);
			return -ENOMEM;
		}

		/* get table of index values (one for each selectable parent) */
		ret = of_property_read_u32_array(np,
						 "input-select",
						 cm_regmap->table,
						 num_parents);
		if (ret) {
			dev_err(&pdev->dev,
				"invalid input-select property for mux %s\n",
				clk_name);
			return -EINVAL;
		}
	}

	/* get shift from devicetree, zero by default */
	of_property_read_u32(np, "shift", &tmp);
	cm_regmap->shift = tmp;

	/* get mask from devicetree */
	ret = of_property_read_u32(np, "mask", &cm_regmap->mask);
	if (ret) {
		dev_err(&pdev->dev, "invalid mask property for mux %s\n",
			clk_name);
		return -EINVAL;
	}

	map = syscon_regmap_lookup_by_phandle(np, "dvf,syscfg");
	if (IS_ERR(map)) {
		dev_err(&pdev->dev, "failed to get syscon");
		return PTR_ERR(map);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to get resources");
		return -EINVAL;
	}

	f = devm_regmap_field_alloc(&pdev->dev, map,
		(struct reg_field)REG_FIELD(res->start, cm_regmap->shift,
					cm_regmap->mask << cm_regmap->shift));
	if (IS_ERR(f)) {
		dev_err(&pdev->dev, "failed to alloc regmap");
		return PTR_ERR(f);
	}

	init_data->ops = &clk_mux_regmap_ops;
	init_data->name = clk_name;
	init_data->num_parents = num_parents;
	init_data->parent_names = parent_names;
	cm_regmap->hw.init = init_data;
	cm_regmap->f = f;

	cm_regmap->clk = clk_register(&pdev->dev, &cm_regmap->hw);
	if (IS_ERR(cm_regmap->clk)) {
		ret = PTR_ERR(cm_regmap->clk);
		dev_err(&pdev->dev, "failed to register clock muxer\n");
		return ret;
	}
	of_clk_add_provider(np, of_clk_src_simple_get, cm_regmap->clk);
	dev_info(&pdev->dev, "successfully probed");
	return 0;
}

static const struct of_device_id clk_mux_regmap_dt_match[] = {
	{
		.compatible = "dspg,clk-mux-regmap",
	},
	{ }
};

static struct platform_driver clk_mux_regmap_driver = {
	.probe = clk_mux_regmap_probe,
	.driver = {
		.name = "clk-mux-regmap",
		.owner = THIS_MODULE,
		.of_match_table = clk_mux_regmap_dt_match,
	},
};

static int __init clk_mux_regmap_init(void)
{
	return platform_driver_register(&clk_mux_regmap_driver);
}
arch_initcall(clk_mux_regmap_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Regmap clock mux driver");
