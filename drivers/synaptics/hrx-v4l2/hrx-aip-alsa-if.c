// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"
#include "hdmi_in_if.h"
#include "hrx-aip.h"

static int aip_alsa_config(AIP_ALSA_CMD_DATA *data)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev_glbl->dev);

	if (data == NULL) {
		HRX_LOG(AIP_ERROR, "Data is NULL :%d\n", __LINE__);
		return -1;
	}

	memcpy(&hrx_dev->in_data, data, sizeof(AIP_ALSA_CMD_DATA));

	aip_config(hrx_dev);

	aip_register_frame_allocation_CB(hrx_dev, data->allocFn, data->hdmi);
	aip_register_frame_free_CB(hrx_dev, data->freeFn, data->hdmi);
	aip_register_event_CB(hrx_dev, data->eventFn, data->hdmi);

	return 0;
}

static int aip_alsa_start(void)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev_glbl->dev);
	hrx_dev->hrx_alsa_state = HRX_ALSA_STREAMING_ON;

	aip_start(hrx_dev, hrx_dev->in_data.channels);
	return 0;
}

static int aip_alsa_stop(void)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev_glbl->dev);

	aip_stop(hrx_dev);

	hrx_dev->hrx_alsa_state = HRX_ALSA_STREAMING_OFF;

	return 0;
}

static int aip_alsa_close(void)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev_glbl->dev);

	return 0;
}

int aip_alsa_get_hrx_status(ENUM_HRX_STATUS *hrx_status)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	*hrx_status = HRX_STATUS_UNSTABLE;
	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev_glbl->dev);

	while (1) {
		/* Give up if we are forcing off */
		if (hrx_dev->force_off)
			goto EXIT;

		if (*hrx_status == HRX_STATUS_VIP_STABLE)
			return 0;

		switch (*hrx_status) {
		case HRX_STATUS_UNSTABLE:
			if (hrx_is_5v_connected(hrx_dev)) {
				*hrx_status = HRX_STATUS_HPD_STABLE;
				break;
			} else
				goto EXIT;
		case HRX_STATUS_HPD_STABLE:
			if (hrx_dev->vip_status == VIP_STATUS_START) {
				*hrx_status = HRX_STATUS_VIP_STABLE;
				break;
			} else
				goto EXIT;
		case HRX_STATUS_VIP_STABLE:
			break;
		default:
			HRX_LOG(AIP_ERROR, "Invalid status\n");
			break;
		}
	}

EXIT:
	hrx_status = HRX_STATUS_UNSTABLE;
	return 0;
}

static const struct alsa_aip_ops a2a_ops = {
	.get_hrx_status = aip_alsa_get_hrx_status,
	.config = aip_alsa_config,
	.start = aip_alsa_start,
	.stop = aip_alsa_stop,
	.close = aip_alsa_close,
};

int syna_hrx_audio_init(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(AIP_INFO, "init audio\n");
	aip_alsa_set_ops(&a2a_ops);

	return 0;
}

void syna_hrx_audio_exit(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(AIP_INFO, "exiting audio\n");
	aip_alsa_set_ops(NULL);
}
