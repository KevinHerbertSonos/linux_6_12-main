// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 - 2024 Synaptics Incorporated.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>

#include "vpu_kernel_compatibility_wrap.h"
#include "vpu_srv.h"
#include "h1_irq.h"

#define DEV_NAME "h1_irq"

struct h1_dev_ctx {
	struct cdev cdev;
	struct device *device;
	dev_t devt;
	struct class *class;
	struct syna_vpu_srv *srv_node;
};

static struct h1_dev_ctx g_h1_ctx;

static long h1_irq_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct h1_dev_ctx *h1_ctx = (struct h1_dev_ctx *)filp->private_data;
	int ret = 0;

	switch (cmd) {
	case H1_IOCTL_POLL_INT:
		vpu_srv_wait_isr(h1_ctx->srv_node, 2000);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int h1_irq_open(struct inode *inode, struct file *filp)
{
	struct h1_dev_ctx *h1_ctx = NULL;

	h1_ctx = container_of(inode->i_cdev, struct h1_dev_ctx, cdev);

	filp->private_data = (void *)h1_ctx;

	return 0;
}

static const struct file_operations h1_irq_fops = {
	.open = h1_irq_open,
	.unlocked_ioctl = h1_irq_ioctl,
	.compat_ioctl = h1_irq_ioctl,
};

int h1_irq_init(struct syna_vpu_srv *srv_node)
{
	int ret = 0;
	dev_t devno;

	ret = alloc_chrdev_region(&g_h1_ctx.devt, 0, 1, DEV_NAME);
	if (ret) {
		pr_err("fail to alloc number\n");
		return ret;
	}

	g_h1_ctx.class = vpu_create_dev_class(DEV_NAME);
	if (IS_ERR(g_h1_ctx.class)) {
		ret = PTR_ERR(g_h1_ctx.class);
		pr_err("Unable to create %s class %d\n", DEV_NAME, ret);
		goto err_0;
	}

	cdev_init(&g_h1_ctx.cdev, &h1_irq_fops);

	devno = MKDEV(MAJOR(g_h1_ctx.devt), 0);
	ret = cdev_add(&g_h1_ctx.cdev, devno, 1);
	if (ret) {
		pr_err("cdev_add() failed %d\n", ret);
		goto err_1;
	}

	g_h1_ctx.device =
		device_create(g_h1_ctx.class, NULL, devno, NULL, DEV_NAME);
	if (IS_ERR(g_h1_ctx.device)) {
		ret = PTR_ERR(g_h1_ctx.device);
		pr_err("Unable to create dev-%d %d\n", MINOR(devno), ret);
		goto err_2;
	}

	g_h1_ctx.srv_node = srv_node;

	pr_info("dev %s [%d,%d] init success\n", DEV_NAME, MAJOR(devno),
		MINOR(devno));

	return 0;
err_2:
	cdev_del(&g_h1_ctx.cdev);
err_1:
	class_destroy(g_h1_ctx.class);
err_0:
	unregister_chrdev_region(g_h1_ctx.devt, 1);
	return ret;
}

void h1_irq_destroy(void)
{
	dev_t devno;

	cdev_del(&g_h1_ctx.cdev);
	devno = MKDEV(MAJOR(g_h1_ctx.devt), 0);
	device_destroy(g_h1_ctx.class, devno);

	class_destroy(g_h1_ctx.class);
	unregister_chrdev_region(g_h1_ctx.devt, 1);
}
