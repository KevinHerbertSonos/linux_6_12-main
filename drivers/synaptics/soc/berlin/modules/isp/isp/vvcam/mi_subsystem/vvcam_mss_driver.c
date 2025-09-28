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
#include <linux/slab.h>
#include <linux/mm.h>

#include "vvcam_mss_driver.h"
#include "vvcam_mss_platform.h"
#include "vvcam_mss.h"


static int vvcam_mss_open(struct inode *inode, struct file *file)
{
    struct miscdevice *pmisc_dev = file->private_data;
    struct vvcam_mss_dev *mss_dev;
    struct vvcam_mss_fh *mss_fh;

    mss_dev = container_of(pmisc_dev, struct vvcam_mss_dev, miscdev);
    if (!mss_dev)
        return -ENOMEM;

    mss_fh = kzalloc(sizeof(struct vvcam_mss_fh), GFP_KERNEL);
    if (!mss_fh)
        return -ENOMEM;

    mss_fh->mss_dev = mss_dev;
    file->private_data = mss_fh;

    mutex_lock(&mss_dev->mlock);
    mss_dev->refcnt++;
    pm_runtime_get_sync(mss_dev->dev);
    mutex_unlock(&mss_dev->mlock);

    return 0;
}

static int vvcam_mss_release(struct inode *inode, struct file *file)
{
    struct vvcam_mss_dev *mss_dev;
    struct vvcam_mss_fh *mss_fh;

    mss_fh = file->private_data;
    mss_dev = mss_fh->mss_dev;

    mutex_lock(&mss_dev->mlock);
    mss_dev->refcnt--;
    pm_runtime_put(mss_dev->dev);
    mutex_unlock(&mss_dev->mlock);

    kfree(mss_fh);

    return 0;
}

static long vvcam_mss_ioctl(struct file *file,
                        unsigned int cmd, unsigned long arg)
{
    struct vvcam_mss_dev *mss_dev;
    struct vvcam_mss_fh *mss_fh;
    uint32_t reset;
    vvcam_mss_reg_t mss_reg;
    int ret = 0;

    mss_fh = file->private_data;
    mss_dev = mss_fh->mss_dev;

    mutex_lock(&mss_dev->mlock);

    switch(cmd) {
    case VVCAM_MSS_RESET:
        ret = copy_from_user(&reset, (void __user *)arg, sizeof(reset));
        if (ret)
            break;
        ret = vvcam_mss_reset(mss_dev, reset);
        break;
    case VVCAM_MSS_READ_REG:
        ret = copy_from_user(&mss_reg, (void __user *)arg, sizeof(mss_reg));
        if (ret)
            break;
        ret = vvcam_mss_read_reg(mss_dev, &mss_reg);
        if (ret)
            break;
        ret = copy_to_user((void __user *)arg, &mss_reg, sizeof(mss_reg));
        break;
    case VVCAM_MSS_WRITE_REG:
        ret = copy_from_user(&mss_reg, (void __user *)arg, sizeof(mss_reg));
        if (ret)
            break;
        ret = vvcam_mss_write_reg(mss_dev, mss_reg);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    mutex_unlock(&mss_dev->mlock);

    return ret;
}

int vvcam_mss_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct vvcam_mss_dev *mss_dev;
    struct vvcam_mss_fh *mss_fh;

    mss_fh = file->private_data;
    mss_dev = mss_fh->mss_dev;

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    return vm_iomap_memory(vma, mss_dev->paddr, mss_dev->regs_size);
}

static struct file_operations vvcam_mss_fops = {
    .owner          = THIS_MODULE,
    .open           = vvcam_mss_open,
    .release        = vvcam_mss_release,
    .unlocked_ioctl = vvcam_mss_ioctl,
    .mmap           = vvcam_mss_mmap,

};

static int vvcam_mss_parse_params(struct vvcam_mss_dev *mss_dev,
                        struct platform_device *pdev)
{
    struct resource *res;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "can't fetch device resource info\n");
        return -EIO;
    }

    mss_dev->paddr = res->start;
    mss_dev->regs_size = resource_size(res);
    mss_dev->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(mss_dev->base)) {
        dev_err(&pdev->dev, "can't remap device resource info\n");
        return PTR_ERR(mss_dev->base);
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) {
        dev_err(&pdev->dev, "can't fetch device reset reg\n");
        return -EIO;
    }

    mss_dev->reset = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (IS_ERR(mss_dev->reset)) {
        dev_err(&pdev->dev, "can't remap device reset reg\n");
        return PTR_ERR(mss_dev->reset);
    }

    return 0;
}

static int vvcam_mss_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct vvcam_mss_dev *mss_dev;

    mss_dev = devm_kzalloc(&pdev->dev,
                sizeof(struct vvcam_mss_dev), GFP_KERNEL);
    if (!mss_dev)
        return -ENOMEM;

    ret = vvcam_mss_parse_params(mss_dev, pdev);
    if (ret) {
        dev_err(&pdev->dev, "failed to parse params\n");
        return -EINVAL;
    }

    mutex_init(&mss_dev->mlock);
    spin_lock_init(&mss_dev->stat_lock);
    platform_set_drvdata(pdev, mss_dev);

    mss_dev->dev = &pdev->dev;
    mss_dev->id  = pdev->id;

    mss_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    mss_dev->miscdev.name  = VVCAM_MSS_NAME;
    mss_dev->miscdev.fops  = &vvcam_mss_fops;

    ret = misc_register(&mss_dev->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "failed to register device\n");
        return -EINVAL;
    }

    dev_info(&pdev->dev, "vvcam mss driver probe success\n");

    return ret;
}

static int vvcam_mss_remove(struct platform_device *pdev)
{
    struct vvcam_mss_dev *mss_dev;

    mss_dev = platform_get_drvdata(pdev);

    misc_deregister(&mss_dev->miscdev);

    return 0;
}

static int vvcam_mss_system_suspend(struct device *dev)
{
    int ret = 0;
    ret = pm_runtime_force_suspend(dev);
    if (ret) {
        dev_err(dev, "force suspend %s failed\n", dev_name(dev));
        return ret;
    }
    return ret;
}

static int vvcam_mss_system_resume(struct device *dev)
{
    int ret = 0;
    ret = pm_runtime_force_resume(dev);
    if (ret) {
        dev_err(dev, "force resume %s failed\n", dev_name(dev));
        return ret;
    }
    return ret;
}

static int vvcam_mss_runtime_suspend(struct device *dev)
{
    return 0;
}

static int vvcam_mss_runtime_resume(struct device *dev)
{
    return 0;
}

static const struct dev_pm_ops vvcam_mss_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(vvcam_mss_system_suspend, vvcam_mss_system_resume)
    SET_RUNTIME_PM_OPS(vvcam_mss_runtime_suspend, vvcam_mss_runtime_resume, NULL)
};

static const struct of_device_id vvcam_mss_of_match[] = {
    {.compatible = "verislicon,mss",},
    { /* sentinel */ },
};

static struct platform_driver vvcam_mss_driver = {
    .probe  = vvcam_mss_probe,
    .remove = vvcam_mss_remove,
    .driver = {
        .name           = VVCAM_MSS_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = vvcam_mss_of_match,
        .pm             = &vvcam_mss_pm_ops,
    }
};

static int __init vvcam_mss_init_module(void)
{
    int ret;
    ret = platform_driver_register(&vvcam_mss_driver);
    if (ret) {
        printk(KERN_ERR "Failed to register mss driver\n");
        return ret;
    }

    ret = vvcam_mss_platform_device_register();
    if (ret) {
        platform_driver_unregister(&vvcam_mss_driver);
        printk(KERN_ERR "Failed to register vvcam mss platform devices\n");
        return ret;
    }

    return ret;
}

static void __exit vvcam_mss_exit_module(void)
{
    platform_driver_unregister(&vvcam_mss_driver);
    vvcam_mss_platform_device_unregister();
}

module_init(vvcam_mss_init_module);
module_exit(vvcam_mss_exit_module);

MODULE_DESCRIPTION("Verisilicon MI sub-system driver");
MODULE_AUTHOR("Verisilicon ISP SW Team");
MODULE_LICENSE("GPL");
