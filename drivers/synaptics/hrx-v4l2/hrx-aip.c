// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-aip.h"
#include "hrx-aip-isr.h"
#include "hrx-aip-i2s.h"
#include "hrx-reg.h"

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>

void aip_init(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int ret;
	AIP_BUFFER aip_buf;
	AIP_FIFO aip_fifo;

	sema_init(&hrx_dev->aip_freeze_sem, 0);
	ret =  aip_buffer_create(&aip_buf, AIP_DEFAULT_BUF_SIZE, AIP_DEFAULT_PAD_SIZE, hrx_dev->aip_mem_list);
	if (ret != HRX_AIP_OK) {
		HRX_LOG(AIP_ERROR, " Buffer is not created");
		ret = HRX_AIP_ENOMEM;
		goto Err1;
	}

	hrx_dev->aip_buf = aip_buf;

	ret = aip_dma_cmd_create_fifo(&aip_fifo, hrx_dev->aip_mem_list);
	if (ret != HRX_AIP_OK) {
		HRX_LOG(AIP_ERROR, " fifo is not created");
		ret = HRX_AIP_ENOMEM;
		goto Err2;
	}

	hrx_dev->aip_fifo = aip_fifo;

	ret = aip_create_isr_task(hrx_dev);
	if (ret < 0) {
		HRX_LOG(AIP_ERROR, " isr task is not created");
		ret = HRX_AIP_ENOMEM;
		goto Err3;
	}

	ret = aip_create_main_task(hrx_dev);
	if (ret < 0) {
		HRX_LOG(AIP_ERROR, " main task is not created");
		ret = HRX_AIP_ENOMEM;
		goto Err4;
	}

	ret = aip_create_watcher_task(hrx_dev);
	if (ret < 0) {
		HRX_LOG(AIP_ERROR, " watcher task is not created");
		ret = HRX_AIP_ENOMEM;
		goto Err5;
	}

	/* chunk size should be divided exactly by AIP buffer size */
	hrx_dev->nDmaChunkSize = AIP_DEFAULT_CHUNK_SIZE;
	hrx_dev->aip_status = AIP_STATUS_INACTIVE;
	hrx_dev->nMode = 1;
	hrx_dev->eSourceType = AIP_SOURCE_AVIF;
	hrx_dev->eMonoMode = AIP_MONO_UNDEF;

	goto Exit;

Err5:
	aip_stop_main_task(hrx_dev);

Err4:
	aip_stop_isr_task(hrx_dev);

Err3:
	aip_dma_cmd_fifo_free(&hrx_dev->aip_fifo, hrx_dev->aip_mem_list);

Err2:
	aip_buffer_free(&hrx_dev->aip_buf, hrx_dev->aip_mem_list);

Err1:
	up(&hrx_dev->aip_freeze_sem);
	sema_init(&hrx_dev->aip_freeze_sem, 0);

Exit:
	return;

}

void aip_config(struct syna_hrx_v4l2_dev *hrx_dev)
{
	aip_reset(hrx_dev);
	aip_set_sample_rate(hrx_dev);
	aip_set_audio_dec_format(hrx_dev);
	aip_set_audio_depth(hrx_dev);
	aip_set_audio_hbr(hrx_dev);
	aip_select_input_source(hrx_dev);
	i2s_rx_mic3_flush(I2S_HDMIRX_BASE);
}

void aip_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;

	rc = HRX_AIP_OK;

    /* TODO: */

	hrx_dev->aip_status = AIP_STATUS_ACTIVE;

    /* clear init time stamp */
	hrx_dev->bInitTimeUpdated = false;

	return;
}

void aip_start(struct syna_hrx_v4l2_dev *hrx_dev, int nChanNum)
{
	unsigned int nPairNum;
	int rc;

	aip_config(hrx_dev);

	rc = HRX_AIP_OK;

	if (!(nChanNum == 2 || nChanNum == 6 || nChanNum == 8)) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	if (hrx_dev->eSourceType == AIP_SOURCE_INVALID) {
		HRX_LOG(AIP_ERROR, "invalid input source: %d\n", hrx_dev->eSourceType);
		rc = HRX_AIP_EUNCONFIG;
		goto EXIT;
	}

	if (nChanNum == 2)
		nPairNum = 1;
	else
		nPairNum = AIP_MAX_IN_PAIR_NR;
	/* Make nPairNum as 1 for dolphin, since dolphin
	 * has only one dhub (interleave mode)
	 */
	if (hrx_dev->eSourceType == AIP_SOURCE_AVIF) {
		nPairNum = 1;
		aip_buffer_set_chan_num(&hrx_dev->aip_buf, nChanNum); /* TODO: may not be required */
	}

	if (nPairNum == AIP_MAX_IN_PAIR_NR)
		aip_bufer_split_to_fourpairs(&hrx_dev->aip_buf, nChanNum);

    /* TODO: */
	hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
	hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;

	hrx_dev->nInputThres = DEFAULT_BURST_SIZE;

	i2s_rx_mic3_start(I2S_HDMIRX_BASE, AUDCH_CTRL_DEBUGEN_DISABLE, &i2sConfig, nChanNum);
	i2s_rx_mic3_clock_gate_enable(0);

    /* enable interrupt */
	rc = aip_isr_enable(hrx_dev);
	if (rc < 0)
		HRX_LOG(AIP_ERROR, "aip_isr_enable failed\n");

	hrx_dev->aip_status = AIP_STATUS_START;

	i2s_rx_mic3_flush(I2S_HDMIRX_BASE);

EXIT:
	return;
}

void aip_call_mute_unmute(struct syna_hrx_v4l2_dev *hrx_dev, int mute)
{
	void *pCallbackCtx;
	int rc;

	rc = HRX_AIP_OK;

	if (mute) {
		pCallbackCtx = hrx_dev->pEventCtx;
		rc = hrx_dev->pEventCB(AIP_EVENT_MUTE, NULL, pCallbackCtx);
		AIP_CHKRC(rc);
	} else {
		pCallbackCtx = hrx_dev->pEventCtx;
		rc = hrx_dev->pEventCB(AIP_EVENT_UNMUTE, NULL, pCallbackCtx);
		AIP_CHKRC(rc);
	}
}

void aip_stop(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;

	rc = HRX_AIP_OK;

	if (hrx_dev->aip_status == AIP_STATUS_STOP) {
		rc = HRX_AIP_EWRONGSTATE;
		HRX_LOG(AIP_ERROR, "Try to stop aip when it's already stop, status %d\n",
			hrx_dev->aip_status);
		goto EXIT;
	}

	hrx_dev->aip_status = AIP_STATUS_STOP;

    /* post the semophore in case main task is waiting it */
	up(&hrx_dev->aip_main_sem);

    /* disable interrupt */
	aip_isr_disable(hrx_dev);
	i2s_rx_mic3_flush(I2S_HDMIRX_BASE);

    /* reset dma commnad fifo */
	aip_dma_cmd_fifo_reset(hrx_dev->aip_fifo.pDmaCmdFifo);

    /* clear internal buffer */
	aip_buffer_reset(&hrx_dev->aip_buf);

    /* clear init time stamp */
	hrx_dev->bInitTimeUpdated = false;

EXIT:
	return;
}

void aip_close(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_dev->aip_status = AIP_STATUS_CLOSE;
	up(&hrx_dev->aip_freeze_sem);
	sema_init(&hrx_dev->aip_freeze_sem, 0);
	aip_stop_watcher_task(hrx_dev);
	aip_stop_main_task(hrx_dev);
	aip_stop_isr_task(hrx_dev);
}

int aip_set_sample_rate(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	unsigned int nTimeStampDelta;
	unsigned int nFs;

	nFs = hrx_dev->in_data.sample_rate;

	rc = HRX_AIP_OK;

	if (nFs > 192000 || nFs == 0) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	rc = aip_buffer_set_samplerate(&hrx_dev->aip_buf, nFs);

	nTimeStampDelta = ((hrx_dev->nDmaChunkSize >> 3) * (u64)AIP_SYS_CLK_90K) / nFs;

	rc = aip_buffer_set_timestamp_delta(&hrx_dev->aip_buf, nTimeStampDelta);

EXIT:
	return rc;
}

int aip_set_audio_dec_format(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	unsigned int nFormat;

	nFormat = hrx_dev->in_data.dec_type;

	rc = HRX_AIP_OK;

	rc = aip_buffer_set_audio_dec_format(&hrx_dev->aip_buf, nFormat);

	return rc;
}

int aip_set_audio_depth(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	unsigned int nBitDepth;

	rc = HRX_AIP_OK;

	nBitDepth = hrx_dev->in_data.bit_depth;

	if (nBitDepth != 16 && nBitDepth != 24 && nBitDepth != 32) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	rc = aip_buffer_set_audio_bitdepth(&hrx_dev->aip_buf, nBitDepth);

EXIT:
	return rc;
}

int aip_set_audio_hbr(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	unsigned int nIsHbr;

	rc = HRX_AIP_OK;
	nIsHbr = 0;
	//TBD::i2sConfig.hdmi_rx_hd_en = nIsHbr ? 1:0;       /* change to use AIO_SetHbr() */

	rc = aip_buffer_set_audio_hbr(&hrx_dev->aip_buf, nIsHbr);

	return rc;
}

int aip_set_audio_mono_mode(struct syna_hrx_v4l2_dev *hrx_dev, ENUM_AIP_MONO_MODE eMonoMode)
{
	int rc;

	rc = HRX_AIP_OK;

	if (eMonoMode != AIP_MONO_LEFT &&
		eMonoMode != AIP_MONO_RGHT &&
		eMonoMode != AIP_STEREO) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	hrx_dev->eMonoMode = eMonoMode;

EXIT:
	return rc;
}

int aip_select_input_source(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	int nInputSource;

	rc = HRX_AIP_OK;
	nInputSource = AIP_SOURCE_AVIF;
//TBC::
	hrx_dev->eSourceType = nInputSource;
	hrx_dev->bSourceTypeUpdated = true;
    /* TODO: */

	return rc;
}

int aip_register_frame_allocation_CB(
	void *pHandle,
	aip_frame_alloc_cb pCallbackFn,
	void *pContextParam)
{
	int rc;
	struct syna_hrx_v4l2_dev *hrx_dev;

	rc = HRX_AIP_OK;

	if (pCallbackFn == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	hrx_dev = (struct syna_hrx_v4l2_dev *)pHandle;

	hrx_dev->pFrameAllocCB = pCallbackFn;
	hrx_dev->pFrameAllocCtx = pContextParam;

EXIT:
	return rc;
}

int aip_register_frame_free_CB(
	void *pHandle,
	aip_frame_free_cb pCallbackFn,
	void *pContextParam)
{
	int rc;
	struct syna_hrx_v4l2_dev *hrx_dev;

	rc = HRX_AIP_OK;

	if (pCallbackFn == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	hrx_dev = (struct syna_hrx_v4l2_dev *)pHandle;

	hrx_dev->pFrameFreeCB = pCallbackFn;
	hrx_dev->pFrameFreeCtx = pContextParam;

EXIT:
	return rc;
}

int aip_register_event_CB(
	void *pHandle,
	aip_event_cb pCallbackFn,
	void *pContextParam)
{
	int rc;
	struct syna_hrx_v4l2_dev *hrx_dev;

	rc = HRX_AIP_OK;

	if (pCallbackFn == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	hrx_dev = (struct syna_hrx_v4l2_dev *)pHandle;

	hrx_dev->pEventCB = pCallbackFn;
	hrx_dev->pEventCtx = pContextParam;

EXIT:
	return rc;
}

static int aip_watcher_task(void *param)
{
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *)param;
	u32 pAudErr;
	ENUM_HRX_STATUS hrx_status;

	while (!kthread_should_stop()) {
		msleep_interruptible(1000);

		/* No need to watch if AIP is not running */
		if ((hrx_dev->aip_status != AIP_STATUS_ACTIVE) || (hrx_dev->aip_status != AIP_STATUS_START))
			continue;

		hrx_get_aud_err(hrx_dev, &pAudErr);
		if (!pAudErr) {
			aip_alsa_get_hrx_status(&hrx_status);
			if (hrx_status == HRX_STATUS_VIP_STABLE) {
				if ((hrx_dev->hrx_alsa_state == HRX_ALSA_STREAMING_ON) && (hrx_dev->aip_status == AIP_STATUS_START)) {
					hrx_dev->aip_hold_enable = 0;
					if (hrx_dev->audio_params.N == 0) {
						/* Resetting audio since N is not detected */
						hrx_audio_reset(hrx_dev);
					}
				}
			} else {
				if ((hrx_dev->hrx_alsa_state == HRX_ALSA_STREAMING_ON) && (hrx_dev->aip_status == AIP_STATUS_START))
					hrx_dev->aip_hold_enable = 1;
			}
			continue;
		} else {
			if ((hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) && (hrx_dev->hrx_alsa_state == HRX_ALSA_STREAMING_ON)) {
				HRX_LOG(AIP_DEBUG, "Aud Err: Stopping Aip\n");
				aip_stop(hrx_dev);
			}
			hrx_toggle_hpd(hrx_dev, 0);
			msleep(200);
			hrx_toggle_hpd(hrx_dev, 1);
			msleep(200);
			if ((hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) && (hrx_dev->hrx_alsa_state == HRX_ALSA_STREAMING_ON)) {
				HRX_LOG(AIP_DEBUG, "Aud Err: Starting Aip\n");
				aip_start(hrx_dev, hrx_dev->in_data.channels);
			}
		}
	}
	return 0;
}

int aip_create_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev)
{

	hrx_dev->aip_watcher_task = kthread_run(aip_watcher_task, hrx_dev, "AIP Watcher Thread");
	if (IS_ERR(hrx_dev->aip_watcher_task))
		return -1;

	return 0;
}

void aip_stop_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	//TODO: Fix to exit from thread
	if (hrx_dev->aip_watcher_task)
		kthread_stop(hrx_dev->aip_watcher_task);
}
