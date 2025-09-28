// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_VIP_ISR_H
#define HRX_VIP_ISR_H

#include "hrx-drv.h"

int vip_isr_handler(struct syna_hrx_v4l2_dev *hrx_dev, u32 intr_id);
int vip_create_isr_task(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_stop_isr_task(struct syna_hrx_v4l2_dev *hrx_dev);
int vip_create_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_stop_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev);
#endif //HRX_VIP_ISR_H
