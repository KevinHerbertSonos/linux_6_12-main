// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <media/v4l2-event.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>

#include <vpu_mfd.h>
#include "vpu_common.h"
#include "vpu_dmaheap.h"
#include "vpu_srv.h"
#include "vpu_fw.h"
#include "vpu_fw_data.h"
#include "v4g_iommu_mtr.h"
#include "vpu_kernel_compatibility_wrap.h"
#include "vpu_dec_drv.h"

/* NOTE: one second is not enough, how comes */
#define DEC_V4G_TIMEOUT_DELAY		(2000U)

#define SYNA_DEFAULT_FRAMERATE_NUM	(60000U)
#define SYNA_DEFAULT_FRAMERATE_DENOM	(1001U)

#define SYNA_DEC_MAX_CTRLS_HINT		(7)

#define SYNA_V4G_DEC_NAME "syna-v4g-vdec"
#define SYNA_V4G_STR "synaptics-v4g"

#define vdpu_dbg(vpu, level, fmt, arg...)	\
		v4l2_dbg(level, vdpu_debug, &vpu->v4l2_dev, fmt, ##arg)
#define vdpu_info(vpu, fmt, arg...)	\
		v4l2_info(&vpu->v4l2_dev, fmt, ##arg)
#define vdpu_err(vpu, fmt, arg...)	\
		v4l2_err(&vpu->v4l2_dev, fmt, ##arg)

static int vdpu_debug;
module_param(vdpu_debug, int, 0660);
MODULE_PARM_DESC(vdpu_debug, "video decoder debug level");

static int vpu_mmu_debug;
module_param(vpu_mmu_debug, int, 0600);
MODULE_PARM_DESC(vpu_mmu_debug, "video mmu debug params");

#define IS_CONTIG_VB	(vpu_mmu_debug & BIT(0))
#define IS_MMU_DISABLE	(vpu_mmu_debug & BIT(1))

static const struct v4l2_frmsize_stepwise syna_v4g_framesizes = {
	.min_width = 64,
	.max_width = 4096,
	.step_width = 1,
	.min_height = 64,
	.max_height = 4096,
	.step_height = 2,
};

/**
 * @mem_planes: Number of memory planes, which includes the alpha plane (1 to 4).
 * @comp_planes: Number of component planes(luma, chroma)
 * @flags: v4l2 Image Format Description Flags
 */
struct v4g_fmt {
	u32 fourcc;
	u64 modifier;
	enum syna_dec_format fw_format;
	int bit_depth;
	u32 mem_planes;
	u32 comp_planes;
	u8 char_per_block[VB2_MAX_PLANES];
	u8 block_w[VB2_MAX_PLANES];
	u8 block_h[VB2_MAX_PLANES];
	bool is_codec;
	bool postprocessed;
	const struct v4g_fmt *tile_fmt;
	struct v4l2_frmsize_stepwise frmsize;
	bool is_skip;
	u32 flags;
};

static const struct v4g_fmt nv12m_h4v1_fmt = {
	.fourcc = V4L2_PIX_FMT_NV12M,
	.modifier = DRM_FORMAT_MOD_SYNA_V4H1_128L128_COMPRESSED,
	.fw_format = VDEC_OUTPUT_TILE,
	.bit_depth = 8,
	.mem_planes = 5,
	.comp_planes = 2,
};

static const struct v4g_fmt nv15m_v4h3p8_fmt = {
	.fourcc = V4L2_PIX_FMT_NV15M,
	.modifier = DRM_FORMAT_MOD_SYNA_V4H3P8_128L128_COMPRESSED,
	.fw_format = VDEC_OUTPUT_TILE,
	.bit_depth = 10,
	.mem_planes = 5,
	.comp_planes = 2,
};

static const struct v4g_fmt syna_v4g_dec_fmts[] = {
	/* Those formats need the extra buffers and post-processing */
	{
	 .fourcc = V4L2_PIX_FMT_NV12M,
	 .fw_format = VDEC_OUTPUT_SPUV,
	 .bit_depth = 8,
	 .mem_planes = 2,
	 .comp_planes = 2,
	 .postprocessed = true,
	 .tile_fmt = &nv12m_h4v1_fmt,
	 },
	{
	 .fourcc = V4L2_PIX_FMT_NV15M,
	 .fw_format = VDEC_OUTPUT_SPUV,
	 .bit_depth = 10,
	 .mem_planes = 2,
	 .comp_planes = 2,
	 .postprocessed = true,
	 .tile_fmt = &nv15m_v4h3p8_fmt,
#ifdef NO_ENUM_V4L2_PIX_FMT_NV15M
	 .is_skip = true,
#endif
	 },
	/* codec formats */
	{
	 .fourcc = V4L2_PIX_FMT_H264,
	 .fw_format = SYNA_VXG_H264,
	 .is_codec = true,
	 .flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	 .frmsize = {
		     .min_width = 64,
		     .max_width = 4096,
		     .step_width = 1,
		     .min_height = 64,
		     .max_height = 4096,
		     .step_height = 2,
		     },

	 },
	{
	 .fourcc = V4L2_PIX_FMT_HEVC,
	 .fw_format = SYNA_VXG_H265,
	 .is_codec = true,
	 .flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	 .frmsize = {
		     .min_width = 64,
		     .max_width = 4096,
		     .step_width = 1,
		     .min_height = 64,
		     .max_height = 4096,
		     .step_height = 2,
		     },
	 },
	{
	 .fourcc = V4L2_PIX_FMT_VP8,
	 .fw_format = SYNA_VXG_VP8,
	 .is_codec = true,
	 .flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	 .frmsize = {
		     .min_width = 64,
		     .max_width = 4096,
		     .step_width = 1,
		     .min_height = 64,
		     .max_height = 4096,
		     .step_height = 2,
		     },
	 },
	{
	 .fourcc = V4L2_PIX_FMT_VP9,
	 .fw_format = SYNA_VXG_VP9,
	 .is_codec = true,
	 .flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	 .frmsize = {
		     .min_width = 64,
		     .max_width = 4096,
		     .step_width = 1,
		     .min_height = 64,
		     .max_height = 4096,
		     .step_height = 2,
		     },
	 },
	{
	 .fourcc = V4L2_PIX_FMT_AV1,
	 .fw_format = SYNA_VXG_AV1,
	 .is_codec = true,
	 .flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	 .frmsize = {
		     .min_width = 64,
		     .max_width = 4096,
		     .step_width = 1,
		     .min_height = 64,
		     .max_height = 4096,
		     .step_height = 2,
		     },
#ifdef NO_ENUM_V4L2_PIX_FMT_AV1
	 .is_skip = true,
#endif
	 },
};

static inline void idx_queue_push(struct idx_queue *q, uint32_t key,
				  uint32_t value)
{
	q->index[key] = value;
	q->push++;
}

static uint32_t idx_queue_pop(struct idx_queue *q, uint32_t key)
{
	return q->index[key];
}

static inline void idx_queue_clear(struct idx_queue *q)
{
	memset(q, 0, sizeof(*q));
}

static int vdec_job_ready(void *priv);
static int syna_vdec_update_pop_status(struct syna_vcodec_ctx *ctx);
static int vdec_capture_setup(struct syna_vcodec_ctx *ctx);

static uint32_t vdpu_v4g_tile_stride(struct v4l2_pix_format_mplane *pix_mp,
				     const struct v4g_fmt *vpu_fmt,
				     uint32_t codec, bool for_vpp)
{
	uint32_t stride = ALIGN(pix_mp->width, 16);

	/* cfg.mtr_x_align_mode */
	if (codec == V4L2_PIX_FMT_HEVC)
		stride++;

	stride = ALIGN(stride, 1 << SYNA_V4G_PM_W_LOG2(vpu_fmt->modifier));

	/**
	 * VPP(downstream device) requires tile format be aligned
	 * with 1024.
	 */
	if (for_vpp)
		stride = ALIGN(stride, SZ_1K);

	return stride;
}

static inline void vdpu_es_mark_ts(struct syna_vcodec_ctx *ctx, uint32_t index,
				   u64 timestamp)
{
	struct syna_vpu_tz_buf *vpu_buf;

	vpu_buf = ctx->input_pool;
	vpu_buf += index;

	vpu_buf->timestamp = timestamp;
}

static int vidioc_vdec_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct syna_vcodec_ctx *ctx;
	struct syna_vdec_config *p;
	struct syna_vpu_dev *vpu;
	int ret;

	ctx = container_of(ctrl->handler, struct syna_vcodec_ctx, ctrl_handler);
	vpu = ctx->vpu;
	p = ctx->dec_params;

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE:
		ctx->enable_user_dpb = ctrl->val;
		ret = 0;
		break;
	case V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY:
		ctx->n_user_dpb = ctrl->val;
		if (ctx->enable_user_dpb) {
			p->user_dpb_size = ctx->n_user_dpb;
		} else {
			switch (ctx->src_fmt.pixelformat) {
			case V4L2_PIX_FMT_VP8:
			case V4L2_PIX_FMT_VP9:
			case V4L2_PIX_FMT_AV1:
				p->user_dpb_size = 8;
				break;
			default:
				p->user_dpb_size = 0;
				break;
			}
		}
		ret = 0;
		/* Firmware can't update this value during the decoding */
		break;
	default:
		vdpu_dbg(vpu, 2, "unknown ctrl id = %d", ctrl->id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int vidioc_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct syna_vcodec_ctx *ctx;
	struct syna_vpu_dev *vpu;
	int ret;

	ctx = container_of(ctrl->handler, struct syna_vcodec_ctx, ctrl_handler);
	vpu = ctx->vpu;

	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ctrl->val = ctx->req_dpb_size;
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops syna_vpu_dec_ctrl_ops = {
	.s_ctrl = vidioc_vdec_s_ctrl,
	.g_volatile_ctrl = vidioc_g_volatile_ctrl,
};

static int syna_vdec_ctrls_setup(struct syna_vcodec_ctx *ctx)
{
	struct v4l2_ctrl_handler *handler = &ctx->ctrl_handler;
	const struct v4l2_ctrl_ops *ops = &syna_vpu_dec_ctrl_ops;

	v4l2_ctrl_handler_init(handler, SYNA_DEC_MAX_CTRLS_HINT);

	v4l2_ctrl_new_std_menu(handler, ops, V4L2_CID_MPEG_VIDEO_H264_PROFILE,
			       V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10,
			       ~((1 << V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE) |
				 (1 << V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE) |
				 (1 << V4L2_MPEG_VIDEO_H264_PROFILE_MAIN) |
				 (1 << V4L2_MPEG_VIDEO_H264_PROFILE_HIGH) |
				 (1 << V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10)),
			       V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE);

	v4l2_ctrl_new_std_menu(handler, ops, V4L2_CID_MPEG_VIDEO_HEVC_PROFILE,
			       V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_10,
			       0, V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN);

	v4l2_ctrl_new_std_menu(handler, ops, V4L2_CID_MPEG_VIDEO_VP8_PROFILE,
			       V4L2_MPEG_VIDEO_VP8_PROFILE_3,
			       0, V4L2_MPEG_VIDEO_VP8_PROFILE_0);

	v4l2_ctrl_new_std_menu(handler, ops, V4L2_CID_MPEG_VIDEO_VP9_PROFILE,
			       V4L2_MPEG_VIDEO_VP9_PROFILE_2,
			       ~((1 << V4L2_MPEG_VIDEO_VP9_PROFILE_0) |
				 (1 << V4L2_MPEG_VIDEO_VP9_PROFILE_2)),
			       V4L2_MPEG_VIDEO_VP9_PROFILE_0);

	v4l2_ctrl_new_std(handler, NULL,
			  V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, 1, 32, 1, 2);

	v4l2_ctrl_new_std(handler, NULL, V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY,
			  0, 16, 1, 0);
	v4l2_ctrl_new_std(handler, NULL,
			  V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE,
			  0, 1, 1, 0);

	if (handler->error)
		return handler->error;

	v4l2_ctrl_handler_setup(handler);

	return 0;
}

static const struct v4g_fmt *v4g_get_default_fmt(struct syna_vcodec_ctx *ctx,
						 bool bitstream)
{
	const struct v4g_fmt *formats;
	unsigned int i, num_fmts;

	formats = syna_v4g_dec_fmts;
	num_fmts = ARRAY_SIZE(syna_v4g_dec_fmts);
	for (i = 0; i < num_fmts; i++) {
		if (bitstream == formats[i].is_codec)
			return &formats[i];
	}

	return NULL;
}

static int vidioc_enum_fmt(struct v4l2_fmtdesc *f,
			   const struct v4g_fmt *formats,
			   size_t num_formats, bool capture)
{
	const struct v4g_fmt *fmt;
	unsigned int i, j = 0;

	if (f->index >= num_formats)
		return -EINVAL;

	for (i = 0; i < num_formats; i++) {
		bool mode_none = !formats[i].is_codec;

		if (capture == mode_none)
			continue;
		if (formats[i].is_skip)
			continue;

		if (j == f->index) {
			fmt = &formats[i];
			f->pixelformat = fmt->fourcc;
			f->flags = fmt->flags;
			return 0;
		}
		++j;
	}

	return -EINVAL;
}

static const struct v4g_fmt *syna_v4g_find_format(u32 fourcc, u64 modifier,
						  const struct v4g_fmt *formats,
						  size_t num_formats)
{
	const struct v4g_fmt *fmt;
	unsigned int i;

	for (i = 0; i < num_formats; i++) {
		fmt = &formats[i];
		if (fourcc == fmt->fourcc)
			return fmt;
	}

	return NULL;
}

static int vidioc_enum_framesizes(struct file *file, void *fh,
				  struct v4l2_frmsizeenum *fsize)
{
	const struct v4g_fmt *fmt;

	fmt = syna_v4g_find_format(fsize->pixel_format, 0,
				   syna_v4g_dec_fmts,
				   ARRAY_SIZE(syna_v4g_dec_fmts));
	if (!fmt)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	if (fmt->frmsize.max_width > 0)
		fsize->stepwise = fmt->frmsize;
	else
		fsize->stepwise = syna_v4g_framesizes;

	return 0;
}

static int vidioc_enum_frameintervals(struct file *file, void *fh,
				      struct v4l2_frmivalenum *f)
{
	const struct v4g_fmt *fmt;

	fmt = syna_v4g_find_format(f->pixel_format, 0,
				   syna_v4g_dec_fmts,
				   ARRAY_SIZE(syna_v4g_dec_fmts));
	if (!fmt)
		return -EINVAL;

	if (fmt->frmsize.max_width > 0) {
		if (f->width < fmt->frmsize.min_width
		    || f->width > fmt->frmsize.max_width
		    || f->height < fmt->frmsize.min_height
		    || f->height > fmt->frmsize.max_height)
			return -EINVAL;
	}

	f->type = V4L2_FRMIVAL_TYPE_CONTINUOUS;
	f->stepwise.min.numerator = 1;
	f->stepwise.min.denominator = 200;
	f->stepwise.max.numerator = 1;
	f->stepwise.max.denominator = 1;
	f->stepwise.step.numerator = 1;
	f->stepwise.step.denominator = 1;

	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	return vidioc_enum_fmt(f, syna_v4g_dec_fmts,
			       ARRAY_SIZE(syna_v4g_dec_fmts), false);
}

static int vidioc_enum_fmt_vid_out(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	return vidioc_enum_fmt(f, syna_v4g_dec_fmts,
			       ARRAY_SIZE(syna_v4g_dec_fmts), true);
}

static int vidioc_vdec_querycap(struct file *file, void *priv,
				struct v4l2_capability *cap)
{
	struct syna_vpu_dev *vpu = video_drvdata(file);

	strscpy(cap->driver, SYNA_V4G_DEC_NAME, sizeof(cap->driver));
	strscpy(cap->card, SYNA_V4G_STR, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform: %s", vpu->dev->driver->name);

	return 0;
}

static int syna_vpu_prepare_mtr_buffer(struct syna_vcodec_ctx *ctx,
				       struct v4l2_pix_format_mplane *pixfmt)
{
	struct syna_vpu_ctrl *ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	size_t luma_size, cr_size, total_size;
	dma_addr_t memid;

	luma_size = pixfmt->plane_fmt[2].sizeimage;
	cr_size = pixfmt->plane_fmt[3].sizeimage;
	if (!(luma_size && cr_size))
		return 0;

	if (ctx->mtr_dh_dev) {
		if (syna_vpu_dh_get_reserved_mem
		    (ctx->mtr_dh_dev, &memid, &ctrl->mtr_base.size))
			return -EINVAL;

		total_size = MTR_DEFAULT_N_BUFS * (luma_size + cr_size);
		if (ctrl->mtr_base.size >= total_size)
			goto mtr_assign;
		else if (syna_vpu_dh_free_mtr_memdev(ctx->mtr_dh_dev))
			return -EINVAL;
	}

	ctx->mtr_dh_dev =
	    syna_vpu_dh_new_mtr_memdev(ctx->vpu->dev, MTR_DEFAULT_N_BUFS,
				       luma_size, cr_size);

	if (syna_vpu_dh_get_reserved_mem
	    (ctx->mtr_dh_dev, &memid, &ctrl->mtr_base.size))
		return -ENOMEM;

mtr_assign:
	ctrl->mtr_base.addr = 0;
	ctrl->mtr_base.offset = 0;
	ctrl->mtr_base.type = CONTIGUOUS_MEM;
	ctrl->mtr_base.memid = memid;
	ctx->dec_params->mtr_enable = 1;

	return 0;
}

static void syna_set_default_colorspace(struct v4l2_pix_format_mplane *pix_mp)
{
	pix_mp->colorspace = V4L2_COLORSPACE_REC709;
	pix_mp->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	pix_mp->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix_mp->quantization = V4L2_QUANTIZATION_DEFAULT;
}

static int v4g_try_fmt(struct syna_vcodec_ctx *ctx,
		       struct v4l2_pix_format_mplane *pix_mp,
		       const struct v4g_fmt **vpu_fmt, enum v4l2_buf_type type)
{
	const struct v4g_fmt *fmt;
	bool is_output = V4L2_TYPE_IS_OUTPUT(type);
	u32 width, height;
	u32 field;
	int i;

	fmt = syna_v4g_find_format(pix_mp->pixelformat, 0,
				   syna_v4g_dec_fmts,
				   ARRAY_SIZE(syna_v4g_dec_fmts));

	if (!fmt) {
		fmt = v4g_get_default_fmt(ctx, is_output);
		pix_mp->pixelformat = fmt->fourcc;
	}

	field = pix_mp->field;
	pix_mp->flags = 0;

	if (is_output) {
		pix_mp->num_planes = 1;

		if (pix_mp->colorspace == V4L2_COLORSPACE_DEFAULT)
			syna_set_default_colorspace(pix_mp);

		pix_mp->plane_fmt[0].sizeimage =
		    max_t(unsigned int, SZ_1M, pix_mp->plane_fmt[0].sizeimage);
		pix_mp->plane_fmt[0].sizeimage =
		    ALIGN(pix_mp->plane_fmt[0].sizeimage, SZ_4K);
	} else {
		width = pix_mp->width;
		height = pix_mp->height;

		v4l2_apply_frmsize_constraints(&pix_mp->width, &pix_mp->height,
					       &syna_v4g_framesizes);

		switch (fmt->fourcc) {
		case V4L2_PIX_FMT_NV15M:
			pix_mp->plane_fmt[0].bytesperline =
			    ALIGN(DIV_ROUND_UP(pix_mp->width, 4) * 5, 64);
			pix_mp->plane_fmt[1].bytesperline =
			    pix_mp->plane_fmt[0].bytesperline;
			pix_mp->plane_fmt[0].sizeimage =
			    pix_mp->plane_fmt[0].bytesperline
			    * ALIGN(height, 64);
			pix_mp->plane_fmt[1].sizeimage =
			    pix_mp->plane_fmt[0].sizeimage >> 1;
			pix_mp->num_planes = 2;
			break;
		default:
			v4l2_fill_pixfmt_mp(pix_mp, fmt->fourcc,
					    ALIGN(pix_mp->width, 64),
					    ALIGN(pix_mp->height, 64));
			break;
		}
		pix_mp->width = width;
		pix_mp->height = height;

		for (i = 0; i < pix_mp->num_planes; i++) {
			pix_mp->plane_fmt[i].sizeimage =
			    ALIGN(pix_mp->plane_fmt[i].sizeimage, SZ_4K);
		}

		/* NOTE: Not check more colorspace here */
		if (pix_mp->colorspace == V4L2_COLORSPACE_DEFAULT)
			syna_set_default_colorspace(pix_mp);
	}

	pix_mp->field = field;
	if (vpu_fmt)
		*vpu_fmt = fmt;

	return 0;
}

static int vdec_set_default_graphics_fmt(struct syna_vcodec_ctx *ctx, u32 codec)
{
	struct syna_vdec_config *p = ctx->dec_params;
	struct syna_vpu_ctrl *ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	const struct v4g_fmt *fmt;

	fmt = v4g_get_default_fmt(ctx, false);
	if (!fmt)
		return -EINVAL;

	ctx->vpu_dst_fmt = fmt;
	p->output_mode = fmt->fw_format;
	/**
	 * NOTE: Firmware won't recalculate the buffer size after we change
	 * the dither option. So we enabled the dither at the first place,
	 * we could push a larger memory for post processing when we don't
	 * want dither.
	 */
	p->dither_enable = 1;
	p->dither_bit = 8;
#ifdef NO_ENUM_V4L2_PIX_FMT_NV15M
	set_bit(SYNA_VPU_DEC_PP_DITHER_EN, &ctx->status);
#endif

	if (fmt->postprocessed)
		fmt = fmt->tile_fmt;

	SYNA_V4G_CFG_FROM_PIX_MOD(fmt->modifier, (&ctrl->v4g_ext_cfg.mmu), p);
	if (IS_MMU_DISABLE)
		ctrl->v4g_ext_cfg.mmu.en = 0;

	return 0;
}

static int vidioc_vdec_s_fmt_out(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct syna_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct syna_vdec_config *p = ctx->dec_params;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct vb2_queue *vq;
	const struct v4g_fmt *fmt;

	vq = v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx);
	if (!vq) {
		vdpu_err(vpu, "fail to get vq");
		return -EINVAL;
	}

	/*
	 * The decoder doesn't admit a format change if
	 * there are OUTPUT buffers allocated.
	 */
	if (vb2_is_busy(vq)) {
		vdpu_err(vpu, "queue busy");
		return -EBUSY;
	}

	if (v4g_try_fmt(ctx, pix_mp, &fmt, f->type))
		return -EINVAL;

	p->format = fmt->fw_format;
	/* About V4L2_FMT_FLAG_CONTINUOUS_BYTESTREAM, the hw itself it doesn't
	 * support this. It depends on the TA.
	 */
	p->frame_in_mode = 1;

	switch (pix_mp->pixelformat) {
	case V4L2_PIX_FMT_VP8:
	case V4L2_PIX_FMT_VP9:
		p->user_dpb_size = 8;
		/* no ivf header, frame (break SuperFrame) */
		p->vp8_strm_mode = 2;
		break;
	case V4L2_PIX_FMT_AV1:
		p->user_dpb_size = 8;
		/* OBU frame(no TU) */
		p->vp8_strm_mode = 1;
		break;
	default:
		break;
	}

	ctx->vpu_src_fmt = fmt;
	ctx->src_fmt = *pix_mp;

	/* FW need a output format to calculate graphics buffer size */
	vdec_set_default_graphics_fmt(ctx, pix_mp->pixelformat);

	return 0;
}

static void vdec_fmt_from_cur_seq_fmt(struct syna_vcodec_ctx *ctx,
				     struct syna_vpu_ctrl *ctrl,
				     struct v4l2_pix_format_mplane *pix_mp)
{
	const struct v4g_fmt *fmt;
	uint32_t bytesperline, sizeimage;
	uint32_t codec;

	pix_mp->width = ctrl->seq_desc.width;
	pix_mp->height = ctrl->seq_desc.height;
	/**
	 * NOTE: in post-processing buffer, these values are applying
	 * for tiled reference buffers.
	 */
	switch (ctrl->seq_desc.format) {
	case SYNA_VXG_NV12:
	default:
		pix_mp->pixelformat = V4L2_PIX_FMT_NV12M;
		v4l2_fill_pixfmt_mp(&ctx->dst_fmt, pix_mp->pixelformat,
				    ALIGN(pix_mp->width, 64),
				    ALIGN(pix_mp->height, 64));
		ctx->dst_fmt.width = pix_mp->width;
		ctx->dst_fmt.height = pix_mp->height;
		break;
	case SYNA_VXG_NV15:
		pix_mp->pixelformat = V4L2_PIX_FMT_NV15M;

		if (test_bit(SYNA_VPU_DEC_PP_DITHER_EN, &ctx->status)) {
			v4l2_fill_pixfmt_mp(&ctx->dst_fmt, V4L2_PIX_FMT_NV12M,
					    ALIGN(pix_mp->width, 64),
					    ALIGN(pix_mp->height, 64));
			ctx->dst_fmt.width = pix_mp->width;
			ctx->dst_fmt.height = pix_mp->height;
			break;
		}

		ctx->dst_fmt.pixelformat = V4L2_PIX_FMT_NV15M;
		ctx->dst_fmt.num_planes = 2;
		ctx->dst_fmt.width = pix_mp->width;
		ctx->dst_fmt.height = pix_mp->height;
		ctx->dst_fmt.pixelformat = pix_mp->pixelformat;
		ctx->dst_fmt.plane_fmt[0].bytesperline =
		    ALIGN(DIV_ROUND_UP(pix_mp->width, 4) * 5, 64);
		ctx->dst_fmt.plane_fmt[1].bytesperline =
		    ctx->dst_fmt.plane_fmt[0].bytesperline;
		break;
	}

	sizeimage = ctrl->seq_desc.disp_luma_size;
	ctx->dst_fmt.plane_fmt[0].sizeimage = sizeimage;

	sizeimage = ctrl->seq_desc.disp_chroma_size;
	ctx->dst_fmt.plane_fmt[1].sizeimage = sizeimage;

	ctx->dst_fmt.field = V4L2_FIELD_NONE;
	pix_mp->field = V4L2_FIELD_NONE;

	ctx->n_req_ref_bufs = ctrl->seq_desc.max_ref_nums;
	switch (ctx->src_fmt.pixelformat) {
	case V4L2_PIX_FMT_VP8:
	case V4L2_PIX_FMT_VP9:
	case V4L2_PIX_FMT_AV1:
		ctx->req_dpb_size = max_t(uint32_t, 8U,
					  ctrl->seq_desc.max_dpb_size);
		break;
	default:
		ctx->req_dpb_size = ctrl->seq_desc.max_dpb_size;
		break;
	}


	fmt = syna_v4g_find_format(pix_mp->pixelformat, 0,
				   syna_v4g_dec_fmts,
				   ARRAY_SIZE(syna_v4g_dec_fmts));
	if (fmt->tile_fmt)
		fmt = fmt->tile_fmt;
	else
		ctx->req_dpb_size = max_t(uint32_t, ctx->n_req_ref_bufs,
					  ctx->req_dpb_size);

	codec = ctx->src_fmt.pixelformat;

	bytesperline = vdpu_v4g_tile_stride(pix_mp, fmt, codec, true);
	pix_mp->plane_fmt[0].bytesperline = bytesperline;
	pix_mp->plane_fmt[1].bytesperline = bytesperline;

	sizeimage = ctrl->seq_desc.luma_size;
	pix_mp->plane_fmt[0].sizeimage = sizeimage;

	sizeimage = ctrl->seq_desc.chroma_size;
	pix_mp->plane_fmt[1].sizeimage = sizeimage;
	pix_mp->num_planes = 2;

	sizeimage = ctrl->seq_desc.meta_luma_size;
	pix_mp->plane_fmt[2].sizeimage = ALIGN(sizeimage, PAGE_SIZE);

	sizeimage = ctrl->seq_desc.meta_chroma_size;
	pix_mp->plane_fmt[3].sizeimage = ALIGN(sizeimage, PAGE_SIZE);
	if (sizeimage)
		pix_mp->num_planes += 2;

	sizeimage = ctrl->seq_desc.tctx_size;
	pix_mp->plane_fmt[BERLIN_BUF_V4G_MVTP_PLANE_IDX].sizeimage = sizeimage;
	if (sizeimage)
		pix_mp->num_planes++;

	vdpu_dbg(ctx->vpu, 1,
		"[%p] newseq %dx%d d(%d %d) r(%d %d) m(%d %d) t(%d) n(d%d, r%d), req_dpb %d",
		ctx->fh.m2m_ctx, ctrl->seq_desc.width, ctrl->seq_desc.height,
		ctrl->seq_desc.disp_luma_size, ctrl->seq_desc.disp_chroma_size,
		ctrl->seq_desc.luma_size, ctrl->seq_desc.chroma_size,
		ctrl->seq_desc.meta_luma_size, ctrl->seq_desc.meta_chroma_size,
		ctrl->seq_desc.tctx_size, ctrl->seq_desc.max_dpb_size,
		ctrl->seq_desc.max_ref_nums, ctx->req_dpb_size);

	ctx->seq_desc = ctrl->seq_desc;

	return;
}

static int vdpu_update_dst_fmt(struct syna_vcodec_ctx *ctx,
			       const struct v4g_fmt *fmt)
{
	struct syna_vdec_config *p = ctx->dec_params;
	struct syna_vpu_ctrl *ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);

	if (fmt->postprocessed) {
		/* Need internal ref bufs */
		set_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status);
		set_bit(SYNA_VPU_DEC_POST_PROCESSING, &ctx->status);

		p->output_mode = fmt->fw_format;
		set_bit(SYNA_VPU_DEC_PP_DITHER_EN, &ctx->status);
		p->dither_enable = 1;
		p->dither_bit = fmt->bit_depth;

		/* dither */
		if (fmt->fourcc == ctx->ref_fmt.pixelformat) {
			struct v4l2_pix_format_mplane *pix_mp = &ctx->dst_fmt;

			p->dither_enable = 0;
			p->dither_bit = 0;
			clear_bit(SYNA_VPU_DEC_PP_DITHER_EN, &ctx->status);

			if (fmt->fourcc == V4L2_PIX_FMT_NV15M) {
				pix_mp->plane_fmt[0].bytesperline =
				    ALIGN(DIV_ROUND_UP(pix_mp->width, 4) * 5,
					  64);
				pix_mp->plane_fmt[1].bytesperline =
				    pix_mp->plane_fmt[0].bytesperline;
				pix_mp->plane_fmt[0].sizeimage =
				    pix_mp->plane_fmt[0].bytesperline *
				    ALIGN(pix_mp->height, 64);
				pix_mp->plane_fmt[1].sizeimage =
				    pix_mp->plane_fmt[0].sizeimage >> 1;
				pix_mp->num_planes = 2;
			}
		}

		fmt = fmt->tile_fmt;
	} else {
		clear_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status);
		clear_bit(SYNA_VPU_DEC_POST_PROCESSING, &ctx->status);
		clear_bit(SYNA_VPU_DEC_PP_DITHER_EN, &ctx->status);
		ctx->ref_buf_fmt = ctx->ref_fmt;
		p->output_mode = fmt->fw_format;
		p->dither_enable = 0;
	}

	SYNA_V4G_CFG_FROM_PIX_MOD(fmt->modifier, (&ctrl->v4g_ext_cfg.mmu), p);
	if (IS_MMU_DISABLE)
		ctrl->v4g_ext_cfg.mmu.en = 0;

	return 0;
}

static int vidioc_vdec_s_fmt_cap(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct syna_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct syna_vdec_config *p = ctx->dec_params;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct vb2_queue *vq;
	const struct v4g_fmt *fmt;

	vq = v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx);
	if (!vq) {
		vdpu_err(vpu, "fail to get vq");
		return -EINVAL;
	}

	if (vb2_is_busy(vq)) {
		vdpu_err(vpu, "queue busy");
		return -EBUSY;
	}

	fmt =
	    syna_v4g_find_format(pix_mp->pixelformat, 0, syna_v4g_dec_fmts,
				 ARRAY_SIZE(syna_v4g_dec_fmts));
	if (!fmt)
		return -EINVAL;

	if (fmt->postprocessed) {
		set_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status);
		set_bit(SYNA_VPU_DEC_POST_PROCESSING, &ctx->status);
	}
	/**
	 * we only support same resolution here,
	 * composition or scaling would be done in S_SELECTION
	 */
	if (ctx->ref_fmt.width) {
		if (pix_mp->width != ctx->ref_fmt.width)
			return -EINVAL;
		if (pix_mp->height != ctx->ref_fmt.height)
			return -EINVAL;
		if ((pix_mp->pixelformat == V4L2_PIX_FMT_NV15M) &&
		    (ctx->ref_fmt.pixelformat != V4L2_PIX_FMT_NV15M))
			return -EINVAL;

		vdpu_update_dst_fmt(ctx, fmt);

		if (test_bit(SYNA_VPU_DEC_PP_DITHER_EN, &ctx->status)) {
			v4l2_fill_pixfmt_mp(&ctx->dst_fmt, fmt->fourcc,
					    ALIGN(pix_mp->width, 64),
					    ALIGN(pix_mp->height, 64));
			ctx->dst_fmt.plane_fmt[0].sizeimage =
			    ALIGN(ctx->dst_fmt.plane_fmt[0].sizeimage, SZ_4K);
			ctx->dst_fmt.plane_fmt[1].sizeimage =
			    ALIGN(ctx->dst_fmt.plane_fmt[1].sizeimage, SZ_4K);
			ctx->dst_fmt.width = pix_mp->width;
			ctx->dst_fmt.height = pix_mp->height;
		}

		if (pix_mp->plane_fmt[0].bytesperline
		     > ctx->dst_fmt.plane_fmt[0].bytesperline) {
			v4l2_fill_pixfmt_mp(&ctx->dst_fmt, fmt->fourcc,
					    pix_mp->plane_fmt[0].bytesperline,
					    ALIGN(pix_mp->height, 64));

			ctx->dst_fmt.width = pix_mp->width;
			ctx->dst_fmt.height = pix_mp->height;

			p->dis_stride = pix_mp->plane_fmt[0].bytesperline;

			ctx->dst_fmt.plane_fmt[0].sizeimage =
			    ALIGN(ctx->dst_fmt.plane_fmt[0].sizeimage, SZ_4K);
			ctx->dst_fmt.plane_fmt[1].sizeimage =
			    ALIGN(ctx->dst_fmt.plane_fmt[1].sizeimage, SZ_4K);
		} else {
			p->dis_stride = 0;
		}
		/**
		 * after the video parsing state, we could confirm what
		 * user requires needs post-processing here.
		 */
		if (fmt->postprocessed) {
			*pix_mp = ctx->dst_fmt;
		} else {
			*pix_mp = ctx->ref_fmt;
			/**
			 * NOTE: force plane num = 2 for NV12M expose to external.
			 * although internal user plane num = 5.
			 */
			if (fmt->mem_planes > 2)
				pix_mp->num_planes = 2;
			else
				pix_mp->num_planes = fmt->mem_planes;
		}
	} else {
		/* This will not apply to hardware */
		v4g_try_fmt(ctx, pix_mp, &fmt, f->type);
		ctx->dst_fmt = *pix_mp;
	}

	ctx->vpu_dst_fmt = fmt;

	return 0;
}

static int v4g_vdec_g_fmt_cap(struct syna_vcodec_ctx *ctx,
			      struct v4l2_pix_format_mplane *pix_mp)
{
	const enum v4l2_buf_type cap = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	struct syna_vpu_ctrl *ctrl;
	const struct v4g_fmt *src_fmt, *dst_fmt;

	ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	src_fmt = ctx->vpu_src_fmt;
	dst_fmt = ctx->vpu_dst_fmt;

	if (test_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status)) {
		/* NOTE: kernel can't enable both tile and non tile */
		dst_fmt = syna_v4g_find_format(ctx->ref_fmt.pixelformat, 0,
					       syna_v4g_dec_fmts,
					       ARRAY_SIZE(syna_v4g_dec_fmts));
		if (!dst_fmt)
			return -EINVAL;

		if (dst_fmt->is_skip) {
			dst_fmt = syna_v4g_find_format(V4L2_PIX_FMT_NV12M, 0,
						       syna_v4g_dec_fmts,
						       ARRAY_SIZE
						       (syna_v4g_dec_fmts));
		}

		vdpu_update_dst_fmt(ctx, dst_fmt);
		ctx->vpu_dst_fmt = dst_fmt;
	}

	/* pixelformat in dst_fmt is updated by vdec_fmt_from_cur_seq_fmt() */
	if (dst_fmt) {
		/**
		 * we only support same resolution here,
		 * composition or scaling would be done in S_SELECTION
		 */
		*pix_mp = ctx->dst_fmt;
	} else {
		/* return fake values here */
		pix_mp->pixelformat = dst_fmt->fourcc;
		pix_mp->width = ctx->src_fmt.width;
		pix_mp->height = ctx->src_fmt.height;

		if (v4g_try_fmt(ctx, pix_mp, &dst_fmt, cap))
			return -EINVAL;

		ctx->dst_fmt = *pix_mp;
	}

	return 0;
}

static int vidioc_vdec_g_fmt(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct syna_vcodec_ctx *ctx = fh_to_ctx(priv);
	bool capture = (!V4L2_TYPE_IS_OUTPUT(f->type));

	if (capture)
		return v4g_vdec_g_fmt_cap(ctx, pix);

	*pix = ctx->src_fmt;
	return 0;
}

static int vidioc_try_fmt_vid_cap_mplane(struct file *file, void *priv,
					 struct v4l2_format *f)
{
	return v4g_try_fmt(fh_to_ctx(priv), &f->fmt.pix_mp, NULL, f->type);
}

static int vidioc_try_fmt_vid_out_mplane(struct file *file, void *priv,
					 struct v4l2_format *f)
{
	return v4g_try_fmt(fh_to_ctx(priv), &f->fmt.pix_mp, NULL, f->type);
}

static int syna_vpu_subscribe_event(struct v4l2_fh *fh,
				    const struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		return v4l2_ctrl_subscribe_event(fh, sub);
	}
}

static int vidioc_vdec_qbuf(struct file *file, void *priv,
			    struct v4l2_buffer *buf)
{
	struct syna_vcodec_ctx *ctx = fh_to_ctx(priv);

	return v4l2_m2m_qbuf(file, ctx->fh.m2m_ctx, buf);
}

static int vidioc_vdec_dqbuf(struct file *file, void *priv,
			     struct v4l2_buffer *buf)
{
	struct syna_vcodec_ctx *ctx = fh_to_ctx(priv);

	return v4l2_m2m_dqbuf(file, ctx->fh.m2m_ctx, buf);
}

static int vidioc_decoder_cmd(struct file *file, void *priv,
			      struct v4l2_decoder_cmd *ec)
{
	struct syna_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct vb2_queue *src_vq, *dst_vq;
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	int ret;

	ret = v4l2_m2m_ioctl_try_decoder_cmd(file, priv, ec);
	if (ret)
		return ret;

	vdpu_dbg(vpu, 1, "[%p] decoder cmd=%u", m2m_ctx, ec->cmd);

	dst_vq = v4l2_m2m_get_dst_vq(m2m_ctx);
	/**
	 * NOTE: In drain section
	 * V4L2_DEC_CMD_START - the decoder will not be reset and will resume
	 * operation normally, with all the state from before the drain.
	 * We don't have a way to let the hardware return all its buffer.
	 */
	switch (ec->cmd) {
	case V4L2_DEC_CMD_STOP:
		src_vq = v4l2_m2m_get_src_vq(m2m_ctx);

		if (!vb2_is_streaming(src_vq)) {
			vdpu_dbg(vpu, 1, "Output stream is off. No need to flush.");
			return 0;
		}
		if (!vb2_is_streaming(dst_vq)) {
			vdpu_dbg(vpu, 1, "Capture stream is off. No need to flush.");
			return 0;
		}
		/* use flush vb to flush decoder.
		 * not call v4l2_update_last_buf_state but update status here.
		 */
		if (m2m_ctx->is_draining)
			return -EBUSY;
		if (m2m_ctx->has_stopped)
			return 0;

		m2m_ctx->is_draining = true;
		v4l2_m2m_buf_queue(m2m_ctx, &ctx->eof_flush_buf.vb);
		v4l2_m2m_try_schedule(m2m_ctx);
		break;
	case V4L2_DEC_CMD_START:
		vb2_clear_last_buffer_dequeued(dst_vq);
		if (test_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status)) {
			ret = vdec_capture_setup(ctx);
			if (ret)
				return ret;
			clear_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status);
			ctx->cap_resetup = false;
		}
		ctx->eos = false;
		ret = v4l2_m2m_decoder_cmd(file, m2m_ctx, ec);
		v4l2_m2m_try_schedule(m2m_ctx);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

const struct v4l2_ioctl_ops syna_vdec_ioctl_ops = {
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = vidioc_vdec_qbuf,
	.vidioc_dqbuf = vidioc_vdec_dqbuf,

	.vidioc_querycap = vidioc_vdec_querycap,
	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out = vidioc_enum_fmt_vid_out,
	.vidioc_enum_framesizes = vidioc_enum_framesizes,
	.vidioc_enum_frameintervals = vidioc_enum_frameintervals,

	.vidioc_try_fmt_vid_cap_mplane = vidioc_try_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_out_mplane = vidioc_try_fmt_vid_out_mplane,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_subscribe_event = syna_vpu_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,

	.vidioc_s_fmt_vid_cap_mplane = vidioc_vdec_s_fmt_cap,
	.vidioc_s_fmt_vid_out_mplane = vidioc_vdec_s_fmt_out,

	.vidioc_g_fmt_vid_cap_mplane = vidioc_vdec_g_fmt,
	.vidioc_g_fmt_vid_out_mplane = vidioc_vdec_g_fmt,

	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,

	.vidioc_decoder_cmd = vidioc_decoder_cmd,
	.vidioc_try_decoder_cmd = v4l2_m2m_ioctl_try_decoder_cmd,
};

static int vb2ops_syna_vpu_queue_setup(struct vb2_queue *vq,
				       unsigned int *nbuffers,
				       unsigned int *nplanes,
				       unsigned int sizes[],
				       struct device *alloc_devs[])
{
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(vq);
	struct syna_vpu_dev *vpu = ctx->vpu;
	unsigned int q_num_bufs = vb2_get_num_buffers(vq);
	struct v4l2_pix_format_mplane *pixfmt;
	unsigned int i;

	vdpu_dbg(vpu, 1, "[%p] %s queue setup %d %d", ctx->fh.m2m_ctx,
		 V4L2_TYPE_IS_OUTPUT(vq->type) ? "output" : "capture",
		 *nbuffers, *nplanes);

	switch (vq->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		pixfmt = &ctx->dst_fmt;
		alloc_devs[0] = sys_iommu_dev;
		alloc_devs[1] = sys_iommu_dev;
		if (IS_MMU_DISABLE || IS_CONTIG_VB) {
			alloc_devs[0] = cma_dh_dev;
			alloc_devs[1] = cma_dh_dev;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		pixfmt = &ctx->src_fmt;
		alloc_devs[0] = cma_dh_dev;
		break;
	default:
		vdpu_err(vpu, "invalid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	if (*nplanes) {
		if (q_num_bufs >= SYNA_VPU_MAX_BUF_SLOT)
			return -ENOBUFS;

		if (*nplanes < pixfmt->num_planes)
			*nplanes = pixfmt->num_planes;
	} else {
		*nplanes = pixfmt->num_planes;

		switch (vq->type) {
		case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
			*nbuffers =
				clamp(*nbuffers, ctx->req_dpb_size + 3, 18U);
			break;
		case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		default:
			*nbuffers = clamp(*nbuffers, 2U, 8U);
			break;
		}
	}

	for (i = 0; i < *nplanes; i++)
		sizes[i] = max_t(unsigned int, sizes[i],
				 pixfmt->plane_fmt[i].sizeimage);

	vdpu_dbg(vpu, 1, "[%p] %s queue setup adjust %d %d", ctx->fh.m2m_ctx,
		 V4L2_TYPE_IS_OUTPUT(vq->type) ? "output" : "capture",
		 *nbuffers, *nplanes);
	return 0;
}

static int vb2ops_vdec_buf_out_validate(struct vb2_buffer *vb)
{
	return 0;
}

static struct vb2_plane *vdpu_alloc_interanl_mvtp_buf(struct syna_vcodec_ctx
						      *ctx, u32 size, u32 idx)
{
	struct vb2_plane *plane;

	if (!size)
		return NULL;

	plane = &(ctx->ref_buffers[idx].planes[BERLIN_BUF_V4G_MVTP_PLANE_IDX]);

	if (plane->mem_priv) {
		if (size > plane->length) {
			vb2_syna_dh_bm_put(plane->mem_priv);
			plane->mem_priv = NULL;
		}
	}

	if (!plane->mem_priv)
		plane->mem_priv = syna_dh_bm_alloc(cma_dh_dev, size);

	plane->length = size;

	return plane;
}

static inline void fill_plane_mem(struct syna_tz_generic_buf *plane,
			     enum syna_vpu_mem_type type, u32 memid,
			     dma_addr_t addr, u32 gfp_flags)
{
	plane->type = type;
	plane->memid = memid;
	plane->addr = addr;
	plane->gfp_flags = gfp_flags;
}

static int vdpu_fill_video_buf(struct syna_vcodec_ctx *ctx,
			       struct vb2_buffer *vb,
			       const struct v4g_fmt *vpu_fmt)
{
	struct syna_vpu_tz_buf *vpu_buf;
	struct syna_tz_generic_buf *plane;
	dma_addr_t memid = 0;
	u32 i;

	if (vpu_fmt && vpu_fmt->postprocessed) {
		vpu_buf = ctx->aux_pool;
		vpu_buf += vb->index;
	} else {
		vpu_buf = ctx->output_pool;
		vpu_buf += vb->index;
	}

	vpu_buf->buf_type = DECODER_PICTURE;

	memset(vpu_buf->planes, 0, sizeof(struct syna_tz_generic_buf) << 3);
	memset(vpu_buf->bytesused, 0, sizeof(vpu_buf->bytesused));

	for (i = 0; i < vb->num_planes; i++) {
		plane = &vpu_buf->planes[i];
		plane->size = vb2_plane_size(vb, i);
		plane->offset = vb->planes[i].data_offset;
		vpu_buf->bytesused[i] = vb->planes[i].bytesused;

		/* NOTE: currently defalut NV12M, plane0=Y, plane1=C, need ajust it
		 * if needd support plalar pixel format
		 */
		memid = vb2_syna_bm_get_addr(vb, i, BM_MEMID);

		if ((i > 1) || (memid && IS_MMU_DISABLE)) {
			fill_plane_mem(plane, MEM_ID_CONTINOUS, memid, 0, 0);
		} else {
			if (memid) {
				fill_plane_mem(plane, MEM_ID_SCATTER, 0,
					       vb2_syna_bm_get_addr(vb, i, BM_PTPHY),
					       BERLIN_GFP_FLAG_NO_MAPPING);
			} else {
				fill_plane_mem(plane, PHY_CONTINOUS, 0,
					       vb2_syna_bm_get_addr(vb, i, BM_DMA),
					       0);
			}
		}
	}
	return 0;
}

static int vdpu_fill_es_buf(struct vb2_buffer *vb)
{
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct syna_vpu_tz_buf *vpu_buf;
	dma_addr_t memid;
	u32 i;

	vpu_buf = ctx->input_pool;
	vpu_buf += vb->index;
	vpu_buf->buf_type = DECODER_STREAM;

	memset(vpu_buf->planes, 0, sizeof(struct syna_tz_generic_buf) << 3);

	for (i = 0; i < vb->num_planes; i++) {
		memid = syna_vpu_bm_dh_plane_memid(vb, i);
		vpu_buf->planes[i].type = CONTIGUOUS_MEM;
		vpu_buf->planes[i].memid = memid;
		vpu_buf->planes[i].size = vb2_plane_size(vb, i);
	}

	return 0;
}

/* it would return how many plane it add */
static int vdpu_tile_assign_mtr_mvtp(struct syna_vcodec_ctx *ctx,
				     const struct v4g_fmt *vpu_fmt,
				     struct v4l2_pix_format_mplane *pix_mp,
				     struct vb2_buffer *vb)
{
	struct vb2_plane *mvtp_plane;
	u32 n_planes = vb->num_planes;
	u32 i, idx;
	size_t luma, chroma;

	/* When we sure it must be ref buf, we don't need vpu_fmt here */
	if (vpu_fmt && vpu_fmt->postprocessed)
		return 0;

	idx = vb->index;
	luma = pix_mp->plane_fmt[2].sizeimage;
	chroma = pix_mp->plane_fmt[3].sizeimage;

	/* mtr */
	for (i = 2; i < BERLIN_BUF_V4G_MVTP_PLANE_IDX; i++) {
		vb->planes[i].data_offset =
			((idx + (i - 2)) * luma) + (idx * chroma);
		vb->planes[i].bytesused = vb->planes[i].data_offset;
		vb->planes[i].bytesused += (i - 2) ? chroma : luma;
		/* FIXME */
		vb->planes[i].length = 1;
	}

	luma = pix_mp->plane_fmt[BERLIN_BUF_V4G_MVTP_PLANE_IDX].sizeimage;
	if (!luma)
		return 0;

	mvtp_plane = vdpu_alloc_interanl_mvtp_buf(ctx, luma, idx);
	if (n_planes < (BERLIN_BUF_V4G_MVTP_PLANE_IDX + 1)) {
		i = BERLIN_BUF_V4G_MVTP_PLANE_IDX;
		vb->num_planes = BERLIN_BUF_V4G_MVTP_PLANE_IDX;
	} else {
		i = n_planes;
	}

	if (mvtp_plane && mvtp_plane->mem_priv) {
		vb->planes[i] = *mvtp_plane;
		vb->num_planes++;
	} else {
		return -ENOMEM;
	}

	return (vb->num_planes - n_planes);
}

/*
 * called from _prepare_dmabuf() when reacquired == true
 * and __vb2_queue_alloc() for MMAP
 */
static int vb2ops_syna_vpu_buf_init(struct vb2_buffer *vb)
{
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct vb2_queue *vq = vb->vb2_queue;
	const struct v4g_fmt *vpu_fmt;
	u32 idx;
	enum syna_vpu_fw_dec_buf buftype;
	int ret;
	int n_planes;

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		buftype = SYNA_VPU_FW_VDEC_STRM;
		ret = vdpu_fill_es_buf(vb);
	} else {
		vpu_fmt = ctx->vpu_dst_fmt;
		if (vpu_fmt->postprocessed)
			buftype = SYNA_VPU_FW_VDEC_DISP;
		else
			buftype = SYNA_VPU_FW_VDEC_REF;

		n_planes =
		    vdpu_tile_assign_mtr_mvtp(ctx, vpu_fmt,
					      &ctx->ref_buf_fmt, vb);
		if (n_planes < 0)
			return n_planes;

		ret = vdpu_fill_video_buf(ctx, vb, vpu_fmt);

		vb->num_planes -= n_planes;
	}
	idx = vb->index;

	if (ret) {
		vdpu_err(vpu, "can't fill vpu buffer: 0x%08x\n", ret);
		return -EINVAL;
	}

	ret = syna_vpu_register_buf(ctx, buftype, idx);
	if (ret) {
		vdpu_err(vpu, "can't register buffer: 0x%08x\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int
v4g_frame_buf_plane_check(struct vb2_buffer *vb,
			  struct v4l2_pix_format_mplane *pixfmt)
{
	unsigned int sz;
	int i;

	for (i = 0; i < pixfmt->num_planes; i++) {
		sz = pixfmt->plane_fmt[i].sizeimage;
		if (vb2_plane_size(vb, i) < sz) {
			pr_err("syna_vpu: plane %d is too small\n", i);
			return -EINVAL;
		}
	}

	return 0;
}

static int vb2ops_vdec_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_pix_format_mplane *pix_fmt;
	int ret;

	if (test_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status))
		return 0;

	if (!V4L2_TYPE_IS_OUTPUT(vq->type)) {
		pix_fmt = &ctx->dst_fmt;
		ret = v4g_frame_buf_plane_check(vb, pix_fmt);
		if (ret)
			return ret;
	}

	return 0;
}

static void vdec_free_internal_refs(struct syna_vcodec_ctx *ctx)
{
	struct vb2_buffer *vb;
	u32 i, plane_no;

	for (i = 0; i < ctx->n_ref_pool; i++) {
		vb = &(ctx->ref_buffers[i]);
		for (plane_no = 0; plane_no < BERLIN_BUF_V4G_MVTP_PLANE_IDX + 1;
		     plane_no++) {
			if (vb->planes[plane_no].mem_priv) {
				vb2_syna_dh_bm_put(vb->planes[plane_no].mem_priv);
				vb->planes[plane_no].mem_priv = NULL;
			}
		}
	}
}

static void vdec_release_internal_ref_bufs(struct syna_vcodec_ctx *ctx)
{
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct syna_vpu_ctrl *ctrl;
	struct syna_vpu_tz_buf *vpu_buf;
	int ret;
	int i;

	if (!test_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status))
		return;

	if (!ctx->n_ref_pool)
		return;

	ctx->ref_slots = 0;
	ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);

	for (i = ctx->n_ref_pool - 1; i >= 0; i--) {
		vpu_buf = ctx->output_pool;
		vpu_buf += i;

		ret = syna_vpu_unregister_buf(ctx, SYNA_VPU_FW_VDEC_REF, i);
		if (ret)
			vdpu_err(vpu,
				 "can't unregister an ref buffer %d: 0x%08x\n",
				 i, ret);
	}

	vdec_free_internal_refs(ctx);
	ctx->n_ref_pool = 0;
	memset(&ctx->ref_buf_fmt, 0, sizeof(ctx->ref_buf_fmt));

	clear_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status);
}

/* It would happen before put memory */
static void vb2ops_syna_vpu_buf_cleanup(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct syna_vpu_dev *vpu = ctx->vpu;
	const struct v4g_fmt *vpu_fmt;
	struct syna_vpu_tz_buf *vpu_buf;
	enum syna_vpu_fw_dec_buf buftype;
	struct vb2_plane *mv_plane;
	u32 i;
	int ret;

	i = vb->index;

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		vpu_buf = ctx->input_pool;
		vpu_buf += i;
		vpu_fmt = ctx->vpu_src_fmt;
		buftype = SYNA_VPU_FW_VDEC_STRM;
	} else {
		vpu_fmt = ctx->vpu_dst_fmt;
		if (vpu_fmt->postprocessed)
			buftype = SYNA_VPU_FW_VDEC_DISP;
		else
			buftype = SYNA_VPU_FW_VDEC_REF;

		/* trigger internal ref bufs releasing here */
		if (ctx->n_ref_pool)
			vdec_release_internal_ref_bufs(ctx);
	}

	ret = syna_vpu_unregister_buf(ctx, buftype, i);
	if (ret)
		vdpu_err(vpu, "failed to unregister buffer: 0x%08x\n", ret);

	if (V4L2_TYPE_IS_OUTPUT(vq->type))
		return;
	if (vpu_fmt->postprocessed)
		return;

	mv_plane = &ctx->ref_buffers[i].planes[BERLIN_BUF_V4G_MVTP_PLANE_IDX];
	if (mv_plane->mem_priv) {
		vb2_syna_dh_bm_put(mv_plane->mem_priv);
		mv_plane->mem_priv = NULL;
	}
}

static int vdec_alloc_internal_refs(struct syna_vcodec_ctx *ctx)
{
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct v4l2_pix_format_mplane *pix_mp = &ctx->ref_fmt;
	struct v4l2_pix_format_mplane *alloc_pix_mp = &ctx->ref_buf_fmt;
	struct vb2_queue *dst_vq;
	struct vb2_buffer *vb;
	struct device *memdev = (IS_MMU_DISABLE) ? cma_dh_dev : sys_iommu_dev;
	u32 n_refs, start;
	u32 i, size;
	int ret;
	bool update_alloc;

	if (!pix_mp->plane_fmt[0].sizeimage)
		return -EINVAL;

	update_alloc = false;
	for (i = 0; i < pix_mp->num_planes; i++) {
		if (alloc_pix_mp->plane_fmt[i].sizeimage <
		    pix_mp->plane_fmt[i].sizeimage) {
			update_alloc = true;
			break;
		}
	}

	/* lazy re-allocation here */
	if ((ctx->n_req_ref_bufs < ctx->n_ref_pool) && !update_alloc) {
		syna_vpu_prepare_mtr_buffer(ctx, pix_mp);
		return ctx->n_ref_pool;
	}

	switch (ctx->src_fmt.pixelformat) {
	case V4L2_PIX_FMT_HEVC:
	case V4L2_PIX_FMT_H264:
		n_refs = min_t(u32, ctx->n_req_ref_bufs + 2, 16);
		break;
	case V4L2_PIX_FMT_VP8:
		n_refs = min_t(u32, ctx->n_req_ref_bufs, 4);
		break;
	case V4L2_PIX_FMT_VP9:
		n_refs = min_t(u32, ctx->n_req_ref_bufs + 1, 8);
		break;
	case V4L2_PIX_FMT_AV1:
		n_refs = min_t(u32, ctx->n_req_ref_bufs + 1, 10);
		break;
	default:
		n_refs = ctx->n_req_ref_bufs;
		break;
	}

	if (update_alloc) {
		vdec_release_internal_ref_bufs(ctx);
		*alloc_pix_mp = *pix_mp;
	}
	pix_mp = alloc_pix_mp;
	start = ctx->n_ref_pool;

	if (syna_vpu_prepare_mtr_buffer(ctx, pix_mp))
		return -EINVAL;

	dst_vq = v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx);
	for (i = start; i < n_refs && ctx->mtr_dh_dev; i++) {
		vb = &(ctx->ref_buffers[i]);

		vb->index = i;
		/* FIXME: assign a real queue for it */
		vb->vb2_queue = dst_vq;
		vb->num_planes = 0;
		/* assign the fake buf here */
		vb->planes[2].mem_priv =
		    syna_dh_bm_alloc(ctx->mtr_dh_dev,
				     pix_mp->plane_fmt[2].sizeimage);
		vb->planes[3].mem_priv =
		    syna_dh_bm_alloc(ctx->mtr_dh_dev,
				     pix_mp->plane_fmt[3].sizeimage);
	}

	for (i = start; i < n_refs; i++) {
		vb = &(ctx->ref_buffers[i]);

		size = pix_mp->plane_fmt[0].sizeimage;

		vb->planes[0].mem_priv = syna_dh_bm_alloc(memdev, size);
		if (IS_ERR_OR_NULL(vb->planes[0].mem_priv)) {
			ctx->n_ref_pool = i;
			ret = PTR_ERR(vb->planes[0].mem_priv);
			goto failed;
		}
		vb->planes[0].length = size;
		vb->num_planes++;

		size = pix_mp->plane_fmt[1].sizeimage;
		vb->planes[1].mem_priv = syna_dh_bm_alloc(memdev, size);
		if (IS_ERR_OR_NULL(vb->planes[1].mem_priv)) {
			ctx->n_ref_pool = i;
			ret = PTR_ERR(vb->planes[1].mem_priv);
			goto failed;
		}
		vb->planes[1].length = size;
		vb->num_planes++;
	}
	ctx->n_ref_pool = n_refs;

	for (i = start; i < n_refs; i++) {
		vb = &(ctx->ref_buffers[i]);
		vb->num_planes += 2;
		ret = vdpu_tile_assign_mtr_mvtp(ctx, NULL, pix_mp, vb);
		if (ret < 0) {
			vdpu_err(vpu,
				 "can't assign ref buffer %d extra planes: 0x%08x\n",
				 i, ret);
			goto failed;
		}
	}

	/* I don't want to unregister bufs when we meet allocation failure */
	for (i = start; i < n_refs; i++) {
		vb = &(ctx->ref_buffers[i]);

		ret = vdpu_fill_video_buf(ctx, vb, NULL);

		if (ret) {
			vdpu_err(vpu, "can't fill vpu buffer: 0x%08x\n", ret);
			goto failed;
		}

		ret = syna_vpu_register_buf(ctx, SYNA_VPU_FW_VDEC_REF, i);
		if (ret) {
			vdpu_err(vpu, "can't register ref buffer %d: 0x%08x\n",
				 i, ret);
			goto buf_reg_failed;
		}
	}

	return ctx->n_ref_pool;
failed:
	vdec_free_internal_refs(ctx);
	return ret;
buf_reg_failed:
	vdec_release_internal_ref_bufs(ctx);
	return ret;
}

static int vdec_capture_setup(struct syna_vcodec_ctx *ctx)
{
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	struct vb2_queue *dst_vq;
	int ret;

	if (test_bit(SYNA_VPU_DEC_POST_PROCESSING, &ctx->status)) {
		ret = vdec_alloc_internal_refs(ctx);
		if (ret <= 0) {
			vdpu_err(ctx->vpu,
				 "can't alloc internal ref bufs: %d\n", ret);
			return ret;
		}
	} else if (!test_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status)) {
		/* Display and Reference pool are the same pool */
		dst_vq = v4l2_m2m_get_dst_vq(m2m_ctx);
		ctx->n_ref_pool = vb2_get_num_buffers(dst_vq);

	}

	ret = syna_vdec_configure_stream(ctx);
	set_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status);
	if (ret) {
		vdpu_err(ctx->vpu, "config stream err: 0x%08x\n", ret);
		return ret;
	}

	return 0;
}

static int vb2ops_vdec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(q);
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	struct vb2_queue *src_vq, *dst_vq;
	int ret;

	v4l2_m2m_update_start_streaming_state(m2m_ctx, q);
	vdpu_dbg(ctx->vpu, 1, "[%p] start_streaming %s", m2m_ctx,
		 V4L2_TYPE_IS_OUTPUT(q->type) ? "output" : "capture");

	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		ctx->cur_src_buf = NULL;
		ctx->eos = false;

		dst_vq = v4l2_m2m_get_dst_vq(m2m_ctx);
		if (!vb2_start_streaming_called(dst_vq)) {
			ret = syna_vdec_configure_stream(ctx);
			set_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status);
			/**
			 * Allow decoder device_run with no new buffers
			 * queued. Let the hardware parse the stream data
			 * then report the information likes resolution.
			 */
			ignore_cap_streaming(m2m_ctx, true);
			return ret;
		}
	} else {
		ignore_cap_streaming(m2m_ctx, false);

		src_vq = v4l2_m2m_get_src_vq(m2m_ctx);
		if (!vb2_start_streaming_called(src_vq))
			return 0;
	}

	ret = vdec_capture_setup(ctx);
	if (ret)
		return ret;

	ctx->cap_resetup = false;
	clear_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status);
	v4l2_m2m_clear_state(m2m_ctx);

	ctx->sequence_out = 0;
	ctx->sequence_cap = 0;

	return 0;
}

static int vdpu_fw_close_stream(struct syna_vcodec_ctx *ctx)
{
	struct syna_vpu_ctrl *ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct syna_vdec_config *p = ctx->dec_params;
	int ret;

	down_write(&vpu->resource_rwsem);
	if (!test_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status)) {
		up_write(&vpu->resource_rwsem);
		return 0;
	}
	/* clear status */
	memset(&ctrl->status, 0, sizeof(ctrl->status));
	ret = syna_vdec_close_stream(ctx);
	clear_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status);
	up_write(&vpu->resource_rwsem);
	p->user_data = 0L;

	if (ret)
		goto bail;

	ret = syna_vdec_update_pop_status(ctx);

bail:
	return ret;
}

static void vdec_job_abort(void *priv)
{
	struct syna_vcodec_ctx *ctx = priv;

	wait_for_completion_killable(&ctx->work_done);
}

static void vb2ops_vdec_stop_streaming(struct vb2_queue *q)
{
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(q);
	struct syna_vcodec_ctx *next_ctx;
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	struct vb2_queue *src_vq, *dst_vq;
	struct vb2_v4l2_buffer *vb2_v4l2;
	int j, ret;

	vpu_srv_remove_from_pending(vpu->srv, ctx);

	vdpu_dbg(vpu, 1, "[%p] stop_streaming %s", m2m_ctx,
		 V4L2_TYPE_IS_OUTPUT(q->type) ? "output" : "capture");

	v4l2_m2m_update_stop_streaming_state(m2m_ctx, q);

	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		while ((vb2_v4l2 = v4l2_m2m_src_buf_remove(m2m_ctx))) {
			if (vb2_v4l2 != &ctx->eof_flush_buf.vb) {
				v4l2_m2m_buf_done(vb2_v4l2,
						  VB2_BUF_STATE_ERROR);
				vdpu_dbg(vpu, 5, "[%p] src done clear %d",
					 m2m_ctx, vb2_v4l2->vb2_buf.index);
			}
		}

		dst_vq = v4l2_m2m_get_dst_vq(m2m_ctx);
		if (!vb2_start_streaming_called(dst_vq))
			goto reset_inst;

		ret = syna_vdec_drain_pool(ctx, SYNA_VPU_FW_VDEC_STRM);
		if (ret) {
			vdpu_err(vpu, "failed to drain the OUTPUT queue %x",
				 ret);
			goto reset_inst;
		}
		ret = syna_vdec_update_pop_status(ctx);
		if (ret)
			goto reset_inst;

		ctx->cur_src_buf = NULL;

		return;
	} else {
		while ((vb2_v4l2 = v4l2_m2m_dst_buf_remove(m2m_ctx))) {
			for (j = 0; j < vb2_v4l2->vb2_buf.num_planes; j++)
				vb2_set_plane_payload(&vb2_v4l2->vb2_buf, j, 0);

			v4l2_m2m_buf_done(vb2_v4l2, VB2_BUF_STATE_ERROR);
			vdpu_dbg(vpu, 5, "[%p] dst done clear %d",
				 m2m_ctx, vb2_v4l2->vb2_buf.index);
		}

		src_vq = v4l2_m2m_get_src_vq(m2m_ctx);
		if (!vb2_start_streaming_called(src_vq))
			goto reset_inst;

		ret = 0;
		if (ctx->dst_vb_bits || ctx->ref_slots)
			ret = syna_vdec_drain_pool(ctx, SYNA_VPU_FW_VDEC_DISP);

		if (ret) {
			vdpu_err(vpu, "failed to drain the CAPTURE queue %x",
				 ret);
			goto reset_inst;
		}

		ret = syna_vdec_update_pop_status(ctx);
		if (ret)
			goto reset_inst;

		return;
	}

reset_inst:
	vpu_srv_release_out(ctx->vpu->srv, ctx, &next_ctx);

	if (next_ctx) {
		m2m_ctx = next_ctx->fh.m2m_ctx;
		v4l2_m2m_try_schedule(m2m_ctx);
	}
}

static void vb2ops_vdec_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct syna_vcodec_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	struct syna_vpu_tz_buf *vpu_buf;
	unsigned int i;

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		vpu_buf = ctx->input_pool;
		vpu_buf += vb->index;

		if (vb2_v4l2->flags & V4L2_BUF_FLAG_KEYFRAME)
			vpu_buf->flags = BERLIN_BUF_FLAG_KEYFRAME;

		for (i = 0; i < vb->num_planes; i++) {
			vpu_buf->planes[i].offset = vb->planes[i].data_offset;
			vpu_buf->bytesused[i] = vb2_get_plane_payload(vb, i);
		}
	} else {
		clear_bit(SYNA_VPU_STATUS_WAIT_DISP_BUF, &ctx->status);
		if (vb2_is_streaming(vb->vb2_queue) &&
		    v4l2_m2m_dst_buf_is_last(ctx->fh.m2m_ctx)) {
			syna_v4l2_m2m_last_buf_done(ctx->fh.m2m_ctx, vb2_v4l2);
			vdpu_dbg(ctx->vpu, 1, "[%p] buf_queue last buf done %d",
				 ctx->fh.m2m_ctx, vb->index);
			return;
		}
	}

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vb2_v4l2);
}

static const struct vb2_ops syan_vdec_vb2_ops = {
	.queue_setup = vb2ops_syna_vpu_queue_setup,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
	.buf_out_validate = vb2ops_vdec_buf_out_validate,
	.buf_init = vb2ops_syna_vpu_buf_init,
	.buf_prepare = vb2ops_vdec_buf_prepare,
	.buf_cleanup = vb2ops_syna_vpu_buf_cleanup,
	.start_streaming = vb2ops_vdec_start_streaming,
	.stop_streaming = vb2ops_vdec_stop_streaming,
	.buf_queue = vb2ops_vdec_buf_queue,
};

static void vdec_device_run(void *priv)
{
	struct syna_vcodec_ctx *ctx = priv;
	struct syna_vpu_dev *vpu = ctx->vpu;

	queue_work(vpu->vcodec_workq, &vpu->job_work);
}

static const struct v4l2_m2m_ops vpu_m2m_v4g_ops = {
	.device_run = vdec_device_run,
	.job_ready = vdec_job_ready,
	.job_abort = vdec_job_abort,
};

static void syna_record_free_ref_display_buf(struct syna_vcodec_ctx *ctx)
{
	struct syna_vpu_ctrl *ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	struct v4l2_m2m_buffer *buf, *n;
	u32 i, idx;

	if (test_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status)) {
		i = 0;
		v4l2_m2m_for_each_dst_buf_safe(m2m_ctx, buf, n) {
			idx = buf->vb.vb2_buf.index;
			if (test_bit(idx, &ctx->dst_vb_bits))
				continue;
			idx_queue_push(&ctrl->dbuf, i, idx);
			set_bit(idx, &ctx->dst_vb_bits);
			i++;
		}

		i = 0;
		for (idx = 0; idx < ctx->n_ref_pool; idx++) {
			if (test_bit(idx, &ctx->ref_slots))
				continue;
			idx_queue_push(&ctrl->rbuf, i, idx);
			i++;
			set_bit(idx, &ctx->ref_slots);
		}
	} else {
		i = 0;
		v4l2_m2m_for_each_dst_buf_safe(m2m_ctx, buf, n) {
			idx = buf->vb.vb2_buf.index;

			if (test_bit(idx, &ctx->ref_slots))
				continue;

			idx_queue_push(&ctrl->dbuf, i, idx);
			idx_queue_push(&ctrl->rbuf, i, idx);
			i++;

			set_bit(idx, &ctx->ref_slots);
			set_bit(idx, &ctx->dst_vb_bits);
		}
	}

	if (ctrl->dbuf.push) {
		ctrl->status.flags |= BERLIN_VPU_STATUS_PUSH_AUX_BUFS;
		/* Allow decoder device_run with no new buffers queued */
		if (!v4l2_m2m_num_dst_bufs_ready(m2m_ctx))
			ignore_cap_streaming(m2m_ctx, true);
	}

	if (ctrl->rbuf.push)
		ctrl->status.flags |= BERLIN_VPU_STATUS_PUSH_OUTPUT_BUFS;

	vdpu_dbg(ctx->vpu, 3, "[%p] dbuf.push %d, rbuf.push %d\n", m2m_ctx,
		 ctrl->dbuf.push, ctrl->rbuf.push);
}

/**
 * NOTE: __v4l2_m2m_try_queue() is in a spin lock, we can't sleep here.
 * So we can't call the TA which would lead a context switch, we just
 * check the last state here.
 */
static int vdec_job_ready(void *priv)
{
	struct syna_vcodec_ctx *ctx = priv;
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;

	if (test_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status)) {
		vdpu_dbg(ctx->vpu, 1, "[%p] wait new res setup\n", m2m_ctx);
		return 0;
	}

	if (!v4l2_m2m_num_src_bufs_ready(m2m_ctx)) {
		vdpu_dbg(ctx->vpu, 1, "[%p] lack of src ready\n", m2m_ctx);
		return 0;
	}

	if (v4l2_m2m_num_dst_bufs_ready(m2m_ctx) < (ctx->req_dpb_size) &&
		v4l2_m2m_next_src_buf(m2m_ctx) != &ctx->eof_flush_buf.vb) {
		vdpu_dbg(ctx->vpu, 4, "[%p] lack dst buffer num %d\n", m2m_ctx,
		       v4l2_m2m_num_dst_bufs_ready(m2m_ctx));
		return 0;
	}

	vdpu_dbg(ctx->vpu, 4, "[%p] ready buf s%d d %d flush %d\n", m2m_ctx,
		 v4l2_m2m_num_src_bufs_ready(m2m_ctx),
		 v4l2_m2m_num_dst_bufs_ready(m2m_ctx),
		 v4l2_m2m_next_src_buf(m2m_ctx) == &ctx->eof_flush_buf.vb);

	return !vpu_srv_push_to_pending(ctx->vpu->srv, ctx);
}

static void mark_empty_last(struct v4l2_m2m_ctx *m2m_ctx,
			    struct vb2_buffer *dst_vb)
{
	struct vb2_v4l2_buffer *dst_buf;
	u32 i;

	dst_buf = to_vb2_v4l2_buffer(dst_vb);
	for (i = 0; i < dst_vb->num_planes; i++)
		vb2_set_plane_payload(dst_vb, i, 0);

	syna_v4l2_m2m_last_buf_done(m2m_ctx, dst_buf);
}

static int syna_vdec_update_pop_status(struct syna_vcodec_ctx *ctx)
{
	struct syna_vpu_dev *vpu = ctx->vpu;
	struct syna_vpu_ctrl *ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;

	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	struct vb2_buffer *dst_vb;
	struct syna_vpu_tz_buf *vpu_buf;
	u32 idx, i, j;
	u32 mtr_flags;
	bool mark_last_buf;

	vdpu_dbg(vpu, 3, "[%p] dbuf.pop %d, sbuf.pop %d, rbuf.pop %d\n", m2m_ctx,
		 ctrl->dbuf.pop, ctrl->sbuf.pop, ctrl->rbuf.pop);

	for (i = 0; i < ctrl->sbuf.pop; i++) {
		idx = idx_queue_pop(&ctrl->sbuf, i);
		src_buf = v4l2_m2m_src_buf_remove_by_idx(m2m_ctx, idx);
		if (!src_buf) {
			vdpu_err(vpu, "[%p]%d is not in OUTPUT queue\n", m2m_ctx, idx);
			continue;
		}
		if (ctx->cur_src_buf == src_buf)
			ctx->cur_src_buf = NULL;

		if (src_buf != &ctx->eof_flush_buf.vb) {
			src_buf->sequence = ctx->sequence_out++;
			v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_DONE);
			vdpu_dbg(vpu, 5, "[%p] src done %d %d", m2m_ctx, idx, src_buf->sequence);
		} else {
			ctx->eos = true;
			vdpu_dbg(vpu, 5, "[%p] src flush back", m2m_ctx);
		}
	}

	mark_last_buf = false;
	for (i = 0; i < ctrl->dbuf.pop; i++) {
		idx = idx_queue_pop(&ctrl->dbuf, i);
		if (idx > VB2_MAX_FRAME) {
			vdpu_err(vpu, "[%p] buf %d is out of CAPTURE range\n", m2m_ctx, idx);
			goto out_of_range;
		}

		clear_bit(idx, &ctx->dst_vb_bits);
		dst_buf = v4l2_m2m_dst_buf_remove_by_idx(m2m_ctx, idx);
		if (!dst_buf) {
			vdpu_err(vpu, "[%p] buf %d is not in CAPTURE queue\n", m2m_ctx, idx);
			continue;
		}
		dst_vb = &dst_buf->vb2_buf;

		if (test_bit(SYNA_VPU_DEC_POST_PROCESSING, &ctx->status))
			/* postproc mode */
			vpu_buf = ctx->aux_pool;
		else
			/* tile mode */
			vpu_buf = ctx->output_pool;

		vpu_buf += dst_vb->index;

		if ((i == (ctrl->dbuf.pop - 1))
		    && (ctx->cap_resetup || ctx->eos))
			WRITE_ONCE(mark_last_buf, true);

		if (READ_ONCE(mark_last_buf)) {
			if ((vpu_buf->flags & BERLIN_BUF_FLAG_ERROR)
			    || !(vpu_buf->flags & BERLIN_BUF_FLAG_DONE)) {
				mark_empty_last(m2m_ctx, dst_vb);
				break;
			}
		}

		if (vpu_buf->flags & BERLIN_BUF_FLAG_ERROR) {
			v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
			vdpu_dbg(vpu, 5, "[%p] dst done err %d", m2m_ctx, idx);
			continue;
		}

		if (!(vpu_buf->flags & BERLIN_BUF_FLAG_DONE)) {
			if (ctx->eos || ctx->cap_resetup)
				v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
			else
				v4l2_m2m_buf_queue(m2m_ctx, dst_buf);
			vdpu_dbg(vpu, 5, "[%p] dst done %s %d", m2m_ctx,
				((ctx->eos || ctx->cap_resetup) ? "err done" : "reuse"), idx);
			continue;
		}

		/* handle normal buffer */
		for (j = 0; j < dst_vb->num_planes; j++)
			vb2_set_plane_payload(dst_vb, j, vpu_buf->bytesused[j]);

		dst_buf->sequence = ctx->sequence_cap++;
		dst_buf->vb2_buf.timestamp = vpu_buf->timestamp;
		dst_buf->field = vpu_buf->field;

		/**
		 * mtr_flags
		 * bit 0: MTR/compression enabled
		 * bit 1: mmu enabled
		 */
		mtr_flags = 0;
		if (ctrl->v4g_ext_cfg.mmu.en)
			mtr_flags |= 0x2;
		if (vpu_buf->flags & BERLIN_BUF_FLAG_COMPRESSED)
			mtr_flags |= 0x1;
		vb2_syna_fill_mtr_meta(dst_vb, 0, &vpu_buf->mtr_cfg0[0],
				       mtr_flags);
		vb2_syna_fill_mtr_meta(dst_vb, 1, &vpu_buf->mtr_cfg1[0],
				       mtr_flags);

		if (READ_ONCE(mark_last_buf))
			v4l2_m2m_last_buffer_done(m2m_ctx, dst_buf);
		else
			v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);
		vdpu_dbg(vpu, 5, "[%p] dst done %d %d", m2m_ctx, idx,
			 dst_buf->sequence);
	}

	/**
	 * those buffers are not used by hardware as the reference buffer
	 * now, we could reuse them if they are in the queue and their
	 * state are not done(not used for display or have not been enqueued
	 * from the userspace)
	 */
	for (i = 0; i < ctrl->rbuf.pop; i++)
		clear_bit(idx_queue_pop(&ctrl->rbuf, i), &ctx->ref_slots);

	/* clear pop status */
	ctrl->sbuf.pop = 0;
	ctrl->rbuf.pop = 0;
	ctrl->dbuf.pop = 0;
	return 0;

out_of_range:
	return -EINVAL;
}

static void syna_vdec_event_new_res(struct syna_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_src_ch = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION,
	};

	v4l2_event_queue_fh(&ctx->fh, &ev_src_ch);
}

static void syna_vdec_v4g_worker(struct work_struct *work)
{
	struct syna_vpu_dev *vpu =
	    container_of(work, struct syna_vpu_dev, job_work);
	struct v4l2_m2m_dev *m2m_dev = vpu->m2m_dev;
	struct syna_vcodec_ctx *ctx, *pending_ctx;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct vb2_v4l2_buffer *src_buf;
	struct syna_vpu_ctrl *ctrl;
	struct vb2_queue *dst_vq;
	u64 timestamp;
	bool switchpoint = false;
	int ret;

	ctx = v4l2_m2m_get_curr_priv(m2m_dev);

	ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	m2m_ctx = ctx->fh.m2m_ctx;

	vpu_srv_prepare_to_run(vpu->srv, ctx);

	if (vpu_srv_clear_pending_isr(vpu->srv))
		vdpu_err(vpu, "has an unhandled irq, HW may in a bad state\n");

	/* clear status */
	memset(&ctrl->status, 0, sizeof(ctrl->status));
	set_mask_bits(&ctx->status, SYNA_VPU_STATUS_WAIT_MASK, 0);

	src_buf = v4l2_m2m_next_src_buf(m2m_ctx);

	idx_queue_clear(&ctrl->sbuf);
	idx_queue_clear(&ctrl->rbuf);
	idx_queue_clear(&ctrl->dbuf);
	if (src_buf != ctx->cur_src_buf) {
		timestamp = src_buf->vb2_buf.timestamp;

		vdpu_es_mark_ts(ctx, src_buf->vb2_buf.index, timestamp);
		idx_queue_push(&ctrl->sbuf, 0, src_buf->vb2_buf.index);

		ctrl->status.flags |= BERLIN_VPU_STATUS_PUSH_INPUT_BUFS;
		ctx->cur_src_buf = src_buf;
	}

	syna_record_free_ref_display_buf(ctx);

	pm_runtime_get(vpu->dev);
decoding:
	ret = syna_vdec_decode_stream(ctx);

	/* TA dead or something else */
	if (ret) {
		v4l2_m2m_src_buf_remove_by_buf(m2m_ctx, src_buf);
		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
		vdpu_err(vpu, "[%p] src done err %d", m2m_ctx, src_buf->vb2_buf.index);
		goto bail;
	}

	if (ctrl->status.flags & BERLIN_VPU_STATUS_POP_BREAK) {
		ret = syna_vdec_update_pop_status(ctx);
		if (ret)
			goto bail;

		/* clear status */
		memset(&ctrl->status, 0, sizeof(ctrl->status));
		set_mask_bits(&ctx->status, SYNA_VPU_STATUS_WAIT_MASK, 0);

		idx_queue_clear(&ctrl->rbuf);
		idx_queue_clear(&ctrl->dbuf);

		syna_record_free_ref_display_buf(ctx);
		goto decoding;
	}

	if (ctrl->status.flags & BERLIN_VPU_STATUS_WAIT_INTERRUPTER) {
		ctrl->status.flags &= (~BERLIN_VPU_STATUS_WAIT_INTERRUPTER);
		ret = vpu_srv_wait_isr(vpu->srv, DEC_V4G_TIMEOUT_DELAY);
		if (ret) {
			ret = syna_vdec_hw_abort(ctx);
			/**
			 * NOTE: if it failed, keep the device power here
			 * then we could dump registers from the device.
			 */
			if (ret)
				goto bail;
		}
		goto decoding;
	}

	pm_runtime_put(vpu->dev);
	/**
	 * it could happen when the hardware thinks the reminded bitstream
	 * is not valid data.
	 */
	if (ctrl->status.flags & BERLIN_VPU_STATUS_DONE) {
		if (vpu_srv_has_pending_isr(vpu->srv))
			vdpu_err(vpu, "still has a irq not handled\n");
		WRITE_ONCE(switchpoint, true);
	}

	/* assume it is a switchpoint */
	if (ctrl->status.flags & BERLIN_VPU_STATUS_WAIT_INPUT_BUF) {
		set_bit(SYNA_VPU_STATUS_WAIT_INPUT_BUF, &ctx->status);

		if (ctrl->status.flags & BERLIN_VPU_STATUS_PROC_END_SAFE)
			WRITE_ONCE(switchpoint, true);
	}

	if (ret) {
		vdpu_err(vpu, "decoding fatal: 0x%x\n", ret);
		v4l2_m2m_src_buf_remove_by_buf(m2m_ctx, src_buf);
		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
		vdpu_err(vpu, "[%p] src done err %d", m2m_ctx, src_buf->vb2_buf.index);
		goto bail;
	}

	/* let this instance be in job queue when a new buffer queued */
	if (ctrl->status.flags & BERLIN_VPU_STATUS_WAIT_AUX_BUF) {
		set_bit(SYNA_VPU_STATUS_WAIT_DISP_BUF, &ctx->status);
		ignore_cap_streaming(m2m_ctx, false);
	}

	if (ctrl->status.flags & BERLIN_VPU_STATUS_WAIT_OUTPUT_BUF) {
		set_bit(SYNA_VPU_STATUS_WAIT_OUTPUT_BUF, &ctx->status);
		if (!test_bit(SYNA_VPU_POOL_ON_REFS, &ctx->status))
			ignore_cap_streaming(m2m_ctx, false);
	}

	/**
	 * NOTE: if let the next instance(can't be the same instance) start
	 * here, could be less device idle time;
	 */

	if (ctrl->status.flags & BERLIN_VPU_STATUS_NEW_SEQUENCE) {
		ignore_cap_streaming(m2m_ctx, false);
		set_bit(SYNA_VPU_STATUS_WAIT_NEW_RES_SETUP, &ctx->status);

		/**
		 * after STREAM_ON in capture clear the flag above, we need
		 * to wait display is ready.
		 */
		set_bit(SYNA_VPU_STATUS_WAIT_DISP_BUF, &ctx->status);
		dst_vq = v4l2_m2m_get_dst_vq(m2m_ctx);
		if (vb2_start_streaming_called(dst_vq))
			ctx->cap_resetup = true;

		vdec_fmt_from_cur_seq_fmt(ctx, ctrl, &ctx->ref_fmt);
		syna_vdec_event_new_res(ctx);
	}

	ret = syna_vdec_update_pop_status(ctx);

bail:
	if (READ_ONCE(switchpoint)) {
		pending_ctx = vpu_srv_schedule_pending(vpu->srv);
		if (pending_ctx) {
			m2m_ctx = pending_ctx->fh.m2m_ctx;
			/**
			 * v4l2_m2m_try_run() won't run until we call
			 * v4l2_m2m_job_finish(). We just need it be the
			 * very in m2m_dev->job_queue.
			 */
			v4l2_m2m_try_schedule(m2m_ctx);
			m2m_ctx = ctx->fh.m2m_ctx;
		}
	} else {
		vpu_srv_schedule_no_yield(vpu->srv);
	}

	v4l2_m2m_job_finish(vpu->m2m_dev, m2m_ctx);
	vdpu_dbg(vpu, 4, "[%p] finish switch %d\n", m2m_ctx, switchpoint);
	complete(&ctx->work_done);
}

static int
queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct syna_vcodec_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &syan_vdec_vb2_ops;
	src_vq->mem_ops = &syna_bm_dh_memops;
	src_vq->dma_attrs = DMA_ATTR_ALLOC_SINGLE_PAGES |
	    DMA_ATTR_NO_KERNEL_MAPPING;
	src_vq->buf_struct_size = 0;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->vpu->mutex;
	src_vq->dev = ctx->vpu->v4l2_dev.dev;
	set_v4l2_q_min_queued_buffers(src_vq, 1);

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &syan_vdec_vb2_ops;
	dst_vq->buf_struct_size = 0;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->vpu->mutex;
	dst_vq->dev = ctx->vpu->v4l2_dev.dev;
	dst_vq->bidirectional = true;
	set_v4l2_q_min_queued_buffers(dst_vq, 0);
	dst_vq->mem_ops = &syna_bm_dh_memops;
	dst_vq->dma_attrs = DMA_ATTR_ALLOC_SINGLE_PAGES |
	    DMA_ATTR_NO_KERNEL_MAPPING;

	return vb2_queue_init(dst_vq);
}

/* v4l2_file_operations */
static int vdpu_driver_open(struct file *filp)
{
	struct syna_vpu_auxiliary_device *auxdev = NULL;
	struct syna_vpu_dev *vpu = NULL;
	struct syna_vcodec_ctx *ctx = NULL;
	struct syna_vpu_ctrl *ctrl = NULL;
	struct syna_vpu_fw_info *fw_info = NULL;
	struct vb2_queue *src_vq = NULL;
	size_t ctrl_buf_size, cfg_buf_size, ctx_buf_size;
	dma_addr_t *memid;
	int ret;

	vpu = video_drvdata(filp);

	auxdev = container_of(vpu->dev, struct syna_vpu_auxiliary_device, dev);
	ret = syna_mfd_request_connect(auxdev);
	if (ret)
		return ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctx->service_link);
	fw_info = &vpu->fw_data.fw_info;
	ctx->vpu = vpu;

	cfg_buf_size = ALIGN(sizeof(struct syna_vdec_config), PAGE_SIZE);
	ret = syna_vpu_tee_alloc(vpu->tee_ctx, cfg_buf_size, (void **)&ctx->cfg_shm);
	if (ret || !syna_vpu_get_cfg_shm_buffer(ctx)) {
		vdpu_err(vpu, "can't allocate cfg shm\n");
		ret = -ENOMEM;
		goto failed;
	}
	ctx->dec_params = syna_vpu_get_cfg_shm_buffer(ctx);
	memset(ctx->dec_params, 0, sizeof(struct syna_vdec_config));

	ctrl_buf_size = ALIGN(sizeof(struct syna_vpu_ctrl), SZ_4K);
	ret = syna_vpu_tee_alloc(vpu->tee_ctx, ctrl_buf_size, (void **)&ctx->ctrl_shm);
	if (ret || !syna_vpu_get_ctrl_shm_buffer(ctx)) {
		vdpu_err(vpu, "can't allocate instance shm\n");
		ret = -ENOMEM;
		goto failed_shm;
	}
	ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);

	ctx_buf_size = ALIGN(BERLIN_VPU_CTX_RESERVED_SIZE, PAGE_SIZE) +
	    ALIGN(fw_info->vdec_strm_context_size, PAGE_SIZE) +
	    ALIGN(fw_info->vdec_strm_hw_context_size, PAGE_SIZE) +
	    fw_info->vdec_pool_size;

	ctx_buf_size = ALIGN(ctx_buf_size, PAGE_SIZE);

	/* NOTE: can move to secure */
	ctx->ctx_buf = syna_dh_bm_alloc(cma_dh_dev, ctx_buf_size);
	if (!ctx->ctx_buf) {
		vdpu_err(vpu, "can't allocate secure instance context\n");
		ret = -ENOMEM;
		goto failed_sec_ctx;
	}
	memid = vb2_syna_bm_cookie(NULL, ctx->ctx_buf);

	ctrl->ctrl_buf.addr = syna_vpu_get_ctrl_shm_paddr(ctx);
	ctrl->ctrl_buf.size = ctrl_buf_size;

	ctrl->ctx_buf.type = CONTIGUOUS_MEM;
	ctrl->ctx_buf.memid = (uintptr_t) (*memid);
	ctrl->ctx_buf.offset = 0;
	ctrl->ctx_buf.size = ctx_buf_size;

	ret = syna_vdec_create_instance(ctx);
	if (ret) {
		vdpu_err(vpu, "Failed to create instance(%d)", ret);
		goto fw_inst_failed;
	}

	ctx->input_pool = &ctrl->inbufs;
	ctx->output_pool = &ctrl->outbufs;
	ctx->aux_pool = &ctrl->auxbufs;

	init_completion(&ctx->work_done);

	v4l2_fh_init(&ctx->fh, video_devdata(filp));
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(vpu->m2m_dev, ctx, queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_ctx_free;
	}

	filp->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	src_vq = v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx);
	ctx->eof_flush_buf.vb.vb2_buf.vb2_queue = src_vq;
	ctx->eof_flush_buf.vb.vb2_buf.index = 31;

	ret = syna_vdec_ctrls_setup(ctx);
	if (ret) {
		vdpu_err(vpu, "Failed to setup controls(%d)", ret);
		goto err_fh_free;
	}
	ctx->fh.ctrl_handler = &ctx->ctrl_handler;

	ctx->vpu_src_fmt = v4g_get_default_fmt(ctx, false);
	/* always on for v2g */
	ctx->dec_params->tile_ref_flag = 1;

	ctx->dec_params->frame_in_mode = 1;
	/* default settings */
	ctx->dec_params->no_reordering = 0;
	ctx->dec_params->pp_hscale = 1;
	ctx->dec_params->pp_vscale = 1;
	ctx->dec_params->dblk_mode = 1;
	ctx->dec_params->disable_gmc23 = 1;
	ctx->dec_params->error_pic_threshold = 10;

	/* tile format */
	ctx->dec_params->mtuTile = 6;
	ctx->dec_params->mtuDisp = 6;

	return 0;

err_fh_free:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
err_ctx_free:
	syna_vdec_destroy_instance(ctx);
fw_inst_failed:
	vb2_syna_dh_bm_put(ctx->ctx_buf);
failed_sec_ctx:
	syna_vpu_tee_release(ctx->ctrl_shm);
failed_shm:
	syna_vpu_tee_release(ctx->cfg_shm);
failed:
	kfree(ctx);
	syna_mfd_disconnect(auxdev);
	return ret;
}

static int vdpu_driver_release(struct file *filp)
{
	struct syna_vpu_auxiliary_device *auxdev;
	struct syna_vcodec_ctx *ctx = fh_to_ctx(filp->private_data);
	struct syna_vpu_dev *vpu = ctx->vpu;
	int ret;

	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);

	syna_vdec_destroy_instance(ctx);
	if (ctx->mtr_dh_dev) {
		syna_vpu_dh_free_mtr_memdev(ctx->mtr_dh_dev);
		ctx->mtr_dh_dev = NULL;
	}

	vb2_syna_dh_bm_put(ctx->ctx_buf);
	syna_vpu_tee_release(ctx->ctrl_shm);
	syna_vpu_tee_release(ctx->cfg_shm);

	auxdev = container_of(vpu->dev, struct syna_vpu_auxiliary_device, dev);
	ret = syna_mfd_disconnect(auxdev);

	kfree(ctx);

	return ret;
}

static const struct v4l2_file_operations syna_vdec_fops = {
	.owner = THIS_MODULE,
	.open = vdpu_driver_open,
	.release = vdpu_driver_release,
	.poll = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
};

static const struct syna_vpu_fw_ops vdpu_fw_ops = {
	.fw_inst_swap = syna_vpu_ctx_switch_inst,
	.release = vdpu_fw_close_stream,
	.switch_in = syna_vdec_stream_switch_in,
	.switch_out = syna_vdec_stream_switch_out,
};

int syna_vdpu_v4g_init(struct syna_vpu_dev *vpu)
{
	struct video_device *vfd;
	int ret;

	vpu->fw_ops = &vdpu_fw_ops;
	INIT_WORK(&vpu->job_work, syna_vdec_v4g_worker);
	mutex_init(&vpu->mutex);

	snprintf(vpu->v4l2_dev.name, sizeof(vpu->v4l2_dev.name), "%s",
		 "[SYNA_V4L2_V4G_VDEC]");
	ret = v4l2_device_register(vpu->dev, &vpu->v4l2_dev);
	if (ret) {
		dev_err(vpu->dev, "Failed to register v4l2 device\n");
		return ret;
	}

	/* We just use the buffer ready queue */
	vpu->m2m_dev = v4l2_m2m_init(&vpu_m2m_v4g_ops);
	if (IS_ERR((__force void *)vpu->m2m_dev)) {
		vdpu_err(vpu, "Failed to init m2m device\n");
		ret = PTR_ERR((__force void *)vpu->m2m_dev);
		goto failed_m2m_init;
	}

	vfd = video_device_alloc();
	if (!vfd) {
		vdpu_err(vpu, "Failed to allocate video device\n");
		goto failed_vfd_alloc;
	}

	strscpy(vfd->name, SYNA_V4G_DEC_NAME, sizeof(vfd->name));
	vfd->fops = &syna_vdec_fops;
	vfd->release = video_device_release;
	vfd->lock = &vpu->mutex;
	vfd->v4l2_dev = &vpu->v4l2_dev;
	vfd->vfl_dir = VFL_DIR_M2M;
	vfd->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_M2M_MPLANE;
	vfd->ioctl_ops = &syna_vdec_ioctl_ops;

	ret = video_register_device(vfd, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto failed_vdec_release;

	vpu->vfd = vfd;
	video_set_drvdata(vfd, vpu);

	return 0;

failed_vdec_release:
	video_device_release(vfd);
failed_vfd_alloc:
	v4l2_m2m_release(vpu->m2m_dev);
failed_m2m_init:
	v4l2_device_unregister(&vpu->v4l2_dev);
	return ret;
}

int syna_vdpu_v4g_finalize(struct syna_vpu_dev *vpu)
{
	if (vpu->m2m_dev)
		v4l2_m2m_release(vpu->m2m_dev);

	if (vpu->vfd)
		video_unregister_device(vpu->vfd);

	v4l2_device_unregister(&vpu->v4l2_dev);

	return 0;
}
