// SPDX-License-Identifier: GPL-2.0-only
/*******************************************************************************
  DWMAC DVF driver

  Copyright (C) 2023 Synaptics Inc

  Author: Andreas Weissel <andreas.weissel@synaptics.com>
*******************************************************************************/

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "stmmac.h"
#include "stmmac_platform.h"

static void dvf_dwmac_fix_speed(void *priv, unsigned int speed)
{
	struct plat_stmmacenet_data *plat_dat =
					(struct plat_stmmacenet_data *)priv;
	unsigned long rate = 125000000UL;
	int err;

	switch (speed) {
	case SPEED_1000:
		rate = 125000000UL;
		break;

	case SPEED_100:
		rate = 25000000UL;
		break;

	case SPEED_10:
		rate = 2500000UL;
		break;

	default:
		pr_err("invalid speed %u\n", speed);
		break;
	}

	if (plat_dat->pclk)
		err = clk_set_rate(plat_dat->pclk, rate);
	else
		err = clk_set_rate(plat_dat->stmmac_clk, rate);
	if (err < 0)
		pr_err("failed to set TX rate: %d\n", err);
}

static int dwmac_dvf_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	int ret;

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
	if (IS_ERR(plat_dat)) {
		dev_err(&pdev->dev, "dt configuration failed\n");
		return PTR_ERR(plat_dat);
	}

	plat_dat->fix_mac_speed = dvf_dwmac_fix_speed;
	plat_dat->bsp_priv = plat_dat;
	plat_dat->has_gmac = 1;
	plat_dat->pmt = 1;

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_exit;

	return 0;

err_exit:
	stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

static const struct of_device_id dwmac_dvf_match[] = {
	{ .compatible = "syna,dvf-gmac"},
	{ }
};
MODULE_DEVICE_TABLE(of, dwmac_dvf_match);

static struct platform_driver dwmac_dvf_driver = {
	.probe  = dwmac_dvf_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name		= STMMAC_RESOURCE_NAME,
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = of_match_ptr(dwmac_dvf_match),
	},
};
module_platform_driver(dwmac_dvf_driver);

MODULE_DESCRIPTION("DVF dwmac driver");
MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@st.com>");
MODULE_LICENSE("GPL");
