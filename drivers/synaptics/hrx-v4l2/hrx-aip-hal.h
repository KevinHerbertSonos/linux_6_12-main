// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef _HRX_AIP_HAL_H_
#define _HRX_AIP_HAL_H_

#include "hrx-drv.h"

void aip_dhub_dataclear(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_dhub_intr_enable(struct syna_hrx_v4l2_dev *hrx_dev, int enable);

#endif
