/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MCM_EVENT_H__
#define __MCM_EVENT_H__

#define MCM_CHN_MAX                 2

#define MCM_DEAMON_EVENT            (V4L2_EVENT_PRIVATE_START + 3000)

#define MCM_IOC_BUF_DONE            _IOWR('M',  BASE_VIDIOC_PRIVATE + 0, struct mcm_buf)
#define MCM_IOC_BUF_ALLOCATE        _IOWR('M',  BASE_VIDIOC_PRIVATE + 1, struct mcm_buf)
#define MCM_IOC_BUF_FREE            _IOW('M',  BASE_VIDIOC_PRIVATE + 2, struct mcm_buf)
#define MCM_IOC_MTR_MCM_WR_UPDATE   _IOWR('M',  BASE_VIDIOC_PRIVATE + 3, struct mtr_mcm_update)
#define MCM_IOC_MTR_ISP_UPDATE      _IOW('M',  BASE_VIDIOC_PRIVATE + 4, struct mtr_mcm_update)
#define MCM_IOC_WRITE_BCM_ARRAY     _IOW('M',  BASE_VIDIOC_PRIVATE + 5, struct mcm_bcm_array)
#define MCM_IOC_BEGIN_CPU_ACCESS    _IOW('M',  BASE_VIDIOC_PRIVATE + 6, struct mtr_mcm_update)
#define MCM_IOC_END_CPU_ACCESS      _IOW('M',  BASE_VIDIOC_PRIVATE + 7, struct mtr_mcm_update)
#define MCM_PAD_MCM_BUF_STATE       _IOWR('M',  BASE_VIDIOC_PRIVATE + 8, struct mcm_buf_state)
#define MCM_PAD_BUF_QUEUE           _IOWR('M',  BASE_VIDIOC_PRIVATE + 9, struct vvcam_pad_buf)
#define MCM_PAD_S_STREAM            _IOWR('M',  BASE_VIDIOC_PRIVATE + 10, uint8_t)

struct mtr_mcm_update {
	uint32_t port;
	uint32_t index;
};

struct mcm_stream_status {
	uint32_t pad;
	uint32_t status;
};

struct mcm_buf_state {
	uint32_t pad;
	uint32_t sensor_out_state;
};

enum mcm_event_id {
	MCM_EVENT_QBUF,
};

struct mcm_plane {
	uint32_t dma_addr;
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint32_t fourcc;
};

struct mcm_buf {
	uint32_t pad;
	uint32_t index;
	struct mcm_plane plane;
};

struct mcm_event_pkg_head {
	uint32_t pad;
	uint8_t  dev;
	uint32_t eid;
	uint64_t shm_addr;
	uint32_t shm_size;
	uint32_t data_size;
};

struct mcm_event_pkg {
	struct mcm_event_pkg_head head;
	uint8_t  ack;
	int32_t  result;
	uint8_t data[2048];
};

struct mcm_bcm_array {
	uint32_t addr;
	uint32_t size;
};

struct mtr_isp_update {
	uint32_t port;
};


/* State 2 and 3 are for debugging purpose, not in use currently */
enum SENSOR_OUT_STATE {
	SENSOR_OUT_DISABLE_MCM_MMU_ENABLE = 0,
	SENSOR_OUT_ENABLE_MCM_MMU_ENABLE,
	SENSOR_OUT_DISABLE_MCM_MMU_DISABLE,
	SENSOR_OUT_ENABLE_MCM_MMU_DISABLE
};

#ifdef __KERNEL__
#include "mcm_subdev.h"
#include "vvcam_v4l2_common.h"

int mcm_qbuf_event(struct mcm_subdev_dev *mcm_dev, int pad,
		struct vvcam_vb2_buffer *buf);

#endif
#endif
