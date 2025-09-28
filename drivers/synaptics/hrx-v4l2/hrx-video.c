// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#include "hrx_kernel_compatibility_wrap.h"
#include "hrx-video.h"
#include "hrx-vip.h"
#include "hrx-vip-isr.h"
#include "hrx-dmaheap.h"

#define MAX_FRAME_SIZE 1

struct syna_hrx_dma_buffer {
	struct	vb2_v4l2_buffer buf;
	struct	list_head queue;
	struct	syna_hrx_v4l2_dev *hrx_dev;
};

#define to_syna_hrx_dma_buffer(vb)	container_of(vb, struct syna_hrx_dma_buffer, buf)

static atomic_t hrx_dev_refcnt = ATOMIC_INIT(0);

#ifdef STEPWISE_SUPPORT
static const struct v4l2_frmsize_stepwise syna_hrx_video_framesizes = {
	.min_width = 64,
	.max_width = 4096,
	.step_width = 8,
	.min_height = 64,
	.max_height = 4096,
	.step_height = 8,
};
#endif
static const struct v4l2_frmsize_discrete syna_hrx_video_dis_framesizes = {
	.width = 1920,
	.height = 1080,
};
static const struct syna_hrx_vid_fmt syna_hrx_video_formats[] = {
	{
		.name = "YCC422",
		.description = "YCC 422 (24 bpp)",
		.fourcc = V4L2_PIX_FMT_UYVY,
		.fps = 30,
		.dis_frmsize = {
			.width = 1920,
			.height = 1080,
		},
	}, {
	.name = "NV12",
	.description = "NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
		.fps = 30,
		.dis_frmsize = {
			.width = 1920,
			.height = 1080,
		},
	}
};
static const struct syna_hrx_vid_fmt *syna_hrx_vid_find_format(u32 fourcc, u64 modifier,
			  const struct syna_hrx_vid_fmt *formats,
			  size_t num_formats)
{
	const struct syna_hrx_vid_fmt *fmt;
	unsigned int i;

	for (i = 0; i < num_formats; i++) {
		fmt = &formats[i];
		if (fourcc == fmt->fourcc)
			return fmt;
	}

	return NULL;
}


static int vidioc_hrx_enum_framesizes(struct file *file, void *fh,
		  struct v4l2_frmsizeenum *fsize)
{
	const struct syna_hrx_vid_fmt *fmt;

	if (fsize->index >= MAX_FRAME_SIZE)
		return -EINVAL;

	fmt = syna_hrx_vid_find_format(fsize->pixel_format, 0,
		   syna_hrx_video_formats,
		   ARRAY_SIZE(syna_hrx_video_formats));
	if (!fmt)
		return -EINVAL;

#ifdef STEPWISE_SUPPORT
	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	if (fmt->frmsize.max_width > 0)
		fsize->stepwise = fmt->frmsize;
	else
		fsize->stepwise = syna_hrx_video_framesizes;
#else
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete = fmt->dis_frmsize;
#endif
	return 0;
}

static int vidioc_hrx_enum_frameintervals(struct file *file, void *fh,
			  struct v4l2_frmivalenum *f)
{
	const struct syna_hrx_vid_fmt *fmt;

	if (f->index >= ARRAY_SIZE(syna_hrx_video_formats))
		return -EINVAL;

	fmt = syna_hrx_vid_find_format(f->pixel_format, 0,
		   syna_hrx_video_formats,
		   ARRAY_SIZE(syna_hrx_video_formats));
	if (!fmt)
		return -EINVAL;

#ifdef STEPWISE_SUPPORT
	if (fmt->frmsize.max_width > 0) {
		if (f->width < fmt->frmsize.min_width ||
			f->width > fmt->frmsize.max_width ||
			f->height < fmt->frmsize.min_height ||
			f->height > fmt->frmsize.max_height)
			return -EINVAL;
	}

	f->type = V4L2_FRMIVAL_TYPE_CONTINUOUS;
	f->stepwise.min.numerator = 1;
	f->stepwise.min.denominator = 200;
	f->stepwise.max.numerator = 1;
	f->stepwise.max.denominator = 1;
	f->stepwise.step.numerator = 1;
	f->stepwise.step.denominator = 1;
#else
	f->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	f->discrete.numerator = 1;
	f->discrete.denominator = fmt->fps;
#endif

	return 0;
}
static int vidioc_hrx_enum_fmt(struct v4l2_fmtdesc *f,
		   const struct syna_hrx_vid_fmt *formats,
		   size_t num_formats, bool capture)
{
	const struct syna_hrx_vid_fmt *fmt;
	int i;

	if (f->index >= num_formats)
		return -EINVAL;

	for (i = 0; i < num_formats; i++) {
		if (i == f->index) {
			fmt = &formats[i];
			f->pixelformat = fmt->fourcc;
			return 0;
		}
	}
	return -EINVAL;
}

static int vidioc_hrx_enum_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_fmtdesc *f)
{
	return vidioc_hrx_enum_fmt(f, syna_hrx_video_formats,
		   ARRAY_SIZE(syna_hrx_video_formats), false);
}

static struct vb2_v4l2_buffer *syna_hrx_dma_buffer_remove(struct syna_hrx_v4l2_dev *hrx_dev)
{
	struct syna_hrx_dma_buffer *b;
	unsigned long irq_flags;

	spin_lock_irqsave(&hrx_dev->queued_lock, irq_flags);
	if (list_empty(&hrx_dev->queued_bufs)) {
		spin_unlock_irqrestore(&hrx_dev->queued_lock, irq_flags);
		return NULL;
	}
	b = list_first_entry(&hrx_dev->queued_bufs, struct syna_hrx_dma_buffer, queue);
	list_del(&b->queue);
	spin_unlock_irqrestore(&hrx_dev->queued_lock, irq_flags);
	return &b->buf;
}

struct vb2_buffer *syna_hrx_get_free_buf(struct syna_hrx_v4l2_dev *hrx_dev)
{
	struct vb2_v4l2_buffer *pbuf;

	pbuf = syna_hrx_dma_buffer_remove(hrx_dev);
	if (pbuf != NULL) {
		return &(pbuf->vb2_buf);
	} else {
		/* Use reserved buffer for hrx, in case sink element is holding rest of the buffers */
		hrx_dev->frame_drop_count++;
		return hrx_dev->reserved_buf;
	}
}

void syna_hrx_buf_processed(struct syna_hrx_v4l2_dev *hrx_dev, struct vb2_buffer *buf)
{
	void *pBuffer = (void *)buf;
	if((hrx_dev->display_frame_index == 0) || ((hrx_dev->frame_count % hrx_dev->display_frame_index) == 0))
	{
		/* Don't submit the reserved buffer */
		if(hrx_dev->reserved_buf != buf) {
			if (!kfifo_is_full(&hrx_dev->processed_buffer_queue))
			{
				kfifo_in(&hrx_dev->processed_buffer_queue, &pBuffer, sizeof(void *));
				up(&(hrx_dev->process_buffer_sem));
			}
			else
				syna_hrx_buf_unused(hrx_dev, buf);
		}
	}
	else
	{
		syna_hrx_buf_unused(hrx_dev, buf);
	}
	hrx_dev->frame_count += 1;
}

static int vb2ops_syna_hrx_queue_setup(struct vb2_queue *vq,
			   unsigned int *nbuffers,
			   unsigned int *nplanes,
			   unsigned int sizes[],
			   struct device *alloc_devs[])
{
	struct syna_hrx_v4l2_dev *hrx_dev = vb2_get_drv_priv(vq);

	*nplanes = 1;
	sizes[0] = hrx_dev->format.plane_fmt[0].sizeimage;
	alloc_devs[0] = hrx_dev->alloc_dev;

	return 0;
}

static int vb2ops_hrx_buf_prepare(struct vb2_buffer *vb)
{
	struct syna_hrx_v4l2_dev *hrx_dev = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct syna_hrx_dma_buffer *buf = to_syna_hrx_dma_buffer(vbuf);
	unsigned long size = hrx_dev->format.plane_fmt[0].sizeimage;

	if (vb2_plane_size(vb, 0) < size) {
		HRX_LOG(HRX_DRV_ERROR, "%s: buffer too small (%lu < %lu)\n", __func__, vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	buf->hrx_dev = hrx_dev;
	vb2_set_plane_payload(vb, 0, size);

	return 0;
}

static int vb2ops_hrx_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct syna_hrx_v4l2_dev *hrx_dev = vb2_get_drv_priv(q);

	/* block reserve buffer */
	hrx_dev->frame_drop_count = 0;
	hrx_dev->reserved_buf = syna_hrx_get_free_buf(hrx_dev);

	switch (hrx_dev->pix_fmt.pixelformat) {
	case V4L2_PIX_FMT_UYVY:
		hrx_dev->vip_omode = VIP_OMODE0_8BIT_YC;
		break;
	case V4L2_PIX_FMT_NV12:
	default:
		hrx_dev->vip_omode = VIP_OMODE2_8BIT_YUV420;
		break;
	}
	wake_up_interruptible(&hrx_dev->vblank_wq);
	hrx_dev->hrx_v4l2_state = HRX_V4L2_STREAMING_ON;

	if ((hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) && (hrx_dev->HrxState == HRX_STATE_ALL_STABLE))
		vip_start(hrx_dev);
	vip_create_watcher_task(hrx_dev);

	return 0;
}

static void vb2ops_hrx_stop_streaming(struct vb2_queue *q)
{
	struct syna_hrx_v4l2_dev *hrx_dev = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *buf;

	hrx_dev->hrx_v4l2_state = HRX_V4L2_STREAMING_OFF;

	vip_stop_watcher_task(hrx_dev);
	vip_stop(hrx_dev);

	while ((buf = syna_hrx_dma_buffer_remove(hrx_dev)))
		vb2_buffer_done(&buf->vb2_buf,  VB2_BUF_STATE_ERROR);

	if (hrx_dev->reserved_buf)
		vb2_buffer_done(hrx_dev->reserved_buf, VB2_BUF_STATE_ERROR);
	hrx_dev->reserved_buf = NULL;
	hrx_dev->frame_count = 0;
	hrx_dev->frame_drop_count = 0;

	if (hrx_dev->dum_thread)
		kthread_stop(hrx_dev->dum_thread);
}

static void vb2ops_hrx_buf_queue(struct vb2_buffer *vb)
{
	struct syna_hrx_v4l2_dev *hrx_dev = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct syna_hrx_dma_buffer *buf = to_syna_hrx_dma_buffer(vbuf);
	unsigned long irq_flags;

	spin_lock_irqsave(&hrx_dev->queued_lock, irq_flags);
	list_add_tail(&buf->queue, &hrx_dev->queued_bufs);
	spin_unlock_irqrestore(&hrx_dev->queued_lock, irq_flags);
}

void syna_hrx_buf_unused(struct syna_hrx_v4l2_dev *hrx_dev, struct vb2_buffer *buf)
{
	/* Don't queue reserved buffer */
	if(hrx_dev->reserved_buf != buf) {
		vb2ops_hrx_buf_queue(buf);
	}
}

static const struct vb2_ops syna_hrx_vb2_ops = {
	.queue_setup = vb2ops_syna_hrx_queue_setup,
	.buf_prepare = vb2ops_hrx_buf_prepare,
	.buf_queue = vb2ops_hrx_buf_queue,
	.start_streaming = vb2ops_hrx_start_streaming,
	.stop_streaming = vb2ops_hrx_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int queue_init(void *priv)
{
	int ret;
	struct syna_hrx_v4l2_dev *hrx_dev = priv;
	struct vb2_queue *dst_vq = &hrx_dev->dst_vq;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = hrx_dev;
	dst_vq->ops = &syna_hrx_vb2_ops;
	dst_vq->buf_struct_size = sizeof(struct syna_hrx_dma_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &hrx_dev->mutex;
	dst_vq->dev = hrx_dev->v4l2_dev.dev;
	set_v4l2_q_min_queued_buffers(dst_vq, SYNA_HRX_MIN_BUFFERS);
	dst_vq->mem_ops = &syna_hrx_dma_contig_memops;
	dst_vq->dma_attrs = DMA_ATTR_ALLOC_SINGLE_PAGES |
		DMA_ATTR_NO_KERNEL_MAPPING;

	ret = vb2_queue_init(dst_vq);
	return ret;
}

/* v4l2_file_operations */
static int hrx_driver_open(struct file *filp)
{
	int ret = 0;
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(filp);

	HRX_LOG(HRX_DRV_DEBUG, "Open\n");
	init_waitqueue_head(&hrx_dev->vblank_wq);
	ret = v4l2_fh_open(filp);

	if (atomic_inc_return(&hrx_dev_refcnt) > 1) {
		HRX_LOG(HRX_DRV_ERROR, "Allowed single instance, hrx reference count %d!\n",
				atomic_read(&hrx_dev_refcnt));
		return 0;
	}
	mutex_init(&hrx_dev->mutex);
	return ret;
}

static int hrx_driver_release(struct file *filp)
{
	int ret;
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(filp);

	HRX_LOG(HRX_DRV_DEBUG, "Release\n");

	if (atomic_read(&hrx_dev_refcnt) == 0) {
		HRX_LOG(HRX_DRV_INFO, "hrx driver is already released!\n");
		return 0;
	}

	if (atomic_dec_return(&hrx_dev_refcnt)) {
		HRX_LOG(HRX_DRV_INFO, "hrx dev ref cnt after this release: %d!\n",
				atomic_read(&hrx_dev_refcnt));
		return 0;
	}

	ret = vb2_fop_release(filp);
	mutex_destroy(&hrx_dev->mutex);
	return ret;
}

static int vidioc_hrx_querycap(struct file *file, void *priv,
		struct v4l2_capability *cap)
{
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(file);

	cap->capabilities = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_CAPTURE;

	strscpy(cap->driver, SYNA_HRX_V4L2_NAME, sizeof(cap->driver));
	strscpy(cap->card, SYNA_HRX_STR, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
			"platform: %s", hrx_dev->v4l2_dev.name);

	return 0;
}

static int syna_hrx_subscribe_event(struct v4l2_fh *fh,
			const struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		return v4l2_ctrl_subscribe_event(fh, sub);
	}
}

static const struct syna_hrx_vid_fmt *syna_hrx_video_get_format_by_fourcc(
		struct syna_hrx_v4l2_dev *hrx_dev, u32 fourcc)
{
	const struct syna_hrx_vid_fmt *format;
	int i, size;

	format = &syna_hrx_video_formats[0];
	size = ARRAY_SIZE(syna_hrx_video_formats);

	for (i = 0; i < size; i++) {
		if (format->fourcc == fourcc)
			return format;
		format++;
	}

	return ERR_PTR(-EINVAL);
}

static int syna_hrx_video_fill_fmt(struct syna_hrx_v4l2_dev *hrx_dev,
		struct v4l2_pix_format_mplane *pix_fmt,
		const struct syna_hrx_vid_fmt **fmtinfo)
{
	const struct syna_hrx_vid_fmt *info;

	info = syna_hrx_video_get_format_by_fourcc(hrx_dev, pix_fmt->pixelformat);
	if (IS_ERR(info))
		return -1;

	if (fmtinfo)
		*fmtinfo = info;

	hrx_dev->pix_fmt.pixelformat = pix_fmt->pixelformat = info->fourcc;

	v4l2_fill_pixfmt_mp(pix_fmt, pix_fmt->pixelformat, ALIGN(pix_fmt->width, 8), ALIGN(pix_fmt->height, 8));

	return 0;
}

static int vidioc_hrx_s_fmt_cap_mplane(struct file *file, void *priv, struct v4l2_format *f)
{
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(file);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct vb2_queue *vq;
	const struct syna_hrx_vid_fmt *fmt;
	int ret;

	vq =  &hrx_dev->dst_vq;

	if (vb2_is_busy(vq)) {
		HRX_LOG(HRX_DRV_ERROR, "queue busy");
		return -EBUSY;
	}

	ret = syna_hrx_video_fill_fmt(hrx_dev, pix_mp, &fmt);
	if (ret)
		return ret;

	hrx_dev->format = *pix_mp;
	hrx_dev->fmtinfo = fmt;

	return 0;
}

static int vidioc_hrx_g_fmt_vid_cap_mplane(struct file *file, void *priv,
		 struct v4l2_format *f)
{
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(file);

	f->fmt.pix_mp = hrx_dev->format;
	return 0;
}

static int vb2_syna_hrx_ioctl_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(file);

	return vb2_querybuf(hrx_dev->vdev->queue, p);
}

static int vidioc_hrx_try_fmt_vid_cap_mplane(struct file *file, void *priv,
			 struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	pix_mp->flags = 0;
	pix_mp->num_planes = 1;

	pix_mp->plane_fmt[0].sizeimage =
		max_t(unsigned int, SZ_1M, pix_mp->plane_fmt[0].sizeimage);
	pix_mp->plane_fmt[0].sizeimage =
		ALIGN(pix_mp->plane_fmt[0].sizeimage, SZ_4K);

	return 0;
}

static int vidioc_hrx_get_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *sp)
{
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(file);
	struct v4l2_captureparm *cp = &sp->parm.capture;

	cp->capability = V4L2_CAP_TIMEPERFRAME;

	//update v4l2 frameinterval structure
	cp->timeperframe.numerator = hrx_dev->video_params.FIActive.numerator;
	cp->timeperframe.denominator = hrx_dev->video_params.FIActive.denominator;

	return 0;
}

static int vidioc_hrx_set_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *sp)
{
	struct syna_hrx_v4l2_dev *hrx_dev = video_drvdata(file);
	struct v4l2_captureparm *cp = &sp->parm.capture;

	// update requested frameinterval structure
	hrx_dev->video_params.FIRequested.numerator = cp->timeperframe.numerator;
	hrx_dev->video_params.FIRequested.denominator = cp->timeperframe.denominator;

	/* update active fps w.r.t. requested fps */
	hrx_update_active_frame_interval(hrx_dev);

	cp->timeperframe.numerator = hrx_dev->video_params.FIActive.numerator;
	cp->timeperframe.denominator = hrx_dev->video_params.FIActive.denominator;

	return 0;
}


static const struct v4l2_file_operations syna_hrx_fops = {
	.owner = THIS_MODULE,
	.open = hrx_driver_open,
	.release = hrx_driver_release,
	.poll = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
};

const struct v4l2_ioctl_ops syna_hrx_ioctl_ops = {
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,

	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_syna_hrx_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,

	.vidioc_querycap = vidioc_hrx_querycap,
	/* image control */
	/* .vidioc_cropcap = vidioc_hrx_cropcap, */
	.vidioc_enum_fmt_vid_cap = vidioc_hrx_enum_fmt_vid_cap,

	.vidioc_enum_framesizes = vidioc_hrx_enum_framesizes,
	.vidioc_enum_frameintervals = vidioc_hrx_enum_frameintervals,

	.vidioc_try_fmt_vid_cap_mplane = vidioc_hrx_try_fmt_vid_cap_mplane,

	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_subscribe_event = syna_hrx_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,

	.vidioc_s_fmt_vid_cap_mplane = vidioc_hrx_s_fmt_cap_mplane,

	.vidioc_g_fmt_vid_cap_mplane = vidioc_hrx_g_fmt_vid_cap_mplane,

	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,

	.vidioc_s_parm = vidioc_hrx_set_parm,
	.vidioc_g_parm = vidioc_hrx_get_parm,
	/*
	 *.vidioc_decoder_cmd = vidioc_decoder_cmd,
	 *.vidioc_try_decoder_cmd = v4l2_m2m_ioctl_try_decoder_cmd,
	 */
};

int syna_hrx_video_init(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int ret = -1;
	struct video_device *vdev;

	INIT_LIST_HEAD(&hrx_dev->queued_bufs);
	spin_lock_init(&hrx_dev->queued_lock);

	snprintf(hrx_dev->v4l2_dev.name, sizeof(hrx_dev->v4l2_dev.name), "syna-hrx");
	ret = v4l2_device_register(hrx_dev->dev, &hrx_dev->v4l2_dev);
	if (ret < 0) {
		HRX_LOG(HRX_DRV_ERROR, "[%s] Register v4l2 device fail", __func__);
		goto err;
	}
	if (ret)
		goto err;

	vdev = video_device_alloc();
	if (!vdev) {
		HRX_LOG(HRX_DRV_ERROR, "Failed to allocate video device\n");
		goto err;
	}

	strscpy(vdev->name, SYNA_HRX_V4L2_NAME, sizeof(vdev->name));
	vdev->fops = &syna_hrx_fops;
	vdev->queue = &hrx_dev->dst_vq;
	vdev->release = video_device_release;
	vdev->lock = NULL;
	vdev->v4l2_dev = &hrx_dev->v4l2_dev;
	vdev->vfl_dir = VFL_DIR_RX;
	vdev->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_CAPTURE;
	vdev->ioctl_ops = &syna_hrx_ioctl_ops;

	ret = syna_hrx_dh_memdev_alloc(&hrx_dev->alloc_dev);
	if (ret)
		HRX_LOG(HRX_DRV_ERROR, "%s(): failed to create allocate evice\n", __func__);

	queue_init(hrx_dev);

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto err;

	hrx_dev->vdev = vdev;
	video_set_drvdata(vdev, hrx_dev);
	return 0;

err:
	return ret;
}

int syna_hrx_video_finalize(struct syna_hrx_v4l2_dev *hrx_dev)
{
	struct vb2_queue *dst_vq = &hrx_dev->dst_vq;

	if (hrx_dev) {
		/* Release queue */
		vb2_queue_release(dst_vq);
		/* Release any memory resources allocated */
		syna_hrx_dh_memdev_release();
		if (hrx_dev->vdev)
			video_unregister_device(hrx_dev->vdev);
		v4l2_device_unregister(&hrx_dev->v4l2_dev);
	}
	return 0;
}
