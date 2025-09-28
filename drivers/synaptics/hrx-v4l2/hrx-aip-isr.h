// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef _HRX_AIP_ISR_H_
#define _HRX_AIP_ISR_H_

#include "hrx-drv.h"

int aip_isr_enable(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_isr_disable(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_isr_task(void *pParam);
int aip_ioctl_sem_up(void);
int aip_create_isr_task(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_stop_isr_task(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_create_main_task(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_stop_main_task(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_isr_handler(struct syna_hrx_v4l2_dev *hrx_dev, u32 intr_id);

#endif
