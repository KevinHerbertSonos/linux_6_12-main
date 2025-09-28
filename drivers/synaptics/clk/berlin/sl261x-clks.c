// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/module.h>

#include "clk.h"

static const struct gateclk_desc sl261x_gates[] = {
	{ "usb0coreclk",	"perifsysclk",	0 },
	{ "sdiosysclk",		"perifsysclk",	1 },
	{ "emmcsysclk",		"perifsysclk",	2 },
	{ "gpuaxiclk",		"hpcclk",	3 },
	{ "gethrgmiisysclk",	"perifsysclk",	4 },
	{ "sdio1sysclk",	"perifsysclk",	5 },
	{ "usb1coreclk",	"perifsysclk",	6 },
	{ "gethrgmii1sysclk",	"perifsysclk",	7 },
	{ "usb0phyrefclk",	"perifsysclk",	8 },
	{ "usb1phyrefclk",	"perifsysclk",	9 },
	{ "apbuart0clk",	"apbcoreclk",	10 },
	{ "apbuart1clk",	"apbcoreclk",	11 },
	{ "apbuart2clk",	"apbcoreclk",	12 },
	{ "apbuart3lck",	"apbcoreclk",	13 },
	{ "apbi2c0clk",		"apbcoreclk",	14 },
	{ "apbi2c1lck",		"apbcoreclk",	15 },
	{ "apbspi0clk",		"apbcoreclk",	16 },
	{ "apbspi1clk",		"apbcoreclk",	17 },
	{ "apbspi2clk",		"apbcoreclk",	18 },
	{ "apbspi3clk",		"apbcoreclk",	19 },
	{ "apbgpioclk",		"apbcoreclk",	20 },
	{ "apbtimersclk",	"apbcoreclk",	21 },
	{ "apbsyscntclk",	"apbcoreclk",	22 },
	{ "apbwdtclk",		"apbcoreclk",	23 },
	{ "apbdmaclk",		"apbcoreclk",	24 },
};

static int sl261x_gateclk_setup(struct platform_device *pdev)
{
	return berlin_gateclk_setup(pdev, sl261x_gates, ARRAY_SIZE(sl261x_gates));
}

static const struct clk_desc sl261x_descs[] = {
	{ "cpufastrefclk",		0x0, CLK_IS_CRITICAL },
	{ "memfastrefclk",		0x4, CLK_IS_CRITICAL },
	{ "cfgclk",			0x8, CLK_IS_CRITICAL },
	{ "sysclk",			0xc, CLK_IS_CRITICAL },
	{ "perifsysclk",		0x10, CLK_IS_CRITICAL },
	{ "apbcoreclk",			0x14, CLK_IS_CRITICAL },
	{ "apbserclk",			0x18, CLK_IS_CRITICAL },
	{ "atbclk",			0x1c, CLK_IS_CRITICAL },
	{ "hpcclk",			0x20, CLK_IS_CRITICAL },
	{ "emmcclk",			0x24 },
	{ "sd0clk",			0x28 },
	{ "sd1clk",			0x2c },
	{ "gethrgmiiclk",		0x30 },
	{ "gethrgmii1clk",		0x34 },
	{ "ge0_ptp_refclk",		0x38 },
	{ "ge1_ptp_refclk",		0x3c },
	{ "usb2testclk",		0x40 },
	{ "usb2test480mg0clk",		0x44 },
	{ "usb2test480mg1clk",		0x48 },
	{ "usb2test480mg2clk",		0x4c },
	{ "usb2test100mg0clk",		0x50 },
	{ "usb2test100mg1clk",		0x54 },
	{ "usb2test100mg2clk",		0x58 },
	{ "usb2test100mg3clk",		0x5c },
	{ "periftest125mg0clk",		0x60 },
	{ "periftest200mg0clk",		0x64 },
	{ "periftest200mg1clk",		0x68 },
	{ "gpuclk",			0x6c },
	{ "npuclk",			0x70 },
	{ "aviosysclk",			0x74 },
	{ "aiosysclk",			0x78 },
	{ "avio_lcdc2scanclk",		0x7c },
	{ "avio_ipiclk",		0x80 },
	{ "avio_pclk",			0x84 },
	{ "avio_dphyrxtxescclk",	0x88 },
	{ "aviofpllclk",		0x8c },
	{ "avio_rx_scanbyteclk",	0x90 },
	{ "avio_rx_scantestclk",	0x94 },
};

static int sl261x_clk_setup(struct platform_device *pdev)
{
	return berlin_clk_setup(pdev, sl261x_descs, ARRAY_SIZE(sl261x_descs));
}

static const struct of_device_id sl261x_clks_match_table[] = {
	{ .compatible = "syna,sl261x-clk",
	  .data = sl261x_clk_setup },
	{ .compatible = "syna,sl261x-gateclk",
	  .data = sl261x_gateclk_setup },
	{ }
};
MODULE_DEVICE_TABLE(of, sl261x_clks_match_table);

static int sl261x_clks_probe(struct platform_device *pdev)
{
	int (*clk_setup)(struct platform_device *pdev);
	int ret;

	clk_setup = of_device_get_match_data(&pdev->dev);
	if (!clk_setup)
		return -ENODEV;

	ret = clk_setup(pdev);
	if (ret)
		return ret;

	return 0;
}

static struct platform_driver sl261x_clks_driver = {
	.probe		= sl261x_clks_probe,
	.driver		= {
		.name	= "syna-sl261x-clks",
		.of_match_table = sl261x_clks_match_table,
	},
};
module_platform_driver(sl261x_clks_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics sl261x clks Driver");
