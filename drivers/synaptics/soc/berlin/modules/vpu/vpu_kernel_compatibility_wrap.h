// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VPU_KERNEL_COMPATIBILITY_WRAP_H__
#define __VPU_KERNEL_COMPATIBILITY_WRAP_H__

#include <linux/version.h>
#include <linux/compiler.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0))
#define vpu_create_dev_class(CLS_NAME) class_create(CLS_NAME)
#else
#define vpu_create_dev_class(CLS_NAME) class_create(THIS_MODULE, CLS_NAME)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
#define iosys_map			dma_buf_map
#define iosys_map_set_vaddr		dma_buf_map_set_vaddr
#define iosys_map_clear			dma_buf_map_clear
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0))
#define vm_flags_clear(vma, flags) \
	(vma->vm_flags) &= ~(flags)

#define vm_flags_set(vma, flags) \
	(vma->vm_flags) |= (flags)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,12,0))
typedef void PLAT_RET_TYPE;

#define PLAT_RET_VALUE(value) \
	return
#else
typedef int PLAT_RET_TYPE;

#define PLAT_RET_VALUE(value) \
	return (value)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,8,0))
#define vb2_get_num_buffers(vq)		(vq->num_buffers)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,8,0))

#define ignore_cap_streaming(ctx, trigger) \
	do {	\
		ctx->ignore_cap_streaming = trigger; \
		v4l2_m2m_set_dst_buffered(ctx, trigger); \
	} while (0)

#define set_v4l2_q_min_queued_buffers(vq, num) \
	(vq->min_queued_buffers) = (num)

#else

#define ignore_cap_streaming(ctx, trigger) \
	v4l2_m2m_set_dst_buffered(ctx, trigger)

#define set_v4l2_q_min_queued_buffers(vq, num) \
	(vq->min_buffers_needed) = (num)
#endif

#endif
