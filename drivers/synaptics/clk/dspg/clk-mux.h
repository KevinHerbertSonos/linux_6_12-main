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
#ifndef __DSPG_CLK_MUX_H
#define __DSPG_CLK_MUX_H

#include <linux/clk-provider.h>

typedef void (*dspg_mux_init_t)(void __iomem *, unsigned long *, u8 *);

struct dspg_mux_data {
	dspg_mux_init_t init;
};

struct clk_mux *dspg_create_clk_mux(struct device_node *node,
				    unsigned int reg_idx,
				    spinlock_t *lock);

#endif
