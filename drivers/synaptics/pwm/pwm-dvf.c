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
 * driver for PWM_GEN blocks of DSPG DVF platforms
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>

#define PWM_CFG1                  0x0000
#define PWM_CFG2                  0x0004
#define PWM_REG_LEN               0x100
#define PWM_MIN_PERIOD            2
#define PWM_MAX_PERIOD            GENMASK(29, 0)

#define PWM_CFG1_EN               31
#define PWM_CFG1_POL              30
#define PWM_CFG1_CYCLE            GENMASK(29, 0)

struct dvf_pwm_data {
	unsigned int nr;
};

static const struct dvf_pwm_data dvf101_pwm_data = {
	.nr = 8,
};

static const struct of_device_id dvf_pwm_dt_ids[] = {
	{ .compatible = "dspg,dvf101-pwm", .data = &dvf101_pwm_data, },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, dvf_pwm_dt_ids);

struct dvf_pwm_chip {
	struct pwm_chip chip;
	struct clk *clk;
	struct reset_control *rc;
	void __iomem *base;
	const struct dvf_pwm_data *data;
	spinlock_t *channel_lock;
	struct timer_list clk_off_timer;
	unsigned long clk_rate;
};

static inline struct dvf_pwm_chip *to_dvf_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct dvf_pwm_chip, chip);
}

static inline u32 dvf_pwm_readl(struct dvf_pwm_chip *chip,
				unsigned long offset)
{
	return readl(chip->base + offset);
}

static inline void dvf_pwm_writel(struct dvf_pwm_chip *chip,
				  unsigned long offset, unsigned long val)
{
	writel(val, chip->base + offset);
}

static void dvf_pwm_ch_bit_on_off(struct dvf_pwm_chip *pwm, unsigned int ch,
				  unsigned int bit, unsigned int on)
{
	unsigned int offset = PWM_CFG1 + ch * PWM_REG_LEN;
	unsigned long flags;
	u32 tmp;

	spin_lock_irqsave(&pwm->channel_lock[ch], flags);
	tmp = dvf_pwm_readl(pwm, offset);
	if (on)
		tmp |= (1 << bit);
	else
		tmp &= ~(1 << bit);
	dvf_pwm_writel(pwm, offset, tmp);
	spin_unlock_irqrestore(&pwm->channel_lock[ch], flags);
}

static int dvf_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm_dev,
			  int duty_ns, int period_ns)
{
	struct dvf_pwm_chip *pwm = to_dvf_pwm_chip(chip);
	unsigned long long cycles;
	unsigned long long duty_cycles;
	const unsigned long long rate = pwm->clk_rate;
	unsigned int ch = pwm_dev->hwpwm;
	unsigned int offset = ch * PWM_REG_LEN;
	unsigned long flags;
	u32 tmp;

	cycles = rate * period_ns;
	do_div(cycles, NSEC_PER_SEC);

	if (cycles > PWM_MAX_PERIOD) {
		dev_warn(pwm->chip.dev, "period exceeds maximum value");
		cycles = PWM_MAX_PERIOD;
	}

	if (cycles < PWM_MIN_PERIOD)
		cycles = PWM_MIN_PERIOD;

	duty_cycles = rate * duty_ns;
	do_div(duty_cycles, NSEC_PER_SEC);

	if (duty_cycles > cycles)
		return -EINVAL;

	if (duty_cycles < 1)
		duty_cycles = 1;

	pm_runtime_get_sync(chip->dev);

	spin_lock_irqsave(&pwm->channel_lock[ch], flags);
	tmp = dvf_pwm_readl(pwm, offset + PWM_CFG1);
	tmp &= ~PWM_CFG1_CYCLE;
	tmp |= cycles;
	dvf_pwm_writel(pwm, offset + PWM_CFG1, tmp);
	dvf_pwm_writel(pwm, offset + PWM_CFG2, (unsigned long)duty_cycles);
	spin_unlock_irqrestore(&pwm->channel_lock[ch], flags);

	pm_runtime_put_sync(chip->dev);

	return 0;
}

static int dvf_pwm_set_polarity(struct pwm_chip *chip,
				struct pwm_device *pwm_dev,
				enum pwm_polarity polarity)
{
	struct dvf_pwm_chip *pwm = to_dvf_pwm_chip(chip);
	unsigned int ch = pwm_dev->hwpwm;

	pm_runtime_get_sync(chip->dev);

	if (polarity == PWM_POLARITY_NORMAL)
		dvf_pwm_ch_bit_on_off(pwm, ch, PWM_CFG1_POL, 0);
	else
		dvf_pwm_ch_bit_on_off(pwm, ch, PWM_CFG1_POL, 1);

	pm_runtime_put_sync(chip->dev);

	return 0;
}

static int dvf_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct dvf_pwm_chip *pwm = to_dvf_pwm_chip(chip);
	unsigned int ch = pwm_dev->hwpwm;

	pm_runtime_get_sync(chip->dev);

	dvf_pwm_ch_bit_on_off(pwm, ch, PWM_CFG1_EN, 1);

	return 0;
}

static void dvf_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct dvf_pwm_chip *pwm = to_dvf_pwm_chip(chip);
	unsigned int ch = pwm_dev->hwpwm;
	u64 period = pwm_get_period(pwm_dev);
	unsigned int msecs = do_div(period, 1000000);

	dvf_pwm_ch_bit_on_off(pwm, ch, PWM_CFG1_EN, 0);

	/* wait one period as the PWM block will finish it anyway even if
	 * the enable bit was cleared. Only when the timer expires the clock
	 * can be switched off. If we do it too fast the PWM block will
	 * continue with the last period when the clock is switched on again.
	 */
	mod_timer(&pwm->clk_off_timer, jiffies + msecs_to_jiffies(msecs));
}

static void dvf_pwm_get_state(struct pwm_chip *chip,
			      struct pwm_device *pwm_dev,
			      struct pwm_state *state)
{
	struct dvf_pwm_chip *pwm = to_dvf_pwm_chip(chip);
	unsigned int ch = pwm_dev->hwpwm;
	unsigned int offset = ch * PWM_REG_LEN;
	u64 tmp;

	pm_runtime_get_sync(chip->dev);

	tmp = dvf_pwm_readl(pwm, offset + PWM_CFG1);
	if (tmp & BIT(PWM_CFG1_EN))
		state->enabled = true;
	if (tmp & BIT(PWM_CFG1_POL))
		state->polarity = PWM_POLARITY_INVERSED;
	tmp &= PWM_CFG1_CYCLE;
	tmp *= NSEC_PER_SEC;
	state->period = DIV_ROUND_CLOSEST_ULL(tmp, pwm->clk_rate);

	tmp = dvf_pwm_readl(pwm, offset + PWM_CFG2);
	tmp *= NSEC_PER_SEC;
	state->duty_cycle = DIV_ROUND_CLOSEST_ULL(tmp, pwm->clk_rate);

	pm_runtime_put_sync(chip->dev);
}

static const struct pwm_ops dvf_pwm_ops = {
	.config = dvf_pwm_config,
	.set_polarity = dvf_pwm_set_polarity,
	.enable = dvf_pwm_enable,
	.disable = dvf_pwm_disable,
	.get_state = dvf_pwm_get_state,
	.owner = THIS_MODULE,
};

static void dvf_pwm_clk_off(struct timer_list *t)
{
	struct dvf_pwm_chip *pwm = from_timer(pwm, t, clk_off_timer);

	pm_runtime_put_sync(pwm->chip.dev);
}

static int dvf_pwm_probe(struct platform_device *pdev)
{
	const struct dvf_pwm_data *data;
	struct dvf_pwm_chip *pwm;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	struct resource *res;
	int i, ret = -ENODEV;

	if (!np)
		goto out;

	match = of_match_device(dvf_pwm_dt_ids, &pdev->dev);
	if (!match)
		goto out;

	data = match->data;

	pwm = devm_kzalloc(&pdev->dev,
			   sizeof(*pwm) + data->nr * sizeof(spinlock_t),
			   GFP_KERNEL);
	if (!pwm) {
		ret = -ENOMEM;
		goto out;
	}

	pwm->data = data;
	pwm->channel_lock = (spinlock_t *)(pwm + 1);
	platform_set_drvdata(pdev, pwm);

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &dvf_pwm_ops;
	pwm->chip.of_xlate = of_pwm_xlate_with_flags;
	pwm->chip.of_pwm_n_cells = 3;
	pwm->chip.base = -1;
	pwm->chip.npwm = data->nr;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -EINVAL;
		goto out;
	}

	pwm->base = devm_ioremap_resource(&pdev->dev, res);
	if (!pwm->base) {
		ret = -EIO;
		goto out;
	}

	pwm->rc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(pwm->rc)) {
		ret = PTR_ERR(pwm->rc);
		goto out;
	}

	pwm->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pwm->clk)) {
		ret = PTR_ERR(pwm->clk);
		goto out;
	}
	pwm->clk_rate = clk_get_rate(pwm->clk);

	ret = reset_control_deassert(pwm->rc);
	if (ret)
		goto out;

	timer_setup(&pwm->clk_off_timer, dvf_pwm_clk_off, 0);

	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	ret = pwmchip_add(&pwm->chip);
	if (ret)
		goto out;

	for (i = 0; i < pwm->chip.npwm; i++)
		if (pwm_is_enabled(&pwm->chip.pwms[i]))
			break;

	if (i == pwm->chip.npwm)
		pm_runtime_put(&pdev->dev);

	dev_info(pwm->chip.dev, "successfully probed");

	ret = 0;
	goto out;

out:
	return ret;
}

static int dvf_pwm_remove(struct platform_device *pdev)
{
	struct dvf_pwm_chip *pwm = platform_get_drvdata(pdev);

	del_timer_sync(&pwm->clk_off_timer);

	if (reset_control_assert(pwm->rc))
		dev_err(pwm->chip.dev, "unable to assert reset");

	pwmchip_remove(&pwm->chip);

	return 0;
}

#ifdef CONFIG_PM
static int dvf_pwm_runtime_suspend(struct device *dev)
{
	struct dvf_pwm_chip *pwm = dev_get_drvdata(dev);

	clk_disable_unprepare(pwm->clk);

	return 0;
}

static int dvf_pwm_runtime_resume(struct device *dev)
{
	struct dvf_pwm_chip *pwm = dev_get_drvdata(dev);

	clk_prepare_enable(pwm->clk);

	return 0;
}
#endif

static const struct dev_pm_ops dvf_pwm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(dvf_pwm_runtime_suspend, dvf_pwm_runtime_resume,
			   NULL)
};

static struct platform_driver dvf_pwm_driver = {
	.driver = {
		.name = "dvf-pwm",
		.pm = &dvf_pwm_pm_ops,
		.of_match_table = of_match_ptr(dvf_pwm_dt_ids),
	},
	.probe = dvf_pwm_probe,
	.remove = dvf_pwm_remove,
};
module_platform_driver(dvf_pwm_driver);

MODULE_ALIAS("platform:dvf-pwm");
MODULE_DESCRIPTION("DVF PWM driver");
MODULE_LICENSE("GPL v2");
