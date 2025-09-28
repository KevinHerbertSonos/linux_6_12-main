// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2024 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014 - 2024 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
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
#include <linux/version.h>
#ifdef DOLPHIN
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#endif

#include "vvcam_isp_driver.h"
#include "vvcam_isp_platform.h"
#include "vvcam_isp.h"
#include "vvcam_event.h"
#include "vvcam_isp_procfs.h"

#ifdef DOLPHIN
#define ISP_GLOBAL_BASE         0x40000
#define ISP_INT_STATUS          0x194
#define MI_IRQ_BIT              0x1
#define ISP_IRQ_BIT             0x2
#define ISP_CORE_BASE           0x60000
#define ISP_ISP_INTR_STS_REG    0x5c4
#define ISP_ISP_INTR_CLR_REG    0x5c8

#define MAX_SENSOR              0x2
#endif


extern void vvcam_isp_irq_stat_tasklet(unsigned long);

#ifdef DOLPHIN
static irqreturn_t vvcam_isp_generic_irq_handler(int irq, void *dev)
{
    unsigned int ra_intstat;
    struct vvcam_isp_dev *isp_dev = dev;
    ra_intstat = readl((isp_dev->base) +ISP_GLOBAL_BASE + ISP_INT_STATUS);

    if (ra_intstat & MI_IRQ_BIT) {
        writel(MI_IRQ_BIT, (isp_dev->base) + ISP_GLOBAL_BASE + ISP_INT_STATUS);
        return vvcam_isp_mi_irq_process(isp_dev);
    }
    if (ra_intstat & ISP_IRQ_BIT) {
        writel(ISP_IRQ_BIT, (isp_dev->base) + ISP_GLOBAL_BASE + ISP_INT_STATUS);
        return vvcam_isp_irq_process(isp_dev);
    }

    return IRQ_NONE;
}
#else
static irqreturn_t vvcam_isp_irq_handler(int irq, void *isp_dev)
{
    return vvcam_isp_irq_process(isp_dev);
}

static irqreturn_t vvcam_isp_mi_irq_handler(int irq, void *isp_dev)
{
    return vvcam_isp_mi_irq_process(isp_dev);
}

static irqreturn_t vvcam_isp_fe_irq_handler(int irq, void *isp_dev)
{
    return vvcam_isp_fe_irq_process(isp_dev);
}

static irqreturn_t vvcam_isp_fusa_irq_handler(int irq, void *isp_dev)
{
    return vvcam_isp_fusa_irq_process(isp_dev);
}
#endif

static int vvcam_isp_open(struct inode *inode, struct file *file)
{
    struct miscdevice *pmisc_dev = file->private_data;
    struct vvcam_isp_dev *isp_dev;
    struct vvcam_isp_fh *isp_fh;

    pr_info("vvcam_isp_open ENTRY\n");
    isp_dev = container_of(pmisc_dev, struct vvcam_isp_dev, miscdev);
    if (!isp_dev)
        return -ENOMEM;

    isp_fh = kzalloc(sizeof(struct vvcam_isp_fh), GFP_KERNEL);
    if (!isp_fh)
        return -ENOMEM;

    isp_fh->isp_dev = isp_dev;
    vvcam_event_fh_init( &isp_dev->event_dev, &isp_fh->event_fh);
    file->private_data = isp_fh;

    mutex_lock(&isp_dev->mlock);
    isp_dev->refcnt++;
    pm_runtime_get_sync(isp_dev->dev);
    mutex_unlock(&isp_dev->mlock);

    pr_info("vvcam_isp_open success\n");
    return 0;
}

static int vvcam_isp_release(struct inode *inode, struct file *file)
{
    struct vvcam_isp_dev *isp_dev;
    struct vvcam_isp_fh *isp_fh;

    isp_fh = file->private_data;
    isp_dev = isp_fh->isp_dev;

    vvcam_event_unsubscribe_all(&isp_fh->event_fh);

    vvcam_event_fh_destroy(&isp_dev->event_dev, &isp_fh->event_fh);

    mutex_lock(&isp_dev->mlock);
    isp_dev->refcnt--;
    pm_runtime_put(isp_dev->dev);
    mutex_unlock(&isp_dev->mlock);

    kfree(isp_fh);

    return 0;
}

static long vvcam_isp_ioctl(struct file *file,
                        unsigned int cmd, unsigned long arg)
{
    struct vvcam_isp_dev *isp_dev;
    struct vvcam_isp_fh *isp_fh;
    uint32_t reset;
    vvcam_isp_reg_t isp_reg;
    vvcam_subscription_t sub;
    vvcam_event_t event;
    int ret = 0;

    isp_fh = file->private_data;
    isp_dev = isp_fh->isp_dev;

    mutex_lock(&isp_dev->mlock);

    switch(cmd) {
    case VVCAM_ISP_RESET:
        ret = copy_from_user(&reset, (void __user *)arg, sizeof(reset));
        if (ret)
            break;
#ifdef DOLPHIN
        ret = 0;
#else
        ret = vvcam_isp_reset(isp_dev, reset);
#endif
        break;
    case VVCAM_ISP_READ_REG:
        ret = copy_from_user(&isp_reg, (void __user *)arg, sizeof(isp_reg));
        if (ret)
            break;
        ret = vvcam_isp_read_reg(isp_dev, &isp_reg);
        if (ret)
            break;
        ret = copy_to_user((void __user *)arg, &isp_reg, sizeof(isp_reg));
        break;
    case VVCAM_ISP_WRITE_REG:
        ret = copy_from_user(&isp_reg, (void __user *)arg, sizeof(isp_reg));
        if (ret)
            break;
        ret = vvcam_isp_write_reg(isp_dev, isp_reg);
        break;
    case VVCAM_ISP_SUBSCRIBE_EVENT:
        ret = copy_from_user(&sub, (void __user *)arg, sizeof(sub));
        if (ret)
            break;
        ret = vvcam_event_subscribe(&isp_fh->event_fh,
                    &sub, VVCAM_ISP_EVENT_ELEMS);
        break;
    case VVCAM_ISP_UNSUBSCRIBE_EVENT:
        ret = copy_from_user(&sub, (void __user *)arg, sizeof(sub));
        if (ret)
            break;
        ret = vvcam_event_unsubscribe(&isp_fh->event_fh, &sub);
        break;
    case VVCAM_ISP_DQEVENT:
        ret = vvcam_event_dequeue(&isp_fh->event_fh, &event);
        if (ret)
            break;
        ret = copy_to_user((void __user *)arg, &event, sizeof(event));
        break;
    default:
        ret = -EINVAL;
        break;
    }

    mutex_unlock(&isp_dev->mlock);

    return ret;
}

static unsigned int vvcam_isp_poll(struct file *file, poll_table *wait)
{
    struct vvcam_isp_fh *isp_fh;

    isp_fh = file->private_data;

    return vvcam_event_poll(file, &isp_fh->event_fh, wait);
}

int vvcam_isp_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct vvcam_isp_dev *isp_dev;
    struct vvcam_isp_fh *isp_fh;

    isp_fh = file->private_data;
    isp_dev = isp_fh->isp_dev;

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    pr_info("vvcam_isp_mmap: paddr %lX \n",(unsigned long)isp_dev->paddr);
    return vm_iomap_memory(vma, isp_dev->paddr, isp_dev->regs_size);
}

static struct file_operations vvcam_isp_fops = {
    .owner          = THIS_MODULE,
    .open           = vvcam_isp_open,
    .release        = vvcam_isp_release,
    .unlocked_ioctl = vvcam_isp_ioctl,
    .poll           = vvcam_isp_poll,
    .mmap           = vvcam_isp_mmap,

};

static int vvcam_isp_parse_params(struct vvcam_isp_dev *isp_dev,
                        struct platform_device *pdev)
{
    struct resource *res;
#ifdef DOLPHIN
    struct gpio_desc *reset_gpio[MAX_SENSOR];
    struct gpio_desc *enable_gpio[MAX_SENSOR];
    int i = 0;
#endif

    res =  platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "can't fetch device resource info\n");
        return -EIO;
    }
    isp_dev->paddr = res->start;
    isp_dev->regs_size = resource_size(res);
#ifdef DOLPHIN
    isp_dev->base = devm_ioremap(&pdev->dev, res->start,  resource_size(res));
#else
    isp_dev->base = devm_ioremap_resource(&pdev->dev, res);
#endif
    if (IS_ERR(isp_dev->base)) {
        dev_err(&pdev->dev, "can't remap device resource info\n");
        return PTR_ERR(isp_dev->base);
    }

    isp_dev->isp_irq = platform_get_irq(pdev, 0);
    if (isp_dev->isp_irq < 0) {
        dev_err(&pdev->dev, "can't get irq resource\n");
        return -ENXIO;
    }
#ifdef DOLPHIN
    for (i = 0; i < MAX_SENSOR; i++) {
        reset_gpio[i] = devm_gpiod_get_index(&pdev->dev, "reset", i, GPIOD_OUT_LOW);
        if (IS_ERR(reset_gpio[i]))
            dev_err(&pdev->dev, "failed to fetch reset pin of sensor device %d!\n", i);
        else
            gpiod_set_value_cansleep(reset_gpio[i], 1);

        enable_gpio[i] = devm_gpiod_get_index(&pdev->dev, "enable", i, GPIOD_OUT_LOW);
        if (IS_ERR(enable_gpio[i]))
            dev_err(&pdev->dev, "failed to fetch enable pin of sensor device %d!\n", i);
        else
            gpiod_set_value_cansleep(enable_gpio[i], 1);
    }
#else
    isp_dev->mi_irq = platform_get_irq(pdev, 1);
    if (isp_dev->mi_irq < 0) {
        dev_err(&pdev->dev, "can't get mi irq resource\n");
        return -ENXIO;
    }

    isp_dev->fe_irq = platform_get_irq(pdev, 2);
    if (isp_dev->fe_irq < 0) {
        dev_err(&pdev->dev, "can't get fe irq resource\n");
        return -ENXIO;
    }

    isp_dev->fusa_irq = platform_get_irq(pdev, 3);
    if (isp_dev->fusa_irq < 0) {
        dev_err(&pdev->dev, "can't get fusa irq resource\n");
        return -ENXIO;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) {
        dev_err(&pdev->dev, "can't fetch device reset reg\n");
        return -EIO;
    }
    isp_dev->reset = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (IS_ERR(isp_dev->reset)) {
        dev_err(&pdev->dev, "can't remap device reset reg\n");
        return PTR_ERR(isp_dev->reset);
    }
#endif

    return 0;
}

static int vvcam_isp_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct vvcam_isp_dev *isp_dev;
    char *ispdev_name;

    isp_dev = devm_kzalloc(&pdev->dev,
                sizeof(struct vvcam_isp_dev), GFP_KERNEL);
    if (!isp_dev)
        return -ENOMEM;

    ret = vvcam_isp_parse_params(isp_dev, pdev);
    if (ret) {
        dev_err(&pdev->dev, "failed to parse params\n");
        return -EINVAL;
    }

    mutex_init(&isp_dev->mlock);
    spin_lock_init(&isp_dev->stat_lock);
    vvcam_event_dev_init(&isp_dev->event_dev);
    platform_set_drvdata(pdev, isp_dev);

    isp_dev->dev = &pdev->dev;
#ifdef DOLPHIN
    fwnode_property_read_u32(of_fwnode_handle(pdev->dev.of_node),
            "id", &isp_dev->id);
#else
    isp_dev->id  = pdev->id;
#endif

    ispdev_name = devm_kzalloc(&pdev->dev, 16, GFP_KERNEL);
    if (!ispdev_name)
        return -ENOMEM;
#ifdef DOLPHIN
    snprintf(ispdev_name, 16, "%s.%d", VVCAM_ISP_NAME, isp_dev->id);
#else
    snprintf(ispdev_name, 16, "%s.%d", VVCAM_ISP_NAME, pdev->id);
#endif

    isp_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    isp_dev->miscdev.name  = ispdev_name;
    isp_dev->miscdev.fops  = &vvcam_isp_fops;

    ret = misc_register(&isp_dev->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "failed to register device\n");
        return -EINVAL;
    }

    ret = vvcam_isp_procfs_register(isp_dev, &isp_dev->pde);
    if (ret) {
        dev_err(&pdev->dev, "isp register procfs failed.\n");
        goto error_register_procfs;
    }

    tasklet_init(&isp_dev->stat_tasklet,
        vvcam_isp_irq_stat_tasklet, (unsigned long)isp_dev);

#ifdef DOLPHIN
    ret = devm_request_irq(&pdev->dev, isp_dev->isp_irq, vvcam_isp_generic_irq_handler,
        IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), isp_dev);
    if (ret) {
        dev_err(&pdev->dev, "can't request isp irq\n");
        goto error_request_isp_irq;
    }
#else

    ret = devm_request_irq(&pdev->dev, isp_dev->isp_irq, vvcam_isp_irq_handler,
                IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), isp_dev);
    if (ret) {
        dev_err(&pdev->dev, "can't request isp irq\n");
        goto error_request_isp_irq;
    }

    ret = devm_request_irq(&pdev->dev, isp_dev->mi_irq, vvcam_isp_mi_irq_handler,
                IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), isp_dev);
    if (ret) {
        dev_err(&pdev->dev, "can't request mi irq\n");
        goto error_request_mi_irq;
    }

    ret = devm_request_irq(&pdev->dev, isp_dev->fe_irq, vvcam_isp_fe_irq_handler,
                IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), isp_dev);
    if (ret) {
        dev_err(&pdev->dev, "can't request fe irq\n");
        goto error_request_fe_irq;
    }

    ret = devm_request_irq(&pdev->dev, isp_dev->fusa_irq, vvcam_isp_fusa_irq_handler,
                IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), isp_dev);
    if (ret) {
        dev_err(&pdev->dev, "can't request fusa irq\n");
        goto error_request_fusa_irq;
    }
#endif

    dev_info(&pdev->dev, "vvcam isp driver probe success\n");

    return 0;

#ifndef DOLPHIN
error_request_fusa_irq:
    devm_free_irq(&pdev->dev, isp_dev->fe_irq, isp_dev);
error_request_fe_irq:
    devm_free_irq(&pdev->dev, isp_dev->mi_irq, isp_dev);
error_request_mi_irq:
#endif
    devm_free_irq(&pdev->dev, isp_dev->isp_irq, isp_dev);
error_request_isp_irq:
    vvcam_isp_procfs_unregister(isp_dev->pde);
error_register_procfs:
    misc_deregister(&isp_dev->miscdev);

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static void vvcam_isp_remove(struct platform_device *pdev)
#else
static int vvcam_isp_remove(struct platform_device *pdev)
#endif
{
    struct vvcam_isp_dev *isp_dev;

    isp_dev = platform_get_drvdata(pdev);

    vvcam_isp_procfs_unregister(isp_dev->pde);

    misc_deregister(&isp_dev->miscdev);
    devm_free_irq(&pdev->dev, isp_dev->isp_irq, isp_dev);
#ifndef DOLPHIN
    devm_free_irq(&pdev->dev, isp_dev->mi_irq, isp_dev);
    devm_free_irq(&pdev->dev, isp_dev->fe_irq, isp_dev);
    devm_free_irq(&pdev->dev, isp_dev->fusa_irq, isp_dev);
#endif
    tasklet_kill(&isp_dev->stat_tasklet);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
	return 0;
#endif

}

static int vvcam_isp_system_suspend(struct device *dev)
{
    int ret = 0;
    ret = pm_runtime_force_suspend(dev);
    if (ret) {
        dev_err(dev, "force suspend %s failed\n", dev_name(dev));
        return ret;
    }
    return ret;
}

static int vvcam_isp_system_resume(struct device *dev)
{
    int ret = 0;
    ret = pm_runtime_force_resume(dev);
    if (ret) {
        dev_err(dev, "force resume %s failed\n", dev_name(dev));
        return ret;
    }
    return ret;
}

static int vvcam_isp_runtime_suspend(struct device *dev)
{
    return 0;
}

static int vvcam_isp_runtime_resume(struct device *dev)
{
    return 0;
}

static const struct dev_pm_ops vvcam_isp_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(vvcam_isp_system_suspend, vvcam_isp_system_resume)
    SET_RUNTIME_PM_OPS(vvcam_isp_runtime_suspend, vvcam_isp_runtime_resume, NULL)
};

static const struct of_device_id vvcam_isp_of_match[] = {
    //TODO review string
#ifdef DOLPHIN
    {.compatible = "verislicon,isp",},
#else
    {.compatible = "verisilicon,isp",},
#endif
    { /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, vvcam_isp_of_match);

static struct platform_driver vvcam_isp_driver = {
    .probe  = vvcam_isp_probe,
    .remove = vvcam_isp_remove,
    .driver = {
        .name           = VVCAM_ISP_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = vvcam_isp_of_match,
        .pm             = &vvcam_isp_pm_ops,
    }
};

static int __init vvcam_isp_init_module(void)
{
    int ret;
    ret = platform_driver_register(&vvcam_isp_driver);
    if (ret) {
        pr_err(KERN_ERR "Failed to register isp driver\n");
        return ret;
    }

#ifdef VVCAM_PLATFORM_REGISTER
    ret = vvcam_isp_platform_device_register();
    if (ret) {
        platform_driver_unregister(&vvcam_isp_driver);
        pr_err(KERN_ERR "Failed to register vvcam isp platform devices\n");
        return ret;
    }
#endif

    return ret;
}

static void __exit vvcam_isp_exit_module(void)
{
    platform_driver_unregister(&vvcam_isp_driver);
#ifdef VVCAM_PLATFORM_REGISTER
    vvcam_isp_platform_device_unregister();
#endif
}

module_init(vvcam_isp_init_module);
module_exit(vvcam_isp_exit_module);

MODULE_DESCRIPTION("Verisilicon isp driver");
MODULE_AUTHOR("Verisilicon ISP SW Team");
MODULE_LICENSE("GPL");
