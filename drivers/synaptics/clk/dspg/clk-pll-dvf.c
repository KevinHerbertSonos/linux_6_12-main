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

#define ABSDIFF(x, y)	((x) > (y) ? ((x) - (y)) : ((y) - (x)))

static long _dvf_pll_round_rate(unsigned long in_rate, unsigned long rate,
				unsigned long *rprediv,
				unsigned long *rmult,
				unsigned long *rpostdiv,
				unsigned long *rbs);

#define PRECISION		3
static inline uint32_t round_div(uint32_t a, uint32_t b)
{
	uint32_t tmp = (a << PRECISION) / b;

	return (tmp >> PRECISION) + ((tmp >> (PRECISION - 1)) & 1);
}

static void dvf_pll_read_configuration(struct dspg_pll *pll)
{
	pll->refdiv   = dspg_pll_read_reg_field(pll, 0,  0, 0x1f) + 1;
	pll->fbdiv    = dspg_pll_read_reg_field(pll, 0,  5, 0x7f) + 1;
	pll->postdiv1 = dspg_pll_read_reg_field(pll, 0, 12,  0x3);
	pll->bs       = dspg_pll_read_reg_field(pll, 0, 14,  0x1);
}

static void dvf_pll_write_configuration(struct dspg_pll *pll)
{
	dspg_pll_write_reg_field(pll, 0,  0, 0x1f, pll->refdiv - 1);
	dspg_pll_write_reg_field(pll, 0,  5, 0x7f, pll->fbdiv - 1);
	dspg_pll_write_reg_field(pll, 0, 12,  0x3, pll->postdiv1);
	dspg_pll_write_reg_field(pll, 0, 14,  0x1, pll->bs);
}

static int dvf_pll_is_locked(struct dspg_pll *pll)
{
	if ((dspg_pll_read_reg(pll, 0) >> 18) & 1)
		return 1;
	return 0;
}

static int dvf_pll_is_power_down(struct dspg_pll *pll)
{
	if ((dspg_pll_read_reg(pll, 0) >> 15) & 1)
		return 1;
	return 0;
}

static void dvf_pll_set_power_down(struct dspg_pll *pll, unsigned long value)
{
	dspg_pll_write_reg_field(pll, 0, 15, 1, value);
}

static void dvf_pll_set_bypass(struct dspg_pll *pll, unsigned long value)
{
	dspg_pll_write_reg_field(pll, 0, 16, 1, value);
}

static void dvf_init_pll(struct dspg_pll *pll, struct device_node *node,
			    struct clk_init_data *init)
{
	int i;
	struct dspg_pll_precomp *precomp = pll->precomp;

	for (i = 0; i < pll->precomp_count; i++) {
		precomp->out_actual = _dvf_pll_round_rate(precomp->in,
							  precomp->out_desired,
							  &precomp->refdiv,
							  &precomp->fbdiv,
							  &precomp->postdiv1,
							  &precomp->bs);
		precomp++;
	}

	dvf_pll_read_configuration(pll);

	init->flags |= CLK_SET_RATE_NO_REPARENT;
}

static int dvf_pll_enable(struct clk_hw *hw)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	if (!dvf_pll_is_power_down(pll))
		/* PLL already enabled */
		return 0;

	dvf_pll_write_configuration(pll);

	dvf_pll_set_power_down(pll, 0);
	dvf_pll_set_bypass(pll, 0);
	while (!dvf_pll_is_locked(pll))
		;

	return 0;
}

static void dvf_pll_disable(struct clk_hw *hw)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	dvf_pll_set_bypass(pll, 1);
	dvf_pll_set_power_down(pll, 1);
}

static int dvf_pll_is_enabled(struct clk_hw *hw)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	if (dvf_pll_is_power_down(pll))
		return 0;
	if (!dvf_pll_is_locked(pll))
		return 0;
	return 1;
}

static unsigned long dvf_pll_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct dspg_pll *pll = to_dspg_pll(hw);

	if (!dvf_pll_is_enabled(hw))
		return parent_rate;

	dvf_pll_read_configuration(pll);

	return ((parent_rate / pll->refdiv) * pll->fbdiv) /
		(1 << pll->postdiv1);
}

static long _dvf_pll_round_rate(unsigned long in_rate, unsigned long rate,
				unsigned long *rprediv,
				unsigned long *rmult,
				unsigned long *rpostdiv,
				unsigned long *rbs)
{
	unsigned long prediv, mult, postdiv;
	unsigned long match_prediv = 0, match_mult = 0, match_postdiv = 0;
	unsigned long match_rate = 0, match_diff = ~0ul, match_vco = ~0ul;

	if (!(rate >> 10))
		return -1;

	/* loop through all settings finding best match */
	for (prediv = 1; prediv <= 32; prediv++) {
		unsigned long phaseclk = (in_rate >> 10) / prediv;

		/* keep in phase detector reference frequeny: 10..50MHz */
		if (phaseclk < 10000000 >> 10 || phaseclk > 50000000 >> 10)
			continue;

		for (mult = 6; mult <= 100; mult++) {
			unsigned long vcoclk = phaseclk * mult;

			/* keep in VCO frequency range: 300..1000MHz */
			if (vcoclk < 30000000 >> 10 ||
			    vcoclk > 1000000000 >> 10)
				continue;

			for (postdiv = 0; postdiv <= 3; postdiv++) {
				unsigned long diff, out = vcoclk >> postdiv;
				unsigned long long tmp;

				/* quick check if in target range */
				if (out - 10000 > rate >> 10 ||
				    out + 10000 < rate >> 10)
					continue;

				/* ok, do the precise calculation */
				tmp = (unsigned long long)in_rate * mult;
				do_div(tmp, prediv);
				out = (unsigned long)tmp >> postdiv;

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
					match_postdiv  = postdiv;
				}
			}
		}
	}

	if (!match_rate)
		return -EINVAL;

	if (rprediv && rmult && rpostdiv && rbs) {
		*rprediv  = match_prediv;
		*rmult    = match_mult;
		*rpostdiv = match_postdiv;
		*rbs      = match_vco > 600000000>>10 ? 1 << 14 : 0;
	}

	return match_rate;

}

static long dvf_pll_round_rate(struct clk_hw *hw, unsigned long rate,
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

	return _dvf_pll_round_rate(*parent_rate, rate,
				      NULL, NULL, NULL, NULL);
}

static int dvf_pll_set_rate(struct clk_hw *hw, unsigned long rate,
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
			pll->bs = precomp->bs;
			break;
		}
		precomp++;
	}

	if (!actual_rate)
		actual_rate = _dvf_pll_round_rate(parent_rate, rate,
						  &pll->refdiv, &pll->fbdiv,
						  &pll->postdiv1, &pll->bs);

	dvf_pll_disable(hw);
	dvf_pll_enable(hw);

	return 0;
}

static const struct clk_ops dspg_dvf_pll_ops = {
	.enable = dvf_pll_enable,
	.disable = dvf_pll_disable,
	.is_enabled = dvf_pll_is_enabled,
	.recalc_rate = dvf_pll_recalc_rate,
	.round_rate = dvf_pll_round_rate,
	.set_rate = dvf_pll_set_rate,
};

const struct dspg_pll_data dspg_dvf_pll_data = {
	.init = dvf_init_pll,
	.ops = &dspg_dvf_pll_ops,
};

static const struct clk_ops dspg_dvf_pll_always_on_ops = {
	.is_enabled = dvf_pll_is_enabled,
	.recalc_rate = dvf_pll_recalc_rate,
};

const struct dspg_pll_data dspg_dvf_pll_always_on_data = {
	.init = dvf_init_pll,
	.ops = &dspg_dvf_pll_always_on_ops,
};
