// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_EDID_H
#define HRX_EDID_H

#include "hrx-drv.h"

void syna_hrx_read_edid(struct syna_hrx_v4l2_dev *hrx_dev, u8 *edid);
void syna_hrx_write_edid(struct syna_hrx_v4l2_dev *hrx_dev, u8 *edid);
void syna_hrx_print_edid(u8 *edid);
int syna_hrx_validate_edid(u8 *edid);
void syna_hrx_write_default_edid(struct syna_hrx_v4l2_dev *hrx_dev);

#endif //HRX_EDID_H
