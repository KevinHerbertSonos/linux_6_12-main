// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_device.h>
#include <linux/input.h>

#define FORCEMUTE_RISE		1
#define FORCEMUTE_FALL		2

#define RA_AIO_IRQENABLE2	0x274
#define RA_AIO_IRQSTS2		0x2A4
#define RA_AIO_FORCEMUTE	0x3C4

#define AIO_FMUTE_DELAY_MAX 255
struct forcemute {
	struct device *dev;
	struct input_dev *input;
	void __iomem *regs;
	void __iomem *int12_regs;
	unsigned int deb_delay;
	unsigned int eff_delay;
	int irq;
};

static irqreturn_t forcemute_isr(int irq, void *dev_id)
{
	struct forcemute *forcemute = dev_id;
	uint32_t stat;

	stat = readl(forcemute->regs + RA_AIO_IRQSTS2);
	writel(stat, forcemute->regs + RA_AIO_IRQSTS2);

	if (stat & FORCEMUTE_RISE)
		input_report_key(forcemute->input, BTN_0, 1);
	else
		input_report_key(forcemute->input, BTN_0, 0);
	input_sync(forcemute->input);

	return IRQ_HANDLED;
}

static int forcemute_probe(struct platform_device *pdev)
{
	struct forcemute *forcemute;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	forcemute = devm_kzalloc(dev, sizeof(struct forcemute), GFP_KERNEL);
	if (!forcemute)
		return -ENOMEM;

	forcemute->dev = dev;
	platform_set_drvdata(pdev, forcemute);

	forcemute->input = devm_input_allocate_device(&pdev->dev);
	if (!forcemute->input)
		return -ENOMEM;

	input_set_drvdata(forcemute->input, forcemute);

	forcemute->deb_delay = 20; //default value
	ret = of_property_read_u32(dev->of_node, "forcemute,deb_delay",
				   &forcemute->deb_delay);
	if ((ret < 0 && ret != -EINVAL) || (forcemute->deb_delay > AIO_FMUTE_DELAY_MAX)) {
		dev_err(&pdev->dev, "invalid forcemute,deb_delay\n");
		return ret;
	}

	forcemute->eff_delay = 0; //default value
	ret = of_property_read_u32(dev->of_node, "forcemute,eff_delay",
				   &forcemute->eff_delay);
	if ((ret < 0 && ret != -EINVAL) || (forcemute->eff_delay > AIO_FMUTE_DELAY_MAX)) {
		dev_err(&pdev->dev, "invalid forcemute,eff_delay\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no registers specified\n");
		return -EINVAL;
	}

	forcemute->regs = devm_ioremap_resource(dev, res);
	if (!forcemute->regs) {
		dev_err(&pdev->dev, "failed to map registers\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "no registers specified\n");
		return -EINVAL;
	}

	forcemute->int12_regs = devm_ioremap_resource(dev, res);
	if (!forcemute->int12_regs) {
		dev_err(&pdev->dev, "failed to map registers\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "no irq specified\n");
		return -EINVAL;
	}

	forcemute->irq = res->start;
	ret = devm_request_irq(&pdev->dev, forcemute->irq, forcemute_isr, 0,
			       "forcemute", forcemute);
	if (ret < 0) {
		dev_err(&pdev->dev, "irq request error\n");
		return ret;
	}

	forcemute->input->name = "forcemute";
	forcemute->input->phys = pdev->name;
	forcemute->input->dev.parent = &pdev->dev;
	input_set_drvdata(forcemute->input, forcemute);
	input_set_capability(forcemute->input, EV_KEY, BTN_0);

	ret = input_register_device(forcemute->input);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register input device\n");
		return ret;
	}

	writel(forcemute->deb_delay | (forcemute->eff_delay << 8),
	       forcemute->regs + RA_AIO_FORCEMUTE);

	/* interrupt on rise & fall */
	writel(FORCEMUTE_RISE | FORCEMUTE_FALL,
	       forcemute->regs + RA_AIO_IRQSTS2);
	writel(FORCEMUTE_RISE | FORCEMUTE_FALL,
	       forcemute->regs + RA_AIO_IRQENABLE2);
	writel(2, forcemute->int12_regs);

	return 0;
}

static int forcemute_remove(struct platform_device *pdev)
{
	struct forcemute *forcemute = platform_get_drvdata(pdev);

	writel(0, forcemute->regs + RA_AIO_IRQENABLE2);
	writel(0, forcemute->int12_regs);

	input_unregister_device(forcemute->input);

	return 0;
}

static const struct of_device_id forcemute_of_match[] = {
	{ .compatible = "syna,myna2-forcemute", },
	{ }
};
MODULE_DEVICE_TABLE(of, forcemute_of_match);

static struct platform_driver forcemute_driver = {
	.driver			= {
		.name		= "syna-forcemute",
		.of_match_table = forcemute_of_match,
	},
	.probe			= forcemute_probe,
	.remove			= forcemute_remove,
};
module_platform_driver(forcemute_driver);

MODULE_AUTHOR("Andreas Weissel <andreas.weissel@synaptics.com>");
MODULE_ALIAS("platform:forcemute");
MODULE_DESCRIPTION("Forcemute Input Driver");
MODULE_LICENSE("GPL v2");
