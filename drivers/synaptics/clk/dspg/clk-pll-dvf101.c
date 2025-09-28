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
#include <linux/delay.h>
#include <linux/of.h>

#include "clk-pll.h"

#define PLL_UNIT_CTRL					0x00
#define PLL_UNIT_LOCK_CNT				0x08
#define PLL_CFG1					0x0c
#define PLL_CFG2					0x10

#define ABSDIFF(x, y)	((x) > (y) ? ((x) - (y)) : ((y) - (x)))

static long _dvf101_pll_round_rate(unsigned long in_rate, unsigned long rate,
				   unsigned long *rprediv,
				   unsigned long *rmult,
				   unsigned long *rpostdiv1,
				   unsigned long *rpostdiv2);

#define PRECISION		3
static inline uint32_t round_div(uint32_t a, uint32_t b)
{
	uint32_t tmp = (a << PRECISION) / b;

	return (tmp >> PRECISION) + ((tmp >> (PRECISION - 1)) & 1);
}

static void dvf101_pll_read_configuration(struct dspg_pll *pll)
{
	pll->refdiv = dspg_pll_read_reg_field(pll, PLL_CFG2,  0, 0x3f);
	pll->fbdiv = dspg_pll_read_reg_field(pll, PLL_CFG2,  8,  0xfff);
	pll->postdiv1 = dspg_pll_read_reg_field(pll, PLL_CFG2,  24, 0x7);
	pll->postdiv2 = dspg_pll_read_reg_field(pll, PLL_CFG2, 28,  0x7);
}

static void dvf101_pll_write_configuration(struct dspg_pll *pll)
{
	dspg_pll_write_reg_field(pll, PLL_CFG2,  0,  0x3f, pll->refdiv);
	dspg_pll_write_reg_field(pll, PLL_CFG2,  8, 0xfff, pll->fbdiv);
	dspg_pll_write_reg_field(pll, PLL_CFG2, 24,   0x7, pll->postdiv1);
	dspg_pll_write_reg_field(pll, PLL_CFG2, 28,   0x7, pll->postdiv2);
}

static int dvf101_pll_in_automatic_mode(struct dspg_pll *pll)
{
	if ((dspg_pll_read_reg(pll, PLL_UNIT_CTRL) >> 5) & 1)
		return 0;
	return 1;
}

static int dvf101_pll_is_locked(struct dspg_pll *pll)
{
	if ((dspg_pll_read_reg(pll, PLL_CFG1) >> 24) & 1)
		return 1;
	return 0;
}

static int dvf101_pll_is_power_down(struct dspg_pll *pll)
{
	if ((dspg_pll_read_reg(pll, PLL_UNIT_CTRL) >> 2) & 1)
		return 1;
	return 0;
}

static int dvf101_pll_clk_in_use(struct dspg_pll *pll)
{
	return (dspg_pll_read_reg(pll, PLL_UNIT_CTRL) >> 7) & 1;
}

static void dvf101_pll_set_power_down(struct dspg_pll *pll, unsigned long value)
{
	dspg_pll_write_reg_field(pll, PLL_UNIT_CTRL,  2, 1, value);
}

static void dvf101_pll_set_bypass(struct dspg_pll *pll, unsigned long value)
{
	dspg_pll_write_reg_field(pll, PLL_UNIT_CTRL,  4, 1, value);
}

static void dvf101_pll_set_lock_cnt(struct dspg_pll *pll, unsigned long value)
{
	dspg_pll_write_reg_field(pll, PLL_UNIT_LOCK_CNT,  0, 0xfffff, value);
}

static void dvf101_init_pll(struct dspg_pll *pll, struct device_node *node,
			    struct clk_init_data *init)
{
	int i;
	struct dspg_pll_precomp *precomp = pll->precomp;

	for (i = 0; i < pll->precomp_count; i++) {
		precomp->out_actual = _dvf101_pll_round_rate(precomp->in,
							precomp->out_desired,
							&precomp->refdiv,
							&precomp->fbdiv,
							&precomp->postdiv1,
							&precomp->postdiv2);
		precomp++;
	}

	dvf101_pll_read_configuration(pll);

	if (dvf101_pll_in_automatic_mode(pll))
		init->flags |= CLK_IGNORE_UNUSED;

	init->flags |= CLK_SET_RATE_NO_REPARENT;

	pll->lock_cnt = 1499;
	of_property_read_u32(node, "lock-cnt", (u32 *)&pll->lock_cnt);
}

static int dvf101_pll_enable(struct clk_hw *hw)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	if (!dvf101_pll_is_power_down(pll))
		/* PLL already enabled */
		return 0;

	if (dvf101_pll_in_automatic_mode(pll)) {
		dvf101_pll_write_configuration(pll);

		/* set lock count */
		dvf101_pll_set_lock_cnt(pll, pll->lock_cnt);

		dvf101_pll_set_power_down(pll, 0);
		while (!dvf101_pll_clk_in_use(pll))
			;
		return 0;
	}
	dvf101_pll_write_configuration(pll);

	/* set lock count */
	dvf101_pll_set_lock_cnt(pll, pll->lock_cnt);

	dvf101_pll_set_power_down(pll, 0);
	while (!dvf101_pll_is_locked(pll))
		;
	dvf101_pll_set_bypass(pll, 1);
	while (!dvf101_pll_clk_in_use(pll))
		;

	return 0;
}

static void dvf101_pll_disable(struct clk_hw *hw)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	if (dvf101_pll_in_automatic_mode(pll)) {
		dvf101_pll_set_power_down(pll, 1);
		while (dvf101_pll_clk_in_use(pll))
			;
		return;
	}

	dvf101_pll_set_bypass(pll, 0);
	while (dvf101_pll_clk_in_use(pll))
		;
	dvf101_pll_set_power_down(pll, 1);
}

static int dvf101_pll_is_enabled(struct clk_hw *hw)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	if (dvf101_pll_in_automatic_mode(pll)) {
		/* pll is always on in automatic mode */
		return 1;
	}

	if (dvf101_pll_is_power_down(pll))
		return 0;
	if (!dvf101_pll_is_locked(pll))
		return 0;
	return 1;
}

static unsigned long dvf101_pll_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct dspg_pll *pll = to_dspg_pll(hw);
	u64 rate;

	if (!dvf101_pll_clk_in_use(pll))
		return parent_rate;

	dvf101_pll_read_configuration(pll);

	rate = (u64)parent_rate * (u64)pll->fbdiv;
	do_div(rate, pll->refdiv);
	do_div(rate, pll->postdiv1 * pll->postdiv2);
	return rate;
}

static long _dvf101_pll_round_rate(unsigned long in_rate, unsigned long rate,
				   unsigned long *rprediv,
				   unsigned long *rmult,
				   unsigned long *rpostdiv1,
				   unsigned long *rpostdiv2)
{
	unsigned long prediv, mult, postdiv1, postdiv2;
	unsigned long match_prediv = 0, match_mult = 0, match_postdiv1 = 0,
		      match_postdiv2 = 0;
	unsigned long match_rate = 0, match_diff = ~0ul, match_vco = ~0ul;

	if (!(rate >> 10))
		return -1;

	for (prediv = 1; prediv <= 63; prediv++) {
		unsigned long phaseclk = (in_rate >> 10) / prediv;

		if (phaseclk < (1000000 >> 10))
			break;

		for (mult = 16; mult <= 2400; mult++) {
			unsigned long vcoclk, postdiv;

			vcoclk = phaseclk * mult;

			/* keep FVCO and FDIVREF in valid range */
			if (phaseclk > vcoclk / 16 ||
			    vcoclk < (600000000 >> 10))
				continue;
			else if (vcoclk > (1600000000 >> 10))
				break;

			postdiv = round_div(vcoclk, rate >> 10);

			if (postdiv < 1)
				postdiv = 1;
			else if (postdiv > 49)
				break;

			for (postdiv1 = 7; postdiv1 >= 1; postdiv1--) {
				unsigned long diff, tmp, out;

				postdiv2 = round_div(postdiv, postdiv1);

				if (postdiv2 < 1)
					postdiv2 = 1;
				else if (postdiv2 > postdiv1)
					break;

				out = vcoclk / postdiv1 / postdiv2;

				/* quick check if in target range */
				if (out - 10000 > (rate >> 10) ||
				    out + 10000 < (rate >> 10))
					continue;

				/* ok, do the precise calculation */
				tmp = in_rate * mult;
				tmp /= prediv;
				out = tmp / postdiv1 / postdiv2;

				/* better match? */
				diff = ABSDIFF(rate, out);
				if ((diff <  match_diff) ||
				    (diff == match_diff &&
				     vcoclk < match_vco)) {
					match_diff     = diff;
					match_vco      = vcoclk;
					match_rate     = out;
					match_prediv   = prediv;
					match_mult     = mult;
					match_postdiv1 = postdiv1;
					match_postdiv2 = postdiv2;
				}
			}
		}
	}

	if (!match_rate)
		return -EINVAL;

	if (rprediv && rmult && rpostdiv1 && rpostdiv2) {
		*rprediv   = match_prediv;
		*rmult     = match_mult;
		*rpostdiv1 = match_postdiv1;
		*rpostdiv2 = match_postdiv2;
	}

	return match_rate;

}

static long dvf101_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *parent_rate)
{
	struct dspg_pll *pll = to_dspg_pll(hw);
	struct dspg_pll_precomp *precomp = pll->precomp;
	int i;

	for (i = 0; i < pll->precomp_count; i++) {
		if (*parent_rate == precomp->in &&
		    rate == precomp->out_desired)
			return precomp->out_actual;
		precomp++;
	}

	return _dvf101_pll_round_rate(*parent_rate, rate,
				      NULL, NULL, NULL, NULL);
}

static int dvf101_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	unsigned long actual_rate = 0;
	struct dspg_pll *pll = to_dspg_pll(hw);
	struct dspg_pll_precomp *precomp = pll->precomp;
	int i;

	for (i = 0; i < pll->precomp_count; i++) {
		if (parent_rate == precomp->in &&
		    rate == precomp->out_desired) {
			actual_rate = precomp->out_actual;
			pll->refdiv = precomp->refdiv;
			pll->fbdiv = precomp->fbdiv;
			pll->postdiv1 = precomp->postdiv1;
			pll->postdiv2 = precomp->postdiv2;
			break;
		}
		precomp++;
	}

	if (!actual_rate)
		actual_rate = _dvf101_pll_round_rate(parent_rate, rate,
						     &pll->refdiv, &pll->fbdiv,
						     &pll->postdiv1,
						     &pll->postdiv2);

	dvf101_pll_disable(hw);
	dvf101_pll_enable(hw);

	return 0;
}

static const struct clk_ops dspg_dvf101_pll_ops = {
	.enable = dvf101_pll_enable,
	.disable = dvf101_pll_disable,
	.is_enabled = dvf101_pll_is_enabled,
	.recalc_rate = dvf101_pll_recalc_rate,
	.round_rate = dvf101_pll_round_rate,
	.set_rate = dvf101_pll_set_rate,
};

const struct dspg_pll_data dspg_dvf101_pll_data = {
	.init = dvf101_init_pll,
	.ops = &dspg_dvf101_pll_ops,
};

static const struct clk_ops dspg_dvf101_pll_always_on_ops = {
	.is_enabled = dvf101_pll_is_enabled,
	.recalc_rate = dvf101_pll_recalc_rate,
};

const struct dspg_pll_data dspg_dvf101_pll_always_on_data = {
	.init = dvf101_init_pll,
	.ops = &dspg_dvf101_pll_always_on_ops,
};
