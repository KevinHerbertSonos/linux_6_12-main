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
#include <linux/clk-provider.h>
#include <linux/io.h>

#include "clk-mux.h"

static void dvf101_mux_init(void __iomem *reg, unsigned long *flags,
			    u8 *mux_flags)
{
	int manual_mode;

	manual_mode = !!((readl(reg) >> 5) & 1);
	/*
	 * if the PLL in front of this muxer is not in manual mode
	 * (FSM_BYPASS == 0) the muxer is automatically controlled by the PLL
	 * and due to this will be marked read-only
	 */
	if (!manual_mode)
		*mux_flags |= CLK_MUX_READ_ONLY;
}

const struct dspg_mux_data dspg_dvf101_mux_data = {
	.init = dvf101_mux_init,
};
