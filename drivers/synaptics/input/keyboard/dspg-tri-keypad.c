/*
 *  Triangular keyboard driver for DSPG keypad controller
 *
 *  Copyright (c) 2015--2018 DSPG Technologies GmbH, Andreas Weissel
 *
 *  Based on corgikbd.c, which is based on xtkbd.c/locomkbd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define TRI_KBS_GLOBAL	0x00
#define TRI_KBS_CON	0x04
#define TRI_KBS_CODE	0x08
#define TRI_KBS_STAT	0x0C

#define KBS_STAT_KP	BIT(0)
#define KBS_STAT_KR	BIT(1)
#define KBS_ERR		BIT(2)

#define MAX_KEYS	45

struct dspg_tri_keypad {
	struct input_dev *input;
	struct resource *mem;
	struct clk *clk;

	int irq;
	void __iomem *regs;

	unsigned int keymap_size;
	unsigned short keycodes[MAX_KEYS];

	int code;
	int key_active;
};

static void
dspg_tri_keypad_reset(struct dspg_tri_keypad *keypad)
{
	keypad->key_active = 0;
	keypad->code = 0xffff;
}

static irqreturn_t
dspg_tri_keypad_interrupt(int irq, void *dev_id)
{
	struct dspg_tri_keypad *keypad = (struct dspg_tri_keypad *)dev_id;
	unsigned int state, code;

	state = readl(keypad->regs + TRI_KBS_STAT);
	code = readl(keypad->regs + TRI_KBS_CODE);

	/* clear interrupt status */
	iowrite16(~state & 0xf, keypad->regs + TRI_KBS_STAT);

	if (state & KBS_ERR)
		return IRQ_HANDLED;

	if (state & KBS_STAT_KP) {
		if (keypad->key_active && code != keypad->code) {
			/* simultaneous press, release last pressed key */
			input_report_key(keypad->input,
					 keypad->keycodes[keypad->code], 0);
			dspg_tri_keypad_reset(keypad);
		}

		if (!keypad->key_active) {
			keypad->code = code;
			keypad->key_active = 1;
			input_report_key(keypad->input,
					 keypad->keycodes[code], 1);
			input_sync(keypad->input);
		}
	}

	if (state & KBS_STAT_KR && keypad->key_active &&
		   code == keypad->code) {
		input_report_key(keypad->input, keypad->keycodes[code], 0);
		input_sync(keypad->input);

		dspg_tri_keypad_reset(keypad);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_PM
static int dspg_tri_keypad_suspend(struct platform_device *dev,
				   pm_message_t state)
{
	struct dspg_tri_keypad *dspg_tri_keypad = platform_get_drvdata(dev);

	if (device_may_wakeup(&dev->dev))
		enable_irq_wake(dspg_tri_keypad->irq);
	else
		clk_disable(dspg_tri_keypad->clk);

	return 0;
}

static int dspg_tri_keypad_resume(struct platform_device *dev)
{
	struct dspg_tri_keypad *dspg_tri_keypad = platform_get_drvdata(dev);

	if (device_may_wakeup(&dev->dev))
		disable_irq_wake(dspg_tri_keypad->irq);
	else
		clk_enable(dspg_tri_keypad->clk);

	return 0;
}
#else
#define dspg_tri_keypad_suspend NULL
#define dspg_tri_keypad_resume  NULL
#endif

static int __init
dspg_tri_keypad_probe(struct platform_device *pdev)
{
	struct dspg_tri_keypad *dspg_tri_keypad;
	struct device_node *np = pdev->dev.of_node;
	struct input_dev *input_dev;
	struct resource *res;
	int ret;
	const __be32 *prop;
	int proplen;
	int i;
	unsigned long rate;

	dspg_tri_keypad = devm_kzalloc(&pdev->dev,
				       sizeof(struct dspg_tri_keypad),
				       GFP_KERNEL);
	if (!dspg_tri_keypad)
		return -ENOMEM;

	dspg_tri_keypad->input = devm_input_allocate_device(&pdev->dev);
	input_dev = dspg_tri_keypad->input;
	if (!input_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, dspg_tri_keypad);

	prop = of_get_property(np, "linux,keymap", &proplen);
	if (!prop)
		return -ENODEV;

	dspg_tri_keypad->keymap_size = proplen / sizeof(u32);

	if ((proplen % sizeof(u32)) || (proplen > MAX_KEYS * sizeof(u32))) {
		pr_warn("Malformed keymap property 'linux,keymap' in %s\n",
			np->full_name);
		return -EINVAL;
	}

	dspg_tri_keypad->irq = platform_get_irq(pdev, 0);
	if (dspg_tri_keypad->irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no memory resource defined\n");
		return -ENODEV;
	}

	dspg_tri_keypad->mem = devm_request_mem_region(&pdev->dev, res->start,
						       resource_size(res),
						       dev_name(&pdev->dev));
	if (!dspg_tri_keypad->mem) {
		dev_err(&pdev->dev, "cannot get register range\n");
		return -EBUSY;
	}

	dspg_tri_keypad->regs = devm_ioremap(&pdev->dev, res->start,
					     resource_size(res));
	if (dspg_tri_keypad->regs == NULL) {
		dev_err(&pdev->dev, "failed to ioremap() registers\n");
		return -ENODEV;
	}

	input_dev->name = "DSPG triangular keypad";
	input_dev->phys = pdev->name;
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	input_dev->dev.parent = &pdev->dev;
	input_set_drvdata(input_dev, dspg_tri_keypad);

	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	input_dev->keycode = dspg_tri_keypad->keycodes;
	input_dev->keycodesize = sizeof(dspg_tri_keypad->keycodes[0]);
	input_dev->keycodemax = dspg_tri_keypad->keymap_size;

	for (i = 0; i < dspg_tri_keypad->keymap_size; i++) {
		u32 tmp = be32_to_cpup(prop + i);
		int code, index;

		index = (tmp >> 16) & 0xff;
		code = tmp & 0xffff;

		dspg_tri_keypad->keycodes[index] = code;
		__set_bit(code, input_dev->keybit);
	}
	__clear_bit(KEY_RESERVED, input_dev->keybit);

	ret = input_register_device(dspg_tri_keypad->input);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register input device\n");
		return ret;
	}

	dspg_tri_keypad->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dspg_tri_keypad->clk)) {
		ret = PTR_ERR(dspg_tri_keypad->clk);
		dev_err(&pdev->dev, "no clock: %d\n", ret);
		goto out_unregister_input;
	}
	rate = clk_round_rate(dspg_tri_keypad->clk, 32000);
	clk_set_rate(dspg_tri_keypad->clk, rate);
	clk_prepare_enable(dspg_tri_keypad->clk);

	iowrite16(0x0, dspg_tri_keypad->regs + TRI_KBS_CON);
	iowrite16(0x0, dspg_tri_keypad->regs + TRI_KBS_STAT);
	dspg_tri_keypad_reset(dspg_tri_keypad);

	/* Setup sense interrupts - RisingEdge Detect, sense lines as inputs */
	ret = devm_request_irq(&pdev->dev, dspg_tri_keypad->irq,
			       dspg_tri_keypad_interrupt, 0,
			       "dspg_tri_keypad", dspg_tri_keypad);
	if (ret < 0) {
		dev_err(&pdev->dev, "cannot get irq\n");
		goto out_clk_disable;
	}

	/* Keypad is wakeup capable by default */
	device_init_wakeup(&pdev->dev, 1);

	return 0;

out_clk_disable:
	clk_disable_unprepare(dspg_tri_keypad->clk);
out_unregister_input:
	input_unregister_device(dspg_tri_keypad->input);

	return ret;
}

static int
dspg_tri_keypad_remove(struct platform_device *pdev)
{
	struct dspg_tri_keypad *dspg_tri_keypad = platform_get_drvdata(pdev);

	clk_disable_unprepare(dspg_tri_keypad->clk);
	input_unregister_device(dspg_tri_keypad->input);

	return 0;
}

static const struct of_device_id dspg_tri_keypad_of_match[] = {
	{ .compatible = "dspg,tri-keypad", },
	{ },
};
MODULE_DEVICE_TABLE(of, dspg_tri_keypad_of_match);

static struct platform_driver dspg_tri_keypad_driver = {
	.remove		= dspg_tri_keypad_remove,
	.suspend	= dspg_tri_keypad_suspend,
	.resume		= dspg_tri_keypad_resume,
	.driver		= {
		.name	= "dspg-tri_keypad",
		.owner  = THIS_MODULE,
		.of_match_table = dspg_tri_keypad_of_match,
	},
};
module_platform_driver_probe(dspg_tri_keypad_driver, dspg_tri_keypad_probe);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("DSPG Triangular Keypad Driver");
MODULE_LICENSE("GPL");
