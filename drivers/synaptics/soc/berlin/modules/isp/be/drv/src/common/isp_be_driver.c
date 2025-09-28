// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "ispbe_api.h"
#include "ispbe_err.h"

#define ISP_BE_NAME                "isp-be"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static void isp_be_remove(struct platform_device *pdev)
#else
static int isp_be_remove(struct platform_device *pdev)
#endif
{
	struct device *dev = &pdev->dev;
	HRESULT result = SUCCESS;

	dev_dbg(dev, "%s is invoked\n", __func__);

	/* Deinitialize ISPBE-CA module */
	ISPBE_CA_DeInitialize();
	if (result != 0) {
		dev_err(dev, "%s(): CA Deinitialize failed!!\n", __func__);
		goto exit;
	}

	/*Release all SHM used by scaler drv */
	result = ispSS_SHM_Deinit(dev);
	if (result) {
		dev_err(dev, "%s(): failed to release shm\n", __func__);
	}

exit:
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
	return result;
#endif
}

static int isp_be_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	HRESULT result = SUCCESS;

	dev_dbg(dev, "%s is invoked\n", __func__);

	result = ispSS_SHM_Init(dev);
	if (result) {
		dev_err(dev, "%s(): failed to create shm\n", __func__);
		result = FAILURE;
		goto memory_init_failed;
	}

	/* Initialize ISPBE-CA module */
	result = ISPBE_CA_Initialize();
	if (result) {
		dev_err(dev, "%s(): CA initialize failed!!\n", __func__);
		if (result == -EINVAL)
			result = -EPROBE_DEFER;
		else
			result = FAILURE;
		goto err_CA_fail;
	}
	return result;

err_CA_fail:
	ispSS_SHM_Deinit(dev);
memory_init_failed:
	return result;
}

/* Compatible string for device tree match */
static const struct of_device_id isp_be_match_types[] = {
	{.compatible = "syna,dolphin-isp-be"},
	{}
};

MODULE_DEVICE_TABLE(of, isp_be_match_types);

static struct platform_driver isp_be_driver = {
	.probe          = isp_be_probe,
	.remove         = isp_be_remove,
	.driver         = {
		.name           = ISP_BE_NAME,
		.of_match_table = isp_be_match_types,
	},
};

module_platform_driver(isp_be_driver);

MODULE_IMPORT_NS(SYNA_BM);
MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ISP BE Driver");
