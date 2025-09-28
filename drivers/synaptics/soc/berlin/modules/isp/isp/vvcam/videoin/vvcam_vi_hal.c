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
#include "vvcam_vi_driver.h"
#include "vvcam_vi_hal.h"

extern void vvcam_vi_proc_stat(unsigned long pde,
                    const uint32_t *irq_mis, const int len);

#define  MCM0_MI_MIS2  0x1224
#define  MCM1_MI_MIS2  0x1624
#define  MCM2_MI_MIS2  0x1a24
#define  MCM3_MI_MIS2  0x1e24

#define  MCM0_MI_ICR2  0x122c
#define  MCM1_MI_ICR2  0x162c
#define  MCM2_MI_ICR2  0x1a2c
#define  MCM3_MI_ICR2  0x1e2c

#define  CH0_XS_FRAME_END_MASK  (1U << 31)
#define  CH0_VS_FRAME_END_MASK  (1U << 30)
#define  CH0_S_FRAME_END_MASK   (1U << 29)
#define  CH0_L_FRAME_END_MASK   (1U << 28)
#define  CH1_XS_FRAME_END_MASK  (1U << 27)
#define  CH1_VS_FRAME_END_MASK  (1U << 26)
#define  CH1_S_FRAME_END_MASK   (1U << 25)
#define  CH1_L_FRAME_END_MASK   (1U << 24)
#define  CH2_XS_FRAME_END_MASK  (1U << 23)
#define  CH2_VS_FRAME_END_MASK  (1U << 22)
#define  CH2_S_FRAME_END_MASK   (1U << 21)
#define  CH2_L_FRAME_END_MASK   (1U << 20)
#define  CH3_XS_FRAME_END_MASK  (1U << 19)
#define  CH3_VS_FRAME_END_MASK  (1U << 18)
#define  CH3_S_FRAME_END_MASK   (1U << 17)
#define  CH3_L_FRAME_END_MASK   (1U << 16)

void vvcam_vi_irq_stat_tasklet(unsigned long vi_dev)
{
    // struct vvcam_vi_dev *vi_dev = (struct vvcam_vi_dev *)vi_dev;
    // uint32_t irq_mis[VVCAM_EID_MAX];
    //     printk("enter %s\n", __func__);

    // memset(irq_mis, 0, sizeof(irq_mis));

    // memcpy(irq_mis, vi_dev->irq_mis, sizeof(irq_mis));
    // memset(vi_dev->irq_mis, 0, sizeof(vi_dev->irq_mis));

    // vvcam_vi_proc_stat(vi_dev->pde, irq_mis, VVCAM_EID_MAX);
    printk("exit %s\n", __func__);
}

static void vvcam_vi_hal_write_reg(void __iomem *base,
			uint32_t addr, uint32_t value)
{
    writel(value, base + addr);
}

static int vvcam_vi_hal_read_reg(void __iomem *base, uint32_t addr)
{
    return readl(base + addr);
}

int vvcam_vi_reset(struct vvcam_vi_dev *vi_dev, uint32_t reset)
{
    // uint32_t reset_reg = 0;

    // reset_reg = vvcam_vi_hal_read_reg(vi_dev->reset, 0);

    // if (reset) {
    //     /* reset */
    //     reset_reg &= ~TPG_ISP_RST_MASK;
    // } else {
    //     /* unreset */
    //     reset_reg |= TPG_ISP_RST_MASK;
    // }

    // vvcam_vi_hal_write_reg(vi_dev->reset, 0, reset_reg);

    return 0;
}

int vvcam_vi_write_reg(struct vvcam_vi_dev *vi_dev, vvcam_vi_reg_t vi_reg)
{
    vvcam_vi_hal_write_reg(vi_dev->base, vi_reg.addr, vi_reg.value);
    return 0;
}

int vvcam_vi_read_reg(struct vvcam_vi_dev *vi_dev, vvcam_vi_reg_t *vi_reg)
{
    uint32_t reg_value = 0;

    reg_value = vvcam_vi_hal_read_reg(vi_dev->base, vi_reg->addr);

    vi_reg->value = reg_value;

    return 0;
}


irqreturn_t vvcam_vi_irq_process(struct vvcam_vi_dev *vi_dev)
{
    uint32_t mcm_mis[4] = {0};
    uint64_t timestamp;
    uint32_t i = 0;
    vvcam_event_t event;
    bool irq = false;

    mcm_mis[0] = vvcam_vi_hal_read_reg(vi_dev->base, MCM0_MI_MIS2);
    if (mcm_mis[0]) {
        vvcam_vi_hal_write_reg(vi_dev->base, MCM0_MI_ICR2, mcm_mis[0]);
        irq = true;
    }
    mcm_mis[1] = vvcam_vi_hal_read_reg(vi_dev->base, MCM1_MI_MIS2);
    if ( mcm_mis[1] ) {
        vvcam_vi_hal_write_reg(vi_dev->base, MCM1_MI_ICR2, mcm_mis[1]);
        irq = true;
    }
    mcm_mis[2] = vvcam_vi_hal_read_reg(vi_dev->base, MCM2_MI_MIS2);
    if ( mcm_mis[2] ) {
        vvcam_vi_hal_write_reg(vi_dev->base, MCM2_MI_ICR2, mcm_mis[2]);
        irq = true;
    }

    mcm_mis[3] = vvcam_vi_hal_read_reg(vi_dev->base, MCM3_MI_MIS2);
    if (mcm_mis[3]) {
        vvcam_vi_hal_write_reg(vi_dev->base, MCM3_MI_ICR2, mcm_mis[3]);
        irq = true;
    }

    timestamp = ktime_get_ns();

    for (i = 0; i < VVCAM_EID_VIDEO_IN_MCM_MAX; i++) {
        if (mcm_mis[i /4]) {
            event.type = VVCAM_VIDEO_IN_EVENT_IRQ_TYPE;
            event.timestamp = timestamp;

            if (mcm_mis[i /4] & (CH0_L_FRAME_END_MASK | CH0_S_FRAME_END_MASK |
                                 CH0_VS_FRAME_END_MASK | CH0_XS_FRAME_END_MASK)) {
                event.id   = i;
                event.irqevent.irq_value = (mcm_mis[i /4] & (CH0_L_FRAME_END_MASK |
                                            CH0_S_FRAME_END_MASK |
                                            CH0_VS_FRAME_END_MASK |
                                            CH0_XS_FRAME_END_MASK));
                vvcam_event_queue(&vi_dev->event_dev, &event);
            }
            if (mcm_mis[i /4] &(CH1_L_FRAME_END_MASK | CH1_S_FRAME_END_MASK |
                                 CH1_VS_FRAME_END_MASK | CH1_XS_FRAME_END_MASK)) {
                event.id   = i + 1;
                event.irqevent.irq_value = (mcm_mis[i /4] & (CH1_L_FRAME_END_MASK |
                                            CH1_S_FRAME_END_MASK |
                                            CH1_VS_FRAME_END_MASK |
                                            CH1_XS_FRAME_END_MASK));
                vvcam_event_queue(&vi_dev->event_dev, &event);
            }
            if (mcm_mis[i /4] & (CH2_L_FRAME_END_MASK | CH2_S_FRAME_END_MASK |
                                 CH2_VS_FRAME_END_MASK | CH2_XS_FRAME_END_MASK)) {
                event.id   = i + 2;
                event.irqevent.irq_value = (mcm_mis[i /4] & (CH2_L_FRAME_END_MASK |
                                            CH2_S_FRAME_END_MASK |
                                            CH2_VS_FRAME_END_MASK |
                                            CH2_XS_FRAME_END_MASK));
                vvcam_event_queue(&vi_dev->event_dev, &event);
            }
            if (mcm_mis[i /4] & (CH3_L_FRAME_END_MASK | CH3_S_FRAME_END_MASK |
                                 CH3_VS_FRAME_END_MASK | CH3_XS_FRAME_END_MASK)) {
                event.id   = i + 3;
                event.irqevent.irq_value = (mcm_mis[i /4] & (CH3_L_FRAME_END_MASK |
                                            CH3_S_FRAME_END_MASK |
                                            CH3_VS_FRAME_END_MASK |
                                            CH3_XS_FRAME_END_MASK));
                vvcam_event_queue(&vi_dev->event_dev, &event);
            }

        }

    }
    if (irq) {
        // vi_dev->irq_mis[VVCAM_EID_mcm_mis] = mcm0_mis;
        // tasklet_schedule(&vi_dev->stat_tasklet);
        return IRQ_HANDLED;
    }
    return IRQ_HANDLED;
}


