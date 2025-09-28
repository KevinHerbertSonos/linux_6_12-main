// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>
#include <sound/pcm_params.h>
#include <sound/pcm-indirect.h>
#include <sound/soc.h>

#include "aio_hal.h"
#include "avio_dhub_drv.h"
#include "avio_common.h"

#define DMA_BUFFER_SIZE	(256 * 1024)
#define DMA_BUFFER_MIN	(512)
#define MAX_BUFFER_SIZE	(DMA_BUFFER_SIZE << 1)

/* PCM_DMA_BUFFER_SIZE = PDM_MAX_DMA_BUFFER_SIZE / 4 */
#define PCM_DMA_BUFFER_SIZE	((256 * 1024) / 4)

#define PCM_DMA_MIN_SIZE	(1024)

/* PCM_MAX_BUFFER_SIZE = PDM_MAX_BUFFER_SIZE / 4 */
#define PCM_MAX_BUFFER_SIZE	((2048 * 1024) / 4)

#define DHUB_DMA_DEPTH	4

#define INDEX_PLAY	0
#define INDEX_CAPT	1

#define MAX_CHANNELS	2

#define DRT_GLOBAL	0x000
#define DRT_CDC1_DI12	0x004
#define DRT_CDC1_DI34	0x008
#define DRT_CDC1_DO12	0x010
#define DRT_CDC1_DO34	0x014
#define DRT_CDC2_DI12	0x01c
#define DRT_CDC2_DI34	0x020
#define DRT_CDC2_DO12	0x028
#define DRT_CDC2_DO34	0x02c
#define DRT_CDC1_DI56	0x050
#define DRT_CDC1_DI78	0x054
#define DRT_CDC1_DO56	0x05c
#define DRT_CDC1_DO78	0x060
#define DRT_CDC2_DI56	0x06c
#define DRT_CDC2_DI78	0x070
#define DRT_CDC2_DO56	0x078
#define DRT_CDC2_DO78	0x07c
#define DRT_CON		0x034
#define DRT_CODTR	0x038
#define DRT_DHBINTF	0x100
#define DRT_TEST	0x104
#define DRT_ISTATUS	0x108
#define DRT_INTENB	0x10c
#define DRT_RSAMP	0x110
#define DRT_WSAMP	0x114
#define DRT_DHBSTAT	0x118

#define DRT_GLOBAL_FILT2_DIS	(1 << 3)
#define DRT_GLOBAL_FILT1_DIS	(1 << 2)
#define DRT_GLOBAL_BLRES_DRT	(1 << 1)
#define DRT_GLOBAL_DRT_ON	(1 << 0)

#define DRT_CHUNK_SIZE 2048 // 1024 //256

struct drt_pcm {
	struct device *dev;
	HDL_dhub *dhub;
	struct mutex dhub_lock;
	struct clk *parent_clk;
	void __iomem *regs;
	void __iomem *clk_regs;
	int ch[2];
	int irq[2];
};

struct drt_playback {
	/*
	 * Tracks the base address of the last submitted DMA block.
	 * Moved to next period in ISR.
	 * read in drt_playback_pointer.
	 *
	 * Total size of ALSA buffer in the format received from userspace.
	 */
	uint32_t dma_offset;

	/*
	 * Is there a submitted DMA request?
	 * Set when a DMA request is submitted to DHUB.
	 * Cleared on 'stop' or ISR.
	 */
	bool dma_pending;

	/*
	 * Indicates if page memory is allocated
	 */
	bool pages_allocated;

	/*
	 * Instance lock between ISR and higher contexts.
	 */
	spinlock_t lock;

	/* DMA buffer */
	uint8_t *dma_area;
	dma_addr_t dma_addr;
	uint32_t dma_bytes_total;

	/* hw parameter */
	uint32_t sample_rate;
	uint32_t sample_format;
	uint32_t channel_num;
	uint32_t buf_size;
	uint32_t period_size;
	uint32_t ch;

	struct snd_pcm_substream *substream;
	struct snd_pcm_indirect pcm_indirect;
	struct drt_pcm *drt;
	int started;
};

struct drt_capture {
	/*
	 * Tracks the base address of the last submitted DMA block.
	 * Moved to next period in ISR.
	 * read in berlin_playback_pointer.
	 */
	uint32_t dma_offset;
	/*
	 * Offset of the next DMA buffer that needs to be decimated in bytes.
	 * Since it is only read/written on a work queue thread,
	 * it does not need locking.
	 */
	uint32_t read_offset;
	/*
	 * Offset in bytes at which decoded PCM data is being written.
	 * Writing should only be done on a work queue thread; must be locked.
	 * Reading on a work queue thread does not require locking.
	 * Reading outside of a work queue thread requires locking.
	 */
	uint32_t runtime_offset;
	/*
	 * Number of bytes read but not decoded yet.
	 * Read and write under lock.
	 */
	uint32_t cnt;
	/*
	 * Is there a submitted DMA request?
	 * Set when a DMA request is submitted to DHUB.
	 * Cleared on 'stop' or ISR.
	 */
	bool dma_pending;

	/*
	 * Indicates if page memory is allocated
	 */
	bool pages_allocated;

	/*
	 * Instance lock between ISR and higher contexts.
	 */
	spinlock_t lock;

	/* DMA buffer */
	uint8_t *dma_area;
	dma_addr_t dma_addr;
	uint32_t dma_bytes_total;
	uint32_t buf_size;
	uint32_t period_size;

	/* hw parameter */
	uint32_t sample_rate;
	uint32_t sample_format;
	uint32_t channel_num;
	uint32_t ch;

	/* capture status */
	bool capturing;

	struct snd_pcm_substream *substream;
	struct drt_pcm *drt;
};

static const unsigned int drt_playback_rates[] = {
	32000, 64000,
};

static struct snd_pcm_hw_constraint_list drt_constraints_rates = {
	.count = ARRAY_SIZE(drt_playback_rates),
	.list  = drt_playback_rates,
	.mask  = 0,
};

static unsigned int drt_read(struct drt_pcm *priv, unsigned int reg)
{
	return readl(priv->regs + reg);
}


static void drt_write(struct drt_pcm *priv, unsigned int reg, unsigned int val)
{
	writel(val, priv->regs + reg);
}

/*
 * Kicks off a DMA transfer to audio IO interface for the |drt_pcm|.
 * Must be called with instance spinlock held.
 * Must be called only when instance is in playing state.
 */
static void drt_start_playback_dma_if_needed(struct drt_playback *play)
{
	dma_addr_t dma_source_address;
	u32 dma_size;
	int ratio = 1;

	assert_spin_locked(&play->lock);

	if (play->pcm_indirect.hw_ready < play->period_size)
		return;

	if (play->dma_pending)
		return;

	if (play->channel_num == 1)
		ratio = MAX_CHANNELS;

	dma_source_address = play->dma_addr + play->dma_offset * ratio;
	dma_size = play->period_size * ratio;
	play->dma_pending = true;

	dhub_channel_write_cmd(play->drt->dhub, play->ch,
			       dma_source_address, dma_size, 0, 0, 0, 1, 0, 0);
}

/* must always be called under lock. */
static void drt_start_capture_dma_if_needed(struct drt_capture *cap)
{
	dma_addr_t dma_source_address;
	u32 dma_size;
	int ratio = 1;

	assert_spin_locked(&cap->lock);

	if (!cap->capturing)
		return;

	if (cap->dma_pending)
		return;

	if (cap->cnt > (cap->buf_size - cap->period_size))
		return;

	if (cap->channel_num == 1)
		ratio = MAX_CHANNELS;

	dma_source_address = cap->dma_addr + cap->dma_offset * ratio;
	dma_size = cap->period_size * ratio;
	cap->dma_pending = true;
	dhub_channel_write_cmd(cap->drt->dhub, cap->ch,
			       dma_source_address, dma_size, 0, 0, 0, 1, 0, 0);
}

static int channel_enable(void *hdl, u32 chanId, u32 enable)
{
	HDL_semaphore *pSemHandle = NULL;
	u32 uiInstate;

	pSemHandle = dhub_semaphore(hdl);
	if (!enable) {
		uiInstate = semaphore_chk_full(pSemHandle, chanId);
		if ((uiInstate >> chanId) & 1) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
		}
	}
	semaphore_intr_enable(pSemHandle, chanId, 0, enable ? 1 : 0, 0, 0, 0);

	return 0;
}

static void drt_capture_copy(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	size_t period_total;
	const size_t pcm_buffer_size_bytes =
		frames_to_bytes(runtime, runtime->buffer_size);
	uint32_t *pcm_dst;
	int frames = bytes_to_frames(runtime, cap->period_size);
	unsigned long flags;
	u32 i, j = 0;

	/* copy dhub to DMA, re-calculate data */
	period_total = frames_to_bytes(runtime, frames);

	/* copy data */
	if (cap->channel_num == 1) {
		const uint32_t *pcm_src = (uint32_t *)(cap->dma_area +
				      cap->read_offset * MAX_CHANNELS);

		pcm_dst = (uint32_t *)(runtime->dma_area + cap->runtime_offset);

		for (i = 0; i < frames / 2; i++, j += 2)
			pcm_dst[i] = pcm_src[j];
	} else {
		const uint16_t *pcm_src =
				(uint16_t *)(cap->dma_area + cap->read_offset);

		pcm_dst = (uint32_t *)(runtime->dma_area + cap->runtime_offset);

		for (i = 0; i < frames; i += 2, j += 4) {
			pcm_dst[i]     = (pcm_src[j + 3] << 16) |
					  pcm_src[j + 1];
			pcm_dst[i + 1] = (pcm_src[j + 2] << 16) |
					  pcm_src[j + 0];
		}
	}

	spin_lock_irqsave(&cap->lock, flags);
	cap->read_offset += cap->period_size;
	cap->read_offset %= cap->buf_size;
	cap->runtime_offset += period_total;
	cap->runtime_offset %= pcm_buffer_size_bytes;
	cap->cnt -= period_total;
	spin_unlock_irqrestore(&cap->lock, flags);

	snd_pcm_period_elapsed(substream);
}

static int drt_capture_isr(struct snd_pcm_substream *substream,
			   unsigned int chan_id)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	int frames = bytes_to_frames(runtime, cap->period_size);
	size_t period_total = frames_to_bytes(runtime, frames);

	spin_lock(&cap->lock);

	/* If we are not running, do not chain, and clear pending */
	if (!cap->capturing) {
		cap->dma_pending = false;
		spin_unlock(&cap->lock);

		return 0;
	}

	/* If we were not pending, avoid pointer manipulation */
	if (!cap->dma_pending) {
		spin_unlock(&cap->lock);

		return 0;
	}

	/* Roll the DMA pointer, and chain if needed */
	cap->dma_offset += cap->period_size;
	cap->dma_offset %= cap->buf_size;
	cap->dma_pending = false;
	cap->cnt += period_total;

	drt_start_capture_dma_if_needed(cap);

	spin_unlock(&cap->lock);

	drt_capture_copy(substream);

	return 0;
}

static int drt_playback_isr(struct snd_pcm_substream *substream,
			    unsigned int chan_id)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;

	spin_lock(&play->lock);

	/* If we were not pending, avoid pointer manipulation */
	if (!play->dma_pending) {
		spin_unlock(&play->lock);

		return 0;
	}

	if (chan_id == play->ch)
		play->dma_pending = false;

	/* Roll the DMA pointer, and chain if needed */
	play->dma_offset += play->period_size;
	play->dma_offset %= play->buf_size;
	spin_unlock(&play->lock);

	snd_pcm_period_elapsed(substream);

	spin_lock(&play->lock);
	drt_start_playback_dma_if_needed(play);
	spin_unlock(&play->lock);

	return 0;
}

static irqreturn_t drt_alsa_io_isr(int irq, void *dev_id)
{
	struct snd_pcm_substream *substream;
	irq_hw_number_t hw_irq;
	int chan_id;

	substream = dev_id;
	hw_irq = irqd_to_hwirq(irq_get_irq_data(irq));
	chan_id = (int)hw_irq;

	if (substream->runtime) {
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			drt_capture_isr(substream, chan_id);
		else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			drt_playback_isr(substream, chan_id);
	}

	return IRQ_HANDLED;
}

static int drt_pcm_request_dma_irq(struct snd_pcm_substream *substream,
				   struct drt_pcm *drt, int irq, char *name)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	uint32_t ch;
	int err;
	void *dev_id = substream;

	err = devm_request_irq(drt->dev, irq, drt_alsa_io_isr, 0, name, dev_id);
	if (unlikely(err < 0)) {
		snd_printk("irq %d request error: %d\n",
			   irq, err);
		return err;
	}

	ch = irqd_to_hwirq(irq_get_irq_data(irq));

	if (runtime->private_data) {
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			struct drt_capture *cap = runtime->private_data;

			cap->ch = ch;
		} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			struct drt_playback *play = runtime->private_data;

			play->ch = ch;
		}
	}

	return err;
}

static void drt_pcm_free_dma_irq(struct snd_pcm_substream *substream,
				 struct drt_pcm *drt, unsigned int irq)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (runtime->private_data) {
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			struct drt_capture *cap = runtime->private_data;

			cap->ch = 0;
		} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			struct drt_playback *play = runtime->private_data;

			play->ch = 0;
		}
	}

	devm_free_irq(drt->dev, irq, (void *)substream);
}

static const struct snd_pcm_hardware drt_playback_hw = {
	.info			= (SNDRV_PCM_INFO_MMAP
					| SNDRV_PCM_INFO_INTERLEAVED
					| SNDRV_PCM_INFO_MMAP_VALID
					| SNDRV_PCM_INFO_PAUSE
					| SNDRV_PCM_INFO_RESUME
					| SNDRV_PCM_INFO_SYNC_START),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE),
	.rates			= (SNDRV_PCM_RATE_32000 |
				   SNDRV_PCM_RATE_64000),
	.channels_min		= 1,
	.channels_max		= MAX_CHANNELS,
	.buffer_bytes_max	= MAX_BUFFER_SIZE,
	.period_bytes_min	= DMA_BUFFER_MIN,
	.period_bytes_max	= DMA_BUFFER_SIZE,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size		= 0
};

int drt_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play;
	int err;

	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &drt_constraints_rates);
	if (err < 0)
		return err;

	runtime->hw = drt_playback_hw;

	/* buffer and period size are multiple of minimum depth size */
	err = snd_pcm_hw_constraint_step(substream->runtime, 0,
					 SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
					 runtime->hw.period_bytes_min);
	if (err)
		return -EINVAL;

	err = snd_pcm_hw_constraint_step(substream->runtime, 0,
					 SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
					 runtime->hw.period_bytes_min);
	if (err)
		return -EINVAL;

	err = snd_pcm_hw_constraint_integer(substream->runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (err < 0)
		return -EINVAL;

	play = kzalloc(sizeof(*play), GFP_KERNEL);
	if (play == NULL)
		return -ENOMEM;

	runtime->private_data = play;

	play->dma_pending = false;

	play->pages_allocated = false;
	play->substream = substream;
	play->ch = 0;

	spin_lock_init(&play->lock);

	snd_printd("%s: finished.\n", __func__);

	return 0;
}

static const struct snd_pcm_hardware drt_capture_hw = {
	.info			= (SNDRV_PCM_INFO_MMAP
					| SNDRV_PCM_INFO_INTERLEAVED
					| SNDRV_PCM_INFO_MMAP_VALID
					| SNDRV_PCM_INFO_PAUSE
					| SNDRV_PCM_INFO_RESUME
					| SNDRV_PCM_INFO_SYNC_START),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE),
	.rates			= (SNDRV_PCM_RATE_32000 |
				   SNDRV_PCM_RATE_64000),
	.channels_min		= 1,
	.channels_max		= MAX_CHANNELS,
	.buffer_bytes_max	= PCM_MAX_BUFFER_SIZE,
	.period_bytes_min	= PCM_DMA_MIN_SIZE,
	.period_bytes_max	= PCM_DMA_BUFFER_SIZE,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size		= 0
};

int drt_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap;
	int err;

	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &drt_constraints_rates);
	if (err < 0)
		return err;

	runtime->hw = drt_capture_hw;

	/* buffer and period size are multiple of minimum depth size */
	err = snd_pcm_hw_constraint_step(substream->runtime, 0,
					 SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
					 runtime->hw.period_bytes_min);
	if (err)
		return -EINVAL;

	err = snd_pcm_hw_constraint_step(substream->runtime, 0,
					 SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
					 runtime->hw.period_bytes_min);
	if (err)
		return -EINVAL;

	err = snd_pcm_hw_constraint_integer(substream->runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (err < 0)
		return -EINVAL;

	cap = kzalloc(sizeof(*cap), GFP_KERNEL);
	if (cap == NULL)
		return -ENOMEM;

	runtime->private_data = cap;

	cap->dma_pending = false;
	cap->capturing = false;
	cap->pages_allocated = false;
	cap->substream = substream;
	cap->dma_area = NULL;
	cap->dma_addr = 0;
	cap->ch = 0;

	spin_lock_init(&cap->lock);

	snd_printd("%s: finished.\n", __func__);

	return 0;
}

static int drt_pcm_open(struct snd_soc_component *component,
			struct snd_pcm_substream *substream)
{
	struct device *dev = component->dev;
	struct drt_pcm *drt = dev_get_drvdata(dev);

	snd_printd("%s stream: %s (%p)\n", __func__,
		   substream->stream == SNDRV_PCM_STREAM_CAPTURE ?
		   "capture" : "playback", substream);

	/* get dhub handle */
	if (mutex_lock_interruptible(&drt->dhub_lock) != 0)
		return  -EINTR;

	if (drt->dhub == NULL) {
		drt->dhub = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB);
		if (unlikely(drt->dhub == NULL)) {
			snd_printk("drt->dhub: get failed\n");
			mutex_unlock(&drt->dhub_lock);

			return -EBUSY;
		}
	}
	mutex_unlock(&drt->dhub_lock);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return drt_capture_open(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return drt_playback_open(substream);

	return 0;
}

static int drt_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;

	kfree(play);
	runtime->private_data = NULL;

	snd_printd("%s: finished.\n", __func__);

	return 0;
}

static int drt_capture_hw_free(struct snd_pcm_substream *substream);

static int drt_capture_close(struct snd_pcm_substream *substream)
{
	/* Disable audio interrupt */
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;

	if (cap) {
		/* need to flush the delayed work after disable interrupt */
		drt_capture_hw_free(substream);

		kfree(cap);
		runtime->private_data = NULL;
	}

	snd_printd("%s: substream 0x%p done\n", __func__, substream);

	return 0;
}

static int drt_pcm_close(struct snd_soc_component *component,
			 struct snd_pcm_substream *substream)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		   substream->stream == SNDRV_PCM_STREAM_CAPTURE ?
		   "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return drt_capture_close(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return drt_playback_close(substream);

	return 0;
}

int drt_playback_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	struct drt_pcm *drt = play->drt;

	if (drt) {
		if (play->dma_area) {
			dma_free_coherent(drt->dev,
					  play->dma_bytes_total,
					  play->dma_area,
					  play->dma_addr);
			play->dma_area = NULL;
			play->dma_addr = 0;
		}
	}

	if (play->pages_allocated == true) {
		snd_pcm_lib_free_pages(substream);
		play->pages_allocated = false;
	}

	return 0;
}

static int drt_playback_hw_params(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	struct device *dev = component->dev;
	struct drt_pcm *drt = dev_get_drvdata(dev);
	uint32_t dhbintf;
	int err;

	snd_printd("%s: fs:%d ch:%d width:%d format:%s, period bytes:%d buffer bytes: %d\n",
		   __func__, params_rate(params), params_channels(params),
		   params_width(params),
		   snd_pcm_format_name(params_format(params)),
		   params_period_bytes(params), params_buffer_bytes(params));

	if (params_format(params) != SNDRV_PCM_FORMAT_S16_LE) {
		snd_printd("Unsupported format: %s\n",
			   snd_pcm_format_name(params_format(params)));

		return -EINVAL;
	}

	err = drt_pcm_request_dma_irq(substream, drt, drt->irq[INDEX_PLAY],
				      "drt-playback");

	play->drt = drt;
	drt_playback_hw_free(substream);

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (err < 0) {
		snd_printk("failed to allocated pages for buffers\n");
		goto err_malloc;
	}
	play->pages_allocated = true;

	play->sample_rate = params_rate(params);
	play->sample_format = params_format(params);
	play->channel_num = params_channels(params);
	play->period_size = params_period_bytes(params);
	play->buf_size = params_buffer_bytes(params);

	if (play->sample_rate == 64000)
		drt_write(drt, DRT_CON, 0xc214);
	else
		drt_write(drt, DRT_CON, 0x4214);

	play->dma_bytes_total = play->buf_size * MAX_CHANNELS;

	play->dma_area =
		dma_alloc_coherent(dev, play->dma_bytes_total,
				   &play->dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!play->dma_area) {
		snd_printk("%s: failed to allocate PCM DMA area\n", __func__);
		goto err_pcm_dma;
	}

	/* RFIFOFLUSH */
	dhbintf = drt_read(play->drt, DRT_DHBINTF);
	drt_write(play->drt, DRT_DHBINTF, dhbintf | 0x1);
	udelay(100);
	drt_write(play->drt, DRT_DHBINTF, dhbintf);

	return 0;

err_pcm_dma:
	snd_pcm_lib_free_pages(substream);
err_malloc:
	drt_pcm_free_dma_irq(substream, play->drt, play->drt->irq[INDEX_PLAY]);

	return -ENOMEM;
}

static int drt_capture_hw_params(struct snd_soc_component *component,
				 struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	struct device *dev = component->dev;
	struct drt_pcm *drt = dev_get_drvdata(dev);
	uint32_t dhbintf;
	int err;

	cap->sample_rate = params_rate(params);
	cap->sample_format = params_format(params);
	cap->channel_num = params_channels(params);
	cap->period_size = params_period_bytes(params);
	cap->buf_size = params_buffer_bytes(params);

	snd_printd("fs:%d ch:%d, width:%d fmt:%s, buf size:%u period size:%u\n",
		   params_rate(params), params_channels(params),
		   params_width(params),
		   snd_pcm_format_name(cap->sample_format), cap->buf_size,
		   cap->period_size);

	err = drt_pcm_request_dma_irq(substream, drt, drt->irq[INDEX_CAPT],
				      "drt-capture");

	cap->drt = drt;
	drt_capture_hw_free(substream);

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (err < 0) {
		snd_printk("fail to alloc pages for buffers %d\n", err);
		goto err_malloc;
	}
	cap->pages_allocated = true;

	if (cap->sample_rate == 64000)
		drt_write(drt, DRT_CON, 0xc214);
	else
		drt_write(drt, DRT_CON, 0x4214);

	cap->dma_bytes_total = cap->buf_size * MAX_CHANNELS;

	cap->dma_area = dma_alloc_coherent(drt->dev,
					   cap->dma_bytes_total,
					   &cap->dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!cap->dma_area) {
		snd_printk("%s: failed to allocate DMA area\n", __func__);
		goto err_dma;
	}

	/* WFIFOFLUSH */
	dhbintf = drt_read(cap->drt, DRT_DHBINTF);
	drt_write(cap->drt, DRT_DHBINTF, dhbintf | 0x2);
	udelay(100);
	drt_write(cap->drt, DRT_DHBINTF, dhbintf);

	/* Microphone takes 32768 cycles to wake up. Sleep for
	 * ceil(32768 cycles * (1/samplerate seconds per cycle) * 10^6) us
	 * This is not in trigger_start() because sleep should not happen
	 * in the atomic trigger callback.
	 * http://www.alsa-project.org/~tiwai/writing-an-alsa-driver/\
	 * ch05s06.html#pcm-interface-operators-trigger-callback
	 */
	/* mic_sleep_us = DIV_ROUND_UP_ULL(32768ull * 1000 * 3000,
	 *				cap->fs * cap->cic[0].decimation);
	 * usleep_range(mic_sleep_us, mic_sleep_us + 100);
	 */

	return 0;

err_dma:
	snd_pcm_lib_free_pages(substream);
err_malloc:
	drt_pcm_free_dma_irq(substream, cap->drt, drt->irq[INDEX_CAPT]);

	return -ENOMEM;
}

static int drt_pcm_hw_params(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		   substream->stream == SNDRV_PCM_STREAM_CAPTURE ?
		   "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return drt_capture_hw_params(component, substream, params);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return drt_playback_hw_params(component, substream, params);

	return 0;
}

static int drt_capture_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	struct drt_pcm *drt = cap->drt;

	/* clear the DMA queue dhub */
	if (cap->ch > 0)
		DhubChannelClear(drt->dhub, cap->ch, 0);

	if (drt && cap->dma_area) {
		dma_free_coherent(drt->dev, cap->dma_bytes_total,
				  cap->dma_area, cap->dma_addr);
		cap->dma_area = NULL;
		cap->dma_addr = 0;
	}

	if (cap->pages_allocated == true) {
		snd_pcm_lib_free_pages(substream);
		cap->pages_allocated = false;
	}

	return 0;
}

static int drt_pcm_hw_free(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_printd("%s stream: %s (%p)\n", __func__,
		   substream->stream == SNDRV_PCM_STREAM_CAPTURE ?
		   "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		struct drt_capture *cap = runtime->private_data;

		drt_pcm_free_dma_irq(substream, cap->drt,
				     cap->drt->irq[INDEX_CAPT]);

		return drt_capture_hw_free(substream);
	} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		struct drt_playback *play = runtime->private_data;

		drt_pcm_free_dma_irq(substream, play->drt,
				     play->drt->irq[INDEX_PLAY]);

		return drt_playback_hw_free(substream);
	}

	return 0;
}

static void drt_playback_trigger_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	uint32_t dhbintf;
	unsigned long flags;

	drt_write(play->drt, DRT_GLOBAL, DRT_GLOBAL_DRT_ON);

	/* READST - start DRT sample reading from (0x10) */
	dhbintf = drt_read(play->drt, DRT_DHBINTF);
	drt_write(play->drt, DRT_DHBINTF, dhbintf | 0x10);

	channel_enable(play->drt->dhub, play->ch, true);

	spin_lock_irqsave(&play->lock, flags);
	play->started = 1;
	drt_start_playback_dma_if_needed(play);
	spin_unlock_irqrestore(&play->lock, flags);

	snd_printd("%s: dma chid%d start done. Period 0x%X\n", __func__,
		   play->ch, play->period_size);
}

static void drt_playback_trigger_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	uint32_t dhbintf;

	play->started = 0;

	dhub_channel_clear_done(play->drt->dhub, play->ch);
	DhubChannelClear(play->drt->dhub, play->ch, 0);
	channel_enable(play->drt->dhub, play->ch, false);

	dhbintf = drt_read(play->drt, DRT_DHBINTF);
	drt_write(play->drt, DRT_DHBINTF, dhbintf & ~(0x10));
	drt_write(play->drt, DRT_GLOBAL, 0);
}

static void drt_capture_trigger_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	unsigned long flags;
	dma_addr_t dma_src;
	uint32_t dma_size, dhbintf;
	int ratio = 1;

	spin_lock_irqsave(&cap->lock, flags);
	if (cap->capturing) {
		spin_unlock_irqrestore(&cap->lock, flags);
		return;
	}

	drt_write(cap->drt, DRT_GLOBAL, DRT_GLOBAL_DRT_ON);

	/* WRITEST - start DRT sample writing into DHUB */
	dhbintf = drt_read(cap->drt, DRT_DHBINTF);
	drt_write(cap->drt, DRT_DHBINTF, dhbintf | 0x20);

	cap->capturing = true;

	dma_size = cap->period_size;
	// Queue two DMA commands when trigger start to avoid FIFO overrun
	/* In each interrupt, write one DMA command, so if interrupts are
	 * handled in time, there will always be 2 DMA commands in queue.
	 * The first interrupt will be raised and stay asserted until cleared.
	 * If the first interrupt isn’t cleared before the second interrupt is
	 * raised, the interrupt line stays asserted.
	 * The overrun happens when IRQ is disabled for longer than 8 ms, such
	 * that IRQ is not served and the next DMA is not set up in time before
	 * FIFO being filled up.
	 * The calculation is based on FIFO size = 2048 bytes, 16khz sampling
	 * rate, stereo and 64 bits per sample for PDM data.
	 * PDM_MAX_DMA_BUFFER_SIZE / (DECIMATION_FACTOR / 32) /
	 * (2 channels * 4 bytes / sample) / (16000 frames / second)
	 * 4096 / (128 / 32) / 8 / 16000 = 0.008 sec
	 * Queueing 2 DMA commands will relax the limit to ~16ms
	 */
	// We intentionally let the dma_src in dhub_channel_write_cmd
	// be the same in the for-loop, because the data in the first few DMA
	// blocks may be corrupted and we just ignore them.

	// skip dma cmd will not trigger interrupt
	if (cap->channel_num == 1)
		ratio = MAX_CHANNELS;

	dma_src = cap->dma_addr + cap->dma_offset * ratio;
	dhub_channel_write_cmd(cap->drt->dhub, cap->ch, dma_src,
			       dma_size * ratio, 0, 0, 0, 0, 0, 0);

	dhub_channel_write_cmd(cap->drt->dhub, cap->ch, dma_src,
			       dma_size * ratio, 0, 0, 0, 1, 0, 0);

	cap->dma_offset += dma_size;

	cap->dma_pending = true;

	spin_unlock_irqrestore(&cap->lock, flags);
	snd_printd("%s: dma capture chid%d start done. Period 0x%X\n", __func__,
		   cap->ch, cap->period_size);
}

static void drt_capture_trigger_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	unsigned long flags;

	spin_lock_irqsave(&cap->lock, flags);
	cap->capturing = false;
	cap->dma_pending = false;
	spin_unlock_irqrestore(&cap->lock, flags);
}

static int drt_capture_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		drt_capture_trigger_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		drt_capture_trigger_stop(substream);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int drt_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		drt_playback_trigger_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		drt_playback_trigger_stop(substream);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int drt_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	unsigned long flags;

	spin_lock_irqsave(&play->lock, flags);
	play->dma_offset = 0;
	memset(&play->pcm_indirect, 0, sizeof(play->pcm_indirect));
	play->pcm_indirect.hw_buffer_size = play->buf_size;
	play->pcm_indirect.sw_buffer_size = snd_pcm_lib_buffer_bytes(substream);
	spin_unlock_irqrestore(&play->lock, flags);

	snd_printd("%s finished. buffer: %zd period: %zd hw %u sw %u\n",
		   __func__,
		   snd_pcm_lib_buffer_bytes(substream),
		   snd_pcm_lib_period_bytes(substream),
		   play->pcm_indirect.hw_buffer_size,
		   play->pcm_indirect.sw_buffer_size);

	return 0;
}

static int drt_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	unsigned long flags;

	spin_lock_irqsave(&cap->lock, flags);
	cap->dma_offset = 0;
	cap->read_offset = 0;
	cap->cnt = 0;
	cap->runtime_offset = 0;
	spin_unlock_irqrestore(&cap->lock, flags);

	return 0;
}

static int drt_pcm_prepare(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		   substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" :
		   "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return drt_capture_prepare(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return drt_playback_prepare(substream);

	return 0;
}

static int drt_pcm_trigger(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_substream *s;

	snd_printd("%s stream: %s %s (%p)\n", __func__,
		   substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" :
		   "playback", cmd == SNDRV_PCM_TRIGGER_START ? "start" :
		   "stop", substream);

	snd_pcm_group_for_each_entry(s, substream) {
		if (s->stream == SNDRV_PCM_STREAM_CAPTURE)
			drt_capture_trigger(s, cmd);
		else if (s->stream == SNDRV_PCM_STREAM_PLAYBACK)
			drt_playback_trigger(s, cmd);
	}

	return 0;
}

static snd_pcm_uframes_t
drt_capture_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_capture *cap = runtime->private_data;
	uint32_t buf_pos;
	unsigned long flags;

	spin_lock_irqsave(&cap->lock, flags);
	buf_pos = cap->runtime_offset;
	spin_unlock_irqrestore(&cap->lock, flags);

	return bytes_to_frames(runtime, buf_pos);
}

static snd_pcm_uframes_t
drt_playback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	uint32_t buf_pos;
	unsigned long flags;

	spin_lock_irqsave(&play->lock, flags);
	buf_pos = play->dma_offset;
	spin_unlock_irqrestore(&play->lock, flags);

	return snd_pcm_indirect_playback_pointer(substream,
						 &play->pcm_indirect, buf_pos);
}

static snd_pcm_uframes_t
drt_pcm_pointer(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return drt_capture_pointer(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return drt_playback_pointer(substream);

	return 0;
}

static int drt_playback_copy(struct snd_pcm_substream *substream,
			     int channel, snd_pcm_uframes_t pos,
			     void *buf, size_t bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	uint32_t *pcm_buf;
	const int frames = bytes_to_frames(runtime, bytes);
	const uint16_t *s16_pcm_source = (uint16_t *)buf;
	int i, j = 0;

	if (pos >= play->buf_size)
		return -EINVAL;

	if (play->channel_num == 1) {
		/* Copy the data to both codecs (might change later) */

		pcm_buf = (uint32_t *)(play->dma_area + pos * MAX_CHANNELS);

		for (i = 0; i < frames; i += 2, j += 2) {
			pcm_buf[i]     = (s16_pcm_source[j + 1] << 16) |
					  s16_pcm_source[j + 0];
			pcm_buf[i + 1] = (s16_pcm_source[j + 1] << 16) |
					  s16_pcm_source[j + 0];
		}
	} else {
		pcm_buf = (uint32_t *)(play->dma_area + pos);

		for (i = 0; i < frames; i += 2, j += 4) {
			pcm_buf[i]     = (s16_pcm_source[j + 3] << 16) |
					  s16_pcm_source[j + 1];
			pcm_buf[i + 1] = (s16_pcm_source[j + 2] << 16) |
					  s16_pcm_source[j + 0];
		}
	}

	return 0;
}

static void drt_playback_transfer(struct snd_pcm_substream *substream,
				  struct snd_pcm_indirect *rec,
				  size_t bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	void *src = (void *)(runtime->dma_area + rec->sw_data);

	if (!src)
		return;

	drt_playback_copy(substream, play->channel_num,
			  rec->hw_data, src, bytes);
}

static int drt_playback_ack(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct drt_playback *play = runtime->private_data;
	struct snd_pcm_indirect *pcm_indirect = &play->pcm_indirect;
	unsigned long flags;

	pcm_indirect->hw_queue_size = play->buf_size;
	snd_pcm_indirect_playback_transfer(substream, pcm_indirect,
					   drt_playback_transfer);

	if (play->started) {
		spin_lock_irqsave(&play->lock, flags);
		drt_start_playback_dma_if_needed(play);
		spin_unlock_irqrestore(&play->lock, flags);
	}

	return 0;
}

static int drt_pcm_ack(struct snd_soc_component *component,
		       struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return drt_playback_ack(substream);

	return 0;
}

#define PREALLOC_BUFFER (2 * 1024 * 1024)
#define PREALLOC_BUFFER_MAX (2 * 1024 * 1024)

static int drt_pcm_new(struct snd_soc_component *component,
		       struct snd_soc_pcm_runtime *rtd)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
	snd_pcm_lib_preallocate_pages_for_all(rtd->pcm,
			SNDRV_DMA_TYPE_CONTINUOUS,
			snd_dma_continuous_data
			(GFP_KERNEL),
			PREALLOC_BUFFER,
			PREALLOC_BUFFER_MAX);
#else
	snd_pcm_lib_preallocate_pages_for_all(rtd->pcm,
			SNDRV_DMA_TYPE_CONTINUOUS,
			component->dev,
			PREALLOC_BUFFER,
			PREALLOC_BUFFER_MAX);
#endif

	return 0;
}

static void drt_pcm_free(struct snd_soc_component *component,
			 struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static const struct snd_soc_component_driver drt_pcm_component = {
	.name		= "syna-drt-pcm",
	.pcm_construct	= drt_pcm_new,
	.pcm_destruct	= drt_pcm_free,
	.open		= drt_pcm_open,
	.close		= drt_pcm_close,
	.hw_params	= drt_pcm_hw_params,
	.hw_free	= drt_pcm_hw_free,
	.prepare	= drt_pcm_prepare,
	.trigger	= drt_pcm_trigger,
	.pointer	= drt_pcm_pointer,
	.ack		= drt_pcm_ack,
};

static struct snd_soc_dai_driver drt_pcm_dai = {
	.name     = "drt-pcm",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= MAX_CHANNELS,
		.rates		= SNDRV_PCM_RATE_32000 |
				  SNDRV_PCM_RATE_64000,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= MAX_CHANNELS,
		.rates		= SNDRV_PCM_RATE_32000 |
				  SNDRV_PCM_RATE_64000,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE,
	},
};

static int drt_probe(struct platform_device *pdev)
{
	struct drt_pcm *drt;
	struct device *dev = &pdev->dev;
	uint32_t dhbintf;
	int irq, ret;

	/* Defer probe until dependent soc module/s are probed/initialized */
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	drt = devm_kzalloc(dev, sizeof(struct drt_pcm), GFP_KERNEL);
	if (drt == NULL)
		return -ENOMEM;

	drt->dev = dev;

	drt->regs = devm_platform_ioremap_resource(pdev, 0);
	if (!drt->regs) {
		dev_err(&pdev->dev, "failed to map registers\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, INDEX_PLAY);
	if (irq < 0) {
		dev_err(&pdev->dev, "no playback irq specified\n");
		return irq;
	}

	drt->irq[INDEX_PLAY] = irq;
	drt->ch[INDEX_PLAY] =
			irqd_to_hwirq(irq_get_irq_data(drt->irq[INDEX_PLAY]));

	irq = platform_get_irq(pdev, INDEX_CAPT);
	if (irq < 0) {
		dev_err(&pdev->dev, "no capture irq specified\n");
		return irq;
	}

	drt->irq[INDEX_CAPT] = irq;
	drt->ch[INDEX_CAPT] =
			irqd_to_hwirq(irq_get_irq_data(drt->irq[INDEX_CAPT]));

	mutex_init(&drt->dhub_lock);
	of_dma_configure(dev, dev->of_node, true);

	dev_set_drvdata(dev, drt);

	drt_write(drt, DRT_GLOBAL, DRT_GLOBAL_BLRES_DRT);
	udelay(10);
	drt_write(drt, DRT_GLOBAL, 0);

	drt_write(drt, DRT_CON, 0x4214);
	drt_write(drt, DRT_CODTR, 0);

	/* No absolute/relative/asap timestamp mode
	 * INTRCTR - Turn off all pInc/semaphore interrupts
	 * Insert MUTE/ZERO sample upon FIFO empty,
	 * keep the last sample repeated, until stopped.
	 */
	dhbintf = 0xc00 | (3 << 7);

	/* RFIFOFLUSH, WFIFOFLUSH */
	drt_write(drt, DRT_DHBINTF, dhbintf | 0x5);
	udelay(100);
	drt_write(drt, DRT_DHBINTF, dhbintf);
	udelay(1);
	/* DRTTGON - enable timing generation control */
	drt_write(drt, DRT_DHBINTF, dhbintf | 0x8);

	ret = devm_snd_soc_register_component(dev, &drt_pcm_component,
					      &drt_pcm_dai, 1);
	if (ret < 0)
		snd_printk("failed to register component\n");

	return ret;
}

static RET_TYPE drt_remove(struct platform_device *pdev)
{
	struct drt_pcm *drt = dev_get_drvdata(&pdev->dev);

	drt_write(drt, DRT_GLOBAL, 0);
	drt_write(drt, DRT_DHBINTF, 0);

	RETURN_VALUE;
}

static const struct of_device_id drt_pcm_of_match[] = {
	{.compatible = "syna,myna2-drt"},
	{}
};
MODULE_DEVICE_TABLE(of, drt_pcm_of_match);

static struct platform_driver drt_driver = {
	.driver			= {
		.name		= "syna-drt-pcm",
		.of_match_table = drt_pcm_of_match,
	},

	.probe			= drt_probe,
	.remove			= drt_remove,
};
module_platform_driver(drt_driver);

MODULE_AUTHOR("Synaptics");
MODULE_DESCRIPTION("DRT Driver");
MODULE_LICENSE("GPL v2");
