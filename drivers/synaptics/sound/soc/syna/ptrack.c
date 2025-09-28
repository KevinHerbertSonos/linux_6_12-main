// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/of_device.h>
#include <linux/sizes.h>
#include <linux/timer.h>

#include <linux/err.h>

#include <linux/kfifo.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "aio_hal.h"
#include "avio_dhub_drv.h"
#include "avio_common.h"
#include "ptrack.h"

#define PTRACK_FIFO_SIZE 128
#define PTRACK_KFIFO_SIZE (10 * PTRACK_FIFO_SIZE)
#define PCM_PERIOD_SIZE_ALSA  (SZ_4K)

#define CLEAR_STATUS 0x7f

struct ptrack_rt {
	struct platform_device *pdev;
	struct snd_pcm_substream *substream;
	unsigned int buffer_offset;
	unsigned int buffer_size;
	struct kfifo fifo;
	struct work_struct work;
	struct ptrack_cfg *ptrack;
};

struct ptrack_cfg {
	struct platform_device *pdev;
	struct ptrack_rt *ptrack_rt_data;
	void *regs;
	void *ctrl_regs;
	void *irq_regs;
	struct clk *clk;
	int irq;
	unsigned int srr_mfin;
	unsigned int srr_nfout;
};

static const char * const ptrack_srr_type_text[] = {
	"I2S1_BCLKIO_DI",
	"I2S2_BCLKIO_DI",
	"I2S3_BCLKIO_DI",
	"I2S4_BCLKIO_DI",
	"I2S5_BCLKIO_DI",
	"PDM_CLKIO_DI",
	"I2S1_MCLK_DI",
	"I2S2_MCLK_DI",
	"aioSysClk",

	"I2S1_LRCLKIO_DI",
	"I2S2_LRCLKIO_DI",
	"I2S3_LRCLKIO_DI",
	"I2S4_LRCLKIO_DI",
	"I2S5_LRCLKIO_DI",
	"drt_int_fsi",
	"USB_SOF"
};

static SOC_ENUM_SINGLE_EXT_DECL(ptrack_srr_type, ptrack_srr_type_text);
static const DECLARE_TLV_DB_MINMAX(ptrack_srr_div, 0, 8096);

static int ptrack_mfin_control_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned int srr = (ptrack->srr_mfin >> 1) & 0x7f; //4bits for tic_clk, 3bits for tic_crl

	if (ptrack->srr_mfin & 1)   // if tic_ctl is selected
		srr = (srr >> 4) + 9; //   shift value and map to continuous enum
	ucontrol->value.enumerated.item[0] = srr;

	return 0;
}

static int ptrack_mfin_control_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned int val = ucontrol->value.enumerated.item[0];

	if (val >= 9)
		ptrack->srr_mfin = (ptrack->srr_mfin & 0xffffff00) |
				   ((val - 9) << 5) | 1;
	else
		ptrack->srr_mfin = (ptrack->srr_mfin & 0xffffff00) | (val << 1);

	return 0;
}

static int ptrack_nfout_control_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned int srr = (ptrack->srr_nfout >> 1) & 0x7f; //4bits for tic_clk, 3bits for tic_crl

	if (ptrack->srr_nfout & 1)   // if tic_ctl is selected
		srr = (srr >> 4) + 9; //   shift value and map to continuous enum
	ucontrol->value.enumerated.item[0] = srr;

	return 0;
}

static int ptrack_nfout_control_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned int val = ucontrol->value.enumerated.item[0];

	if (val >= 9)
		ptrack->srr_nfout = (ptrack->srr_nfout & 0xffffff00) |
				   ((val - 9) << 5) | 1;
	else
		ptrack->srr_nfout = (ptrack->srr_nfout & 0xffffff00) | (val << 1);

	return 0;
}

static int ptrack_mfin_div_control_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);

	ucontrol->value.enumerated.item[0] = ptrack->srr_mfin >> 8;

	return 0;
}

static int ptrack_mfin_div_control_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);

	ptrack->srr_mfin = (ptrack->srr_mfin & 0xff) +
			(ucontrol->value.enumerated.item[0] << 8);
	return 0;
}

static int ptrack_nfout_div_control_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);

	ucontrol->value.enumerated.item[0] = ptrack->srr_nfout >> 8;

	return 0;
}

static int ptrack_nfout_div_control_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(cpu_dai);

	ptrack->srr_nfout = (ptrack->srr_nfout & 0xff) +
			(ucontrol->value.enumerated.item[0] << 8);

	return 0;
}

static struct snd_kcontrol_new ptrack_mixer_ctrls[] = {
	SOC_ENUM_EXT("MFIN SELECT", ptrack_srr_type,
			ptrack_mfin_control_get, ptrack_mfin_control_put),
	SOC_ENUM_EXT("NFOUT SELECT", ptrack_srr_type,
			ptrack_nfout_control_get, ptrack_nfout_control_put),
	SOC_SINGLE_EXT_TLV("MFIN DIV", 0, 0, 8096, 0,
			ptrack_mfin_div_control_get, ptrack_mfin_div_control_put,
			ptrack_srr_div),
	SOC_SINGLE_EXT_TLV("NFOUT DIV", 0, 0, 8096, 0,
			ptrack_nfout_div_control_get, ptrack_nfout_div_control_put,
			ptrack_srr_div),
};

static const struct snd_pcm_hardware ptrack_capture_hw = {
	.info			= (SNDRV_PCM_INFO_MMAP
				   | SNDRV_PCM_INFO_INTERLEAVED
				   | SNDRV_PCM_INFO_MMAP_VALID
				   | SNDRV_PCM_INFO_PAUSE
				   | SNDRV_PCM_INFO_RESUME),
	.formats		= (SNDRV_PCM_FMTBIT_S32_LE),
	.rates			= (SNDRV_PCM_RATE_CONTINUOUS
				   | SNDRV_PCM_RATE_KNOT),
	.channels_min		= 1,
	.channels_max		= 1,
	.buffer_bytes_max	= 255 * PCM_PERIOD_SIZE_ALSA,
	.period_bytes_min	= PTRACK_KFIFO_SIZE * 4,
	.period_bytes_max	= PCM_PERIOD_SIZE_ALSA * 4,
	.periods_min		= 32,
	.periods_max		= 255,
	.fifo_size		= 0
};

static void capture_work(struct work_struct *work)
{
	struct ptrack_rt *ptrack_rt_data =
			container_of(work, struct ptrack_rt, work);
	struct snd_pcm_runtime *runtime = ptrack_rt_data->substream->runtime;
	unsigned int *dst = (unsigned int *)(runtime->dma_area + ptrack_rt_data->buffer_offset);
	unsigned int data;

	while (kfifo_out(&ptrack_rt_data->fifo, &data, sizeof(int))) {
		*dst++ = data;
		ptrack_rt_data->buffer_offset += 4;
		if (ptrack_rt_data->buffer_offset + 3 >= ptrack_rt_data->buffer_size) {
			ptrack_rt_data->buffer_offset = 0;
			dst = (unsigned int *)runtime->dma_area;
		}
	};
	snd_pcm_period_elapsed(ptrack_rt_data->substream);
}

static inline void ptrack_writel(void *base, unsigned int val, unsigned int offset)
{
	writel(val, base + offset);
}

static inline unsigned int ptrack_readl(void *base, unsigned int offset)
{
	return readl(base + offset);
}

static irqreturn_t ptrack_isr(int irq, void *dev_id)
{
	struct snd_pcm_substream *substream = (struct snd_pcm_substream *)dev_id;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ptrack_rt *ptrack_rt_data = runtime->private_data;
	struct ptrack_cfg *cfg = ptrack_rt_data->ptrack;
	struct device *dev = &ptrack_rt_data->pdev->dev;
	void *base = cfg->regs;
	int i = 0;
	unsigned int status;
	unsigned int buf[PTRACK_FIFO_SIZE];

	status = ptrack_readl(base, PTRACK_STATUS_OFFS);
	status = ptrack_readl(base, PTRACK_STATUS_OFFS);
	if (status & PTRACK_STATUS_AFIFO_WM_STAT_MASK) {
		unsigned int fifo_entries = 0;

		fifo_entries  = ptrack_readl(base, PTRACK_BUS_IF_CFG_OFFS);
		fifo_entries  = ptrack_readl(base, PTRACK_BUS_IF_CFG_OFFS);
		fifo_entries &= PTRACK_BUS_IF_CFG_FIFO_LEVEL_MASK;
		fifo_entries >>= PTRACK_BUS_IF_CFG_FIFO_LEVEL_BIT;
		if (fifo_entries == 0 && (status & PTRACK_STATUS_AFIFO_OVERRUN_STAT_MASK)) {
			dev_warn(dev, "%s fifo overflow", __func__);
			fifo_entries = PTRACK_FIFO_SIZE;
		}
		while (fifo_entries && i < PTRACK_FIFO_SIZE) {
			buf[i++] = ptrack_readl(base, PTRACK_TOP_FDATA_OFFS) >> 1;
			fifo_entries--;
		};

		status = PTRACK_STATUS_AFIFO_WM_STAT_SET(1); //clear int
		ptrack_writel(base, status, PTRACK_STATUS_OFFS);

		kfifo_in(&ptrack_rt_data->fifo, buf, i * sizeof(int));
		schedule_work(&ptrack_rt_data->work);

	}
	return IRQ_HANDLED;
}

static int ptrack_prepare_device(struct device *dev)
{
	struct ptrack_cfg *cfg = dev_get_drvdata(dev);
	void *base = cfg->regs;
	unsigned int val, ret;
	unsigned int params[16];
	unsigned int mfin[2];
	unsigned int nfout[2];

	ret = of_property_read_u32_array(dev->of_node, "params", params, 16);
	if (ret)
		return ret;
	ret = of_property_read_u32_array(dev->of_node, "mfin", mfin, 2);
	if (ret)
		return ret;
	ret = of_property_read_u32_array(dev->of_node, "nfout", nfout, 2);
	if (ret)
		return ret;

	val = PTRACK_PHASE_MEAS_CFG_PHASE_SCALE_SET(params[0]) |
	      PTRACK_PHASE_MEAS_CFG_T_RATIO_SHIFT_SET(params[1]) |
	      PTRACK_PHASE_MEAS_CFG_FRAC_PHASE_CALC_EN_SET(1) |
	      PTRACK_PHASE_MEAS_CFG_T_RATIO_CALC_EN_SET(0);
	ptrack_writel(base, val, PTRACK_PHASE_MEAS_CFG_OFFS);

	val = PTRACK_DPLL_CFG_S_ERR_SHIFT_SET(0) |
	      PTRACK_DPLL_CFG_DPLL_CORE1_EN_SET(1) |
	      PTRACK_DPLL_CFG_ERR_SRC_SET(0) |
	      PTRACK_DPLL_CFG_LOCK_WAIT_SET(2);
	ptrack_writel(base, val, PTRACK_DPLL_CFG_OFFS);

	/* set Error Threshold */
	val = PTRACK_ERR_TRSH_LOW_ERR_TH_SET(params[2]) |
	      PTRACK_ERR_TRSH_HIGH_ERR_TH_SET(params[3]);
	ptrack_writel(base, val, PTRACK_ERR_TRSH_OFFS);

	/* parameter used by Matlab simulation */
	val = PTRACK_GEAR0_KP_SHIFT_SET(params[4]) |
	      PTRACK_GEAR0_KF_SHIFT_SET(params[5]) |
	      PTRACK_GEAR0_ORDER_SET(1)            |
	      PTRACK_GEAR0_GEAR_LEN_SET(16);
	ptrack_writel(base, val, PTRACK_GEAR0_OFFS);

	val = PTRACK_GEAR0_KP_SHIFT_SET(params[6]) |
	      PTRACK_GEAR0_KF_SHIFT_SET(params[7]) |
	      PTRACK_GEAR0_ORDER_SET(1)            |
	      PTRACK_GEAR0_GEAR_LEN_SET(16);
	ptrack_writel(base, val, PTRACK_GEAR1_OFFS);

	val = PTRACK_GEAR0_KP_SHIFT_SET(params[8]) |
	      PTRACK_GEAR0_KF_SHIFT_SET(params[9]) |
	      PTRACK_GEAR0_ORDER_SET(1)            |
	      PTRACK_GEAR0_GEAR_LEN_SET(16);
	ptrack_writel(base, val, PTRACK_GEAR2_OFFS);

	val = PTRACK_GEAR0_KP_SHIFT_SET(params[10]) |
	      PTRACK_GEAR0_KF_SHIFT_SET(params[11]) |
	      PTRACK_GEAR0_ORDER_SET(1)            |
	      PTRACK_GEAR0_GEAR_LEN_SET(16);
	ptrack_writel(base, val, PTRACK_GEAR3_OFFS);

	val = PTRACK_GEAR0_KP_SHIFT_SET(params[12]) |
	      PTRACK_GEAR0_KF_SHIFT_SET(params[13]) |
	      PTRACK_GEAR0_ORDER_SET(1)            |
	      PTRACK_GEAR0_GEAR_LEN_SET(16);
	ptrack_writel(base, val, PTRACK_GEAR4_OFFS);

	val = ptrack_readl(base, PTRACK_BUS_IF_CFG_OFFS);
	val |= PTRACK_BUS_IF_CFG_BUS_IF_WM_LEVEL_SET(params[14]);
	val |= PTRACK_BUS_IF_CFG_BUS_IF_DAT_SRC_SET(params[15]);
	ptrack_writel(base, val, PTRACK_BUS_IF_CFG_OFFS);

	/* clear Lock bit */
	val = CLEAR_STATUS;
	ptrack_writel(base, val, PTRACK_STATUS_OFFS);

	val = PTRACK_IE_AFIFO_WM__IE_SET(1);
	ptrack_writel(base, val, PTRACK_IE_OFFS);

	dev_dbg(dev, "mfin %u, %u\n", mfin[0], mfin[1]);
	cfg->srr_mfin  = mfin[0] | (mfin[1] << 8);
	cfg->srr_nfout = nfout[0] | (nfout[1] << 8);

	return 0;
}

static int ptrack_start_device(struct device *dev)
{
	struct ptrack_cfg *cfg = dev_get_drvdata(dev);
	unsigned int val;
	void *base = cfg->regs;

	/* setup clocks */
	ptrack_writel(base, cfg->srr_mfin, PTRACK_TOP_MFIN_SRR_OFFS);
	ptrack_writel(base, cfg->srr_nfout, PTRACK_TOP_NFOUT_SRR_OFFS);

	dev_dbg(dev, "%s called\n", __func__);
	// TODO set INIT_T_RATIO according to mfin clock (mfin/opClk * 2^26)

	val = 0x5; // bit 0 srr_en, bit2 ahb_en
	ptrack_writel(base, val, PTRACK_TOP_CTRL_OFFS);

	/* Enable Ptrack */
	val = ptrack_readl(base, PTRACK_BUS_IF_CFG_OFFS);
	val |= PTRACK_BUS_IF_CFG_PTRACK_EN_MASK;
	ptrack_writel(base, val, PTRACK_BUS_IF_CFG_OFFS);

	/* enable calc */
	val = PTRACK_TIC_GEN_CFG_MFIN_DIV_SET(0) |
	      PTRACK_TIC_GEN_CFG_NFOUT_DIV_SET(0) |
	      PTRACK_TIC_GEN_CFG_MFIN_SYNC_EN_SET(1) |
	      PTRACK_TIC_GEN_CFG_FOUT_SYNC_EN_SET(1) |
	      PTRACK_TIC_GEN_CFG_DMFIN_IS_OP_CLK_SET(0) |
	      PTRACK_TIC_GEN_CFG_FOUT_EN_SET(1) |
	      PTRACK_TIC_GEN_CFG_MFIN_EN_SET(1);

	ptrack_writel(base, val, PTRACK_TIC_GEN_CFG_OFFS);

	return 0;
}

static int ptrack_stop_device(struct device *dev)
{
	struct ptrack_cfg *cfg = dev_get_drvdata(dev);
	void *base = cfg->regs;
	unsigned int val;

	dev_dbg(dev, "%s called\n", __func__);
	/* Disable Ptrack */
	val = ptrack_readl(base, PTRACK_BUS_IF_CFG_OFFS);
	val &= ~PTRACK_BUS_IF_CFG_PTRACK_EN_MASK;
	ptrack_writel(base, val, PTRACK_BUS_IF_CFG_OFFS);

	return 0;
}

static int ptrack_open(struct snd_soc_component *component,
		       struct snd_pcm_substream *substream)
{
	struct device *dev = component->dev;
	struct ptrack_cfg *ptrack = dev_get_drvdata(dev);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ptrack_rt *ptrack_rt_data;

	dev_dbg(dev, " %s stream 0x%p 0x%p\n", __func__, substream, ptrack);

	if (ptrack->ptrack_rt_data != NULL)
		return -EBUSY;
	ptrack_rt_data = kzalloc(sizeof(struct ptrack_rt), GFP_KERNEL);
	if (ptrack_rt_data == NULL)
		return -ENOMEM;

	ptrack->ptrack_rt_data = ptrack_rt_data;
	ptrack->ptrack_rt_data->ptrack = ptrack;
	runtime->private_data = ptrack_rt_data;
	runtime->hw = ptrack_capture_hw;

	ptrack_rt_data->pdev = ptrack->pdev;
	ptrack_rt_data->buffer_offset = 0;
	ptrack_rt_data->substream = substream;
	if (kfifo_alloc(&ptrack_rt_data->fifo,
			PTRACK_KFIFO_SIZE * sizeof(int),
			GFP_KERNEL))
		return -ENOMEM;
	INIT_WORK(&ptrack_rt_data->work, capture_work);

	return 0;
}

static int ptrack_close(struct snd_soc_component *component,
			struct snd_pcm_substream *substream)
{
	struct device *dev = component->dev;
	struct ptrack_cfg *ptrack = dev_get_drvdata(dev);

	struct ptrack_rt *ptrack_rt_data = ptrack->ptrack_rt_data;

	dev_dbg(dev, " %s stream 0x%p 0x%p\n", __func__, substream, ptrack_rt_data);

	if (ptrack_rt_data) {
		cancel_work_sync(&ptrack_rt_data->work);
		kfifo_free(&ptrack_rt_data->fifo);
		kfree(ptrack_rt_data);

		ptrack->ptrack_rt_data = NULL;
	}

	return 0;
}

static int ptrack_hw_params(struct snd_soc_component *component,
			    struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
	int ret;
	struct device *dev = component->dev;
	struct ptrack_cfg *ptrack = dev_get_drvdata(dev);

	struct ptrack_rt *ptrack_rt_data = ptrack->ptrack_rt_data;
	const size_t pcm_buffer_size_bytes = params_buffer_bytes(params);

	dev_dbg(dev, " %s stream %p\n", __func__, substream);

	ptrack_rt_data->buffer_size = pcm_buffer_size_bytes;

	ret = devm_request_irq(dev, ptrack->irq, ptrack_isr, 0, "ptrack-pcm",
			       (void *)substream);
	if (ret < 0) {
		dev_err(dev, "%s: devm_request_irq failed %d\n", __func__, ret);
		return ret;
	}
	ret = snd_pcm_lib_alloc_vmalloc_buffer(substream, pcm_buffer_size_bytes);
	if (ret < 0) {
		devm_free_irq(dev, ptrack->irq, (void *)substream);
		return ret;
	}

	/* CONFIGURATION */

	return 0;
}

static int ptrack_hw_free(struct snd_soc_component *component,
			  struct snd_pcm_substream *substream)
{
	struct device *dev = component->dev;
	struct ptrack_cfg *ptrack = dev_get_drvdata(dev);
	struct ptrack_rt *ptrack_rt_data = ptrack->ptrack_rt_data;

	dev_dbg(dev, " %s stream %p\n", __func__, substream);
	devm_free_irq(dev, ptrack->irq, (void *)substream);
	cancel_work_sync(&ptrack_rt_data->work);
	snd_pcm_lib_free_vmalloc_buffer(substream);
	return 0;
}

static snd_pcm_uframes_t
ptrack_pointer(struct snd_soc_component *component,
	       struct snd_pcm_substream *ss)
{
	if (ss->stream == SNDRV_PCM_STREAM_CAPTURE) {
		struct snd_pcm_runtime *runtime = ss->runtime;
		struct ptrack_rt *ptrack_rt_data = runtime->private_data;

		return bytes_to_frames(runtime, ptrack_rt_data->buffer_offset);
	}
	return 0;
}

static const struct snd_soc_component_driver ptrack_component = {
	.name		= "syna-ptrack-pcm",
	.open		= ptrack_open,
	.close		= ptrack_close,
	.hw_params	= ptrack_hw_params,
	.hw_free	= ptrack_hw_free,
	.pointer	= ptrack_pointer,
};

static int ptrack_dai_trigger(struct snd_pcm_substream *ss,
			      int cmd,
			      struct snd_soc_dai *dai)
{
	struct ptrack_cfg *ptrack = snd_soc_dai_get_drvdata(dai);
	struct device *dev = &ptrack->pdev->dev;

	dev_dbg(dev, "%s: end 0x%p 0x%p\n", __func__, ss, dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* START */
		 ptrack_start_device(dev);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* STOP */
		ptrack_stop_device(dev);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ptrack_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, ptrack_mixer_ctrls,
				 ARRAY_SIZE(ptrack_mixer_ctrls));

	return 0;
}

static struct snd_soc_dai_ops ptrack_dai_ops = {
	.trigger = ptrack_dai_trigger,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0))
	.probe = ptrack_dai_probe,
#endif
};

static struct snd_soc_dai_driver ptrack0_dai = {
	.name = "ptrack0_dai",
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
	.probe = ptrack_dai_probe,
#endif
	.capture = {
		.stream_name = "Ptrack0-Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &ptrack_dai_ops,
};

static struct snd_soc_dai_driver ptrack1_dai = {
	.name = "ptrack1_dai",
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
	.probe = ptrack_dai_probe,
#endif
	.capture = {
		.stream_name = "Ptrack1-Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &ptrack_dai_ops,
};

static int ptrack_probe(struct platform_device *pdev)
{
	struct ptrack_cfg *ptrack;
	struct device *dev = &pdev->dev;
	struct snd_soc_dai_driver *pptrack_dai;
	int instance;
	struct resource *res;
	int ret;

	ret = of_property_read_u32(dev->of_node, "instance", &instance);
	snd_printd("ptrack probe %d\n", instance);

	if (instance == 1)
		pptrack_dai = &ptrack1_dai;
	else
		pptrack_dai = &ptrack0_dai;

	ptrack = devm_kzalloc(dev, sizeof(struct ptrack_cfg), GFP_KERNEL);
	if (ptrack == NULL)
		return -ENOMEM;

	ptrack->pdev = pdev;

	ptrack->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ptrack->regs))
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res != NULL) {
		unsigned int val;

		ptrack->ctrl_regs = devm_ioremap(dev, res->start,
						 resource_size(res));
		if (ptrack->ctrl_regs == NULL)
			return -EINVAL;

		val = readl(ptrack->ctrl_regs);
		val |= 0x3 << (2 * instance); //sysClk_en | clk_en
		writel(val, ptrack->ctrl_regs);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (res != NULL) {
		unsigned int val;

		ptrack->irq_regs = devm_ioremap(dev, res->start,
						resource_size(res));
		if (ptrack->irq_regs == NULL)
			return -EINVAL;

		val = readl(ptrack->irq_regs);
		val |= 0x1 << (2 + instance);
		writel(val, ptrack->irq_regs);
	}

	ptrack->clk = devm_clk_get_optional(&pdev->dev, NULL);
	if (IS_ERR(ptrack->clk))
		return PTR_ERR(ptrack->clk);
	clk_prepare_enable(ptrack->clk);

	ptrack->irq = platform_get_irq(pdev, 0);
	if (ptrack->irq < 0)
		return ptrack->irq;

	dev_set_drvdata(&pdev->dev, ptrack);

	ret = ptrack_prepare_device(dev);
	if (ret < 0) {
		dev_err(dev, "%s: ptrack_prepare_device failed\n", __func__);
		return ret;
	}

	ret = devm_snd_soc_register_component(&pdev->dev,
					&ptrack_component,
					pptrack_dai, 1);
	if (ret < 0)
		dev_err(dev, "%s can not do snd soc register\n", __func__);

	return ret;
}

static const struct of_device_id ptrack_of_match[] = {
	{.compatible = "syna,ptrack"},
	{}
};
MODULE_DEVICE_TABLE(of, ptrack_of_match);

static struct platform_driver ptrack_driver = {
	.driver			= {
		.name		= "syna-ptrack-pcm",
		.of_match_table = ptrack_of_match,
	},

	.probe			= ptrack_probe,
};
module_platform_driver(ptrack_driver);

MODULE_AUTHOR("Synaptics");
MODULE_DESCRIPTION("Ptrack Driver");
MODULE_LICENSE("GPL v2");
