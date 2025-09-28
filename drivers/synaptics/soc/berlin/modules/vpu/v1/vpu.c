// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 * Copyright (C) 2022 - 2023 Synaptics Incorporated.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include "vpu_kernel_compatibility_wrap.h"

#include <vpu_mfd.h>

#define VPU_INT_TIMEOUT				(2000U)
#define VPU_POWER_TRANS_TIMEOUT			(200U)
#define VPU_POWER_TRANS_RETRY			(3U)

#define VPU_DEVICE_TAG		"[vpu kernel driver] "
#define vpu_debug(...)		pr_dbg( VPU_DEVICE_TAG __VA_ARGS__)
#define vpu_trace(...)		pr_info( VPU_DEVICE_TAG __VA_ARGS__)
#define vpu_error(...)		pr_err( VPU_DEVICE_TAG __VA_ARGS__)

enum {
	VPU_IOCTL_CMD_MSG = 0,
	VDEC_IOCTL_POLL_INT,
	VPU_IOCTL_DISABLE_INT,
	VPU_IOCTL_ENABLE_INT,
	VPU_IOCTL_SET_INT_AFFINITY,
	VPU_IOCTL_WAKEUP,
	/* In the device critial state, system would not suspend or sleep */
	VPU_IOCTL_ENTRY_CRITICAL,
	VPU_IOCTL_EXIT_CRITICAL,
};

typedef struct _VPU_HW_IP_CTX_ {
	u32 hw_id;
	const char *hw_name;
	int irq;
	struct clk *vpuclk;
	struct cdev cdev;
	struct class *dev_class;
	dev_t vpu_devt;
	struct device *dev;
	atomic_t inst_available;

	struct completion ree_irq;

	struct completion power_transaction;
	struct notifier_block pm_notifier;

	struct syna_vpu_auxiliary_device *auxdev;
} VPU_HW_IP_CTX;

/*******************************************************************************
  Module internal function
  */
static irqreturn_t vpu_devices_isr(int irq, void *dev_id)
{
	VPU_HW_IP_CTX *vpu = (VPU_HW_IP_CTX *) dev_id;

	complete(&vpu->ree_irq);

	return IRQ_HANDLED;
}

/*
 * Module Register API
 */
static long vpu_driver_ioctl_unlocked(struct file *filp, unsigned int cmd,
				      unsigned long arg)
{
	VPU_HW_IP_CTX *pVpuHwCtx = (VPU_HW_IP_CTX *) filp->private_data;
	cpumask_t cpu_mask;
	long timeout;
	int retry, i;
	bool pending;

	switch (cmd) {
	case VDEC_IOCTL_POLL_INT:
		timeout =
		    wait_for_completion_killable_timeout(&pVpuHwCtx->ree_irq,
							 msecs_to_jiffies
							 (VPU_INT_TIMEOUT));
		if (timeout > 0)
			return 0;
		if (-ERESTARTSYS == timeout)
			dev_err(pVpuHwCtx->dev, "waiting irq is cancelled\n");
		else
			/* timeout here */
			return -1;

		break;
	case VPU_IOCTL_DISABLE_INT:
		if (completion_done(&pVpuHwCtx->ree_irq))
			dev_err(pVpuHwCtx->dev, "didn't handle all the irq\n");
		break;
	case VPU_IOCTL_ENABLE_INT:
		if (pending =
		    try_wait_for_completion(&pVpuHwCtx->ree_irq), pending) {
			dev_err(pVpuHwCtx->dev,
				"has unhandled irq from the last time, HW may in a bad state\n");
		}
		break;
	case VPU_IOCTL_SET_INT_AFFINITY:
		{
			if (get_user(i, (unsigned int __user *)arg))
				return -EFAULT;

			cpumask_clear(&cpu_mask);
			cpumask_set_cpu(i % num_online_cpus(), &cpu_mask);
			return irq_set_affinity_hint(pVpuHwCtx->irq, &cpu_mask);
		}
		break;
	case VPU_IOCTL_ENTRY_CRITICAL:
		retry = 0;
		do {
			timeout =
				wait_for_completion_interruptible_timeout
				(&pVpuHwCtx->power_transaction,
				msecs_to_jiffies(VPU_POWER_TRANS_TIMEOUT));
			if (timeout > 0)
				return 0;

			if (-ERESTARTSYS == timeout)
				try_to_freeze();
			else
				retry++;
		} while (retry < VPU_POWER_TRANS_RETRY);

		return -EBUSY;
		break;
	case VPU_IOCTL_EXIT_CRITICAL:
		complete(&pVpuHwCtx->power_transaction);
		break;
	case VPU_IOCTL_WAKEUP:
	default:
		break;
	}
	return 0;
}

static int vpu_pm_notifier(struct notifier_block *notifier,
			   unsigned long pm_event, void *unused)
{
	VPU_HW_IP_CTX *vpu = container_of(notifier, VPU_HW_IP_CTX, pm_notifier);

	switch (pm_event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		complete(&vpu->power_transaction);
		break;
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		dev_info(vpu->dev, "prevents system sleeping\n");
		wait_for_completion(&vpu->power_transaction);
		dev_info(vpu->dev, "could continue to sleep\n");
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static int vpu_driver_open(struct inode *inode, struct file *filp)
{
	VPU_HW_IP_CTX *vpu =
	    container_of(inode->i_cdev, struct _VPU_HW_IP_CTX_, cdev);
	int ret;

	if (!atomic_dec_and_test(&vpu->inst_available)) {
		atomic_inc(&vpu->inst_available);
		return -EBUSY;
	}

	ret = syna_mfd_request_connect(vpu->auxdev);

	filp->private_data = vpu;

	return nonseekable_open(inode, filp);
}

static int vpu_driver_release(struct inode *inode, struct file *filp)
{
	VPU_HW_IP_CTX *vpu =
	    container_of(inode->i_cdev, struct _VPU_HW_IP_CTX_, cdev);

	atomic_inc(&vpu->inst_available);

	if (!completion_done(&vpu->power_transaction))
		complete(&vpu->power_transaction);

	return syna_mfd_disconnect(vpu->auxdev);
}

static const struct file_operations vpu_ops = {
	.open = vpu_driver_open,
	.release = vpu_driver_release,
	.unlocked_ioctl = vpu_driver_ioctl_unlocked,
	.compat_ioctl = vpu_driver_ioctl_unlocked,
	.owner = THIS_MODULE,
};

static int syna_vpu_amp_connect(struct syna_vpu_auxiliary_device *auxdev)
{
	VPU_HW_IP_CTX *pVpuHwCtx = dev_get_drvdata(&auxdev->dev);
	int ret;

	ret = devm_request_threaded_irq(&auxdev->dev, auxdev->irq, NULL,
					vpu_devices_isr, IRQF_ONESHOT,
					pVpuHwCtx->hw_name, (void *)pVpuHwCtx);
	if (ret) {
		dev_err(&auxdev->dev, "register interrupter runtime failed\n");
		return ret;
	}

	return 0;
}

static int syna_vpu_amp_disconnect(struct syna_vpu_auxiliary_device *auxdev)
{
	VPU_HW_IP_CTX *pVpuHwCtx = dev_get_drvdata(&auxdev->dev);

	devm_free_irq(&auxdev->dev, auxdev->irq, (void *)pVpuHwCtx);

	return 0;
}

static int berlin_vpu_probe(struct syna_vpu_auxiliary_device *auxdev,
			    const struct syna_vpu_variant *variant)
{
	int err;
	struct device *dev = NULL;
	VPU_HW_IP_CTX *pVpuHwCtx = NULL;
	const char *hw_name = NULL;

	vpu_trace("vpu probe enter\n");
	pVpuHwCtx = devm_kzalloc(&auxdev->dev, sizeof(VPU_HW_IP_CTX),
				 GFP_KERNEL);

	if (!pVpuHwCtx)
		return -ENOMEM;

	pVpuHwCtx->dev = &(auxdev->dev);
	switch (variant->hw_type) {
	case VPU_V2G:
		hw_name = "vxg";
		break;
	case VPU_H1_0:
		hw_name = "h1";
		break;
	case VPU_H1_1:
		hw_name = "h1_1";
		break;
	case VPU_VMETA:
		hw_name = "vmeta";
		break;
	case VPU_G2:
		hw_name = "g2";
		break;
	case VPU_G1:
		hw_name = "g1";
		break;
	default:
		return -EINVAL;
		break;
	};
	vpu_trace("find vpu device %s\n", hw_name);

	pVpuHwCtx->hw_name = hw_name;
	init_completion(&pVpuHwCtx->ree_irq);

	err =
	    alloc_chrdev_region(&pVpuHwCtx->vpu_devt, 0, 1, hw_name);
	if (err < 0) {
		vpu_error("failed to allocate dev num\n");
		goto err_prob_device_0;
	}

	cdev_init(&pVpuHwCtx->cdev, &vpu_ops);
	pVpuHwCtx->cdev.owner = THIS_MODULE;
	/* Add the device */
	err =
	    cdev_add(&pVpuHwCtx->cdev, MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), 1);
	if (err)
		goto err_prob_device_1;

	/* add vpu devices to sysfs */
	pVpuHwCtx->dev_class = vpu_create_dev_class(hw_name);

	if (IS_ERR(pVpuHwCtx->dev_class)) {
		vpu_error("class_create failed.\n");
		err = PTR_ERR(pVpuHwCtx->dev_class);
		goto err_prob_device_2;
	}

	atomic_set(&pVpuHwCtx->inst_available, 1);

	dev = device_create(pVpuHwCtx->dev_class, NULL,
			    MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), NULL,
			    hw_name);
	if (IS_ERR(dev)) {
		vpu_error("device_create failed.\n");
		err = PTR_ERR(dev);
		goto err_prob_device_3;
	}

	init_completion(&pVpuHwCtx->power_transaction);
	complete(&pVpuHwCtx->power_transaction);
	pVpuHwCtx->pm_notifier.notifier_call = vpu_pm_notifier;
	register_pm_notifier(&pVpuHwCtx->pm_notifier);

	pVpuHwCtx->auxdev = auxdev;
	dev_set_drvdata(&auxdev->dev, pVpuHwCtx);

	vpu_trace("vpu:%s probe ok\n", hw_name);
	return 0;

err_prob_device_3:
	class_destroy(pVpuHwCtx->dev_class);
err_prob_device_2:
	cdev_del(&pVpuHwCtx->cdev);
err_prob_device_1:
	unregister_chrdev_region(MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), 1);
err_prob_device_0:
	return err;
}

static void berlin_vpu_remove(struct syna_vpu_auxiliary_device *auxdev)
{
	VPU_HW_IP_CTX *pVpuHwCtx = NULL;
	vpu_trace("vpu remove enter\n");

	pVpuHwCtx = dev_get_drvdata(&auxdev->dev);

	vpu_trace("revomve vpu:%s\n", pVpuHwCtx->hw_name);

	if (pVpuHwCtx->dev_class) {
		device_destroy(pVpuHwCtx->dev_class,
			       MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0));
		class_destroy(pVpuHwCtx->dev_class);
	}

	cdev_del(&pVpuHwCtx->cdev);
	unregister_chrdev_region(MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), 1);

	unregister_pm_notifier(&pVpuHwCtx->pm_notifier);
	vpu_trace("vpu:%s remove ok\n", pVpuHwCtx->hw_name);
}

const struct syna_vpu_auxiliary_driver syna_vpu_amp_drv = {
	.probe = &berlin_vpu_probe,
	.remove = &berlin_vpu_remove,
	.connect = &syna_vpu_amp_connect,
	.disconnect = &syna_vpu_amp_disconnect,
};
