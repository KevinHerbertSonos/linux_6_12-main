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
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <dt-bindings/clk/dvf101-clk.h>

#define DVF_TX_PAD_SRC_SEL(x)	(((x) & 0x3) << 30)
#define DVF_TX_PAD_OEN		(1 << 29)
#define DVF_RX_PAD_OEN		(1 << 28)
#define DVF_RX_CLK_SEL		(1 << 24)
#define DVF_TX_CLK_SEL		(1 << 23)

#define DVF_INPUT_SEL(x)		(((x) & 0x3) <<  8)

#define DVF_INT_DIV_VAL(x)	((x) & 0x3F)
#define DVF_DIV_VAL(x)		(((x) & 0xFF) << 20)

#define ETHMAC_CLK_EN		(1 << 3)
#define RMII_IF_CLK_EN		(1 << 5)
#define CMU_INT_ETHERDIV_EN	(1 << 17)

#define DVF101_RX_PAD_OEN		(1 << 25)
#define DVF101_TX_PAD_OEN		(1 << 24)
#define DVF101_RMII_IF_CLK_EN		(1 << 20)
#define DVF101_CORE_CLK_EN		(1 << 19)
#define DVF101_TX_PAD_SRC_SEL		(1 << 18)
#define DVF101_RX_CLK_SEL		(1 << 17)
#define DVF101_TX_CLK_SEL		(1 << 16)
#define DVF101_PHASE_SHIFT_EN		(1 << 13)
#define DVF101_INT_DIV_EN_STAT		(1 << 12)
#define DVF101_INT_DIV_POL		(1 << 11)
#define DVF101_INT_DIV_SRC_SEL		(1 << 10)
#define DVF101_MAIN_DIV_EN_STAT		(1 <<  7)
#define DVF101_MAIN_CLK_EN		(1 <<  6)
#define DVF101_AXI_CLK_EN		(1 <<  5)
#define DVF101_INT_DIV_EN		(1 <<  4)

#define DVF101_TX_CLK_PHASE_SEL(x)	(((x) & 0x7) << 21)
#define DVF101_INPUT_SEL(x)		(((x) & 0x3) <<  8)

#define DVF101_INT_DIV_VAL(x)		(((x) &  0x3F) << 24)
#define DVF101_MAIN_DIV_VAL(x)		(((x) & 0x1FF) <<  0)

#define to_dspg_eth_clk(_hw)	container_of(_hw, struct dspg_eth_clk, hw)

enum eth_mode {
	ETH_MII,
	ETH_SLVMII,
	ETH_RMII,
	ETH_SLVRMII,
	ETH_RGMII,

	NUM_ETH_MODES
};

struct dspg_eth_clk {
	struct clk_hw		hw;
	void __iomem		*reg_ctrl;
	void __iomem		*reg_ctrl1;
	void __iomem		*reg_div;
	void __iomem		*reg_en;
	int			mode;
	spinlock_t		lock;
};

static int
dvf_eth_clk_prepare(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;
	int ret = 0;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_ctrl);

	reg &= ~(DVF_TX_PAD_SRC_SEL(3) | DVF_TX_PAD_OEN |
		 DVF_RX_CLK_SEL | DVF_TX_CLK_SEL);
	reg |= DVF_RX_PAD_OEN;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		break;
	case DSPG_ETH_MII_EXT:
		reg |= DVF_TX_CLK_SEL;
		break;
	case DSPG_ETH_RMII_EXT:
		reg |= DVF_TX_PAD_OEN;
		fallthrough;
	case DSPG_ETH_RMII_INT:
		reg |= DVF_TX_CLK_SEL | DVF_RX_CLK_SEL;
		break;
	case DSPG_ETH_RGMII:
		reg |= DVF_TX_CLK_SEL | DVF_TX_PAD_SRC_SEL(3);
		break;
	default:
		ret = -EIO;
		goto out;
	}

	iowrite32(reg, eth_clk->reg_ctrl);

out:
	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return ret;
}

static int
dvf_eth_clk_enable(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_en);

	reg |= ETHMAC_CLK_EN;
	if (eth_clk->mode == DSPG_ETH_RMII_INT ||
	    eth_clk->mode == DSPG_ETH_RMII_EXT)
		reg |= RMII_IF_CLK_EN;

	iowrite32(reg, eth_clk->reg_en);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return 0;
}

static void
dvf_eth_clk_disable(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_en);

	reg &= ~ETHMAC_CLK_EN;
	if (eth_clk->mode == DSPG_ETH_RMII_INT ||
	    eth_clk->mode == DSPG_ETH_RMII_EXT)
		reg &= ~RMII_IF_CLK_EN;

	iowrite32(reg, eth_clk->reg_en);

	spin_unlock_irqrestore(&eth_clk->lock, flags);
}

static int
dvf_eth_clk_is_enabled(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	u32 reg = ioread32(eth_clk->reg_en);

	if (reg & ETHMAC_CLK_EN)
		return 1;

	return 0;
}

static long
dvf_round_int_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		   long main_clk_rate, u32 *div_out)
{
	long input;
	u32 div;

	if (!rate)
		return -EINVAL;

	switch (eth_clk->mode) {
	case DSPG_ETH_RGMII:
	case DSPG_ETH_RMII_INT:
		input = main_clk_rate;
		break;
	case DSPG_ETH_RMII_EXT:
		input = 50000000;
		break;
	default:
		/* no need to use internal divider */
		return 0;
	}

	if (input < 0)
		return -EINVAL;

	div = DIV_ROUND_UP(input, rate);
	if (div != 1 && div != 2 && div != 10 && div != 20 && div != 50)
		/* illegal divider value */
		return -EIO;

	*div_out = div;

	return (long)(input / div);
}

static int
dvf_set_int_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		 long main_clk_rate)
{
	unsigned long flags;
	u32 div, reg;
	long ret;

	ret = dvf_round_int_rate(eth_clk, rate, main_clk_rate, &div);
	if (ret <= 0)
		/* error or internal divider not used */
		return ret;

	spin_lock_irqsave(&eth_clk->lock, flags);

	if (eth_clk->mode == DSPG_ETH_RGMII) {
		reg = ioread32(eth_clk->reg_ctrl);
		reg &= ~(DVF_TX_PAD_SRC_SEL(3));
		if (rate != 125000000)
			reg |= DVF_TX_PAD_SRC_SEL(1);
		else
			reg |= DVF_TX_PAD_SRC_SEL(3);
		iowrite32(reg, eth_clk->reg_ctrl);
	}

	reg = ioread32(eth_clk->reg_div);
	reg &= ~DVF_INT_DIV_VAL(0x3F);
	reg |=  DVF_INT_DIV_VAL(div - 1);
	iowrite32(reg, eth_clk->reg_div);

	reg = ioread32(eth_clk->reg_en);
	reg |= CMU_INT_ETHERDIV_EN;
	iowrite32(reg, eth_clk->reg_en);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return 0;
}

static long
dvf_round_main_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		    unsigned long parent_rate, u32 *div_out)
{
	unsigned long div;

	if (!parent_rate)
		return -EINVAL;

	div = DIV_ROUND_UP(parent_rate, rate);
	if (!div)
		return -EIO;

	*div_out = div;

	return (long)(parent_rate / div);
}

static long
dvf_set_main_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		  unsigned long parent_rate)
{
	unsigned long flags;
	u32 div, reg;
	long ret;

	ret = dvf_round_main_rate(eth_clk, rate, parent_rate, &div);
	if (ret < 0)
		return ret;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_div);
	reg &= ~DVF_DIV_VAL(0xFF);
	reg |=  DVF_DIV_VAL(div - 1);
	iowrite32(reg, eth_clk->reg_div);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return parent_rate / div;
}

static int
dvf_eth_clk_set_rate(struct clk_hw *hw, unsigned long rate,
		     unsigned long parent_rate)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	long main_rate = 0;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
	case DSPG_ETH_MII_EXT:
		main_rate = dvf_set_main_rate(eth_clk, 25000000, parent_rate);
		break;
	case DSPG_ETH_RMII_INT:
		main_rate = dvf_set_main_rate(eth_clk, 50000000, parent_rate);
		break;
	case DSPG_ETH_RGMII:
		main_rate = dvf_set_main_rate(eth_clk, 125000000, parent_rate);
		break;
	default:
		break;
	}

	return dvf_set_int_rate(eth_clk, rate, main_rate);
}

static unsigned long
dvf_eth_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	u32 ctrl_reg = ioread32(eth_clk->reg_ctrl);
	u32 ctrl1_reg = ioread32(eth_clk->reg_ctrl1);
	u32 div_reg = ioread32(eth_clk->reg_div);
	u32 en_reg = ioread32(eth_clk->reg_en);
	u32 int_div, main_div;

	/* vicariously use TX clock frequency for ethernet clock */
	if (!(ctrl_reg >> 23 & 0x1)) {
		/* external clock from pad */
		switch (eth_clk->mode) {
		case DSPG_ETH_MII_INT:
		case DSPG_ETH_MII_EXT:
			return 25000000;
		case DSPG_ETH_RMII_INT:
		case DSPG_ETH_RMII_EXT:
			return 50000000;
		case DSPG_ETH_RGMII:
			return 125000000;
		default:
			/* TODO */
			return 0;
		}
	}

	int_div = (en_reg & (1 << 17)) ? (div_reg & 0x3F) : 0;
	int_div++;

	if (ctrl1_reg >> 23 & 0x1) {
		/* internal divider source is external clock from pad */
		switch (eth_clk->mode) {
		case DSPG_ETH_MII_INT:
		case DSPG_ETH_MII_EXT:
			return 25000000;
		case DSPG_ETH_RMII_INT:
		case DSPG_ETH_RMII_EXT:
			return 50000000;
		case DSPG_ETH_RGMII:
			return 125000000;
		default:
			/* TODO */
			return 0;
		}
	}

	main_div = (div_reg >> 20) & 0xFF;
	main_div++;

	return parent_rate / main_div / int_div;
}

static long
dvf_eth_clk_round_rate(struct clk_hw *hw, unsigned long rate,
		       unsigned long *parent_rate)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	long main_rate = 0;
	long int_rate;
	u32 foo;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		main_rate = dvf_round_main_rate(eth_clk, 25000000,
						*parent_rate, &foo);
		break;
	case DSPG_ETH_RMII_INT:
		main_rate = dvf_round_main_rate(eth_clk, 50000000,
						*parent_rate, &foo);
		break;
	case DSPG_ETH_RGMII:
		main_rate = dvf_round_main_rate(eth_clk, 125000000,
						*parent_rate, &foo);
		break;
	default:
		break;
	}

	int_rate = dvf_round_int_rate(eth_clk, rate, main_rate, &foo);
	if (!int_rate)
		/* internal divider not used */
		return main_rate;

	/* error or real rate */
	return int_rate;
}

static const struct clk_ops dspg_dvf_eth_clk_ops = {
	.prepare = dvf_eth_clk_prepare,
	.enable = dvf_eth_clk_enable,
	.disable = dvf_eth_clk_disable,
	.is_enabled = dvf_eth_clk_is_enabled,
	.set_rate = dvf_eth_clk_set_rate,
	.recalc_rate = dvf_eth_clk_recalc_rate,
	.round_rate = dvf_eth_clk_round_rate,
};

static int
dvf101_eth_clk_prepare(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;
	int ret = 0;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_ctrl);

	reg &= ~(DVF101_TX_PAD_SRC_SEL | DVF101_RX_CLK_SEL |
		 DVF101_TX_CLK_SEL | DVF101_INT_DIV_SRC_SEL |
		 DVF101_INT_DIV_POL | DVF101_RX_PAD_OEN | DVF101_TX_PAD_OEN);

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		reg |= DVF101_TX_PAD_SRC_SEL;
		break;
	case DSPG_ETH_MII_EXT:
		reg |= DVF101_RX_PAD_OEN | DVF101_TX_PAD_OEN;
		break;
	case DSPG_ETH_RMII_EXT:
		reg |= DVF101_TX_PAD_OEN;
		fallthrough;
	case DSPG_ETH_RMII_INT:
		reg |= DVF101_INT_DIV_SRC_SEL | DVF101_TX_CLK_SEL |
		       DVF101_RX_CLK_SEL | DVF101_INT_DIV_POL;
		break;
	case DSPG_ETH_RGMII:
		reg |= DVF101_TX_CLK_SEL | DVF101_RX_PAD_OEN;
		break;
	default:
		ret = -EIO;
		goto out;
	}

	reg |= DVF101_AXI_CLK_EN;

	iowrite32(reg, eth_clk->reg_ctrl);
out:
	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return ret;
}

static void
dvf101_eth_clk_unprepare(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_ctrl);

	/* TODO: check */
	/*reg &= ~DVF101_AXI_CLK_EN;*/

	iowrite32(reg, eth_clk->reg_ctrl);

	spin_unlock_irqrestore(&eth_clk->lock, flags);
}

static int
dvf101_eth_clk_enable(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_ctrl);
	reg |= DVF101_CORE_CLK_EN;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		reg |= DVF101_MAIN_CLK_EN;
		reg |= DVF101_INT_DIV_EN;
		break;
	case DSPG_ETH_RMII_INT:
		reg |= DVF101_MAIN_CLK_EN;
		fallthrough;
	case DSPG_ETH_RMII_EXT:
		reg |= DVF101_RMII_IF_CLK_EN;
		reg |= DVF101_INT_DIV_EN;
		break;
	case DSPG_ETH_RGMII:
		reg |= DVF101_PHASE_SHIFT_EN;
		reg |= DVF101_MAIN_CLK_EN;
		break;
	}

	iowrite32(reg, eth_clk->reg_ctrl);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return 0;
}

static void
dvf101_eth_clk_disable(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_ctrl);
	reg &= ~DVF101_CORE_CLK_EN;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		reg &= ~DVF101_MAIN_CLK_EN;
		reg &= ~DVF101_INT_DIV_EN;
		break;
	case DSPG_ETH_RMII_INT:
		reg &= ~DVF101_MAIN_CLK_EN;
		fallthrough;
	case DSPG_ETH_RMII_EXT:
		reg &= ~DVF101_RMII_IF_CLK_EN;
		reg &= ~DVF101_INT_DIV_EN;
		break;
	case DSPG_ETH_RGMII:
		reg &= ~DVF101_PHASE_SHIFT_EN;
		reg &= ~DVF101_MAIN_CLK_EN;
		break;
	}

	iowrite32(reg, eth_clk->reg_ctrl);

	spin_unlock_irqrestore(&eth_clk->lock, flags);
}

static int
dvf101_eth_clk_is_enabled(struct clk_hw *hw)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	u32 reg = ioread32(eth_clk->reg_ctrl);

	if (reg & DVF101_CORE_CLK_EN)
		return 1;

	return 0;
}

static long
dvf101_round_int_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		      long main_clk_rate, u32 *div_out)
{
	long input;
	u32 div;

	if (!rate)
		return -EINVAL;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
	case DSPG_ETH_RMII_INT:
		input = main_clk_rate;
		break;
	case DSPG_ETH_RMII_EXT:
		input = 50000000;
		break;
	default:
		/* no need to use internal divider */
		return 0;
	}

	if (input < 0)
		return -EINVAL;

	div = DIV_ROUND_UP(input, rate);
	if (div != 1 && div != 2 && div != 10 && div != 20 && div != 50)
		/* illegal divider value */
		return -EIO;

	*div_out = div;

	return (long)(input / div);
}

static int
dvf101_set_int_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		    long main_clk_rate)
{
	unsigned long flags;
	u32 div, reg;
	long ret;

	ret = dvf101_round_int_rate(eth_clk, rate, main_clk_rate, &div);
	if (ret <= 0)
		/* error or internal divider not used */
		return ret;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_div);
	reg &= ~DVF101_INT_DIV_VAL(0xFFFF);
	reg |=  DVF101_INT_DIV_VAL(div - 1);
	iowrite32(reg, eth_clk->reg_div);

	reg = ioread32(eth_clk->reg_ctrl);
	reg |= DVF101_INT_DIV_EN;
	iowrite32(reg, eth_clk->reg_ctrl);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return 0;
}

static long
dvf101_round_main_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		       unsigned long parent_rate, u32 *div_out)
{
	unsigned long div;

	if (!parent_rate)
		return -EINVAL;

	div = DIV_ROUND_UP(parent_rate, rate);
	if (div % 2)
		div--;
	if (!div)
		return -EIO;

	*div_out = div;

	return (long)(parent_rate / div);
}

static long
dvf101_set_main_rate(struct dspg_eth_clk *eth_clk, unsigned long rate,
		     unsigned long parent_rate)
{
	unsigned long flags;
	u32 div, reg;
	long ret;

	ret = dvf101_round_main_rate(eth_clk, rate, parent_rate, &div);
	if (ret < 0)
		return ret;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg = ioread32(eth_clk->reg_div);
	reg &= ~DVF101_MAIN_DIV_VAL(0xFFFF);
	reg |=  DVF101_MAIN_DIV_VAL(div - 1);
	iowrite32(reg, eth_clk->reg_div);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return parent_rate / div;
}

static int
dvf101_eth_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	long main_rate = 0;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		main_rate =
			dvf101_set_main_rate(eth_clk, 25000000, parent_rate);
		break;
	case DSPG_ETH_RMII_INT:
		main_rate =
			dvf101_set_main_rate(eth_clk, 50000000, parent_rate);
		break;
	case DSPG_ETH_RGMII:
		main_rate =
			dvf101_set_main_rate(eth_clk, rate, parent_rate);
		break;
	default:
		break;
	}

	return dvf101_set_int_rate(eth_clk, rate, main_rate);
}

static unsigned long
dvf101_eth_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	u32 ctrl_reg = ioread32(eth_clk->reg_ctrl);
	u32 div_reg = ioread32(eth_clk->reg_div);
	u32 int_div, main_div;

	/* vicariously use TX clock frequency for ethernet clock */
	if (!(ctrl_reg >> 16 & 0x1)) {
		/* external clock from pad */
		switch (eth_clk->mode) {
		case DSPG_ETH_MII_INT:
		case DSPG_ETH_MII_EXT:
		case DSPG_ETH_RMII_INT:
		case DSPG_ETH_RMII_EXT:
		case DSPG_ETH_RGMII:
		default:
			/* TODO */
			return 0;
		}
	}

	int_div = (div_reg >> 12 & 0x1) ? (div_reg >> 24 & 0x3F) : 0;
	int_div++;

	if (ctrl_reg >> 10 & 0x1) {
		/* internal divider source is external clock from pad */
		switch (eth_clk->mode) {
		case DSPG_ETH_MII_INT:
		case DSPG_ETH_MII_EXT:
		case DSPG_ETH_RMII_INT:
		case DSPG_ETH_RMII_EXT:
		case DSPG_ETH_RGMII:
		default:
			/* TODO */
			return 0;
		}
	}

	main_div = div_reg & 0x1FF;
	main_div++;

	return parent_rate / main_div / int_div;
}

static long
dvf101_eth_clk_round_rate(struct clk_hw *hw, unsigned long rate,
			  unsigned long *parent_rate)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	long main_rate = 0;
	long int_rate;
	u32 foo;

	switch (eth_clk->mode) {
	case DSPG_ETH_MII_INT:
		main_rate =
			dvf101_round_main_rate(eth_clk, 25000000,
					       *parent_rate, &foo);
		break;
	case DSPG_ETH_RMII_INT:
		main_rate =
			dvf101_round_main_rate(eth_clk, 50000000,
					       *parent_rate, &foo);
		break;
	case DSPG_ETH_RGMII:
		main_rate =
			dvf101_round_main_rate(eth_clk, rate,
					       *parent_rate, &foo);
		break;
	default:
		break;
	}

	int_rate = dvf101_round_int_rate(eth_clk, rate, main_rate, &foo);
	if (!int_rate)
		/* internal divider not used */
		return main_rate;
	/* error or real rate */
	return int_rate;
}

static int
dvf101_eth_clk_set_phase(struct clk_hw *hw, int degrees)
{
	struct dspg_eth_clk *eth_clk = to_dspg_eth_clk(hw);
	unsigned long flags;
	u32 reg_ctrl, main_div_val, degrees_per_unit, phase_val;
	int ret = 0;

	spin_lock_irqsave(&eth_clk->lock, flags);

	reg_ctrl = ioread32(eth_clk->reg_ctrl);
	reg_ctrl &= ~DVF101_PHASE_SHIFT_EN;
	iowrite32(reg_ctrl, eth_clk->reg_ctrl);

	main_div_val = (ioread32(eth_clk->reg_div) & 0x1FF) + 1;
	degrees_per_unit = (180 << 8) / main_div_val;
	phase_val = (degrees << 8) / degrees_per_unit;

	if (phase_val > 0x7)
		ret = -EIO;
	else
		reg_ctrl |= DVF101_TX_CLK_PHASE_SEL(phase_val);

	reg_ctrl |= DVF101_PHASE_SHIFT_EN;
	iowrite32(reg_ctrl, eth_clk->reg_ctrl);

	spin_unlock_irqrestore(&eth_clk->lock, flags);

	return ret;
}

static const struct clk_ops dspg_dvf101_eth_clk_ops = {
	.prepare = dvf101_eth_clk_prepare,
	.unprepare = dvf101_eth_clk_unprepare,
	.enable = dvf101_eth_clk_enable,
	.disable = dvf101_eth_clk_disable,
	.is_enabled = dvf101_eth_clk_is_enabled,
	.set_rate = dvf101_eth_clk_set_rate,
	.set_phase = dvf101_eth_clk_set_phase,
	.recalc_rate = dvf101_eth_clk_recalc_rate,
	.round_rate = dvf101_eth_clk_round_rate,
};

static const struct of_device_id dspg_eth_clk_of_match[] = {
	{
		.compatible = "dspg,eth-clk-dvf",
		.data = &dspg_dvf_eth_clk_ops,
	},
	{
		.compatible = "dspg,eth-clk-dvf101",
		.data = &dspg_dvf101_eth_clk_ops,
	},
	{ /* sentinel */ }
};

void __init
of_dspg_eth_clk_setup(struct device_node *node)
{
	struct dspg_eth_clk *priv;
	int ret;
	struct clk_init_data init;
	struct clk *clk;
	const char *parent_name;
	const struct of_device_id *match;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return;

	parent_name = of_clk_get_parent_name(node, 0);
	if (!parent_name) {
		pr_err("eth-clk: no valid parent\n");
		goto free;
	}

	priv->reg_ctrl = of_iomap(node, 0);
	if (!priv->reg_ctrl) {
		pr_err("eth-clk: could not get I/O mapping\n");
		goto free;
	}
	priv->reg_div = of_iomap(node, 1);
	if (!priv->reg_div) {
		pr_err("eth-clk: could not get I/O mapping\n");
		goto free;
	}
	priv->reg_ctrl1 = of_iomap(node, 2);
	if (!priv->reg_ctrl1 &&
			of_device_is_compatible(node, "dspg,eth-clk-dvf")) {
		pr_err("eth-clk: could not get I/O mapping\n");
		goto free;
	}
	priv->reg_en = of_iomap(node, 3);
	if (!priv->reg_en &&
			of_device_is_compatible(node, "dspg,eth-clk-dvf")) {
		pr_err("eth-clk: could not get I/O mapping\n");
		goto free;
	}

	ret = of_property_read_u32(node, "dspg-mode", &priv->mode);
	if (ret) {
		pr_err("eth-clk: could not get 'dspg-mode' property\n");
		goto free;
	}

	match = of_match_node(dspg_eth_clk_of_match, node);
	if (match)
		init.ops = match->data;
	else
		init.ops = &dspg_dvf101_eth_clk_ops;

	init.name = node->name;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	priv->hw.init = &init;
	spin_lock_init(&priv->lock);

	clk = clk_register(NULL, &priv->hw);
	if (IS_ERR(clk)) {
		pr_err("eth-clk registration failed with %ld\n", PTR_ERR(clk));
		goto free;
	}
	of_clk_add_provider(node, of_clk_src_simple_get, clk);

	return;
free:
	kfree(priv);
}
CLK_OF_DECLARE(eth_clk, "dspg,eth-clk", of_dspg_eth_clk_setup);
