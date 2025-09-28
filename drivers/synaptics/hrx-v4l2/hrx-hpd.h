// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_HDP_H
#define HRX_HDP_H

#include "hrx-drv.h"

inline void syna_hrx_set_hpd(struct syna_hrx_v4l2_dev *hrx_dev, int value)
{
	gpiod_set_value_cansleep(hrx_dev->gpiod_hrxhpd, value);
}

inline int syna_hrx_get_hrx5v(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return gpiod_get_value_cansleep(hrx_dev->gpiod_hrx5v);
}

#endif //HRX_HDP_H
