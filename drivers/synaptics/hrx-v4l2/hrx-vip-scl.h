// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_VIP_SCL_H
#define HRX_VIP_SCL_H

#include "hrx-drv.h"

int vip_scl_reset(struct syna_hrx_v4l2_dev *hrx_dev);
int vip_scl_config(struct syna_hrx_v4l2_dev *hrx_dev);
int vip_prog_scl(struct syna_hrx_v4l2_dev *hrx_dev, int *width, int *height, bool update_required);

#endif //HRX_VIP_SCL_H
