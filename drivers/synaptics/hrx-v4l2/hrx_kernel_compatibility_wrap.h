// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __HRX_KERNEL_COMPATIBILITY_WRAP_H__
#define __HRX_KERNEL_COMPATIBILITY_WRAP_H__

#include <linux/version.h>
#include <linux/compiler.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,8,0))

#define set_v4l2_q_min_queued_buffers(vq, num) \
        (vq->min_queued_buffers) = (num)

#else

#define set_v4l2_q_min_queued_buffers(vq, num) \
        (vq->min_buffers_needed) = (num)
#endif

#endif //__HRX_KERNEL_COMPATIBILITY_WRAP_H__

