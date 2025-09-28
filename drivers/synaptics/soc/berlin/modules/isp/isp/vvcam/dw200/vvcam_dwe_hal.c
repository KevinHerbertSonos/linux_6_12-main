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

#include <linux/io.h>
#include <linux/spinlock.h>
#include "vvcam_dw_driver.h"
#include "vvcam_dwe_hal.h"

static void vvcam_dwe_hal_write_reg(void __iomem *base,
			uint32_t addr, uint32_t value)
{
	writel(value, base + addr);
}

static int vvcam_dwe_hal_read_reg(void __iomem *base, uint32_t addr)
{
	return readl(base + addr);
}

int vvcam_dwe_write_reg(struct vvcam_dw_dev *dw_dev, vvcam_dw_reg_t dwe_reg)
{
	vvcam_dwe_hal_write_reg(dw_dev->dwe.base,
			dwe_reg.addr - (dw_dev->dwe.paddr -  dw_dev->vse.paddr),
			dwe_reg.value);
	return 0;
}

int vvcam_dwe_read_reg(struct vvcam_dw_dev *dw_dev, vvcam_dw_reg_t *dwe_reg)
{
	uint32_t reg_value = 0;

	reg_value = vvcam_dwe_hal_read_reg(dw_dev->dwe.base,
			dwe_reg->addr - (dw_dev->dwe.paddr - dw_dev->vse.paddr));
	dwe_reg->value = reg_value;

	return 0;
}

int vvcam_dwe_reset(struct vvcam_dw_dev *dw_dev)
{
	vvcam_dwe_hal_write_reg(dw_dev->reset, 0, 0);
	vvcam_dwe_hal_write_reg(dw_dev->reset, 0, 1);

	vvcam_dwe_hal_write_reg(dw_dev->dwe.base, DWE_CTRL_REG, 0x0c);

	return 0;
}

irqreturn_t vvcam_dwe_irq_process(struct vvcam_dw_dev *dw_dev)
{
	uint32_t dwe_irq = 0;
	uint64_t timestamp = 0;

	vvcam_event_t event;

	if (!dw_dev->refcnt)
		return IRQ_NONE;

	dwe_irq = vvcam_dwe_hal_read_reg(dw_dev->dwe.base, DWE_INT_STATUS);
	if (dwe_irq & DWE_INT_STATUS_MASK) {
		vvcam_dwe_hal_write_reg(dw_dev->dwe.base,
			DWE_INT_STATUS, DWE_INT_CLEAR_STATUS_MASK);
	}

	timestamp = ktime_get_ns();

	if (dwe_irq & DWE_INT_STATUS_MASK) {
		event.type = VVCAM_EVENT_IRQ_TYPE;
		event.id   = VVCAM_EID_DWE_INT;
		event.timestamp = timestamp;
		event.irqevent.irq_value = dwe_irq;
		vvcam_event_queue(&dw_dev->event_dev, &event);
	}

	return IRQ_HANDLED;
}

irqreturn_t vvcam_dw_fe_irq_process(struct vvcam_dw_dev *dw_dev)
{
	uint32_t fe_mis = 0;
	uint64_t timestamp = 0;

	vvcam_event_t event;

	if (!dw_dev->refcnt)
		return IRQ_NONE;

	fe_mis = vvcam_dwe_hal_read_reg(dw_dev->dwe.base, DW_FE_MIS);
	if (fe_mis) {
		vvcam_dwe_hal_write_reg(dw_dev->dwe.base, DW_FE_ICR, fe_mis);
		if (fe_mis & DW_FE_INT_CFG_END) {
			vvcam_dwe_hal_write_reg(dw_dev->dwe.base, DW_FE_CTRL, 0x00000000);
		}
	} else {
		return IRQ_NONE;
	}

	timestamp = ktime_get_ns();

	if (fe_mis) {
		event.type = VVCAM_EVENT_IRQ_TYPE;
		event.id   = VVCAM_EID_DW_FE_MIS;
		event.timestamp = timestamp;
		event.irqevent.irq_value = fe_mis;
		vvcam_event_queue(&dw_dev->event_dev, &event);
	}

	return IRQ_HANDLED;
}
