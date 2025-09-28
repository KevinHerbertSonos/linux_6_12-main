// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_VIDEO_H
#define HRX_VIDEO_H

#include "hrx-drv.h"

#define SYNA_HRX_V4L2_NAME "syna-hrx"
#define SYNA_HRX_STR "synaptics-hrx"
#define SYNA_HRX_MIN_BUFFERS 8
#define SYNA_HRX_MAX_PLANES 8

int syna_hrx_video_init(struct syna_hrx_v4l2_dev *hrx_dev);
int syna_hrx_video_finalize(struct syna_hrx_v4l2_dev *hrx_dev);
struct vb2_buffer *syna_hrx_get_free_buf(struct syna_hrx_v4l2_dev *hrx_dev);
void syna_hrx_buf_processed(struct syna_hrx_v4l2_dev *hrx_dev, struct vb2_buffer *buf);
void syna_hrx_buf_unused(struct syna_hrx_v4l2_dev *hrx_dev, struct vb2_buffer *buf);

#endif //HRX_VIDEO_H
