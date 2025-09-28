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
#ifndef __DSPG_CLK_GATE_H
#define __DSPG_CLK_GATE_H

#include <linux/clk-provider.h>

struct dspg_clk_gate {
	struct clk_hw	hw;
	void __iomem	*reg;
	u32		en_bits;
	u32		dis_bits;
	u32		stat_bits;
	int		phase_shift;
	int		phase_width;
	u8		flags;
	spinlock_t	*lock;
};

struct dspg_clk_gate *dspg_create_clk_gate(struct device_node *node,
					   unsigned int reg_idx,
					   spinlock_t *lock);

extern const struct clk_ops dspg_clk_gate_ops;

#endif
