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
#include "vvcam_vse_hal.h"

static void vvcam_vse_hal_write_reg(void __iomem *base,
			uint32_t addr, uint32_t value)
{
	writel(value, base + addr);
}

static int vvcam_vse_hal_read_reg(void __iomem *base, uint32_t addr)
{
	return readl(base + addr);
}

int vvcam_vse_write_reg(struct vvcam_dw_dev *dw_dev, vvcam_dw_reg_t vse_reg)
{
	vvcam_vse_hal_write_reg(dw_dev->vse.base, vse_reg.addr, vse_reg.value);
	return 0;
}

int vvcam_vse_read_reg(struct vvcam_dw_dev *dw_dev, vvcam_dw_reg_t *vse_reg)
{
	uint32_t reg_value = 0;

	reg_value = vvcam_vse_hal_read_reg(dw_dev->vse.base, vse_reg->addr);

	vse_reg->value = reg_value;

	return 0;
}

int vvcam_vse_reset(struct vvcam_dw_dev *dw_dev)
{
	vvcam_vse_hal_write_reg(dw_dev->reset, 0, 0);
	vvcam_vse_hal_write_reg(dw_dev->reset, 0, 1);

	vvcam_vse_hal_write_reg(dw_dev->vse.base, VSE_CTRL_REG, 0x100);

	return 0;
}

irqreturn_t vvcam_vse_irq_process(struct vvcam_dw_dev *dw_dev)
{
	uint32_t vse_mi_mis0 = 0;
	uint32_t vse_mi_mis1 = 0;
#ifdef DW200_V42
	uint32_t vse1_mi_mis0 = 0;
	uint32_t vse1_mi_mis1 = 0;
#endif
	uint64_t timestamp = 0;
	vvcam_event_t event;

	if (!dw_dev->refcnt)
		return IRQ_NONE;

	vse_mi_mis0 = vvcam_vse_hal_read_reg(dw_dev->vse.base, VSE_MI_MIS0);
	if (vse_mi_mis0) {
		vvcam_vse_hal_write_reg(dw_dev->vse.base, VSE_MI_ICR0, vse_mi_mis0);
	}

	vse_mi_mis1 = vvcam_vse_hal_read_reg(dw_dev->vse.base, VSE_MI_MIS1);
	if (vse_mi_mis1) {
		vvcam_vse_hal_write_reg(dw_dev->vse.base, VSE_MI_ICR1, vse_mi_mis1);
	}

#ifdef DW200_V42
	vse1_mi_mis0 = vvcam_vse_hal_read_reg(dw_dev->vse.base, VSE1_MI_MIS0);
	if (vse_mi_mis0) {
		vvcam_vse_hal_write_reg(dw_dev->vse.base, VSE1_MI_ICR0, vse1_mi_mis0);
	}

	vse1_mi_mis1 = vvcam_vse_hal_read_reg(dw_dev->vse.base, VSE1_MI_MIS1);
	if (vse_mi_mis1) {
		vvcam_vse_hal_write_reg(dw_dev->vse.base, VSE1_MI_ICR1, vse1_mi_mis1);
	}
#endif

	timestamp = ktime_get_ns();

	if (vse_mi_mis0) {
		event.type = VVCAM_EVENT_IRQ_TYPE;
		event.id   = VVCAM_EID_VSE_MI_MIS0;
		event.timestamp = timestamp;
		event.irqevent.irq_value = vse_mi_mis0;
		vvcam_event_queue(&dw_dev->event_dev, &event);
	}

	if (vse_mi_mis1) {
		event.type = VVCAM_EVENT_IRQ_TYPE;
		event.id   = VVCAM_EID_VSE_MI_MIS1;
		event.timestamp = timestamp;
		event.irqevent.irq_value = vse_mi_mis1;
		vvcam_event_queue(&dw_dev->event_dev, &event);
	}

#ifdef DW200_V42
	if (vse1_mi_mis0) {
		event.type = VVCAM_EVENT_IRQ_TYPE;
		event.id   = VVCAM_EID_VSE1_MI_MIS0;
		event.timestamp = timestamp;
		event.irqevent.irq_value = vse1_mi_mis0;
		vvcam_event_queue(&dw_dev->event_dev, &event);
	}

	if (vse1_mi_mis1) {
		event.type = VVCAM_EVENT_IRQ_TYPE;
		event.id   = VVCAM_EID_VSE1_MI_MIS1;
		event.timestamp = timestamp;
		event.irqevent.irq_value = vse1_mi_mis1;
		vvcam_event_queue(&dw_dev->event_dev, &event);
	}
#endif

	return IRQ_HANDLED;
}
