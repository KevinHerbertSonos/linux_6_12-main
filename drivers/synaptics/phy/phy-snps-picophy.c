/*
 * Copyright (c) 2016, DSP Group.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/phy/phy.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>

#define DRIVER_NAME			"snps_picophy"

#define PHY_STRAP_txrestune_shift		30
#define PHY_STRAP_txhsxvtune_shift		28
#define PHY_STRAP_txvreftune_shift		24
#define PHY_STRAP_txrisetune_shift		22
#define PHY_STRAP_txpreemppulsetune_shift	21
#define PHY_STRAP_txpreempamptune_shift		19
#define PHY_STRAP_txfslstune_shift		15
#define PHY_STRAP_sqrxtune0_shift		12
#define PHY_STRAP_otgtune0_shift		 9
#define PHY_STRAP_compdistune_shift		 6
#define PHY_STRAP_commononn_shift		 5
#define PHY_STRAP_fsel_shift			 2
#define PHY_STRAP_refclksel_shift		 0

#define PHY_STRAP_txrestune		GENMASK(31, 30)
#define PHY_STRAP_txhsxvtune		GENMASK(29, 28)
#define PHY_STRAP_txvreftune		GENMASK(27, 24)
#define PHY_STRAP_txrisetune		GENMASK(23, 22)
#define PHY_STRAP_txpreemppulsetune	BIT(21)
#define PHY_STRAP_txpreempamptune	GENMASK(20, 19)
#define PHY_STRAP_txfslstune		GENMASK(18, 15)
#define PHY_STRAP_sqrxtune0		GENMASK(14, 12)
#define PHY_STRAP_otgtune0		GENMASK(11,  9)
#define PHY_STRAP_compdistune		GENMASK( 8,  6)
#define PHY_STRAP_commononn		BIT(5)
#define PHY_STRAP_fsel			GENMASK( 4,  2)
#define PHY_STRAP_refclksel		GENMASK( 1,  0)

enum refclksel {
	REFCLK_CRYSTAL = 0,
	REFCLK_EXTERNAL,
	REFCLK_CLKCORE,
};

enum phy_type {
	PHY_USB,
	PHY_USB_AND_CLOCKPROVIDER,
	PHY_CLOCKPROVIDER,
};

struct snps_picophy {
	/* 12MHz OHCI clk */
	struct clk_hw hw;
	struct device *dev;
	unsigned int rate;
	struct clk *clk12mohci;
	unsigned long fixed_rate;
	unsigned long fixed_accuracy;
	unsigned int active;
	unsigned int commonon;
	unsigned int fsel;
	unsigned int pm_off;
	unsigned int bc_enable;
	u32 reg_defaults[3];

	struct clk *refclk;
	struct reset_control *reset;
	enum refclksel refclksel;
	void __iomem *ctrl_regs;
	void __iomem *strap_reg;
	struct mutex lock;
};

static void phy_write_ctrl(struct snps_picophy *phy, unsigned int reg,
			   u32 value, u32 mask, unsigned int shift)
{
	u32 v;

	reg *= sizeof(u32);

	v = ioread32(phy->ctrl_regs + reg);
	v &= ~mask;
	v |= (value << shift) & mask;
	iowrite32(v, phy->ctrl_regs + reg);
}

static u32 phy_read_ctrl(struct snps_picophy *phy, unsigned int reg,
			 u32 mask, unsigned int shift)
{
	u32 v;

	reg *= sizeof(u32);

	v = ioread32(phy->ctrl_regs + reg);
	v &= mask;
	v >>= shift;

	return v;
}

static void phy_sleep_off(struct snps_picophy *phy)
{
	phy_write_ctrl(phy, 0, 1, BIT(7), 7);
}

static void phy_sleep_on(struct snps_picophy *phy)
{
	phy_write_ctrl(phy, 0, 0, BIT(7), 7);
}

static void phy_suspend_on(struct snps_picophy *phy)
{
	phy_write_ctrl(phy, 0, 0, BIT(1), 1);
}

static void phy_suspend_off(struct snps_picophy *phy)
{
	phy_write_ctrl(phy, 0, 1, BIT(1), 1);
}

static void phy_write_strap(struct snps_picophy *phy, u32 value, u32 mask,
			    unsigned int shift)
{
	u32 reg;

	reg = ioread32(phy->strap_reg);
	reg &= ~mask;
	reg |= (value << shift) & mask;
	iowrite32(reg, phy->strap_reg);
}

static u32 phy_read_strap(struct snps_picophy *phy, u32 mask,
			  unsigned int shift)
{
	u32 reg;

	reg = ioread32(phy->strap_reg);
	reg &= mask;
	reg >>= shift;

	return reg;
}

static int phy_reset_release(struct snps_picophy *phy)
{
	int ret;

	if (phy->active)
		return 0;

	clk_enable(phy->refclk);
	ret = reset_control_deassert(phy->reset);
	if (ret) {
		clk_disable(phy->refclk);
		return ret;
	}
	phy->active++;
	return 0;
}
#if 0
static int phy_reset_set(struct snps_picophy *phy)
{
	int ret;

	if (!phy->active)
		return -EINVAL;

	if (phy->active > 1)
		return -EBUSY;

	ret = reset_control_assert(phy->reset);
	if (ret)
		return ret;

	clk_disable(phy->refclk);
	phy->active--;

	return 0;
}
#endif
#define to_phy_clk(_hw) container_of(_hw, struct snps_picophy, hw)

static unsigned long phy_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct snps_picophy *phy = to_phy_clk(hw);

	return phy->fixed_rate;
}

static unsigned long phy_recalc_accuracy(struct clk_hw *hw,
		unsigned long parent_accuracy)
{
	struct snps_picophy *phy = to_phy_clk(hw);

	return phy->fixed_accuracy;
}

static const struct clk_ops phy_clk_ops = {
	.recalc_rate = phy_recalc_rate,
	.recalc_accuracy = phy_recalc_accuracy,
};

static struct clk_init_data phy_clkout_init = {
	.ops = &phy_clk_ops,
	.flags = CLK_IGNORE_UNUSED,
};

static int clk_init(struct platform_device *pdev)
{
	struct snps_picophy *phy = platform_get_drvdata(pdev);
	struct device_node *np = pdev->dev.of_node;
	const char *clk_name;
	int ret;

	ret = of_property_read_u32(np, "clock-frequency", &phy->rate);
	if (ret)
		return ret;

	ret = of_property_read_string_index(np, "clock-output-names", 0,
					    &clk_name);
	if (ret) {
		dev_err(phy->dev, "missing clock output name\n");
		return ret;
	}

	phy->fixed_rate = phy->rate;
	phy->fixed_accuracy = phy->rate;

	phy_clkout_init.name = clk_name;
	phy->hw.init = &phy_clkout_init;

	/* set commononn to 0 to keep the PLL, XO and BIAS clocks enabled */
	phy_write_strap(phy, 0, PHY_STRAP_commononn, PHY_STRAP_commononn_shift);
	phy->commonon = 1;

	phy->clk12mohci = clk_register(phy->dev, &phy->hw);
	if (IS_ERR(phy->clk12mohci)) {
		ret = PTR_ERR(phy->clk12mohci);
		dev_err(phy->dev, "failed to register phy output clock\n");
		return ret;
	}

	ret = phy_reset_release(phy);
	if (ret)
		dev_err(phy->dev, "failed to enable clock\n");

	return 0;
}

static int snps_picophy_init(struct phy *phy)
{
	struct snps_picophy *sphy = phy_get_drvdata(phy);
	int ret;

	mutex_lock(&sphy->lock);
	ret = phy_reset_release(sphy);
	mutex_unlock(&sphy->lock);

	return ret;
}

static int snps_picophy_power_on(struct phy *phy)
{
	struct snps_picophy *sphy = phy_get_drvdata(phy);

	mutex_lock(&sphy->lock);

	if (sphy->pm_off)
		goto out;

	phy_sleep_off(sphy);
	if (sphy->commonon)
		udelay(5);
	else
		usleep_range(45, 50);

out:
	mutex_unlock(&sphy->lock);
	return 0;
}

static int snps_picophy_power_off(struct phy *phy)
{
	struct snps_picophy *sphy = phy_get_drvdata(phy);

	mutex_lock(&sphy->lock);

	if (!sphy->pm_off)
		phy_sleep_on(sphy);

	mutex_unlock(&sphy->lock);

	return 0;
}

static const struct phy_ops snps_picophy_ops = {
	.init		= snps_picophy_init,
	.power_on	= snps_picophy_power_on,
	.power_off	= snps_picophy_power_off,
	.owner		= THIS_MODULE,
};

static const char *xlate_strap_txrestune[] = {
	"1.5ohm", "default", "2ohm", "4ohm",
};

static const char *xlate_strap_txhsxvtune[] = {
	"reserved", "-15mV", "+15mV", "default",
};

static const char *xlate_strap_txvreftune[] = {
	"-6%", "-4%", "-2%", "default", "+2%", "+4%", "+6%", "+8%", "+10%",
	"+12%", "+14%", "+16%", "+18%", "+20%", "+22%", "+24%",
};

static const char *xlate_strap_txrisetune[] = {
	"+10%", "default", "-20%", "-40%",
};

static const char *xlate_strap_txpreemppulsetune[] = {
	"2x long pre-emphasis", "1x short pre-emphasis",
};

static const char *xlate_strap_txpreempamptune[] = {
	"disabled", "1x", "2x", "3x",
};

static const char *xlate_strap_txfslstune[] = {
	"+5%", "+2.5%", "reserved", "default",
	"reserved", "reserved", "reserved", "-2.5%",
	"reserved", "reserved", "reserved", "reserved",
	"reserved", "reserved", "reserved", "-5%",
};

static const char *xlate_strap_sqrxtune0[] = {
	"+15%", "+10%", "+5%", "default", "-5%", "-10%", "-15%", "-20%",
};

static const char *xlate_strap_otgtune0[] = {
	"-12%", "-9%", "-6%", "-3%", "default", "+3%", "+6%", "+9%",
};

static const char *xlate_strap_compdistune[] = {
	"-6%", "-4.5%", "-3%", "-1.5%", "default", "+1.5%", "+3%", "+4.5%",
};

/* RO */
static const char *xlate_strap_commononn[] = {
	"XO, Bias and PLL blocks remain powered",
	"XO, Bias and PLL blocks can be powered down",
};

/* RO */
static const char *xlate_strap_fsel[] = {
	"9.6MHz", "10MHz", "12MHz", "19.2MHz", "20MHz", "24MHz", "reserved",
	"50MHz",
};

/* RO */
static const char *xlate_strap_refclksel[] = {
	"crystal", "XO", "clkcore", "reserved",
};

#define STRAP_ATTR_R(_name)                                                   \
static ssize_t _name ## _show(struct device *dev,                             \
			      struct device_attribute *attr,                  \
			      char *buf)                                      \
{                                                                             \
	struct snps_picophy *phy = dev_get_drvdata(dev);                      \
	ssize_t	status = 0;                                                   \
	u32 reg;                                                              \
                                                                              \
	mutex_lock(&phy->lock);                                               \
	reg = phy_read_strap(phy, PHY_STRAP_ ## _name,                        \
			     PHY_STRAP_ ## _name ## _shift);                  \
	mutex_unlock(&phy->lock);                                             \
                                                                              \
	status = sprintf(buf, "%s\n", xlate_strap_ ## _name [reg]);           \
                                                                              \
	return status;                                                        \
}


#define STRAP_ATTR_W(_name)                                                   \
static ssize_t _name ## _store(struct device *dev,                            \
			       struct device_attribute *attr, const char *buf,\
			       size_t size)                                   \
{                                                                             \
	struct snps_picophy *phy = dev_get_drvdata(dev);                      \
	unsigned long value;                                                  \
	int status;                                                           \
                                                                              \
	status = kstrtoul(buf, 0, &value);                                    \
	if (status == 0) {                                                    \
		mutex_lock(&phy->lock);                                       \
		phy_write_strap(phy, value, PHY_STRAP_ ## _name,              \
				PHY_STRAP_ ## _name ## _shift);               \
		mutex_unlock(&phy->lock);                                     \
	}                                                                     \
                                                                              \
	return status == 0? size: -EINVAL;                                    \
}

#define STRAP_ATTR_RW(_name)                                                  \
	STRAP_ATTR_R(_name)                                                   \
	STRAP_ATTR_W(_name)

STRAP_ATTR_RW(txrestune)
STRAP_ATTR_RW(txhsxvtune)
STRAP_ATTR_RW(txvreftune)
STRAP_ATTR_RW(txrisetune)
STRAP_ATTR_RW(txpreemppulsetune)
STRAP_ATTR_RW(txpreempamptune)
STRAP_ATTR_RW(txfslstune)
STRAP_ATTR_RW(sqrxtune0)
STRAP_ATTR_RW(otgtune0)
STRAP_ATTR_RW(compdistune)
STRAP_ATTR_R(commononn)
STRAP_ATTR_R(fsel)
STRAP_ATTR_R(refclksel)

static DEVICE_ATTR_RW(txrestune);
static DEVICE_ATTR_RW(txhsxvtune);
static DEVICE_ATTR_RW(txvreftune);
static DEVICE_ATTR_RW(txrisetune);
static DEVICE_ATTR_RW(txpreemppulsetune);
static DEVICE_ATTR_RW(txpreempamptune);
static DEVICE_ATTR_RW(txfslstune);
static DEVICE_ATTR_RW(sqrxtune0);
static DEVICE_ATTR_RW(otgtune0);
static DEVICE_ATTR_RW(compdistune);
static DEVICE_ATTR_RO(commononn);
static DEVICE_ATTR_RO(fsel);
static DEVICE_ATTR_RO(refclksel);

static struct attribute *phy_strap_attributes[] = {
	&dev_attr_txrestune.attr,
	&dev_attr_txhsxvtune.attr,
	&dev_attr_txvreftune.attr,
	&dev_attr_txrisetune.attr,
	&dev_attr_txpreemppulsetune.attr,
	&dev_attr_txpreempamptune.attr,
	&dev_attr_txfslstune.attr,
	&dev_attr_sqrxtune0.attr,
	&dev_attr_otgtune0.attr,
	&dev_attr_compdistune.attr,
	&dev_attr_commononn.attr,
	&dev_attr_fsel.attr,
	&dev_attr_refclksel.attr,
	NULL,
};

static const struct attribute_group phy_strap_group = {
	.name = "strap",
	.attrs = phy_strap_attributes,
};

#define CTRL_ATTR_R(_name, _nr)                                               \
static ssize_t _name ## _nr ##  _show(struct device *dev,                     \
			      struct device_attribute *attr,                  \
			      char *buf)                                      \
{                                                                             \
	struct snps_picophy *phy = dev_get_drvdata(dev);                      \
	u32 reg;                                                              \
                                                                              \
	mutex_lock(&phy->lock);                                               \
	reg = phy_read_ctrl(phy, _nr, ~0UL, 0);                               \
	mutex_unlock(&phy->lock);                                             \
                                                                              \
	return sprintf(buf, "0x%x\n", reg);                                   \
}


#define CTRL_ATTR_W(_name, _nr)                                               \
static ssize_t _name ## _nr ##  _store(struct device *dev,                    \
			       struct device_attribute *attr, const char *buf,\
			       size_t size)                                   \
{                                                                             \
	struct snps_picophy *phy = dev_get_drvdata(dev);                      \
	unsigned long value;                                                  \
	int status;                                                           \
                                                                              \
	mutex_lock(&phy->lock);                                               \
	if (!phy->pm_off)                                                     \
		dev_warn(phy->dev, "power management still enabled!");        \
                                                                              \
	status = kstrtoul(buf, 0, &value);                                    \
	if (status == 0)                                                      \
		phy_write_ctrl(phy, _nr, value, ~0UL, 0);                     \
	mutex_unlock(&phy->lock);                                             \
                                                                              \
	return status == 0? size: -EINVAL;                                    \
}

#define CTRL_ATTR_RW(_name, _nr)                                              \
	CTRL_ATTR_R(_name, _nr)                                               \
	CTRL_ATTR_W(_name, _nr)

CTRL_ATTR_RW(ctrl, 0)
CTRL_ATTR_RW(ctrl, 1)

static ssize_t charger_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct snps_picophy *phy = dev_get_drvdata(dev);

	dev_dbg(dev, "charger enable is: %d\n", phy->bc_enable);

	return sprintf(buf, "%u\n", phy->bc_enable);
}

static ssize_t charger_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t size)
{
	u32 enable;
	struct snps_picophy *phy = dev_get_drvdata(dev);

	if (kstrtoul(buf, 0, (unsigned long *)&enable)) {
		dev_err(dev, "enter a number!\n");
		return -EINVAL;
	}

	if (enable == 1) {
		phy_write_ctrl(phy, 0, 1, BIT(6), 6); /* Enable CHRGSEL */
		phy_write_ctrl(phy, 0, 1, BIT(5), 5); /* Enable VDATDET */
		usleep_range(500, 550);
		phy_write_ctrl(phy, 0, 1, BIT(4), 4); /* Enable VDATSRC */
		phy->bc_enable = enable;
	} else if (enable == 0) {
		phy_write_ctrl(phy, 0, 0, BIT(6), 6); /* Disable CHRGSEL */
		phy_write_ctrl(phy, 0, 0, BIT(5), 5); /* Disable VDATDET */
		phy_write_ctrl(phy, 0, 0, BIT(4), 4); /* Disable VDATSRC */
		phy->bc_enable = enable;
	} else
		return -EINVAL;

	return size;
}

static ssize_t pm_off_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return 0;
}


static ssize_t pm_off_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	struct snps_picophy *phy = dev_get_drvdata(dev);
	unsigned long value;
	int status;

	/* if to be switched off, take out of sleep/suspend */
	mutex_lock(&phy->lock);
	status = kstrtoul(buf, 0, &value);
	if (status == 0) {
		value = !!value;
		if (!phy->pm_off && value) {
			/* about to be switched off */
			phy_sleep_off(phy);
			phy_suspend_off(phy);
			usleep_range(1000, 1100);
		}
		phy->pm_off = value;
	}
	mutex_unlock(&phy->lock);
	return 0;
}

static DEVICE_ATTR_RW(pm_off);
static DEVICE_ATTR_RW(ctrl0);
static DEVICE_ATTR_RW(ctrl1);

static struct attribute *phy_ctrl_attributes[] = {
	&dev_attr_pm_off.attr,
	&dev_attr_ctrl0.attr,
	&dev_attr_ctrl1.attr,
	NULL,
};

static const struct attribute_group phy_ctrl_group = {
	.name = "control",
	.attrs = phy_ctrl_attributes,
};

static DEVICE_ATTR_RW(charger);

static struct attribute *battery_charging_attributes[] = {
	&dev_attr_charger.attr,
	NULL,
};

static const struct attribute_group battery_charging_group = {
	.name = "battery_charging",
	.attrs = battery_charging_attributes,
};

static const struct of_device_id snps_picophy_dt_match[] = {
	{
		.compatible = "dspg,snps-usb-picophy",
		.data = (void *)PHY_USB,
	},
	{
		.compatible = "dspg,snps-usb-picophy-clock",
		.data = (void *)PHY_USB_AND_CLOCKPROVIDER,
	},
	{
		.compatible = "dspg,snps-usb-picophy-clock-only",
		.data = (void *)PHY_CLOCKPROVIDER,
	},
	{ }
};
MODULE_DEVICE_TABLE(of, snps_picophy_dt_match);

static const unsigned long phy_rates[] = {
	50000000, 24000000, 20000000, 19200000, 12000000, 10000000, 9600000,
};

static int snps_picophy_probe(struct platform_device *pdev)
{
	int ret, i;
	struct phy *phy;
	unsigned long rate = 0;
	u32 preferred_rate;
	struct resource *res;
	const struct of_device_id *match;
	struct snps_picophy *snps_phy;
	struct phy_provider *phy_provider;
	enum phy_type type;

	snps_phy = devm_kzalloc(&pdev->dev, sizeof(*snps_phy), GFP_KERNEL);
	if (!snps_phy)
		return -ENOMEM;
	snps_phy->dev = &pdev->dev;

	mutex_init(&snps_phy->lock);

	snps_phy->refclk = devm_clk_get(&pdev->dev, "osc12");
	if (IS_ERR(snps_phy->refclk)) {
		snps_phy->refclk = devm_clk_get(&pdev->dev, "extclk");
		if (IS_ERR(snps_phy->refclk)) {
			snps_phy->refclk = devm_clk_get(&pdev->dev, "clkcore");
			if (IS_ERR(snps_phy->refclk)) {
				dev_err(&pdev->dev, "cannot get clock %ld\n", PTR_ERR(snps_phy->refclk));
				return PTR_ERR(snps_phy->refclk);
			} else
				snps_phy->refclksel = REFCLK_CLKCORE;
		} else
			snps_phy->refclksel = REFCLK_EXTERNAL;
	} else {
		snps_phy->refclksel = REFCLK_CRYSTAL;
		rate = 12000000;
	}

	snps_phy->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(snps_phy->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		return PTR_ERR(snps_phy->reset);
	}

	ret = reset_control_assert(snps_phy->reset);
	if (ret)
		return ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ctrl");
	snps_phy->ctrl_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(snps_phy->ctrl_regs))
		return PTR_ERR(snps_phy->ctrl_regs);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "strap");
	snps_phy->strap_reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(snps_phy->strap_reg))
		return PTR_ERR(snps_phy->strap_reg);

	platform_set_drvdata(pdev, snps_phy);
	dev_set_drvdata(&pdev->dev, snps_phy);

	match = of_match_node(snps_picophy_dt_match, pdev->dev.of_node);
	if (!match)
		return -EINVAL;
	type = (enum phy_type)match->data;

	/* read in and set default values */
	ret = of_property_read_u32_array(pdev->dev.of_node, "init-values",
					 snps_phy->reg_defaults, 3);
	if (ret == 0) {
		dev_info(&pdev->dev, "setting default values:");
		dev_info(&pdev->dev, "  strap: 0x%8.8x",
			 snps_phy->reg_defaults[0]);
		dev_info(&pdev->dev, "  ctrl1: 0x%8.8x",
			 snps_phy->reg_defaults[1]);
		dev_info(&pdev->dev, "  ctrl2: 0x%8.8x",
			 snps_phy->reg_defaults[2]);
		phy_write_strap(snps_phy, snps_phy->reg_defaults[0], ~0UL, 0);
		phy_write_ctrl(snps_phy, 0, snps_phy->reg_defaults[1],
			       ~0x300UL, 0); /* Do not touch D+/D- pull-down */
		phy_write_ctrl(snps_phy, 1, snps_phy->reg_defaults[2], ~0UL, 0);
	}
	if (ret && ret != -EINVAL)
		dev_err(&pdev->dev, "invalid init-values");

	preferred_rate = 0;
	ret = of_property_read_u32(pdev->dev.of_node, "preferred-rate",
				   &preferred_rate);

	if (snps_phy->refclksel == REFCLK_CLKCORE) {
		if (preferred_rate) {
			ret = clk_set_rate(snps_phy->refclk, preferred_rate);
			if (!ret)
				rate = preferred_rate;
		}
		if (!rate) {
			for (i = 0; i < ARRAY_SIZE(phy_rates); i++) {
				/* set desired rate */
				ret = clk_set_rate(snps_phy->refclk,
						   phy_rates[i]);
				if (ret) {
					dev_err(&pdev->dev,
					"failed during set-rate for %lu",
						phy_rates[i]);
					return ret;
				}

				rate = clk_get_rate(snps_phy->refclk);
				if (rate == phy_rates[i])
					break;
			}
		}
	}
	clk_prepare_enable(snps_phy->refclk);

	/* set phy clock strap configuration */
	switch (rate) {
	case 9600000:
		snps_phy->fsel = 0;
		break;
	case 10000000:
		snps_phy->fsel = 1;
		break;
	case 12000000:
		snps_phy->fsel = 2;
		break;
	case 19200000:
		snps_phy->fsel = 3;
		break;
	case 20000000:
		snps_phy->fsel = 4;
		break;
	case 24000000:
		snps_phy->fsel = 5;
		break;
	case 50000000:
		snps_phy->fsel = 7;
		break;
	default:
		dev_err(&pdev->dev, "invalid input frequency: %lu\n", rate);
		return -EINVAL;
	}
	dev_info(&pdev->dev, "input frequency: %lu\n", rate);
	phy_write_strap(snps_phy, snps_phy->fsel, PHY_STRAP_fsel,
			PHY_STRAP_fsel_shift);
	phy_write_strap(snps_phy, snps_phy->refclksel, PHY_STRAP_refclksel,
			PHY_STRAP_refclksel_shift);

	/* init clock provider if not just configured for USB phy */
	if (type != PHY_USB) {
		ret = clk_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "clock init failed\n");
			return ret;
		}
	}

	/* register phy if not just configured as clock provider */
	if (type != PHY_CLOCKPROVIDER) {
		phy = devm_phy_create(&pdev->dev, NULL, &snps_picophy_ops);
		if (IS_ERR(phy)) {
			dev_err(&pdev->dev, "failed to create phy\n");
			return PTR_ERR(phy);
		}

		phy_set_bus_width(phy, 8);
		phy_set_drvdata(phy, snps_phy);

		phy_provider = devm_of_phy_provider_register(&pdev->dev,
							     of_phy_simple_xlate);
		if (IS_ERR_OR_NULL(phy_provider)) {
			dev_err(&pdev->dev, "failed to register phy\n");
			return !phy_provider? -ENOSYS: PTR_ERR(phy_provider);
		}
	}


	ret = sysfs_create_group(&pdev->dev.kobj, &phy_strap_group);
	if (ret) {
		dev_err(&pdev->dev, "failed to create sysfs strap group\n");
		return ret;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &phy_ctrl_group);
	if (ret) {
		dev_err(&pdev->dev, "failed to create sysfs ctrl group\n");
		return ret;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &battery_charging_group);
	if (ret) {
		dev_err(&pdev->dev,
			"failed to create sysfs battery charging group\n");
		return ret;
	}

	dev_info(&pdev->dev, "successfully registered phy %s\n",
		 type != PHY_USB? "with clock provider": "");
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static const unsigned int phy_suspend_udelay[] = {
	24, 25, 30, 48, 50, 60, 0, 125,
};


static int snps_picophy_pm_suspend(struct device *dev)
{
	struct snps_picophy *phy = dev_get_drvdata(dev);

	mutex_lock(&phy->lock);

	if (phy->pm_off)
		goto out;

	phy_suspend_on(phy);
	usleep_range(phy_suspend_udelay[phy->fsel],
		     phy_suspend_udelay[phy->fsel] + 5);

out:
	mutex_unlock(&phy->lock);
	return 0;
}

static int snps_picophy_pm_resume(struct device *dev)
{
	struct snps_picophy *phy = dev_get_drvdata(dev);

	mutex_lock(&phy->lock);

	if (phy->pm_off)
		goto out;

	phy_suspend_off(phy);
	if (phy->commonon)
		usleep_range(16, 20);
	else {
		if (phy->refclksel == REFCLK_CRYSTAL)
			usleep_range(805, 810);
		else
			usleep_range(45, 50);
	}
out:
	mutex_unlock(&phy->lock);
	return 0;
}
#endif

static const struct dev_pm_ops snps_picophy_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(snps_picophy_pm_suspend, snps_picophy_pm_resume)
};

static struct platform_driver snps_picophy_driver = {
	.probe = snps_picophy_probe,
	.driver = {
		.name = DRIVER_NAME,
		.pm = &snps_picophy_dev_pm_ops,
		.of_match_table = snps_picophy_dt_match,
	},
};
module_platform_driver(snps_picophy_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synopsys USB2 PicoPHY driver");
