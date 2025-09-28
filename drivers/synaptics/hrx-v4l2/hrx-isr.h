// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_ISR_H
#define HRX_ISR_H

#include "hrx-drv.h"

int hrx_isr_handler(struct syna_hrx_v4l2_dev *hrx_dev);
int hrx_create_isr_task(struct syna_hrx_v4l2_dev *hrx_dev);
void hrx_stop_isr_task(struct syna_hrx_v4l2_dev *hrx_dev);
#endif //HRX_ISR_H
