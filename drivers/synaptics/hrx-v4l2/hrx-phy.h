// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_PHY_H
#define HRX_PHY_H

#include "hrx-drv.h"

int hrx_phy_init(struct syna_hrx_v4l2_dev *hrx_dev, bool data_rate_6g, bool full_config);
void hrx_phy_re_init(struct syna_hrx_v4l2_dev *hrx_dev);
bool has_clock(struct syna_hrx_v4l2_dev *hrx_dev);
void hrx_handle_clock_change(struct syna_hrx_v4l2_dev *hrx_dev);
bool is_scrambled(struct syna_hrx_v4l2_dev *hrx_dev);
unsigned int is_frl(struct syna_hrx_v4l2_dev *hrx_dev);
void hrx_phy_tmds_clock_ratio(struct syna_hrx_v4l2_dev *hrx_dev, bool enable);
u32 hrx_get_tmds_clk(struct syna_hrx_v4l2_dev *hrx_dev);
bool has_audio_clock(struct syna_hrx_v4l2_dev *hrx_dev);
#endif
