// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 VeriSilicon Holdings Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 * The GPL License (GPL)
 *
 * Copyright (c) 2023 VeriSilicon Holdings Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 *****************************************************************************
 *
 * Note: This software is released under dual MIT and GPL licenses. A
 * recipient may use this file under the terms of either the MIT license or
 * GPL License. If you wish to use only one license not the other, you can
 * indicate your decision by deleting one of the above license notices in your
 * version of this file.
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "vvcam_dw_driver.h"
#include "vvcam_dw_platform.h"
#include "vvcam_dw.h"
#include "vvcam_event.h"

#define ALIGN_UP(x, align_to)	(((x) + ((align_to) - 1)) & ~((align_to) -1))

static irqreturn_t vvcam_dwe_irq_handler(int irq, void *dw_dev)
{
	return vvcam_dwe_irq_process(dw_dev);
}

static irqreturn_t vvcam_vse_irq_handler(int irq, void *dw_dev)
{
	return vvcam_vse_irq_process(dw_dev);
}

static irqreturn_t vvcam_dw_fe_irq_handler(int irq, void *dw_dev)
{
	return vvcam_dw_fe_irq_process(dw_dev);
}

int vvcam_dw_get_reg_mem(struct vvcam_dw_dev *dwe_dev,
		vvcam_dw_reg_mem_t *dw_reg_mem)
{
	dw_reg_mem->base = dwe_dev->vse.paddr;
	dw_reg_mem->size = ALIGN_UP(dwe_dev->dwe.paddr - dwe_dev->vse.paddr
						+ dwe_dev->dwe.regs_size, PAGE_SIZE);
	return 0;
}

static int vvcam_dw_open(struct inode *inode, struct file *file)
{
	struct miscdevice *pmisc_dev = file->private_data;
	struct vvcam_dw_dev *dw_dev;
	struct vvcam_dw_fh *dw_fh;

	dw_dev = container_of(pmisc_dev, struct vvcam_dw_dev, miscdev);
	if (!dw_dev)
		return -ENOMEM;

	dw_fh = kzalloc(sizeof(struct vvcam_dw_fh), GFP_KERNEL);
	if (!dw_fh)
		return -ENOMEM;

	dw_fh->dw_dev = dw_dev;
	vvcam_event_fh_init( &dw_dev->event_dev, &dw_fh->event_fh);
	file->private_data = dw_fh;

	mutex_lock(&dw_dev->mlock);
	dw_dev->refcnt++;
	pm_runtime_get_sync(dw_dev->dev);
	mutex_unlock(&dw_dev->mlock);

	return 0;
}

static int vvcam_dw_release(struct inode *inode, struct file *file)
{
	struct vvcam_dw_dev *dw_dev;
	struct vvcam_dw_fh *dw_fh;

	dw_fh = file->private_data;
	dw_dev = dw_fh->dw_dev;

	vvcam_event_unsubscribe_all(&dw_fh->event_fh);

	vvcam_event_fh_destroy(&dw_dev->event_dev, &dw_fh->event_fh);

	mutex_lock(&dw_dev->mlock);
	dw_dev->refcnt--;
	pm_runtime_put(dw_dev->dev);
	mutex_unlock(&dw_dev->mlock);

	kfree(dw_fh);

	return 0;
}

static long vvcam_dw_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct vvcam_dw_dev *dw_dev;
	struct vvcam_dw_fh *dw_fh;
	vvcam_dw_reg_t dw_reg;
	vvcam_subscription_t sub;
	vvcam_event_t event;
	vvcam_dw_reg_mem_t dw_reg_mem;
	int ret = 0;

	dw_fh = file->private_data;
	dw_dev = dw_fh->dw_dev;

	mutex_lock(&dw_dev->mlock);

	switch(cmd) {
		case VVCAM_DWE_RESET:
			ret = vvcam_dwe_reset(dw_dev);
			break;
		case VVCAM_DWE_READ_REG:
			ret = copy_from_user(&dw_reg, (void __user *)arg, sizeof(dw_reg));
			if (ret)
				break;
			ret = vvcam_dwe_read_reg(dw_dev, &dw_reg);
			if (ret)
				break;
			ret = copy_to_user((void __user *)arg, &dw_reg, sizeof(dw_reg));
			break;
		case VVCAM_DWE_WRITE_REG:
			ret = copy_from_user(&dw_reg, (void __user *)arg, sizeof(dw_reg));
			if (ret)
				break;
			ret = vvcam_dwe_write_reg(dw_dev, dw_reg);
			break;
		case VVCAM_VSE_RESET:
			ret = vvcam_vse_reset(dw_dev);
			break;
		case VVCAM_VSE_READ_REG:
			ret = copy_from_user(&dw_reg, (void __user *)arg, sizeof(dw_reg));
			if (ret)
				break;
			ret = vvcam_vse_read_reg(dw_dev, &dw_reg);
			if (ret)
				break;
			ret = copy_to_user((void __user *)arg, &dw_reg, sizeof(dw_reg));
			break;
		case VVCAM_VSE_WRITE_REG:
			ret = copy_from_user(&dw_reg, (void __user *)arg, sizeof(dw_reg));
			if (ret)
				break;
			ret = vvcam_vse_write_reg(dw_dev, dw_reg);
			break;

		case VVCAM_DW_SUBSCRIBE_EVENT:
			ret = copy_from_user(&sub, (void __user *)arg, sizeof(sub));
			if (ret)
				break;
			ret = vvcam_event_subscribe(&dw_fh->event_fh,
						&sub, VVCAM_DW_EVENT_ELEMS);
			break;
		case VVCAM_DW_UNSUBSCRIBE_EVENT:
			ret = copy_from_user(&sub, (void __user *)arg, sizeof(sub));
			if (ret)
				break;
			ret = vvcam_event_unsubscribe(&dw_fh->event_fh, &sub);
			break;
		case VVCAM_DW_DQEVENT:
			ret = vvcam_event_dequeue(&dw_fh->event_fh, &event);
			if (ret)
				break;
			ret = copy_to_user((void __user *)arg, &event, sizeof(event));
			break;
		case VVCAM_DW_REG_MEM:
			ret = copy_from_user(&dw_reg_mem, (void __user *)arg, sizeof(dw_reg_mem));
			if (ret)
				break;
			ret = vvcam_dw_get_reg_mem(dw_dev, &dw_reg_mem);
			if (ret)
				break;
			ret = copy_to_user((void __user *)arg, &dw_reg_mem, sizeof(dw_reg_mem));
			break;
		default:
			ret = -EINVAL;
			break;
	}

	mutex_unlock(&dw_dev->mlock);

	return ret;
}

static unsigned int vvcam_dw_poll(struct file *file, poll_table *wait)
{
	struct vvcam_dw_fh *dw_fh;

	dw_fh = file->private_data;

	return vvcam_event_poll(file, &dw_fh->event_fh, wait);
}

int vvcam_dw_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct vvcam_dw_dev *dw_dev;
	struct vvcam_dw_fh *dw_fh;
	uint32_t size = 0;

	dw_fh = file->private_data;
	dw_dev = dw_fh->dw_dev;

	size = dw_dev->dwe.paddr - dw_dev->vse.paddr + dw_dev->dwe.regs_size;
	size = ALIGN_UP(size, PAGE_SIZE);

	vma->vm_pgoff = 0;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return vm_iomap_memory(vma, dw_dev->vse.paddr, size);
}

static struct file_operations vvcam_dw_fops = {
	.owner			= THIS_MODULE,
	.open			= vvcam_dw_open,
	.release		= vvcam_dw_release,
	.unlocked_ioctl = vvcam_dw_ioctl,
	.poll			= vvcam_dw_poll,
	.mmap			= vvcam_dw_mmap,
};

static int vvcam_dw_parse_params(struct vvcam_dw_dev *dw_dev,
						struct platform_device *pdev)
{
	struct resource *res;

	res =  platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "can't fetch device resource info\n");
		return -EIO;
	}
	dw_dev->dwe.paddr = res->start;
	dw_dev->dwe.regs_size = resource_size(res);
	dw_dev->dwe.base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dw_dev->dwe.base)) {
		dev_err(&pdev->dev, "can't remap device resource info\n");
		return PTR_ERR( dw_dev->dwe.base);
	}

	dw_dev->dwe.irq = platform_get_irq(pdev, 0);
	if (dw_dev->dwe.irq < 0) {
		dev_err(&pdev->dev, "can't get dwe irq resource\n");
		return -ENXIO;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "can't fetch device resource info\n");
		return -EIO;
	}
	dw_dev->vse.paddr = res->start;
	dw_dev->vse.regs_size = resource_size(res);
	dw_dev->vse.base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dw_dev->vse.base)) {
		dev_err(&pdev->dev, "can't remap device resource info\n");
		return PTR_ERR( dw_dev->vse.base);
	}

	dw_dev->vse.irq = platform_get_irq(pdev, 1);
	if (dw_dev->vse.irq < 0) {
		dev_err(&pdev->dev, "can't get vse irq resource\n");
		return -ENXIO;
	}

	dw_dev->fe_irq = platform_get_irq(pdev, 2);
	if (dw_dev->fe_irq < 0) {
		dev_err(&pdev->dev, "can't get dw fe resource\n");
		return -ENXIO;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res) {
		dev_err(&pdev->dev, "can't fetch device resource info\n");
		return -EIO;
	}
	dw_dev->reset = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dw_dev->vse.base)) {
		dev_err(&pdev->dev, "can't remap device resource info\n");
		return PTR_ERR( dw_dev->vse.base);
	}

	return 0;
}

static int vvcam_dw_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct vvcam_dw_dev *dw_dev;

	dw_dev = devm_kzalloc(&pdev->dev,
				sizeof(struct vvcam_dw_dev), GFP_KERNEL);
	if (!dw_dev)
		return -ENOMEM;

	ret = vvcam_dw_parse_params(dw_dev, pdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to parse params\n");
		return -EINVAL;
	}

	mutex_init(&dw_dev->mlock);
	vvcam_event_dev_init(&dw_dev->event_dev);
	platform_set_drvdata(pdev, dw_dev);

	dw_dev->dev = &pdev->dev;
	dw_dev->id  = pdev->id;

	dw_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dw_dev->miscdev.name  = VVCAM_DW_NAME;
	dw_dev->miscdev.fops  = &vvcam_dw_fops;

	ret = misc_register(&dw_dev->miscdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register device\n");
		return -EINVAL;
	}

	ret = devm_request_irq(&pdev->dev, dw_dev->dwe.irq, vvcam_dwe_irq_handler,
				IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), dw_dev);
	if (ret) {
		dev_err(&pdev->dev, "can't request dewarp irq\n");
		goto error_request_dwe_irq;
	}

	ret = devm_request_irq(&pdev->dev, dw_dev->vse.irq, vvcam_vse_irq_handler,
				IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), dw_dev);
	if (ret) {
		dev_err(&pdev->dev, "can't request vse irq\n");
		goto error_request_vse_irq;
	}

	ret = devm_request_irq(&pdev->dev, dw_dev->fe_irq, vvcam_dw_fe_irq_handler,
				IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), dw_dev);
	if (ret) {
		dev_err(&pdev->dev, "can't request fe irq\n");
		goto error_request_vse_irq;
	}

	dev_info(&pdev->dev, "vvcam dewarp driver probe success\n");

	return 0;

error_request_dwe_irq:
	devm_free_irq(&pdev->dev, dw_dev->dwe.irq, dw_dev);
error_request_vse_irq:
	misc_deregister(&dw_dev->miscdev);
	return ret;
}

static int vvcam_dw_remove(struct platform_device *pdev)
{
	struct vvcam_dw_dev *dw_dev;

	dw_dev = platform_get_drvdata(pdev);

	misc_deregister(&dw_dev->miscdev);
	devm_free_irq(&pdev->dev, dw_dev->dwe.irq, dw_dev);
	devm_free_irq(&pdev->dev, dw_dev->vse.irq, dw_dev);

	return 0;
}

static int vvcam_dw_system_suspend(struct device *dev)
{
	int ret = 0;
	ret = pm_runtime_force_suspend(dev);
	if (ret) {
		dev_err(dev, "force suspend %s failed\n", dev_name(dev));
		return ret;
	}
	return ret;
}

static int vvcam_dw_system_resume(struct device *dev)
{
	int ret = 0;
	ret = pm_runtime_force_resume(dev);
	if (ret) {
		dev_err(dev, "force resume %s failed\n", dev_name(dev));
		return ret;
	}
	return ret;
}

static int vvcam_dw_runtime_suspend(struct device *dev)
{
	return 0;
}

static int vvcam_dw_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops vvcam_dw_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vvcam_dw_system_suspend, vvcam_dw_system_resume)
	SET_RUNTIME_PM_OPS(vvcam_dw_runtime_suspend, vvcam_dw_runtime_resume, NULL)
};

static const struct of_device_id vvcam_dw_of_match[] = {
	{.compatible = "verislicon,dw200",},
	{ /* sentinel */ },
};

static struct platform_driver vvcam_dw_driver = {
	.probe  = vvcam_dw_probe,
	.remove = vvcam_dw_remove,
	.driver = {
		.name			= VVCAM_DW_NAME,
		.owner			= THIS_MODULE,
		.of_match_table	= vvcam_dw_of_match,
		.pm				= &vvcam_dw_pm_ops,
	}
};

static int __init vvcam_dw_init_module(void)
{
	int ret;
	ret = platform_driver_register(&vvcam_dw_driver);
	if (ret) {
		printk(KERN_ERR "Failed to register dw driver\n");
		return ret;
	}

	ret = vvcam_dw_platform_device_register();
	if (ret) {
		platform_driver_unregister(&vvcam_dw_driver);
		printk(KERN_ERR "Failed to register vvcam dw platform devices\n");
		return ret;
	}

	return ret;
}

static void __exit vvcam_dw_exit_module(void)
{
	platform_driver_unregister(&vvcam_dw_driver);
	vvcam_dw_platform_device_unregister();
}

module_init(vvcam_dw_init_module);
module_exit(vvcam_dw_exit_module);

MODULE_DESCRIPTION("Verisilicon dw200 driver");
MODULE_AUTHOR("Verisilicon ISP SW Team");
MODULE_LICENSE("GPL");
