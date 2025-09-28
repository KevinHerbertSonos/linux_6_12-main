// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VPU_COMMON_H__
#define __VPU_COMMON_H__

#include <linux/cdev.h>
#include <linux/dma-buf.h>
#include <linux/kfifo.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/clk.h>

#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>

#include "vpu_fw_data.h"
#include "v4g_pix_mod.h"
#include "vpu_mfd.h"
#include "vpu_enc_ctrls.h"
#include "vpu_tee_wrap.h"

#define SYNA_VPU_STATUS_ERROR			(0)
#define SYNA_VPU_STATUS_EOS			(1)
#define SYNA_VPU_STATUS_SET_FMT			(2)
/* HW would be idle when these status flags are set */
#define SYNA_VPU_STATUS_WAIT_INPUT_BUF		(5)
#define SYNA_VPU_STATUS_WAIT_OUTPUT_BUF		(6)
#define SYNA_VPU_STATUS_WAIT_DISP_BUF		(7)
#define SYNA_VPU_STATUS_WAIT_RECON_BUF		SYNA_VPU_STATUS_WAIT_DISP_BUF
#define SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP	(8)
/* buffer pool status flag */
#define SYNA_VPU_POOL_ON_INPUT			(15)
#define SYNA_VPU_POOL_ON_OUTPUT			(16)
#define SYNA_VPU_POOL_ON_AUX			(17)
#define SYNA_VPU_POOL_ON_REFS			SYNA_VPU_POOL_ON_AUX
/* the other status flags */
#define SYNA_VPU_DEC_POST_PROCESSING		(20)
#define SYNA_VPU_DEC_PP_DITHER_EN		(21)

#define SYNA_VPU_STATUS_WAIT_MASK	GENMASK(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, \
						SYNA_VPU_STATUS_WAIT_INPUT_BUF)

#define SYNA_VPU_MAX_BUF_SLOT		(32U)
#define BUFPOOL_SLOT_SHIFT		(5)
#define MAX_BUFPOOL_SLOT_NUM		(1 << BUFPOOL_SLOT_SHIFT)

#define IS_HW_MATCH(dev, ver)		(((dev)->hw_ver_bit & (ver)) != 0)

#define SYNA_DOLPHIN_BIT		BIT(0)
#define SYNA_PLATYPUS_BIT		BIT(1)

#define SYNA_VPU_ENC_HW			(SYNA_DOLPHIN_BIT | SYNA_PLATYPUS_BIT)
#define SYNA_VPU_DEC_HW			(SYNA_DOLPHIN_BIT | SYNA_PLATYPUS_BIT)

/* GKI */
#ifndef V4L2_PIX_FMT_AV1
#define V4L2_PIX_FMT_AV1		v4l2_fourcc('A', 'V', '1', '0')
#define NO_ENUM_V4L2_PIX_FMT_AV1	(1)
#endif
#ifndef V4L2_PIX_FMT_NV15M
#define V4L2_PIX_FMT_NV15M		v4l2_fourcc('N', 'M', '1', '5')
#define NO_ENUM_V4L2_PIX_FMT_NV15M	(1)
#endif

struct syna_vpu_task;
struct syna_vpu_srv;
struct syna_vpu_fw_ops;
struct syna_vpu_variant;

struct syna_vpu_dev {
	struct device *dev;

	struct syna_vpu_variant const *variant;
	/* we can reuse variant->hw_ver if dtsi support it */
	u32 hw_ver_bit;
	struct clk *core_clk;
	int irq;
	struct syna_vpu_srv *srv;
	/* device_run work */
	struct workqueue_struct *vcodec_workq;
	struct work_struct job_work;

	struct cdev cdev;
	dev_t dev_id;

	struct rw_semaphore resource_rwsem;

	struct syna_vpu_fw_data fw_data;
	const struct syna_vpu_fw_ops *fw_ops;

	VPU_TEEC_Context *tee_ctx;
	VPU_TEEC_Session tee_session;

	/*
	 * layout:
	 * ************************
	 * BERLIN_VPU_CFG_RESERVED_SIZE
	 * ************************
	 * Aligned to page size(4K)
	 * ************************
	 * vpu_context_size
	 * ************************
	 * Aligned to page size(4K)
	 * ************************
	 * vpu_hw_context_size
	 * ************************
	 * Aligned to 1K
	 * ************************
	 */
	void *vpu_ctx_buf;

	/* v4l2, only encoder uses them now */
	/* vb2 queue lock, why all the instace share the same lock ? */
	struct mutex mutex;
	struct v4l2_device v4l2_dev;
	struct video_device *vfd;
	struct v4l2_m2m_dev *m2m_dev;
};

enum flush_state {
	FLUSH_NONE,
	FLUSH_START,
	FLUSH_DONE,
	RES_CHANGING,
};

struct syna_vcodec_ctx {
	struct syna_vpu_dev *vpu;

	unsigned long status;

	u32 sequence_out;
	u32 sequence_cap;
	u32 input_es_seq;
	u32 input_pic_seq;

	const void *vpu_src_fmt;
	struct v4l2_pix_format_mplane src_fmt;
	const void *vpu_dst_fmt;
	struct v4l2_pix_format_mplane dst_fmt;

	struct v4l2_fh fh;

	struct v4l2_ctrl_handler ctrl_handler;
	union {
		struct vb2_buffer recon_buffers[32];
		struct vb2_buffer ref_buffers[32];
	};

	struct v4l2_pix_format_mplane ref_fmt;

	/* Only decoder use these variables */
	struct v4l2_pix_format_mplane ref_buf_fmt;
	struct vb2_v4l2_buffer *cur_src_buf;
	/* dummy buffer to notify flush */
	struct v4l2_m2m_buffer eof_flush_buf;
	enum flush_state flushing;

	u32 n_req_ref_bufs;
	u32 req_dpb_size;
	bool enable_user_dpb;
	u32 n_user_dpb;
	/* Only for decoder */
	struct syna_vdec_seq_desc seq_desc;

	struct completion work_done;

	struct list_head service_link;

#if IS_ENABLED(CONFIG_OPTEE)
	struct tee_shm *ctrl_shm;
	void *ctx_buf;
	struct tee_shm *cfg_shm;
#else
	/*
	 * layout:
	 * *********************
	 * sizeof(struct syna_vpu_ctrl)
	 * *********************
	 * Aligned to page size(4K)
	 * *********************
	 */
	TEEC_SharedMemory *ctrl_shm;
	/*
	 * strm_context_size
	 * *********************
	 * Aligned to page size(4K)
	 * *********************
	 * strm_hw_context_size
	 * *********************
	 * Aligned to page size(4K)
	 * *********************
	 * pool, input and output are in the same buffer
	 * layout:
	 * *********************
	 * Input buffer description * 32
	 * *********************
	 * Output buffer description * 32
	 * *********************
	 * Auxiliary buffer description * 32
	 * *********************
	 * Aligned to page size(4K)
	 * *********************
	 */
	void *ctx_buf;

	TEEC_SharedMemory *cfg_shm;
#endif
	union {
		struct syna_venc_strm_config *enc_params;
		struct syna_vdec_config *dec_params;
	};

	struct vpu_enc_ctrls v4l2_ctrls;

	/* pool */
	void *input_pool;
	void *output_pool;
	void *aux_pool;
	void *ds_pool;

	u32 input_pool_num;
	u32 output_pool_num;

	/* restruction buffer for encoder and reference buffers for decoder */
	union {
		u32 n_recon_pool;
		u32 n_ref_pool;
	};

	union {
		long recon_slots;
		long ref_slots;
	};
	long dst_vb_bits;
	bool eos;
	bool cap_resetup;

	struct device *mtr_dh_dev;
};

struct syna_vpu_fw_ops {
	/* stream context switching */
	int (*fw_inst_swap) (struct syna_vcodec_ctx *, struct syna_vcodec_ctx *);
	/* stream context close */
	int (*release) (struct syna_vcodec_ctx *);
	/* stream context switch in */
	int (*switch_in) (struct syna_vcodec_ctx *);
	/* stream context switch out */
	int (*switch_out) (struct syna_vcodec_ctx *);
};

static inline struct syna_vcodec_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct syna_vcodec_ctx, fh);
}

/* GKI wrap func */
static inline void syna_v4l2_m2m_last_buf_done(struct v4l2_m2m_ctx *m2m_ctx,
					       struct vb2_v4l2_buffer *vbuf)
{
	v4l2_m2m_mark_stopped(m2m_ctx);

	vbuf->flags |= V4L2_BUF_FLAG_LAST;
	vb2_buffer_done(&vbuf->vb2_buf, VB2_BUF_STATE_DONE);
}
#endif
