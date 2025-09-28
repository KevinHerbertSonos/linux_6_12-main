// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_VIP_H
#define HRX_VIP_H

#include "hrx-drv.h"

void vip_init(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_config(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_reset(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_start(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_stop(struct syna_hrx_v4l2_dev *hrx_dev);
void vip_close(struct syna_hrx_v4l2_dev *hrx_dev);
#endif
