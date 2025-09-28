/*
 *  Copyright (C) 2020 DSPG Technologies GmbH
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
#include <dt-bindings/reset-controller/dvf99-resets.h>

#define USBOTG1_GCR1_SUSPEND		(1 << 19)
#define USBOTG1_GCR1_SUSPEND_MODE	(1 << 20)

#define CMU_CLKOSCCR_12M_OSC_EN		(1 << 2)
#define CMU_CLKOSCCR_12M_OSC_STAT	(1 << 5)

#define to_dspg_usb_clk(_hw)	container_of(_hw, struct dspg_usb_clk, hw)

struct dspg_usb_clk {
	struct clk_hw		hw;
	void __iomem		*reg_ctrl;
	void __iomem		*reg_osccr;
	void __iomem		*reg_phy;
	void __iomem		*reg_reset;
	spinlock_t		lock;
	u32			en_phy;
	u32			en_mac;
	int			enabled;
};

static void
dvf_usb_clk_mac(struct dspg_usb_clk *usb_clk, int en)
{
	u32 reg;

	reg = ioread32(usb_clk->reg_ctrl);
	if (en)
		reg |= 1 << usb_clk->en_mac;
	else
		reg &= ~(1 << usb_clk->en_mac);
	iowrite32(reg, usb_clk->reg_ctrl);
}

static void
dvf_usb_clk_phy(struct dspg_usb_clk *usb_clk, int en)
{
	u32 reg;

	reg = ioread32(usb_clk->reg_ctrl);
	if (en)
		reg |= 1 << usb_clk->en_phy;
	else
		reg &= ~(1 << usb_clk->en_phy);
	iowrite32(reg, usb_clk->reg_ctrl);
}

static void
dvf_usb_cfg_phy(struct dspg_usb_clk *usb_clk, int cfg)
{
	u32 reg;

	reg = ioread32(usb_clk->reg_phy);
	reg |= cfg;
	iowrite32(reg, usb_clk->reg_phy);
}

static void
dvf_usb_reset(struct dspg_usb_clk *usb_clk, int block, int reset)
{
	u32 reg, offset = 0, bit = block;

	if (bit >= 32) {
		offset += 4;
		bit -= 32;
	}

	reg = ioread32(usb_clk->reg_reset + offset);
	if (reset)
		reg |= 1 << bit;
	else
		reg &= ~(1 << bit);
	iowrite32(reg, usb_clk->reg_reset + offset);
}

static int
dvf_usb_clk_enable(struct clk_hw *hw)
{
	struct dspg_usb_clk *usb_clk = to_dspg_usb_clk(hw);
	u32 reg;

	if (usb_clk->enabled)
		return 0;

	reg = ioread32(usb_clk->reg_osccr);
	reg |= CMU_CLKOSCCR_12M_OSC_EN;
	iowrite32(reg, usb_clk->reg_osccr);

	/* wait until stable */
	do {
		reg = ioread32(usb_clk->reg_osccr);
	} while (!(reg & CMU_CLKOSCCR_12M_OSC_STAT));

	/* Special power-up sequence needed */
	dvf_usb_clk_phy(usb_clk, 1);

	udelay(20);

	dvf_usb_cfg_phy(usb_clk, USBOTG1_GCR1_SUSPEND);

	mdelay(7);

	dvf_usb_reset(usb_clk, DVF_99_USB_PHY_POR_RESET, 0);

	dvf_usb_clk_mac(usb_clk, 1);

	udelay(20);

	dvf_usb_clk_mac(usb_clk, 0);

	dvf_usb_reset(usb_clk, DVF_99_USB_MAC_PHYIF_RESET, 0);

	dvf_usb_clk_mac(usb_clk, 1);

	udelay(20);

	dvf_usb_clk_mac(usb_clk, 0);

	dvf_usb_reset(usb_clk, DVF_99_USB_MAC_RESET, 0);

	dvf_usb_clk_mac(usb_clk, 1);

	dvf_usb_cfg_phy(usb_clk, USBOTG1_GCR1_SUSPEND_MODE);

	udelay(20);

	usb_clk->enabled = 1;

	return 0;
}

static void
dvf_usb_clk_disable(struct clk_hw *hw)
{
	/* Keep clock enabled. */
}

static int
dvf_usb_clk_is_enabled(struct clk_hw *hw)
{
	struct dspg_usb_clk *usb_clk = to_dspg_usb_clk(hw);

	if (ioread32(usb_clk->reg_ctrl) & 1 << usb_clk->en_mac)
		return 1;

	return 0;
}

static const struct clk_ops dspg_dvf_usb_clk_ops = {
	.enable = dvf_usb_clk_enable,
	.disable = dvf_usb_clk_disable,
	.is_enabled = dvf_usb_clk_is_enabled,
};

void __init
of_dspg_usb_clk_setup(struct device_node *node)
{
	struct dspg_usb_clk *priv;
	int ret;
	struct clk_init_data init;
	struct clk *clk;
	const char *parent_name;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return;

	parent_name = of_clk_get_parent_name(node, 0);
	if (!parent_name) {
		pr_err("usb-clk: no valid parent\n");
		goto free;
	}

	priv->reg_ctrl = of_iomap(node, 0);
	if (!priv->reg_ctrl) {
		pr_err("usb-clk: could not get I/O mapping\n");
		goto free;
	}
	priv->reg_osccr = of_iomap(node, 1);
	if (!priv->reg_osccr) {
		pr_err("usb-clk: could not get I/O mapping\n");
		goto free;
	}
	priv->reg_phy = of_iomap(node, 2);
	if (!priv->reg_phy) {
		pr_err("usb-clk: could not get I/O mapping\n");
		goto free;
	}
	priv->reg_reset = of_iomap(node, 3);
	if (!priv->reg_reset) {
		pr_err("usb-clk: could not get I/O mapping\n");
		goto free;
	}

	ret = of_property_read_u32(node, "en_phy", &priv->en_phy);
	if (ret) {
		pr_err("usb-clk: could not get 'en_phy' property\n");
		goto free;
	}

	ret = of_property_read_u32(node, "en_mac", &priv->en_mac);
	if (ret) {
		pr_err("usb-clk: could not get 'en_mac' property\n");
		goto free;
	}

	init.ops = &dspg_dvf_usb_clk_ops;
	init.name = "usb";
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	priv->hw.init = &init;
	spin_lock_init(&priv->lock);

	clk = clk_register(NULL, &priv->hw);
	if (IS_ERR(clk)) {
		pr_err("usb-clk registration failed with %ld\n", PTR_ERR(clk));
		goto free;
	}
	of_clk_add_provider(node, of_clk_src_simple_get, clk);

	return;
free:
	kfree(priv);
}
CLK_OF_DECLARE(usb_clk, "dspg,usb-clk", of_dspg_usb_clk_setup);
