// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_AIP_H
#define HRX_AIP_H

#include "hrx-drv.h"
#include "hrx-aip-hal.h"

#define AIP_FREEZE_LOCK(m) down(&(m)->aip_freeze_sem)
#define AIP_FREEZE_UNLOCK(m) up(&(m)->aip_freeze_sem)
#define AIP_CHECK_FREEZE(m)                                      \
	do {                                                             \
		if (m->bFreeze) {                                            \
			down(&(m)->aip_freeze_sem);                       \
			up(&(m)->aip_freeze_sem);                       \
		}                                                            \
	} while (0)

#define    bTST(x, b)                    (((x) >> (b)) & 1)
#define HDMI_AUDIO_ID 2
#define I2S_PRI_AUDIO_ID 0
#define HDMI_AUDIO_FB_ID 4
#define HDMI_AUDIO_RX_ID 2


void aip_init(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_config(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_reset(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_start(struct syna_hrx_v4l2_dev *hrx_dev, int nChanNum);
void aip_stop(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_close(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_set_sample_rate(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_set_audio_dec_format(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_set_audio_depth(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_set_audio_hbr(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_set_audio_mono_mode(struct syna_hrx_v4l2_dev *hrx_dev, ENUM_AIP_MONO_MODE eMonoMode);
int aip_select_input_source(struct syna_hrx_v4l2_dev *hrx_dev);

int aip_register_frame_allocation_CB(void *pHandle, aip_frame_alloc_cb pCallbackFn, void *pContextParam);
int aip_register_frame_free_CB(void *pHandle, aip_frame_free_cb pCallbackFn, void *pContextParam);
int aip_register_event_CB(void *pHandle, aip_event_cb pCallbackFn, void *pContextParam);

int aip_create_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_stop_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev);
void aip_call_mute_unmute(struct syna_hrx_v4l2_dev *hrx_dev, int mute);

#endif
