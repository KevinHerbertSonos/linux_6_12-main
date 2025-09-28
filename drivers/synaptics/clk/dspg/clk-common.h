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
#ifndef __DSPG_CLK_COMMON_H
#define __DSPG_CLK_COMMON_H

#include <linux/of.h>
#include <linux/clk-provider.h>

extern int of_dspg_get_mux_flags(struct device_node *node, u8 *flags);
extern int of_dspg_get_div_flags(struct device_node *node, u8 *flags);
extern int of_dspg_get_gate_flags(struct device_node *node, u8 *flags);
extern int of_dspg_get_clk_flags(struct device_node *node,
				 unsigned long *flags);
extern int of_dspg_get_div_table(struct device_node *node,
				 unsigned int div_width,
				 unsigned int div_flags,
				 struct clk_div_table **rdiv_table);

#endif
