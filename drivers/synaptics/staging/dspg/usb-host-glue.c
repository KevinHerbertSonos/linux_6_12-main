/*
 * linux/drivers/staging/dspg/usb-host-glue.c
 *
 *  Copyright (C) 2016 DSP Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/reset.h>

static int usb_host_glue_probe(struct platform_device *pdev)
{
	struct reset_control *aux_reset, *phy_reset;
	struct reset_control *utmi0_reset, *utmi1_reset;
	int ret;

	aux_reset = devm_reset_control_get(&pdev->dev, "aux");
	if (IS_ERR(aux_reset)) {
		dev_err(&pdev->dev, "cannot get auxiliary reset control\n");
		return PTR_ERR(aux_reset);
	}

	phy_reset = devm_reset_control_get(&pdev->dev, "phy");
	if (IS_ERR(phy_reset)) {
		dev_err(&pdev->dev, "cannot get phy reset control\n");
		return PTR_ERR(phy_reset);
	}

	utmi1_reset = devm_reset_control_get(&pdev->dev, "utmi1");
	if (IS_ERR(utmi1_reset)) {
		int err = PTR_ERR(utmi1_reset);
		if (err != -ENOENT && err != -EPROBE_DEFER) {
			dev_err(&pdev->dev,
				"cannot get utmi1 reset control\n");
			return PTR_ERR(utmi1_reset);
		}
		utmi1_reset = NULL;
	}
	dev_info(&pdev->dev, "UTMI1 port used\n");

	utmi0_reset = devm_reset_control_get(&pdev->dev, "utmi0");
	if (IS_ERR(utmi0_reset)) {
		int err = PTR_ERR(utmi0_reset);
		if (err != -ENOENT && err != -EPROBE_DEFER) {
			dev_err(&pdev->dev,
				"cannot get utmi0 reset control\n");
			return PTR_ERR(utmi0_reset);
		}
		utmi0_reset = NULL;
	}
	dev_info(&pdev->dev, "UTMI0 port used\n");

	if (reset_control_status(aux_reset)) {
		ret = reset_control_deassert(aux_reset);
		if (ret)
			return ret;

		ret = reset_control_deassert(phy_reset);
		if (ret)
			return ret;

		if (utmi1_reset) {
			ret = reset_control_deassert(utmi1_reset);
			if (ret)
				return ret;
		}
		if (utmi0_reset) {
			ret = reset_control_deassert(utmi0_reset);
			if (ret)
				return ret;
		}

		dev_info(&pdev->dev,
			 "successfully taken USB host out of reset\n");
	} else {
		/* USB devices are already taken out of reset */
		/* maybe more to do here */
		dev_info(&pdev->dev, "USB host already out of reset\n");
	}

	return 0;
}

static struct of_device_id usb_host_glue_of_device_ids[] = {
	{ .compatible = "dspg,dvf101-usb-host-reset" },
	{ },
};

static struct platform_driver usb_host_glue_driver = {
	.probe = usb_host_glue_probe,
	.driver = {
		.name = "usb-host-glue",
		.owner = THIS_MODULE,
		.of_match_table = usb_host_glue_of_device_ids,
	},
};

static int __init usb_host_glue_driver_init(void)
{
	return platform_driver_register(&usb_host_glue_driver);
}
arch_initcall(usb_host_glue_driver_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("USB host reset glue");
MODULE_AUTHOR("DSP Group, Inc.");
