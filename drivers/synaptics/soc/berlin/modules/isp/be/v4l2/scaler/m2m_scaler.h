/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __M2M_SCALER_H__
#define __M2M_SCALER_H__

#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#include "scaler_drv_api.h"
#include "scaler_dbg.h"

#define SCALER_NAME             "scaler"
#define MAX_MEMORY_DEVICE       2
#define SCALER_MAX_INSTANCE     4

/* struct m2m_scaler_ctrls - scaler control set
 * @io_mmu_buffer_output_ctrl:      output buffer is io mmu or not
 * @io_mmu_buffer_capture_ctrl:     capture buffer is io mmu or not
 * @output_buffer_memory_type_ctrl: output buffer is noncontigous or not
 */
struct m2m_scaler_ctrls {
	struct v4l2_ctrl            *io_mmu_buffer_output_ctrl;
	struct v4l2_ctrl            *io_mmu_buffer_capture_ctrl;
	struct v4l2_ctrl            *output_buffer_memory_type_ctrl;
};

/**
 * struct m2m_scaler_ctx - device context data
 *
 * @src:        source frame properties
 * @dst:        destination frame properties
 * @state:      flags to keep track of user configuration
 * @m2m_scaler_dev:  the device this context applies to
 * @fh:         v4l2 file handle
 * @scaler_client_id: scaler client handle
 * @ctrl_handler: v4l2 control handler
 * @m2m_scaler_ctrls: v4l2 controls for the context
 * @io_mmu_buffer_output:      output buffer is io mmu or not
 * @io_mmu_buffer_capture:     capture buffer is io mmu or not
 * @output_buffer_memory_type: output buffer type is noncontigous or not
 * @ctrls_rdy:    whether v4l2 controls are ready to use
 */
struct m2m_scaler_ctx {
	struct frame_info           src;
	struct frame_info           dst;
	u32                         state;
	struct m2m_scaler_dev       *m2m_scaler_dev;
	struct v4l2_fh              fh;
	struct scaler_drv_ctx       *m2m_scaler_ctx;
	struct delayed_work         scaler_delayed_work;
#ifdef USE_WORKER_PROCESS_SCALER
	struct work_struct          scaler_work;
#endif
	struct v4l2_ctrl_handler    ctrl_handler;
	struct m2m_scaler_ctrls     m2m_scaler_ctrls;
	u8                          io_mmu_buffer_output;
	u8                          io_mmu_buffer_capture;
	u8                          output_buffer_memory_type;
	bool                        ctrls_rdy;
	struct mutex                lock;
	struct completion           work_done;
	u8                          cropping_enable;
};

/**
 * struct scaler_m2m_device - v4l2 memory-to-memory device data
 *
 * @vdev:       video device node for v4l2 m2m mode
 * @m2m_dev:    v4l2 m2m device data
 * @ctx:        hardware context data
 * @refcnt:     reference counter
 */
struct scaler_m2m_device {
	struct video_device         *vdev;
	struct v4l2_m2m_dev         *m2m_dev;
	int                         refcnt;
};

/**
 * struct m2m_scaler_dev - abstraction for scaler entity
 *
 * @v4l2_dev:   v4l2 device
 * @vdev:       video device
 * @pdev:       platform device
 * @dev:        device
 * @lock:       mutex protecting this data structure
 * @id:         device index
 * @m2m:        memory-to-memory V4L2 device information
 * @state:      flags used to synchronize m2m and capture mode operation
 * @ref_count:	Reference count to track open instances
 */
struct m2m_scaler_dev {
	struct v4l2_device          v4l2_dev;
	struct video_device         vdev;
	struct platform_device      *pdev;
	struct device               *dev;
	struct device               *alloc_dev[MAX_MEMORY_DEVICE];
	struct mutex                lock;
	u16                         id;
	struct scaler_m2m_device    m2m;
	unsigned long               state;
	struct scaler_dbg           dbg[MAX_SCALER_INSTANCE];
	int ref_count;
};

#endif //__M2M_SCALER_H__
