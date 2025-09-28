// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SYNA_VPU_DEC_DRV_H__
#define __SYNA_VPU_DEC_DRV_H__

#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>

#define SYNA_VDEC_MAX_AUX_BUFS_NUM	(32U)
#define SYNA_VDEC_MAX_AUX_BUFS_SHIFT	(5U)

int syna_vdpu_v4g_init(struct syna_vpu_dev *vpu);
int syna_vdpu_v4g_finalize(struct syna_vpu_dev *vpu);
#endif
