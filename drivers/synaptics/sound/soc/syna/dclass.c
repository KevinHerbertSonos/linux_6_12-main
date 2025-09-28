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
#define DMA_BUFFER_MIN	(256)
#define MAX_BUFFER_SIZE	(DMA_BUFFER_SIZE << 1)

#define DCLS_CNTRL	0x00
#define DCLS_STAT	0x04
#define DCLS_MODE	0x08
#define DCLS_DAT	0x0C
#define DCLS_BQ1_B0	0x10
#define DCLS_BQ1_B1	0x14
#define DCLS_BQ1_B2	0x18
#define DCLS_BQ1_A1N	0x1c
#define DCLS_BQ1_A2N	0x20
#define DCLS_BQ2_B0	0x24
#define DCLS_BQ2_B1	0x28
#define DCLS_BQ2_B2	0x2c
#define DCLS_BQ2_A1N	0x30
#define DCLS_BQ2_A2N	0x34
#define DCLS_BQ3_B0	0x38
#define DCLS_BQ3_B1	0x3c
#define DCLS_BQ3_B2	0x40
#define DCLS_BQ3_A1N	0x44
#define DCLS_BQ3_A2N	0x48
#define DCLS_BQ4_B0	0x4c
#define DCLS_BQ4_B1	0x50
#define DCLS_BQ4_B2	0x54
#define DCLS_BQ4_A1N	0x58
#define DCLS_BQ4_A2N	0x5c
#define DCLS_ISTATUS	0x60
#define DCLS_INTENB	0x64
#define DCLS_DCLIENT	0x68
#define DCLS_DINP	0x6c
#define DCLS_TEST	0x74
#define DCLS_DSTAT	0x78
#define DCLS_SAMP	0x7c

#define DCLS_CNTRL_ENABLE                 0
#define DCLS_CNTRL_ZIS                    1
#define DCLS_CNTRL_FIFO_WMT               2
#define DCLS_CNTRL_FIFO_WMT_MASK          0x1f
#define DCLS_CNTRL_DCA_IE                 7
#define DCLS_CNTRL_ZI_RATIO               8
#define DCLS_CNTRL_ZI_RATIO_MASK          0x3f
#define DCLS_CNTRL_UPS_RATIO              14
#define DCLS_CNTRL_UPS_RATIO_MASK         0xff
#define DCLS_CNTRL_MOD_MODE               22
#define DCLS_CNTRL_MOD_MODE_MASK          0x3
#define DCLS_CNTRL_INV_OUTPUT             24

#define DCLS_MODE_STEPS                   0
#define DCLS_MODE_STEPS_MASK              0x7
#define DCLS_MODE_AHB_ERR_IE              3
#define DCLS_MODE_DITHER2_MODE            4
#define DCLS_MODE_DITHER2_MODE_MASK       0x3
#define DCLS_MODE_MOD_LIM                 8
#define DCLS_MODE_MOD_LIM_MASK            0x7f
#define DCLS_MODE_LONG_PULSE              15
#define DCLS_MODE_DITHER1_P               16
#define DCLS_MODE_DITHER1_P_MASK          0xff
#define DCLS_MODE_DITHER1_N               24
#define DCLS_MODE_DITHER1_N_MASK          0xff

#define DCLS_CNTRL_VALUE(en, zis, fwm, zi, ups, mode, inv) \
	((uint32_t) 0 | \
	(((en) & 1) << DCLS_CNTRL_ENABLE) | \
	(((zis) & 1) << DCLS_CNTRL_ZIS) | \
	(((fwm) & DCLS_CNTRL_FIFO_WMT_MASK) << DCLS_CNTRL_FIFO_WMT) | \
	(((zi) & DCLS_CNTRL_ZI_RATIO_MASK) << DCLS_CNTRL_ZI_RATIO) | \
	(((ups) & DCLS_CNTRL_UPS_RATIO_MASK) << DCLS_CNTRL_UPS_RATIO) | \
	(((mode) & DCLS_CNTRL_MOD_MODE_MASK) << DCLS_CNTRL_MOD_MODE) | \
	(((inv) & 1) << DCLS_CNTRL_INV_OUTPUT))

#define DCLS_MODE_VALUE(steps, dither2, modlim, longp, dither1p, dither1n) \
	((uint32_t) 0 | \
	(((steps) & DCLS_MODE_STEPS_MASK) << DCLS_MODE_STEPS) | \
	(((dither2) & DCLS_MODE_DITHER2_MODE_MASK) << DCLS_MODE_DITHER2_MODE) | \
	(((modlim) & DCLS_MODE_MOD_LIM_MASK) << DCLS_MODE_MOD_LIM) | \
	(((longp) & 1) << DCLS_MODE_LONG_PULSE) | \
	(((dither1p) & DCLS_MODE_DITHER1_P_MASK) << DCLS_MODE_DITHER1_P) | \
	(((dither1n) & DCLS_MODE_DITHER1_N_MASK) << DCLS_MODE_DITHER1_N))

static const uint32_t r8000_f256_clk276480[] = {
	0x00360, 0x006c0, 0x00360, 0x1edc7, 0x311cc,
	0x00042, 0x00085, 0x00042, 0x1efc4, 0x30f29,
	0x00038, 0x00071, 0x00038, 0x1f9ac, 0x303a2,
	0x001b0, 0x00360, 0x001b0, 0x1f3c5, 0x30a38,
};

static const uint32_t r16000_f256_clk276480[] = {
	0x00520, 0x00a50, 0x00520, 0x1e022, 0x31e95,
	0x000ca, 0x00194, 0x000ca, 0x1e29f, 0x31a1d,
	0x00203, 0x00406, 0x00203, 0x1f158, 0x30658,
	0x0018e, 0x0031b, 0x0018e, 0x1e829, 0x311b3,
};

static const uint32_t r32000_f256_clk276480[] = {
	0x00c80, 0x01900, 0x00c80, 0x1b8f8, 0x340c8,
	0x003d2, 0x007a3, 0x003d2, 0x1b89c, 0x33779,
	0x00a16, 0x0142c, 0x00a16, 0x1c889, 0x30dda,
	0x0081f, 0x0103f, 0x0081f, 0x1bbe3, 0x325ed,
};

static const uint32_t r48000_f192_clk276480[] = {
	0x01094, 0x02124, 0x01094, 0x18b10, 0x36468,
	0x00a07, 0x0140d, 0x00a07, 0x18086, 0x35593,
	0x01ada, 0x035b4, 0x01ada, 0x17bf4, 0x3158d,
	0x01542, 0x02a85, 0x01542, 0x17655, 0x33a60,
};

static const uint32_t r64000_f256_clk276480[] = {
	0x03c3c, 0x3b869, 0x03c3c, 0x18da2, 0x32b34,
	0x03109, 0x3dfd7, 0x03109, 0x18839, 0x34d1e,
	0x00f84, 0x011fa, 0x00f84, 0x185e4, 0x367b9,
	0x0be69, 0x3011d, 0x0be69, 0x198c6, 0x30daa,
};

struct dclass_pcm {
	struct device *dev;
	HDL_dhub *dhub;
	struct mutex dhub_lock;
	struct clk *parent_clk;
	void __iomem *regs;
	void __iomem *clk_regs;
	int ch;
	int irq;
};

struct dclass_playback {
	/*
	 * Tracks the base address of the last submitted DMA block.
	 * Moved to next period in ISR.
	 * read in dclass_playback_pointer.
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
	uint32_t buf_size;
	uint32_t period_size;
	uint32_t ch;

	struct snd_pcm_substream *substream;
	struct snd_pcm_indirect pcm_indirect;
	struct dclass_pcm *dclass;
	int started;
};

static unsigned int dclass_playback_rates[] = {
	8000, 16000, 32000, 48000, 64000,
};

static struct snd_pcm_hw_constraint_list dclass_constraints_rates = {
	.count = ARRAY_SIZE(dclass_playback_rates),
	.list  = dclass_playback_rates,
	.mask  = 0,
};

static unsigned int dclass_read(struct dclass_pcm *dclass, unsigned int reg)
{
	return readl(dclass->regs + reg);
}

static void dclass_write(struct dclass_pcm *dclass, unsigned int reg,
			      unsigned int val)
{
	writel(val, dclass->regs + reg);
}

/*
 * Kicks off a DMA transfer to audio IO interface for the |dclass_pcm|.
 * Must be called with instance spinlock held.
 * Must be called only when instance is in playing state.
 */
static void start_dma_if_needed(struct dclass_playback *play)
{
	dma_addr_t dma_source_address;
	int dma_size;

	assert_spin_locked(&play->lock);

	if (play->pcm_indirect.hw_ready < play->period_size)
		return;

	if (!play->dma_pending) {
		dma_source_address = play->dma_addr + play->dma_offset;
		dma_size = play->period_size;
		play->dma_pending = true;

		dhub_channel_write_cmd(play->dclass->dhub, play->ch,
				       dma_source_address, dma_size,
				       0, 0, 0, 1, 0, 0);
	}
}

static int channel_enable(void *hdl, uint32_t chanId, uint32_t enable)
{
	HDL_semaphore *pSemHandle = NULL;
	uint32_t uiInstate;

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

static irqreturn_t dclass_alsa_io_isr(int irq, void *dev_id)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	struct dclass_playback *play;
	irq_hw_number_t hw_irq;
	int chan_id;

	substream = dev_id;
	runtime = substream->runtime;
	if (!runtime)
		return IRQ_HANDLED;

	hw_irq = irqd_to_hwirq(irq_get_irq_data(irq));
	chan_id = (int)hw_irq;

	play = runtime->private_data;
	/* If we were not pending or started, avoid pointer manipulation */
	if (!play->started || !play->dma_pending)
		return IRQ_HANDLED;

	spin_lock(&play->lock);

	if (chan_id == play->ch)
		play->dma_pending = false;

	/* Roll the DMA pointer, and chain if needed */
	play->dma_offset += play->period_size;
	play->dma_offset %= play->buf_size;
	spin_unlock(&play->lock);

	snd_pcm_period_elapsed(substream);

	spin_lock(&play->lock);
	start_dma_if_needed(play);
	spin_unlock(&play->lock);

	return IRQ_HANDLED;
}

static int dclass_pcm_request_dma_irq(struct snd_pcm_substream *substream,
				      struct dclass_pcm *dclass, int irq,
				      char *name)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	uint32_t ch;
	int err;

	err = devm_request_irq(dclass->dev, irq, dclass_alsa_io_isr, 0, name,
			       substream);
	if (unlikely(err < 0)) {
		snd_printk("irq %d request error: %d\n", irq, err);

		return err;
	}

	ch = irqd_to_hwirq(irq_get_irq_data(irq));

	if (runtime->private_data) {
		struct dclass_playback *play = runtime->private_data;

		play->ch = ch;
	}

	return err;
}

static const struct snd_pcm_hardware dclass_playback_hw = {
	.info			= (SNDRV_PCM_INFO_MMAP
					| SNDRV_PCM_INFO_INTERLEAVED
					| SNDRV_PCM_INFO_MMAP_VALID
					| SNDRV_PCM_INFO_PAUSE
					| SNDRV_PCM_INFO_RESUME
					| SNDRV_PCM_INFO_SYNC_START),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE),
	.rates			= (SNDRV_PCM_RATE_8000  |
				   SNDRV_PCM_RATE_16000 |
				   SNDRV_PCM_RATE_32000 |
				   SNDRV_PCM_RATE_48000 |
				   SNDRV_PCM_RATE_64000),
	.channels_min		= 1,
	.channels_max		= 1,
	.buffer_bytes_max	= MAX_BUFFER_SIZE,
	.period_bytes_min	= DMA_BUFFER_MIN,
	.period_bytes_max	= DMA_BUFFER_SIZE,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size		= 0
};

static int dclass_pcm_open(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream)
{
	struct device *dev = component->dev;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_pcm *dclass = dev_get_drvdata(dev);
	struct dclass_playback *play;
	int err;

	snd_printd("%s stream: playback (%p)\n", __func__, substream);

	if (mutex_lock_interruptible(&dclass->dhub_lock) != 0)
		return -EINTR;

	/* get dhub handle */
	if (dclass->dhub == NULL) {
		dclass->dhub = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB);
		if (unlikely(dclass->dhub == NULL)) {
			snd_printk("dclass->dhub: get failed\n");
			mutex_unlock(&dclass->dhub_lock);

			return -EBUSY;
		}
	}
	mutex_unlock(&dclass->dhub_lock);

	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &dclass_constraints_rates);
	if (err < 0)
		return err;

	runtime->hw = dclass_playback_hw;

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
	play->ch = -1;

	spin_lock_init(&play->lock);

	snd_printd("%s: finished.\n", __func__);

	return 0;
}

static int dclass_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;

	kfree(play);
	runtime->private_data = NULL;

	snd_printd("%s: finished.\n", __func__);

	return 0;
}

static int dclass_pcm_close(struct snd_soc_component *component,
			 struct snd_pcm_substream *substream)
{
	snd_printd("%s stream: playback (%p)\n", __func__, substream);

	return dclass_playback_close(substream);
}

static int dclass_playback_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	struct dclass_pcm *dclass = play->dclass;

	if (dclass) {
		if (play->dma_area) {
			dma_free_coherent(dclass->dev,
					  play->buf_size,
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

static int dclass_pcm_hw_params(struct snd_soc_component *component,
				struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	struct device *dev = component->dev;
	struct dclass_pcm *dclass = dev_get_drvdata(dev);
	const uint32_t *coefficients;
	uint32_t ctrl;
	int i, err;

	snd_printd("%s: fs:%d ch:%d width:%d format:%s, period bytes:%d buffer bytes: %d\n",
		__func__,
		params_rate(params), params_channels(params),
		params_width(params),
		snd_pcm_format_name(params_format(params)),
		params_period_bytes(params),
		params_buffer_bytes(params));

	err = dclass_pcm_request_dma_irq(substream, dclass, dclass->irq,
					 "dclass-playback");
	if (err < 0) {
		snd_printk("failed to request irq\n");

		return err;
	}

	play->dclass = dclass;
	dclass_playback_hw_free(substream);

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (err < 0) {
		snd_printk("failed to allocated pages for buffers\n");

		goto err_malloc;
	}
	play->pages_allocated = true;

	snd_printd("%s: sample_rate:%d channels:%d format:%s\n", __func__,
		   params_rate(params), params_channels(params),
		   snd_pcm_format_name(params_format(params)));

	play->sample_rate = params_rate(params);
	play->sample_format = params_format(params);
	play->period_size = params_period_bytes(params);
	play->buf_size = params_buffer_bytes(params);

	switch (play->sample_rate) {
	case 8000:
		coefficients = r8000_f256_clk276480;
		ctrl = DCLS_CNTRL_VALUE(0, 0, 0, 32, 108, 1, 0);
		break;
	case 16000:
		coefficients = r16000_f256_clk276480;
		ctrl = DCLS_CNTRL_VALUE(0, 0, 0, 16, 108, 1, 0);
		break;
	case 32000:
		coefficients = r32000_f256_clk276480;
		ctrl = DCLS_CNTRL_VALUE(0, 0, 0, 8, 108, 1, 0);
		break;
	case 48000:
		coefficients = r48000_f192_clk276480;
		ctrl = DCLS_CNTRL_VALUE(0, 0, 0, 4, 144, 1, 0);
		break;
	case 64000:
		coefficients = r64000_f256_clk276480;
		ctrl = DCLS_CNTRL_VALUE(0, 0, 0, 4, 108, 1, 0);
		break;
	default:
		snd_printd("Unsupported sample rate: %d\n", play->sample_rate);
		break;
	}

	for (i = 0; i < 20; i++)
		dclass_write(dclass, DCLS_BQ1_B0 + (i * 4), coefficients[i]);

	ctrl |= 9 << DCLS_CNTRL_FIFO_WMT;

	dclass_write(dclass, DCLS_CNTRL, ctrl);
	dclass_write(dclass, DCLS_MODE, DCLS_MODE_VALUE(0, 3, 121, 0, 0, 0));

	snd_printd("%s: period_size:%d buf_size:%d\n", __func__,
		   params_period_bytes(params), params_buffer_bytes(params));

	play->dma_bytes_total = play->buf_size;

	play->dma_area =
		dma_alloc_coherent(dev, play->dma_bytes_total,
				   &play->dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!play->dma_area) {
		snd_printk("%s: failed to allocate PCM DMA area\n", __func__);

		err = -ENOMEM;
		goto err_pcm_dma;
	}

	return 0;

err_pcm_dma:
	snd_pcm_lib_free_pages(substream);
err_malloc:
	play->ch = 0;
	devm_free_irq(dclass->dev, dclass->irq, (void *)substream);

	return err;
}

static int dclass_pcm_hw_free(struct snd_soc_component *component,
			      struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_printd("%s stream: playback (%p)\n", __func__, substream);

	if (runtime->private_data) {
		struct dclass_playback *play = runtime->private_data;

		play->ch = 0;
		devm_free_irq(play->dclass->dev, play->dclass->irq,
			      (void *)substream);
	}

	return dclass_playback_hw_free(substream);
}

static void dclass_playback_trigger_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	unsigned long flags;
	uint32_t val;

	/* flush and clear async fifo */
	dclass_write(play->dclass, DCLS_DCLIENT, 3);
	udelay(100);
	dclass_write(play->dclass, DCLS_DCLIENT, 0);

	/* PWMSEL, DATASEL=DHUB */
	dclass_write(play->dclass, DCLS_DINP, 1 << 6);
	dclass_write(play->dclass, DCLS_INTENB, 3);

	val = dclass_read(play->dclass, DCLS_CNTRL);
	dclass_write(play->dclass, DCLS_CNTRL, val | (1 << DCLS_CNTRL_ENABLE));

	channel_enable(play->dclass->dhub, play->ch, true);

	spin_lock_irqsave(&play->lock, flags);
	play->started = 1;

	start_dma_if_needed(play);
	spin_unlock_irqrestore(&play->lock, flags);
	snd_printd("%s: dma chid%d start done. Period 0x%X\n",
			__func__, play->ch, play->period_size);
}

static void dclass_playback_trigger_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	uint32_t val;

	play->started = 0;

	dhub_channel_clear_done(play->dclass->dhub, play->ch);
	DhubChannelClear(play->dclass->dhub, play->ch, 0);
	channel_enable(play->dclass->dhub, play->ch, false);

	val = dclass_read(play->dclass, DCLS_CNTRL);
	dclass_write(play->dclass, DCLS_CNTRL, val & ~(1 << DCLS_CNTRL_ENABLE));
	dclass_write(play->dclass, DCLS_DINP, 0);
	dclass_write(play->dclass, DCLS_INTENB, 0);
}

int dclass_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		dclass_playback_trigger_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dclass_playback_trigger_stop(substream);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int dclass_pcm_trigger(struct snd_soc_component *component,
			      struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_substream *s;

	snd_printd("%s stream: playback %s (%p)\n", __func__,
		   cmd == SNDRV_PCM_TRIGGER_START ? "start" : "stop",
		   substream);

	snd_pcm_group_for_each_entry(s, substream)
		dclass_playback_trigger(s, cmd);

	return 0;
}

static int dclass_pcm_prepare(struct snd_soc_component *component,
			      struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	unsigned long flags;

	snd_printd("%s stream: playback (%p)\n", __func__, substream);

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

static snd_pcm_uframes_t
dclass_pcm_pointer(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	uint32_t buf_pos;
	unsigned long flags;

	spin_lock_irqsave(&play->lock, flags);
	buf_pos = play->dma_offset;
	spin_unlock_irqrestore(&play->lock, flags);

	return snd_pcm_indirect_playback_pointer(substream,
						 &play->pcm_indirect, buf_pos);
}

static void dclass_playback_transfer(struct snd_pcm_substream *substream,
				     struct snd_pcm_indirect *rec,
				     size_t bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;

	if (!runtime->dma_area || (rec->hw_data >= play->buf_size))
		return;

	memcpy((void *)(play->dma_area + rec->hw_data),
	       (void *)(runtime->dma_area + rec->sw_data), bytes);
}

static int dclass_pcm_ack(struct snd_soc_component *component,
			  struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dclass_playback *play = runtime->private_data;
	struct snd_pcm_indirect *pcm_indirect = &play->pcm_indirect;

	pcm_indirect->hw_queue_size = play->buf_size;
	snd_pcm_indirect_playback_transfer(substream, pcm_indirect,
					   dclass_playback_transfer);

	return 0;
}

#define PREALLOC_BUFFER (2 * 1024 * 1024)
#define PREALLOC_BUFFER_MAX (2 * 1024 * 1024)

static int dclass_pcm_new(struct snd_soc_component *component,
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

static void dclass_pcm_free(struct snd_soc_component *component,
			    struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static const struct snd_soc_component_driver dclass_pcm_component = {
	.name		= "syna-dclass-pcm",
	.pcm_construct	= dclass_pcm_new,
	.pcm_destruct	= dclass_pcm_free,
	.open		= dclass_pcm_open,
	.close		= dclass_pcm_close,
	.hw_params	= dclass_pcm_hw_params,
	.hw_free	= dclass_pcm_hw_free,
	.prepare	= dclass_pcm_prepare,
	.trigger	= dclass_pcm_trigger,
	.pointer	= dclass_pcm_pointer,
	.ack		= dclass_pcm_ack,
};

static struct snd_soc_dai_driver dclass_pcm_dai = {
	.name     = "dclass-pcm",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 1,
		.rates		= (SNDRV_PCM_RATE_8000  |
				   SNDRV_PCM_RATE_16000 |
				   SNDRV_PCM_RATE_32000 |
				   SNDRV_PCM_RATE_48000 |
				   SNDRV_PCM_RATE_64000),
		.formats	= SNDRV_PCM_FMTBIT_S16_LE,
	},
};

static int dclass_probe(struct platform_device *pdev)
{
	struct dclass_pcm *dclass;
	struct device *dev = &pdev->dev;
	int ret;

	/* Defer probe until dependent soc module/s are probed/initialized */
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	dclass = devm_kzalloc(dev, sizeof(struct dclass_pcm), GFP_KERNEL);
	if (dclass == NULL)
		return -ENOMEM;

	dclass->dev = dev;

	dclass->parent_clk = devm_clk_get(dev, "parent_clk");
	if (IS_ERR(dclass->parent_clk))
		dclass->parent_clk = NULL;

	dclass->regs = devm_platform_ioremap_resource(pdev, 0);
	if (!dclass->regs) {
		dev_err(&pdev->dev, "failed to map registers\n");
		return -EINVAL;
	}

	dclass->irq = platform_get_irq(pdev, 0);
	if (dclass->irq < 0) {
		dev_err(&pdev->dev, "no irq specified\n");
		return dclass->irq;
	}

	dclass->ch = irqd_to_hwirq(irq_get_irq_data(dclass->irq));

	dclass->clk_regs = devm_platform_ioremap_resource(pdev, 1);
	if (dclass->clk_regs && dclass->parent_clk) {
		uint32_t rate, div1, div2 = 0;

		rate = clk_get_rate(dclass->parent_clk);
		div1 = DIV_ROUND_CLOSEST(rate, 27648000);

		if (div1 >= 31) {
			div1 /= 2;
			div2++;
		}

		writel((1 << 15) | (1 << 13) | (div2 << 9) | (div1 << 3),
		       dclass->clk_regs);
	}

	mutex_init(&dclass->dhub_lock);
	of_dma_configure(dev, dev->of_node, true);

	dev_set_drvdata(dev, dclass);

	ret = devm_snd_soc_register_component(dev, &dclass_pcm_component,
					      &dclass_pcm_dai, 1);
	if (ret < 0)
		snd_printk("failed to register component\n");

	return ret;
}

static RET_TYPE dclass_remove(struct platform_device *pdev)
{
	struct dclass_pcm *dclass = dev_get_drvdata(&pdev->dev);

	dclass_write(dclass, DCLS_CNTRL, 0);
	dclass_write(dclass, DCLS_DINP, 0);
	dclass_write(dclass, DCLS_INTENB, 0);

	RETURN_VALUE;
}

static const struct of_device_id dclass_pcm_of_match[] = {
	{.compatible = "syna,myna2-dclass"},
	{}
};
MODULE_DEVICE_TABLE(of, dclass_pcm_of_match);

static struct platform_driver dclass_driver = {
	.driver			= {
		.name		= "syna-dclass-pcm",
		.of_match_table = dclass_pcm_of_match,
	},

	.probe			= dclass_probe,
	.remove			= dclass_remove,
};
module_platform_driver(dclass_driver);

MODULE_AUTHOR("Synaptics");
MODULE_ALIAS("platform:dclass");
MODULE_DESCRIPTION("DClass Driver");
MODULE_LICENSE("GPL v2");
