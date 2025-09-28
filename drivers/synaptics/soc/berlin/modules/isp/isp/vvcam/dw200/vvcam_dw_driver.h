/* SPDX-License-Identifier: MIT OR GPL-2.0-or-later */
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

#ifndef __VVCAM_DW_DRIVER_H__
#define __VVCAM_DW_DRIVER_H__

#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include "vvcam_dw.h"
#include "vvcam_event.h"

#define VVCAM_DW_EVENT_ELEMS 10

#define VVCAM_DW_NAME "vvcam-dw200"

struct vvcam_dwe_core {
    phys_addr_t paddr;
    uint32_t regs_size;
    void __iomem *base;
    int32_t irq;
};

struct vvcam_vse_core {
    phys_addr_t paddr;
    uint32_t regs_size;
    void __iomem *base;
    int32_t irq;
};

struct vvcam_dw_dev {
    struct vvcam_dwe_core dwe;
    struct vvcam_vse_core vse;
    int  id;
    int32_t fe_irq;
    void __iomem *reset;
    struct miscdevice miscdev;
    struct device *dev;
    struct vvcam_event_dev event_dev;
    struct mutex mlock;
    uint32_t refcnt;
    
};

struct vvcam_dw_fh {
    struct vvcam_dw_dev *dw_dev;
    struct vvcam_event_fh event_fh;
};

int vvcam_dwe_reset(struct vvcam_dw_dev *dwe_dev);
int vvcam_dwe_write_reg(struct vvcam_dw_dev *dwe_dev, vvcam_dw_reg_t dwe_reg);
int vvcam_dwe_read_reg(struct vvcam_dw_dev *dwe_dev, vvcam_dw_reg_t *dwe_reg);
int vvcam_vse_reset(struct vvcam_dw_dev *dwe_dev);
int vvcam_vse_write_reg(struct vvcam_dw_dev *dwe_dev, vvcam_dw_reg_t vse_reg);
int vvcam_vse_read_reg(struct vvcam_dw_dev *dwe_dev, vvcam_dw_reg_t *vse_reg);
int vvcam_dw_get_reg_mem(struct vvcam_dw_dev *dwe_dev, vvcam_dw_reg_mem_t *dw_reg_mem);
irqreturn_t vvcam_dwe_irq_process(struct vvcam_dw_dev *isp_dev);
irqreturn_t vvcam_vse_irq_process(struct vvcam_dw_dev *isp_dev);
irqreturn_t vvcam_dw_fe_irq_process(struct vvcam_dw_dev *isp_dev);

#endif
