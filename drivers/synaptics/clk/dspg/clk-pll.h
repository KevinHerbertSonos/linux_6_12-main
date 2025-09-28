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
#ifndef __DSPG_CLK_PLL_H
#define __DSPG_CLK_PLL_H

#include <linux/clk-provider.h>
#include <linux/of.h>

struct dspg_pll;

typedef void (*pll_init_fn_t)(struct dspg_pll *, struct device_node *node,
			      struct clk_init_data *);

struct dspg_pll_data {
	pll_init_fn_t init;
	const struct clk_ops *ops;
};

struct dspg_pll_precomp {
	unsigned long in;
	unsigned long out_desired;
	unsigned long out_actual;
	unsigned long refdiv;
	unsigned long fbdiv;
	unsigned long postdiv1;
	unsigned long postdiv2;
	unsigned long bs;
};

struct dspg_pll {
	unsigned long refdiv;
	unsigned long fbdiv;
	unsigned long postdiv1;
	unsigned long postdiv2;
	unsigned long bs;
	unsigned long lock_cnt;
	struct clk_hw hw;
	struct dspg_pll_data *data;
	void __iomem *regs;
	int precomp_count;
	struct dspg_pll_precomp *precomp;
};
#define to_dspg_pll(_hw) container_of(_hw, struct dspg_pll, hw)

extern unsigned long dspg_pll_read_reg(struct dspg_pll *pll, unsigned long reg);
extern void dspg_pll_write_reg(struct dspg_pll *pll, unsigned long reg,
			       unsigned long value);
extern unsigned long dspg_pll_read_reg_field(struct dspg_pll *pll,
					     unsigned long reg,
					     unsigned long shift,
					     unsigned long mask);
extern void dspg_pll_write_reg_field(struct dspg_pll *pll, unsigned long reg,
				     unsigned long shift, unsigned long mask,
				     unsigned long value);

#endif
