// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_AUDIO_H
#define HRX_AUDIO_H

#include "hrx-drv.h"

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/core.h>
#include "avio_common.h"

int aip_call_start_cmd(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_call_stop_cmd(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_resume_cmd(struct syna_hrx_v4l2_dev *hrx_dev);


#endif //HRX_AUDIO_H
