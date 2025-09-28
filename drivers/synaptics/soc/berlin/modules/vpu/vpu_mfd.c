// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/compat.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include "vpu_tee_wrap.h"
#include "vpu_kernel_compatibility_wrap.h"
#include "vpu_mfd.h"
#include "v2/vpu_fw.h"

struct syna_vpu_mfd_parent {
	struct device *dev;
	int irq;
	const struct syna_vpu_variant *variant;
	struct syna_vpu_auxiliary_device subdev[2];

	struct mutex lock;
	unsigned int refcount;
	unsigned int slot;
};

static struct platform_driver syna_vpu_driver;

static DEFINE_MUTEX(vpu_firmware_mutex);

static const struct syna_vpu_variant vpu_v4g_data = {
	.hw_type = VPU_V2G,
	.hw_ver = 0,
	.hw_variant =
	    HW_SUPPORT_DEC_MPEG2 | HW_SUPPORT_DEC_H264 | HW_SUPPORT_DEC_H265 |
	    HW_SUPPORT_DEC_VP8 | HW_SUPPORT_DEC_VP9 | HW_SUPPORT_DEC_AV1,
};

static const struct syna_vpu_variant vpu_h1_lite_data = {
	.hw_type = VPU_H1_0,
	.hw_ver = 0,
	.hw_variant = HW_SUPPORT_ENC_H264,
};

static const struct syna_vpu_variant vpu_h1_data = {
	.hw_type = VPU_H1_0,
	.hw_ver = 0,
	.hw_variant = HW_SUPPORT_ENC_H264 | HW_SUPPORT_ENC_VP8,
};

static const struct of_device_id syna_vpu_of_match[] = {
	{
	 .compatible = "syna,berlin-vxg",
	 .data = (void *)&vpu_v4g_data,
	 },
	{
	 .compatible = "syna,berlin-h1",
	 .data = (void *)&vpu_h1_data,
	 },
	{
	 .compatible = "syna,dolphin-v4g",
	 .data = (void *)&vpu_v4g_data,
	 },
	{
	 .compatible = "syna,platypus-h1",
	 .data = (void *)&vpu_h1_lite_data,
	 },
	{
	 .compatible = "syna,dolphin-h1",
	 .data = (void *)&vpu_h1_data,
	 },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, syna_vpu_of_match);


int syna_mfd_request_connect(struct syna_vpu_auxiliary_device *auxdev)
{
	struct device *parent = auxdev->dev.parent;
	struct platform_device *pdev = to_platform_device(parent);
	struct syna_vpu_mfd_parent *vpu_dev = platform_get_drvdata(pdev);
	int ret;

	ret = mutex_lock_interruptible(&vpu_dev->lock);
	if (ret)
		return ret;

	if (vpu_dev->refcount) {
		if (vpu_dev->slot != auxdev->id) {
			mutex_unlock(&vpu_dev->lock);
			return -EBUSY;
		}
	} else {
		vpu_dev->slot = auxdev->id;
		ret = auxdev->drv->connect(auxdev);
		if (ret) {
			mutex_unlock(&vpu_dev->lock);
			return ret;
		}
	}
	vpu_dev->refcount++;
	mutex_unlock(&vpu_dev->lock);

	return 0;
}

int syna_mfd_disconnect(struct syna_vpu_auxiliary_device *auxdev)
{
	struct device *parent = auxdev->dev.parent;
	struct platform_device *pdev = to_platform_device(parent);
	struct syna_vpu_mfd_parent *vpu_dev = platform_get_drvdata(pdev);
	int ret;

	ret = mutex_lock_interruptible(&vpu_dev->lock);
	if (ret)
		return ret;

	vpu_dev->refcount--;
	if (vpu_dev->refcount == 0) {
		ret = auxdev->drv->disconnect(auxdev);
		if (ret) {
			vpu_dev->refcount++;
			mutex_unlock(&vpu_dev->lock);
			return ret;
		}
	}
	mutex_unlock(&vpu_dev->lock);

	return 0;
}

static const void *syna_vpu_get_drv_data(struct platform_device *pdev)
{
	struct syna_vpu_variant const *driver_data = NULL;

	if (pdev->dev.of_node) {
		const struct of_device_id *id = NULL;

		id = of_match_device(syna_vpu_of_match, &pdev->dev);
		if (id)
			driver_data = id->data;
	}
	return driver_data;
}

static int syna_vpu_mfd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct syna_vpu_mfd_parent *vpu_dev = NULL;
	const char *name = NULL;
	int ret = 0;

	dev_dbg(dev, "VPU probing\n");

	vpu_dev = devm_kzalloc(&pdev->dev, sizeof(struct syna_vpu_mfd_parent),
			       GFP_KERNEL);
	if (!vpu_dev)
		return -ENOMEM;

	vpu_dev->variant = syna_vpu_get_drv_data(pdev);
	if (!vpu_dev->variant)
		return -ENXIO;

	vpu_dev->dev = dev;
	mutex_init(&vpu_dev->lock);

#if defined(CONFIG_BERLIN_VPU_V4L2)
	ret = syna_vpu_fw_probe(dev);
	if (ret)
		goto failed;
#endif

	pm_runtime_enable(dev);

	vpu_dev->irq = platform_get_irq(pdev, 0);
	if (vpu_dev->irq < 0) {
		dev_err(dev, "No interrupt resource found\n");
		ret = -ENODEV;
		goto failed;
	}

	switch (vpu_dev->variant->hw_type) {
	case VPU_V2G:
		name = "dec";
		break;
	case VPU_H1_0:
		name = "enc";
		break;
	default:
		ret = -EINVAL;
		goto failed;
	}

#if defined(CONFIG_BERLIN_VPU_AMP)
	vpu_dev->subdev[0].id = 0;
	vpu_dev->subdev[0].drv = &syna_vpu_amp_drv;
	vpu_dev->subdev[0].irq = vpu_dev->irq;
	dev = &(vpu_dev->subdev[0].dev);
	device_initialize(dev);
	dev_set_name(dev, "vpu_amp_%s", name);
	dev->driver = &syna_vpu_driver.driver;
	dev->parent = &pdev->dev;
	if (device_add(dev))
		goto failed;

	ret = syna_vpu_amp_drv.probe(&vpu_dev->subdev[0], vpu_dev->variant);
	if (ret) {
		dev_err(dev, "probe failed\n");
		goto failed;
	}
#endif

#if defined(CONFIG_BERLIN_VPU_V4L2)
	vpu_dev->subdev[1].id = 1;
	vpu_dev->subdev[1].drv = &syna_vpu_v4l2_drv;
	vpu_dev->subdev[1].irq = vpu_dev->irq;
	dev = &(vpu_dev->subdev[1].dev);
	device_initialize(dev);
	dev_set_name(dev, "vpu_v4l2_%s", name);
	dev->driver = &syna_vpu_driver.driver;
	dev->parent = &pdev->dev;

	if (device_add(dev))
		goto failed;

	ret = syna_vpu_v4l2_drv.probe(&vpu_dev->subdev[1], vpu_dev->variant);
	if (ret) {
		dev_err(dev, "probe failed\n");
		goto failed;
	}
#endif
	platform_set_drvdata(pdev, vpu_dev);

	return 0;

failed:
	pm_runtime_disable(dev);
	return ret;
}

static PLAT_RET_TYPE syna_vpu_mfd_remove(struct platform_device *pdev)
{
	struct syna_vpu_mfd_parent *vpu_dev = platform_get_drvdata(pdev);
	struct device *dev;

#if defined(CONFIG_BERLIN_VPU_V4L2)
	syna_vpu_v4l2_drv.remove(&vpu_dev->subdev[1]);
	dev = &(vpu_dev->subdev[1].dev);
	device_del(dev);
#endif

#if defined(CONFIG_BERLIN_VPU_AMP)
	syna_vpu_amp_drv.remove(&vpu_dev->subdev[0]);
	dev = &(vpu_dev->subdev[0].dev);
	device_del(dev);
#endif
	pm_runtime_disable(vpu_dev->dev);

	PLAT_RET_VALUE(0);
}

static struct platform_driver syna_vpu_driver = {
	.probe = syna_vpu_mfd_probe,
	.remove = syna_vpu_mfd_remove,
	.driver = {
		   .name = "synaptics,berlin-vpu",
		   .of_match_table = of_match_ptr(syna_vpu_of_match),
		   },
};

static int __init syna_vpu_device_init(void)
{
	int ret;

	ret = syna_vpu_dh_memdev_alloc();
	if (ret) {
		pr_err("can't init ION memdevs\n");
		goto failed;
	}

	ret = platform_driver_register(&syna_vpu_driver);
	if (ret) {
		syna_vpu_dh_memdev_release();
		goto failed;
	}

	return ret;

failed:
	return ret;
}

static void __exit syna_vpu_device_exit(void)
{
	platform_driver_unregister(&syna_vpu_driver);
	syna_vpu_dh_memdev_release();
}

module_init(syna_vpu_device_init);
module_exit(syna_vpu_device_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("VideoSmart VPU");
